#include <optional>

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "RtlWidth.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlWidth::RtlWidth()
    : msb_idx(std::nullopt),
      lsb_idx(0),
      is_signed(false) {}

rtl::RtlWidth::RtlWidth(Type* type, bool is_signed)
    : lsb_idx(0),
      is_signed(is_signed)
{
    assert(!isa<PointerType>(type));
    unsigned char primitive_width = type->getPrimitiveSizeInBits();

    if (primitive_width > 1) {
        msb_idx = primitive_width - 1;
    } else {
        msb_idx = std::nullopt;
    }
}

rtl::RtlWidth::RtlWidth(Value* val, bool is_signed)
    : lsb_idx(0),
      is_signed(is_signed)
{
    auto* type = val->getType();

    assert(!isa<PointerType>(type));
    unsigned char primitive_width = type->getPrimitiveSizeInBits();

    if (primitive_width > 1) {
        msb_idx = primitive_width - 1;
    } else {
        msb_idx = std::nullopt;
    }
}

rtl::RtlWidth::RtlWidth(unsigned char width, bool is_signed)
    : lsb_idx(0),
      is_signed(is_signed)
{
    if (width == 1) {
        msb_idx = std::nullopt;
    } else {
        msb_idx = width - 1;
    }
}

rtl::RtlWidth::RtlWidth(unsigned char msb_idx, unsigned char lsb_idx, bool is_signed)
    : msb_idx(msb_idx),
      lsb_idx(lsb_idx),
      is_signed(is_signed) {}

unsigned char rtl::RtlWidth::getBitwidth() {
    return msb_idx.value_or(0) + 1;
}

std::optional<unsigned char> rtl::RtlWidth::getMsbIndex() {
    return msb_idx;
}

std::optional<unsigned char> rtl::RtlWidth::getLsbIndex() {
    return lsb_idx;
}

bool rtl::RtlWidth::isSigned() {
    return is_signed;
}