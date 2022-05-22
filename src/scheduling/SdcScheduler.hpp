#ifndef __SCHEDULING_SDC_SCHEDULER_HPP__
#define __SCHEDULING_SDC_SCHEDULER_HPP__

#include <map>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include <lpsolve/lp_lib.h>

#include "fsm/Fsm.hpp"
#include "dag/Dag.hpp"
#include "Scheduler.hpp"

namespace llvm {
    namespace bphls {

class SdcScheduler : public Scheduler {
public:
    SdcScheduler();

    Fsm* schedule(Function& function, Dag& dag) override;

private:
    unsigned int n_instr;

    lprec* lp_solver;

    std::map<InstructionNode*, unsigned int> instr_start_lpvar_lookup;
    std::map<InstructionNode*, unsigned int> instr_end_lpvar_lookup;

    enum Axap {
        Asap,
        Alap,
    };

    static unsigned int getInstructionCycles(Instruction& instr);

    unsigned int createLpVariables(Function& function, Dag& dag);

    void addMulticycleConstraints(Function& function, Dag& dag);

    void addDependencyConstraints(Function& function, Dag& dag);

    void addTimingConstraints(Function& function, Dag& dag);

    void addAlapConstraints(Function& function, Dag& dag);

    void addResourseConstraint(Function& function, Dag& dag, unsigned int opcode, unsigned int constraint);

    void scheduleResourseConstrained(Function& function, Dag& dag);

    void scheduleAxap(Axap axap);

    void scheduleAsap();

    void scheduleAlap();

    SchedulerMapping* mapSchedule(Function& function, Dag& dag);
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SDC_SCHEDULER_HPP__ */

