#ifndef __SCHEDULING_SDC_SCHEDULER_HPP__
#define __SCHEDULING_SDC_SCHEDULER_HPP__

#include <map>
#include <utility>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>

#include <lpsolve/lp_lib.h>

#include "SchedulerMapping.hpp"
#include "Dag.hpp"
#include "Scheduler.hpp"

namespace llvm {
    namespace bphls {

class SdcScheduler : public Scheduler {
public:
    SdcScheduler(Function& function, Dag& dag);

    SchedulerMapping& schedule() override;

    ~SdcScheduler() {};

private:
    Function& function;
    Dag& dag;

    unsigned int n_instr;

    lprec* lp_solver;

    SchedulerMapping mapping;

    std::map<InstructionNode*, std::pair<unsigned int, unsigned int>> instr_node_lp_var_lookup;

    enum Axap {
        Asap,
        Alap,
    };

    unsigned int createLpVariables();

    void addMulticycleConstraints();

    void addDependencyConstraints();

    void addTimingConstraints();

    void addAlapConstraints();

    void addResourseConstraint(unsigned int opcode, unsigned int constraint);

    void scheduleResourseConstrained();

    void scheduleAxap(Axap axap);

    void scheduleAsap();

    void scheduleAlap();

    void mapSchedule();

#ifndef NDEBUG
    void printLp();
#endif
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __SCHEDULING_SDC_SCHEDULER_HPP__ */

