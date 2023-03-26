# Ported_DIFUZE

## Build
Requirements: Ubuntu 22.04
```shell
sudo apt install -y git llvm cmake
git clone https://github.com/ZHYfeng/2023-Ported_DIFUZE.git
cd 2023-Ported_DIFUZE
bash ./script/build.bash
```

## Run
```shell
build/tools/Difuze/Difuze --bitcode=built-in.bc
```
Or
```shell
build/tools/DifuzeFix/DifuzeFix --bitcode=built-in.bc
```

## Generate syzlang format syscall descriptions
```
python3 script/syzlang_gen.py results.json
```

## Linked LLVM Bitcode for Linux Kernel
refer to `https://github.com/ZHYfeng/Generate_Linux_Kernel_Bitcode/tree/master/v5.12`
> use `-save-temps` and `-g` to generate LLVM bitcode with debug info and less optimization
