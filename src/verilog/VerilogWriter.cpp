#include <iostream>
#include <map>

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

    /* Port declaration */
    out << "(\n";

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

    out << ");\n\n";

    /* Parameters declaration */
    for (auto* param : rtl_module.iter_params()) {
        printSignalDeclaration(param);
        out << ";\n";
    }
    out << "\n";

    /* Signals declaration */
    for (auto* signal : rtl_module.iter_signals()) {
        printSignalDeclaration(signal);
        out << ";\n";
    }
    out << "\n";
}

void verilog::VerilogWriter::printBody() {
    /* NOTE: FSM is build during "cur_state" signal definition */
    for (auto* signal : rtl_module.iter_signals()) {
        printSignalDefinition(signal);
    }

    for (auto* signal : rtl_module.iter_ports()) {
        printSignalDefinition(signal);
    }
}

void verilog::VerilogWriter::printFooter() {
    out << "endmodule\n";
}

void verilog::VerilogWriter::printSignalDeclaration(rtl::RtlSignal* signal) {
    auto type = signal->getType().value_or("unknown");

    /* Type */
    if (type == "wire"
            && ((signal->getConditionsNum() != 0)
                    || (signal->getDriversNum() != 0)))
    {
        out << "reg ";
    } else if (type == "output"
            && ((signal->getConditionsNum() != 0)
                    || (signal->getDriversNum() != 0)))
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
        out << " next_state";
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

    if (signal->getName().value_or("") == "cur_state") {
        out << "/* FSM BEGIN ---------------------------------------------------------------*/\n\n";
    }

    if (is_register) {
        out << "always @ (posedge clk) begin\n";
    } else {
        out << "always @ (*) begin\n";
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
        /* Special case for FSM state signal */
        if (signal->getName().value_or("") == "cur_state") {
            printFsmController(signal, is_block_assign);
        } else {
            printConditions(signal, is_block_assign);
        }
    }

    out << "end\n\n";

    if (signal->getName().value_or("") == "cur_state") {
        out << "/* FSM END -----------------------------------------------------------------*/\n\n";
    }
}

