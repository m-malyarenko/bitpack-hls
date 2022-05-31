#include <stdint.h>

uint32_t foo(uint32_t a, uint32_t b, uint32_t c) {
    uint32_t y_1 = a * b;
    uint32_t y_2 = c * 7;
    uint32_t z = y_1 + y_2;

    return z;
}