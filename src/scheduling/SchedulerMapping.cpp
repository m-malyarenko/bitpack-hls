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

Fsm* SchedulerMapping::createFSM(Function& function, Dag& dag) {
    Fsm* fsm = new Fsm();

    FsmState* wait_state = fsm->createState();
    wait_state->setDefaultTransition(wait_state);

    std::map<BasicBlock*, unsigned int> bb_ids;
    std::map<BasicBlock*, FsmState*> bb_first_states;
    std::map<BasicBlock*, unsigned int> state_index;

    unsigned int n_basic_block = 0;
    for (auto& basic_block : function) {
        bb_ids[&basic_block] = n_basic_block;

        if (utility::isBasicBlockEmpty(basic_block)) {
            continue;
        }

        FsmState* state = fsm->createState();

        state->setBasicBlock(&basic_block);
        bb_first_states[&basic_block] = state;
        state_index[&basic_block] = 0;

        n_basic_block++;
    }

    {
        auto& first_bb = function.front();
        auto first_bb_succ = utility::isBasicBlockEmpty(function.front());

        if (first_bb_succ.has_value()) {
            wait_state->setTerminatingFlag(true);
            wait_state->setBasicBlock(&first_bb);
        
            assert(bb_first_states.find(first_bb_succ.value()) != bb_first_states.end());
        
            wait_state->addTransition(bb_first_states[first_bb_succ.value()]);
        } else {
            assert(bb_first_states.find(&first_bb) != bb_first_states.end());
            wait_state->addTransition(bb_first_states[&first_bb]);
        }
    }

    for (auto& basic_block : function) {
        std::map<unsigned int, FsmState*> states_order;

        if (!utility::isBasicBlockEmpty(basic_block).has_value()) {
            continue;
        }

        states_order[0] = bb_first_states[&basic_block];
        unsigned int last_state = getNumStates(&basic_block);

        assert(!states_order.empty());

        for (unsigned int i = 1; i <= last_state; i++) {
            states_order[i] = fsm->createState(states_order[i - 1]);
        }

        for (auto& instr : basic_block) {
            unsigned int state_order = getState(&dag.getNode(instr));
            states_order[state_order]->pushInstruction(&instr);

            unsigned int delay_state = Scheduler::getInstructionCycles(instr);

            if (delay_state == 0) {
                fsm->setEndState(&instr, states_order[state_order]);
                continue;
            }

            delay_state += state_order;
            if (delay_state > last_state) {
                for (unsigned int i = last_state + 1; i <= delay_state; i++) {
                    states_order[i] = fsm->createState(states_order[i - 1]);
                }
                last_state = delay_state;
            }

            fsm->setEndState(&instr, states_order[delay_state]);
        }

        setFsmStateTransitions(
            states_order[last_state],
            wait_state,
            basic_block.getTerminator(),
            bb_first_states
        );

        states_order[last_state]->setBasicBlock(&basic_block);

        for (unsigned int i = 0; i < last_state; i++) {
            assert(states_order.find(i) != states_order.end());

            FsmState* state = states_order[i];
            state->setBasicBlock(&basic_block);
            state->setDefaultTransition(states_order[i + 1]);
        }
    }

    for (auto& state : fsm->states()) {
        if (state.getBasicBlock() == nullptr) {
            assert(&state == wait_state);
            continue;
        }

        state_index[state.getBasicBlock()] += 1;

        std::string new_state_name =
            "BPHLS_F_" + function.getName().str() + "_BB_" + std::to_string(bb_ids[state.getBasicBlock()]);

        state.setName(new_state_name);
    }

    if (!fsm->states().empty()) {
        fsm->states().begin()->setName("LEGUP");
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