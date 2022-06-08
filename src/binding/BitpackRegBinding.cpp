#include <iostream>

#include <map>
#include <vector>
#include <optional>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/raw_ostream.h>

#include "BitpackRegBinding.hpp"

using namespace llvm;
using namespace bphls;

binding::BitpackRegBinding::BitpackRegBinding(Function& function, Fsm& fsm)
    : function(function),
      fsm(fsm) {}

binding::BitpackRegBinding::~BitpackRegBinding() {
    for (auto& var_lt : lt_table) {
        auto* lt = var_lt.second;

        if (lt != nullptr) {
            delete lt;
        }
    }

    for (auto* pool : lt_pool_table) {
        if (pool != nullptr) {
            delete pool;
        }
    }
}

binding::BitpackRegBinding::LifetimeInterval::LifetimeInterval()
    : def(0),
      use(0),
      weight(0) {}

binding::BitpackRegBinding::LifetimeInterval::LifetimeInterval(unsigned int weight)
    : def(0),
      use(0),
      weight(weight) {}

binding::BitpackRegBinding::LifetimeInterval::LifetimeInterval(unsigned int def,
                                                                    unsigned int use,
                                                                    unsigned int weight)
    : def(def),
      use(use),
      weight(weight) {}

bool binding::BitpackRegBinding::LifetimeInterval::overlap(LifetimeInterval& other) {
    return !((this->def >= other.use) || (other.def >= this->use));
}

binding::BitpackRegBinding::Reg::Reg()
    : id(0),
      width(0) {}

binding::BitpackRegBinding::Reg::Reg(RegId id, unsigned int width)
    : id(id),
      width(width) {}

binding::BitpackRegBinding::RegBitfield::RegBitfield()
    : reg(),
      bitfield(std::make_pair(0, 0)) {}

binding::BitpackRegBinding::RegBitfield::RegBitfield(Reg reg)
    : reg(reg),
      bitfield(std::make_pair(0, reg.width - 1)) {}

binding::BitpackRegBinding::RegBitfield::RegBitfield(Reg reg, unsigned int lsb, unsigned int msb)
    : reg(reg),
      bitfield(std::make_pair(lsb, msb)) {}

binding::BitpackRegBinding::RegBitfield binding::BitpackRegBinding::RegBitfield::operator + (RegBitfield& other) {
    auto min_lsb = std::min(bitfield.first, other.bitfield.first);
    auto max_msb = std::max(bitfield.second, other.bitfield.second);

    return RegBitfield(this->reg, min_lsb, max_msb);
}

binding::BitpackRegBinding::LifetimeWidthPool::LifetimeWidthPool(Value* var,
                                                                 LifetimeInterval* lt,
                                                                 unsigned int id)
    : id(id),
      var(var),
      lt(lt) {}

void binding::BitpackRegBinding::bindRegisters() {
    analyzeLifetime();

#ifndef NDEBUG
    printLifetimeTable();
#endif

    auto pool_width = splitTableIntoEqWidthPools();

    performLeftEdge();

    mergeSubRegisters(pool_width);

    mapVariablesToBitfields();
}

std::map<Value*, binding::BitpackRegBinding::RegBitfield>& binding::BitpackRegBinding::getRegisterMapping() {
    return binding_map;
}

std::vector<binding::BitpackRegBinding::Reg>& binding::BitpackRegBinding::getRegisters() {
    return registers;
}

