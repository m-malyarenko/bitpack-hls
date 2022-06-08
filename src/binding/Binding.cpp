#include <iostream>

#include <string>
#include <map>
#include <set>

#include <llvm/IR/Instruction.h>

#include "../math/HungarianMethod.hpp"

#include "../hardware/HardwareConstraints.hpp"
#include "../scheduling/fsm/FsmState.hpp"
#include "../scheduling/fsm/Fsm.hpp"
#include "Binding.hpp"

extern llvm::bphls::hardware::HardwareConstraints* constraints;

using namespace llvm;
using namespace bphls;

void binding::Binding::assignInstructions() {
    auto& fu_num_constrints = constraints->getFuNumConstraints();

    std::set<Instruction*> shareable_instr;

    for (auto* state : fsm.states()) {
        for (auto* instr : state->instructions()) {
            // TODO Add check if operation can be shared (by default it can)
            if (!isa<ReturnInst>(instr)) {
                shareable_instr.insert(instr);
            }
        }
    }

    AssignInfo assign_info;

    findIndependantInstructions(
        shareable_instr,
        assign_info.independant_instructions
    );

    // for (auto& instr_map : assign_info.independant_instructions) {
    //     std::cout << "\nInstruction: " << instr_map.first->getOpcodeName() << std::endl;
    //     for (auto* ind_instr : instr_map.second) {
    //         std::cout << "\tIndep from : " << ind_instr->getOpcodeName() << std::endl;
    //     }
    // }

    for (auto* state : fsm.states()) {
        state_idx_instr_lookup.clear();

        for (auto& fu_num : fu_num_constrints) {
            auto* fu = fu_num.first;
            auto num_constrint = fu_num.second;

            if (num_constrint.has_value()) {
                bindFuInState(state, fu, num_constrint.value(), assign_info);
            }
        }
    }
}

bool binding::Binding::exists(Instruction* instr) {
    return instr_fu_map.count(instr) != 0;
}

binding::Binding::FuInstId& binding::Binding::getBindedFu(Instruction* instr) {
    assert(exists(instr));
    return instr_fu_map[instr];
}

void binding::Binding::bindFuInState(FsmState* state,
                                     hardware::FunctionalUnit* fu,
                                     unsigned int n_available,
                                     AssignInfo& assign_info)
{
    Table weights(n_available, std::vector<int>(n_available));
    Table assignment(n_available, std::vector<int>(n_available));

    int instr_idx = 0;
    for (auto* instr : state->instructions()) {
        if (constraints->getInstructionFu(*instr) != fu) {
            continue;
        }
        // std::cout << "Sharing instruction: " << instr->getOpcodeName() << std::endl;
        // TODO Add check if operation can be shared (by default it can)

        createWeights(
            instr,
            instr_idx,
            fu,
            n_available,
            assign_info,
            weights
        );

        state_idx_instr_lookup[instr_idx] = instr;
        instr_idx += 1;
    }
    
    unsigned int n_share = instr_idx;
    if (n_share >= 1) {
        assert(n_share <= n_available);

        solveBwm(weights, assignment);

        verifyBwm(n_share, n_available, assignment);

        updateAssignment(
            n_share,
            n_available,
            fu,
            assign_info,
            assignment
        );
    }
}

void binding::Binding::createWeights(Instruction* instr,
                                     unsigned int instr_idx,
                                     hardware::FunctionalUnit* fu,
                                     unsigned int n_available,
                                     AssignInfo& assign_info,
                                     Table& weights)
{
    static const int EXIST_IN_MUX_FACTOR = 1;
    static const int NEW_IN_MUX_FACTOR = 10;
    static const int OUT_SHARE_REG_FACTOR = -5;


    for (unsigned int fu_inst = 0; fu_inst < n_available; fu_inst++) {
        int weight = 0;
        auto fu_inst_id = std::make_pair(fu, fu_inst);

        for (auto& op : instr->operands()) {
            auto* op_instr = dyn_cast<Instruction>(&op);

            if (op_instr == nullptr) {
                continue;
            }

            if (assign_info.existing_operands[fu_inst_id].count(op_instr) == 0) {
                weight += NEW_IN_MUX_FACTOR;
            }
        }

        bool is_out_reg_sharable = false;
        for (auto* instr : assign_info.existing_instructions[fu_inst_id]) {
            bool is_independant =
                assign_info.independant_instructions.count(instr) != 0;

            if (is_independant) {
                is_out_reg_sharable = true;
                break;
            }
        }

        if (is_out_reg_sharable) {
            weight += OUT_SHARE_REG_FACTOR;
        }

        weight += EXIST_IN_MUX_FACTOR * assign_info.mux_inputs[fu_inst_id];

        weights[fu_inst][instr_idx] = weight;
    }
}

