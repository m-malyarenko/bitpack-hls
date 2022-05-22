#include <map>

#include <llvm/IR/Instruction.h>

#include "HardwareConstraints.hpp"

using namespace llvm;
using namespace bphls;

hardware::HardwareConstraints* hardware::HardwareConstraints::constraints = nullptr;

hardware::HardwareConstraints* hardware::HardwareConstraints::getHardwareConstraints() {
    return constraints == nullptr ? new HardwareConstraints() : constraints;
}

hardware::HardwareConstraints::HardwareConstraints() {
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Add] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Sub] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Mul] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Or] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Xor] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::And] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::LShr] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::AShr] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Shl] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::CastOps::BitCast] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::CastOps::ZExt] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::UnaryOps::FNeg] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::MemoryOps::Alloca] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::MemoryOps::Load] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::MemoryOps::Store] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::OtherOps::Call] = new Operation;
    instr_impl[(InstructionOpcode) Instruction::TermOps::Ret] = new Operation;
}

hardware::HardwareConstraints::~HardwareConstraints() {
    for (auto inst_op_pair : instr_impl) {
        delete inst_op_pair.second;
    }
}