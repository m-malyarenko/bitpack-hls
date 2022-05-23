#include "Fsm.hpp"

#include "FsmState.hpp"

using namespace llvm;
using namespace bphls;

void FsmState::setDefaultTransition(FsmState* s) {
    transition.default_transition = s;
}

FsmState* FsmState::getDefaultTransition() {
    return transition.default_transition;
}

void FsmState::setName(std::string new_name) {
    name = new_name;
}

std::string& FsmState::getName() {
    return name;
}

void FsmState::setBasicBlock(BasicBlock* basic_block) {
    this->basic_block = basic_block; 
}

void FsmState::setTerminatingFlag(bool term_flag) {
    terminating = term_flag;
}

void FsmState::setTransitionVariable(Value* var) {
    transition.variable = var;
}

void FsmState::addTransition(FsmState* state, Value* value) {
    transition.states.push_back(state);
    transition.values.push_back(value);
}

void FsmState::pushInstruction(Instruction* instr) {
    instr_list.push_back(instr);
    parent->setStartState(instr, this);
}

BasicBlock* FsmState::getBasicBlock() {
    return basic_block;
}