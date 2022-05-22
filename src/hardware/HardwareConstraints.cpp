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
    InstructionOpcode AddOpCode
        = static_cast<InstructionOpcode>(Instruction::BinaryOps::Add);  
    instr_impl[AddOpCode] = new FunctionalUnit;
    fu_num_constraints[instr_impl[AddOpCode]] = 2;

    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Sub] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Mul] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Or] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Xor] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::And] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::LShr] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::AShr] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::BinaryOps::Shl] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::CastOps::BitCast] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::CastOps::ZExt] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::UnaryOps::FNeg] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::MemoryOps::Alloca] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::MemoryOps::Load] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::MemoryOps::Store] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::OtherOps::Call] = new FunctionalUnit;
    instr_impl[(InstructionOpcode) Instruction::TermOps::Ret] = new FunctionalUnit;
}

hardware::HardwareConstraints::~HardwareConstraints() {
    for (auto inst_op_pair : instr_impl) {
        delete inst_op_pair.second;
    }
}