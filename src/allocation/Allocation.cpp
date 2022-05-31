#include <map>

#include "Allocation.hpp"
#include "../hardware/FunctionalUnit.hpp"

using namespace llvm;
using namespace bphls;
using namespace hardware;

Allocation::Allocation(Function& function, Fsm& fsm)
    : function(function),
      fsm(fsm) {}

void Allocation::calculateFuAllocation() {
    std::map<FunctionalUnit*, unsigned int> min_fu;
    std::map<FunctionalUnit*, unsigned int> max_fu;

    for (auto* state : fsm.states()) {
        std::map<FunctionalUnit*, unsigned int> fu_num;

        for (auto* instr : state->instructions()) {
            // auto* fu = 
        }
    }
}