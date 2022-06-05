#include <iostream>
#include <fstream>
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

#include "binding/LifetimeAnalysis.hpp"
#include "binding/Binding.hpp"

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

        std::ofstream file_out("./hls/out/dag.dot");
        file_out << out_buffer;
    }

    SdcScheduler sched(function, dag);

    Fsm& fsm = sched.schedule().createFsm();

    for (auto* state : fsm.states()) {
        std::cout << state->getName() << std::endl;
        state->printStateInfo();
    }

    binding::LifetimeAnalysis lva(function);
    lva.analize();

    binding::Binding binding(fsm, lva);

    rtl::RtlGenerator rtl_gen(function, fsm, lva, binding);

    rtl_module = &rtl_gen.generate();

    std::string hls_output_buffer;
    llvm::raw_string_ostream hls_output(hls_output_buffer);

    verilog::VerilogWriter verilog_write(hls_output, *rtl_module.value());

    verilog_write.print();

    {
        std::ofstream file_out("./hls/out/" + function.getName().str() + ".v");
        file_out << hls_output_buffer;
    }

    return true;
}

void bphls::BitpackHls::writeOut(raw_ostream& hls_out) {

}