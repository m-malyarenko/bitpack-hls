#ifndef __SCHEDULING_SCHEDULER_MAPPING_HPP__
#define __SCHEDULING_SCHEDULER_MAPPING_HPP__

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include <llvm/ADT/DenseMap.h>

#include "InstructionNode.hpp"
#include "Dag.hpp"
#include "fsm/FsmState.hpp"
#include "fsm/Fsm.hpp"

namespace llvm {
    namespace bphls {

class SchedulerMapping {
public:
    unsigned int getState(InstructionNode* instr) { return map[instr]; }

    void setState(InstructionNode* instr, unsigned int state) { map[instr] = state; }

    unsigned int getNumStates(BasicBlock* bb) { return state_num[bb]; }

    void setNumStates(BasicBlock* bb, unsigned int state) { state_num[bb] = state; }

    Fsm* createFSM(Function& function, Dag& dag);

private:
    DenseMap<InstructionNode*, unsigned int> map;
    DenseMap<BasicBlock*, unsigned int> state_num;

    void setFsmStateTransitions(FsmState* last_state,
                                FsmState* wait_state,
                                Instruction* term_instr,
                                std::map<BasicBlock*, FsmState*> bb_firs_state);
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_MAPPING_HPP__ */