void binding::Binding::solveBwm(Table& weights, Table& assignment) {
    hungarian_problem_t hungarian_solver;

    unsigned int rows = weights.size();
    assert(rows > 0);
    unsigned int cols = weights[0].size();
    assert(cols > 0);

    // not necessary but good to enforce
    assert(rows == cols);

    int** matrix = tableToRawMatrix(weights, rows, cols);

    /* Initialize the hungarian_problem using the cost matrix*/
    hungarian_init(&hungarian_solver, matrix, rows, cols, HUNGARIAN_MODE_MINIMIZE_COST);

    /* solve the assignement problem */
    hungarian_solve(&hungarian_solver);

    for(unsigned int i = 0; i < rows; i++) {
        for(unsigned int j = 0; j < cols; j++) {
            assignment[i][j] = hungarian_solver.assignment[i][j];
        }
    }

    /* free used memory */
    hungarian_free(&hungarian_solver);

    for(unsigned int i = 0; i < rows; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;
}

void binding::Binding::verifyBwm(unsigned int n_share, unsigned int n_available, Table& assignment) {
    unsigned int n_assigned = 0;
    for (unsigned int fu_inst = 0; fu_inst < n_available; fu_inst++) {
        for (unsigned int fu_inst_shared = 0; fu_inst_shared < n_share; fu_inst_shared++) {
            if (assignment[fu_inst][fu_inst_shared] != 0) {
                n_assigned += 1;
            }
        }
    }

    if (n_assigned != n_share) {
        std::cerr << "Assigned: " << n_assigned << "\n";
        std::cerr << "To share: " << n_share << "\n";

        assert(0 && "All operations weren't assigned!");
    }
}

void binding::Binding::updateAssignment(unsigned int n_share,
                                        unsigned int n_available,
                                        hardware::FunctionalUnit* fu,
                                        AssignInfo& assign_info,
                                        Table& assignment)
{
    assert(n_share <= n_available);

    for (unsigned int fu_inst = 0; fu_inst < n_available; fu_inst++) {
        for (int fu_inst_shared= 0; fu_inst_shared < n_share; fu_inst_shared++) {
            if (assignment[fu_inst][fu_inst_shared] != 0) {
                auto* instr = state_idx_instr_lookup[fu_inst_shared];

                auto fu_inst_id = std::make_pair(fu, fu_inst);
                instr_fu_map[instr] = fu_inst_id;
                fu_instr_set_map[fu_inst_id].push_back(instr);

                unsigned int n_operands = 0;
                for (auto& op : instr->operands()) {
                    n_operands++;
                    
                    auto* operand = dyn_cast<Instruction>(&op);

                    if (operand == nullptr) {
                        continue;
                    }

                    if (assign_info.existing_operands[fu_inst_id].count(operand) == 0) {
                        assign_info.existing_operands[fu_inst_id].insert(operand);
                        assign_info.mux_inputs[fu_inst_id]++;
                    }
                }

                assign_info.existing_instructions[fu_inst_id].insert(instr);
            }
        }
    }
}

void binding::Binding::findIndependantInstructions(
    std::set<Instruction*>& instructions,
    std::map<Instruction*, std::set<Instruction*>>& independant_instructions
) {
    std::map<Instruction*, std::set<BasicBlock*>> live_blocks;
    findInstructionsLiveBlocks(instructions, live_blocks);

    for (auto libe_block : live_blocks) {
        independant_instructions.insert(
            std::make_pair(libe_block.first, std::set<Instruction*>())
        );
    }

    findIndependantInstructionsInLiveBlocks(independant_instructions, live_blocks);

    for (auto& instr_instr_set : independant_instructions) {
        auto* instr = instr_instr_set.first;

        /* Erase instruction from its independant list */
        independant_instructions[instr].erase(instr);
    }
}

void binding::Binding::findInstructionsLiveBlocks(
    std::set<Instruction*>& instructions,
    std::map<Instruction*, std::set<BasicBlock*>>& live_blocks
) {
    for (auto* instr : instructions) {
        live_blocks.insert(std::make_pair(instr, std::set<BasicBlock*>()));
        auto instr_idx = lva.getBitPosition(instr);

        for (auto bb_info : lva.iter_bb_info()) {
            auto* basic_block = bb_info.first;
            auto* info = bb_info.second;

            bool instr_is_alive =
                info->use.test(instr_idx)
                    || info->def.test(instr_idx)
                    || info->in.test(instr_idx)
                    || info->out.test(instr_idx);

            if (instr_is_alive) {
                live_blocks[instr].insert(basic_block);
            }
        }
    }
}

void binding::Binding::findIndependantInstructionsInLiveBlocks(
    std::map<Instruction*, std::set<Instruction*>>& independant_instructions,
    std::map<Instruction*, std::set<BasicBlock*>>& live_blocks
) {
    auto end = live_blocks.end();
    for (auto instr_bb_set_a = live_blocks.begin(); instr_bb_set_a != end; instr_bb_set_a++) {
        for (auto instr_bb_set_b = instr_bb_set_a; instr_bb_set_b != end; instr_bb_set_b++) {

            auto* instr_a = instr_bb_set_a->first;
            auto& bb_set_a = instr_bb_set_a->second;

            auto* instr_b = instr_bb_set_b->first;
            auto& bb_set_b = instr_bb_set_b->second;

            // std::cout << "Instr A: " << instr_a->getOpcodeName() << std::endl;
            // std::cout << "Instr b: " << instr_b->getOpcodeName() << std::endl;

            if (independant_instructions[instr_a].count(instr_b) != 0) {
                continue;
            }

            /* Must be both optimized or non-optimaized */
            if (isRegOptimized(instr_a, fsm.getEndState(instr_a))
                    != isRegOptimized(instr_b, fsm.getEndState(instr_b)))
            {
                continue;
            }

            std::set<BasicBlock*> intersection;
            std::set_intersection(
                bb_set_a.begin(), bb_set_a.end(),
                bb_set_b.begin(), bb_set_b.end(),
                std::inserter(intersection, intersection.begin())
            );

            if (intersection.empty()
                    || areIndependantInStates(instr_a, instr_b, intersection))
            {
                /* Insreuctions are independant */
                independant_instructions[instr_a].insert(instr_b);
                independant_instructions[instr_b].insert(instr_a);
            }
        }
    }
}

bool binding::Binding::isRegOptimized(Instruction* instr, FsmState* state) {
    std::set<Instruction*> phi_nodes;
    if (checkPhiSuccessors(instr, state, phi_nodes)) {
        return false;
    }

    if (phi_nodes.empty()) {
        return true;
    }

    visitTransitionStates(state, phi_nodes);

    return phi_nodes.empty();
}

bool binding::Binding::areIndependantInStates(Instruction* instr_a,
                            Instruction* instr_b,
                            std::set<BasicBlock*>& intersection)
{
    std::map<FsmState*, unsigned int> state_encoding;
    std::map<BasicBlock*, unsigned int> bb_last_state;

    createStateEncoding(
        (*intersection.begin())->getParent(),
        state_encoding,
        bb_last_state
    );

    bool are_independant = true;

    for (auto* basic_block : intersection) {
        assert(bb_last_state.count(basic_block) != 0);

        unsigned int first_state = 0;
        unsigned int last_state = bb_last_state[basic_block];

        std::vector<unsigned int> instr_a_states;
        std::vector<unsigned int> instr_b_states;

        addInstructionDefState(
            basic_block,
            instr_a,
            instr_a_states,
            instr_b,
            instr_b_states,
            state_encoding
        );

        addInstructionUseState(
            basic_block,
            instr_a,
            instr_a_states,
            instr_b,
            instr_b_states,
            state_encoding,
            last_state
        );

        addInstructionLiveInOut(
            basic_block,
            instr_a,
            instr_a_states,
            instr_b,
            instr_b_states,
            state_encoding,
            first_state,
            last_state
        );

        assert(!instr_a_states.empty());
        assert(!instr_b_states.empty());

        auto min_state_a = *std::min_element(instr_a_states.begin(), instr_a_states.end());
        auto max_state_a = *std::max_element(instr_a_states.begin(), instr_a_states.end());
        auto min_state_b = *std::max_element(instr_b_states.begin(), instr_b_states.end());
        auto max_state_b = *std::max_element(instr_b_states.begin(), instr_b_states.end());

        if (!((min_state_a > max_state_b) || (max_state_a < min_state_b))) {
            are_independant = false;
            break;
        }
    }

    return are_independant;
}

bool binding::Binding::checkPhiSuccessors(Instruction* instr,
                        FsmState* state,
                        std::set<Instruction*>& phi_nodes)
{
    for (auto& val : instr->uses()) {
        auto* successor = dyn_cast<Instruction>(&val);

        if (successor == nullptr) {
            continue;
        }

        if (isa<PHINode>(successor)) {
            phi_nodes.insert(successor);
            continue;
        }

        if (fsm.getEndState(successor) != state) {
            return true;
        }
    }
    return false;
}

void binding::Binding::visitTransitionStates(FsmState* state, std::set<Instruction*>& phi_nodes) {
    auto* default_state = state->getDefaultTransition();   
    for (auto* instr : default_state->instructions()) {
        if (!isa<PHINode>(instr)) {
            break;
        } else {
            phi_nodes.erase(instr);
        }
    }

    auto n_transition = state->getTransitionsNum();

    if (n_transition > 1) {
        for (unsigned int i = 0; i < n_transition; i++) {
            auto* trans_state = state->getTransitionState(i);

            for (auto* instr : trans_state->instructions()) {
                if (!isa<PHINode>(instr)) {
                    break;
                } else {
                    phi_nodes.erase(instr);
                }
            }
        }
    }
}

void binding::Binding::createStateEncoding(Function* function,
                                           std::map<FsmState*, unsigned int>& state_encoding,
                                           std::map<BasicBlock*, unsigned int>& bb_last_state)
{
    for (auto& basic_block : *function) {
        unsigned order = 0;

        FsmState* cur_state = nullptr;

        for (auto* state : fsm.states()) {
            if (state->getBasicBlock() == &basic_block) {
                cur_state = state;
                break;
            }

            for (auto* instr : state->instructions()) {
                if (instr->getParent() == &basic_block) {
                    cur_state = state;
                    break;
                }
            }

            if (cur_state != nullptr) {
                break;
            }
        }

        assert(cur_state != nullptr);

        while (cur_state != nullptr) {
            if (state_encoding.count(cur_state) == 0) {
                state_encoding[cur_state] = order;
                order++;
            }

            if (cur_state->getTerminatingFlag()) {
                break;
            };

            if (cur_state == cur_state->getDefaultTransition()) {
                /* For function calls - not used */
                assert(cur_state->getTransitionsNum() == 2);
                cur_state = cur_state->getTransitionState(0);
            } else {
                cur_state = cur_state->getDefaultTransition();
            }
        }

        bb_last_state[&basic_block] = order - 1;
    }
}

void binding::Binding::addInstructionDefState(
    BasicBlock* basic_block,
    Instruction* instr_a,
    std::vector<unsigned int>& instr_a_states,
    Instruction* instr_b,
    std::vector<unsigned int>& instr_b_states,
    std::map<FsmState*, unsigned int>& state_encoding
) {
    if (instr_a->getParent() == basic_block) {
        assert(state_encoding.count(fsm.getEndState(instr_a)) != 0);
        instr_a_states.push_back(state_encoding[fsm.getEndState(instr_a)]);
    }

    if (instr_b->getParent() == basic_block) {
        assert(state_encoding.count(fsm.getEndState(instr_b)) != 0);
        instr_b_states.push_back(state_encoding[fsm.getEndState(instr_b)]);
    }
}

void binding::Binding::addInstructionUseState(
    BasicBlock* basic_block,
    Instruction* instr_a,
    std::vector<unsigned int>& instr_a_states,
    Instruction* instr_b,
    std::vector<unsigned int>& instr_b_states,
    std::map<FsmState*, unsigned int>& state_encoding,
    unsigned int last_state
) {
    for (auto* user : instr_a->users()) {
        auto* successor = dyn_cast<Instruction>(user);
        if (successor == nullptr) {
            continue;
        }

        if (successor == basic_block->getTerminator()) {
            instr_a_states.push_back(last_state);
            continue;
        }

        if (state_encoding.count(fsm.getEndState(successor)) != 0) {
            instr_a_states.push_back(state_encoding[fsm.getEndState(successor)]);
        }
    }

    for (auto* user : instr_b->users()) {
        auto* successor = dyn_cast<Instruction>(user);
        if (successor == nullptr) {
            continue;
        }

        if (successor == basic_block->getTerminator()) {
            instr_b_states.push_back(last_state);
            continue;
        }

        if (state_encoding.count(fsm.getEndState(successor)) != 0) {
            instr_b_states.push_back(state_encoding[fsm.getEndState(successor)]);
        }
    }
}

void binding::Binding::addInstructionLiveInOut(
    BasicBlock* basic_block,
    Instruction* instr_a,
    std::vector<unsigned int>& instr_a_states,
    Instruction* instr_b,
    std::vector<unsigned int>& instr_b_states,
    std::map<FsmState*, unsigned int>& state_encoding,
    unsigned int first_state,
    unsigned int last_state
) {
    auto instr_a_idx = lva.getBitPosition(instr_a);
    auto instr_b_idx = lva.getBitPosition(instr_b);

    if (lva.getInfo(basic_block)->out.test(instr_a_idx)) {
        instr_a_states.push_back(last_state);
    }
    if (lva.getInfo(basic_block)->in.test(instr_a_idx)) {
        instr_a_states.push_back(first_state);
    }

    if (lva.getInfo(basic_block)->out.test(instr_b_idx)) {
        instr_a_states.push_back(last_state);
    }
    if (lva.getInfo(basic_block)->in.test(instr_b_idx)) {
        instr_a_states.push_back(first_state);
    }
}

int** binding::Binding::tableToRawMatrix(Table& table, unsigned int rows, unsigned int cols) {
    int** matrix = new int*[rows];
    for (unsigned int i = 0; i < rows; i++) {
        matrix[i] = new int[cols];
        for (unsigned int j = 0; j < cols; j++) {
            matrix[i][j] = table[i][j];
        }
    }
    
    return matrix;
}