void verilog::VerilogWriter::printFsmController(rtl::RtlSignal* signal, bool assign_block) {
    assert(
        signal->getName().has_value()
            && signal->getName().value() == "cur_state"
    );

    std::map<std::string, std::vector<unsigned int>> fsm_cases;

    auto n_conditions = signal->getConditionsNum();
    auto width = signal->getWidth().getBitwidth();

    assert(n_conditions != 0);

    for (unsigned int i = 0; i < n_conditions; i++) {
        auto* driver = signal->getDriver(i);
        auto* condition = signal->getCondition(i);

        assert(driver != nullptr);
        assert(condition != nullptr);

        std::string state_name;
        utility::getConditionStateName(condition, state_name);

        fsm_cases[state_name].push_back(i);
    }

    out << "\tif (reset == 1'b1) begin\n";
    out << "\t\tcur_state <= BPHLS;\n"; // Start state = BPHLS
    out << "\tend else begin \n";
    out << "\t\tcur_state <= next_state;\n";
    out << "\tend\n";
    out << "end\n\n";

    out << "always @ (*) begin\n";
    out << "\tnext_state = cur_state;\n\n";
    out << "\tcase (";
    printValue(signal);
    out << ")\n";

    for (auto& fsm_case : fsm_cases) {
        if (fsm_case.first.empty()) {
            continue;
        }

        out << "\t" << fsm_case.first << ":\n";

        unsigned int count = 0;

        /*if (fsm_case.second.size() == 2) {
            for (auto i : fsm_case.second) {
                if ((signal->getDriver(i)->getName().value_or("") == fsm_case.first)
                        && (count != 0))
                {
                    count += 1;
                    continue;
                } else {
                    out << "\t\tnext_state = ";
                    printValue(signal->getDriver(i), width);
                    out << ";\n";
                    break;
                }
            }
        } else {
            Case with case conditions
        }*/
    
        std::vector<rtl::RtlSignal*> clauses;

        for (auto i : fsm_case.second) {
            auto driver_name = signal->getDriver(i)->getName().value_or("");
            if ((driver_name == fsm_case.first)) {
                continue;
            }

            clauses.clear();
            auto* condition = signal->getCondition(i);

            utility::getCaseClauseConditions(condition, clauses);

            if (!clauses.empty()) {
                if (count == 0) {
                    out << "\t\tif (";
                } else {
                    out << "\t\telse if (";
                }

                auto clauses_iter = clauses.begin();
                auto clauses_iter_end = clauses.end();

                for (; clauses_iter != clauses_iter_end; clauses_iter++) {
                    if ((*clauses_iter)->getName().value_or("") == fsm_case.first) {
                        continue;
                    } else {
                        printValue(*clauses_iter);
                        if (clauses_iter != (clauses_iter_end - 1)) {
                            out << " && ";
                        }
                    }
                }

                out << ") begin\n";
                out << "\t\t\tnext_state = ";
                printValue(signal->getDriver(i), width);
                out << ";\n";
                out << "\t\tend\n";
            } else {
                out << "\t\tnext_state = ";
                printValue(signal->getDriver(i), width);
                out << ";\n";
            }

            count += 1;
        }    

    }

    out << "\tdefault:\n\t\tnext_state = cur_state;\n";
    out << "\tendcase\n";
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

void verilog::VerilogWriter::printValue(rtl::RtlSignal* signal, unsigned int width, bool zext) {
    assert(signal != nullptr);

    if (signal->isOperation()) {
        printOperator(static_cast<rtl::RtlOperation*>(signal), width);
        return;
    }

    if (signal->getType().value_or("") == "parameter") {
        out << signal->getName().value_or("unknown");
        return;
    }

    if (!signal->getValue().has_value()) {
        // TODO Printing concatenation & minimizing biwidth
        out << signal->getName().value_or("unknown");
    } else {
        if (utility::isNumeric(signal->getValue().value())) {
            printBiwidthPrefix(signal->getWidth());
            out << "d";
        }
        out << signal->getValue().value();
    }
}

void verilog::VerilogWriter::printOperator(rtl::RtlOperation* operation, unsigned int width) {
    assert(operation != nullptr);

    auto opcode = operation->getOpcode();

    switch (operation->getOperandsNum()) {
    case 2:
        // if (operation->getOpcode() == rtl::RtlOperation::Concat) {
        //     out << "{";
        //     unsigned op0_width =
        //         op->getOperand(0)->getWidth().numNativeBits(rtl, alloc);
        //     printValue(op->getOperand(0), op0_width, true);
        //     out << ",";
        //     unsigned op1_width =
        //         op->getOperand(1)->getWidth().numNativeBits(rtl, alloc);
        //     printValue(op->getOperand(1), op1_width, true);
        //     out << "}";
        // } else {
        out << "(";
        printValue(operation->getOperand(0), width);
        printOpcode(operation->getOpcode());
        printValue(operation->getOperand(1), width);
        out << ")";
        break;
    case 1:
        if (opcode == rtl::RtlOperation::Not) {
            out << "~(";
            printValue(operation->getOperand(0), width);
            out << ")";
        } else if (opcode == rtl::RtlOperation::SExt) {
            // out << "$signed(";
            // auto* op_0 = operation->getOperand(0);
            // bool is_op_0_signed = op_0->getWidth().isSigned();
            // unsigned nativeBits = op0->getWidth().numNativeBits(rtl, alloc);
            // unsigned bits = op0->getWidth().numBits(rtl, alloc);
            // if (op0->getWidth().isNewStyleRTLWidth()) {
            //     nativeBits = op0->getWidth().numNativeBits(rtl, alloc);
            // }
            // if (op0IsSigned && nativeBits > bits) {
            //     printValue(op->getOperand(0), nativeBits, true);
            // } else
            //     printValue(op->getOperand(0), w);
            // out << ")";
        } else if (opcode == rtl::RtlOperation::ZExt) {
            printValue(operation->getOperand(0), width, true);
        } else {
            llvm_unreachable("Unsupported RTL unary operation");
        }
        // } else {
        //     assert(op->getOpcode() == rtl::RtlOperation::Trunc);
        //     // ie. memory_controller_out[31:0]
        //     printValue(op->getOperand(0), w);
        //     // don't truncate a constant
        //     if (!op->getOperand(0)->isConst()) {
        //         out << op->getWidth().str();
        //     }
        // }
        break;
    default:
        llvm_unreachable("Invalid RTL operation");
    }
}

void verilog::VerilogWriter::printConditions(rtl::RtlSignal* signal, bool assign_block) {
    unsigned int n_conditions = signal->getConditionsNum();
    assert(n_conditions != 0);

    for (unsigned int i = 0; i < n_conditions; i++) {
        printCondition(signal, i, assign_block);
    }
}

void verilog::VerilogWriter::printOpcode(rtl::RtlOperation::Opcode opcode) {
    switch (opcode) {
    case rtl::RtlOperation::Add:
        out << " + ";
        break;
    case rtl::RtlOperation::Sub:
        out << " - ";
        break;
    case rtl::RtlOperation::Mul:
        out << " * ";
        break;
    case rtl::RtlOperation::And:
        out << " & ";
        break;
    case rtl::RtlOperation::Or:
        out << " | ";
        break;
    case rtl::RtlOperation::Xor:
        out << " ^ ";
        break;
    case rtl::RtlOperation::Eq:
        out << " == ";
        break;
    case rtl::RtlOperation::Ne:
        out << " != ";
        break;
    case rtl::RtlOperation::Lt:
        out << " < ";
        break;
    case rtl::RtlOperation::Le:
        out << " <= ";
        break;
    case rtl::RtlOperation::Gt:
        out << " > ";
        break;
    case rtl::RtlOperation::Ge:
        out << " >= ";
        break;
    default:
        llvm_unreachable("Invalid operator type!");
    }
}

void verilog::VerilogWriter::printCondition(rtl::RtlSignal* signal,
                                            unsigned int condition_idx,
                                            bool assign_block)
{
    bool is_register = signal->isRegister();
    auto width = signal->getWidth().getBitwidth();
    auto n_conditions = signal->getConditionsNum();

    auto* driver = signal->getDriver(condition_idx);   
    auto* condition = signal->getCondition(condition_idx);

    assert(driver != nullptr);
    assert(condition != nullptr);

    // const std::string driverBits = driver->getDriverBits();
    // const Instruction *I = signal->getInst(conditionNum);

    bool no_else = is_register || (signal->getDefaultDriver() != nullptr);

    bool if_clause = false;

    if (no_else) {
        out << "\tif (";
        printValue(condition);
        out << ") ";
        if_clause = true;
    } else { // add "else"s
        if (n_conditions > 1) {
            if ((n_conditions > 1) && (n_conditions == (condition_idx - 1))) {
                out << "\telse ";
                out << "/* if (";
                printValue(condition);
                out << ") */ ";
            } else {
                if (n_conditions > 0) {
                    out << "\telse if (";
                } else {
                    out << "\tif (";
                }
                printValue(condition);
                out << ") ";
            }
            if_clause = true;
        } else {
            /* Allways trigger on const */
            if (driver->isConstant()) {
                out << "if (reset) begin\n";
                out << "\t" << driver->getName().value_or("unknown") << " = 0;\n";
                out << "end\n";
            }
            if_clause = false;
        }
    }

    if (if_clause) {
        out << "begin\n";
        out << "\t\t";
    }

    static std::string assign_non_block_symb = "<=";
    static std::string assign_block_symb = "=";

    std::string& assign =
        assign_block
            ? assign_block_symb
            : assign_non_block_symb;

    out << signal->getName().value_or("unknown");
    printWidth(driver->getWidth()); // FIXME Указать какие биты драфйвера использовать
    out << " " << assign << " ";
    printValue(driver, width, false);
    out << ";\n";

    if (if_clause) {
        out << "\tend\n";
    }
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
    out << std::to_string(width.getBitwidth()) << "'";
}
