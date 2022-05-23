#include <llvm/IR/Instruction.h>

#include "InstructionNode.hpp"

using namespace llvm;
using namespace bphls;

InstructionNode::InstructionNode(Instruction& instruction)
    : instruction(instruction),
      delay(0.0F) {}

void InstructionNode::setDelay(float delay) {
    this->delay = delay;
}

void InstructionNode::setMaxDelay() {
    this->delay = 1.0F;
}

void  InstructionNode::addDependence(InstructionNode& dep_instr_node) {
    dependencies_list.push_back(&dep_instr_node);
}

void InstructionNode::addUse(InstructionNode& use_instr_node) {
    users_list.push_back(&use_instr_node);
}
