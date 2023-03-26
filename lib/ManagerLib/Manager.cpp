//
// Created by yu on 4/18/21.
//

#include "Manager.h"

#include <utility>
#include "../ToolLib/log.h"
#include "../ToolLib/llvm_related.h"
#include "../TemplateLib/define.h"

#include "../DIFUZE/EntryPointIdentifier.cpp"
#include "../DIFUZE/DeviceNameFinder.cpp"
#include "../DIFUZE/NewIoctlCmdParser.cpp"

#include "../DIFUZE_FIX/EntryPointIdentifier.cpp"
#include "../DIFUZE_FIX/DeviceNameFinder.cpp"
#include "../DIFUZE_FIX/NewIoctlCmdParser.cpp"


lddt::Manager::Manager() {
    t_module = new T_module();
}

void lddt::Manager::test() {
    std::string str;
    int64_t ret;
    yhao_log(1, "test start");

    yhao_log(1, "test end");
}

void lddt::Manager::start(std::string _bitcode) {
    this->bitcode = _bitcode;
    yhao_log(1, "bitcode: " + bitcode);
    this->t_module->read_bitcode(bitcode);
}

void lddt::Manager::test_difuze() {
    nlohmann::json j1;
    yhao_log(1, "start difuze");
    yhao_log(0, "start difuze EntryPointIdentifier");
    IOCTL_CHECKER::difuze_EntryPointIdentifier(this->t_module->llvm_module.get(), &j1);
    yhao_log(0, "end difuze EntryPointIdentifier");
    for (auto &temp_j: j1) {
        if (!temp_j.contains(ENTRY_POINT_TYPE) || !temp_j[ENTRY_POINT_TYPE].is_string()) {
            continue;
        }
        //std::string type = temp_j[ENTRY_POINT_TYPE];
        //if (type.find(IOCTL_HDR) == std::string::npos) {
        //    continue;
        //}
        if (!temp_j.contains(ENTRY_POINT)) {
            continue;
        }
        std::string function_name = temp_j[ENTRY_POINT];

        yhao_log(0, "start difuze DeviceNameFinderPass");
        IOCTL_CHECKER::DeviceNameFinderPass temp1;
        temp1.set_function_name(function_name);
        temp1.set_result_json(&temp_j[DEVICE_FILE_NAME]);
        temp1.runOnModule(*this->t_module->llvm_module);
        yhao_log(0, "end difuze DeviceNameFinderPass");

        yhao_log(0, "start difuze IoctlCmdCheckerPass");
        IOCTL_CHECKER::IoctlCmdCheckerPass temp2;
        temp2.set_function_name(function_name);
        temp2.set_result_json(&temp_j[CMD_TYPE]);
        temp2.runOnModule(*this->t_module->llvm_module);
        yhao_log(0, "end difuze IoctlCmdCheckerPass");

        yhao_log(0, "difuze result: " + temp_j.dump(2));
    }
    yhao_log(1, "end difuze");
    yhao_log(1, j1.dump(2));
    yhao_log(1, "difuze result: \n" + format_result_json(&j1)->dump(2));
}

void lddt::Manager::test_difuze_fix() {
    nlohmann::json j2;
    yhao_log(1, "start difuze fix");
    yhao_log(0, "start difuze fix EntryPointIdentifier");
    IOCTL_CHECKER_FIX::difuze_EntryPointIdentifier(this->t_module->llvm_module.get(), &j2);
    yhao_log(0, "end difuze fix EntryPointIdentifier");
    for (auto &temp_j: j2) {
        if (!temp_j.contains(ENTRY_POINT_TYPE) || !temp_j[ENTRY_POINT_TYPE].is_string()) {
            continue;
        }
        //std::string type = temp_j[ENTRY_POINT_TYPE];
        //if (type.find(IOCTL_HDR) == std::string::npos) {
        //    continue;
        //}
        if (!temp_j.contains(ENTRY_POINT)) {
            continue;
        }
        std::string function_name = temp_j[ENTRY_POINT];

        yhao_log(0, "start difuze fix DeviceNameFinderPass");
        IOCTL_CHECKER_FIX::DeviceNameFinderPass temp1;
        temp1.set_function_name(function_name);
        temp1.set_result_json(&temp_j[DEVICE_FILE_NAME]);
        temp1.runOnModule(*this->t_module->llvm_module);
        yhao_log(0, "end difuze fix DeviceNameFinderPass");

        yhao_log(0, "start difuze fix IoctlCmdCheckerPass");
        IOCTL_CHECKER_FIX::IoctlCmdCheckerPass temp2;
        temp2.set_function_name(function_name);
        temp2.set_result_json(&temp_j[CMD_TYPE]);
        temp2.runOnModule(*this->t_module->llvm_module);
        yhao_log(0, "end difuze fix IoctlCmdCheckerPass");

        yhao_log(0, "difuze fix result: " + temp_j.dump(2));
    }
    yhao_log(1, "end difuze fix");
    yhao_log(1, j2.dump(2));
    yhao_log(1, "difuze fix result: \n" + format_result_json(&j2)->dump(2));
}

