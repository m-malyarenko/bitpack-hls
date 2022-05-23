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

// namespace llvm {
//     namespace bphls {

// std::map<Instruction*, unsigned> asap_control_step;
// std::map<Instruction*, unsigned> alap_control_step;

//     } /* namespace bphls */
// } /* namespace llvm */

extern llvm::bphls::hardware::HardwareConstraints* constraints;

SdcScheduler::SdcScheduler()
    : n_instr(0) {}

SchedulerMapping* SdcScheduler::schedule(Function& function, Dag& dag) {
    unsigned int n_lpvar = createLpVariables(function, dag);

    lp_solver = make_lp(0, n_lpvar);

    /* Add multicycle variable constraints */
    addMulticycleConstraints(function, dag);

    /* Add dependency constraints */
    addDependencyConstraints(function, dag);

    /* Add timing constraints */
    addTimingConstraints(function, dag);

    /* Schdule with ASAP strategy */
    scheduleAsap();

    /* Take into account resourse constraints */
    scheduleResourseConstrained(function, dag);

    auto* mapping = mapSchedule(function, dag);

    delete_lp(lp_solver);

    return mapping;
}

unsigned int SdcScheduler::createLpVariables(Function& function, Dag& dag) {
    unsigned int n_lpvar = 0;
    // FIXME Only one basic block supported
    auto& basic_block = function.getBasicBlockList().front();

    /* Create LP variables */
    for (auto& instr : basic_block) {
        auto& instr_node = dag.getNode(instr);

        const unsigned int clk_latency = getInstructionCycles(instr);

        instr_start_lpvar_lookup[&instr_node] = n_lpvar;
        instr_end_lpvar_lookup[&instr_node] = n_lpvar + clk_latency;

        n_lpvar += clk_latency + 1;
        n_instr += 1;
    }

    return n_lpvar;
}

void SdcScheduler::addMulticycleConstraints(Function& function, Dag& dag) {
    int lpvar_col[2] = { 0, 0 };
    double lpvar_coeff[2] = { 0.0, 0.0 };

    for (auto& instr_node_lpvar : instr_start_lpvar_lookup) {
        auto* instr_node = instr_node_lpvar.first;
        auto start_lpvar = instr_node_lpvar.second;
        auto end_lpvar = instr_end_lpvar_lookup[instr_node];

        if (start_lpvar == end_lpvar) {
            continue;
        }

        for (unsigned int i = start_lpvar + 1; i <= end_lpvar; i++) {
            lpvar_col[0] = 1 + i;
            lpvar_col[1] = 1 + (i - 1);

            lpvar_coeff[0] = 1.0;
            lpvar_coeff[0] = -1.0;

            add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, EQ, 1.0);
        }
    }
}

void SdcScheduler::addDependencyConstraints(Function& function, Dag& dag) {
    // FIXME Only one basic block supported
    auto& basic_block = function.getBasicBlockList().front();

    for (auto& instr : basic_block) {
        auto& instr_node = dag.getNode(instr);

        int lpvar_col[2] = { 0, 0 };
        double lpvar_coeff[2] = { 0.0, 0.0 };

        // First make sure each instruction is scheduled into a cycle >= 0
        lpvar_col[0] = 1 + instr_start_lpvar_lookup[&instr_node];
        lpvar_coeff[0] = 1.0;
        add_constraintex(lp_solver, 1, lpvar_coeff, lpvar_col, GE, 0.0);
        
        // Now handle the dependencies between instructions: producer/consumer relationships
        for (auto dep_instr : instr_node.dependencies()) {
            lpvar_col[0] = 1 + instr_start_lpvar_lookup[&instr_node];
            lpvar_col[1] = 1 + instr_end_lpvar_lookup[dep_instr];

            lpvar_coeff[0] = 1.0;
            lpvar_coeff[1] = -1.0;

            add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, GE, 1.0); // TODO Only no chaining option
        }


        /* TODO mamory dependencies */
    }
}

void SdcScheduler::addTimingConstraints(Function& function, Dag& dag) {
    // TODO Implement method
}

void SdcScheduler::addAlapConstraints(Function& function, Dag& dag) {
    double* lpvars = new REAL[get_Nrows(lp_solver)];
    get_variables(lp_solver, lpvars);

    std::map<BasicBlock*, unsigned int> bb_max_cycles;

    // FIXME Only one basic block supported
    auto& basic_block = function.getBasicBlockList().front();

    for (auto& instr : basic_block) {
        unsigned int idx = instr_end_lpvar_lookup[&dag.getNode(instr)];
        unsigned int assigned_state = lpvars[idx];

        bb_max_cycles[&basic_block] = std::max(bb_max_cycles[&basic_block], assigned_state);
    }

    int lpvar_col[2] = { 0, 0 };
    double lpvar_coeff[2] = { 0.0, 0.0 };

    // FIXME Only one basic block supported
    for (auto& instr : basic_block) {
        auto& instr_node = dag.getNode(instr);

        int start_idx = instr_start_lpvar_lookup[&instr_node];
        int end_idx = instr_end_lpvar_lookup[&instr_node];

        lpvar_col[0] = 1 + end_idx;
        lpvar_coeff[0] = 1.0;

        if (!instr.isTerminator()
            && (instr_node.dependencies().empty())
            && (lpvars[start_idx] == 0))
        {
            double end_state = end_idx - start_idx;
            unsigned int latency = SdcScheduler::getInstructionCycles(instr);

            assert(end_state == latency);

            add_constraintex(lp_solver, 1, lpvar_coeff, lpvar_col, EQ, end_state);
        } else {
            add_constraintex(lp_solver, 1, lpvar_coeff, lpvar_col, LE, (double) bb_max_cycles[&basic_block]);
        }
    }

    delete[] lpvars;
    lpvars = nullptr;
}

