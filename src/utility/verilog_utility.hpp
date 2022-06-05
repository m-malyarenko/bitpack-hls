#ifndef __UTILITY_VERILOG_UTILITY_HPP__
#define __UTILITY_VERILOG_UTILITY_HPP__

#include <string>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Instruction.h>

#include "../rtl/RtlSignal.hpp"

namespace llvm {
    namespace bphls {
        namespace utility {

std::string getVerilogName(Value* val);

std::string getFuInstVerilogName(Instruction* instr, unsigned int idx);

std::string getLabel(Value* val);

bool isNumeric(std::string val_string);

void getConditionStateName(rtl::RtlSignal* condition, std::string& name);

void getCaseClauseConditions(rtl::RtlSignal* condition,
                             std::vector<rtl::RtlSignal*>& out_clauses);

        } /* namespace utility */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __UTILITY_VERILOG_UTILITY_HPP__ */
