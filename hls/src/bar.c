#include <stdint.h>

uint32_t bar(uint8_t a,
             uint8_t b,
             uint8_t c,
             uint16_t d,
             uint16_t e,
             uint32_t f)
{
    uint8_t x1 = a + 6;
    uint8_t x2 = b + c;
    uint16_t x3 = d + 12;
    uint16_t x4 = x1 * x2;
    uint16_t x5 = x3 + e;

    uint16_t y1 = x3 + x4;
    uint32_t y2 = y1 * x5;
    uint32_t y3 = f + y2;

    return y3;
}