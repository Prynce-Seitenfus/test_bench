#include <stdint.h>

/* Target functions compiled with -finstrument-functions */
uint32_t profiler_test_target_add(uint32_t a, uint32_t b)
{
    return a + b;
}

uint32_t profiler_test_target_sub(uint32_t a, uint32_t b)
{
    return a - b;
}
