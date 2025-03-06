#ifndef KERNEL_H
#define KERNEL_H 

#include "types.h"
#include "memory.h"

//---------------- Definitions ----------------------
#define STDOUT 0 
#define STDIN 1 
#define STDERR 2

// --------------- State ----------------------------
typedef struct {
    int disk;
    int timezone;
} KernelState;

KernelState state = {
    .disk = -1,
    .timezone = 0
};

// --------------- Externs --------------------------
extern void kernel_panic(const char *msg);
extern int getchar();
// --------------- Utils ----------------------------

void kernel_delay(int iterations) {
    volatile int i;
    for (i = 0; i < iterations; i++) {}
}

void kernel_timezone(int ntimezone) {
    state.timezone = ntimezone;
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
#define EOF (-1)

volatile unsigned short* vga_buffer = (unsigned short*)0xB8000;
static unsigned short output_buffer[VGA_WIDTH * VGA_HEIGHT];
int term_row = 0;
int term_col = 0;
unsigned char term_color = 0x07;

void kernel_flush_buffer(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = output_buffer[i];
    }
}

void kernel_clear_screen(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            output_buffer[y * VGA_WIDTH + x] = (unsigned short)(term_color << 8) | ' ';
        }
    }
    term_row = 0;
    term_col = 0;
    kernel_flush_buffer();
}

void kernel_scroll_up(void) {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            output_buffer[(y - 1) * VGA_WIDTH + x] = output_buffer[y * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        output_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (unsigned short)(term_color << 8) | ' ';
    }
    if (term_row > 0) term_row--;
    term_col = 0;
    kernel_flush_buffer();
}

void kernel_scroll_down(void) {
    for (int y = VGA_HEIGHT - 2; y >= 0; y--) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            output_buffer[(y + 1) * VGA_WIDTH + x] = output_buffer[y * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        output_buffer[x] = (unsigned short)(term_color << 8) | ' ';
    }
    if (term_row < VGA_HEIGHT - 1) term_row++;
    term_col = 0;
    kernel_flush_buffer();
}

void kernel_set_cursor_pos(int col, int row) {
    if (col < 0) col = 0;
    if (col >= VGA_WIDTH) col = VGA_WIDTH - 1;
    if (row < 0) row = 0;
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    term_row = row;
    term_col = col;
}

void kernel_print_string(const char *str) {
    while (*str) {
        if (*str == '\n') {
            term_col = 0;
            term_row++;
            if (term_row >= VGA_HEIGHT) {
                kernel_scroll_up();
            }
            str++;
            continue;
        }
        output_buffer[term_row * VGA_WIDTH + term_col] = (unsigned short)(term_color << 8) | *str;
        term_col++;
        if (term_col >= VGA_WIDTH) {
            term_col = 0;
            term_row++;
            if (term_row >= VGA_HEIGHT) {
                kernel_scroll_up();
            }
        }
        str++;
    }
    kernel_flush_buffer();
}

void kernel_write(int fd, const char *str, int count) {
    for (int i = 0; i < count && str[i]; i++) {
        if (fd == STDOUT || fd == STDERR) {
            if (str[i] == '\n') {
                term_col = 0;
                term_row++;
                if (term_row >= VGA_HEIGHT) {
                    kernel_scroll_up();
                }
                continue;
            }
            output_buffer[term_row * VGA_WIDTH + term_col] = (unsigned short)(term_color << 8) | str[i];
            term_col++;
            if (term_col >= VGA_WIDTH) {
                term_col = 0;
                term_row++;
                if (term_row >= VGA_HEIGHT) {
                    kernel_scroll_up();
                }
            }
        }
    }
    kernel_flush_buffer();
}

int kernel_read(int fd, char *buf, size_t count) {
    if (fd == STDIN) {
        size_t read_count = 0;
        for (size_t i = 0; i < count; i++) {
            int ch = getchar();
            if (ch == EOF) {
                if (read_count == 0) {
                    return -1;
                }
                break;
            }
            buf[i] = (char)ch;
            read_count++;
            if (ch == '\n') {
                break;
            }
        }
        return read_count;
    } else if (fd >= 3 && state.disk != -1) {
        return -1;
    }
    return -1;
}

