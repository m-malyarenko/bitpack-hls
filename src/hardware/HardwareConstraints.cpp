#include <iostream>

#include <map>
#include <tuple>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>

#include "Operation.hpp"
#include "FunctionalUnit.hpp"
#include "HardwareConstraints.hpp"

using namespace llvm;
using namespace bphls;

hardware::HardwareConstraints* hardware::HardwareConstraints::constraints = nullptr;

hardware::HardwareConstraints* hardware::HardwareConstraints::getHardwareConstraints() {
    return constraints == nullptr ? new HardwareConstraints() : constraints;
}

hardware::FunctionalUnit* hardware::HardwareConstraints::getInstructionFu(Instruction& instr) {
    switch (instr.getNumOperands()) {
    case 1:
    {
        auto bw = instr.getOperand(0)->getType()->getPrimitiveSizeInBits();

        UnaryOperationDescriptor descr = std::make_tuple(
            instr.getOpcode(),
            bw
        );
        return unary_op_fu_lookup[descr];
    }
    case 2:
    {
        auto bw_0 = instr.getOperand(0)->getType()->getPrimitiveSizeInBits();
        auto bw_1 = instr.getOperand(1)->getType()->getPrimitiveSizeInBits();
        BinaryOperationDescriptor descr = std::make_tuple(
            instr.getOpcode(),
            bw_0,
            bw_1
        );
        return binary_op_fu_lookup[descr];
    }
    default:
        return nullptr;
    }
}

std::optional<unsigned int> hardware::HardwareConstraints::getFuNumConstraint(FunctionalUnit& fu) {
    return fu_num_constraints[&fu];
}

std::map<hardware::FunctionalUnit*, std::optional<unsigned int>>&
hardware::HardwareConstraints::getFuNumConstraints() {
    return fu_num_constraints;
}

hardware::Operation* hardware::HardwareConstraints::getInstructionOperation(Instruction& instr) {
    // FIXME Mock return
    return &(*binary_op_fu_lookup.begin()).second->op;
}

