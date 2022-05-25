#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "instruction_utility.hpp"

using namespace llvm;
using namespace bphls;

bool utility::isDummyCall(Instruction& instr) {
    const auto* caller = dyn_cast<CallInst>(&instr);
    if (caller == nullptr) {
        return false;
    }

    const auto* callee = caller->getCalledFunction();
    if (callee == nullptr) {
        return false;
    }

    std::string callee_name = callee->getName().str();

    const bool is_dummy_call_name =
        callee_name == "exit"
            || callee_name == "llvm.lifetime.start"
            || callee_name == "llvm.lifetime.end"
            || callee_name == "llvm.invariant.start"
            || callee_name == "llvm.invariant.end"
            || callee_name == "llvm.dbg.declare"
            || callee_name == "llvm.dbg.value";

    return is_dummy_call_name;
}

Value* utility::getPointerOperand(Instruction& instr) {
    if (auto* load_instr = dyn_cast<LoadInst>(&instr)) {
        return load_instr->getPointerOperand();
    }

    if (auto* store_instr = dyn_cast<StoreInst>(&instr)) {
        return store_instr->getPointerOperand();
    }

    return nullptr;
}