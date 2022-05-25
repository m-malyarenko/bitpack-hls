#ifndef __UTILITY_BASIC_BLOCK_UTILITY_HPP__
#define __UTILITY_BASIC_BLOCK_UTILITY_HPP__

#include <llvm/IR/BasicBlock.h>
#include <optional>

namespace llvm {
    namespace bphls {
        namespace utility {

bool isBasicBlockEmpty(BasicBlock& basic_block, BasicBlock*& successor);

bool isBasicBlockEmpty(BasicBlock& basic_block);

        } /* namespace utility */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __UTILITY_BASIC_BLOCK_UTILITY_HPP__ */