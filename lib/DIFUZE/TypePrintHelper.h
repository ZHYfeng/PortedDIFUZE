//
// Created by machiry on 4/26/17.
//

#ifndef PROJECT_TYPEPRINTHELPER_H
#define PROJECT_TYPEPRINTHELPER_H

#include "../ToolLib/log.h"
#include "../ToolLib/llvm_related.h"
#include "../ToolLib/json.hpp"

using namespace llvm;
using namespace std;
namespace IOCTL_CHECKER {
    class IOInstVisitor;

    class TypePrintHelper {
    public:
        static Type *typeOutputHandler(Value *targetVal, nlohmann::json *current_result_json, IOInstVisitor *currFunc);

        static Type *getInstructionTypeRecursive(Value *currValue, std::set<Instruction *> &visited);

        static Value *getInstructionRecursive(Value *currValue, std::set<Instruction *> &visited);
    };
}

#endif //PROJECT_TYPEPRINTHELPER_H
