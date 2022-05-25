#ifndef __SCHEDULING_SCHEDULER_MAPPING_HPP__
#define __SCHEDULING_SCHEDULER_MAPPING_HPP__

#include <map>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>

#include "InstructionNode.hpp"
#include "Dag.hpp"
#include "fsm/FsmState.hpp"
#include "fsm/Fsm.hpp"

namespace llvm {
    namespace bphls {

class SchedulerMapping {
public:
    SchedulerMapping(Function& function, Dag& dag);

    unsigned int getState(InstructionNode* instr);

    void setState(InstructionNode* instr, unsigned int state);

    unsigned int getBasicBlockStatesNum(BasicBlock* basic_block);

    void setBasicBlockStatesNum(BasicBlock* basic_block, unsigned int state);

    Fsm& createFsm();

    std::map<InstructionNode*, unsigned int>& getMap() { return instr_state_lookup; }

private:
    Function& function;
    Dag& dag;

    Fsm fsm;

    std::map<InstructionNode*, unsigned int> instr_state_lookup;
    std::map<BasicBlock*, unsigned int> bb_states_num_lookup;

    void setFsmStateTransitions(FsmState* last_state,
                                FsmState* wait_state,
                                Instruction* term_instr,
                                std::map<BasicBlock*, FsmState*> bb_firs_state);
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_MAPPING_HPP__ */
