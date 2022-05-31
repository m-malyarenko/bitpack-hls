#ifndef __RTL_RTL_CONSTANT_HPP__
#define __RTL_RTL_CONSTANT_HPP__

#include <string>

#include "RtlWidth.hpp"
#include "RtlSignal.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlConstant : public RtlSignal {
public:
    RtlConstant(std::string value, RtlWidth width);

    bool isConstant() const override { return true; }
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_RTL_CONSTANT_HPP__ */
