#ifndef __BITPACK_HLS_PASS_HPP__
#define __BITPACK_HLS_PASS_HPP__

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

namespace llvm {
    namespace bphls {

class BitpackHls {
public:
    BitpackHls(Function& function /*, constrains */);

    bool run();

    void writeOut(raw_ostream& hls_output);

private:
    Function& function;
};

    } /* bphls */
} /* namespace llvm */

#endif /* __BITPACK_HLS_PASS_HPP__ */
