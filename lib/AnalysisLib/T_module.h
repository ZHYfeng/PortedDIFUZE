//
// Created by yhao on 3/16/21.
//

#ifndef INC_2021_TEMPLATE_T_MODULE_H
#define INC_2021_TEMPLATE_T_MODULE_H

#include "../ToolLib/basic.h"

namespace lddt {
    // things at LLVM T_module level
    class T_module {
    public:
        T_module();

        virtual ~T_module();

        void read_bitcode(const std::string &BitcodeFileName);

        std::string bitcode;
        std::unique_ptr<llvm::Module> llvm_module;
        std::string work_dir;
    };
}


#endif //INC_2021_TEMPLATE_T_MODULE_H
