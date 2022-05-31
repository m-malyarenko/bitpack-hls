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
        std::cout << instr.getOpcodeName() << " BW_0: " << bw << std::endl;
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
        std::cout << instr.getOpcodeName() << " BW_0: "
            << bw_0 << " BW_1: " << bw_1 << std::endl;
        return binary_op_fu_lookup[descr];
    }
    default:
        return nullptr;
    }
}

std::optional<unsigned int> hardware::HardwareConstraints::getFuNumConstraint(FunctionalUnit& fu) {
    return fu_num_constraints[&fu];
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

    binary_op_fu_lookup[std::make_tuple(Instruction::Add, 32, 32)] = new FunctionalUnit;

    binary_op_fu_lookup[std::make_tuple(Instruction::Sub, 32, 32)] = new FunctionalUnit;

    auto* mul_fu = new FunctionalUnit;
    binary_op_fu_lookup[std::make_tuple(Instruction::Mul, 32, 32)] = mul_fu;

    fu_num_constraints[mul_fu] = 1;

    binary_op_fu_lookup[std::make_tuple(Instruction::And, 32, 32)] = new FunctionalUnit;

    binary_op_fu_lookup[std::make_tuple(Instruction::Or, 32, 32)] = new FunctionalUnit;

    binary_op_fu_lookup[std::make_tuple(Instruction::Xor, 32, 32)] = new FunctionalUnit;

    binary_op_fu_lookup[std::make_tuple(Instruction::ICmp, 32, 32)] = new FunctionalUnit;

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