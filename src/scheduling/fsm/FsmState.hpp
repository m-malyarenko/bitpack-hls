#ifndef __SCHEDULING_FSM_FSM_STATE_HPP__
#define __SCHEDULING_FSM_FSM_STATE_HPP__

#include <map>
#include <set>
#include <list>
#include <vector>

#include "llvm/ADT/ilist.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/IR/Instructions.h"

#include "../../rtl/RtlSignal.hpp"

namespace llvm {
    namespace bphls {

class FsmState : public ilist_node<FsmState> {
public:
    struct Transition {
        Value* variable;
        rtl::RtlSignal* signal;

        std::vector<FsmState*> values;
        std::vector<FsmState*> states;

        std::set<FsmState*> predecessors;
        FsmState* default_transition;

        Transition()
            : variable(0),
              signal(0),
              default_transition(0) {};
    };
};

    } /* namespace bphls */
} /* namespace llvm */

#endif /* __SCHEDULING_FSM_FSM_STATE_HPP__ */
