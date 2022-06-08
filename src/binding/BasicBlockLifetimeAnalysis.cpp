#include <iostream>

#include <string>
#include <map>
#include <set>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/ADT/BitVector.h>
#include <llvm/Support/raw_ostream.h>

#include "../scheduling/fsm/Fsm.hpp"
#include "../scheduling/fsm/FsmState.hpp"

#include "BasicBlockLifetimeAnalysis.hpp"

using namespace llvm;
using namespace bphls;

binding::BasicBlockLifetimeAnalysis::~BasicBlockLifetimeAnalysis() {
    for (auto var_lt : table) {
        delete var_lt.second;
    }
}

void binding::BasicBlockLifetimeAnalysis::analyze() {
    std::map<Value*, LifetimeInterval*> val_lt_lookup;

    /* For now only first basic block */
    auto& basic_block = function.front();

    unsigned int stage_idx = 0;
    for (auto& arg : function.args()) {
        unsigned int width =
            static_cast<unsigned int>(arg.getType()->getPrimitiveSizeInBits());

        val_lt_lookup[&arg] = new LifetimeInterval(stage_idx, stage_idx, width);
    }

    for (auto& instr : basic_block) {
        if (isa<ReturnInst>(instr) || isa<CallInst>(instr)) {
            continue;
        }

        unsigned int width =
            static_cast<unsigned int>(instr.getType()->getPrimitiveSizeInBits());

        val_lt_lookup[&instr] = new LifetimeInterval(width);
    }

    assert(fsm.getStatesNum() > 1);
    auto* cur_state = *fsm.states().begin();

    assert(cur_state != nullptr);

    stage_idx += 1;
    std::set<FsmState*> visited_states;
    while (cur_state != nullptr) {
        if (visited_states.count(cur_state) != 0) {
            break;
        }

        if (!cur_state->instructions().empty()) {
            for (auto* instr : cur_state->instructions()) {
                if (isa<ReturnInst>(instr) || isa<CallInst>(instr)) {
                    continue;
                }

                if (!isa<ReturnInst>(instr) || !isa<CallInst>(instr)) {
                    val_lt_lookup[instr]->def = stage_idx;
                }

                for (auto& op : instr->operands()) {
                    if (!isa<Constant>(op.get())) {
                        val_lt_lookup[op.get()]->use = stage_idx;
                    }
                }
            }

            stage_idx += 1;
        }

        visited_states.insert(cur_state);
        if (cur_state == cur_state->getDefaultTransition()) {
            assert(cur_state->getTransitionsNum() == 2);
            cur_state = cur_state->getTransitionState(0);
        } else {
            cur_state = cur_state->getDefaultTransition();
        }
    }

    for (auto& var_lt : val_lt_lookup) {
        auto* lt = var_lt.second;

        if (lt->use == 0) {
            lt->use = stage_idx - 1;
        }
    }

    for (auto& var_lt : val_lt_lookup) {
        table.push_back(var_lt);
    }
}

void binding::BasicBlockLifetimeAnalysis::printLifetimeTable() {
    std::string out_buffer;
    raw_string_ostream out(out_buffer);

    for (auto var_lt : table) {
        auto* var = var_lt.first;
        auto& lt = var_lt.second;

        assert(lt->use > lt->def);

        out << "Var: ";
        var->printAsOperand(out);
        out << " Def: " << std::to_string(lt->def);
        out << " Use: " << std::to_string(lt->use);
        out << "\n";
    }

    out << "\n";

    for (auto var_lt : table) {
        auto* var = var_lt.first;
        auto* lt = var_lt.second;

        assert(lt->use > lt->def);

        out << "Var: ";
        var->printAsOperand(out);
        out << " ";
        std::string def_space(lt->def, ' ');
        std::string alive_space(lt->use - lt->def, '#');
        out << def_space << alive_space << "\n";
    }

    std::cout << out_buffer;
}
