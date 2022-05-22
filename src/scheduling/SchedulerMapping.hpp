#ifndef __SCHEDULING_SCHEDULER_MAPPING_HPP__
#define __SCHEDULING_SCHEDULER_MAPPING_HPP__

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include <llvm/ADT/DenseMap.h>

#include "dag/InstructionNode.hpp"
#include "dag/Dag.hpp"
#include "fsm/Fsm.hpp"

namespace llvm {
    namespace bphls {

class SchedulerMapping {
public:
    unsigned int getState(InstructionNode *i) { return map[i]; }

    void setState(InstructionNode *i, unsigned int state) { map[i] = state; }

    unsigned int getNumStates(BasicBlock* bb) { return state_num[bb]; }

    void setNumStates(BasicBlock* bb, unsigned int state) { state_num[bb] = state; }

    Fsm* createFSM(Function& function, Dag& dag);

private:
    DenseMap<InstructionNode*, unsigned int> map;
    DenseMap<BasicBlock*, unsigned int> state_num;
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_MAPPING_HPP__ */
