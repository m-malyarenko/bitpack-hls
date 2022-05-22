#ifndef __SCHEDULING_SCHEDULER_HPP__
#define __SCHEDULING_SCHEDULER_HPP__

#include <llvm/IR/Function.h>

#include "dag/Dag.hpp"
#include "fsm/Fsm.hpp"

namespace llvm {
    namespace bphls {

class Scheduler {
public:
    virtual Fsm* schedule(Function& function, Dag& dag) = 0;
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_HPP__ */
