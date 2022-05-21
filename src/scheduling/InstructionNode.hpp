#ifndef __SCHEDULING_INSTRUCTION_NODE_HPP__
#define __SCHEDULING_INSTRUCTION_NODE_HPP__

#include <vector>

#include <llvm/IR/Instruction.h>

namespace llvm {
    namespace bphls {

class InstructionNode {
public:
    InstructionNode(Instruction& instruction);

    void setDelay(float delay);

    void setMaxDelay();

    void addDependence(InstructionNode& dep_instr_node);

    void addUse(InstructionNode& use_instr_node);

    Instruction& getInstruction() { return instruction; };

private:
    Instruction& instruction;

    float delay;

    std::vector<InstructionNode*> dependencies;
    std::vector<InstructionNode*> users;
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_INSTRUCTION_NODE_HPP__ */