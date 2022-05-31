#ifndef __VERILOG_VERILOG_WRITER_HPP__
#define __VERILOG_VERILOG_WRITER_HPP__

#include <llvm/Support/raw_ostream.h>

#include "../rtl/RtlSignal.hpp"
#include "../rtl/RtlModule.hpp"

namespace llvm {
    namespace bphls {
        namespace verilog {

class VerilogWriter {
public:
    VerilogWriter(raw_ostream& out, rtl::RtlModule& rtl_module)
        : out(out),
          rtl_module(rtl_module) {}

    void print();

private:
    raw_ostream& out;
    rtl::RtlModule& rtl_module;

    void printHeader();

    void printBody();

    void printFooter();

    void printSignalDeclaration(rtl::RtlSignal* signal);

    void printSignalDefinition(rtl::RtlSignal* signal);

    void printAssignment(rtl::RtlSignal* signal,
                         rtl::RtlSignal* driver,
                         unsigned int width,
                         bool blocking);

    void printValue(rtl::RtlSignal* signal, unsigned int width = 0, bool zext = false);

    void printConditions(rtl::RtlSignal* signal, bool blocking);

    void printWidth(rtl::RtlWidth width);

    void printBiwidthPrefix(rtl::RtlWidth width);
};

        } /* namespace verilog */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __VERILOG_VERILOG_WRITER_HPP__ */
