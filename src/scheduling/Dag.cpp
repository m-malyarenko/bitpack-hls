#include <iostream>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/User.h>

#include "../utility/instruction_utility.hpp"

#include "../hardware/HardwareConstraints.hpp"
#include "../hardware/Operation.hpp"

#include "Dag.hpp"

extern llvm::bphls::hardware::HardwareConstraints* constraints;

using namespace llvm;
using namespace bphls;

Dag::Dag() {};

Dag::~Dag() {
    for (auto instr_node : instr_node_lookup) {
        delete instr_node.second;
    }
}

bool Dag::create(BasicBlock& basic_block) {
    for (auto& instr : basic_block) {
        insertInstruction(instr);
    }

    for (auto& instr : basic_block) {
        constructDependencies(instr);
    }

    return true;
}

void Dag::insertInstruction(Instruction& instr) {
    InstructionNode* instr_node = new InstructionNode(instr); 
    instr_node_lookup[&instr] = instr_node;

    assert(constraints->instr_impl.count(instr.getOpcode()) != 0);

    const auto opeartion = constraints->instr_impl[instr.getOpcode()];
    const float crit_delay = opeartion->crit_delay;

    if (crit_delay > constraints->max_delay) {
        instr_node->setMaxDelay();
    } else {
        instr_node->setDelay(crit_delay);
    }
}

void Dag::constructDependencies(Instruction& instr) {
    auto& instr_node = *instr_node_lookup[&instr];

    /* Filter instructions without dependencies */
    if (isa<AllocaInst>(instr) || isa<PHINode>(instr)) {
        return;
    }

    /* Register dependencies */
    for (auto& user : instr.operands()) {
        Instruction* dep_instr = dyn_cast<Instruction>(user);

        if ((dep_instr == nullptr)
                || isa<AllocaInst>(dep_instr)
                || (dep_instr->getParent() != instr.getParent()))
        {
            continue;
        }

        auto& dep_instr_node = *instr_node_lookup[dep_instr];

        instr_node.addDependence(dep_instr_node);
        dep_instr_node.addUse(instr_node);
    }

    /* Alias dependencies */
    // if (isa<LoadInst>(instr) || isa<StoreInst>(instr)) {
    //     auto& basic_block = *instr.getParent();

    //     for (auto& dep_instr : basic_block) {
    //         auto& instr_a = dep_instr;
    //         auto& instr_b = instr;

    //         if (&instr_a == &instr_b) {
    //             /* The investigated instruction has been reached */
    //             break;
    //         }

    //         if ((!isa<LoadInst>(instr_a)
    //                 && !isa<StoreInst>(instr_a)
    //                 && !isa<CallInst>(instr_a))
    //                     || utility::isDummyCall(instr_a)) {
    //             continue;
    //         }

    //     if (hasAliasDependency(instr_a, instr_b)) {
    //         auto& dep_instr_node = *instr_node_lookup[&instr_a];

    //         instr_node.addDependence(dep_instr_node);
    //         dep_instr_node.addUse(instr_node);
    //     }
    //     }
    // }

    /* Call dependencies */
    // TODO Not supported
    assert(!isa<CallInst>(instr));
}

void exportDot() {
    // TODO Implement method
}