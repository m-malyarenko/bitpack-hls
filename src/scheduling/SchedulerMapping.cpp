#include <iostream>

#include <map>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include <llvm/ADT/DenseMap.h>

#include "../utility/basic_block_utility.hpp"
#include "fsm/FsmState.hpp"
#include "fsm/Fsm.hpp"
#include "InstructionNode.hpp"
#include "Dag.hpp"
#include "SdcScheduler.hpp"

#include "SchedulerMapping.hpp"

using namespace llvm;
using namespace bphls;

SchedulerMapping::SchedulerMapping(Function& function, Dag& dag)
    : function(function),
      dag(dag) {} 

unsigned int SchedulerMapping::getState(InstructionNode* instr) {
    return instr_state_lookup[instr];
}

void SchedulerMapping::setState(InstructionNode* instr, unsigned int state) {
    instr_state_lookup[instr] = state;
}

unsigned int SchedulerMapping::getBasicBlockStatesNum(BasicBlock* basic_block) {
    return bb_states_num_lookup[basic_block];
}

void SchedulerMapping::setBasicBlockStatesNum(BasicBlock* basic_block, unsigned int state) {
    bb_states_num_lookup[basic_block] = state;
}

Fsm& SchedulerMapping::createFsm() {
    FsmState* wait_state = fsm.createState();
    wait_state->setDefaultTransition(wait_state);

    std::map<BasicBlock*, unsigned int> bb_ids;
    std::map<BasicBlock*, unsigned int> bb_state_ids; // Значение инкрементируется при добавлении нового состояния в бб
    std::map<BasicBlock*, FsmState*> bb_first_states;

    unsigned int bb_count = 0;
    for (auto& basic_block : function) {
        bb_ids[&basic_block] = bb_count;

        if (utility::isBasicBlockEmpty(basic_block)) {
            continue;
        }

        FsmState* state = fsm.createState();

        state->setBasicBlock(&basic_block);
        bb_first_states[&basic_block] = state;
        bb_state_ids[&basic_block] = 0;

        bb_count++;
    }

    {
        auto& first_bb = function.front();
        BasicBlock* first_bb_succ = nullptr;

        if (utility::isBasicBlockEmpty(first_bb, first_bb_succ)) {
            wait_state->setTerminatingFlag(true);
            wait_state->setBasicBlock(&first_bb);
        
            assert(bb_first_states.find(first_bb_succ) != bb_first_states.end());
        
            wait_state->addTransition(bb_first_states[first_bb_succ]);
        } else {
            assert(bb_first_states.find(&first_bb) != bb_first_states.end());
            wait_state->addTransition(bb_first_states[&first_bb]);
        }
    }

    for (auto& basic_block : function) {
        std::map<unsigned int, FsmState*> bb_states_order;

        if (utility::isBasicBlockEmpty(basic_block)) {
            continue;
        }

        bb_states_order[0] = bb_first_states[&basic_block];
        unsigned int last_state = getBasicBlockStatesNum(&basic_block);

        assert(!bb_states_order.empty());

        for (unsigned int i = 1; i <= last_state; i++) {
            bb_states_order[i] = fsm.createState(bb_states_order[i - 1]);
        }

        for (auto& instr : basic_block) {
            unsigned int state_order = getState(&dag.getNode(instr));
            bb_states_order[state_order]->pushInstruction(&instr);

            unsigned int delay_state = Scheduler::getInstructionCycles(instr);

            if (delay_state == 0) {
                fsm.setEndState(&instr, bb_states_order[state_order]);
                continue;
            }

            delay_state += state_order;
            if (delay_state > last_state) {
                for (unsigned int i = last_state + 1; i <= delay_state; i++) {
                    bb_states_order[i] = fsm.createState(bb_states_order[i - 1]);
                }
                last_state = delay_state;
            }

            fsm.setEndState(&instr, bb_states_order[delay_state]);
        }

        setFsmStateTransitions(
            bb_states_order[last_state],
            wait_state,
            basic_block.getTerminator(),
            bb_first_states
        );

        for (unsigned int i = 0; i < last_state; i++) {
            assert(bb_states_order.find(i) != bb_states_order.end());

            FsmState* state = bb_states_order[i];
            state->setBasicBlock(&basic_block);
            state->setDefaultTransition(bb_states_order[i + 1]);
        }

        bb_states_order[last_state]->setBasicBlock(&basic_block);
    }

    for (auto* state : fsm.states()) {
        if (state->getBasicBlock() == nullptr) {
            assert(state == wait_state);
            continue;
        }

        auto* state_bb = state->getBasicBlock();

        std::string new_state_name =
            "BPHLS_F_" + function.getName().str()
                + "_BB_" + std::to_string(bb_ids[state_bb])
                + "_S_" + std::to_string(bb_state_ids[state_bb]);

        state->setName(new_state_name);

        bb_state_ids[state->getBasicBlock()] += 1;
    }

    if (!fsm.states().empty()) {
        auto* state = *(fsm.states().begin());
        state->setName("BPHLS");
    }

    return fsm;
}

void SchedulerMapping::setFsmStateTransitions(FsmState* last_state,
                                              FsmState* wait_state,
                                              Instruction* term_instr,
                                              std::map<BasicBlock*, FsmState*> bb_firs_state)
{
    last_state->setTerminatingFlag(true);

    if (isa<UnreachableInst>(term_instr) || isa<ReturnInst>(term_instr)) {
        last_state->setDefaultTransition(wait_state);
        return;
    }

    last_state->setTransitionVariable(term_instr->getOperand(0));

    BasicBlock* default_branch_bb = nullptr;
    
    if (auto* switch_instr = dyn_cast<SwitchInst>(term_instr)) {
        /* Switch */
        const unsigned int switch_operands = switch_instr->getNumOperands(); 

        for (unsigned i = 2; i < switch_operands; i += 2) {
            Value* value = switch_instr->getOperand(i);
            assert(value != nullptr);

            auto* successor_bb = dyn_cast<BasicBlock>(switch_instr->getOperand(i + 1));
            FsmState* successor_state = bb_firs_state[successor_bb];

            last_state->addTransition(successor_state, value);
        }

        default_branch_bb = dyn_cast<BasicBlock>(switch_instr->getDefaultDest());
    } else if (auto* branch_instr = dyn_cast<BranchInst>(term_instr)) {
        if (branch_instr->isConditional()) {
            /* Conditional Branch */
            default_branch_bb = dyn_cast<BasicBlock>(term_instr->getSuccessor(1));

            auto* successor_bb = dyn_cast<BasicBlock>(branch_instr->getSuccessor(0));
            FsmState* successor_state = bb_firs_state[successor_bb];

            last_state->addTransition(successor_state);
        } else {
            /* Unconditional Branch */
            default_branch_bb = dyn_cast<BasicBlock>(term_instr->getSuccessor(0));
        }
    } else {
        llvm_unreachable(0);
    }

    last_state->setDefaultTransition(bb_firs_state[default_branch_bb]);
}