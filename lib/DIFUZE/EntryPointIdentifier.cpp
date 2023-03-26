//
// Created by machiry on 1/30/17.
//

#include "../TemplateLib/define.h"
#include "../ToolLib/log.h"
#include "../ToolLib/llvm_related.h"
#include "../ToolLib/json.hpp"

using namespace std;
using namespace llvm;

namespace IOCTL_CHECKER {

    nlohmann::json *result_json;

    typedef struct {
        std::string st_name;
        long mem_id;
        std::string method_lab;
    } INT_STS;

    INT_STS kernel_sts[]{
            {"struct.watchdog_ops",                    9,  IOCTL_HDR},
            {"struct.bin_attribute",                   3,  READ_HDR},
            {"struct.bin_attribute",                   4,  WRITE_HDR},
            {"struct.atmdev_ops",                      3,  IOCTL_HDR},
            {"struct.atmphy_ops",                      1,  IOCTL_HDR},
            {"struct.atm_ioctl",                       1,  IOCTL_HDR},
            {"struct.kernfs_ops",                      4,  READ_HDR},
            {"struct.kernfs_ops",                      6,  WRITE_HDR},
            {"struct.ppp_channel_ops",                 1,  IOCTL_HDR},
            {"struct.hdlcdrv_ops",                     4,  IOCTL_HDR},
            {"struct.vfio_device_ops",                 3,  READ_HDR},
            {"struct.vfio_device_ops",                 4,  WRITE_HDR},
            {"struct.vfio_device_ops",                 5,  IOCTL_HDR},
            {"struct.vfio_iommu_driver_ops",           4,  READ_HDR},
            {"struct.vfio_iommu_driver_ops",           5,  WRITE_HDR},
            {"struct.vfio_iommu_driver_ops",           6,  IOCTL_HDR},
            {"struct.rtc_class_ops",                   2,  IOCTL_HDR},
            {"struct.net_device_ops",                  10, IOCTL_HDR},
            {"struct.kvm_device_ops",                  6,  IOCTL_HDR},
            {"struct.ide_disk_ops",                    8,  IOCTL_HDR},
            {"struct.ide_ioctl_devset",                0,  IOCTL_HDR},
            {"struct.ide_ioctl_devset",                1,  IOCTL_HDR},
            {"struct.pci_ops",                         0,  READ_HDR},
            {"struct.pci_ops",                         1,  WRITE_HDR},
            {"struct.cdrom_device_info",               10, IOCTL_HDR},
            {"struct.cdrom_device_ops",                12, IOCTL_HDR},
            {"struct.iio_chan_spec_ext_info",          2,  READ_HDR},
            {"struct.iio_chan_spec_ext_info",          3,  WRITE_HDR},
            {"struct.proto_ops",                       9,  IOCTL_HDR},
            {"struct.usb_phy_io_ops",                  0,  READ_HDR},
            {"struct.usb_phy_io_ops",                  1,  WRITE_HDR},
            {"struct.usb_gadget_ops",                  6,  IOCTL_HDR},
            {"struct.uart_ops",                        23, IOCTL_HDR},
            {"struct.tty_ldisc_ops",                   8,  READ_HDR},
            {"struct.tty_ldisc_ops",                   9,  WRITE_HDR},
            {"struct.tty_ldisc_ops",                   10, IOCTL_HDR},
            {"struct.fb_ops",                          17, IOCTL_HDR},
            {"struct.v4l2_subdev_core_ops",            13, IOCTL_HDR},
            {"struct.m2m1shot_devops",                 8,  IOCTL_HDR},
            {"struct.nfc_phy_ops",                     0,  WRITE_HDR},
            {"struct.snd_ac97_bus_ops",                2,  WRITE_HDR},
            {"struct.snd_ac97_bus_ops",                3,  READ_HDR},
            {"struct.snd_hwdep_ops",                   1,  READ_HDR},
            {"struct.snd_hwdep_ops",                   2,  WRITE_HDR},
            {"struct.snd_hwdep_ops",                   6,  IOCTL_HDR},
            {"struct.snd_hwdep_ops",                   7,  IOCTL_HDR},
            {"struct.snd_soc_component",               14, READ_HDR},
            {"struct.snd_soc_component",               15, WRITE_HDR},
            {"struct.snd_soc_codec_driver",            14, READ_HDR},
            {"struct.snd_soc_codec_driver",            15, WRITE_HDR},
            {"struct.snd_pcm_ops",                     2,  IOCTL_HDR},
            {"struct.snd_ak4xxx_ops",                  2,  WRITE_HDR},
            {"struct.snd_info_entry_text",             0,  READ_HDR},
            {"struct.snd_info_entry_text",             1,  WRITE_HDR},
            {"struct.snd_info_entry_ops",              2,  READ_HDR},
            {"struct.snd_info_entry_ops",              3,  WRITE_HDR},
            {"struct.snd_info_entry_ops",              6,  IOCTL_HDR},
            {"struct.tty_buffer",                      4,  READ_HDR},
            {"struct.tty_operations",                  7,  WRITE_HDR},
            {"struct.tty_operations",                  12, IOCTL_HDR},
            {"struct.posix_clock_operations",          10, IOCTL_HDR},
            {"struct.posix_clock_operations",          15, READ_HDR},
            {"struct.block_device_operations",         3,  IOCTL_HDR},
            {"struct.security_operations",             64, IOCTL_HDR},
            {"struct.file_operations",                 2,  READ_HDR},
            {"struct.file_operations",                 3,  WRITE_HDR},
            {"struct.file_operations",                 10, IOCTL_HDR},
            {"struct.efi_pci_io_protocol_access_32_t", 0,  READ_HDR},
            {"struct.efi_pci_io_protocol_access_32_t", 1,  WRITE_HDR},
            {"struct.efi_pci_io_protocol_access_64_t", 0,  READ_HDR},
            {"struct.efi_pci_io_protocol_access_64_t", 1,  WRITE_HDR},
            {"struct.efi_pci_io_protocol_access_t",    0,  READ_HDR},
            {"struct.efi_pci_io_protocol_access_t",    1,  WRITE_HDR},
            {"struct.efi_file_handle_32_t",            4,  READ_HDR},
            {"struct.efi_file_handle_32_t",            5,  WRITE_HDR},
            {"struct.efi_file_handle_64_t",            4,  READ_HDR},
            {"struct.efi_file_handle_64_t",            5,  WRITE_HDR},
            {"struct._efi_file_handle",                4,  READ_HDR},
            {"struct._efi_file_handle",                5,  WRITE_HDR},
            {"struct.video_device",                    21, IOCTL_HDR},
            {"struct.video_device",                    22, IOCTL_HDR},
            {"struct.v4l2_file_operations",            1,  READ_HDR},
            {"struct.v4l2_file_operations",            2,  WRITE_HDR},
            {"struct.v4l2_file_operations",            4,  IOCTL_HDR},
            {"struct.v4l2_file_operations",            5,  IOCTL_HDR},
            {"struct.media_file_operations",           1,  READ_HDR},
            {"struct.media_file_operations",           2,  WRITE_HDR},
            {"struct.media_file_operations",           4,  IOCTL_HDR},
            {"END",                                    0,  END_HDR}
    };

