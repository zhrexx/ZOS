#include "msstd.h"

void kernel_main(unsigned int magic, unsigned int* mboot_info) {
    (void) magic, (void) mboot_info;
    kernel_clear_screen();
    printf("Welcome to ZOS\n");
    while(1) __asm__("hlt");
}