nlohmann::json *lddt::Manager::format_result_json(nlohmann::json *input_json) {
    int64_t debug = -1;
    std::string str;
    assert(input_json != nullptr && (input_json->empty() || input_json->is_array()));
    std::vector<nlohmann::json *> result_json_vector;
    for (auto ep_json: *input_json) {

        if (ep_json.contains(ENTRY_POINT_TYPE) && ep_json[ENTRY_POINT_TYPE].is_string()) {
            std::string entry_point_type = ep_json[ENTRY_POINT_TYPE].get<std::string>();
            if (entry_point_type == IOCTL_HDR || entry_point_type == WRITE_HDR || entry_point_type == READ_HDR) {
            } else {
                continue;
            }
        }

        // find the ops with the same parent and the ops name
        if (ep_json.contains(OPS_NAME) && ep_json[OPS_NAME].is_string()) {
        } else {
            continue;
        }
        std::string ops_name = ep_json[OPS_NAME].get<std::string>();
        std::string parent;
        if (ep_json.contains(PARENT) && ep_json[PARENT].is_string()) {
            parent = ep_json[PARENT].get<std::string>();
        }
        bool has_parent_point = false;
        if ((ep_json).contains(PARENT_POINT) && !(ep_json)[PARENT_POINT].empty()) {
            has_parent_point = true;
        }
        nlohmann::json *ops_json = nullptr;
        for (auto temp2_json: result_json_vector) {
            std::string temp_parent;
            if (temp2_json->contains(PARENT) && (*temp2_json)[PARENT].is_string()) {
                temp_parent = (*temp2_json)[PARENT].get<std::string>();
            }
            if (parent == temp_parent) {
            } else {
              if (has_parent_point || ((*temp2_json).contains(PARENT_POINT) &&
                                       !(*temp2_json)[PARENT_POINT].empty())) {
                continue;
              }
            }

            if ((*temp2_json).contains(OPS_NAME) && (*temp2_json)[OPS_NAME].is_string()) {
            } else {
                continue;
            }
            if ((*temp2_json)[OPS_NAME] == ops_name) {
                ops_json = temp2_json;
                break;
            }
        }

        // set fields if it is a new entry point
        bool new_json = false;
        if (ops_json == nullptr) {
            ops_json = new nlohmann::json();
            new_json = true;
        }
        if (new_json) {
            (*ops_json)[OPS_NAME] = ops_name;
            if (ep_json.contains(SOURCE_FILE) && ep_json[SOURCE_FILE].is_string()) {
                (*ops_json)[SOURCE_FILE] = ep_json[SOURCE_FILE];
            }
            if (ep_json.contains(PARENT) && ep_json[PARENT].is_string()) {
                (*ops_json)[PARENT] = ep_json[PARENT];
            }
            (*ops_json)[DEVICE_FILE_NAME] = {};
            result_json_vector.push_back(ops_json);
        }
        // merge all name
        std::set<std::string> name_set;
        for (const auto &name: (*ops_json)[DEVICE_FILE_NAME]) {
            if (name.is_string()) {
                name_set.insert(name.get<std::string>());
            } else {
                yhao_log(3, "DEVICE_FILE_NAME !name.is_string()");
            }
        }
        if (ep_json.contains(DEVICE_FILE_NAME) && ep_json[DEVICE_FILE_NAME].is_array()) {
            for (const auto &name: ep_json[DEVICE_FILE_NAME]) {
                if (name.is_object()) {
                    for (const auto &name1: name) {
                        if (name1.is_string()) {
                            name_set.insert(name1.get<std::string>());
                        } else {
                            yhao_log(3, "DEVICE_FILE_NAME !name1.is_string()");
                        }
                    }
                } else {
                    yhao_log(3, "DEVICE_FILE_NAME !name.is_object()");
                }
            }
        }
        (*ops_json)[DEVICE_FILE_NAME] = {};
        for (auto name: name_set) {
            // remove "" and "%s"
            if (name.empty() || name == "%s") {
                continue;
            }
            // remove name with multiply '%'
            uint64_t temp_number = 0;
            for (auto c: name) {
                if (c == '%') {
                    temp_number++;
                }
            }
            if (temp_number > 2) {
                continue;
            }
            (*ops_json)[DEVICE_FILE_NAME].push_back(name);
        }
        // merge all parent point
        std::set<std::string> parent_set;
        if ((*ops_json).contains(PARENT_POINT) && (*ops_json)[PARENT_POINT].is_array()) {
            for (const auto &name: (*ops_json)[PARENT_POINT]) {
                if (name.is_string()) {
                    parent_set.insert(name.get<std::string>());
                } else {
                    yhao_log(3, "PARENT_POINT !name.is_string()");
                }
            }
        }
        if (ep_json.contains(PARENT_POINT) && ep_json[PARENT_POINT].is_array()) {
            for (const auto &name: ep_json[PARENT_POINT]) {
                if (name.is_string()) {
                    parent_set.insert(name.get<std::string>());
                } else {
                    yhao_log(3, "PARENT_POINT !name.is_string()");
                }
            }
        }
        (*ops_json)[PARENT_POINT] = {};
        for (auto name: parent_set) {
            (*ops_json)[PARENT_POINT].push_back(name);
        }

        if (ep_json.contains(ENTRY_POINT) && ep_json[ENTRY_POINT].is_string()) {
        } else {
            continue;
        }
        if (ep_json.contains(ENTRY_POINT_TYPE) && ep_json[ENTRY_POINT_TYPE].is_string()) {
        } else {
            continue;
        }
        std::string entry_point_type = ep_json[ENTRY_POINT_TYPE].get<std::string>();
        if (entry_point_type == IOCTL_HDR || entry_point_type == WRITE_HDR || entry_point_type == READ_HDR) {
        } else {
            continue;
        }
        if (ops_json->contains(entry_point_type)) {
        } else {
            (*ops_json)[entry_point_type] = nlohmann::json();;
        }
        auto &temp2_json = (*ops_json)[entry_point_type];
        if (ep_json.contains(ENTRY_POINT)) {
            temp2_json[ENTRY_POINT] = ep_json[ENTRY_POINT];
        }
        yhao_log(-1, "format_result_json CMD_TYPE ep_json:" + ep_json.dump(2));
        yhao_log(-1, "format_result_json CMD_TYPE before:" + ops_json->dump(2));
        if (ep_json.contains(CMD_TYPE)) {
            if (ep_json[CMD_TYPE].empty()) {

            } else {
                temp2_json[CMD_TYPE] = ep_json[CMD_TYPE];
            }
        }
        yhao_log(-1, "format_result_json CMD_TYPE after:" + ops_json->dump(2));
    }

    auto *result_json = new nlohmann::json();
    for (auto j: result_json_vector) {
        result_json->push_back(*j);
    }

    // format type
    for (auto &ops_json: *result_json) {
        std::set<std::string> types_set;
        if (ops_json.contains(IOCTL_HDR) && ops_json[IOCTL_HDR].contains(CMD_TYPE)) {
            auto &cmd_json = ops_json[IOCTL_HDR][CMD_TYPE];
            yhao_log(debug, cmd_json.dump());
            uint64_t type_num = 0;
            for (auto &cmd: cmd_json) {
                std::set<std::string> ts;
                for (const auto &type: cmd) {
                    if (type.is_string()) {
                        auto type_str = type.get<std::string>();
                        yhao_log(debug, "type_str: " + type_str);
                        if (type_str.find(" = ") != std::string::npos) {
                            yhao_log(debug, "type_str: = " + type_str);
                            type_str = type_str.substr(0, type_str.find(" = "));
                            // if (type_str.starts_with("%struct.") || type_str.starts_with("%union.")) {
                            if (std::string temp0 = "%struct."; type_str.size() >= temp0.size() &&
                                                                type_str.compare(0, temp0.size(), temp0) == 0) {
                                types_set.insert(type_str);
                                yhao_log(debug, "types_set.insert: " + type_str);
                            } else if (std::string temp1 = "%union."; type_str.size() >= temp1.size() &&
                                                                      type_str.compare(0, temp1.size(), temp1) == 0) {
                                types_set.insert(type_str);
                                yhao_log(debug, "types_set.insert: " + type_str);
                            } else {
                                assert(0 && "not handle");
                            }
                            ts.insert(type_str);
                            //} else if (type_str.starts_with(START_BB)) {
                        } else if (type_str.find(START_BB) != std::string::npos) {
                            yhao_log(debug, "type_str: start_bb$ " + type_str);
                            // remove "start_bb$"
                        } else {
                            yhao_log(debug, "type_str: else " + type_str);
                            ts.insert(type_str);
                        }
                    } else if (type.is_array()) {
                        // for "E_no_cmd_type"
                    }
                }
                cmd.clear();
                for (auto type: ts) {
                    cmd.push_back(type);
                }
                for (const auto &type: ts) {
                    if (type != "i64") {
                        type_num++;
                        break;
                    }
                }
            }
            ops_json[IOCTL_HDR][CMD_NUM] = cmd_json.size();
            ops_json[IOCTL_HDR][TYPE_NUM] = type_num;
        }
        std::set<std::string> all_types_set;
        add_type_set(all_types_set, types_set);
        for (auto type: all_types_set) {
            ops_json[ALL_TYPES].push_back(type);
        }
    }
    std::ofstream json_ofstream("result.json");
    json_ofstream << result_json->dump(2);
    json_ofstream.close();
    return result_json;
}

