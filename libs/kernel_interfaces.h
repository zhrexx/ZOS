#ifndef KERNEL_INTERFACES_H
#define KERNEL_INTERFACES_H

#include "kernel.h"
#include "interfaces.h"

void kernel_panic(const char *msg) {
    printf("KERNEL PANIC: %s\n", msg);
    asm volatile("cli; hlt");
    while (1);
}

#endif // KERNEL_INTERFACES_H