static double* static_lpvar_coeffs;
static std::map<InstructionNode*, unsigned int>* static_start_lpvar;

bool predicateAlap(InstructionNode* instr_a, InstructionNode* instr_b) {
  int diff =
    (int) static_lpvar_coeffs[(*static_start_lpvar)[instr_a]]
        - (int) static_lpvar_coeffs[(*static_start_lpvar)[instr_b]];

  if (diff < 0) {
    return true;
  } else if (diff == 0) {
    return ((*static_start_lpvar)[instr_a] - (*static_start_lpvar)[instr_b]) < 0;
  } else {
      return false;
  }
}

void SdcScheduler::addResourseConstraint(Function& function, Dag& dag, unsigned int opcode, unsigned int constraint) {
    static_lpvar_coeffs = new double[get_Nrows(lp_solver)];
    static_start_lpvar = &instr_start_lpvar_lookup;

    get_variables(lp_solver, static_lpvar_coeffs);
    std::vector<InstructionNode*> constrained_instr_nodes;

    // FIXME Only one basic block supported
    auto& basic_block = function.getBasicBlockList().front();

    constrained_instr_nodes.clear();

    for (auto& instr : basic_block) {
        auto& instr_node = dag.getNode(instr);

        unsigned int instr_opcode = instr.getOpcode();

        if (instr_opcode == opcode) {
            constrained_instr_nodes.push_back(&instr_node);
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

        lpvar_col[0] = 1 + instr_start_lpvar_lookup[instr_a];
        lpvar_col[1] = 1 + instr_start_lpvar_lookup[instr_b];

        lpvar_coeff[0] = 1.0;
        lpvar_coeff[1] = -1.0;

        // FIXME Add variable initiation cycles instead of 1 
        add_constraintex(lp_solver, 2, lpvar_coeff, lpvar_col, GE, 1);
    }

    delete[] static_lpvar_coeffs;
    static_lpvar_coeffs = nullptr;
}

void SdcScheduler::scheduleResourseConstrained(Function& function, Dag& dag) {
    // FIXME Only one basic block supported
    auto& basic_block = function.getBasicBlockList().front();

    const unsigned int n_lpvar_asap = get_Nrows(lp_solver);

    addAlapConstraints(function, dag);

    scheduleAlap();

    /* Delete ALAP constraints */
    for (unsigned int i = get_Nrows(lp_solver); i > n_lpvar_asap; i--) {
        del_constraint(lp_solver, i);
    }

    std::set<unsigned int> seen_opcodes;

    for (auto& instr : basic_block) {
        unsigned int instr_opcode = instr.getOpcode();

        if (seen_opcodes.find(instr_opcode) != seen_opcodes.end()) {
            continue;
        }

        auto* fu = constraints->instr_impl[instr_opcode];
        auto fu_num_constraint = constraints->fu_num_constraints[fu];

        if (fu_num_constraint.has_value()) {
            addResourseConstraint(function, dag, instr_opcode, fu_num_constraint.value());
        }
    }

    scheduleAsap();
}

void SdcScheduler::scheduleAxap(Axap axap) {
    int* lpvar_cols = new int[n_instr];
    double* lpvar_coeffs = new double[n_instr];

    unsigned int count = 0;
    for (auto& instr_node_lpvar : instr_start_lpvar_lookup) {
        unsigned lpvar_col = instr_node_lpvar.second;
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

#ifndef NDEBUG
    write_LP(lp_solver, stdout);
#endif

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

SchedulerMapping* SdcScheduler::mapSchedule(Function& function, Dag& dag) {
    SchedulerMapping* mapping = new SchedulerMapping();

    double* lpvars = new REAL[get_Nrows(lp_solver)];
    get_variables(lp_solver, lpvars);

    // FIXME Only one basic block supported
    auto& basic_block = function.getBasicBlockList().front();

    for (auto& instr : basic_block) {
        auto& instr_node = dag.getNode(instr);
        unsigned int start_idx = instr_start_lpvar_lookup[&instr_node];
        unsigned int assigned_state = static_cast<double>(lpvars[start_idx]);

        mapping->setState(&dag.getNode(instr), assigned_state);

        assigned_state += SdcScheduler::getInstructionCycles(instr);
    }

    delete[] lpvars;
    lpvars = nullptr;

    return mapping;
}
