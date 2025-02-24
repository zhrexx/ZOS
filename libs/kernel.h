#ifndef KERNEL_H
#define KERNEL_H 

#include "types.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
int term_row = 0;
int term_col = 0;
unsigned char term_color = 0x07; 

void kernel_clear_screen(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = (unsigned short)0x0720;
        }
    }
    term_row = 0;
    term_col = 0;
}

void kernel_print_string(const char* str) {
    while (*str) {
        if (*str == '\n') {
            term_col = 0;
            term_row++;
            str++;
            continue;
        }

        if (term_row >= VGA_HEIGHT) {
            kernel_clear_screen();
        }

        const size_t index = term_row * VGA_WIDTH + term_col;
        vga_buffer[index] = (unsigned short)(term_color << 8) | *str;

        term_col++;
        if (term_col >= VGA_WIDTH) {
            term_col = 0;
            term_row++;
        }
        str++;
    }
}

#endif
