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

unsigned int FsmState::getTransitionsNum() {   
    return transition.states.size() + 1;
}

Value* FsmState::getTransitionVariable() {
    return transition.variable;
}

Value* FsmState::getTransitionValue(unsigned int trans) {
    return transition.values[trans];
}

FsmState* FsmState::getTransitionState(unsigned int state) {
    return transition.states[state];
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
    assert(var != nullptr);

    transition.variable = var;
}

void FsmState::setTransitionSignal(rtl::RtlSignal* signal) {
    assert(signal != nullptr);

    transition.signal = signal;
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