hardware::HardwareConstraints::HardwareConstraints() {
    /* Functional Units Types */
    static unsigned char bin_i_width[4][4][2] = {
        {{ 8, 8}, { 8, 16}, { 8, 32}, { 8, 64}},
        {{16, 8}, {16, 16}, {16, 32}, {16, 64}},
        {{32, 8}, {32, 16}, {32, 32}, {32, 64}},
        {{64, 8}, {64, 16}, {64, 32}, {64, 64}},
    };

    auto add_32_32 = bin_op(Instruction::Add, 32, 32);
    auto mul_32_32 = bin_op(Instruction::Mul, 32, 32);
    auto and_32_32 = bin_op(Instruction::And, 32, 32);
    auto or_32_32 = bin_op(Instruction::Or, 32, 32);
    auto cmp_32_32 = bin_op(Instruction::ICmp, 32, 32);

    binary_op_fu_lookup[add_32_32] = new FunctionalUnit;
    binary_op_fu_lookup[mul_32_32] = new FunctionalUnit;
    binary_op_fu_lookup[and_32_32] = new FunctionalUnit;
    binary_op_fu_lookup[or_32_32] = new FunctionalUnit;
    binary_op_fu_lookup[cmp_32_32] = new FunctionalUnit;

    fu_num_constraints[binary_op_fu_lookup[add_32_32]] = 1;
    fu_num_constraints[binary_op_fu_lookup[mul_32_32]] = 1;
    fu_num_constraints[binary_op_fu_lookup[and_32_32]] = 1;
    fu_num_constraints[binary_op_fu_lookup[or_32_32]] = 1;
    fu_num_constraints[binary_op_fu_lookup[cmp_32_32]] = 1;

    auto add_16_16 = bin_op(Instruction::Add, 16, 16);
    auto mul_16_16 = bin_op(Instruction::Mul, 16, 16);
    auto and_16_16 = bin_op(Instruction::And, 16, 16);
    auto or_16_16 = bin_op(Instruction::Or, 16, 16);
    auto cmp_16_16 = bin_op(Instruction::ICmp, 16, 16);

    binary_op_fu_lookup[add_16_16] = new FunctionalUnit;
    binary_op_fu_lookup[mul_16_16] = new FunctionalUnit;
    binary_op_fu_lookup[and_16_16] = new FunctionalUnit;
    binary_op_fu_lookup[or_16_16] = new FunctionalUnit;
    binary_op_fu_lookup[cmp_16_16] = new FunctionalUnit;

    fu_num_constraints[binary_op_fu_lookup[add_16_16]] = 2;
    fu_num_constraints[binary_op_fu_lookup[mul_16_16]] = 1;
    fu_num_constraints[binary_op_fu_lookup[and_16_16]] = 1;
    fu_num_constraints[binary_op_fu_lookup[or_16_16]] = 1;
    fu_num_constraints[binary_op_fu_lookup[cmp_16_16]] = 1;

    auto add_8_8 = bin_op(Instruction::Add, 8, 8);
    auto mul_8_8 = bin_op(Instruction::Mul, 8, 8);
    auto and_8_8 = bin_op(Instruction::And, 8, 8);
    auto or_8_8 = bin_op(Instruction::Or, 8, 8);
    auto cmp_8_8 = bin_op(Instruction::ICmp, 8, 8);

    binary_op_fu_lookup[add_8_8] = new FunctionalUnit;
    binary_op_fu_lookup[mul_8_8] = new FunctionalUnit;
    binary_op_fu_lookup[and_8_8] = new FunctionalUnit;
    binary_op_fu_lookup[or_8_8] = new FunctionalUnit;
    binary_op_fu_lookup[cmp_8_8] = new FunctionalUnit;

    fu_num_constraints[binary_op_fu_lookup[add_8_8]] = 1;
    fu_num_constraints[binary_op_fu_lookup[mul_8_8]] = 1;
    fu_num_constraints[binary_op_fu_lookup[and_8_8]] = 1;
    fu_num_constraints[binary_op_fu_lookup[or_8_8]] = 1;
    fu_num_constraints[binary_op_fu_lookup[cmp_8_8]] = 1;

//     InstructionOpcode AddOpCode = (InstructionOpcode) Instruction::BinaryOps::Add;
//     InstructionOpcode SubOpCode = (InstructionOpcode) Instruction::BinaryOps::Sub;
//     InstructionOpcode ICmpOpCode = (InstructionOpcode) Instruction::ICmp;
//     InstructionOpcode BrOpCode = (InstructionOpcode) Instruction::Br;
  
//     instr_fu_lookup[AddOpCode] = new FunctionalUnit;
//     fu_num_constraints[instr_fu_lookup[AddOpCode]] = 1;

//     instr_fu_lookup[SubOpCode] = new FunctionalUnit;
//     fu_num_constraints[instr_fu_lookup[SubOpCode]] = 1;

//     instr_fu_lookup[ICmpOpCode] = new FunctionalUnit;
//     fu_num_constraints[instr_fu_lookup[ICmpOpCode]] = 1;

//     instr_fu_lookup[BrOpCode] = new FunctionalUnit;

//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Mul] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Or] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Xor] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::And] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::LShr] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::AShr] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::BinaryOps::Shl] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::CastOps::BitCast] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::CastOps::ZExt] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::UnaryOps::FNeg] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::MemoryOps::Alloca] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::MemoryOps::Load] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::MemoryOps::Store] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::OtherOps::Call] = new FunctionalUnit;
//     instr_fu_lookup[(InstructionOpcode) Instruction::TermOps::Ret] = new FunctionalUnit;
}

hardware::HardwareConstraints::~HardwareConstraints() {
    for (auto inst_op_pair : instr_fu_lookup) {
        delete inst_op_pair.second;
    }
}