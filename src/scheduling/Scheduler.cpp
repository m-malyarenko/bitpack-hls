#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include "Scheduler.hpp"

using namespace llvm;
using namespace bphls;

unsigned int Scheduler::getInstructionCycles(Instruction& instr) {
    /* TODO Fill in correct data */
    switch (instr.getOpcode()) {
    case Instruction::Store:
        return 1;
    case Instruction::Load:
        return 1;
    case Instruction::Mul:
        return 0;
    case Instruction::Add:
        return 0;
    default:
        return 0;
    }
}