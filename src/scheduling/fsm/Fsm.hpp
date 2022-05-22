#ifndef __SCHEDULING_FSM_FSM_HPP__
#define __SCHEDULING_FSM_FSM_HPP__

#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>

#include "FsmState.hpp"

namespace llvm {
    namespace bphls {

class Fsm {
public:
    FsmState* createState(FsmState* after = nullptr, std::string name = "bphls");

private:
    iplist<FsmState> state_list;
    DenseMap<const Instruction*, FsmState*> instr_state_lookup;
    DenseMap<const Instruction*, FsmState*> end_state_lookup;
};

    } /* namespace bphls */
} /* namespace llvm */

#endif /* __SCHEDULING_FSM_FSM_HPP__ */