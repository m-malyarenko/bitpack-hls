#ifndef __SCHEDULING_SCHEDULER_DAG_HPP__
#define __SCHEDULING_SCHEDULER_DAG_HPP__

#include <llvm/IR/BasicBlock.h>
#include <llvm/ADT/DenseMap.h>

#include <llvm/Support/FormattedStream.h>

#include "InstructionNode.hpp"

namespace llvm {
    namespace bphls {

class Dag {
public:
    Dag();
    ~Dag();

    bool create(BasicBlock& basic_block);

    void exportDot(formatted_raw_ostream& out, BasicBlock& basic_block);

private:
    DenseMap<Instruction*, InstructionNode*> instr_node_lookup;

    void insertInstruction(Instruction& instr);

    void constructDependencies(Instruction& instr);

    bool hasAliasDependency(Instruction& instr_a, Instruction& instr_b);
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SCHEDULER_DAG_HPP__ */
