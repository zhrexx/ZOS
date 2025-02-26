#ifndef INTERFACES_H
#define INTERFACES_H

#include "types.h"
#include "memory.h"
#include "kernel.h"
#include "stdarg.h"


uint64_t __udivdi3(uint64_t a, uint64_t b) {
    uint64_t quotient = 0;
    while (a >= b) {
        uint64_t temp = b;
        uint64_t i = 1;
        while ((temp << 1) <= a) {
            temp <<= 1;
            i <<= 1;
        }
        a -= temp;
        quotient += i;
    }
    return quotient;
}

int64_t __divdi3(int64_t a, int64_t b) {
    if (b == 0) {
        kernel_panic("zero division");
        return 0;
    }

    int64_t quotient = 0;
    int sign = (a < 0) ^ (b < 0);
    uint64_t a_abs = (a < 0)? -a : a;
    uint64_t b_abs = (b < 0)? -b : b;

    while (a_abs >= b_abs) {
        uint64_t temp = b_abs;
        uint64_t i = 1;
        while ((temp << 1) <= a_abs) {
            temp <<= 1;
            i <<= 1;
        }
        a_abs -= temp;
        quotient += i;
    }

    return sign? -quotient : quotient;
}

uint64_t __moddi3(int64_t a, int64_t b) {
    if (b == 0) {
        kernel_panic("zero division");
        return 0;
    }

    int64_t remainder = a % b;
    if (remainder < 0) {
        remainder += b;
    }
    return (uint64_t)remainder;
}


//----------------------

int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    while (*str == ' ') {
        str++;
    }
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    } 
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result * sign;
}


void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}


int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

size_t strlen(const char *s) {
    size_t c = 0;
    while (s[c] != '\0') {
        c++;
    }
    return c;
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return (unsigned char)*str1 - (unsigned char)*str2;
}

int strncmp(const char *str1, const char *str2, size_t n) {
    while (n-- && *str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    if (n == (size_t)-1) return 0;
    return (unsigned char)*str1 - (unsigned char)*str2;
}

char *strdup(const char *str) {
    size_t len = strlen(str) + 1;
    char *copy = aarena_alloc(&arena, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

char *strchr(const char *str, int c) {
    while (*str) {
        if (*str == c) {
            return (char *)str;
        }
        str++;
    }
    return NULL;
}

char *strtok(char *str, const char *delim) {
    static char *save_ptr;
    if (str) save_ptr = str; 
    if (!save_ptr) return NULL;

    char *start = save_ptr;
    while (*save_ptr && strchr(delim, *save_ptr)) save_ptr++;

    if (!*save_ptr) return NULL; 

    char *end = save_ptr;
    while (*end && !strchr(delim, *end)) end++;

    if (*end) *end++ = '\0';
    save_ptr = end;
    return start;
}


char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        const char *h = haystack, *n = needle;
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        if (!*n) return (char *)haystack;
    }
    return NULL;
}

static void int_to_str(int num, char* buf) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    if (i > 1) {
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
    }
}

static void long_long_to_str(long long num, char* buf) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    } else if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    if (i > 1) {
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
    }
}

static void unsigned_long_to_str(unsigned long num, char* buf) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    if (i > 0) {
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
    }
}

static void unsigned_int_to_str(unsigned int num, char* buf) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
    }
    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    if (i > 0) {
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
    }
}

static void float_to_str(float num, char* buf) {
    int i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }
    int integer_part = (int)num;
    int_to_str(integer_part, buf);
    i = strlen(buf);
    buf[i++] = '.';
    float fractional_part = num - integer_part;
    for (int j = 0; j < 2; j++) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        buf[i++] = digit + '0';
        fractional_part -= digit;
    }
    buf[i] = '\0';
}

static void pointer_to_str(void* ptr, char* buf) {
    unsigned long long address = (unsigned long long)ptr;
    int i = 0;
    buf[i++] = '0';
    buf[i++] = 'x';
    while (address > 0) {
        unsigned char byte = address & 0xf;
        if (byte < 10) {
            buf[i++] = byte + '0';
        } else {
            buf[i++] = byte - 10 + 'a';
        }
        address >>= 4;
    }
    for (int j = 2; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = temp;
    }
}

