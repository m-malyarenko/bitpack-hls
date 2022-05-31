#include <iostream>
#include <set>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>

#include <lpsolve/lp_lib.h>

#include "../hardware/HardwareConstraints.hpp"
#include "Dag.hpp"
#include "SchedulerMapping.hpp"

#include "SdcScheduler.hpp"

using namespace llvm;
using namespace bphls;

extern llvm::bphls::hardware::HardwareConstraints* constraints;

SdcScheduler::SdcScheduler(Function& function, Dag& dag)
    : function(function),
      dag(dag),
      n_instr(0),
      mapping(function, dag) {}

SchedulerMapping& SdcScheduler::schedule() {
    unsigned int n_lp_var = createLpVariables();

    lp_solver = make_lp(0, n_lp_var);

    /* Add multicycle variable constraints */
    addMulticycleConstraints();

    /* Add dependency constraints */
    addDependencyConstraints();

    /* Add timing constraints */
    addTimingConstraints();

    /* Schdule with ASAP strategy */
    scheduleAsap();

#ifndef NDEBUG
    std::cout << "ASAP scheduling:" << std::endl;
    printLp();
#endif

    /* Take into account resourse constraints */
    scheduleResourseConstrained();

#ifndef NDEBUG
    std::cout << "Resource constrained scheduling:" << std::endl;
    printLp();
#endif

    mapSchedule();

    // TODO ? Add register type for roots and sinks of multicycle paths

    delete_lp(lp_solver);

    return mapping;
}

unsigned int SdcScheduler::createLpVariables() {
    unsigned int n_lpvar = 0;

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            auto& instr_node = dag.getNode(instr);

            const unsigned int clk_latency = getInstructionCycles(instr);

            instr_node_lp_var_lookup[&instr_node] =
                std::make_pair(n_lpvar, n_lpvar + clk_latency);

            n_lpvar += clk_latency + 1;
            n_instr += 1;
        }
    }

    return n_lpvar;
}

void SdcScheduler::addMulticycleConstraints() {
    int lpvar_col[2] = { 0, 0 };
    double lpvar_coeff[2] = { 0.0, 0.0 };

    for (auto& instr_node_lpvar : instr_node_lp_var_lookup) {
        auto start_lpvar = instr_node_lpvar.second.first;
        auto end_lpvar = instr_node_lpvar.second.second;

        if (start_lpvar == end_lpvar) {
            continue;
        }

        for (unsigned int i = start_lpvar + 1; i <= end_lpvar; i++) {
            lpvar_col[0] = 1 + i;
            lpvar_col[1] = 1 + (i - 1);

            lpvar_coeff[0] = 1.0;
            lpvar_coeff[1] = -1.0;

            add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, EQ, 1.0);
        }
    }
}

void SdcScheduler::addDependencyConstraints() {
    int lpvar_col[2] = { 0, 0 };
    double lpvar_coeff[2] = { 0.0, 0.0 };

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            auto& instr_node = dag.getNode(instr);

            lpvar_col[0] = 1 + instr_node_lp_var_lookup[&instr_node].first;
            lpvar_coeff[0] = 1.0;
            add_constraintex(lp_solver, 1, lpvar_coeff, lpvar_col, GE, 0.0);

            for (auto dep_instr : instr_node.dependencies()) {
                lpvar_col[0] = 1 + instr_node_lp_var_lookup[&instr_node].first;
                lpvar_col[1] = 1 + instr_node_lp_var_lookup[dep_instr].second;

                lpvar_coeff[0] = 1.0;
                lpvar_coeff[1] = -1.0;

                add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, GE, 1.0); // TODO Only no chaining option
            }

            for (auto* mem_dep_instr : instr_node.memory_dependencies()) {
                lpvar_col[0] = 1 + instr_node_lp_var_lookup[&instr_node].first;
                lpvar_col[1] = 1 + instr_node_lp_var_lookup[mem_dep_instr].second;

                lpvar_coeff[0] = 1.0;
                lpvar_coeff[1] = -1.0;

                add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, GE, 0.0);
            }
        }
    }
}

void SdcScheduler::addTimingConstraints() {
    // TODO Implement method
}

