import copy
import json
from operator import index
import os
from pathlib import Path
import re
import sys

from enum import IntEnum
from tkinter import ALL
from typing import Any, Dict, List, Optional, Pattern

THIS_TYPES: Dict[str, "StructType"] = {}
ALL_TYPES: Dict[str, "StructType"] = {}
ALL_SYS: List[str] = []


def int2bytes(value, size):
    ret = []
    for _ in range(size):
        ret.append((value & 0xff))
        value = value >> 8
    return ret


def size_2_type(size):
    if size == 0:
        return "int64"
    if size == 1:
        return "int1"
    if size == 8:
        return "int8"
    if size == 16:
        return "int16"
    if size == 32:
        return "int32"
    if size == 64:
        return "int64"
    return f"array[int8, {size // 8}]"


ARRAY_REGEX: Pattern[str] = re.compile(rf"\[(?P<num>\d+) x (?P<type>.+)\]")


class Context:
    def __init__(self) -> None:
        self.path: List[int] = []
        self.dir: Dir


class BaseType:
    def __init__(self, data: Dict[str, Any], offset: int = 0, size: int = 0, typename: str = None) -> None:
        self.data = data
        self.name = None
        self.offset: int = offset
        self.size: int = size
        self.typename: str = typename
        # path to another field that this field relates to
        self.path: Optional[List[int]] = None

    @property
    def type(self) -> str:
        raise NotImplementedError()

    def get_typename(self, syscall: "Syscall"):
        if not self.typename:
            self.typename = syscall.assign_new_name(self.type)
        return self.typename

    def copy(self) -> "BaseType":
        return copy.deepcopy(self)

    def repr(self, indent=0) -> str:
        ret = " " * indent + self.type + " " + str(self.size) + "\n"
        return ret

    @staticmethod
    def construct(data: str, offset: int = 0):
        # pointer
        if data[len(data) - 1] == '*':
            return PtrType({"ref": data[:len(data) - 1]}, offset=offset)
        # int
        if data in ["i1", "i8", "i16", "i32", "i64", "intptr"]:
            if data == "intptr":
                size = -1
            else:
                size = int(data[1:])
            return BufferType({}, size=size)
        # array
        if data[0] == '[':
            m = ARRAY_REGEX.match(data)
            if m is None:
                raise RuntimeError(f"failed to match {data}")
            num = int(m.group("num"))
            subtype = m.group("type")
            return ArrayType({"num": num, "ref": subtype}, offset=offset)

        # struct
        if data.startswith("type {"):
            data = data[len("type {"):-1]
            fields = [each.strip() for each in data.split(",")]
            return StructType({"fields": fields}, offset=offset)

        # struct packed
        if data.startswith("type <{"):
            data = data[len("type <{"):-2]
            fields = [each.strip() for each in data.split(",")]
            return StructType({"fields": fields}, is_packed=True, offset=offset)

        if data.startswith("%struct."):
            typename = data[len("%struct."):] + "_syzgen"
            typename = data[len("%struct."):] + "" + module_name
            typename = typename.replace(".", "_")
            if typename in THIS_TYPES:
                return THIS_TYPES[typename].copy()

        if data.startswith("%union."):
            typename = data[len("%union."):] + "_syzgen"
            typename = data[len("%union."):] + module_name
            typename = typename.replace(".", "_")
            if typename in THIS_TYPES:
                return THIS_TYPES[typename].copy()

        raise RuntimeError(f"unknown type {data}")

    def visit(self, ctx: Context, func):
        return func(ctx, self)

    def generate_template(self, syscall: "Syscall", temp_dir: "Dir", f, depth=0):
        raise NotImplementedError()


class Dir(IntEnum):
    Default = 0
    DirIn = 1
    DirOut = 2
    DirInOut = 3


class PtrType(BaseType):
    DirStr = ["in", "in", "out", "inout"]  # default: in

    def __init__(self, data: Dict[str, Any], offset: int = 0, size: int = 0, typename: str = None) -> None:
        super().__init__(data, offset=offset, size=size, typename=typename)

        self.dir: Dir = Dir.Default
        self.ref: Optional[BaseType] = BaseType.construct(data["ref"])

    @property
    def type(self) -> str:
        return "ptr"

    def visit(self, ctx: Context, func):
        if func(ctx, self):
            return True

        old_dir = ctx.dir
        ctx.dir = self.dir if self.dir else ctx.dir
        ctx.path.append(0)
        ret = self.ref.visit(ctx, func)
        ctx.path.pop()
        ctx.dir = old_dir
        return ret

    def generate_template(self, syscall: "Syscall", temp_dir: "Dir", f, depth=0):
        temp_dir = self.dir or temp_dir
        typ, _ = self.ref.generate_template(
            syscall, temp_dir, f, depth=depth + 1)
        return f"ptr[{PtrType.DirStr[temp_dir]}, {typ}]", self.get_typename(syscall)


