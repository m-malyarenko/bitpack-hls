#ifndef __SCHEDULING_FSM_FSM_HPP__
#define __SCHEDULING_FSM_FSM_HPP__

#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/iterator_range.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/IR/Instructions.h>

#include "FsmState.hpp"

namespace llvm {
    namespace bphls {

class Fsm {
public:
    FsmState* createState(FsmState* after = nullptr, std::string name = "bphls");

    void setStartState(Instruction* instr, FsmState* state);

    void setEndState(Instruction* instr, FsmState* state);

    typedef iplist<FsmState>::iterator StateIterator;

    iterator_range<StateIterator> states() {
        return make_range(state_list.begin(), state_list.end());
    }

private:
    iplist<FsmState> state_list;
    DenseMap<const Instruction*, FsmState*> start_state_lookup;
    DenseMap<const Instruction*, FsmState*> end_state_lookup;
};

    } /* namespace bphls */
} /* namespace llvm */

#endif /* __SCHEDULING_FSM_FSM_HPP__ */