void SdcScheduler::addAlapConstraints() {
    double* lp_vars = new double[get_Nrows(lp_solver)];
    get_variables(lp_solver, lp_vars);

    std::map<BasicBlock*, unsigned int> bb_max_cycles;

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            unsigned int end_lpvar = instr_node_lp_var_lookup[&dag.getNode(instr)].second;
            unsigned int assigned_state = lp_vars[end_lpvar];

            bb_max_cycles[&basic_block] = std::max(bb_max_cycles[&basic_block], assigned_state);
        }
    }

    int lp_var_col[1] = { 0 };
    double lp_var_coeff[1] = { 0.0 };

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            auto& instr_node = dag.getNode(instr);

            int start_lp_var_idx = instr_node_lp_var_lookup[&instr_node].first;
            int end_lp_var_idx = instr_node_lp_var_lookup[&instr_node].second;

            lp_var_col[0] = 1 + end_lp_var_idx;
            lp_var_coeff[0] = 1.0;

            if (!instr.isTerminator()
                && (instr_node.dependencies().empty())
                && (static_cast<unsigned int>(lp_vars[start_lp_var_idx]) == 0))
            {
                double end_state = end_lp_var_idx - start_lp_var_idx;
                unsigned int latency = SdcScheduler::getInstructionCycles(instr);

                assert(end_state == latency);

                add_constraintex(lp_solver, 1, lp_var_coeff, lp_var_col, EQ, end_state);
            } else {
                add_constraintex(lp_solver, 1, lp_var_coeff, lp_var_col, LE, (double) bb_max_cycles[&basic_block]);
            }
        }
    }

    delete[] lp_vars;
    lp_vars = nullptr;
}

static double* static_lp_var_coeffs;
static std::map<InstructionNode*, std::pair<unsigned int, unsigned int>>* static_lp_var_lookup;

bool predicateAlap(InstructionNode* instr_a, InstructionNode* instr_b) {
  int diff =
    (int) static_lp_var_coeffs[(*static_lp_var_lookup)[instr_a].first]
        - (int) static_lp_var_coeffs[(*static_lp_var_lookup)[instr_b].first];

  if (diff < 0) {
    return true;
  } else if (diff == 0) {
    return ((*static_lp_var_lookup)[instr_a].first
                - (*static_lp_var_lookup)[instr_b].first) < 0;
  } else {
      return false;
  }
}

void SdcScheduler::addResourseConstraint(unsigned int opcode, unsigned int constraint) {
    static_lp_var_coeffs = new double[get_Nrows(lp_solver)];
    static_lp_var_lookup = &instr_node_lp_var_lookup;

    get_variables(lp_solver, static_lp_var_coeffs);
    std::vector<InstructionNode*> constrained_instr_nodes;

    for (auto& basic_block : function) {
        constrained_instr_nodes.clear();

        for (auto& instr : basic_block) {
            auto& instr_node = dag.getNode(instr);

            unsigned int instr_opcode = instr.getOpcode();

            if (instr_opcode == opcode) {
                constrained_instr_nodes.push_back(&instr_node);
            }
        }
    }

    std::sort(
        constrained_instr_nodes.begin(),
        constrained_instr_nodes.end(),
        predicateAlap
    );

    int lpvar_col[2] = { 0, 0 };
    double lpvar_coeff[2] = { 0.0, 0.0 };

    for (unsigned int i = constraint; i < constrained_instr_nodes.size(); i++) {
        auto* instr_a = constrained_instr_nodes[i];
        auto* instr_b = constrained_instr_nodes[i - constraint];

        lpvar_col[0] = 1 + instr_node_lp_var_lookup[instr_a].first;
        lpvar_col[1] = 1 + instr_node_lp_var_lookup[instr_b].first;

        lpvar_coeff[0] = 1.0;
        lpvar_coeff[1] = -1.0;

        // TODO Add variable initiation cycles instead of 1 
        add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, GE, 1);
    }

    delete[] static_lp_var_coeffs;
    static_lp_var_coeffs = nullptr;
}

void SdcScheduler::scheduleResourseConstrained() {
    const unsigned int n_lpvar_asap = get_Nrows(lp_solver);

    addAlapConstraints();

    scheduleAlap();

    /* Delete ALAP constraints */
    for (unsigned int i = get_Nrows(lp_solver); i > n_lpvar_asap; i--) {
        del_constraint(lp_solver, i);
    }

    std::set<unsigned int> seen_opcodes;

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            unsigned int instr_opcode = instr.getOpcode();

            if (seen_opcodes.find(instr_opcode) != seen_opcodes.end()) {
                continue;
            } else {
                seen_opcodes.insert(instr_opcode);
            }

            auto* fu = constraints->getInstructionFu(instr);
            auto fu_num_constraint = constraints->getFuNumConstraint(*fu);

            if (fu_num_constraint.has_value()) {
                std::cout << "Adding constraint\n\t OPCODE: " << instr_opcode
                    << " FU number: " << fu_num_constraint.value() << std::endl;

                addResourseConstraint(instr_opcode, fu_num_constraint.value());
            }
        }
    }

    scheduleAsap();
}