class BufferType(BaseType):
    def __init__(self, data: Dict[str, Any], offset: int = 0, size: int = 0, typename: str = None) -> None:
        self.data: List[int] = data["data"] if "data" in data else [0] * size

        super().__init__(data, offset=offset, size=size, typename=typename)

    @property
    def type(self) -> str:
        return "buffer"

    def generate_template(self, syscall: "Syscall", dir: "Dir", f, depth=0):
        name = self.get_typename(syscall)
        if self.size == -1:
            return "intptr", name
        if self.size == 0:
            return "array[int8]", name
            # return "int64", name
        return size_2_type(self.size), name


class ConstType(BufferType):
    def __init__(self, data: Dict[str, Any], offset: int = 0, size: int = 0,
                 typename: str = None) -> None:
        super().__init__(data, offset=offset, size=size, typename=typename)

    @property
    def type(self) -> str:
        return "const"

    def get_data(self):
        return int.from_bytes(self.data["data"], "little")

    def generate_template(self, syscall: "Syscall", temp_dir: "Dir", f, depth=0):
        if self.size <= 8:
            if depth == 0:
                return f"const[{self.get_data()}]", self.get_typename(syscall)
            return f"const[{self.get_data(), size_2_type(self.size)}]", self.get_typename(syscall)
        raise RuntimeError()


class ResourceType(BaseType):
    def __init__(self, data: Dict[str, Any], offset: int = 0, size: int = 0, typename: str = None) -> None:
        super().__init__(data, offset=offset, size=size, typename=typename)

        self.name = data["name"] if "name" in data else None
        self.parent = data["parent"] if "parent" in data else None

    @property
    def type(self) -> str:
        return "resource"

    def generate_template(self, syscall: "Syscall", dir: "Dir", f, depth=0):
        return self.name, self.get_typename(syscall)


class StructType(BaseType):
    def __init__(self, data: Dict[str, Any], is_packed: bool = False, offset: int = 0, size: int = 0, typename: str = None) -> None:
        super().__init__(data, offset=offset, size=size, typename=typename)
        self.fields: List[BaseType] = []
        self.is_union = False
        self.is_packed = is_packed
        self.count = 0

    def real_init(self):
        for field in self.data["fields"]:
            self.fields.append(BaseType.construct(field, self.offset))
            self.size += self.fields[-1].size

    @property
    def type(self) -> str:
        return "struct"

    def visit(self, ctx: Context, func):
        if func(ctx, self):
            return True
        for i, field in enumerate(self.fields):
            ctx.path.append(i)
            ret = field.visit(ctx, func)
            ctx.path.pop()
            if ret:
                return True

    def generate_template(self, syscall: "Syscall", dir: "Dir", f, depth=0):
        if self.typename in THIS_TYPES:
            return self.typename, self.typename

        return self.generate_template_force(syscall, dir, f, depth=depth)

    def generate_template_force(self, syscall: "Syscall", dir: "Dir", f, depth=0):
        index = 0
        typename = self.get_typename(syscall)
        if self.is_union:
            definition = "%s [\n" % self.typename
        else:
            definition = "%s {\n" % self.typename

        for field in self.fields:
            typ, name = field.generate_template(
                syscall, dir, f, depth=depth + 1)
            name = name + str(index)
            index = index + 1
            if self.typename in typ:
                typ = typ.replace("]", ", opt]")
            definition += f"    {name}  {typ}\n"
        if self.is_union:
            definition += "]"
        else:
            definition += "}"

        if self.is_packed:
            definition += "[packed]"
        self.definition = definition

        f.write(definition + "\n")
        return self.typename, typename


class ArrayType(BaseType):
    def __init__(self, data: Dict[str, Any], offset: int = 0, size: int = 0, typename: str = None) -> None:
        super().__init__(data, offset=offset, size=size, typename=typename)

        self.num: int = data["num"] if "num" in data else 0
        self.ref: BaseType = BaseType.construct(data["ref"])

        self.size = self.num * self.ref.size

    @property
    def type(self) -> str:
        return "array"

    def generate_template(self, syscall: "Syscall", dir: "Dir", f, depth=0):
        subtype, _ = self.ref.generate_template(
            syscall, dir, f, depth=depth + 1)
        if self.num == 0:
            return f"array[{subtype}]", self.get_typename(syscall)
        return f"array[{subtype}, {self.num}]", self.get_typename(syscall)


