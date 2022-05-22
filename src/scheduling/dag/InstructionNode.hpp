#ifndef __SCHEDULING_INSTRUCTION_NODE_HPP__
#define __SCHEDULING_INSTRUCTION_NODE_HPP__

#include <vector>

#include <llvm/ADT/iterator_range.h>
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

    typedef std::vector<InstructionNode*>::iterator InstructionNodeIterator;

    iterator_range<InstructionNodeIterator> dependencies() {
        return make_range(dependencies_list.begin(), dependencies_list.end());
    }

    iterator_range<InstructionNodeIterator> users() {
        return make_range(users_list.begin(), users_list.end());
    }

private:
    Instruction& instruction;

    float delay;

    std::vector<InstructionNode*> dependencies_list;
    std::vector<InstructionNode*> users_list;
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_INSTRUCTION_NODE_HPP__ */