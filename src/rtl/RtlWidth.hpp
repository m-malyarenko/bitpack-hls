#ifndef __RTL_RTL_WIDTH_HPP__
#define __RTL_RTL_WIDTH_HPP__

#include <optional>

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlWidth {
public:
    RtlWidth();

    RtlWidth(Type* type, bool is_signed = false);

    RtlWidth(Value* val, bool is_signed = false);

    RtlWidth(unsigned char msb_idx, unsigned char lsb_idx, bool is_signed = false);

    RtlWidth(unsigned char width, bool is_signed = false);

    unsigned char getBitwidth();

    std::optional<unsigned char> getMsbIndex();

    std::optional<unsigned char> getLsbIndex();

    bool isSigned();

private:
    std::optional<unsigned char> msb_idx;
    std::optional<unsigned char> lsb_idx;
    bool is_signed;
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */


#endif /* __RTL_RTL_WIDTH_HPP__ */