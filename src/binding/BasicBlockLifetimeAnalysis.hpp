#ifndef __BINDING_BASIC_BLOCK_LIFETIME_ANALYSIS_HPP__
#define __BINDING_BASIC_BLOCK_LIFETIME_ANALYSIS_HPP__

#include <map>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/ValueMap.h>

#include "../scheduling/fsm/Fsm.hpp"

namespace llvm {
    namespace bphls {
        namespace binding {

class BasicBlockLifetimeAnalysis {
public:
    BasicBlockLifetimeAnalysis(Function& function, Fsm& fsm)
        : function(function),
          fsm(fsm) {}

    ~BasicBlockLifetimeAnalysis();

    struct LifetimeInterval {
        unsigned int def;
        unsigned int use;
        unsigned int weight;

        LifetimeInterval()
            : def(0),
              use(0),
              weight(0) {}

        LifetimeInterval(unsigned int def,
                         unsigned int use,
                         unsigned int weight)
            : def(def),
              use(use),
              weight(weight) {}

        LifetimeInterval(unsigned int weight)
            : def(0),
              use(0),
              weight(weight) {}
    };

    void analyze();

    void printLifetimeTable();

    std::vector<std::pair<Value*, LifetimeInterval*>> table;
private:
    Function& function;
    Fsm& fsm;
};

        } /* namespace binding */
    } /* namespace bphls */
} /* namespace llvm */

#endif /* __BINDING_BASIC_BLOCK_LIFETIME_ANALYSIS_HPP__ */
