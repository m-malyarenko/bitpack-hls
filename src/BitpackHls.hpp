#ifndef __BITPACK_HLS_PASS_HPP__
#define __BITPACK_HLS_PASS_HPP__

#include <optional>

#include <llvm/Support/raw_ostream.h>

#include <llvm/IR/Function.h>   /* Compiler input */
#include "rtl/RtlModule.hpp"    /* Compiler output */

namespace llvm {
    namespace bphls {

class BitpackHls {
public:
    BitpackHls(Function& function /*, constrains */);

    bool run();

    void writeOut(raw_ostream& hls_output);

private:
    Function& function;
    std::optional<rtl::RtlModule*> rtl_module; 
};

    } /* bphls */
} /* namespace llvm */

#endif /* __BITPACK_HLS_PASS_HPP__ */
