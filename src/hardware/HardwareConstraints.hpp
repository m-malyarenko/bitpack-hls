#ifndef __HARDWARE_OHARDWARE_CONSTRAINTS_HPP__
#define __HARDWARE_OHARDWARE_CONSTRAINTS_HPP__

#include <map>
#include <optional>

#include <llvm/IR/Instruction.h>
#include "FunctionalUnit.hpp"

namespace llvm {
    namespace bphls {
        namespace hardware {

class HardwareConstraints {
public:
    static HardwareConstraints* getHardwareConstraints();

    ~HardwareConstraints();

    typedef unsigned int InstructionOpcode;

    float max_delay;
    std::map<InstructionOpcode, FunctionalUnit*> instr_impl;
    std::map<FunctionalUnit*, std::optional<unsigned int>> fu_num_constraints;

private:
    static HardwareConstraints* constraints;

    HardwareConstraints();
};

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_OHARDWARE_CONSTRAINTS_HPP__ */
