//
// Created by machiry on 4/26/17.
//

#include "../ToolLib/log.h"
#include "../ToolLib/llvm_related.h"
#include "../ToolLib/json.hpp"
#include "TypePrintHelper.h"
#include "IOInstVisitor.h"


using namespace llvm;
using namespace std;

namespace IOCTL_CHECKER {

    struct IoctlCmdCheckerPass : public ModulePass {
    public:
        static char ID;
        std::string checkFunctionName;
        //GlobalState moduleState;

        IoctlCmdCheckerPass() : ModulePass(ID) {
        }

        ~IoctlCmdCheckerPass() {
        }

        void set_function_name(std::string name) {
            checkFunctionName = std::move(name);
        }

        nlohmann::json *result_json;
        void set_result_json(nlohmann::json *j) {
            assert(j->empty() || j->is_array());
            result_json = j;
        }

        bool runOnModule(Module &m) override {
            yhao_log(0, "Provided Function Name: " + checkFunctionName);
            //unimelb::WrappedRangePass &range_analysis = getAnalysis<unimelb::WrappedRangePass>();
            for (auto &currFunction: m) {
                std::string tmpFilePath;
                std::string includePrefix = ".includes";
                std::string preprocessedPrefix = ".preprocessed";

                SmallVector<std::pair<unsigned, MDNode *>, 4> MDs;
                std::string targetFName;
                currFunction.getAllMetadata(MDs);

                //if the current function is the target function.
                if (!currFunction.isDeclaration() && currFunction.hasName() &&
                    currFunction.getName().str() == checkFunctionName) {

                    //InterProceduralRA<CropDFS> &range_analysis = getAnalysis<InterProceduralRA<CropDFS>>();
                    std::set<int> cArg;
                    std::set<int> uArg;
                    std::vector<Value *> callStack;
                    std::map<unsigned, Value *> callerArguments;
                    cArg.insert(1);
                    uArg.insert(2);
                    callerArguments.clear();
                    IOInstVisitor *currFuncVis = new IOInstVisitor(&currFunction, cArg, uArg, callerArguments,
                                                                   callStack, nullptr, 0);
                    // start analyzing
                    currFuncVis->result_json = result_json;
                    currFuncVis->analyze();
                }
            }
            return false;
        }
    };

    char IoctlCmdCheckerPass::ID = 0;
    static RegisterPass<IoctlCmdCheckerPass> y("new-ioctl-cmd-parser", "IOCTL Command Parser", false, true);
}