void binding::BitpackRegBinding::analyzeLifetime() {
    /* For now only first basic block */
    auto& basic_block = function.front();

    unsigned int stage_idx = 0;
    for (auto& arg : function.args()) {
        unsigned int width =
            static_cast<unsigned int>(arg.getType()->getPrimitiveSizeInBits());

        lt_table[&arg] = new LifetimeInterval(stage_idx, stage_idx, width);
    }

    for (auto& instr : basic_block) {
        if (instr.isTerminator()) {
            continue;
        }

        unsigned int width =
            static_cast<unsigned int>(instr.getType()->getPrimitiveSizeInBits());

        lt_table[&instr] = new LifetimeInterval(width);
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
                if (instr->isTerminator()) {
                    continue;
                }

                lt_table[instr]->def = stage_idx;

                for (auto& op : instr->operands()) {
                    auto* op_value = op.get();

                    if (!isa<Constant>(op_value)) {
                        lt_table[op_value]->use = stage_idx;
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

    for (auto& var_lt : lt_table) {
        auto* lt = var_lt.second;

        if (lt->use == 0) {
            lt->use = stage_idx - 1;
        }
    }

    this->n_stages = stage_idx - 1;
}

void binding::BitpackRegBinding::printLifetimeTable() {
    std::string out_buffer;
    raw_string_ostream out(out_buffer);

    out << "\n";

    for (auto& var_lt : lt_table) {
        auto* var = var_lt.first;
        auto* lt = var_lt.second;

        out << "Var: ";
        var->printAsOperand(out);
        out << " Def: " << std::to_string(lt->def);
        out << " Use: " << std::to_string(lt->use);
        out << "\t\t|";

        assert(lt->use > lt->def);

        std::string def_space;
        for (unsigned int i = 0; i < lt->def; i++) {
            def_space += "  |";
        }

        std::string alive_space;
        for (unsigned int i = 0; i < lt->use - lt->def; i++) {
            alive_space += "##|";
        }

        std::string after_use_space;
        for (unsigned int i = 0; i < n_stages - lt->use; i++) {
            after_use_space += "  |";
        }

        out << def_space << alive_space << after_use_space << "\n";
    }

    out << "\n";

    std::cout << out_buffer;
}

unsigned int binding::BitpackRegBinding::splitTableIntoEqWidthPools() {
    unsigned int min_width = UINT_MAX;

    for (auto& var_lt : lt_table) {
        auto width = var_lt.second->weight;

        if (width < min_width) {
            min_width = width;
        }
    }

    for (auto& var_lt : lt_table) {
        auto width = var_lt.second->weight;
        /* For now only multiple of min width */
        assert((width % min_width) == 0);
        auto n_pools = width / min_width;

        for (unsigned int pool_id = 0; pool_id < n_pools; pool_id++) {
            auto* new_pool = 
                new LifetimeWidthPool(
                    var_lt.first,
                    var_lt.second,
                    pool_id
                );

            lt_pool_table.push_back(new_pool);
            lt_pool_map[var_lt.first].push_back(new_pool);
        }
    }

    return min_width;
}

bool binding::BitpackRegBinding::defAfter(LifetimeWidthPool* first, LifetimeWidthPool* second) {
    return first->lt->def > second->lt->def;
}

bool binding::BitpackRegBinding::defBefore(LifetimeWidthPool* first, LifetimeWidthPool* second) {
    return first->lt->def < second->lt->def;
}

void binding::BitpackRegBinding::performLeftEdge()
{
    std::sort(lt_pool_table.begin(), lt_pool_table.end(), defBefore);

    std::string out_buffer;
    raw_string_ostream out(out_buffer);

    out << "\n";

    for (auto& pool : lt_pool_table) {
        auto* var = pool->var;
        auto* lt = pool->lt;

        out << "Var: ";
        var->printAsOperand(out);
        out << "\tID: " << std::to_string(pool->id);
        out << " Def: " << std::to_string(lt->def);
        out << " Use: " << std::to_string(lt->use);
        out << "\t\t|";

        assert(lt->use > lt->def);

        std::string def_space;
        for (unsigned int i = 0; i < lt->def; i++) {
            def_space += "  |";
        }

        std::string alive_space;
        for (unsigned int i = 0; i < lt->use - lt->def; i++) {
            alive_space += "##|";
        }

        std::string after_use_space;
        for (unsigned int i = 0; i < n_stages - lt->use; i++) {
            after_use_space += "  |";
        }

        out << def_space << alive_space << after_use_space << "\n";
    }

    out << "\n";

    RegId cur_sub_reg_id = 0;

    unsigned int pool_size = lt_pool_table.size();
    for (unsigned int i = 0; i < pool_size; i++) {
        if (sub_binding_map.size() == lt_pool_table.size()) {
            break;
        }

        auto* cur_pool = lt_pool_table[i];
        auto cur_pool_id = lt_pool_table[i]->id; 
        auto* cur_var = lt_pool_table[i]->var;
        auto* cur_lt = lt_pool_table[i]->lt;

        auto last_use_edge = cur_lt->use;

        if (sub_binding_map.count(cur_pool) != 0) {
            continue;
        } else {
            sub_binding_map[cur_pool] = cur_sub_reg_id;
        }

        out << "Reg ID: " << std::to_string(cur_sub_reg_id);
        out << " Var: ";
        cur_var->printAsOperand(out);
        out << " Pool: " << std::to_string(cur_pool_id);

        for (unsigned int j = 0; j < pool_size; j++) {
            if (sub_binding_map.size() == lt_pool_table.size()) {
                break;
            }

            auto* other_pool = lt_pool_table[j];
            auto other_pool_id = lt_pool_table[j]->id; 
            auto* other_var = lt_pool_table[j]->var;
            auto* other_lt = lt_pool_table[j]->lt;

            if (sub_binding_map.count(other_pool) != 0) {
                continue;
            }

            if (other_lt->def >= last_use_edge) {
                sub_binding_map[other_pool] = cur_sub_reg_id;
                last_use_edge += (other_lt->use - other_lt->def);

                out << " | Var: ";
                other_var->printAsOperand(out);
                out << " Pool: " << std::to_string(other_pool_id);
            }
        }

        cur_sub_reg_id++;
        out << "\n";
    }

    n_sub_regs = cur_sub_reg_id;

    std::cout << out_buffer;
}

bool binding::BitpackRegBinding::subBindingPredicate(std::pair<Value*, std::set<SubRegId>>& first,
                                                          std::pair<Value*, std::set<SubRegId>>& second)
{
    return first.second.size() > second.second.size();
}

void binding::BitpackRegBinding::mergeSubRegisters(unsigned int pool_width)
{
    std::string out_buffer;
    raw_string_ostream out(out_buffer);

    for (auto& var_pool_set : lt_pool_map) {
        auto sub_regs = std::make_pair(var_pool_set.first, std::set<SubRegId>());
        for (auto* pool : var_pool_set.second) {
            sub_regs.second.insert(sub_binding_map[pool]);
        }
        var_sub_regs.push_back(sub_regs);
    }

    std::sort(
        var_sub_regs.begin(),
        var_sub_regs.end(),
        subBindingPredicate
    );

#ifndef NDEBUG
    out << "\n";

    for (auto& v_sr : var_sub_regs) {
        out << "Var: ";
        v_sr.first->printAsOperand(out);
        out << "Regs: { ";
        for (auto reg_id : v_sr.second) {
            out << std::to_string(reg_id) << " ";
        }

        out << "}\n";
    }
#endif

    /* Merge subregisters */
    std::map<SubRegId, RegId> bitfield_reg_lookup;
    std::map<RegId, std::set<SubRegId>> reg_bitfields;

    RegId cur_reg_id = 0;
    for (auto& v_sr : var_sub_regs) {
        std::optional<RegId> binded_reg = std::nullopt;

        for (auto sub_reg_id : v_sr.second) {
            if (bitfield_reg_lookup.count(sub_reg_id) != 0) {
                binded_reg = bitfield_reg_lookup[sub_reg_id];
                break;
            }
        }

        if (!binded_reg.has_value()) {
            binded_reg = cur_reg_id;
            cur_reg_id++;
        }

        for (auto sub_reg_id : v_sr.second) {
            bitfield_reg_lookup[sub_reg_id] = binded_reg.value();
            reg_bitfields[binded_reg.value()].insert(sub_reg_id);
        }
    }

    n_regs = cur_reg_id;

#ifndef NDEBUG
    out << "\n";

    for (auto& reg_bitfield_set : reg_bitfields) {
        out << "Reg ID: " << std::to_string(reg_bitfield_set.first);
        out << " SubReg ID: { ";
        for (auto reg_id : reg_bitfield_set.second) {
            out << std::to_string(reg_id) << " ";
        }

        out << "}\n";
    }
#endif

    /* Verify merging */
    for (SubRegId id = 0; id < n_sub_regs; id++) {
        assert(bitfield_reg_lookup.count(id) != 0 && "Sub reg was not merged");
    }

    /* Create ordered bitfield set for each register */
    std::map<RegId, std::vector<SubRegId>> ordered_reg_bitfields;
    for (auto& reg_bitfield_set : reg_bitfields) {
        auto& sub_reg_ids = ordered_reg_bitfields[reg_bitfield_set.first];

        sub_reg_ids.assign(
            reg_bitfield_set.second.begin(),
            reg_bitfield_set.second.end()
        );

        std::sort(sub_reg_ids.begin(),sub_reg_ids.end());
    }

    /* Create register and its bitfields */

    registers.resize(n_regs);
    bitfields.resize(n_sub_regs);

    for (auto& reg_bitfield_set : ordered_reg_bitfields) {
        unsigned int width = reg_bitfield_set.second.size() * pool_width;
        RegId id = reg_bitfield_set.first;
 
        auto reg = Reg(id, width);
        registers[id] = reg;

        unsigned int lsb = 0;
        unsigned int msb = pool_width - 1;

        for (auto sub_reg_id : reg_bitfield_set.second) {
            bitfields[sub_reg_id] = RegBitfield(reg, lsb, msb);
            lsb += pool_width;
            msb += pool_width;
        }
    }

#ifndef NDEBUG
    std::cout << out_buffer;
#endif
}

void binding::BitpackRegBinding::mapVariablesToBitfields() {
    for (auto& v_sr : var_sub_regs) {
        auto* var = v_sr.first;

        assert(!v_sr.second.empty());
        RegBitfield bf(bitfields[*v_sr.second.begin()]);

        for (auto& sub_reg_id : v_sr.second) {
            bf = bf + bitfields[sub_reg_id];
        }

        binding_map[var] = bf;
    }

#ifndef NDEBUG
    std::string out_buffer;
    raw_string_ostream out(out_buffer);

    out << "\n";

    for (auto& var_bind : binding_map) {
        out << "Var: ";
        var_bind.first->printAsOperand(out);
        out << "\tReg ID: " << std::to_string(var_bind.second.reg.id);
        out << "\tReg Width: " << std::to_string(var_bind.second.reg.width);
        out << "\tBitfield: [" << std::to_string(var_bind.second.bitfield.second);
        out << ":" << std::to_string(var_bind.second.bitfield.first) << "]\n";
    }

    std::cout << out_buffer;
#endif
}