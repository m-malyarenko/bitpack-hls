#ifndef __RTL_RTL_OPERATION_HPP__
#define __RTL_RTL_OPERATION_HPP__

#include <map>

#include <llvm/IR/Instruction.h>

#include "RtlSignal.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlOperation : public RtlSignal {
public:
    enum Opcode {
        Add,    /* Add */
        Sub,    /* Substract */
        Mul,    /* Multply */
        Or,     /* Bitwise OR */
        Xor,    /* Bitwise XOR */
        And,    /* Bitwise AND */
        Eq,     /* == */
        Ne,     /* != */
        Lt,     /* < */
        Le,     /* <= */
        Gt,     /* > */
        Ge,     /* >= */
        SExt,   /* Signed extension */
        ZExt,   /* Zero extension */
        Concat, /* Bitwise concatenation */
    };

    RtlOperation(Instruction* instr);

    RtlOperation(Opcode opcode);

    bool isConstant() const override;

    bool isOperation() const override { return true; }

    void setOperand(unsigned int i, RtlSignal* signal);

    RtlSignal* getOpearnd(unsigned int i);

    void setCastWidth(RtlWidth cast_width);

    RtlWidth getCastWidth();

    unsigned int getOpearndsNum();

    Opcode getOpcode();

private:
    Opcode opcode;
    bool cast_width;

    std::map<unsigned int, RtlSignal*> operands;
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_RTL_OPERATION_HPP__ */
