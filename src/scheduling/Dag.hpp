#ifndef __SCHEDULING_SCHEDULER_DAG_HPP__
#define __SCHEDULING_SCHEDULER_DAG_HPP__

#include <map>

#include <llvm/IR/Function.h>
#include <llvm/Support/FormattedStream.h>

#include "InstructionNode.hpp"

namespace llvm {
    namespace bphls {

class Dag {
public:
    Dag(Function& function);

    ~Dag();

    void exportDot(formatted_raw_ostream& out, BasicBlock& basic_block);

    InstructionNode& getNode(Instruction& instr);

private:
    std::map<Instruction*, InstructionNode*> instr_node_lookup;

    bool create(Function& function);

    void insertInstruction(Instruction& instr);

    void constructDependencies(Instruction& instr);
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_DAG_HPP__ */
