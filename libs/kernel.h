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

struct time_info {
    uint32_t seconds;
    uint32_t microseconds;
};

void cmos_read(uint8_t reg, uint8_t *value) {
    asm volatile("outb %%al, $0x70" : : "a" (reg));
    asm volatile("inb $0x71, %%al" : "=a" (*value));
}

struct time_info kernel_time() {
    struct time_info time_info;
    uint8_t seconds, minutes, hours, day, month, year;
    
    cmos_read(0x00, &seconds);
    cmos_read(0x02, &minutes);
    cmos_read(0x04, &hours);
    cmos_read(0x07, &day);
    cmos_read(0x08, &month);
    cmos_read(0x09, &year);

    uint64_t total_seconds = ((uint64_t)(year - 70) * 31536000) + 
                             ((uint64_t)(month - 1) * 2628000) + 
                             ((uint64_t)day * 86400) + 
                             ((uint64_t)hours * 3600) + 
                             ((uint64_t)minutes * 60) + 
                             (uint64_t)seconds;
    time_info.seconds = (uint32_t)total_seconds;

    uint16_t pit_counter;
    asm volatile("inw $0x40, %0" : "=a" (pit_counter));
    time_info.microseconds = (65536 - pit_counter) * 1000000 / 1193180; 

    return time_info;
}

// ---------------- Other -------------------------------

uint32_t rand_state = 1;

#define LCG_A 1664525
#define LCG_C 1013904223
#define LCG_M (1ULL << 32)

uint32_t kernel_rand() {
    rand_state = (LCG_A * rand_state + LCG_C) % LCG_M;
    return rand_state;
}

uint32_t kernel_rand_range(uint32_t min, uint32_t max) {
    if (min > max) {
        uint32_t temp = min;
        min = max;
        max = temp;
    }

    return min + (kernel_rand() % (max - min + 1));
}

void kernel_panic(char *msg) {
    kernel_print_string("Kernel Panic: ");
    kernel_print_string(msg);
    kernel_print_string("\n");
}

#endif
