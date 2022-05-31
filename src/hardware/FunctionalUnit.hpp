#ifndef __HARDWARE_FUNCTIONAL_UNIT_HPP__
#define __HARDWARE_FUNCTIONAL_UNIT_HPP__

#include <optional>

#include "Operation.hpp"

namespace llvm {
    namespace bphls {
        namespace hardware {

struct FunctionalUnit {
    Operation op;
    std::optional<unsigned int> num_constraint;

    FunctionalUnit(float f_max = 50.0F,
                   float crit_delay = 0.0F,
                   unsigned short latency = 0,
                   unsigned short n_lut = 1,
                   unsigned short n_reg = 1,
                   std::optional<unsigned int> num_constraint = std::nullopt)
        : op(f_max, crit_delay, latency, n_lut, n_reg),
          num_constraint(num_constraint) {};
};

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_FUNCTIONAL_UNIT_HPP__ */
