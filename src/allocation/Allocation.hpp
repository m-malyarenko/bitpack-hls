#ifndef __ALLOCATION_ALLOCATION_HPP__
#define __ALLOCATION_ALLOCATION_HPP__

#include <map>
#include <string>

#include <llvm/IR/Function.h>

#include "../scheduling/fsm/Fsm.hpp"
#include "../hardware/FunctionalUnit.hpp"

namespace llvm {
    namespace bphls {

class Allocation {
public:
    Allocation(Function& function, Fsm& fsm);

    void calculateFuAllocation();
private:
    Function& function;
    Fsm& fsm;

    std::map<hardware::FunctionalUnit, unsigned int> fu_allocation;
};

    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __ALLOCATION_ALLOCATION_HPP__ */
