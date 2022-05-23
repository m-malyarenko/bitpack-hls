#include <string>

#include "Fsm.hpp"

using namespace llvm;
using namespace bphls;

FsmState* Fsm::createState(FsmState* after, std::string name) {
    FsmState* state = new FsmState(this);

    if (after != nullptr) {
        // C++...
        StateIterator iter;
        for (iter = states().begin(); iter != states().end(); iter++) {
            auto* node_data_ptr = &(*iter);

            if (node_data_ptr == after) {
                break;
            }
        }
        // FIXME iter may be end()
        state_list.insertAfter(iter, state);
    } else {
        state_list.push_back(state);
    }

    state->setName(name);
    return state;
}

void Fsm::setStartState(Instruction* instr, FsmState* state) {
    start_state_lookup[instr] = state;
}

void Fsm::setEndState(Instruction* instr, FsmState* state) {
    end_state_lookup[instr] = state;
}