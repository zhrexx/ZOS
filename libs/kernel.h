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
extern void *memmove(void *, const void *, size_t);
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

int term_row = 0;
int term_col = 0;
unsigned char term_color = 0x07;
unsigned char term_background = 0x00;

extern volatile uint32_t* vga_buffer;
extern int VGA_WIDTH;
extern int VGA_HEIGHT;
extern int VGA_PITCH;
#define EOF (-1)

static const unsigned char font[256][8] = {
    [0] = {0},                       // NULL
    [32] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // Space
    [48] = {0x3C,0x66,0x76,0x6E,0x66,0x66,0x3C,0x00}, // 0
    [49] = {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 1
    [50] = {0x3C,0x66,0x06,0x0C,0x30,0x60,0x7E,0x00}, // 2
    [51] = {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 3
    [52] = {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00}, // 4
    [53] = {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 5
    [54] = {0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 6
    [55] = {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00}, // 7
    [56] = {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 8
    [57] = {0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, // 9
    
    [65] = {0x18,0x3C,0x66,0x7E,0x66,0x66,0x66,0x00}, // A
    [66] = {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // B
    [67] = {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // C
    [68] = {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // D
    [69] = {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, // E
    [70] = {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, // F
    [71] = {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // G
    [72] = {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // H
    [73] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, // I
    [74] = {0x3E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // J
    [75] = {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // K
    [76] = {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // L
    [77] = {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // M
    [78] = {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // N
    [79] = {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // O
    [80] = {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // P
    [81] = {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x06,0x00}, // Q
    [82] = {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, // R
    [83] = {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // S
    [84] = {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // T
    [85] = {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // U
    [86] = {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // V
    [87] = {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // W
    [88] = {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // X
    [89] = {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // Y
    [90] = {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // Z
    [97] = {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // a
    [98] = {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // b
    [99] = {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // c
    [100] = {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // d
    [101] = {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // e
    [102] = {0x1C,0x36,0x30,0x7C,0x30,0x30,0x30,0x00}, // f
    [103] = {0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C,0x00}, // g
    [104] = {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // h
    [105] = {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // i
    [106] = {0x0C,0x00,0x1C,0x0C,0x0C,0x6C,0x38,0x00}, // j
    [107] = {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // k
    [108] = {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // l
    [109] = {0x00,0x00,0x76,0x7F,0x6B,0x6B,0x63,0x00}, // m
    [110] = {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // n
    [111] = {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // o
    [112] = {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // p
    [113] = {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x07}, // q
    [114] = {0x00,0x00,0x6C,0x76,0x60,0x60,0x60,0x00}, // r
    [115] = {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // s
    [116] = {0x30,0x30,0x7C,0x30,0x30,0x34,0x18,0x00}, // t
    [117] = {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // u
    [118] = {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // v
    [119] = {0x00,0x00,0x63,0x6B,0x6B,0x7F,0x36,0x00}, // w
    [120] = {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // x
    [121] = {0x00,0x00,0x66,0x66,0x3E,0x06,0x3C,0x00}, // y
    [122] = {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // z
    [33] = {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},  // !
    [34] = {0x36,0x36,0x36,0x00,0x00,0x00,0x00,0x00},  // "
    [35] = {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},  // #
    [36] = {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00},  // $
    [37] = {0x63,0x33,0x18,0x0C,0x66,0x63,0x00,0x00}, // %
    [38] = {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00},  // &
    [39] = {0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, // '
    [40] = {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00},  // (
    [41] = {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00},  // )
    [42] = {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00},  // *
    [43] = {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // +
    [44] = {0x00,0x00,0x00,0x00,0x18,0x18,0x30,0x00},  // ,
    [45] = {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // -
    [46] = {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},  // .
    [47] = {0x00,0x03,0x06,0x0C,0x18,0x30,0x60,0x00},  // /
};


static void draw_char(int col, int row, char c) {
    int x = col * 8;
    int y = row * 8;
    const unsigned char *glyph = font[(unsigned char)c];
    for (int dy = 0; dy < 8; dy++) {
        unsigned char row_bits = glyph[dy];
        for (int dx = 0; dx < 8; dx++) {
            int px = x + dx;
            int py = y + dy;
            if (px >= VGA_WIDTH || py >= VGA_HEIGHT) continue;
            uint32_t color = (row_bits & (0x80 >> dx)) ? 0xFFFFFFFF : 0x00000000;
            int row_stride = VGA_PITCH / sizeof(uint32_t);
            vga_buffer[py * row_stride + px] = color;
        }
    }
}

void kernel_scroll_up() {
    uint32_t *fb = (uint32_t*)vga_buffer;
    int row_stride = VGA_PITCH / sizeof(uint32_t);
    int pixel_rows_to_move = VGA_HEIGHT - 8; 
    
    memmove(fb, fb + 8 * row_stride, pixel_rows_to_move * row_stride * sizeof(uint32_t));
    
    for(int y = pixel_rows_to_move; y < VGA_HEIGHT; y++) {
        for(int x = 0; x < row_stride; x++) {
            fb[y * row_stride + x] = 0x00000000;
        }
    }
    term_row = (VGA_HEIGHT / 8) - 1;
}

void kernel_scroll_down() {
    uint32_t *fb = (uint32_t*)vga_buffer;
    int row_stride = VGA_PITCH / sizeof(uint32_t);
    int pixel_rows_to_move = VGA_HEIGHT - 8;
    memmove(fb + 8 * row_stride, fb, pixel_rows_to_move * row_stride * sizeof(uint32_t));
    for(int y = 0; y < 8; y++) {
        for(int x = 0; x < row_stride; x++) {
            fb[y * row_stride + x] = 0x00000000;
        }
    }
    term_row = 0;
}

void kernel_clear_screen() {
    int row_stride = VGA_PITCH / sizeof(uint32_t);
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < row_stride; x++) {
            vga_buffer[y * row_stride + x] = 0x00000000;
        }
    }
    term_row = 0;
    term_col = 0;
}

void kernel_print_string(const char *str) {
    while (*str) {
        if (*str == '\n') {
            term_col = 0;
            term_row++;
            if (term_row >= (VGA_HEIGHT / 8)) {
                kernel_scroll_up();
            }
            str++;
            continue;
        }
        
        draw_char(term_col, term_row, *str);
        
        if (++term_col >= (VGA_WIDTH / 8)) {
            term_col = 0;
            term_row++;
            if (term_row >= (VGA_HEIGHT / 8)) {
                kernel_scroll_up();
            }
        }
        str++;
    }
}

void kernel_write(int fd, const char *str, int count) {
    for (int i = 0; i < count && str[i]; i++) {
        if (fd == STDOUT || fd == STDERR) {
            if (str[i] == '\n') {
                term_col = 0;
                if (++term_row >= VGA_HEIGHT / 8) {
                    kernel_scroll_up();
                }
                continue;
            }
            
            draw_char(term_col, term_row, str[i]);
            
            if (++term_col >= VGA_WIDTH / 8) {
                term_col = 0;
                if (++term_row >= VGA_HEIGHT / 8) {
                    kernel_scroll_up();
                }
            }
        }
    }
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

void kernel_clean_latest_char() {
    if (term_col > 0) {
        term_col--;
    } else if (term_row > 0) {
        term_row--;
        term_col = (VGA_WIDTH / 8) - 1;
    }
    
    int x = term_col * 8;
    int y = term_row * 8;
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            vga_buffer[(y + dy) * VGA_WIDTH + (x + dx)] = 0x00000000;
        }
    }
}


void kernel_clean_latest_line() {
    if (term_row > 0) term_row--;
    
    int y_start = term_row * 8;
    for(int y = y_start; y < y_start + 8; y++) {
        for(int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = 0x00000000;
        }
    }
    
    term_col = 0;
}


void kernel_change_color(char *color) {
    if (!color) return;
    const struct { char *name; unsigned char id; } colors[] = {
        {"black", 0x00}, {"blue", 0x01}, {"green", 0x02},
        {"cyan", 0x03}, {"red", 0x04}, {"magenta", 0x05},
        {"brown", 0x06}, {"light_gray", 0x07}, {"dark_gray", 0x08},
        {"light_blue", 0x09}, {"light_green", 0x0A}, {"light_cyan", 0x0B},
        {"light_red", 0x0C}, {"light_magenta", 0x0D}, {"yellow", 0x0E},
        {"white", 0x0F}, {0, 0x07}
    };
    
    for (int i = 0; colors[i].name; i++) {
        if (kernel_strcmp(color, colors[i].name) == 0) {
            term_color = colors[i].id;
            return;
        }
    }
    term_color = 0x07;
}

void kernel_reset_color(void) {
    term_color = 0x7;
}


const char spinner_chars[] = {'|', '/', '-', '\\'};
void kernel_display_spinner(int row, int col, int frame) {
    char c = spinner_chars[frame % 4];
    int saved_row = term_row;
    int saved_col = term_col;
    
    term_row = row;
    term_col = col;
    
    int x = col * 8;
    int y = row * 8;
    for (int dy = 0; dy < 8; dy++) {
        for (int dx = 0; dx < 8; dx++) {
            vga_buffer[(y + dy) * VGA_WIDTH + (x + dx)] = 0x00000000;
        }
    }
    
    draw_char(col, row, c);
    
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

// --------------------------------------------------
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
} regs_t;

void kernel_syscall(int code, regs_t *regs) {
    switch (code) {
        case 1:
            kernel_shutdown();
            break;
        case 2:
            break;
        default:
            printf("ERROR: invalid syscall\n");
            break;
    }
}

void init_idt(void);
void init_pic(void);

struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256] = {0};
struct idt_ptr idtp = {sizeof(idt) - 1, (uint32_t)&idt};

void init_idt() {
    asm volatile("lidt %0" : : "m"(idtp));
}

void exception_handler() {
    kernel_panic("Unhandled exception!");
}

static inline void ___outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void init_pic() {
    ___outb(0x20, 0x11);
    ___outb(0xA0, 0x11);
    
    ___outb(0x21, 0x20);
    ___outb(0xA1, 0x28);
    
    ___outb(0x21, 0x04);
    ___outb(0xA1, 0x02);
    
    ___outb(0x21, 0x01);
    ___outb(0xA1, 0x01);
    
    ___outb(0x21, 0xFF);
    ___outb(0xA1, 0xFF);
}

#endif
