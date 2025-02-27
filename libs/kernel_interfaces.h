#ifndef KERNEL_INTERFACES_H
#define KERNEL_INTERFACES_H

#include "kernel.h"
#include "interfaces.h"

void kernel_panic(const char *msg) {
    printf("Kernel Paniced with message: %s\n", msg);
}


#endif // KERNEL_INTERFACES_H
