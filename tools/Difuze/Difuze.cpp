//
// Created by yhao on 3/3/21.
//

#include "../../lib/ToolLib/basic.h"
#include "../../lib/ToolLib/log.h"
#include "../../lib/ManagerLib/Manager.h"

llvm::cl::opt<std::string> bitcode("bitcode",
                                       llvm::cl::desc("The bitcode file."),
                                       llvm::cl::value_desc("file name"),
                                       llvm::cl::init("./built-in.bc"));

int main(int argc, char **argv) {
    llvm::cl::ParseCommandLineOptions(argc, argv, "");
    start_log(argv);

    auto manager = new lddt::Manager();
    manager->start(bitcode);
    manager->test_difuze();

    end_log();
    return 0;
}