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
    // instr_impl[static_cast<InstructionOpcode>(Instruction::BinaryOps::Add)] = new Operation {
    //     .crit_delay = 2.0,
    //     .n_lut = 2,
    // };
}

hardware::HardwareConstraints::~HardwareConstraints() {
    for (auto inst_op_pair : instr_impl) {
        delete inst_op_pair.second;
    }
}