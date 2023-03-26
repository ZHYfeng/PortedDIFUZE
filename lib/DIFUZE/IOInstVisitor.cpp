//
// Created by machiry on 4/24/17.
//

#include "IOInstVisitor.h"

#include "CFGUtils.h"
#include "TypePrintHelper.h"

using namespace llvm;
using namespace std;
#define MAX_FUNC_DEPTH 10
namespace IOCTL_CHECKER {
    // the main analysis function.
    void IOInstVisitor::analyze() {
        // No what we need is:
        // Traverse the CFG in Breadth first order.
        std::vector<BasicBlock *> processQueue;

        std::vector<std::vector<BasicBlock *> *> *traversalOrder =
                BBTraversalHelper::getSCCTraversalOrder(*(this->targetFunction));
        for (auto currSCC: *traversalOrder) {
            // current strongly connected component.
            for (auto currBB: *currSCC) {
                processQueue.insert(processQueue.end(), currBB);
            }

        }

        bool is_handled;
        std::set<BasicBlock *> totalVisitedBBs;
        while (!processQueue.empty()) {
            BasicBlock *currBB = processQueue[0];
            // remove first element
            processQueue.erase(processQueue.begin());
            if (this->visitBB(currBB)) {
                yhao_log(1, "Found a common structure");
            }
            Instruction *terminInst = currBB->getTerminator();
            is_handled = false;

            if (terminInst != nullptr) {
                // first check if the instruction is affected by cmd value
                if (terminInst->getNumSuccessors() > 1 && isCmdAffected(terminInst)) {
                    // is this a switch?
                    auto *dstSwitch = dyn_cast<SwitchInst>(currBB->getTerminator());
                    if (dstSwitch != nullptr) {
                        //dbgs() << "Trying switch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                        // is switch handle cmd switch.
                        is_handled = handleCmdSwitch(dstSwitch, totalVisitedBBs);
                    } else {
                        //dbgs() << "START:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                        // not switch?, check if the branch instruction
                        // if this is branch, handle cmd branch
                        auto *brInst = dyn_cast<BranchInst>(terminInst);
                        if (brInst == nullptr) {
                            yhao_log(1, "Culprit:");
//                            currBB->dump();
                        }
                        assert(brInst != nullptr);
                        is_handled = handleCmdCond(brInst, totalVisitedBBs);
                        //dbgs() << "END:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                    }
                }
                if (is_handled) {
                    std::vector<BasicBlock *> reachableBBs;
                    reachableBBs.clear();
                    BBTraversalHelper::getAllReachableBBs(currBB, processQueue, reachableBBs);
                    //dbgs() << "Removing all successor BBs from:" << currBB->getName() << ":" << this->targetFunction->getName() << "\n";
                    for (auto &reachableBB: reachableBBs) {
                        // remove all reachable BBs as these are already handled.
                        processQueue.erase(std::remove(processQueue.begin(), processQueue.end(), reachableBB),
                                           processQueue.end());
                    }
                }
            } else {
                assert(false);
            }
        }
    }

    void IOInstVisitor::visitAllBBs(BasicBlock *startBB, std::set<BasicBlock *> &visitedBBs,
                                    std::set<BasicBlock *> &totalVisited,
                                    std::set<BasicBlock *> &visitedInThisIteration) {
        Instruction *terminInst;
        bool is_handled;

        if (visitedBBs.find(startBB) == visitedBBs.end() && totalVisited.find(startBB) == totalVisited.end()) {
            visitedBBs.insert(startBB);
            visitedInThisIteration.insert(startBB);
            // if no copy_from/to_user function is found?
            if (!this->visitBB(startBB)) {

                terminInst = startBB->getTerminator();
                is_handled = false;

                if (terminInst != nullptr) {
                    // first check if the instruction is affected by cmd value
                    if (terminInst->getNumSuccessors() > 1 && isCmdAffected(terminInst)) {
                        // is this a switch?
                        SwitchInst *dstSwitch = dyn_cast<SwitchInst>(startBB->getTerminator());
                        if (dstSwitch != nullptr) {
                            //dbgs() << "Trying switch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                            // is switch handle cmd switch.
                            is_handled = handleCmdSwitch(dstSwitch, totalVisited);
                        } else {
                            //dbgs() << "START:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                            // not switch?, check if the branch instruction
                            // if this is branch, handle cmd branch
                            BranchInst *brInst = dyn_cast<BranchInst>(terminInst);
                            if (brInst == nullptr) {
                                yhao_log(1, "Culprit:");
//                                startBB->dump();
                            }
                            assert(brInst != nullptr);
                            is_handled = handleCmdCond(brInst, totalVisited);
                            //dbgs() << "END:Trying branch processing for:" << currBB->getName() << ":" << this->targetFunction->getName() <<"\n";
                        }
                    }
                }
                if (!is_handled) {
                    // then visit its successors.
                    for (auto sb = succ_begin(startBB), se = succ_end(startBB); sb != se; sb++) {
                        BasicBlock *currSucc = *sb;
                        this->visitAllBBs(currSucc, visitedBBs, totalVisited, visitedInThisIteration);
                    }
                }
            }
            visitedBBs.erase(startBB);
        }
    }

    bool IOInstVisitor::handleCmdSwitch(SwitchInst *targetSwitchInst, std::set<BasicBlock *> &totalVisited) {
        int64_t debug = 0;
        Value *targetSwitchCond = targetSwitchInst->getCondition();
        std::set<BasicBlock *> visitedInThisIteration;
        if (this->isCmdAffected(targetSwitchCond)) {
            for (auto cis = targetSwitchInst->case_begin(), cie = targetSwitchInst->case_end(); cis != cie; cis++) {
                ConstantInt *cmdVal = cis->getCaseValue();
                BasicBlock *caseStartBB = cis->getCaseSuccessor();
                std::set<BasicBlock *> visitedBBs;
                visitedBBs.clear();
                // start the print
                yhao_log(debug, "Found Cmd:" + std::to_string(cmdVal->getValue().getZExtValue()) + ":START");
                std::string cmd_value = std::to_string(cmdVal->getValue().getZExtValue());
                if (result_json->contains(cmd_value)) {

                } else {
                    (*result_json)[cmd_value] = {};
                }
                current_result_json = &(*result_json)[cmd_value];
                // Now visit all the successors
                visitAllBBs(caseStartBB, visitedBBs, totalVisited, visitedInThisIteration);
                yhao_log(debug, "Found Cmd:" + std::to_string(cmdVal->getValue().getZExtValue()) + ":END");
            }
            totalVisited.insert(visitedInThisIteration.begin(), visitedInThisIteration.end());
            return true;
        }
        return false;
    }

    bool IOInstVisitor::handleCmdCond(BranchInst *I, std::set<BasicBlock *> &totalVisited) {
        int64_t debug = 0;
        // OK, So, this is a branch instruction,
        // We could possibly have cmd value compared against.
        if (this->currCmdValue != nullptr) {
            //dbgs() << "CMDCOND:::" << *(this->currCmdValue) << "\n";
            std::set<BasicBlock *> visitedBBs;
            visitedBBs.clear();
            std::set<BasicBlock *> visitedInThisIteration;
            Value *cmdValue = this->currCmdValue;
            ConstantInt *cInt = dyn_cast<ConstantInt>(cmdValue);
            if (cInt != nullptr) {
                yhao_log(debug, "Found Cmd:" + std::to_string(cInt->getZExtValue()) + " (BR): START");
                std::string cmd_value = std::to_string(cInt->getZExtValue());
                if (result_json->contains(cmd_value)) {

                } else {
                    (*result_json)[cmd_value] = {};
                }
                current_result_json = &(*result_json)[cmd_value];
                this->currCmdValue = nullptr;
                for (auto sb = succ_begin(I->getParent()), se = succ_end(I->getParent()); sb != se; sb++) {
                    BasicBlock *currSucc = *sb;
                    if (totalVisited.find(currSucc) == totalVisited.end()) {
                        this->visitAllBBs(currSucc, visitedBBs, totalVisited, visitedInThisIteration);
                    }
                }
                yhao_log(debug, "Found Cmd:" + std::to_string(cInt->getZExtValue()) + " (BR): END");
                // insert all the visited BBS into total visited.
                totalVisited.insert(visitedInThisIteration.begin(), visitedInThisIteration.end());
                return true;
            }
        }
        return false;
    }

    Function *IOInstVisitor::getFinalFuncTarget(CallInst *I) {
        if (I->getCalledFunction() == nullptr) {
            Value *calledVal = I->getCalledOperand();
            if (dyn_cast<Function>(calledVal)) {
                return dyn_cast<Function>(calledVal->stripPointerCasts());
            }
        }
        return I->getCalledFunction();

    }

    // visitor instructions
    void IOInstVisitor::visitCallInst(CallInst &I) {
        Function *dstFunc = this->getFinalFuncTarget(&I);
        if (dstFunc != nullptr) {
            if (dstFunc->isDeclaration()) {
                if (dstFunc->hasName()) {
                    string calledfunc = I.getCalledFunction()->getName().str();
                    Value *targetOperand = nullptr;
                    Value *srcOperand = nullptr;
                    if (calledfunc.find("_copy_from_user") != string::npos) {
//                        dbgs() << "copy_from_user:\n";
                        if (I.getNumOperands() >= 2) {
                            targetOperand = I.getArgOperand(0);
                        }
                        if (I.getNumOperands() >= 3) {
                            srcOperand = I.getArgOperand(1);
                        }
                    } else if (calledfunc.find("_copy_to_user") != string::npos) {
                        // some idiot doesn't know how to parse
//                        dbgs() << "copy_to_user:\n";
//                        srcOperand = I.getArgOperand(0);
//                        targetOperand = I.getArgOperand(1);
                    }
                    if (srcOperand != nullptr) {
                        // sanity, this should be user value argument.
                        // only consider value arguments.
                        if (!this->isArgAffected(srcOperand)) {
                            yhao_log(1, "Found a copy from user from non-user argument");
                            //srcOperand = nullptr;
                            //targetOperand = nullptr;
                        }
                    }
                    if (targetOperand != nullptr) {
                        std::string str;
                        llvm::raw_string_ostream dump(str);
                        TypePrintHelper::typeOutputHandler(targetOperand, current_result_json, this);
                    }
                }
            } else {
                // check if maximum function depth is reached.
                if (this->curr_func_depth > MAX_FUNC_DEPTH) {
                    return;
                }
                // we need to follow the called function, only if this is not recursive.
                if (std::find(this->callStack.begin(), this->callStack.end(), &I) == this->callStack.end()) {
                    std::vector<Value *> newCallStack;
                    newCallStack.insert(newCallStack.end(), this->callStack.begin(), this->callStack.end());
                    newCallStack.insert(newCallStack.end(), &I);
                    std::set<int> cmdArg;
                    std::set<int> uArg;
                    std::map<unsigned, Value *> callerArgMap;
                    cmdArg.clear();
                    uArg.clear();
                    callerArgMap.clear();
                    // get propagation info
                    this->getArgPropogationInfo(&I, cmdArg, uArg, callerArgMap);
                    // analyze only if one of the argument is a command or argument
                    if (cmdArg.size() > 0 || uArg.size() > 0) {
                        IOInstVisitor *childFuncVisitor = new IOInstVisitor(dstFunc, cmdArg, uArg, callerArgMap,
                                                                            newCallStack, this,
                                                                            this->curr_func_depth + 1);
                        childFuncVisitor->result_json = this->result_json;
                        childFuncVisitor->current_result_json = this->current_result_json;
                        childFuncVisitor->analyze();
                    }

                }
            }
        } else {
            // check if maximum function depth is reached.
            if (this->curr_func_depth > MAX_FUNC_DEPTH) {
                return;
            }
            if (!I.isInlineAsm()) {

            }
        }
    }

    void IOInstVisitor::visitICmpInst(ICmpInst &I) {
        // check if we doing cmd == comparision.
        if (I.isEquality()) {
            Value *op1 = I.getOperand(0);
            Value *op2 = I.getOperand(1);
            Value *targetValueArg = nullptr;
            if (this->isCmdAffected(op1)) {
                targetValueArg = op2;
            } else if (this->isCmdAffected(op2)) {
                targetValueArg = op1;
            }
            if (targetValueArg != nullptr) {
                //dbgs() << "Setting value for:" << I << "\n";
                this->currCmdValue = targetValueArg;
            }
        }
    }

    bool IOInstVisitor::visitBB(BasicBlock *BB) {
//        dbgs() << "START TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        _super->visit(BB->begin(), BB->end());
//        dbgs() << "END TRYING TO VISIT:" << BB->getName() << ":" << this->targetFunction->getName() << "\n";
        return false;
    }

    void IOInstVisitor::getArgPropogationInfo(CallInst *I, std::set<int> &cmdArg, std::set<int> &uArg,
                                              std::map<unsigned, Value *> &callerArgInfo) {
        int curr_arg_indx = 0;
        for (User::op_iterator arg_begin = I->arg_begin(), arg_end = I->arg_end();
             arg_begin != arg_end; arg_begin++) {
            Value *currArgVal = (*arg_begin).get();
            if (this->isCmdAffected(currArgVal)) {
                cmdArg.insert(curr_arg_indx);
            }
            if (this->isArgAffected(currArgVal)) {
                uArg.insert(curr_arg_indx);
            }
            callerArgInfo[curr_arg_indx] = currArgVal;
            curr_arg_indx++;
        }
    }

    bool IOInstVisitor::isInstrToPropogate(Instruction *I) {
        return false;
    }
}
