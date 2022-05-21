#ifndef __HARDWARE_OPERATION_HPP__
#define __HARDWARE_OPERATION_HPP__

#include <cstdint>

namespace llvm {
    namespace bphls {
        namespace hardware {

struct Operation {
    float f_max;
    float crit_delay;
    uint16_t latency;
    uint16_t n_lut;
    uint16_t n_reg;
    uint16_t n_log_element;
};

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_OPERATION_HPP__ */
