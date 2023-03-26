//
// Created by yu on 4/18/21.
//

#ifndef INC_2021_TEMPLATE_MANAGER_H
#define INC_2021_TEMPLATE_MANAGER_H

#include "../ToolLib/json.hpp"
#include "../AnalysisLib/T_module.h"

namespace lddt {
    class Manager {
    public:
        std::string bitcode;
        T_module *t_module;

    public:
        Manager();

        void test();

        void start(std::string _bitcode);

        void test_difuze();

        void test_difuze_fix();

        nlohmann::json *format_result_json(nlohmann::json *input_json);

        void add_type_set(std::set<std::string> &all_types_set, std::set<std::string> &types_set);

        void add_type_set(std::set<std::string> &all_types_set, std::set<llvm::StructType *> &types_set);

        llvm::Type *get_real_type(llvm::Type *t);
    };
}


#endif //INC_2021_TEMPLATE_MANAGER_H
