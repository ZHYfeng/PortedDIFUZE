#ifndef PROJECT_TYPEPRINTHELPER_H_FIX
#define PROJECT_TYPEPRINTHELPER_H_FIX

#include "../ToolLib/log.h"
#include "../ToolLib/json.hpp"

using namespace llvm;
using namespace std;
namespace IOCTL_CHECKER_FIX {
    class IOInstVisitor;

    class TypePrintHelper {
    public:
        static Type *typeOutputHandler(Value *targetVal, nlohmann::json *current_result_json, IOInstVisitor *currFunc);

        static Type *getInstructionTypeRecursive(Value *currValue, std::set<Instruction *> &visited);

        static Value *getInstructionRecursive(Value *currValue, std::set<Instruction *> &visited);
    };
}

#endif //PROJECT_TYPEPRINTHELPER_H_FIX
