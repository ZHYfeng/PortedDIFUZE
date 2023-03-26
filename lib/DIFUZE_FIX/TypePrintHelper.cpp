#include "TypePrintHelper.h"
#include "IOInstVisitor.h"

using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER_FIX {

    Type *getTargetStType(Instruction *targetbc) {
        BitCastInst *tarcs = dyn_cast<BitCastInst>(targetbc);
        if (tarcs != nullptr) {
            return tarcs->getSrcTy();
        }
        GetElementPtrInst *gepInst = dyn_cast<GetElementPtrInst>(targetbc);
        if (gepInst != nullptr) {
            return gepInst->getSourceElementType();
        }
        return nullptr;
    }

    Type *getTargetTypeForValue(Value *targetVal, IOInstVisitor *currFunction, nlohmann::json *current_result_json,
                                bool &is_printed) {
        int64_t debug = -1;
        std::string str = "getTargetTypeForValue: ";
        yhao_add_dump(debug, targetVal->print, str);
        targetVal = targetVal->stripPointerCasts();
        std::set<Value *> all_uses;
        all_uses.clear();
        for (auto us = targetVal->use_begin(), ue = targetVal->use_end(); us != ue; us++) {
            all_uses.insert((*us).get());
        }
        if (all_uses.size() > 1) {
            yhao_log(1, "More than one use :");
        }
        for (auto sd: all_uses) {
            GlobalVariable *gv = dyn_cast<GlobalVariable>(sd);
            if (gv) {
                return gv->getType();
            }
        }

        // OK, unable to get the type.
        // Now follow the arguments.
        if (currFunction) {
            if (currFunction->callerArgs.find(targetVal) != currFunction->callerArgs.end()) {
                // OK, this is function arguments and we have propagation information.
                Value *callerArg = currFunction->callerArgs[targetVal];
                Type *retVal = TypePrintHelper::typeOutputHandler(callerArg, current_result_json,
                                                                  currFunction->calledVisitor);
                is_printed = true;
                return retVal;
            }
        }
        return nullptr;
    }

    void printTypeRecursive(Type *targetType, nlohmann::json *current_result_json,
                            std::string &prefix_space) {
        std::string str;
        int64_t debug = -1;
        if (targetType->isStructTy() || targetType->isPointerTy()) {
            if (targetType->isPointerTy()) {
                targetType = targetType->getContainedType(0);
            }
            bool is_handled = false;
            if (targetType->isStructTy()) {
                string src_st_name = targetType->getStructName().str();
                if (src_st_name.find(".anon") != string::npos) {
                    is_handled = true;
                    // OK, this is anonymous struct or union.
                    yhao_log(0, prefix_space + src_st_name + ": START ELEMENTS:");
                    string child_space = prefix_space;
                    child_space = child_space + "  ";
                    for (unsigned int curr_no = 0; curr_no < targetType->getStructNumElements(); curr_no++) {
                        // print by adding space
                        // yu_hao: fix3: should be curr_no not 0
                        printTypeRecursive(targetType->getStructElementType(curr_no), current_result_json, child_space);
                    }
                    yhao_log(0, prefix_space + src_st_name + ": END ELEMENTS:");
                }
            }
            if (!is_handled) {
                // Regular structure, print normally.
                if (current_result_json == nullptr) {
                    yhao_log(0, "type without cmd");
                } else {
                    yhao_print(4, targetType->print, str)
                    current_result_json->push_back(str);
                }
                str = prefix_space;
                yhao_add_dump(debug, targetType->print, str)
            }
        } else {
            if (current_result_json == nullptr) {
                yhao_log(0, "type without cmd");
            } else {
                yhao_print(4, targetType->print, str)
                current_result_json->push_back(str);

            }
            str = prefix_space;
            yhao_add_dump(debug, targetType->print, str)
        }

    }

    void printType(Type *targetType, nlohmann::json *current_result_json) {
        yhao_log(0, "START TYPE");
        string curr_prefix = "printType: ";
        printTypeRecursive(targetType, current_result_json, curr_prefix);
        yhao_log(0, "END TYPE");
    }

    Type *
    TypePrintHelper::typeOutputHandler(Value *targetVal, nlohmann::json *current_result_json, IOInstVisitor *currFunc) {
        int64_t debug = -1;
        std::string str = "typeOutputHandler: ";
        yhao_add_dump(debug, targetVal->print, str);

        Type *retType = nullptr;
        bool is_printed = false;
        if (!dyn_cast<Instruction>(targetVal)) {
            Type *targetType = getTargetTypeForValue(targetVal, currFunc, current_result_json, is_printed);
            if (targetType != nullptr) {
                if (!is_printed) {
                    printType(targetType, current_result_json);
                }
            } else {
                if (dyn_cast<Instruction>(targetVal)) {
                    std::set<Instruction *> visitedInstr;
                    visitedInstr.clear();
                    targetType = TypePrintHelper::getInstructionTypeRecursive(targetVal, visitedInstr);
                    if (targetType != nullptr) {
                        printType(targetType, current_result_json);
                    } else {
                        str = " !targetType != nullptr: ";
                        yhao_add_dump(debug, targetVal->print, str);
                    }
                } else {
                    str = " !dyn_cast<Instruction>(targetVal): ";
                    yhao_add_dump(debug, targetVal->print, str);
                }
            }
            retType = targetType;
        } else {
            Type *targetType = getTargetStType(dyn_cast<Instruction>(targetVal));
            if (targetType == nullptr) {
                targetType = getTargetTypeForValue(targetVal, currFunc, current_result_json, is_printed);
            }
            // yu_hao: fix2: data flow like store i8* %to, i8** %to.addr, align 8
            if (targetType == nullptr) {
                std::set<Instruction *> visitedInstr;
                visitedInstr.clear();
                yhao_log(0, "before: targetVal = getInstructionRecursive(targetVal, visitedInstr)");
                targetVal = getInstructionRecursive(targetVal, visitedInstr);
                yhao_log(0, "after: targetVal = getInstructionRecursive(targetVal, visitedInstr)");
                if (targetVal != nullptr) {
                    targetType = getTargetTypeForValue(targetVal, currFunc, current_result_json, is_printed);
                }
            }

            if (targetType != nullptr) {
                yhao_log(0, "typeOutputHandler: targetType != nullptr");
                if (!is_printed) {
                    printType(targetType, current_result_json);
                }
            } else if (targetVal == nullptr) {
            } else {
                yhao_log(0, "typeOutputHandler: !targetType != nullptr");
                if (dyn_cast<Instruction>(targetVal)) {
                    std::set<Instruction *> visitedInstr;
                    visitedInstr.clear();
                    targetType = TypePrintHelper::getInstructionTypeRecursive(targetVal, visitedInstr);
                    if (targetType != nullptr) {
                        printType(targetType, current_result_json);
                    } else {
                        str = " ! !targetType != nullptr: ";
                        yhao_add_dump(debug, targetVal->print, str);
                    }
                } else {
                    str = " ! !dyn_cast<Instruction>(targetVal): ";
                    yhao_add_dump(debug, targetVal->print, str);
                }
            }
            retType = targetType;
        }

        return retType;
    }

    Type *TypePrintHelper::getInstructionTypeRecursive(Value *currValue, std::set<Instruction *> &visited) {
        int64_t debug = -1;
        std::string str = "getInstructionTypeRecursive";
        yhao_add_dump(debug, currValue->print, str)
        Instruction *currInstr = dyn_cast<Instruction>(currValue);
        if (currInstr != nullptr) {
            if (visited.find(currInstr) == visited.end()) {
                visited.insert(currInstr);
                if (dyn_cast<LoadInst>(currInstr)) {
                    LoadInst *currLI = dyn_cast<LoadInst>(currInstr);
                    Value *poiterVal = currLI->getPointerOperand()->stripPointerCasts();
                    for (auto u: poiterVal->users()) {
                        Value *tmpVal = dyn_cast<Value>(u);
                        if (dyn_cast<StoreInst>(tmpVal)) {
                            Type *currChType = TypePrintHelper::getInstructionTypeRecursive(tmpVal, visited);
                            if (currChType) {
                                return currChType;
                            }
                        }
                    }
                }
                if (dyn_cast<StoreInst>(currInstr)) {
                    StoreInst *currSI = dyn_cast<StoreInst>(currInstr);
                    Value *valueOp = currSI->getValueOperand();
                    if (dyn_cast<Instruction>(valueOp)) {
                        Type *currChType = TypePrintHelper::getInstructionTypeRecursive(valueOp, visited);
                        if (currChType) {
                            return currChType;
                        }
                    }
                }
                if (dyn_cast<BitCastInst>(currInstr)) {
                    BitCastInst *currBCI = dyn_cast<BitCastInst>(currInstr);
                    return currBCI->getSrcTy();
                }
                if (dyn_cast<PHINode>(currInstr)) {
                    PHINode *currPN = dyn_cast<PHINode>(currInstr);
                    unsigned i = 0;
                    while (i < currPN->getNumOperands()) {
                        Value *targetOp = currPN->getOperand(i);
                        Type *currChType = TypePrintHelper::getInstructionTypeRecursive(targetOp, visited);
                        if (currChType) {
                            return currChType;
                        }
                        i++;
                    }
                }
                if (dyn_cast<AllocaInst>(currInstr)) {
                    AllocaInst *currAI = dyn_cast<AllocaInst>(currInstr);
                    return currAI->getType();
                }
                visited.erase(currInstr);

            }
        }
        return nullptr;
    }

    Value *TypePrintHelper::getInstructionRecursive(Value *currValue, set<Instruction *> &visited) {
        int64_t debug = -1;
        std::string str = "getInstructionRecursive";
        yhao_add_dump(debug, currValue->print, str)
        auto currInstr = dyn_cast<Instruction>(currValue);
        if (currInstr != nullptr) {
            if (visited.find(currInstr) == visited.end()) {
                visited.insert(currInstr);
                if (dyn_cast<LoadInst>(currInstr)) {
                    auto currLI = dyn_cast<LoadInst>(currInstr);
                    Value *pointerVal = currLI->getPointerOperand()->stripPointerCasts();
                    for (auto u: pointerVal->users()) {
                        auto *tmpVal = dyn_cast<Value>(u);
                        if (dyn_cast<StoreInst>(tmpVal)) {
                            auto currInst = TypePrintHelper::getInstructionRecursive(tmpVal, visited);
                            if (currInst) {
                                return currInst;
                            }
                        }
                    }
                }
                if (dyn_cast<StoreInst>(currInstr)) {
                    auto *currSI = dyn_cast<StoreInst>(currInstr);
                    Value *valueOp = currSI->getValueOperand();
                    if (dyn_cast<Instruction>(valueOp)) {
                        auto currInst = TypePrintHelper::getInstructionRecursive(valueOp, visited);
                        if (currInst) {
                            return currInst;
                        }
                    } else {
                        return valueOp;
                    }
                }
                if (dyn_cast<BitCastInst>(currInstr)) {
                    auto currBCI = dyn_cast<BitCastInst>(currInstr);
                    return currBCI;
                }
                if (dyn_cast<PHINode>(currInstr)) {
                    auto currPN = dyn_cast<PHINode>(currInstr);
                    unsigned i = 0;
                    while (i < currPN->getNumOperands()) {
                        Value *targetOp = currPN->getOperand(i);
                        auto currInst = TypePrintHelper::getInstructionRecursive(targetOp, visited);
                        if (currInst) {
                            return currInst;
                        }
                        i++;
                    }
                }
                if (dyn_cast<AllocaInst>(currInstr)) {
                    auto currAI = dyn_cast<AllocaInst>(currInstr);
                    return currAI;
                }
                visited.erase(currInstr);

            }
        }
        return nullptr;
    }
}