    std::string file_op_st("struct.file_operations");
    std::string dev_attr_st("struct.device_attribute");
    std::string dri_attr_st("struct.driver_attribute");
    std::string bus_attr_st("struct.bus_attribute");
    std::string net_dev_st("struct.net_device_ops");
    std::string snd_pcm_st("struct.snd_pcm_ops");
    std::string v4l2_ioctl_st("struct.v4l2_ioctl_ops");
    std::string v4l2_file_ops_st("struct.v4l2_file_operations");

    std::string getFunctionFileName(Function *targetFunc) {
        SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
        targetFunc->getAllMetadata(MDs);
        std::string targetFName = "";
        for (auto &MD: MDs) {
            if (MDNode *N = MD.second) {
                if (auto *subProgram = dyn_cast<DISubprogram>(N)) {
                    targetFName = subProgram->getFilename();
                }
            }
        }
        return targetFName;
    }

    void add_entry_point(std::string ops_struct, Function *targetFunction, std::string method_lab) {
        auto *temp_j_1 = new nlohmann::json();
        (*temp_j_1)[OPS_NAME] = ops_struct;
        (*temp_j_1)[ENTRY_POINT] = targetFunction->getName().str();
        (*temp_j_1)[SOURCE_FILE] = getFunctionFileName(targetFunction);
        (*temp_j_1)[ENTRY_POINT_TYPE] = method_lab;
        if (!result_json->empty()) {
            assert(result_json->is_array() && "add_entry_point");
        }
        (*result_json).push_back(*temp_j_1);
    }