class Syscall:
    def __init__(self, callName, subName) -> None:
        self.CallName = callName
        self.SubName = subName
        self.final_name = ""

        self.args: List[BaseType] = []
        self.ret: Optional[ResourceType] = None
        self.arg_names: List[str] = []

        self._counter = 0

    @property
    def Name(self) -> str:
        global open_index
        if self.final_name:
            return self.final_name
        if self.SubName:
            open_index = open_index + 1
            self.final_name = f"{self.CallName}${self.SubName}_{open_index}"
            return self.final_name
        return self.CallName

    def validate(self):
        if self.arg_names and len(self.arg_names) == len(self.args):
            for i, arg in enumerate(self.args):
                arg.typename = self.arg_names[i]
        return True

    def assign_new_name(self, prefix: str) -> str:
        self._counter += 1
        return f"{self.SubName}_{prefix}_{self._counter}"

    def visit(self, ctx: Context, func):
        for i, arg in enumerate(self.args):
            ctx.path.append(i)
            ctx.dir = Dir.DirIn
            ret = arg.visit(ctx, func)
            ctx.path.pop()
            if ret:
                return

        if self.ret:
            ctx.path.append(-1)
            ctx.dir = Dir.DirOut
            self.ret.visit(ctx, func)
            ctx.path.pop()

    def generate_template(self, f):
        args = []
        for arg in self.args:
            typ, name = arg.generate_template(self, Dir.DirIn, f)
            args.append(f"{name} {typ}")
        # print("generate_template: " + str(args))
        f.write(
            f'{self.Name}({", ".join(args)}) {self.ret.name if self.ret else ""}\n')


class SYZDevOpen(Syscall):
    """syz_open_dev
    e.g., syz_open_dev$mouse(dev ptr[in, string["/dev/input/mouse#"]], id intptr,
    flags flags[open_flags]) fd
    """

    def __init__(self, subName: str, dev_file: str) -> None:
        super().__init__("syz_open_dev", f"{subName}_syzgen")

        self.dev_file = dev_file
        self.ret = ResourceType(
            {"name": f"{subName}_fd", "parent": "fd"},
            size=4,
            typename="fd",
        )

    def generate_template(self, f):
        args = [
            f'dev ptr[in, string["{self.dev_file}"]]',
            "id intptr",
            "flags flags[open_flags]",
        ]
        # syscallname "(" [arg ["," arg]*] ")" [type]
        f.write(
            f'{self.Name}({", ".join(args)}) {self.ret.name if self.ret else ""}\n')


class IOCTLOpen(Syscall):
    """openat to open dev
    e.g., openat$ptmx(fd const[AT_FDCWD], file ptr[in, string["/dev/ptmx"]],
    flags flags[open_flags], mode const[0]) fd_tty
    """

    def __init__(self, subName: str, dev_file: str):
        super().__init__("openat", f"{subName}_syzgen")

        self.dev_file = dev_file
        self.ret = ResourceType(
            {"name": f"{subName}_fd", "parent": "fd"},
            size=4,
            typename="fd",
        )

    def generate_template(self, f):
        args = [
            "fd const[AT_FDCWD]",
            f'file ptr[in, string["{self.dev_file}"]]',
            "flags flags[open_flags]",
            "mode const[0]",
        ]
        # syscallname "(" [arg ["," arg]*] ")" [type]
        f.write(
            f'{self.Name}({", ".join(args)}) {self.ret.name if self.ret else ""}\n')


class IOCTLMethod(Syscall):
    """Syscall template for ioctl in Linux
    long ioctl(fd fd, unsigned int cmd, unsigned long arg)
    """

    def __init__(self, subName: str, fd: ResourceType, cmd: int, data: Optional[str]) -> None:
        super().__init__("ioctl", subName)

        self.args.append(fd.copy())
        self.args.append(
            ConstType({"data": cmd.to_bytes(4, 'little')}, typename="cmd"))
        # self.args.append(ConstType({"data": int2bytes(cmd, 4)}, typename="cmd"))
        if data is None or data.startswith("i"):
            self.args.append(BaseType.construct("intptr"))
        else:
            self.args.append(BaseType.construct(f"{data}*"))

        self.arg_names = ["fd", "cmd", "arg"]
        self.validate()
        self.index = index