void kernel_clean_latest_char(void) {
    if (term_col > 0) {
        term_col--;
    } else if (term_row > 0) {
        term_row--;
        term_col = VGA_WIDTH - 1;
    }
    output_buffer[term_row * VGA_WIDTH + term_col] = (unsigned short)(term_color << 8) | ' ';
    kernel_flush_buffer();
}

void kernel_clean_latest_line(void) {
    if (term_row > 0) {
        term_row--;
    } else {
        term_row = 0;
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        output_buffer[term_row * VGA_WIDTH + x] = (unsigned short)(term_color << 8) | ' ';
    }
    term_col = 0;
    kernel_flush_buffer();
}

void kernel_change_color(char *color) {
    unsigned char col = 0;
    if (color == 0) {
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

void kernel_reset_color(void) {
    term_color = 0x7;
}

const char spinner_chars[] = {'|', '/', '-', '\\'};
void kernel_display_spinner(int row, int col, int frame) {
    char spinner_char = spinner_chars[frame % 4];
    int saved_row = term_row;
    int saved_col = term_col;
    term_row = row;
    term_col = col;
    output_buffer[term_row * VGA_WIDTH + term_col] = (unsigned short)(term_color << 8) | spinner_char;
    term_row = saved_row;
    term_col = saved_col;
    kernel_flush_buffer();
}


//--------------------------------- Hardware ------------------------------------

static void cpuid(uint32_t code, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d) {
    asm volatile (
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(code)
    );
}

char *kernel_cpu_get_info() {
    char *vendor = malloc(256);
    uint32_t a, b, c, d;
    cpuid(0, &a, &b, &c, &d);
    *(uint32_t *)(vendor)     = b;
    *(uint32_t *)(vendor + 4) = d;
    *(uint32_t *)(vendor + 8) = c;
    vendor[12] = '\0';
    return vendor;
}


#define SMBIOS_SIGNATURE 0x5F736D62

typedef struct {
    uint8_t  anchor[4];
    uint8_t  checksum;
    uint8_t  length;
    uint8_t  major_version;
    uint8_t  minor_version;
    uint32_t table_length;
    uint32_t table_address;
    uint16_t structure_count;
} SMBIOSHeader;

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t handle;
} SMBIOSStructureHeader;

typedef struct {
    SMBIOSStructureHeader header;
    uint8_t vendor_string;
} BIOSInfoStructure;

SMBIOSHeader* kernel_bios_get_info() {
    uint32_t smbios_address = 0;

    asm volatile (
        "mov $0xF0000, %%ebx;"
        "mov $0x10000, %%ecx;"
        "find_smbios:"
        "cmpb $'_', (%%ebx);"
        "jne next_byte;"
        "cmpb $'S', 1(%%ebx);"
        "jne next_byte;"
        "cmpb $'M', 2(%%ebx);"
        "jne next_byte;"
        "cmpb $'B', 3(%%ebx);"
        "jne next_byte;"
        "mov %%ebx, %0;"
        "jmp done;"
        "next_byte: add $1, %%ebx;"
        "loop find_smbios;"
        "done:"
        : "=m"(smbios_address)
        : 
        : "%eax", "%ebx", "%ecx"
    );

    SMBIOSHeader* smbios_header = (SMBIOSHeader*)smbios_address;

    return smbios_header;
}

char* kernel_bios_get_vendor(SMBIOSHeader* smbios_header) {
    uint32_t table_address = smbios_header->table_address;
    SMBIOSStructureHeader* structure_header = (SMBIOSStructureHeader*)table_address;

    while (structure_header->type != 0) {
        if (structure_header->type == 0) {
            BIOSInfoStructure* bios_info = (BIOSInfoStructure*)structure_header;
            return (char*)&bios_info->vendor_string;
        }
        structure_header = (SMBIOSStructureHeader*)((uint8_t*)structure_header + structure_header->length);
    }

    return NULL;
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
    struct Day* dt = malloc(sizeof(struct Day));
    uint32_t days = timestamp / 86400;
    uint32_t remaining_seconds = timestamp % 86400;
    
    dt->hours = remaining_seconds / 3600 + state.timezone;
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

void kernel_wait(unsigned int seconds) {
    struct time_info start = kernel_time();
    while ((kernel_time().seconds - start.seconds) < seconds) {
        kernel_delay(10000);
    }
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
