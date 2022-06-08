#include <iostream>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/User.h>

#include "../utility/instruction_utility.hpp"
#include "../utility/DotGraph.hpp"

#include "../hardware/HardwareConstraints.hpp"
#include "../hardware/FunctionalUnit.hpp"

#include "Dag.hpp"

extern llvm::bphls::hardware::HardwareConstraints* constraints;

using namespace llvm;
using namespace bphls;

Dag::Dag(Function& function) {
    create(function);
};

Dag::~Dag() {
    for (auto instr_node : instr_node_lookup) {
        delete instr_node.second;
    }
}

bool Dag::create(Function& function) {
    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            insertInstruction(instr);
        }
    }

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            constructDependencies(instr);
        }
    }

    filterInstructions(function);

    return true;
}

bool Dag::hasNode(Instruction& instr) {
    return instr_node_lookup.count(&instr) != 0;
}

InstructionNode& Dag::getNode(Instruction& instr) {
    return *instr_node_lookup[&instr];
}

void Dag::filterInstructions(Function& function) {
    /* WARNING Must be called after dependencies construction */
    std::vector<Instruction*> to_erase;

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            if (isa<ZExtInst>(instr) || isa<SExtInst>(instr)) {
                std::cout << "Filtering: " << instr.getOpcodeName() << std::endl;
                to_erase.push_back(&instr);
            }
        }
    }

    for (auto* instr : to_erase) {
        instr->eraseFromParent();
    }
}

void Dag::insertInstruction(Instruction& instr) {
    InstructionNode* instr_node = new InstructionNode(instr);
    instr_node_lookup[&instr] = instr_node;

    auto* op = constraints->getInstructionOperation(instr);
    assert(op != nullptr);

    float crit_delay = op->crit_delay;

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

    /* Filter insteger extension istructions */
    if (isa<ZExtInst>(instr) || isa<SExtInst>(instr)) {
        return;
    }

    /* Register dependencies */
    for (auto& operand : instr.operands()) {
        Instruction* dep_instr = dyn_cast<Instruction>(operand);

        if ((dep_instr == nullptr)
                || isa<AllocaInst>(dep_instr)
                || (dep_instr->getParent() != instr.getParent()))
        {
            continue;
        }

        /* Remove bit extension instructions */
        if (isa<ZExtInst>(dep_instr) || isa<SExtInst>(dep_instr)) {
            auto* before_cast_operand = dep_instr->getOperand(0);

            auto* before_cast_instr = dyn_cast<Instruction>(before_cast_operand);
            if (before_cast_instr != nullptr) {
                auto& before_cast_instr_node = *instr_node_lookup[before_cast_instr];

                instr_node.addDependence(before_cast_instr_node);
                before_cast_instr_node.addUse(instr_node);
            }

            // FIXME Обработать рекeрсивное приведение типов
            operand.set(before_cast_operand);
        } else {
            auto& dep_instr_node = *instr_node_lookup[dep_instr];

            instr_node.addDependence(dep_instr_node);
            dep_instr_node.addUse(instr_node);
        }
    }

    /* TODO Call dependencies */
    assert(!(isa<CallInst>(instr) && utility::isDummyCall(instr)));

    /* TODO Alias dependencies */
    if (isa<LoadInst>(instr) || isa<StoreInst>(instr)) {
        auto& basic_block = *instr.getParent();

        auto* instr_addr = utility::getPointerOperand(instr);

        for (auto& dep_instr : basic_block) {
            auto& instr_a = dep_instr;
            auto& instr_b = instr;

            if (&instr_a == &instr_b) {
                break;
            }

            if ((!isa<LoadInst>(instr_a)
                    && !isa<StoreInst>(instr_a)
                    && !isa<CallInst>(instr_a))
                        || utility::isDummyCall(instr_a)) {
                continue;
            }

            auto* dep_instr_addr = utility::getPointerOperand(dep_instr);

            if (instr_addr == dep_instr_addr) {
                auto& dep_instr_node = *instr_node_lookup[&instr_a];

                instr_node.addMemoryDependence(dep_instr_node);
                dep_instr_node.addMemoryUse(instr_node);
            }
        }
    }
}

void printNodeLabel(raw_ostream &out, InstructionNode* instr_node) {
    out << instr_node->getInstruction().getOpcodeName();
}

void Dag::exportDot(formatted_raw_ostream& out, BasicBlock& basic_block) {
    dotGraph<InstructionNode> graph(out, printNodeLabel);
    graph.setLabelLimit(40);

    for (auto& instr : basic_block) {
        auto& instr_node = *instr_node_lookup[&instr];

        if (utility::isDummyCall(instr)) {
            continue;
        }

        auto width = instr.getType()->getPrimitiveSizeInBits();
        auto label = "label=\"W: " + std::to_string(width) + "\",";

        for (auto* user_val : instr.users()) {
            // auto* user_instr = &user_node->getInstruction();

            if (Instruction* dep_instr = dyn_cast<Instruction>(user_val)) {

                if (utility::isDummyCall(*dep_instr)) {
                    continue;
                }

                auto* dep_instr_node = instr_node_lookup[dep_instr];
                graph.connectDot(out, &instr_node, dep_instr_node, label + "color=blue");
            }   
        }

        for (auto memory_user : instr_node.memory_users()) {
            if (!utility::isDummyCall(memory_user->getInstruction())) {
                graph.connectDot(out, &instr_node, memory_user, label + "color=red");
            }
        }
    }
}