    bool printFuncVal(std::string ops_struct, Value *currVal, const char *hdr_str) {
        Function *targetFunction = dyn_cast<Function>(currVal->stripPointerCasts());
        if (targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()) {
            yhao_log(0, "*****************************************");
            yhao_log(0, "difuze: find: " + std::string(hdr_str) + ": " + targetFunction->getName().str());
            yhao_log(0, "difuze: " + getFunctionFileName(targetFunction));
            yhao_log(0, "*****************************************");
            add_entry_point(std::move(ops_struct), targetFunction, std::string(hdr_str));
            return true;
        }
        return false;
    }

    void process_netdev_st(GlobalVariable *currGlobal) {
        if (currGlobal->hasInitializer()) {
            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                // net device ioctl: 10
                if (actualStType->getNumOperands() > 10) {
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(10), NETDEV_IOCTL);
                }
            }
        }
    }

    void process_device_attribute_st(GlobalVariable *currGlobal) {
        if (currGlobal->hasInitializer()) {

            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                if (actualStType->getNumOperands() > 1) {
                    // dev show: 1
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(1), DEVATTR_SHOW);
                }

                if (actualStType->getNumOperands() > 2) {
                    // dev store : 2
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(2), DEVATTR_STORE);
                }
            }

        }
    }

    void process_driver_attribute_st(GlobalVariable *currGlobal) {
        if (currGlobal->hasInitializer()) {

            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                if (actualStType->getNumOperands() > 1) {
                    // dev show: 1
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(1), DEVATTR_SHOW);
                }

                if (actualStType->getNumOperands() > 2) {
                    // dev store : 2
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(2), DEVATTR_STORE);
                }
            }

        }
    }

    inline bool ends_with(std::string const &value, std::string const &ending) {
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    void process_file_operations_st(GlobalVariable *currGlobal) {

        if (currGlobal->hasInitializer()) {

            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            bool ioctl_found = false, ioctl_found2 = false;
            if (actualStType != nullptr) {
                Value *currFieldVal;

                // read: 2
                if (actualStType->getNumOperands() > 2) {
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(2), READ_HDR);
                }

                // write: 3
                if (actualStType->getNumOperands() > 3) {
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(3), WRITE_HDR);
                }

                // ioctl : 10
                if (actualStType->getNumOperands() > 10) {
                    ioctl_found = printFuncVal(currGlobal->getName().str(), actualStType->getOperand(10), IOCTL_HDR);
                }

                // ioctl : 11
                if (actualStType->getNumOperands() > 11) {
                    ioctl_found2 = printFuncVal(currGlobal->getName().str(), actualStType->getOperand(11), IOCTL_COMPAT_HDR);
                }

                // ioctl function identification heuristic
                if (!ioctl_found || !ioctl_found2) {
                    unsigned int idx = 0;
                    std::string ioctlEnd = "_ioctl";
                    for (idx = 0; idx < actualStType->getNumOperands(); idx++) {
                        if (idx == 10 || idx == 8) {
                            continue;
                        }
                        currFieldVal = actualStType->getOperand(idx);
                        Function *targetFunction = dyn_cast<Function>(currFieldVal->stripPointerCasts());
                        if (targetFunction != nullptr && !targetFunction->isDeclaration() &&
                            targetFunction->hasName() &&
                            ends_with(targetFunction->getName().str(), ioctlEnd)) {
                            printFuncVal(currGlobal->getName().str(), currFieldVal, IOCTL_HDR);
                        }
                    }
                }
            }

        }
    }

    void process_snd_pcm_ops_st(GlobalVariable *currGlobal) {
        if (currGlobal->hasInitializer()) {
            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                if (actualStType->getNumOperands() > 2) {
                    // ioctl: 2
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(2), IOCTL_HDR);
                }
            }

        }
    }

    void process_v4l2_ioctl_st(GlobalVariable *currGlobal) {
        if (currGlobal->hasInitializer()) {

            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                // all fields are function pointers
                for (unsigned int i = 0; i < actualStType->getNumOperands(); i++) {
                    Value *currFieldVal = actualStType->getOperand(i);
                    Function *targetFunction = dyn_cast<Function>(currFieldVal);
                    if (targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()) {
                        yhao_log(0, "*****************************************");
                        yhao_log(0, "difuze: v4l2_ioctl: " + std::string(V4L2_IOCTL_FUNC) + ": " +
                                    targetFunction->getName().str());
                        yhao_log(0, "difuze: v4l2_ioctl: " + std::to_string(i) + ": " +
                                    getFunctionFileName(targetFunction));
                        yhao_log(0, "*****************************************");
                        add_entry_point(currGlobal->getName().str(), targetFunction, std::string(V4L2_IOCTL_FUNC));
                    }
                }
            }

        }
    }

    void process_v4l2_file_ops_st(GlobalVariable *currGlobal) {
        if (currGlobal->hasInitializer()) {
            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                // read: 1
                if (actualStType->getNumOperands() > 1) {
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(1), READ_HDR);
                }

                // write: 2
                if (actualStType->getNumOperands() > 2) {
                    printFuncVal(currGlobal->getName().str(), actualStType->getOperand(2), WRITE_HDR);
                }
            }

        }
    }

    char **str_split(char *a_str, const char a_delim) {
        char **result = 0;
        size_t count = 0;
        char *tmp = a_str;
        char *last_comma = 0;
        char delim[2];
        delim[0] = a_delim;
        delim[1] = 0;

        /* Count how many elements will be extracted. */
        while (*tmp) {
            if (a_delim == *tmp) {
                count++;
                last_comma = tmp;
            }
            tmp++;
        }

        /* Add space for trailing token. */
        count += last_comma < (a_str + strlen(a_str) - 1);

        /* Add space for terminating null string so caller
           knows where the list of returned strings ends. */
        count++;

        result = (char **) malloc(sizeof(char *) * count);

        if (result) {
            size_t idx = 0;
            char *token = strtok(a_str, delim);

            while (token) {
                assert(idx < count);
                *(result + idx++) = strdup(token);
                token = strtok(0, delim);
            }
            assert(idx == count - 1);
            *(result + idx) = 0;
        }

        return result;
    }

    bool process_struct_in_custom_entry_files(GlobalVariable *currGlobal, std::vector<string> &allentries) {
        bool retVal = false;
        if (currGlobal->hasInitializer()) {
            // get the initializer.
            Constant *targetConstant = currGlobal->getInitializer();
            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
            if (actualStType != nullptr) {
                Type *targetType = currGlobal->getType();
                assert(targetType->isPointerTy());
                Type *containedType = targetType->getContainedType(0);
                std::string curr_st_name = containedType->getStructName().str();
                char hello_str[1024];
                for (auto curre: allentries) {
                    if (curre.find(curr_st_name) != std::string::npos) {
                        strcpy(hello_str, curre.c_str());
                        // structure found
                        char **tokens = str_split(hello_str, ',');
                        assert(!strcmp(curr_st_name.c_str(), tokens[0]));
                        long ele_ind = strtol(tokens[1], NULL, 10);
                        if (actualStType->getNumOperands() > ele_ind) {
                            Value *currFieldVal = actualStType->getOperand(ele_ind);
                            Function *targetFunction = dyn_cast<Function>(currFieldVal);

                            if (targetFunction != nullptr && !targetFunction->isDeclaration() &&
                                targetFunction->hasName()) {
                                yhao_log(0, "*****************************************");
                                yhao_log(0,
                                         "difuze: custom_entry_files: " + std::string(tokens[2]) + ": " +
                                         targetFunction->getName().str());
                                yhao_log(0, "difuze: custom_entry_files: " + getFunctionFileName(targetFunction));
                                yhao_log(0, "*****************************************");
                                add_entry_point(currGlobal->getName().str(), targetFunction, std::string(tokens[2]));
                            }
                        }
                        if (tokens) {
                            int i;
                            for (i = 0; *(tokens + i); i++) {
                                free(*(tokens + i));
                            }
                            free(tokens);
                        }
                        retVal = true;
                    }
                }
            }

        }
        return retVal;
    }

    void process_global(GlobalVariable *currGlobal, std::vector<string> &allentries) {

        Type *targetType = currGlobal->getType();
        assert(targetType->isPointerTy());
        Type *containedType = targetType->getContainedType(0);
        if (containedType->isStructTy()) {
            StructType *targetSt = dyn_cast<StructType>(containedType);
            if (targetSt->isLiteral()) {
                return;
            }
            if (process_struct_in_custom_entry_files(currGlobal, allentries)) {
                return;
            }
            if (containedType->getStructName() == file_op_st) {
                process_file_operations_st(currGlobal);
            } else if (containedType->getStructName() == dev_attr_st) {
                process_device_attribute_st(currGlobal);
            } else if (containedType->getStructName() == dri_attr_st) {
                process_driver_attribute_st(currGlobal);
            } else if (containedType->getStructName() == net_dev_st) {
                process_netdev_st(currGlobal);
            } else if (containedType->getStructName() == snd_pcm_st) {
                process_snd_pcm_ops_st(currGlobal);
            } else if (containedType->getStructName() == v4l2_file_ops_st) {
                process_v4l2_file_ops_st(currGlobal);
            } else if (containedType->getStructName() == v4l2_ioctl_st) {
                process_v4l2_ioctl_st(currGlobal);
            } else {
                unsigned long i = 0;
                while (kernel_sts[i].method_lab != END_HDR) {
                    if (kernel_sts[i].st_name == containedType->getStructName()) {
                        if (currGlobal->hasInitializer()) {
                            Constant *targetConstant = currGlobal->getInitializer();
                            ConstantStruct *actualStType = dyn_cast<ConstantStruct>(targetConstant);
                            Value *currFieldVal = actualStType->getOperand(kernel_sts[i].mem_id);
                            Function *targetFunction = dyn_cast<Function>(currFieldVal);
                            if (targetFunction != nullptr && !targetFunction->isDeclaration() &&
                                targetFunction->hasName()) {
                                yhao_log(0, "*****************************************");
                                yhao_log(0, "difuze: currGlobal->getName().str(): " + targetFunction->getName().str());
                                yhao_log(0, "difuze: kernel_sts[i].method_lab: " + kernel_sts[i].method_lab);
                                yhao_log(0, "difuze: kernel_sts[i].mem_id: " + std::to_string(kernel_sts[i].mem_id));
                                yhao_log(0, "difuze: ops structure: " + currGlobal->getName().str());
                                yhao_log(0, "difuze: find ioctl" + getFunctionFileName(targetFunction));
                                yhao_log(0, "*****************************************");
                                add_entry_point(currGlobal->getName().str(), targetFunction, std::string(kernel_sts[i].method_lab));
                            } else {
                                yhao_log(0, "*****************************************");
                                yhao_log(0,
                                         "difuze: targetFunction != nullptr && !targetFunction->isDeclaration() && targetFunction->hasName()");
                                yhao_log(0, "difuze: currGlobal->getName().str(): " + currGlobal->getName().str());
                                yhao_log(0, "difuze: kernel_sts[i].method_lab: " + kernel_sts[i].method_lab);
                                yhao_log(0, "difuze: kernel_sts[i].mem_id: " + std::to_string(kernel_sts[i].mem_id));
                                yhao_log(0, "difuze: ops structure: " + currGlobal->getName().str());
                                yhao_log(0, "*****************************************");
                            }
                        } else {
                            yhao_log(0, "*****************************************");
                            yhao_log(0, "difuze: !currGlobal->hasInitializer()");
                            yhao_log(0, "difuze: currGlobal->getName().str(): " + currGlobal->getName().str());
                            yhao_log(0, "difuze: kernel_sts[i].method_lab: " + kernel_sts[i].method_lab);
                            yhao_log(0, "difuze: kernel_sts[i].mem_id: " + std::to_string(kernel_sts[i].mem_id));
                            yhao_log(0, "difuze: ops structure: " + currGlobal->getName().str());
                            yhao_log(0, "*****************************************");
                        }
                    }
                    i++;
                }
            }
        }
    }

    int difuze_EntryPointIdentifier(Module *m, nlohmann::json *j) {
        assert(j != nullptr);
        result_json = j;
        std::vector<string> entryPointStrings;
        entryPointStrings.clear();
        Module::GlobalListType &currGlobalList = m->getGlobalList();
        for (auto &gstart: currGlobalList) {
            GlobalVariable *currGlobal = &gstart;
            process_global(currGlobal, entryPointStrings);
        }
        return 0;
    }

}