void SdcScheduler::scheduleAxap(Axap axap) {
    int* lpvar_cols = new int[n_instr];
    double* lpvar_coeffs = new double[n_instr];

    unsigned int count = 0;
    for (auto& instr_node_lpvar : instr_node_lp_var_lookup) {
        unsigned int lpvar_col = instr_node_lpvar.second.first;
        lpvar_cols[count] = 1 + lpvar_col;
        lpvar_coeffs[count] = 1.0;
        count += 1;
    }

    assert(count == n_instr);  

    set_obj_fnex(lp_solver, count, lpvar_coeffs, lpvar_cols);

    assert((axap == Asap) || (axap == Alap));

    switch (axap) {
    case Asap:
        set_minim(lp_solver);
        break;
    case Alap:
        set_maxim(lp_solver);
        break;
    }

// #ifndef NDEBUG
//     write_LP(lp_solver, stdout);
// #endif

    int lp_solver_status = solve(lp_solver);

    if (lp_solver_status != 0) {
        errs() << "LP solver returned: " << lp_solver_status << "\n";
        report_fatal_error("LP solver could not find an optimal solution");
    }

    delete[] lpvar_cols;
    lpvar_cols = nullptr;

    delete[] lpvar_coeffs;
    lpvar_coeffs = nullptr;
}

void SdcScheduler::scheduleAsap() {
    scheduleAxap(Asap);
}

void SdcScheduler::scheduleAlap() {
    scheduleAxap(Alap);
}

void SdcScheduler::mapSchedule() {
    double* lp_vars = new REAL[get_Nrows(lp_solver)];
    get_variables(lp_solver, lp_vars);

#ifndef NDEBUG
    std::string out_buffer;
    raw_string_ostream out_stream(out_buffer);
#endif

    unsigned bb_count = 0;

    for (auto& basic_block : function) {
        unsigned int bb_states_count = 0;

        for (auto& instr : basic_block) {
            auto& instr_node = dag.getNode(instr);
            auto start_lp_var_idx = instr_node_lp_var_lookup[&instr_node].first;
            auto assigned_state = static_cast<unsigned int>(lp_vars[start_lp_var_idx]);

#ifndef NDEBUG
            out_stream << "BB#" << bb_count << ": ";
            basic_block.printAsOperand(out_stream, false);
            out_stream << " OPCODE: " << instr.getOpcodeName();
            out_stream << " IDX: " << instr_node_lp_var_lookup[&instr_node].first + 1;
            out_stream << " CLOCK ASSIGNED: " << assigned_state << "\n";
#endif

            mapping.setState(&dag.getNode(instr), assigned_state);

            assigned_state += SdcScheduler::getInstructionCycles(instr);

            if (assigned_state > bb_states_count) {
                bb_states_count = assigned_state;
            }
        }

#ifndef NDEBUG
        std::cout << "\n" << out_stream.str() << std::endl;
#endif

        bb_count++;
        mapping.setBasicBlockStatesNum(&basic_block, bb_states_count);

        auto* ret_instr = dyn_cast<ReturnInst>(basic_block.getTerminator());
        if (ret_instr != nullptr) {
            mapping.setState(&dag.getNode(*ret_instr), bb_states_count);
        }
    }

    delete[] lp_vars;
    lp_vars = nullptr;
}

#ifndef NDEBUG
void SdcScheduler::printLp() {
    double* lp_vars = new double[get_Nrows(lp_solver)];
    get_variables(lp_solver, lp_vars);

    std::string out_buffer;
    raw_string_ostream out_stream(out_buffer);

    for (auto& basic_block : function) {
        for (auto& instr : basic_block) {
            auto& instr_node = dag.getNode(instr);
            auto end_lp_var_idx = instr_node_lp_var_lookup[&instr_node].second;

            out_stream << "BB: ";
            basic_block.printAsOperand(out_stream, false);
            out_stream << " OPCODE: " << instr.getOpcodeName();
            out_stream << " IDX: " << instr_node_lp_var_lookup[&instr_node].first + 1;
            out_stream << " CLOCK: " << lp_vars[end_lp_var_idx] << "\n";
        }
    }

    std::cout << "\n" << out_stream.str() << std::endl;
}
#endif
