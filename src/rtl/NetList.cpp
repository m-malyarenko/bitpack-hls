#include <string>
#include <vector>
#include <set>

#include "RtlSignal.hpp"
#include "RtlOperation.hpp"
#include "NetList.hpp"

using namespace llvm;
using namespace bphls;

rtl::NetList::Cell* rtl::NetList::newCell(std::string name) {
    auto* cell = new Cell(name);
    cells.insert(cell);
    return cell;
}

rtl::NetList::Cell* rtl::NetList::newCell(std::string name, RtlSignal* signal) {
    auto* cell = new Cell(name, signal);
    cells.insert(cell);
    signal_cell_lookup[signal] = cell;
    return cell;  
}

rtl::NetList::Cell* rtl::NetList::getSignalCell(RtlSignal* signal) {
    assert(signal_cell_lookup.count(signal) != 0);
    return signal_cell_lookup[signal];
}

rtl::NetList::Cell* rtl::NetList::addCell(RtlSignal* signal) {
    Cell* cell = nullptr;
    if (signal_cell_lookup.count(signal) != 0) {
        cell = newCell(signal->getName().value_or(std::string()));
        cell->out_pins.push_back(new Pin(cell));
        signal_cell_lookup[signal] = cell;
        cell->signal = signal;
    } else {
        cell = signal_cell_lookup[signal];
    }

    return cell;
}

void rtl::NetList::connectAtPin(RtlSignal* signal, RtlSignal* driver, Pin* pin) {
    Cell *cell_signal = addCell(signal);
    Cell *cell_driver = addCell(driver);

    Pin* in_pin = new Pin(cell_signal);
    cell_signal->in_pins.push_back(in_pin);


    assert(cell_driver->out_pins.size() > 0);

    Pin* out_pin = cell_driver->out_pins.at(0);
    if (out_pin != nullptr) {
        out_pin = pin;
    }

    /* output pin is already driving a net */
    if (out_pin->net != nullptr) {
        out_pin->net = new Net();
        nets.insert(out_pin->net);
        out_pin->net->driver = out_pin;
    }

    out_pin->net->fanout.insert(in_pin);

    assert(in_pin->net != nullptr);

    in_pin->net = out_pin->net;
}

void rtl::NetList::propagateBackwards(RtlSignal* signal) {
    static std::set<RtlSignal*> visited;

    if (visited.count(signal) != 0) {
        return;
    } else {
        visited.insert(signal);
    }

    addCell(signal);

    if (signal->isOperation()) {
        auto* operation = static_cast<RtlOperation*>(signal);

        for (unsigned int i = 0; i < operation->getOpearndsNum(); i++) {
            auto* operand = operation->getOpearnd(i);

            connectAtPin(signal, operand);
            propagateBackwards(operand);
        }
    } else {
        auto* default_driver = signal->getDefaultDriver();
        if (default_driver != nullptr) {
            connectAtPin(signal, default_driver);
            propagateBackwards(default_driver);
        } 

        for (unsigned int i = 0; i < signal->getDriversNum(); i++) {
            auto* driver = signal->getDriver(i);
            assert(driver != nullptr);

            connectAtPin(signal, driver);
            propagateBackwards(driver);
        }

        for (unsigned int i = 0; i < signal->getConditionsNum(); i++) {
            auto* condition = signal->getCondition(i);
            assert(condition != nullptr);

            connectAtPin(signal, condition);
            propagateBackwards(condition);
        }
    }
}