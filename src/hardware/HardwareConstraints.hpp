#ifndef __HARDWARE_OHARDWARE_CONSTRAINTS_HPP__
#define __HARDWARE_OHARDWARE_CONSTRAINTS_HPP__

#include <map>
#include <optional>
#include <tuple>

#include <llvm/IR/Instruction.h>

#include "Operation.hpp"
#include "FunctionalUnit.hpp"

namespace llvm {
    namespace bphls {
        namespace hardware {

class HardwareConstraints {
public:
    static HardwareConstraints* getHardwareConstraints();

    Operation* getInstructionOperation(Instruction& instr);

    FunctionalUnit* getInstructionFu(Instruction& instr);

    std::optional<unsigned int> getFuNumConstraint(FunctionalUnit& fu);

    ~HardwareConstraints();

    float max_delay;

private:
    static HardwareConstraints* constraints;

    typedef std::tuple<
        unsigned int,   /* Opcode */
        unsigned char   /* Input Width */
    > UnaryOperationDescriptor;

    typedef std::tuple<
        unsigned int,   /* Opcode */
        unsigned char,  /* Input Width 1 */
        unsigned char   /* Input Width 2 */
    > BinaryOperationDescriptor;

    std::map<UnaryOperationDescriptor, FunctionalUnit*> unary_op_fu_lookup;
    std::map<BinaryOperationDescriptor, FunctionalUnit*> binary_op_fu_lookup;

    std::map<Instruction*, FunctionalUnit*> instr_fu_lookup;
    std::map<FunctionalUnit*, std::optional<unsigned int>> fu_num_constraints;

    HardwareConstraints();
};

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_OHARDWARE_CONSTRAINTS_HPP__ */
