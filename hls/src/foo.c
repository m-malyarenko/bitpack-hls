#include <stdint.h>

uint16_t foo(uint8_t a,
             uint8_t b,
             uint8_t c,
             uint8_t d,
             uint8_t e,
             uint16_t f)
{
    uint8_t x1 = a + d;
    uint8_t x2 = b + e;
    uint8_t x3 = x1 + c;

    uint16_t y1 = x3 * x2;
    uint16_t y2 = y1 + f;

    return y2;
}