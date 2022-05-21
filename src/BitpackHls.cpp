#include <iostream>
#include <string>

#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>

#include "scheduling/Dag.hpp"
#include "BitpackHls.hpp"

using namespace llvm;

bphls::BitpackHls::BitpackHls(Function& function /*, constrains */)
    : function(function) {}

bool bphls::BitpackHls::run() {
    // std::string hls_output_buffer;
    // llvm::raw_string_ostream hls_output(hls_output_buffer);

    auto& basic_block_list = function.getBasicBlockList();

    if (basic_block_list.size() != 1) {
        std::cerr << "Function must contain one basic block" << std::endl;
        return false;
    }

    auto& basic_block = basic_block_list.front();

    Dag dag;
    const bool dag_status = dag.create(basic_block);

    // for (auto& instr : bb) {
    //     hls_output << instr.getOpcodeName() << "\n\t";

    //     std::size_t oper_num = instr.getNumOperands();

    //     for (std::size_t i = 0; i < oper_num; i++) {
    //         auto oper = instr.getOperand(i);
    //         auto type = oper->getType();

    //         auto int_type = llvm::dyn_cast<llvm::IntegerType>(type);

    //         if (int_type != nullptr) {
    //             hls_output << *int_type
    //                         << " Bitwidth: " << int_type->getBitWidth()
    //                         << "\n\t";
    //         } else {
    //             hls_output << *type << "\n\t";
    //         }
    //     }

    //     hls_output << "\n";
    // }

    // std::cout << hls_output.str() << std::endl;

    return dag_status;
}

void bphls::BitpackHls::writeOut(raw_ostream& hls_out) {}