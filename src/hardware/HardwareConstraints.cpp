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
    InstructionOpcode AddOpCode = (InstructionOpcode) Instruction::BinaryOps::Add;
    InstructionOpcode SubOpCode = (InstructionOpcode) Instruction::BinaryOps::Sub;
    InstructionOpcode ICmpOpCode = (InstructionOpcode) Instruction::ICmp;
    InstructionOpcode BrOpCode = (InstructionOpcode) Instruction::Br;
  
    instr_fu_lookup[AddOpCode] = new FunctionalUnit;
    fu_num_constraints[instr_fu_lookup[AddOpCode]] = 1;

    instr_fu_lookup[SubOpCode] = new FunctionalUnit;
    fu_num_constraints[instr_fu_lookup[SubOpCode]] = 1;

    instr_fu_lookup[ICmpOpCode] = new FunctionalUnit;
    fu_num_constraints[instr_fu_lookup[ICmpOpCode]] = 1;

    instr_fu_lookup[BrOpCode] = new FunctionalUnit;

    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Mul] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Or] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Xor] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::And] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::LShr] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::AShr] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Shl] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::CastOps::BitCast] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::CastOps::ZExt] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::UnaryOps::FNeg] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::MemoryOps::Alloca] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::MemoryOps::Load] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::MemoryOps::Store] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::OtherOps::Call] = new FunctionalUnit;
    instr_fu_lookup[(InstructionOpcode) Instruction::TermOps::Ret] = new FunctionalUnit;
}

hardware::HardwareConstraints::~HardwareConstraints() {
    for (auto inst_op_pair : instr_fu_lookup) {
        delete inst_op_pair.second;
    }
}