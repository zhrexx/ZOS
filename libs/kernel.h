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

void kernel_clean_latest_char(void) {
    if (term_col > 0) {
        term_col--;
    } else if (term_row > 0) {
        term_row--;
        term_col = VGA_WIDTH - 1;
    }

    const size_t index = term_row * VGA_WIDTH + term_col;
    vga_buffer[index] = (unsigned short)0x0720; 

    if (term_col == 0 && term_row == 0) {
        return;
    }
}

static int kernel_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

void kernel_change_color(char *color) {
    unsigned char col = 0;
    
    if (color == NULL) {
        return;
    }
    
    if (kernel_strcmp(color, "black") == 0) {
        col = 0x0;
    } else if (kernel_strcmp(color, "blue") == 0) {
        col = 0x1;
    } else if (kernel_strcmp(color, "green") == 0) {
        col = 0x2;
    } else if (kernel_strcmp(color, "cyan") == 0) {
        col = 0x3;
    } else if (kernel_strcmp(color, "red") == 0) {
        col = 0x4;
    } else if (kernel_strcmp(color, "magenta") == 0) {
        col = 0x5;
    } else if (kernel_strcmp(color, "brown") == 0) {
        col = 0x6;
    } else if (kernel_strcmp(color, "light_gray") == 0) {
        col = 0x7;
    } else if (kernel_strcmp(color, "dark_gray") == 0) {
        col = 0x8;
    } else if (kernel_strcmp(color, "light_blue") == 0) {
        col = 0x9;
    } else if (kernel_strcmp(color, "light_green") == 0) {
        col = 0xA;
    } else if (kernel_strcmp(color, "light_cyan") == 0) {
        col = 0xB;
    } else if (kernel_strcmp(color, "light_red") == 0) {
        col = 0xC;
    } else if (kernel_strcmp(color, "light_magenta") == 0) {
        col = 0xD;
    } else if (kernel_strcmp(color, "yellow") == 0) {
        col = 0xE;
    } else if (kernel_strcmp(color, "white") == 0) {
        col = 0xF;
    } else {
        col = 0x7;
    }
    
    term_color = col;
}

void kernel_reset_color() {
    term_color = 0x7;
}

const char spinner_chars[] = {'|', '/', '-', '\\'};

void kernel_display_spinner(int row, int col, int frame) {
    char spinner_char = spinner_chars[frame % 4];
    
    int saved_row = term_row;
    int saved_col = term_col;
    
    term_row = row;
    term_col = col;
    
    const size_t index = term_row * VGA_WIDTH + term_col;
    vga_buffer[index] = (unsigned short)(term_color << 8) | spinner_char;
    
    term_row = saved_row;
    term_col = saved_col;
}

void kernel_delay(int iterations) {
    volatile int i;
    for (i = 0; i < iterations; i++) {}
}


// --------------------------------- TIME ------------------------------------------

struct Time {
    unsigned char hours;
    unsigned char minutes;
    unsigned char seconds;
    unsigned char hundredths;
};

struct Time kernel_get_time() {
    struct Time t;

    __asm__ (
        "movb $0x00, %%ah;"
        "int $0x1A;"
        "movb %%ch, %0;"
        "movb %%cl, %1;"
        "movb %%dh, %2;"
        "movb %%dl, %3;"
        : "=m" (t.hours), "=m" (t.minutes), "=m" (t.seconds), "=m" (t.hundredths)
        :
        : "ah", "ch", "cl", "dh", "dl"
    );

    return t;
}

void kernel_exit() {
    __asm__ (
        "movb $0x53, %%ah;"
        "int $0x15;"
        "jne 1f;"

        "1: movl $0x2000, %%eax;"
        "outb %%al, $0xB2;"
        "jne 2f;"

        "2: hlt;"
        :
        :
        : "%eax", "%ah"
    );
}

#endif
