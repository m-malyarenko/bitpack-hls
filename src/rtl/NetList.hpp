#ifndef __RTL_NET_LIST_HPP__
#define __RTL_NET_LIST_HPP__

#include <string>
#include <vector>
#include <set>
#include <map>

#include "RtlSignal.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class NetList {
public:
    struct Pin;
    struct Cell;

    struct Net {
        Pin* driver;
        std::set<Pin*> fanout;
        std::string name;

        Net(std::string name = std::string())
            : driver(nullptr),
              name(name) {}
    };

    struct Pin {
        Cell* cell;
        Net* net;
        std::string name;

        Pin(Cell* cell, std::string name = std::string())
            : cell(cell),
              net(nullptr),
              name(name) {}
    };

    struct Cell {
        std::vector<Pin*> in_pins;
        std::vector<Pin*> out_pins;
        RtlSignal* signal;
        std::string name;

        Cell(std::string name = std::string(), RtlSignal* signal = nullptr)
            : signal(signal),
              name(name) {}
    };

    Cell* newCell(std::string name);

    Cell* newCell(std::string name, RtlSignal* signal);

    Cell* getSignalCell(RtlSignal* signal);

    Cell* addCell(RtlSignal* signal);

    void connectAtPin(RtlSignal* signal, RtlSignal* driver, Pin* pin = nullptr);

    void propagateBackwards(RtlSignal* signal);

private:
    std::set<Net*> nets;
    std::set<Cell*> cells;

    std::map<RtlSignal*, NetList::Cell*> signal_cell_lookup;
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_NET_LIST_HPP__ */