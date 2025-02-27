#ifndef MSSTD_H
#define MSSTD_H

#include "libs/types.h"
#include "libs/memory.h"
#include "libs/interfaces.h"
#include "libs/kernel.h"
#include "libs/stdarg.h"
#include "libs/stdio.h"
#include "libs/kernel_interfaces.h"

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    asm volatile ("lfence; rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint64_t calibrate_tsc(void) {
    uint64_t start = rdtsc();
    for (volatile uint32_t i = 0; i < 1000000; i++);  
    return (rdtsc() - start) / 1000;
}

void wait(uint64_t milliseconds) {
    static uint64_t cycles_per_ms = 0;
    if (cycles_per_ms == 0) {
        cycles_per_ms = calibrate_tsc();
    }
    uint64_t start = rdtsc();
    while ((rdtsc() - start) < milliseconds * cycles_per_ms);
}


#endif // MSSTD_H
