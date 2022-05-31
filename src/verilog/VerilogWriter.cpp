#include <llvm/Support/raw_ostream.h>

#include "../utility/verilog_utility.hpp"
#include "VerilogWriter.hpp"

using namespace llvm;
using namespace bphls;

void verilog::VerilogWriter::print() {
    printHeader();

    printBody();

    printFooter();
}

void verilog::VerilogWriter::printHeader() {
    auto module_name = rtl_module.getName();

    /* Module name */
    out << "module " << module_name << "\n";

    /* Ports */
    out << "(" << "\n";

    auto port_iter = rtl_module.iter_ports().begin();
    auto port_iter_end = rtl_module.iter_ports().end();

    for (; port_iter != port_iter_end; port_iter++) {
        out << "\t";
        printSignalDeclaration(*port_iter);
    
        if (port_iter != (port_iter_end - 1)) {
            out << ",";
        }
    
        out << "\n";
    }

    out << ");" << "\n\n";

    /* Parameters declaration */
    for (auto* param : rtl_module.iter_params()) {
        printSignalDeclaration(param);
        out << ";\n";
    }
    out << "\n";

    /* Signals declaration */
    for (auto* signal : rtl_module.iter_signals()) {
        printSignalDeclaration(signal);
        out << "\n";
    }
    out << "\n";
}

void verilog::VerilogWriter::printBody() {
    for (auto* signal : rtl_module.iter_signals()) {
        printSignalDeclaration(signal);
    }
}

void verilog::VerilogWriter::printFooter() {
    out << "endmodule" << "\n";
}

void verilog::VerilogWriter::printSignalDeclaration(rtl::RtlSignal* signal) {
    auto type = signal->getType().value_or("unknown");

    /* Type */
    if (type == "wire"
            && (signal->getConditionsNum() != 0)
            && (signal->getDriversNum() != 0))
    {
        out << "reg ";
    } else if (type == "output"
                && (signal->getConditionsNum() != 0)
                && (signal->getDriversNum() != 0))
    {
        out << "output reg ";
    } else {
        out << type << " ";
    }

    /* Width */
    printWidth(signal->getWidth());
    out << " ";

    /* Name */
    out << signal->getName().value_or("unknown");

    /* Special case for FSM state signal */
    if (signal->getName() == "cur_state") {
        out << ";\n";
        out << "reg ";
        printWidth(signal->getWidth());
        out << "next_state";
    }

    /* Value */
    if (signal->getValue().has_value()) {
        out << " = ";

        auto value = signal->getValue().value(); 
        if (utility::isNumeric(value)) {
            printBiwidthPrefix(signal->getWidth());
            out << "d" << value;
        } else {
            out << value;
        }
    }

    // TODO 'Keep signal' wire
}

void verilog::VerilogWriter::printSignalDefinition(rtl::RtlSignal* signal) {
    if (signal->getDriversNum() == 0) {
        return;
    }

    bool is_register = signal->isRegister();
    bool is_block_assign = !is_register;
    unsigned char width = signal->getWidth().getBitwidth();

    if (is_register) {
        out << "allways @ (posedge clk) begin\n";
    } else {
        out << "allways @ (*) begin\n";
    }

    if (signal->getDefaultDriver() != nullptr) {
        auto* default_driver = signal->getDefaultDriver();

        printAssignment(signal, default_driver, width, is_block_assign);
    }

    if (signal->getConditionsNum() == 0) {
        auto* driver = signal->getDriver(0);
        assert(driver != nullptr);

        printAssignment(signal, driver, width, is_block_assign);

        /* Allways trigger on const */
        if (driver->isConstant()) {
            out << "if (reset) begin\n";
            out << "\t" << driver->getName().value_or("unknown") << " = 0;\n";
            out << "end\n";
        }
    } else {
        printConditions(signal, is_block_assign);
    }

    out << "end\n";
}

void verilog::VerilogWriter::printAssignment(rtl::RtlSignal* signal,
                                             rtl::RtlSignal* driver,
                                             unsigned int width,
                                             bool blocking)
{
    static std::string assign_non_block = "<=";
    static std::string assign_block = "=";

    std::string& assign = blocking ? assign_block : assign_non_block;

    out << "\t" << signal->getName().value_or("unknown");
    out << " " << assign << " ";
    printValue(driver, width);
    out << ";\n";
}

void verilog::VerilogWriter::printValue(rtl::RtlSignal* signal, unsigned int width = 0, bool zext = false) {
    assert(signal != nullptr);

    if (signal->isOperation()) {

    } else {
        auto type = signal->getType().value_or("");

        if (type == "parameter") {

        } else {

        }
    }
}

void verilog::VerilogWriter::printConditions(rtl::RtlSignal* signal, bool blocking) {
    // TODO Implement method
}

void verilog::VerilogWriter::printWidth(rtl::RtlWidth width) {
    if (width.getBitwidth() > 1) {
        assert(width.getMsbIndex().has_value());
        assert(width.getLsbIndex().has_value());

        out << "[" << std::to_string(width.getMsbIndex().value()) << ":"
                   << std::to_string(width.getLsbIndex().value()) << "]";
    }
}

void verilog::VerilogWriter::printBiwidthPrefix(rtl::RtlWidth width) {
    out << width.getBitwidth() << "'";
}
