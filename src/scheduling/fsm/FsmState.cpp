#include <iostream>
#include <string>

#include <llvm/Support/raw_ostream.h>

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

bool FsmState::getTerminatingFlag() {
    return terminating;
}

void FsmState::setTransitionVariable(Value* var) {
    assert(var != nullptr);

    transition.variable = var;
}

rtl::RtlSignal* FsmState::getTransitionSignal() {
    return transition.signal;
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

void FsmState::printStateInfo() {
    std::string out_buffer;
    llvm::raw_string_ostream out(out_buffer);

    std::cout << "Transition:\n";

    std::cout << "\tTransition states:\n";
    for (auto* state : transition.states) {
        std::cout << "\t\t-" << state->getName() << std::endl;
    }

    if (getDefaultTransition() != nullptr) {
        std::cout << "\tDefault transition state: " << getDefaultTransition()->getName() << std::endl;
    }

    std::cout << "\tInstructions:\n";
    for (auto* instr : instructions()) {
            std::cout << "\t\t-" << instr->getOpcodeName() << std::endl;
    }
}