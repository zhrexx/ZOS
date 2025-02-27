#ifndef KERNEL_H
#define KERNEL_H 

#include "types.h"
#include "memory.h"

// --------------- State ----------------------------
typedef struct {
    // TODO: Do something
} KernelState;

// --------------- Externs --------------------------
extern void kernel_panic(const char *msg);

// --------------- Utils ----------------------------

void kernel_delay(int iterations) {
    volatile int i;
    for (i = 0; i < iterations; i++) {}
}

static int kernel_strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}


#define VGA_WIDTH 80
#define VGA_HEIGHT 40

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

void kernel_clean_latest_line(void) {
    if (term_row > 0) {
        term_row--;
    } else {
        term_row = 0;
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        const size_t index = term_row * VGA_WIDTH + x;
        vga_buffer[index] = (unsigned short)0x0720; 
    }
    term_col = 0;
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



//--------------------------------- Hardware ------------------------------------

static void cpuid(uint32_t code, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    asm volatile (
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(code)
    );
}

void kernel_cpu_get_info(char *vendor) {
    uint32_t a, b, c, d;
    cpuid(0, &a, &b, &c, &d);
    *(uint32_t *)(vendor)     = b;
    *(uint32_t *)(vendor + 4) = d;
    *(uint32_t *)(vendor + 8) = c;
    vendor[12] = '\0';
}

// --------------------------------- TIME ------------------------------------------

struct time_info {
    uint32_t seconds;
    uint32_t microseconds;
};

static uint8_t bcd_to_bin(uint8_t value) {
    return (value & 0x0F) + ((value >> 4) * 10);
}

void cmos_read(uint8_t reg, uint8_t *value) {
    asm volatile ("cli");
    asm volatile ("outb %%al, $0x70" : : "a" (reg));
    asm volatile ("inb $0x71, %%al" : "=a" (*value));
    asm volatile ("sti");
}

struct time_info kernel_time() {
    struct time_info time_info;
    uint8_t seconds, minutes, hours, day, month, year, status_b;
    
    cmos_read(0x00, &seconds);
    cmos_read(0x02, &minutes);
    cmos_read(0x04, &hours);
    cmos_read(0x07, &day);
    cmos_read(0x08, &month);
    cmos_read(0x09, &year);
    cmos_read(0x0B, &status_b);
    
    if (!(status_b & 0x04)) {
        seconds = bcd_to_bin(seconds);
        minutes = bcd_to_bin(minutes);
        hours = bcd_to_bin(hours);
        day = bcd_to_bin(day);
        month = bcd_to_bin(month);
        year = bcd_to_bin(year);
    }
    uint16_t full_year = 2000 + year;
    uint32_t total_seconds = 0;
    uint32_t days_since_epoch = 0;
    for (uint16_t y = 1970; y < full_year; y++) {
        days_since_epoch += 365 + (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0) ? 1 : 0);
    }
    
    static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    for (uint8_t m = 0; m < month - 1; m++) {
        days_since_epoch += days_in_month[m];
        if (m == 1 && (full_year % 4 == 0 && (full_year % 100 != 0 || full_year % 400 == 0))) {
            days_since_epoch += 1;
        }
    }
    
    days_since_epoch += day - 1;
    
    total_seconds = days_since_epoch * 86400 + hours * 3600 + minutes * 60 + seconds;
    
    time_info.seconds = total_seconds;
    
    uint16_t pit_counter;
    asm volatile ("inb $0x61, %%al" : : ); 
    asm volatile ("inb $0x42, %%al" : "=a" (pit_counter) : );
    uint8_t low = (uint8_t)pit_counter;
    asm volatile ("inb $0x42, %%al" : "=a" (pit_counter) : );
    uint8_t high = (uint8_t)pit_counter;
    pit_counter = (high << 8) | low;
    
    time_info.microseconds = ((65536 - pit_counter) * 1000000) / 1193182;
    
    return time_info;
}

struct Day {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

static int is_leap_year(uint16_t year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

struct Day kernel_localtime(uint32_t timestamp) {
    struct Day* dt = aarena_alloc(&arena, sizeof(struct Day));
    uint32_t days = timestamp / 86400;
    uint32_t remaining_seconds = timestamp % 86400;
    
    dt->hours = remaining_seconds / 3600;
    remaining_seconds %= 3600;
    dt->minutes = remaining_seconds / 60;
    dt->seconds = remaining_seconds % 60;
    
    uint16_t year = 1970;
    while (days >= ((uint32_t)(365 + is_leap_year(year)))) {
        days -= 365 + is_leap_year(year);
        year++;
    }
    dt->year = year;
    
    static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    uint8_t month = 0;
    
    while (month < 12) {
        uint8_t mdays = days_in_month[month];
        if (month == 1 && is_leap_year(year)) {
            mdays++;
        }
        
        if (days < mdays) {
            break;
        }
        
        days -= mdays;
        month++;
    }
    
    dt->month = month + 1;
    dt->day = days + 1;
    
    struct Day result = *dt;
    return result;
}


// ---------------- Random -------------------------------

uint32_t rand_state = 1;

#define LCG_A 1664525
#define LCG_C 1013904223
#define LCG_M (1ULL << 32)

uint32_t kernel_rand() {
    uint32_t t = kernel_time().seconds;
    rand_state = (LCG_A * rand_state + LCG_C + (t%2)/2) % LCG_M;
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

// ------------------- Critical / System ------------------------------

void kernel_shutdown(void) {
    __asm__ volatile (
        "mov $0x2000, %%ax\n\t"
        "mov $0x604, %%dx\n\t"
        "out %%ax, %%dx\n\t"
        :
        :
        : "ax", "dx"
    );
    __asm__ volatile (
        "mov $0x5307, %%ax\n\t"
        "mov $0xb2, %%dx\n\t"
        "out %%ax, %%dx\n\t"
        :
        :
        : "ax", "dx"
    );
    __asm__ volatile (
        "1:\n\t"
        "in $0x64, %%al\n\t"
        "test $0x02, %%al\n\t"
        "jnz 1b\n\t"
        "mov $0xd1, %%al\n\t"
        "out %%al, $0x64\n\t"
        "2:\n\t"
        "in $0x64, %%al\n\t"
        "test $0x02, %%al\n\t"
        "jnz 2b\n\t"
        "mov $0xfe, %%al\n\t"
        "out %%al, $0x60\n\t"
        :
        :
        : "al"
    );

    __asm__ volatile (
        "cli\n\t"
        "hlt\n\t"
        :
        :
        :
    );

    while (1) {
        __asm__ volatile ("hlt");
    }
}

#endif
