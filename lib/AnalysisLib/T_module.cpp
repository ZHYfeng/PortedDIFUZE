//
// Created by yhao on 3/16/21.
//

#include "T_module.h"

#define DEBUG_CHECK -1

lddt::T_module::T_module() {}

lddt::T_module::~T_module() = default;

void lddt::T_module::read_bitcode(const std::string &BitcodeFileName) {
    this->bitcode = BitcodeFileName;
    llvm::LLVMContext *cxts;
    llvm::SMDiagnostic Err;
    cxts = new llvm::LLVMContext[1];
    llvm_module = llvm::parseIRFile(BitcodeFileName, Err, cxts[0]);
    if (!llvm_module) {
        exit(0);
    } else {

    }
}
