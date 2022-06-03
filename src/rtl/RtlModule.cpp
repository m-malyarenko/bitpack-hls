#include <iostream>

#include <set>
#include <queue>

#include "NetList.hpp"
#include "RtlModule.hpp"

using namespace llvm;
using namespace bphls;

rtl::RtlModule::RtlModule(std::string name)
    : name(name) {}

rtl::NetList& rtl::RtlModule::buildNetList() {
    /* Create cell for each i/o port */
    for (auto* port : iter_ports()) {
        auto* cell =
            net_list.newCell(port->getName().value_or(""), port);

        if (isSignalInput(port)) {
            input_cells.push_back(cell);
            cell->out_pins.push_back(new NetList::Pin(cell, "input"));
        } else {
            assert(isSignalOutput(port));
            output_cells.push_back(cell);
            cell->out_pins.push_back(new NetList::Pin(cell, "output"));
        }
    }

    /* TODO Create cells for nested moduule instances */

    /* Propagate port signals */
    for (auto* port : iter_ports()) {
        net_list.propagateBackwards(port);
    }

    /* Propagate normal signals */
    for (auto* signal : iter_signals()) {
        net_list.propagateBackwards(signal);
    }

    /* TODO Print unsynthesizable signals */

    return net_list;
}

void rtl::RtlModule::removeUnconnectedSignals() {
    std::set<NetList::Cell*> marked;
    std::queue<NetList::Cell*> queue;

    for (auto* cell : iter_output_cells()) {
        queue.push(cell);
    }

    while (!queue.empty()) {
        auto* cell = queue.front();
        queue.pop();

        if (marked.count(cell) != 0) {
            continue;
        }
        marked.insert(cell);

        for (auto* pin : cell->in_pins) {
            auto* net = pin->net;
            assert(net != nullptr);

            auto* driver = net->driver;
            assert(driver != nullptr);

            auto* fanout = driver->cell;
            assert(fanout != nullptr);

            queue.push(fanout);
        }
    }

    auto signal_iter = signals.begin();
    while (signal_iter != signals.end()) {
        auto* cell = net_list.getSignalCell(*signal_iter);
        assert(cell != nullptr);

        if (marked.count(cell) == 0) {
            delete *signal_iter;
            signal_iter = signals.erase(signal_iter);
        } else {
            signal_iter++;
        }
    }
}

rtl::RtlSignal* rtl::RtlModule::find(std::string signal) {
    for (auto* port : iter_ports()) {
        auto name = port->getName();

        if (name.has_value() && (name == signal)) {
            return port;
        }
    }

    for (auto* sig : iter_signals()) {
        auto name = sig->getName();

        if (name.has_value() && (name == signal)) {
            return sig;
        }
    }

    for (auto* param : iter_params()) {
        auto name = param->getName();

        if (name.has_value() && (name == signal)) {
            return param;
        }
    }

    return nullptr;
}

bool rtl::RtlModule::exists(std::string signal) {
    return find(signal) != nullptr;
}

rtl::RtlSignal* rtl::RtlModule::addInputWire(std::string name, RtlWidth width) {
    auto* in_wire = new RtlSignal(name, std::nullopt, "input wire", width);
    ports.push_back(in_wire);
    return in_wire;
}

rtl::RtlSignal* rtl::RtlModule::addOutputWire(std::string name, RtlWidth width) {
    auto* out_wire = new RtlSignal(name, std::nullopt, "output wire", width);
    ports.push_back(out_wire);
    return out_wire;
}

rtl::RtlSignal* rtl::RtlModule::addOutputReg(std::string name, RtlWidth width) {
    auto* out_reg = new RtlSignal(name, std::nullopt, "output reg", width);
    ports.push_back(out_reg);
    return out_reg;
}

rtl::RtlSignal* rtl::RtlModule::addWire(std::string name, RtlWidth width) {
    auto* wire = find(name);

    if (wire != nullptr) {
        wire->setWidth(width);
    } else {
        wire = new RtlSignal(name, std::nullopt, "wire", width);
        signals.push_back(wire);
    }

    return wire;
}

rtl::RtlSignal* rtl::RtlModule::addReg(std::string name, RtlWidth width) {
    auto* reg = find(name);

    if (reg != nullptr) {
        reg->setWidth(width);
    } else {
        reg = new RtlSignal(name, std::nullopt, "reg", width);
        signals.push_back(reg);
    }

    return reg;
}

rtl::RtlSignal* rtl::RtlModule::addParam(std::string name, std::string value){
    auto* param = new RtlSignal(name, value, "parameter");
    params.push_back(param);
    return param;
}

rtl::RtlConstant* rtl::RtlModule::addConstant(std::string value, RtlWidth width){
    auto* constant = new RtlConstant(value, width);
    constants.insert(constant);
    return constant;
}

rtl::RtlOperation* rtl::RtlModule::addOperation(RtlOperation::Opcode opcode) {
    auto* operation = new RtlOperation(opcode);
    operations.insert(operation);
    return operation;
}

rtl::RtlOperation* rtl::RtlModule::addOperation(Instruction& instr) {
    auto* operation = new RtlOperation(&instr);
    operations.insert(operation);
    return operation;
}

bool rtl::RtlModule::isSignalInput(RtlSignal* signal) {
    if (signal == nullptr) {
        return false;
    }

    auto& type = signal->getType();

    if (!type.has_value()) {
        return false;
    } else {
        auto& type_value = type.value();
        return (type_value == "input") || (type_value == "input wire");
    }
}

bool rtl::RtlModule::isSignalOutput(RtlSignal* signal) {
    if (signal == nullptr) {
        return false;
    }

    auto& type = signal->getType();

    if (!type.has_value()) {
        return false;
    } else {
        auto& type_value = type.value();
        return (type_value == "output")
                    || (type_value == "output wire")
                    || (type_value == "output reg");
    }
}

std::string rtl::RtlModule::getName() {
    return name;
}

void rtl::RtlModule::printSignals() {
    std::cout << "Module: " << name << std::endl;

    std::cout << "\nParameters: " << std::endl;
    for (auto* param : params) {
        auto param_name = param->getName().value_or("");
        auto param_value = param->getValue().value_or("");

        std::cout << "\t" << param_name << ": " << param_value << std::endl;
    }

    std::cout << "\nPorts: " << std::endl;
    for (auto* port : ports) {
        auto port_name = port->getName().value_or("");
        auto port_type = port->getType().value_or("");

        std::cout << "\t" << port_type << " " << port_name << std::endl;
    }

    std::cout << "\nSignals: " << std::endl;
    for (auto* signal : signals) {
        auto signal_name = signal->getName().value_or("");
        auto signal_type = signal->getType().value_or("");

        std::cout << "\t" << signal_type << " " << signal_name << std::endl;
    }

    std::cout << "\nConstants: " << std::endl;
    for (auto* constant : constants) {
        auto const_value = constant->getValue().value_or("");

        std::cout << "\t" << const_value << std::endl;
    }

    std::cout << "\nOperations: " << std::endl;
    for (auto* operation : operations) {
        auto operation_opcode = operation->getOpcode();

        std::string opcode_name;
        switch (operation_opcode) {
        case RtlOperation::Add:
            opcode_name = "Add";
            break;
        case RtlOperation::And:
            opcode_name = "And";
            break;
        case RtlOperation::Eq:
            opcode_name = "Eq";
            break;
        case RtlOperation::Mul:
            opcode_name = "Mul";
            break;       
        default:
            break;
        }

        std::cout << "\t" << opcode_name
            << " Operands Num: " << operation->getOperandsNum() << std::endl;
    }
}
