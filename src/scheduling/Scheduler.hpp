#ifndef __SCHEDULING_SCHEDULER_HPP__
#define __SCHEDULING_SCHEDULER_HPP__

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include "SchedulerMapping.hpp"
#include "Dag.hpp"

namespace llvm {
    namespace bphls {

class Scheduler {
public:
    virtual ~Scheduler() {};

    virtual SchedulerMapping* schedule(Function& function, Dag& dag) = 0;

    static unsigned int getInstructionCycles(Instruction& instr);
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_HPP__ */
