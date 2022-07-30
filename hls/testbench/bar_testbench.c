#include <stdint.h>
#include <stdio.h>

uint32_t bar(uint8_t a,
             uint8_t b,
             uint8_t c,
             uint16_t d,
             uint16_t e,
             uint32_t f);

int main(int argc, char const *argv[]) {
    uint32_t bar_result = bar (
        10,
        5,
        3,
        90,
        189,
        780
    );

    printf("bar result: %u\n", bar_result);

    return 0;
}
