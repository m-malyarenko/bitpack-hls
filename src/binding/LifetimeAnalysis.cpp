#include <map>

#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/BitVector.h>

#include "LifetimeAnalysis.hpp"

using namespace llvm;
using namespace bphls;

void binding::LifetimeAnalysis::analize() {
    /* Initialize instruction bit encoding map */
    {
        unsigned int instr_idx = 0;

        for (auto& arg : function.args()) {
            value_bit_pos_lookup[&arg] = instr_idx;
            instr_idx++;
        }

        for (auto& basic_block : function) {
            for (auto& instr : basic_block) {
                value_bit_pos_lookup[&instr] = instr_idx;
                instr_idx++;
            }
        }
    }

    /* Number of definitions in function */
    auto def_count = value_bit_pos_lookup.size();

    /* Initialize basic block lifetime info */
    {
        for (auto& basic_block : function) {
            auto* bb_info = new BasicBlockLifetimeInfo(&basic_block, def_count);
            bb_info_lookup[&basic_block] = bb_info;
        }
    }

    /* Initialize definition bits in mask */
    {
        auto* entry_bb = &function.front();
        auto* entry_bb_info = bb_info_lookup[entry_bb];

        for (auto& arg : function.args()) {
            entry_bb_info->def.set(getBitPosition(&arg));
        }

        for (auto& basic_block : function) {
            auto& info = *bb_info_lookup[&basic_block];

            for (auto& instr : basic_block) {
                info.def.set(getBitPosition(&instr));
            }
        }
    }

    /* Initialize use bits in mask */
    {
        for (auto& arg : function.args()) {
            markUses(&arg);
        }

        for (auto& basic_block : function) {
            for (auto& instr : basic_block) {
                markUses(&instr);
            }
        }
    }

    /* Create flow mask */
    FlowMask flow_mask;
    initFlowMask(flow_mask);

    /* Collect lifetime information */
    bool is_changed = true;
    while (is_changed) {
        is_changed = false;

        for (auto& basic_block : function) {
            bb_info_lookup[&basic_block]->out.reset();

            for (auto* succ_basic_block : successors(&basic_block)) {
                auto flow_mask_key = std::make_pair(&basic_block, succ_basic_block);

                if (flow_mask.count(flow_mask_key) != 0) {
                    BitVector mask(bb_info_lookup[succ_basic_block]->in);
                    mask &= (*flow_mask[flow_mask_key]);
                    bb_info_lookup[&basic_block]->out |= mask;
                } else {
                    bb_info_lookup[&basic_block]->out |= bb_info_lookup[succ_basic_block]->in;
                }
            }

            BitVector old_in(bb_info_lookup[&basic_block]->in);
            BitVector temp_def(bb_info_lookup[&basic_block]->def);

            temp_def.flip();
            temp_def &= bb_info_lookup[&basic_block]->out;
            temp_def |= bb_info_lookup[&basic_block]->use;

            bb_info_lookup[&basic_block]->in = temp_def;

            if (bb_info_lookup[&basic_block]->in != old_in) {
                is_changed = true;
            }
        }
    }
}

void binding::LifetimeAnalysis::markUses(Value* def_value) {
    assert(def_value != nullptr);

    for (auto* user : def_value->users()) {
        auto* use_instr = dyn_cast<Instruction>(user);
        if (use_instr == nullptr) {
            continue;
        }

        auto* use_instr_bb = use_instr->getParent();

        auto* def_instr = dyn_cast<Instruction>(def_value);
        if ((def_instr != nullptr)
                && (use_instr_bb == def_instr->getParent())
                && !isa<PHINode>(use_instr))
        {
            continue;
        }

        auto* info = bb_info_lookup[use_instr_bb];
        info->use.set(getBitPosition(def_value));
    }
}

void binding::LifetimeAnalysis::initFlowMask(FlowMask& flow_mask) {
    auto def_count = static_cast<unsigned int>(value_bit_pos_lookup.size());

    for (auto& basic_block : function) {
        BitVector basic_block_mask(def_count, true);
        initPhiFlowMask(basic_block_mask, basic_block);

        for (auto& instr : basic_block) {
            if (!isa<PHINode>(instr)) {
                continue;
            }

            auto* phi_node = dyn_cast<PHINode>(&instr);
            auto n_income_val = static_cast<unsigned int>(phi_node->getNumIncomingValues());

            for (unsigned int i = 0; i < n_income_val; i++) {
                auto* income_basic_bock = phi_node->getIncomingBlock(i);
                auto* income_val = phi_node->getIncomingValue(i);

                if (!isa<Instruction>(income_val)) {
                    continue;
                }

                auto flow_mask_key = std::make_pair(income_basic_bock, &basic_block);
                auto mask_iter = flow_mask.find(flow_mask_key);

                if (mask_iter == flow_mask.end()) {
                    flow_mask[flow_mask_key] = new BitVector(basic_block_mask);
                } else {
                    flow_mask[flow_mask_key]->set(getBitPosition(income_val));
                }
            }
        }
    }
}

void binding::LifetimeAnalysis::initPhiFlowMask(BitVector& basic_block_mask, BasicBlock& basic_block) {
    for (auto& instr : basic_block) {
        if (!isa<PHINode>(instr)) {
            continue;
        }

        auto* phi_node = dyn_cast<PHINode>(&instr);

        for (auto& income_val : phi_node->incoming_values()) {
            auto* income_instr = dyn_cast<Instruction>(&income_val);
            if (income_instr == nullptr) {
                continue;
            }

            auto* def_basic_block = income_instr->getParent();

            bool is_used_in_non_phi = false;
            for (auto* user : income_instr->users()) {
                auto* user_instr = dyn_cast<Instruction>(user);
                if (user_instr == nullptr) {
                    continue;
                }

                auto* user_basicc_block = user_instr->getParent();
                if ((user_basicc_block == def_basic_block)
                        || (user_basicc_block != &basic_block)
                        || isa<PHINode>(user_instr))
                {
                    continue;
                }

                is_used_in_non_phi = true;
            }

            if (!is_used_in_non_phi) {
                basic_block_mask.reset(getBitPosition(income_val));
            }
        }
    }
}

unsigned int binding::LifetimeAnalysis::getBitPosition(Value* val) {
    assert(value_bit_pos_lookup.count(val) != 0);
    return value_bit_pos_lookup[val];
}

binding::LifetimeAnalysis::BasicBlockLifetimeInfo* binding::LifetimeAnalysis::getInfo(BasicBlock* basic_block) {
    assert(bb_info_lookup.count(basic_block) != 0);
    return bb_info_lookup[basic_block];
}

binding::LifetimeAnalysis::~LifetimeAnalysis() {
    for (auto& bb_info : bb_info_lookup) {
        delete bb_info.second;
    }
}