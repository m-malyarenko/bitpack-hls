#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "RtlOperation.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlOperation::RtlOperation(Instruction* instr)
    : cast_width(false)
{
    if (isa<BinaryOperator>(instr)) {
        switch (instr->getOpcode()) {
            case Instruction::Add:  opcode = Add; break;
            case Instruction::Sub:  opcode = Sub; break;
            case Instruction::Mul:  opcode = Mul; break;
            case Instruction::And:  opcode = And; break;
            case Instruction::Or:   opcode = Or; break;
            case Instruction::Xor:  opcode = Xor; break;
            default: llvm_unreachable("Unsupported binary operator type");
        }
    } else if (isa<ICmpInst>(instr)) {
        auto* cmp_instr = dyn_cast<ICmpInst>(instr);

        switch (cmp_instr->getPredicate()) {
            case ICmpInst::ICMP_EQ:  opcode = Eq; break;
            case ICmpInst::ICMP_NE:  opcode = Ne; break;
            case ICmpInst::ICMP_SLT:
            case ICmpInst::ICMP_ULT: opcode = Lt; break;
            case ICmpInst::ICMP_SLE:
            case ICmpInst::ICMP_ULE: opcode = Le; break;
            case ICmpInst::ICMP_SGT:
            case ICmpInst::ICMP_UGT: opcode = Gt; break;
            case ICmpInst::ICMP_SGE:
            case ICmpInst::ICMP_UGE: opcode = Ge; break;
            default: llvm_unreachable("Invalid comparison type");
        }
    } else {
        llvm_unreachable("Unsupported operation type");
    }
}

rtl::RtlOperation::RtlOperation(Opcode opcode)
    : opcode(opcode),
      cast_width(false) {}

bool rtl::RtlOperation::isConstant() const {
    for (auto idx_operand : operands) {
        auto* operand = idx_operand.second;
        if (!operand->isConstant()) {
            return false;
        }
    }
    return false;
}

void rtl::RtlOperation::setOperand(unsigned int i, RtlSignal* signal) {
    if (operands.empty()) {
        switch (opcode) {
            case Eq:
            case Ne:
            case Lt:
            case Le:
            case Gt:
            case Ge: {
                setWidth(RtlWidth());
                break;
            }
            case Concat: {
                if (!cast_width) {
                    setWidth(signal->getWidth());
                    cast_width = true;
                } else {
                    setWidth(
                        RtlWidth(getWidth().getBitwidth()
                            + signal->getWidth().getBitwidth())
                    );
                }
                break;
            }
            case ZExt:
            case SExt: {
                if (!cast_width) {
                    llvm_unreachable("No cast width for ZExt and SExt");
                }
                break;
            }
            default: {
                setWidth(signal->getWidth());
                break;
            }
        }
    }

    operands[i] = signal;
}

rtl::RtlSignal* rtl::RtlOperation::getOpearnd(unsigned int i) {
    return operands.at(i);
}

unsigned int rtl::RtlOperation::getOpearndsNum() {
    return operands.size();
}

void rtl::RtlOperation::setCastWidth(RtlWidth cast_width) {
    cast_width = true;
    setWidth(cast_width);
}

rtl::RtlOperation::Opcode rtl::RtlOperation::getOpcode() {
    return opcode;
}