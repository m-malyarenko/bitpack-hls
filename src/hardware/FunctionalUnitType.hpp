#ifndef __HARDWARE_FUNCTIONAL_UNIT_TYPE_HPP__
#define __HARDWARE_FUNCTIONAL_UNIT_TYPE_HPP__

namespace llvm {
    namespace bphls {
        namespace hardware {

class FunctionalUnitType {
public:
    virtual unsigned int getOpCode() = 0;

    virtual unsigned int getOperandsNum() = 0;

    virtual unsigned char getOperandWidth(unsigned int i) = 0;

    virtual unsigned int getOutputWidth() = 0;
};

template<const unsigned int OpNum>
class OperationFunctionalUnit : public FunctionalUnitType {
public:
    OperationFunctionalUnit(unsigned int opcode,
                            unsigned char in_w[OpNum],
                            unsigned char out_w);

    unsigned int getOpCode() override { return opcode; }

    unsigned int getOperandsNum() override { return OpNum; }

    unsigned char getOperandWidth(unsigned int i) override { i < OpNum ? in_width[i] : 0; }

    unsigned int getOutputWidth() override { return out_width; }

private:
    unsigned int opcode;
    unsigned char in_width[OpNum];
    unsigned char out_width;
};

template<const unsigned int OpNum>
OperationFunctionalUnit<OpNum>::OperationFunctionalUnit(unsigned int opcode,
                                unsigned char in_w[OpNum],
                                unsigned char out_w)
    : opcode(opcode),
      out_width(out_w)
{
    for (unsigned int i = 0; i < OpNum; i++) {
        in_width[i] = in_w[i];
    }
}

typedef OperationFunctionalUnit<1> UnaryOperationFu;

typedef OperationFunctionalUnit<2> BinaryOperationFu;

typedef OperationFunctionalUnit<3> TernaryOperationFu;

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_FUNCTIONAL_UNIT_TYPE_HPP__ */