char* str_format(const char* fmt, va_list args) {
    int fmt_index = 0;
    size_t total_length = 0;

    va_list args_copy;
    va_copy(args_copy, args);

    while (fmt[fmt_index]!= '\0') {
        if (fmt[fmt_index] == '%' && fmt[fmt_index + 1]!= '\0') {
            switch (fmt[fmt_index + 1]) {
                case's': {
                    char* arg = va_arg(args_copy, char*);
                    total_length += strlen(arg);
                    break;
                }
                case 'd': {
                    char temp[20];
                    int_to_str(va_arg(args_copy, int), temp);
                    total_length += strlen(temp);
                    break;
                }
                case 'f': {
                    char temp[32];
                    float_to_str((float)va_arg(args_copy, double), temp);
                    total_length += strlen(temp);
                    break;
                }
                case 'c': {
                    total_length += 1;
                    break;
                }
                case 'p': {
                    char temp[20];
                    pointer_to_str(va_arg(args_copy, void*), temp);
                    total_length += strlen(temp);
                    break;
                }
                case 'l': {
                    if (fmt[fmt_index + 2] == 'l') {
                        char temp[20];
                        long_long_to_str(va_arg(args_copy, long long), temp);
                        total_length += strlen(temp);
                        fmt_index += 2;
                    } else {
                        if (fmt[fmt_index + 2] == 'd') {
                            char temp[20];
                            long_long_to_str(va_arg(args_copy, long long), temp);
                            total_length += strlen(temp);
                        } else if (fmt[fmt_index + 2] == 'u') {
                            char temp[20];
                            unsigned_long_to_str(va_arg(args_copy, unsigned long), temp);
                            total_length += strlen(temp);
                        }
                        fmt_index += 2;
                    }
                    break;
                }
                case 'u': {
                    char temp[20];
                    unsigned_int_to_str(va_arg(args_copy, unsigned int), temp);
                    total_length += strlen(temp);
                    break;
                }
                default:
                    total_length += 2;
                    break;
            }
            fmt_index += 2;
        } else {
            total_length++;
            fmt_index++;
        }
    }

    va_end(args_copy);

    char* buffer = (char*)aarena_alloc(&arena, total_length + 1);
    if (buffer == NULL) {
        return NULL;
    }

    fmt_index = 0;
    int buffer_index = 0;
    while (fmt[fmt_index]!= '\0') {
        if (fmt[fmt_index] == '%' && fmt[fmt_index + 1]!= '\0') {
            switch (fmt[fmt_index + 1]) {
                case's': {
                    char* arg = va_arg(args, char*);
                    while (*arg!= '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                case 'd': {
                    char temp[20];
                    int_to_str(va_arg(args, int), temp);
                    char* arg = temp;
                    while (*arg!= '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                case 'f': {
                    char temp[32];
                    float_to_str((float)va_arg(args, double), temp);
                    char* arg = temp;
                    while (*arg!= '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                case 'c': {
                    buffer[buffer_index++] = va_arg(args, int);
                    break;
                }
                case 'p': {
                    char temp[20];
                    pointer_to_str(va_arg(args, void*), temp);
                    char* arg = temp;
                    while (*arg!= '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                case 'l': {
                    if (fmt[fmt_index + 2] == 'l') {
                        char temp[20];
                        long_long_to_str(va_arg(args, long long), temp);
                        char* arg = temp;
                        while (*arg!= '\0') {
                            buffer[buffer_index++] = *arg++;
                        }
                        fmt_index += 2;
                    } else {
                        if (fmt[fmt_index + 2] == 'd') {
                            char temp[20];
                            long_long_to_str(va_arg(args, long long), temp);
                            char* arg = temp;
                            while (*arg!= '\0') {
                                buffer[buffer_index++] = *arg++;
                            }
                        } else if (fmt[fmt_index + 2] == 'u') {
                            char temp[20];
                            unsigned_long_to_str(va_arg(args, unsigned long), temp);
                            char* arg = temp;
                            while (*arg!= '\0') {
                                buffer[buffer_index++] = *arg++;
                            }
                        }
                        fmt_index += 2;
                    }
                    break;
                }
                case 'u': {
                    char temp[20];
                    unsigned_int_to_str(va_arg(args, unsigned int), temp);
                    char* arg = temp;
                    while (*arg!= '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                default:
                    buffer[buffer_index++] = fmt[fmt_index++];
                    buffer[buffer_index++] = fmt[fmt_index];
                    break;
            }
            fmt_index += 2;
        } else {
            buffer[buffer_index++] = fmt[fmt_index++];
        }
    }
    buffer[buffer_index] = '\0';

    return buffer;
}

void printf(const char *s, ...) {
    va_list args;
    va_start(args, s);
    char *fmt = str_format(s, args);
    va_end(args);
    
    if (fmt) {
        kernel_print_string(fmt);
    }
}

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

// TODO: Somehow use str_format
char *time_format(struct Day* day) {
    char* buffer = aarena_alloc(&arena, 32);
    uint8_t pos = 0;
    uint16_t y = day->year;
    buffer[pos++] = '0' + (y / 1000);
    buffer[pos++] = '0' + ((y / 100) % 10);
    buffer[pos++] = '0' + ((y / 10) % 10);
    buffer[pos++] = '0' + (y % 10);
    buffer[pos++] = '-';
    buffer[pos++] = '0' + (day->month / 10);
    buffer[pos++] = '0' + (day->month % 10);
    buffer[pos++] = '-';
    buffer[pos++] = '0' + (day->day / 10);
    buffer[pos++] = '0' + (day->day % 10);
    buffer[pos++] = ' ';
    buffer[pos++] = '0' + (day->hours / 10);
    buffer[pos++] = '0' + (day->hours % 10);
    buffer[pos++] = ':';
    buffer[pos++] = '0' + (day->minutes / 10);
    buffer[pos++] = '0' + (day->minutes % 10);
    buffer[pos++] = ':';
    buffer[pos++] = '0' + (day->seconds / 10);
    buffer[pos++] = '0' + (day->seconds % 10);
    buffer[pos] = '\0';
    
    return buffer;
}

char *time_now() {
    struct Day now = kernel_localtime(kernel_time().seconds);
    return time_format(&now);
}
#endif
