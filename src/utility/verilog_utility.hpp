#ifndef __UTILITY_VERILOG_UTILITY_HPP__
#define __UTILITY_VERILOG_UTILITY_HPP__

#include <string>

#include <llvm/IR/Value.h>

namespace llvm {
    namespace bphls {
        namespace utility {

std::string getVerilogName(Value* val);

std::string getLabel(Value* val);

bool isNumeric(std::string val_string);

        } /* namespace utility */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __UTILITY_VERILOG_UTILITY_HPP__ */
