#include <string>

#include <llvm/IR/Value.h>

#include "../scheduling/fsm/FsmState.hpp"

#include "transition_utility.hpp"

using namespace llvm;
using namespace bphls;

std::string utility::getTransitionOperands(FsmState* state) {
    auto* trans_value = state->getTransitionVariable();

    if (trans_value != nullptr) {
        return trans_value->getName().str();
    } else {
        return std::string("null");
    }
}