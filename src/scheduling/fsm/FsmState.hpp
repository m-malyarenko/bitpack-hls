#ifndef __SCHEDULING_FSM_FSM_STATE_HPP__
#define __SCHEDULING_FSM_FSM_STATE_HPP__

#include <map>
#include <set>
#include <list>
#include <vector>

#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include "../../rtl/RtlSignal.hpp"

namespace llvm {
    namespace bphls {

class Fsm;

class FsmState : public ilist_node<FsmState> {
public:
    struct Transition {
        Value* variable;
        rtl::RtlSignal* signal;

        std::vector<Value*> values;
        std::vector<FsmState*> states;

        std::set<FsmState*> predecessors;
        FsmState* default_transition;

        Transition()
            : variable(0),
              signal(0),
              default_transition(0) {};
    };

    typedef std::list<Instruction*> InstructionList;
    typedef InstructionList::iterator InstructionListIterator;

    iterator_range<InstructionListIterator> instructions() {
        return make_range(instr_list.begin(), instr_list.end());
    }

    FsmState(Fsm* parent)
        : parent(parent),
          basic_block(nullptr),
          terminating(false) {};

    void addTransition(FsmState* state, Value* value = nullptr);

    void setDefaultTransition(FsmState* state);

    void pushInstruction(Instruction* instr);

    void setTerminatingFlag(bool term_flag);

    bool getTerminatingFlag();

    void setTransitionVariable(Value* var);

    Value* getTransitionVariable();

    rtl::RtlSignal* getTransitionSignal();

    void setTransitionSignal(rtl::RtlSignal* signal);

    Value* getTransitionValue(unsigned int trans);

    FsmState* getTransitionState(unsigned int state);

    FsmState* getDefaultTransition();

    unsigned int getTransitionsNum();

    void setName(std::string new_name);

    std::string& getName();

    void setBasicBlock(BasicBlock* basic_block);

    BasicBlock* getBasicBlock();

    void printStateInfo();

private:
    InstructionList instr_list;

    std::string name;
    Fsm* parent;
    BasicBlock* basic_block;

    Transition transition;
    bool terminating;
};

    } /* namespace bphls */
} /* namespace llvm */

#endif /* __SCHEDULING_FSM_FSM_STATE_HPP__ */
