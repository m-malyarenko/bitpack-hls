#ifndef __UTILITY_INSTRUCTION_UTILITY_HPP__
#define __UTILITY_INSTRUCTION_UTILITY_HPP__

#include <llvm/IR/Value.h>

namespace llvm {
    namespace bphls {
        namespace utility {

bool isDummyCall(Instruction& instr);

Value* getPointerOperand(Instruction& instr);

        } /* namespace utility */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __UTILITY_INSTRUCTION_UTILITY_HPP__ */
