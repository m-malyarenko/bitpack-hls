#include <optional>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "basic_block_utility.hpp"

using namespace llvm;
using namespace bphls;

std::optional<BasicBlock*> utility::isBasicBlockEmpty(BasicBlock& basic_block) {
    if (basic_block.size() != 1) {
        return std::nullopt;
    }

    auto& firt_basic_block = basic_block.getParent()->front();
    if (&firt_basic_block != &basic_block) {
        return std::nullopt;
    }

    auto* term_instr = basic_block.getTerminator();
    assert(term_instr);

    if (isa<ReturnInst>(term_instr) || isa<BranchInst>(term_instr)) {
        return std::nullopt;
    }

    if (term_instr->getNumSuccessors() == 1) {
        BasicBlock* succ = dyn_cast<BasicBlock>(term_instr->getSuccessor(0));
        assert(succ);
        return succ;
    } else {
        return std::nullopt;
    }
}
