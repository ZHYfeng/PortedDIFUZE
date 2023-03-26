//
// Created by yhao on 7/2/21.
//

#include "llvm_related.h"
#include "log.h"

#define DELIMITER '$'

llvm::BasicBlock *get_real_basic_block(llvm::BasicBlock *b) {
    llvm::BasicBlock *rb;
    if (b->hasName()) {
        rb = b;
    } else {
        for (auto *Pred: llvm::predecessors(b)) {
            rb = get_real_basic_block(Pred);
            break;
        }
    }
    return rb;
}

llvm::BasicBlock *get_final_basic_block(llvm::BasicBlock *b) {
    auto *inst = b->getTerminator();
    for (unsigned int i = 0, end = inst->getNumSuccessors(); i < end; i++) {
        std::string name = inst->getSuccessor(i)->getName().str();
        if (inst->getSuccessor(i)->hasName()) {
        } else {
            return get_final_basic_block(inst->getSuccessor(i));
        }
    }
    return b;
}

std::string get_file_name(llvm::Function *f) {
    if (f->hasMetadata()) {
        llvm::SmallVector<std::pair<unsigned, llvm::MDNode *>, 4> MDs;
        f->getAllMetadata(MDs);
        for (auto &MD: MDs) {
            if (llvm::MDNode *N = MD.second) {
                if (auto *SP = llvm::dyn_cast<llvm::DISubprogram>(N)) {
                    std::string Path = SP->getFilename().str();
                    return Path;
                }
            }
        }
    }
    return "";
}

std::string get_real_function_name(llvm::Function *f) {
    std::string name = f->getName().str();
    std::string ret;
    if (name.find('.') != std::string::npos) {
        ret = name.substr(0, name.find('.'));
    } else {
        ret = name;
    }
    return ret;
}

std::string get_real_function_type(llvm::FunctionType *ft) {
    std::string type;
    std::string str;
    int64_t debug = 4;
    auto ret = ft->getReturnType();
    if (ret->isPointerTy() && ret->getPointerElementType()->isStructTy()) {
        yhao_print(debug, ret->print, str)
        str = str.substr(0, str.size()-1);
        if (str.find("struct") == std::string::npos) {
            type += str;
        } else {
            type += (get_structure_name(str) + "*");
        }
    } else if (ret->isStructTy()) {
        yhao_print(debug, ret->print, str)
        if (str.find("struct") == std::string::npos) {
            type += str;
        } else {
            type += get_structure_name(str);
        }
    } else {
        yhao_print(debug, ret->print, str)
        type += str;
    }
    type += " (";
    for (auto i = 0; i < ft->getNumParams(); i++) {
        if (i != 0) {
            type += ", ";
        }
        auto temp = ft->getParamType(i);
        if (temp->isPointerTy() && temp->getPointerElementType()->isStructTy()) {
            yhao_print(debug, temp->print, str)
            str = str.substr(0, str.size()-1);
            if (str.find("struct") == std::string::npos) {
                type += str;
            } else {
                type += (get_structure_name(str) + "*");
            }
        } else if (temp->isStructTy()) {
            yhao_print(debug, temp->print, str)
            if (str.find("struct") == std::string::npos) {
                type += str;
            } else {
                type += get_structure_name(str);
            }
        } else {
            yhao_print(debug, temp->print, str)
            type += str;
        }
    }
    type +=")";
    return type;
}

std::string get_structure_name(const std::string &name) {
    std::string ret;
    uint64_t index_1 = name.find('.');
    if (index_1 != std::string::npos) {
        uint64_t index_2 = name.find('.', index_1 + 1);
        if (index_2 != std::string::npos) {
            ret = name.substr(0, index_2);
        } else {
            ret = name;
        }
    } else {
        ret = name;
        yhao_log(3, "get_structure_name: " + name);
    }
    return ret;
}

void dump_inst(llvm::Instruction *inst) {
    if (inst != nullptr) {
        // inst->dump();
    } else {
        return;
    }
    auto b = inst->getParent();
    auto f = b->getParent();

    unsigned int line = 1;
    std::string Path = get_file_name(f);
    if (inst->hasMetadata()) {
        const llvm::DebugLoc &debugInfo = inst->getDebugLoc();
        if (debugInfo) {
            Path = debugInfo->getFilename().str();
            std::cout << Path << " : ";
            line = debugInfo->getLine();
            unsigned int column = debugInfo->getColumn();
            std::cout << std::dec << line << " : ";
            std::cout << column << " : ";
        }
    }

    std::string FunctionName = get_real_function_name(f);
    std::cout << FunctionName << " : ";
    std::string BasicBlockName = get_real_basic_block(b)->getName().str();
    std::cout << BasicBlockName << " : ";

//        std::string directory = debugInfo->getDirectory().str();
//        std::string filePath = debugInfo->getFilename().str();

    std::cout << std::endl;

    std::cout << "https://elixir.bootlin.com/linux/v5.12/source/" << Path << "#L" << line << std::endl;

}

