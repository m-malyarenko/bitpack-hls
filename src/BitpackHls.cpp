#include <iostream>
#include <string>
#include <optional>

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FormattedStream.h>

#include <llvm/IR/Function.h>

#include "scheduling/Dag.hpp"
#include "scheduling/SdcScheduler.hpp"
#include "scheduling/SchedulerMapping.hpp"
#include "scheduling/fsm/Fsm.hpp"

#include "rtl/RtlModule.hpp"
#include "rtl/RtlGenerator.hpp"

#include "verilog/VerilogWriter.hpp"

#include "BitpackHls.hpp"

using namespace llvm;

bphls::BitpackHls::BitpackHls(Function& function /*, constrains */)
    : function(function),
      rtl_module(std::nullopt) {}

bool bphls::BitpackHls::run() {
    std::string out_buffer;
    raw_string_ostream out_stream(out_buffer);
    formatted_raw_ostream fmt_out_stream(out_stream);

    Dag dag(function);

    for (auto& basic_block: function) {
        dag.exportDot(fmt_out_stream, basic_block);

        std::cout << out_stream.str() << std::endl;
    }

    SdcScheduler sched(function, dag);

    Fsm& fsm = sched.schedule().createFsm();

    for (auto* state : fsm.states()) {
        std::cout << state->getName() << std::endl;
        // for (unsigned int i = 0; i < state->getTransitionsNum(); i++) {
        //     std::cout << "#" << i << "\tTransition state " << state->getTransitionState(i)->getName() << std::endl;
        //     // if (state->getTransitionValue(i) != nullptr) {
        //     //     std::cout << "#" << i << "\tTransition value " << state->getTransitionValue(i)->getName().str() << std::endl;
        //     // }
        // }
        // std::cout << "\tTransition variable: " << state->getTransitionVariable()->getName().str() << std::endl;
        // std::cout << "\tDefaul transition " << state->getDefaultTransition()->getName() << std::endl;

        for (auto* instr : state->instructions()) {
            std::cout << "\t\t" << instr->getOpcodeName() << std::endl;
        }
    }

    rtl::RtlGenerator rtl_gen(function, fsm);

    rtl_module = &rtl_gen.generate();

    std::string hls_output_buffer;
    llvm::raw_string_ostream hls_output(hls_output_buffer);

    verilog::VerilogWriter verilog_write(hls_output, *rtl_module.value());

    verilog_write.print();

    std::cout << hls_output_buffer << std::endl;

    // auto& map = sched_mapping.getMap();

    // for (auto& instr_node_stage : map) {
    //     std::cout << instr_node_stage.first->getInstruction().getOpcodeName()
    //                 << " #" << instr_node_stage.second << std::endl;
    // }

    // Fsm* fsm = mapping->createFsm(function, *dag);

    // out_buffer.clear();

    // fsm->exportDot(fmt_out_stream);

    // std::cout << out_stream.str() << std::endl;

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
    // delete fsm;

    return true;
}

void bphls::BitpackHls::writeOut(raw_ostream& hls_out) {

}