#ifndef __UTILITY_TRANSITION_UTILITY_HPP__
#define __UTILITY_TRANSITION_UTILITY_HPP__

#include <string>

#include "../scheduling/fsm/FsmState.hpp"

namespace llvm {
    namespace bphls {
        namespace utility {

std::string getTransitionOperands(FsmState* state);

        } /* namespace utility */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __UTILITY_TRANSITION_UTILITY_HPP__ */
