#include <stdint.h>

void foo(volatile uint8_t a, volatile uint8_t b, volatile uint16_t c) {
    volatile uint16_t d = a * b;
    volatile uint16_t y = c + d;
}