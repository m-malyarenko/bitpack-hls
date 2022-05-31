#ifndef __RTL_RTL_MODULE_HPP__
#define __RTL_RTL_MODULE_HPP__

#include <vector>
#include <string>
#include <set>

#include <llvm/ADT/iterator_range.h>

#include "RtlWidth.hpp"
#include "RtlSignal.hpp"
#include "RtlConstant.hpp"
#include "RtlOperation.hpp"
#include "NetList.hpp"

namespace llvm {
    namespace bphls {
        namespace rtl {

class RtlModule {
public:
    RtlModule(std::string name);

    NetList& buildNetList();

    RtlSignal* find(std::string signal);

    bool exists(std::string signal);

    void remove(std::string signal);

    RtlSignal* addInputWire(std::string name, RtlWidth width = RtlWidth());

    RtlSignal* addOutputWire(std::string name, RtlWidth width = RtlWidth());

    RtlSignal* addOutputReg(std::string name, RtlWidth width = RtlWidth());

    RtlSignal* addWire(std::string name, RtlWidth width = RtlWidth());

    RtlSignal* addReg(std::string name, RtlWidth width = RtlWidth());

    RtlSignal* addParam(std::string name, std::string value);

    RtlConstant* addConstant(std::string value, RtlWidth width = RtlWidth());

    RtlOperation* addOperation(RtlOperation::Opcode opcode);

    RtlOperation* addOperation(Instruction& instr);

    bool isSignalInput(RtlSignal* signal);

    bool isSignalOutput(RtlSignal* signal);

    std::string getName();

    void printSignals();

    typedef std::vector<RtlSignal*>::iterator RtlSignalIterator;

    iterator_range<RtlSignalIterator> iter_params() {
        return make_range(params.begin(), params.end());
    }

    iterator_range<RtlSignalIterator> iter_ports() {
        return make_range(ports.begin(), ports.end());
    }

    iterator_range<RtlSignalIterator> iter_signals() {
        return make_range(signals.begin(), signals.end());
    }

private:
    std::string name;

    std::vector<RtlSignal*> params;
    std::vector<RtlSignal*> ports;
    std::vector<RtlSignal*> signals;
    std::set<RtlConstant*> constants;
    std::set<RtlOperation*> operations;

    NetList net_list;
    std::vector<NetList::Cell*> input_cells;
    std::vector<NetList::Cell*> output_cells;

    typedef std::vector<NetList::Cell*>::iterator CellIterator;

    iterator_range<CellIterator> iter_input_cells() {
        return make_range(input_cells.begin(), input_cells.end());
    }

    iterator_range<CellIterator> iter_output_cells() {
        return make_range(output_cells.begin(), output_cells.end());
    }

    void removeUnconnectedSignals();
};

        } /* namespace rtl */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __RTL_RTL_MODULE_HPP__ */
