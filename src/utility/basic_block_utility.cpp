#include <optional>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "basic_block_utility.hpp"

using namespace llvm;
using namespace bphls;

bool utility::isBasicBlockEmpty(BasicBlock& basic_block, BasicBlock*& successor) {
    if (basic_block.size() != 1) {
        return false;
    }

    auto& firt_basic_block = basic_block.getParent()->front();
    if (&firt_basic_block != &basic_block) {
        return false;
    }

    auto* term_instr = basic_block.getTerminator();
    assert(term_instr != nullptr);

    if (isa<ReturnInst>(term_instr) || isa<BranchInst>(term_instr)) {
        return false;
    }

    if (term_instr->getNumSuccessors() == 1) {
        BasicBlock* succ = dyn_cast<BasicBlock>(term_instr->getSuccessor(0));
        assert(succ != nullptr);
        successor = succ;
        return true;
    } else {
        return false;
    }
}

bool utility::isBasicBlockEmpty(BasicBlock& basic_block) {
    BasicBlock* dummy = nullptr;
    return isBasicBlockEmpty(basic_block, dummy);
}