void lddt::Manager::add_type_set(std::set<std::string> &all_types_set, std::set<std::string> &types_set) {
    std::string str;
    std::set<llvm::StructType *> new_type_set;
    auto struct_type = this->t_module->llvm_module->getIdentifiedStructTypes();
    for (const auto &type: types_set) {
        bool find = false;
        llvm::StructType *st;
        for (auto temp_st: struct_type) {
            if (type == "%" + temp_st->getName().str()) {
                find = true;
                st = temp_st;
                break;
            }
        }
        if (find) {
            yhao_print(4, st->print, str)
            all_types_set.insert(str);
            for (auto et: st->elements()) {
                et = get_real_type(et);
                if (et->isStructTy()) {
                    yhao_print(4, et->print, str)
                    yhao_log(-1, "add_type_set1: " + str);
                    if (all_types_set.find(str) == all_types_set.end()) {
                        new_type_set.insert(llvm::dyn_cast<llvm::StructType>(et));
                    }
                }
            }
        } else {
            str = "not find type" + type;
            assert(0 && str.c_str());
        }
    }
    add_type_set(all_types_set, new_type_set);
}

void lddt::Manager::add_type_set(std::set<std::string> &all_types_set, set<llvm::StructType *> &types_set) {
    std::string str;
    std::set<llvm::StructType *> new_type_set;
    for (const auto &type: types_set) {
        yhao_print(4, type->print, str)
        all_types_set.insert(str);
        for (auto et: type->elements()) {
            et = get_real_type(et);
            if (et->isStructTy()) {
                yhao_print(4, et->print, str)
                if (all_types_set.find(str) == all_types_set.end()) {
                    new_type_set.insert(llvm::dyn_cast<llvm::StructType>(et));
                }
            }
        }
    }
    if (new_type_set.empty()) {
        return;
    }
    add_type_set(all_types_set, new_type_set);
}

llvm::Type *lddt::Manager::get_real_type(llvm::Type *t) {
    if (t->isPointerTy()) {
        return get_real_type(t->getPointerElementType());
    } else if (t->isArrayTy()) {
        return get_real_type(t->getArrayElementType());
    } else if (t->isVectorTy()) {
        return get_real_type(t->getScalarType());
    }
    return t;
}