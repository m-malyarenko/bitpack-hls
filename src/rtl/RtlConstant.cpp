#include <string>

#include "RtlWidth.hpp"
#include "RtlConstant.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlConstant::RtlConstant(std::string value, RtlWidth width)
    : RtlSignal(std::nullopt, value, std::nullopt, width) {};