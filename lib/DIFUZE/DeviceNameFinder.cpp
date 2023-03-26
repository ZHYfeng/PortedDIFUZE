//
// Created by machiry on 4/26/17.
//

#include "../TemplateLib/define.h"
#include "../ToolLib/json.hpp"
#include "../ToolLib/log.h"
#include "../ToolLib/llvm_related.h"

using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {
    struct DeviceNameFinderPass : public ModulePass {
    public:
        static char ID;
        std::string checkFunctionName;
        //GlobalState moduleState;

        DeviceNameFinderPass() : ModulePass(ID) {
        }

        ~DeviceNameFinderPass() {
        }

        void set_function_name(std::string name) {
            checkFunctionName = std::move(name);
        }

        nlohmann::json *result_json;
        void set_result_json(nlohmann::json *j) {
            assert(j->empty() || j->is_array());
            result_json = j;
        }

        void add_device_file_name(std::string name) {
            auto *temp_j_1 = new nlohmann::json();
            (*temp_j_1)[DEVICE_FILE_NAME] = name;
            if (!(*result_json).empty()) {
                assert((*result_json).is_array() && "add_device_file_name");
            }
            (*result_json).push_back(*temp_j_1);
        }

        bool isCalledFunction(CallInst *callInst, std::string &funcName) {
            Function *targetFunction = nullptr;
            if (callInst != nullptr) {
                targetFunction = callInst->getCalledFunction();
                if (targetFunction == nullptr) {
                    targetFunction = dyn_cast<Function>(callInst->getCalledOperand()->stripPointerCasts());
                }
            }
            return (targetFunction != nullptr && targetFunction->hasName() &&
                    targetFunction->getName() == funcName);
        }

        llvm::GlobalVariable *getSrcTrackedVal(Value *currVal) {
            llvm::GlobalVariable *currGlob = dyn_cast<llvm::GlobalVariable>(currVal);
            if (currGlob == nullptr) {
                Instruction *currInst = dyn_cast<Instruction>(currVal);
                if (currInst != nullptr) {
                    for (unsigned int i = 0; i < currInst->getNumOperands(); i++) {
                        Value *currOp = currInst->getOperand(i);
                        llvm::GlobalVariable *opGlobVal = getSrcTrackedVal(currOp);
                        if (opGlobVal == nullptr) {
                            opGlobVal = getSrcTrackedVal(currOp->stripPointerCasts());
                        }
                        if (opGlobVal != nullptr) {
                            currGlob = opGlobVal;
                            break;
                        }
                    }
                } else {
                    return nullptr;
                }
            }
            return currGlob;
        }

        llvm::AllocaInst *getSrcTrackedAllocaVal(Value *currVal) {
            llvm::AllocaInst *currGlob = dyn_cast<AllocaInst>(currVal);
            if (currGlob == nullptr) {
                Instruction *currInst = dyn_cast<Instruction>(currVal);
                if (currInst != nullptr) {
                    for (unsigned int i = 0; i < currInst->getNumOperands(); i++) {
                        Value *currOp = currInst->getOperand(i);
                        llvm::AllocaInst *opGlobVal = getSrcTrackedAllocaVal(currOp);
                        if (opGlobVal == nullptr) {
                            opGlobVal = getSrcTrackedAllocaVal(currOp->stripPointerCasts());
                        }
                        if (opGlobVal != nullptr) {
                            currGlob = opGlobVal;
                            break;
                        }
                    }
                } else {
                    return nullptr;
                }
            }
            return currGlob;
        }

        llvm::Value *getSrcTrackedArgVal(Value *currVal, Function *targetFunction) {
            Instruction *currInst = dyn_cast<Instruction>(currVal);
            if (currInst == nullptr) {
                for (auto &a: targetFunction->args()) {
                    if (&a == currVal) {
                        return &a;
                    }
                }
            } else {

                for (unsigned int i = 0; i < currInst->getNumOperands(); i++) {
                    Value *currOp = currInst->getOperand(i);
                    llvm::Value *opGlobVal = getSrcTrackedArgVal(currOp, targetFunction);
                    if (opGlobVal == nullptr) {
                        opGlobVal = getSrcTrackedArgVal(currOp->stripPointerCasts(), targetFunction);
                    }
                    if (opGlobVal != nullptr) {
                        return opGlobVal;
                    }
                }
            }
            return nullptr;

        }

        bool getDeviceString(Value *currVal) {
            int64_t debug = -1;
            const GEPOperator *gep = dyn_cast<GEPOperator>(currVal);
            const llvm::GlobalVariable *strGlobal = nullptr;
            if (gep != nullptr) {
                strGlobal = dyn_cast<GlobalVariable>(gep->getPointerOperand());

            }
            if (strGlobal != nullptr && strGlobal->hasInitializer()) {
                const Constant *currConst = strGlobal->getInitializer();
                const ConstantDataArray *currDArray = dyn_cast<ConstantDataArray>(currConst);
                if (currDArray != nullptr) {
                    yhao_log(0, "difuze: Device Name: " + currDArray->getAsCString().str());
                    add_device_file_name(currDArray->getAsCString().str());
                } else {
                    std::string str;
                    yhao_print(4, currConst->print, str)
                    yhao_log(0, "difuze: Device Name: " + str);
                    add_device_file_name(str);
                }
                return true;
            }
            return false;
        }

        CallInst *getRecursiveCallInst(Value *srcVal, std::string &targetFuncName, std::set<Value *> &visitedVals) {
            CallInst *currInst = nullptr;
            if (visitedVals.find(srcVal) != visitedVals.end()) {
                return nullptr;

            }
            visitedVals.insert(srcVal);
            for (auto curr_use: srcVal->users()) {
                currInst = dyn_cast<CallInst>(curr_use);
                if (currInst && isCalledFunction(currInst, targetFuncName)) {
                    break;
                }
                currInst = nullptr;
            }
            if (currInst == nullptr) {
                for (auto curr_use: srcVal->users()) {
                    currInst = getRecursiveCallInst(curr_use, targetFuncName, visitedVals);
                    if (currInst && isCalledFunction(currInst, targetFuncName)) {
                        break;
                    }
                    currInst = nullptr;
                }
            }
            visitedVals.erase(visitedVals.find(srcVal));
            return currInst;
        }

        bool handleGenericCDevRegister(Function *currFunction) {
            for (Function::iterator fi = currFunction->begin(), fe = currFunction->end(); fi != fe; fi++) {
                BasicBlock &currBB = *fi;
                for (BasicBlock::iterator i = currBB.begin(), ie = currBB.end(); i != ie; ++i) {
                    Instruction *currInstPtr = &(*i);
                    CallInst *currCall = dyn_cast<CallInst>(currInstPtr);

                    std::string allocChrDev = "alloc_chrdev_region";
                    if (isCalledFunction(currCall, allocChrDev)) {
                        Value *devNameOp = currCall->getArgOperand(3);
                        return getDeviceString(devNameOp);
                    }
                    // is the usage device_create?
                    std::string devCreateName = "device_create";
                    if (isCalledFunction(currCall, devCreateName)) {
                        Value *devNameOp = currCall->getArgOperand(4);
                        return getDeviceString(devNameOp);
                    }

                    std::string regChrDevRegion = "register_chrdev_region";
                    if (isCalledFunction(currCall, regChrDevRegion)) {
                        Value *devNameOp = currCall->getArgOperand(2);
                        return getDeviceString(devNameOp);
                    }
                }
            }
            return false;
        }

        bool handleCdevHeuristic(CallInst *cdevCallInst, llvm::GlobalVariable *fopsStructure) {
            Value *deviceStructure = cdevCallInst->getArgOperand(0);
            // get the dev structure
            //llvm::GlobalVariable *globalDevSt = dyn_cast<llvm::GlobalVariable>(deviceStructure);
            llvm::GlobalVariable *globalDevSt = getSrcTrackedVal(deviceStructure);
            if (globalDevSt != nullptr) {
                // globalDevSt->dump();
                CallInst *cdev_call = nullptr;

                // get the cdev_add function which is using the dev structure.
                /*for(auto curr_use:globalDevSt->users()) {
                    cdev_call = dyn_cast<CallInst>(curr_use);
                    if(isCalledFunction(cdev_call, "cdev_add")) {
                        break;
                    }
                }*/
                std::set<Value *> visitedVals;
                visitedVals.clear();
                std::string cdevaddFunc = "cdev_add";
                cdev_call = getRecursiveCallInst(globalDevSt, cdevaddFunc, visitedVals);
                if (cdev_call != nullptr) {
                    // find, the devt structure.
                    llvm::GlobalVariable *target_devt = getSrcTrackedVal(cdev_call->getArgOperand(1));

                    llvm::AllocaInst *allocaVal = nullptr;
                    Value *targetDevNo;
                    targetDevNo = target_devt;

                    if (target_devt == nullptr) {
                        allocaVal = getSrcTrackedAllocaVal(cdev_call->getArgOperand(1));
                        targetDevNo = allocaVal;
                    }

                    if (targetDevNo == nullptr) {
                        targetDevNo = getSrcTrackedArgVal(cdev_call->getArgOperand(1), cdevCallInst->getFunction());
                    }

                    // find the uses.
                    if (targetDevNo != nullptr) {
                        visitedVals.clear();
                        std::string allocChrDevReg = "alloc_chrdev_region";
                        CallInst *currCall = getRecursiveCallInst(targetDevNo, allocChrDevReg, visitedVals);
                        if (currCall != nullptr) {
                            Value *devNameOp = currCall->getArgOperand(3);
                            return getDeviceString(devNameOp);
                        }
                        visitedVals.clear();
                        std::string devCreate = "device_create";
                        currCall = getRecursiveCallInst(targetDevNo, devCreate, visitedVals);
                        // is the usage device_create?
                        if (currCall) {
                            Value *devNameOp = currCall->getArgOperand(4);
                            return getDeviceString(devNameOp);
                        }

                    }
                }
                return handleGenericCDevRegister(cdevCallInst->getParent()->getParent());
            }
            return false;
        }

        bool handleProcCreateHeuristic(CallInst *procCallInst, llvm::GlobalVariable *fopsStructure) {
            Value *devNameOp = procCallInst->getArgOperand(0);
            return getDeviceString(devNameOp);
        }

        bool handleMiscDevice(llvm::GlobalVariable *miscDevice) {
            if (miscDevice->hasInitializer()) {
                ConstantStruct *outputSt = dyn_cast<ConstantStruct>(miscDevice->getInitializer());
                if (outputSt != nullptr) {
                    Value *devNameVal = outputSt->getOperand(1);
                    return getDeviceString(devNameVal);
                }
            }
            return false;
        }


        bool handleDynamicMiscOps(StoreInst *srcStoreInst) {
#define MISC_FILENAME_INDX 1
            // OK this is the store instruction which is trying to store fops into a misc device
            BasicBlock *targetBB = srcStoreInst->getParent();
            // iterate thru all the instructions to find any store to a misc device name field.
            std::set<Value *> nameField;
            for (BasicBlock::iterator i = targetBB->begin(), ie = targetBB->end(); i != ie; ++i) {
                Instruction *currInstPtr = &(*i);
                GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(currInstPtr);
                if (gepInst && gepInst->getNumOperands() > 2) {
                    StructType *targetStruct = dyn_cast<StructType>(gepInst->getSourceElementType());
                    if (targetStruct != nullptr && targetStruct->getName() == "struct.miscdevice") {
                        ConstantInt *fieldInt = dyn_cast<ConstantInt>(gepInst->getOperand(2));
                        if (fieldInt) {
                            if (fieldInt->getZExtValue() == MISC_FILENAME_INDX) {
                                nameField.insert(&(*i));
                            }
                        }
                    }
                }

                // Are we storing into name field of a misc structure?
                StoreInst *currStInst = dyn_cast<StoreInst>(currInstPtr);
                if (currStInst) {
                    Value *targetPtr = currStInst->getPointerOperand()->stripPointerCasts();
                    if (nameField.find(targetPtr) != nameField.end()) {
                        // YES.
                        // find the name
                        return getDeviceString(currStInst->getOperand(0));
                    }
                }
            }
            return false;
        }

        void handleV4L2Dev(Module &m) {
#define VFL_TYPE_GRABBER    0
#define VFL_TYPE_VBI        1
#define VFL_TYPE_RADIO        2
#define VFL_TYPE_SUBDEV        3
#define VFL_TYPE_SDR        4
#define VFL_TYPE_TOUCH        5

            std::map<uint64_t, std::string> v4l2TypeNameMap;
            v4l2TypeNameMap[VFL_TYPE_GRABBER] = "/dev/video[X]";
            v4l2TypeNameMap[VFL_TYPE_VBI] = "/dev/vbi[X]";
            v4l2TypeNameMap[VFL_TYPE_RADIO] = "/dev/radio[X]";
            v4l2TypeNameMap[VFL_TYPE_SUBDEV] = "/dev/subdev[X]";
            v4l2TypeNameMap[VFL_TYPE_SDR] = "/dev/swradio[X]";
            v4l2TypeNameMap[VFL_TYPE_TOUCH] = "/dev/v4l-touch[X]";

            // find all functions
            for (Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {
                Function &currFunction = *mi;
                if (!currFunction.isDeclaration() && currFunction.hasName()) {
                    string currFuncName = currFunction.getName().str();
                    if (currFuncName.find("init") != string::npos || currFuncName.find("probe") != string::npos) {
                        for (Function::iterator fi = currFunction.begin(), fe = currFunction.end(); fi != fe; fi++) {
                            BasicBlock &currBB = *fi;
                            for (BasicBlock::iterator i = currBB.begin(), ie = currBB.end(); i != ie; ++i) {
                                Instruction *currInstPtr = &(*i);
                                CallInst *currCall = dyn_cast<CallInst>(currInstPtr);
                                if (currCall != nullptr) {
                                    Function *calledFunc = currCall->getCalledFunction();
                                    if (calledFunc != nullptr && calledFunc->hasName() &&
                                        calledFunc->getName() == "__video_register_device") {
                                        Value *devType = currCall->getArgOperand(1);
                                        //InterProceduralRA<CropDFS> &range_analysis = getAnalysis<InterProceduralRA<CropDFS>>();
                                        //Range devRange = range_analysis.getRange(devType);
                                        ConstantInt *cInt = dyn_cast<ConstantInt>(devType);
                                        if (cInt) {
                                            uint64_t typeNum = cInt->getZExtValue();
                                            if (v4l2TypeNameMap.find(typeNum) != v4l2TypeNameMap.end()) {
                                                yhao_log(1, "difuze: V4L2 Device: " + v4l2TypeNameMap[typeNum]);
                                                add_device_file_name(v4l2TypeNameMap[typeNum]);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }


        bool runOnModule(Module &m) override {
            yhao_log(0, "difuze: Provided Function Name: " + checkFunctionName);
            assert(result_json != nullptr);

            for (Module::iterator mi = m.begin(), ei = m.end(); mi != ei; mi++) {

                Function &currFunction = *mi;
                //if the current function is the target function.
                if (!currFunction.isDeclaration() && currFunction.hasName() &&
                    currFunction.getName().str() == checkFunctionName) {

                    llvm::GlobalVariable *targetFopsStructure = nullptr;
                    for (auto curr_use: currFunction.users()) {
                        if (curr_use->getType()->isStructTy()) {
                            for (auto curr_use1: curr_use->users()) {
                                llvm::GlobalVariable *currGlobal = dyn_cast<llvm::GlobalVariable>(curr_use1);
                                if (currGlobal != nullptr) {
                                    targetFopsStructure = currGlobal;
                                    yhao_log(0,
                                             "difuze: Found Fops Structure: " + targetFopsStructure->getName().str());
                                    break;

                                }
                            }
                            if (targetFopsStructure != nullptr) {
                                break;
                            }
                        }
                    }

                    if (targetFopsStructure == nullptr) {
                        yhao_log(1, "difuze: Unable to find fops structure which is using this function.");
                    } else {
                        for (auto curr_usage: targetFopsStructure->users()) {
                            CallInst *currCallInst = dyn_cast<CallInst>(curr_usage);
                            std::string cdevInitName = "cdev_init";
                            if (isCalledFunction(currCallInst, cdevInitName)) {
                                if (handleCdevHeuristic(currCallInst, targetFopsStructure)) {
                                    yhao_log(0, "difuze: Device Type: char");
                                    yhao_log(0, "difuze: Found using first cdev heuristic");
                                }

                            }

                            std::string regChrDev = "__register_chrdev";
                            if (isCalledFunction(currCallInst, regChrDev)) {
                                if (getDeviceString(currCallInst->getArgOperand(3))) {
                                    yhao_log(0, "difuze: Device Type: char");
                                    yhao_log(0, "difuze: Found using register chrdev heuristic");
                                }

                            }


                            std::string procCreateData = "proc_create_data";
                            std::string procCreateName = "proc_create";
                            if (isCalledFunction(currCallInst, procCreateData) ||
                                isCalledFunction(currCallInst, procCreateName)) {
                                if (handleProcCreateHeuristic(currCallInst, targetFopsStructure)) {
                                    yhao_log(0, "difuze: Device Type: proc");
                                    yhao_log(0, "difuze: Found using proc create heuristic");
                                }
                            }


                            // Handling misc devices
                            // Standard. (when misc device is defined as static)
                            if (curr_usage->getType()->isStructTy()) {
                                llvm::GlobalVariable *currGlobal = nullptr;
                                // OK, find the miscdevice
                                for (auto curr_use1: curr_usage->users()) {
                                    currGlobal = dyn_cast<llvm::GlobalVariable>(curr_use1);
                                    if (currGlobal != nullptr) {
                                        break;
                                    }
                                }
                                if (currGlobal != nullptr) {
                                    for (auto second_usage: currGlobal->users()) {

                                        CallInst *currCallInst = dyn_cast<CallInst>(second_usage);
                                        std::string miscRegFunc = "misc_register";
                                        if (isCalledFunction(currCallInst, miscRegFunc)) {
                                            if (handleMiscDevice(currGlobal)) {
                                                yhao_log(0, "difuze: Device Type: misc");
                                                yhao_log(0, "difuze: Found using misc heuristic");
                                            }

                                        }
                                    }
                                }
                            }

                            // when misc device is created manually
                            // using kmalloc
                            StoreInst *currStore = dyn_cast<StoreInst>(curr_usage);
                            if (currStore != nullptr) {
                                // OK, we are storing into a structure.
                                if (handleDynamicMiscOps(currStore)) {
                                    yhao_log(0, "difuze: Device Type: misc");
                                    yhao_log(0, "difuze: Found using dynamic misc heuristic");
                                }
                            }

                            // Handling v4l2 devices
                            // More information: https://01.org/linuxgraphics/gfx-docs/drm/media/kapi/v4l2-dev.html
                            Type *targetFopsType = targetFopsStructure->getType()->getContainedType(0);
                            if (targetFopsType->isStructTy()) {
                                StructType *fopsStructType = dyn_cast<StructType>(targetFopsType);
                                if (fopsStructType->hasName()) {
                                    std::string structureName = fopsStructType->getStructName().str();
                                    if (structureName == "struct.v4l2_ioctl_ops") {
                                        handleV4L2Dev(m);
                                        yhao_log(0, "difuze: Device Type: v4l2");
                                        yhao_log(0,
                                                 "difuze: Look into: /sys/class/video4linux/<devname>/name to know the details");
                                        return false;
                                    }
                                }
                            }

                        }
                    }
                }
            }
            return false;
        }
    };

    char DeviceNameFinderPass::ID = 0;
    static RegisterPass<DeviceNameFinderPass> x("dev-name-finder", "Device name finder", false, true);
}