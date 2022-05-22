#include <string>

#include "Fsm.hpp"

using namespace llvm;
using namespace bphls;

FsmState* Fsm::createState(FsmState* after = nullptr, std::string name = "bphls") {
    FsmState* state = new FsmState(this); 
    if (after != nullptr) {
        state_list.insertAfter(after, state);
    } else {
        state_list.push_back(state);
    }

    state->setName(name);
    return state;
}