std::string dump_inst_booltin(llvm::Instruction *inst) {
    std::string res;

    if (inst != nullptr) {
//            inst->dump();
    } else {
        return res;
    }
    auto b = inst->getParent();
    auto f = b->getParent();

    unsigned int line = 1;
    std::string Path = get_file_name(f);
    if (inst->hasMetadata()) {
        const llvm::DebugLoc &debugInfo = inst->getDebugLoc();
        if (debugInfo) {
            Path = debugInfo->getFilename().str();
            line = debugInfo->getLine();
        }
    }

    res += "https://elixir.bootlin.com/linux/v5.12/source/" + Path + "#L" + std::to_string(line);
    return res;
}

std::string real_inst_str(std::string str) {
    auto index = str.find_last_of('#');
    if (index == std::string::npos) {
        return str;
    } else {
        return str.substr(0, index);
    }
}

/// Compute the true target of a function call, resolving LLVM aliases
/// and bitcasts.
llvm::Function *get_target_function(llvm::Value *calledVal) {
    llvm::SmallPtrSet<const llvm::GlobalValue *, 3> Visited;

    auto *c = llvm::dyn_cast<llvm::Constant>(calledVal);
    if (!c)
        return nullptr;

    while (true) {
        if (auto *gv = llvm::dyn_cast<llvm::GlobalValue>(c)) {
            if (!Visited.insert(gv).second)
                return nullptr;

            if (auto *f = llvm::dyn_cast<llvm::Function>(gv))
                return f;
            else if (auto *ga = llvm::dyn_cast<llvm::GlobalAlias>(gv))
                c = ga->getAliasee();
            else
                return nullptr;
        } else if (auto *ce = llvm::dyn_cast<llvm::ConstantExpr>(c)) {
            if (ce->getOpcode() == llvm::Instruction::BitCast)
                c = ce->getOperand(0);
            else
                return nullptr;
        } else
            return nullptr;
    }
}

std::string function_to_strID(llvm::Function *f) {
    std::string ret;
    if (f == nullptr) {
        return ret;
    }
    std::string Path = get_file_name(f);
    std::string NameFunction = f->getName().str();
    ret += Path;
    ret += DELIMITER;
    ret += NameFunction;
    return ret;
}

std::string basicblock_to_strID(llvm::BasicBlock *b) {
    std::string ret;
    if (b == nullptr) {
        return ret;
    }
    auto f = b->getParent();
    ret += function_to_strID(f);
    int64_t No;
    No = 0;
    for (auto &bb: *f) {
        if (b == &bb) {
            break;
        }
        No++;
    }
    int64_t NoBB = No;
    ret += DELIMITER;
    ret += std::to_string(NoBB);
    return ret;
}

std::string inst_to_strID(llvm::Instruction *inst) {
    std::string ret;
    if (inst == nullptr) {
        return ret;
    }
    auto b = inst->getParent();
    ret += basicblock_to_strID(b);
    int64_t No;
    No = 0;
    for (auto &i: *b) {
        if (inst == &i) {
            break;
        }
        No++;
    }
    int64_t NoInst = No;
    ret += DELIMITER;
    ret += std::to_string(NoInst);
    return ret;
}

llvm::Function *strID_to_function(llvm::Module *m, const std::string &str) {
    uint64_t start;
    start = str.find(DELIMITER);
    if (start == str.size()) {
        yhao_log(3, "no function: " + str);
        return nullptr;
    }
    uint64_t end = str.find(DELIMITER, start + 1);
    std::string NameFunction = str.substr(start + 1, end - start - 1);
    llvm::Function *f = m->getFunction(NameFunction);
    if (f == nullptr) {
        yhao_log(3, "bad function name str: " + str);
        yhao_log(3, "bad function name: " + NameFunction);
    }
    return f;
}

llvm::BasicBlock *strID_to_basicblock(llvm::Module *m, const std::string &str) {
    llvm::Function *f = strID_to_function(m, str);
    if (f == nullptr) {
        return nullptr;
    }
    uint64_t start;
    start = str.find(DELIMITER);
    start = str.find(DELIMITER, start + 1);
    if (start == str.size()) {
        yhao_log(3, "no basicblock: " + str);
        return nullptr;
    }
    uint64_t end = str.find(DELIMITER, start + 1);
    std::string NoBB = str.substr(start + 1, end - start - 1);
    int64_t No = std::stoi(NoBB);
    llvm::BasicBlock *b = nullptr;
    for (auto &bb: *f) {
        if (No == 0) {
            b = &bb;
            break;
        }
        No--;
    }
    return b;
}

llvm::Instruction *strID_to_inst(llvm::Module *m, const std::string &str) {
    llvm::BasicBlock *b = strID_to_basicblock(m, str);
    if (b == nullptr) {
        return nullptr;
    }
    uint64_t start;
    start = str.find(DELIMITER);
    start = str.find(DELIMITER, start + 1);
    start = str.find(DELIMITER, start + 1);
    if (start == str.size()) {
        yhao_log(3, "no inst: " + str);
        return nullptr;
    }
    uint64_t end = str.find(DELIMITER, start + 1);
    std::string NoInst = str.substr(start + 1, end - start - 1);
    int64_t No = std::stoi(NoInst);
    llvm::Instruction *i;
    for (auto &ii: *b) {
        if (No == 0) {
            i = &ii;
            break;
        }
        No--;
    }
    return i;
}
