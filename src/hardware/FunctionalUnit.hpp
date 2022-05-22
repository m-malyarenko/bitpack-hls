#ifndef __HARDWARE_OPERATION_HPP__
#define __HARDWARE_OPERATION_HPP__

#include <cstdint>

namespace llvm {
    namespace bphls {
        namespace hardware {

struct FunctionalUnit {
    float f_max;
    float crit_delay;
    uint16_t latency;
    uint16_t n_lut;
    uint16_t n_reg;
    uint16_t n_log_element;

    FunctionalUnit(
        float f_max = 50.0F,
        float crit_delay = 0.0F,
        uint16_t latency = 0,
        uint16_t n_lut = 1,
        uint16_t n_reg = 1,
        uint16_t n_log_element = 1
    ) :
    f_max(f_max),
    crit_delay(crit_delay),
    latency(latency),
    n_lut(n_lut),
    n_reg(n_reg),
    n_log_element(n_log_element) {};
};

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_OPERATION_HPP__ */