class device_driver:
    def __init__(self, data: json):
        self.data = data

    def run(self) -> None:
        global module_name
        global open_index
        open_index = 0
        if "A_parent" in self.data:
            module_name = self.data["A_parent"] + "_"
        else:
            module_name = ""
        module_name = "_" + module_name + self.data["A_ops_name"]
        module_name = module_name.replace(".", "_")

        THIS_TYPES.clear()
        if "all_types" in self.data:
            for each in self.data["all_types"]:
                name, typ = each.split("=")
                name = name.strip()
                if name.startswith("%struct."):
                    typename = name[len("%struct."):] + "_syzgen"
                    typename = name[len("%struct."):] + module_name
                    typename = typename.replace(".", "_")
                    THIS_TYPES[typename] = BaseType.construct(typ.strip())
                    THIS_TYPES[typename].typename = typename
                elif name.startswith("%union."):
                    typename = name[len("%union."):] + "_syzgen"
                    typename = name[len("%union."):] + module_name
                    typename = typename.replace(".", "_")
                    THIS_TYPES[typename] = BaseType.construct(typ.strip())
                    THIS_TYPES[typename].is_union = True
                    THIS_TYPES[typename].typename = typename
                else:
                    RuntimeError(f"unknown type {name}")
            for each in THIS_TYPES.values():
                each.real_init()

        all_syscall: List[Syscall] = []
        is_continue = False
        if "C_device_file_name" in self.data and isinstance(self.data["C_device_file_name"], list):
            is_continue = True
            for dev_file in self.data["C_device_file_name"]:
                dev_file = "/dev/" + dev_file
                if "%d" in dev_file:
                    dev_file = dev_file.replace("%d", "#")
                    open_syscall = SYZDevOpen(module_name, dev_file)
                elif "%u" in dev_file:
                    dev_file = dev_file.replace("%u", "#")
                    open_syscall = SYZDevOpen(module_name, dev_file)
                else:
                    open_syscall = IOCTLOpen(module_name, dev_file)
                all_syscall.append(open_syscall)
        elif "A_parent_point" in self.data and isinstance(self.data["A_parent_point"], list):
            dev_file = "non-open"
            open_syscall = IOCTLOpen(module_name, dev_file)
        else:
            return

        if "IOCTL" not in self.data or not isinstance(self.data["IOCTL"], dict):
            return

        if "E_cmd_type" not in self.data["IOCTL"] or not isinstance(self.data["IOCTL"]["E_cmd_type"], dict):
            return

        ioctl = self.data["IOCTL"]["E_cmd_type"]
        for cmd, types in ioctl.items():
            cmd = int(cmd)
            if not isinstance(types, list) or len(types) == 0:
                all_syscall.append(IOCTLMethod(
                    f"{module_name}_{cmd:x}_0", open_syscall.ret, cmd, None))
            elif isinstance(types, list):
                for i, each in enumerate(types):
                    all_syscall.append(IOCTLMethod(
                        f"{module_name}_{cmd:x}_{i}",
                        open_syscall.ret, cmd, each
                    ))
        resources: Dict[str, ResourceType] = {}

        def search(ctx: Context, typ: BaseType) -> bool:
            if typ.type == "resource":
                resources[typ.name] = typ

        for syscall in all_syscall:
            syscall.visit(Context(), search)

        # each template
        name = path.name + "-" + module_name
        with open(name + ".each.txt", "w") as fp:
            for name, typ in resources.items():
                fp.write(
                    f"resource {name}[{typ.parent if typ.parent else size_2_type(typ.size)}]\n")
            fp.write("\n")

            dummy_sys = Syscall("", module_name)
            for name, typ in THIS_TYPES.items():
                typ.generate_template_force(dummy_sys, Dir.Default, fp)
                fp.write("\n")
                ALL_TYPES[name] = typ

            for syscall in all_syscall:
                syscall.generate_template(fp)

            fp.write("\n")
            fp.close()

        name = path.name + "-" + module_name
        with open(name + ".each.json", "w") as fp:
            sys = []
            for syscall in all_syscall:
                sys.append(syscall.Name)
            json.dump(sys, fp, indent=4)
            fp.close()

        # output all sys
        name = os.path.basename(path) + "-" + \
               path.parent.parent.name + "-" + path.parent.name
        with open(name + ".txt", "a") as fp:
            for name, typ in resources.items():
                fp.write(
                    f"resource {name}[{typ.parent if typ.parent else size_2_type(typ.size)}]\n")
            fp.write("\n")

            for syscall in all_syscall:
                syscall.generate_template(fp)
                ALL_SYS.append(syscall.Name)

            fp.write("\n")
            fp.close


cwd = os.getcwd()
path = Path(cwd)


def main(filepath):
    name = os.path.basename(path) + "-" + \
           path.parent.parent.name + "-" + path.parent.name
    with open(name + ".txt", "w") as fp1:
        fp1.write("\n")
        fp1.close()

    with open(filepath, "r") as fp:
        results = json.load(fp)
        fp.close

    if results == None:
        return

    for data in results:
        dd = device_driver(data)
        dd.run()

    name = os.path.basename(path) + "-" + \
           path.parent.parent.name + "-" + path.parent.name
    with open(name + ".json", "w") as fp1:
        json.dump(ALL_SYS, fp1, indent=4)
        fp1.close()
    with open(name + ".txt", "a") as fp1:
        for name, typ in ALL_TYPES.items():
            fp1.write(typ.definition + "\n")
        fp1.close()


if __name__ == '__main__':
    main(sys.argv[1])
