#include <string>
#include <map>

#include <llvm/Support/raw_ostream.h>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include "../rtl/RtlSignal.hpp"
#include "../rtl/RtlOperation.hpp"
#include "verilog_utility.hpp"

using namespace llvm;
using namespace bphls;

std::string utility::getVerilogName(Value* val) {
    static unsigned int var_count = 0;

    auto name = getLabel(val);

    if (name.empty()) {
        name = "var_" + std::to_string(var_count);
        var_count++; 
    }
    
    if (isa<Instruction>(val)) {
        /* Name of instruction -> wire/reg verilog name */
        auto* instr = dyn_cast<Instruction>(val);
        auto* basic_block = instr->getParent();

        assert(basic_block != nullptr);

        auto basic_block_label = getLabel(basic_block);
        auto function_label = basic_block->getParent()->getName().str();

        name = function_label + "_" + basic_block_label + "_" + name;
    } else if (isa<Argument>(val)) {
        /* Name of function argument -> int port verilog name */
        name = "arg_" + name;
    }

   std::string forbidden_chars = "%/.@#'`";

    for (char c : forbidden_chars) {
        name.erase (std::remove(name.begin(), name.end(), c), name.end());
    }

    return name;

    // TODO Unique names tracing
}

std::string utility::getLabel(Value* val) {
    static std::map<Value*, std::string> label_cache;

    if (val == nullptr) {
        return std::string();
    }

    if (label_cache.find(val) != label_cache.end()) {
        return label_cache[val];
    }

    std::string label_buffer;
    llvm::raw_string_ostream out(label_buffer);

    /* Get label */
    val->printAsOperand(out, false);

    return label_buffer;
}

bool utility::isNumeric(std::string val_string) {
    if (val_string.empty()) {
        return false;
    }

    for (std::size_t i = 0; i < val_string.length(); i++) {
        if (!std::isdigit(val_string[i])) {
            return false;
        }
    }

    return true;
}

void utility::getConditionStateName(rtl::RtlSignal* condition, std::string& name) {
    if ((condition == nullptr) || !condition->isOperation()) {
        return;
    }

    auto* operation = static_cast<rtl::RtlOperation*>(condition);

    assert(operation->getOperandsNum() >= 2);

    bool is_op_0_operation = operation->getOperand(0)->isOperation(); 
    bool is_op_1_operation = operation->getOperand(1)->isOperation(); 

    if (is_op_0_operation) {
        getConditionStateName(operation->getOperand(0), name);
    }

    if (is_op_1_operation) {
        getConditionStateName(operation->getOperand(1), name);
    }

    if (!is_op_0_operation
            && !is_op_1_operation
            && operation->getOperand(0)->getName().value_or("") == "cur_state")
    {
        assert(operation->getOperand(1)->getName().has_value());
        name = operation->getOperand(1)->getName().value();
    }
}

void utility::getCaseClauseConditions(rtl::RtlSignal* condition,
                             std::vector<rtl::RtlSignal*>& out_clauses)
{
    if (!condition->isOperation()) {
        return;
    }

    auto* operation = static_cast<rtl::RtlOperation*>(condition);

    assert(operation->getOperandsNum() >= 2);

    bool is_op_0_operation = operation->getOperand(0)->isOperation();
    bool is_op_1_operation = operation->getOperand(1)->isOperation();

    if (is_op_0_operation) {
        getCaseClauseConditions(operation->getOperand(0), out_clauses);
    }

    if (is_op_1_operation) {
        getCaseClauseConditions(operation->getOperand(1), out_clauses);
    }

    if (!is_op_0_operation && !is_op_1_operation) {
        if (operation->getOperand(0)->getName().value_or("") != "cur_state") {
            out_clauses.push_back(condition);
        }
    }
}
