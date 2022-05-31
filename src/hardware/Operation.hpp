#ifndef __HARDWARE_OPERATION_HPP__
#define __HARDWARE_OPERATION_HPP__

namespace llvm {
    namespace bphls {
        namespace hardware {

struct Operation {
    float f_max;
    float crit_delay;
    unsigned short latency;
    unsigned short n_lut;
    unsigned short n_reg;

    Operation(float f_max = 50.0F,
              float crit_delay = 0.0F,
              unsigned short latency = 0,
              unsigned short n_lut = 1,
              unsigned short n_reg = 1)
        : f_max(f_max),
          crit_delay(crit_delay),
          latency(latency),
          n_lut(n_lut),
          n_reg(n_reg) {};
};

        } /* namespace hardware */
    } /* namespace bphls */ 
} /* namespace llvm */

#endif /* __HARDWARE_OPERATION_HPP__ */

