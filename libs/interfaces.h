#ifndef INTERFACES_H
#define INTERFACES_H

#include "types.h"
#include "memory.h"
#include "kernel.h"
#include "stdarg.h"

typedef struct {
    char **lines;
    int lines_count;
} OutputBuffer;

OutputBuffer obuffer = {0};

//------------------------

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

size_t strcspn(const char *s, const char *reject) {
    const char *p, *r;
    
    for (p = s; *p; p++) {
        for (r = reject; *r; r++) {
            if (*p == *r) {
                return p - s;
            }
        }
    }
    return p - s;
}

int memcmp(const void *s1, const void *s2, unsigned int n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;

    for (unsigned int i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0; 
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

char *strcat(char *dest, const char *src) {
    char *dest_end = dest;
    while (*dest_end) {
        dest_end++;
    }
    while (*src) {
        *dest_end++ = *src++;
    }
    *dest_end = '\0';
    return dest;
}
char *strrchr(const char *s, int c) {
    char *last = NULL;
    while (*s) {
        if (*s == (char)c) {
            last = (char *)s;
        }
        s++;
    }
    if ((char)c == '\0') {
        return (char *)s;
    }
    return last;
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

char *strcpy(char *dest, const char *src) {
    char *original_dest = dest;

    while (*src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = '\0';
    return original_dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *original_dest = dest;
    while (n-- > 0 && *src != '\0') {
        *dest = *src;
        dest++;
        src++;
    }
    while (n-- > 0) {
        *dest = '\0';
        dest++;
    }
    return original_dest;
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
    buf[i] = '\0';
    if (i > 1 && buf[0] == '-') {
        for (int j = 1; j < i / 2 + 1; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j];
            buf[i - j] = temp;
        }
    } else if (i > 1) {
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
    buf[i] = '\0';
    if (i > 1 && buf[0] == '-') {
        for (int j = 1; j < i / 2 + 1; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j];
            buf[i - j] = temp;
        }
    } else if (i > 1) {
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
    buf[i] = '\0';
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
    buf[i] = '\0';
    if (i > 0) {
        for (int j = 0; j < i / 2; j++) {
            char temp = buf[j];
            buf[j] = buf[i - j - 1];
            buf[i - j - 1] = temp;
        }
    }
}

static void float_to_str(float num, char* buf, int precision) {
    int i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }
    int integer_part = (int)num;
    int_to_str(integer_part, buf + i);
    i = strlen(buf);
    buf[i++] = '.';
    float fractional_part = num - integer_part;
    
    if (precision <= 0) precision = 2; 
    
    for (int j = 0; j < precision; j++) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        buf[i++] = digit + '0';
        fractional_part -= digit;
    }
    buf[i] = '\0';
}

static void double_to_str(double num, char* buf, int precision) {
    int i = 0;
    if (num < 0) {
        buf[i++] = '-';
        num = -num;
    }
    
    unsigned long long integer_part = (unsigned long long)num;
    unsigned_long_to_str(integer_part, buf + i);
    i = strlen(buf);
    buf[i++] = '.';
    double fractional_part = num - integer_part;
    
    if (precision <= 0) precision = 6;  
    
    for (int j = 0; j < precision; j++) {
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
    
    if (address == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return;
    }
    
    int start_pos = i;
    while (address > 0) {
        unsigned char byte = address & 0xf;
        if (byte < 10) {
            buf[i++] = byte + '0';
        } else {
            buf[i++] = byte - 10 + 'a';
        }
        address >>= 4;
    }
    buf[i] = '\0';
    
    int end_pos = i - 1;
    while (start_pos < end_pos) {
        char temp = buf[start_pos];
        buf[start_pos] = buf[end_pos];
        buf[end_pos] = temp;
        start_pos++;
        end_pos--;
    }
}

static void int_to_hex_str(unsigned int num, char* buf, int uppercase) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return;
    }
    
    while (num > 0) {
        unsigned char digit = num & 0xf;
        if (digit < 10) {
            buf[i++] = digit + '0';
        } else {
            buf[i++] = digit - 10 + (uppercase ? 'A' : 'a');
        }
        num >>= 4;
    }
    buf[i] = '\0';
    
    for (int j = 0; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = temp;
    }
}

static void int_to_oct_str(unsigned int num, char* buf) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return;
    }
    
    while (num > 0) {
        buf[i++] = (num & 7) + '0';
        num >>= 3;
    }
    buf[i] = '\0';
    
    for (int j = 0; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = temp;
    }
}

static void int_to_bin_str(unsigned int num, char* buf) {
    int i = 0;
    if (num == 0) {
        buf[i++] = '0';
        buf[i] = '\0';
        return;
    }
    
    while (num > 0) {
        buf[i++] = (num & 1) + '0';
        num >>= 1;
    }
    buf[i] = '\0';
    
    for (int j = 0; j < i / 2; j++) {
        char temp = buf[j];
        buf[j] = buf[i - j - 1];
        buf[i - j - 1] = temp;
    }
}

static int parse_int_from_fmt(const char* fmt, int* i) {
    int value = 0;
    while (fmt[*i] >= '0' && fmt[*i] <= '9') {
        value = value * 10 + (fmt[*i] - '0');
        (*i)++;
    }
    return value;
}

static void apply_padding(char* dest, const char* src, int width, int left_justify, char pad_char) {
    int len = strlen(src);
    if (width <= len) {
        strcpy(dest, src);
        return;
    }
    
    if (left_justify) {
        strcpy(dest, src);
        for (int i = len; i < width; i++) {
            dest[i] = pad_char;
        }
        dest[width] = '\0';
    } else {
        int pad_count = width - len;
        for (int i = 0; i < pad_count; i++) {
            dest[i] = pad_char;
        }
        strcpy(dest + pad_count, src);
    }
}

char* str_vformat(const char* fmt, va_list args) {
    int fmt_index = 0;
    size_t total_length = 0;

    va_list args_copy;
    va_copy(args_copy, args);

    while (fmt[fmt_index] != '\0') {
        if (fmt[fmt_index] == '%' && fmt[fmt_index + 1] != '\0') {
            fmt_index++;
            
            int left_justify = 0;
            char pad_char = ' ';
            
            if (fmt[fmt_index] == '-') {
                left_justify = 1;
                fmt_index++;
            }
            
            if (fmt[fmt_index] == '0' && !left_justify) {
                pad_char = '0';
                fmt_index++;
            }
            
            int width = 0;
            if (fmt[fmt_index] >= '0' && fmt[fmt_index] <= '9') {
                width = parse_int_from_fmt(fmt, &fmt_index);
            } else if (fmt[fmt_index] == '*') {
                width = va_arg(args_copy, int);
                if (width < 0) {
                    width = -width;
                    left_justify = 1;
                }
                fmt_index++;
            }
            
            int precision = -1;
            if (fmt[fmt_index] == '.') {
                fmt_index++;
                if (fmt[fmt_index] == '*') {
                    precision = va_arg(args_copy, int);
                    fmt_index++;
                } else {
                    precision = parse_int_from_fmt(fmt, &fmt_index);
                }
            }
            
            // Length modifiers
            int length_mod = 0;  // 0 = none, 1 = h, 2 = l, 3 = ll, 4 = L
            if (fmt[fmt_index] == 'h') {
                length_mod = 1;
                fmt_index++;
                if (fmt[fmt_index] == 'h') {
                    fmt_index++;
                }
            } else if (fmt[fmt_index] == 'l') {
                length_mod = 2;
                fmt_index++;
                if (fmt[fmt_index] == 'l') {
                    length_mod = 3;  // long long
                    fmt_index++;
                }
            } else if (fmt[fmt_index] == 'L') {
                length_mod = 4;  // long double
                fmt_index++;
            }
            
            switch (fmt[fmt_index]) {
                case's': {
                    char* arg = va_arg(args_copy, char*);
                    if (arg == NULL) arg = "(null)";
                    int len = strlen(arg);
                    if (precision >= 0 && precision < len) {
                        len = precision;
                    }
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'd':
                case 'i': {
                    char temp[32];
                    long long value;
                    
                    if (length_mod == 3) {         // long long
                        value = va_arg(args_copy, long long);
                        long_long_to_str(value, temp);
                    } else if (length_mod == 2) {  // long
                        value = va_arg(args_copy, long);
                        long_long_to_str(value, temp);
                    } else {                        // int or default
                        value = va_arg(args_copy, int);
                        int_to_str(value, temp);
                    }
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'f': {
                    char temp[64];
                    if (length_mod == 4) {  // long double
                        double_to_str(va_arg(args_copy, long double), temp, precision);
                    } else {
                        float_to_str((float)va_arg(args_copy, double), temp, precision);
                    }
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'g':
                case 'e': {
                    char temp[64];
                    float_to_str((float)va_arg(args_copy, double), temp, precision);
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'c': {
                    va_arg(args_copy, int); // consume
                    total_length += (width > 1) ? width : 1;
                    break;
                }
                case 'p': {
                    char temp[32];
                    pointer_to_str(va_arg(args_copy, void*), temp);
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'x':
                case 'X': {
                    char temp[32];
                    int uppercase = (fmt[fmt_index] == 'X');
                    
                    if (length_mod == 2) {  // long
                        unsigned long value = va_arg(args_copy, unsigned long);
                        int_to_hex_str((unsigned int)value, temp, uppercase);
                    } else {
                        int_to_hex_str(va_arg(args_copy, unsigned int), temp, uppercase);
                    }
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'o': {
                    char temp[32];
                    int_to_oct_str(va_arg(args_copy, unsigned int), temp);
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'b': {  // Custom format: binary
                    char temp[64];
                    int_to_bin_str(va_arg(args_copy, unsigned int), temp);
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case 'u': {
                    char temp[32];
                    if (length_mod == 3) {  // unsigned long long
                        unsigned long value = va_arg(args_copy, unsigned long long);
                        unsigned_long_to_str(value, temp);
                    } else if (length_mod == 2) {  // unsigned long
                        unsigned_long_to_str(va_arg(args_copy, unsigned long), temp);
                    } else {
                        unsigned_int_to_str(va_arg(args_copy, unsigned int), temp);
                    }
                    
                    int len = strlen(temp);
                    total_length += (width > len) ? width : len;
                    break;
                }
                case '%': {
                    total_length += 1;  // Just a % character
                    break;
                }
                default:
                    total_length += 2;  // Unknown format - print as is
                    break;
            }
            fmt_index++;
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
    while (fmt[fmt_index] != '\0') {
        if (fmt[fmt_index] == '%' && fmt[fmt_index + 1] != '\0') {
            fmt_index++;
            
            int left_justify = 0;
            char pad_char = ' ';
            
            if (fmt[fmt_index] == '-') {
                left_justify = 1;
                fmt_index++;
            }
            
            if (fmt[fmt_index] == '0' && !left_justify) {
                pad_char = '0';
                fmt_index++;
            }
            
            int width = 0;
            if (fmt[fmt_index] >= '0' && fmt[fmt_index] <= '9') {
                width = parse_int_from_fmt(fmt, &fmt_index);
            } else if (fmt[fmt_index] == '*') {
                width = va_arg(args, int);
                if (width < 0) {
                    width = -width;
                    left_justify = 1;
                }
                fmt_index++;
            }
            
            int precision = -1;
            if (fmt[fmt_index] == '.') {
                fmt_index++;
                if (fmt[fmt_index] == '*') {
                    precision = va_arg(args, int);
                    fmt_index++;
                } else {
                    precision = parse_int_from_fmt(fmt, &fmt_index);
                }
            }
            
            // Length modifiers
            int length_mod = 0;  // 0 = none, 1 = h, 2 = l, 3 = ll, 4 = L
            if (fmt[fmt_index] == 'h') {
                length_mod = 1;
                fmt_index++;
                if (fmt[fmt_index] == 'h') {
                    fmt_index++;
                }
            } else if (fmt[fmt_index] == 'l') {
                length_mod = 2;
                fmt_index++;
                if (fmt[fmt_index] == 'l') {
                    length_mod = 3;  // long long
                    fmt_index++;
                }
            } else if (fmt[fmt_index] == 'L') {
                length_mod = 4;  // long double
                fmt_index++;
            }
            
            char temp[128] = {0};
            char padded[128] = {0};
            
            switch (fmt[fmt_index]) {
                case's': {
                    char* arg = va_arg(args, char*);
                    if (arg == NULL) arg = "(null)";
                    
                    if (precision >= 0) {
                        strncpy(temp, arg, precision);
                        temp[precision] = '\0';
                    } else {
                        strcpy(temp, arg);
                    }
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'd':
                case 'i': {
                    long long value;
                    
                    if (length_mod == 3) {         // long long
                        value = va_arg(args, long long);
                        long_long_to_str(value, temp);
                    } else if (length_mod == 2) {  // long
                        value = va_arg(args, long);
                        long_long_to_str(value, temp);
                    } else {                        // int or default
                        value = va_arg(args, int);
                        int_to_str(value, temp);
                    }
                    
                    if (width > 0) {
                        if (pad_char == '0' && temp[0] == '-') {
                            buffer[buffer_index++] = '-';
                            apply_padding(padded, temp + 1, width - 1, left_justify, pad_char);
                        } else {
                            apply_padding(padded, temp, width, left_justify, pad_char);
                        }
                        
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'f': {
                    if (length_mod == 4) {  // long double
                        double_to_str(va_arg(args, long double), temp, precision);
                    } else {
                        float_to_str((float)va_arg(args, double), temp, precision);
                    }
                    
                    if (width > 0) {
                        if (pad_char == '0' && temp[0] == '-') {
                            buffer[buffer_index++] = '-';
                            apply_padding(padded, temp + 1, width - 1, left_justify, pad_char);
                        } else {
                            apply_padding(padded, temp, width, left_justify, pad_char);
                        }
                        
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'g':
                case 'e': {
                    float_to_str((float)va_arg(args, double), temp, precision);
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'c': {
                    temp[0] = (char)va_arg(args, int);
                    temp[1] = '\0';
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        buffer[buffer_index++] = temp[0];
                    }
                    break;
                }
                case 'p': {
                    pointer_to_str(va_arg(args, void*), temp);
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'x':
                case 'X': {
                    int uppercase = (fmt[fmt_index] == 'X');
                    
                    if (length_mod == 2) {  // long
                        unsigned long value = va_arg(args, unsigned long);
                        int_to_hex_str((unsigned int)value, temp, uppercase);
                    } else {
                        int_to_hex_str(va_arg(args, unsigned int), temp, uppercase);
                    }
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'o': {
                    int_to_oct_str(va_arg(args, unsigned int), temp);
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'b': { 
                    int_to_bin_str(va_arg(args, unsigned int), temp);
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case 'u': {
                    if (length_mod == 3) {  // unsigned long long
                        unsigned long value = va_arg(args, unsigned long long);
                        unsigned_long_to_str(value, temp);
                    } else if (length_mod == 2) {
                        unsigned_long_to_str(va_arg(args, unsigned long), temp);
                    } else {
                        unsigned_int_to_str(va_arg(args, unsigned int), temp);
                    }
                    
                    if (width > 0) {
                        apply_padding(padded, temp, width, left_justify, pad_char);
                        char* pad_ptr = padded;
                        while (*pad_ptr != '\0') {
                            buffer[buffer_index++] = *pad_ptr++;
                        }
                    } else {
                        char* temp_ptr = temp;
                        while (*temp_ptr != '\0') {
                            buffer[buffer_index++] = *temp_ptr++;
                        }
                    }
                    break;
                }
                case '%': {
                    buffer[buffer_index++] = '%';
                    break;
                }
                default:
                    buffer[buffer_index++] = '%';
                    buffer[buffer_index++] = fmt[fmt_index];
                    break;
            }
            fmt_index++;
        } else {
            buffer[buffer_index++] = fmt[fmt_index++];
        }
    }
    buffer[buffer_index] = '\0';

    return buffer;
}

// simply a interface for the user
char *str_format(const char *s, ...) {
    va_list a;
    va_start(a, s);
    char *r = str_vformat(s, a);
    va_end(a);
    return r;
}

int vfprintf(int fd, const char *s, va_list args) {
    char *fmt = str_vformat(s, args);
    if (!fmt) return -1;
    int len = strlen(fmt);
    kernel_write(fd, fmt, len);
    return len;
}

int fprintf(int fd, const char *s, ...) {
    va_list args;
    va_start(args, s);
    int ret = vfprintf(fd, s, args);
    va_end(args);
    return ret;
}

int vprintf(const char *s, va_list args) {
    return vfprintf(STDOUT, s, args);
}

int printf(const char *s, ...) {
    va_list args;
    va_start(args, s);
    int ret = vfprintf(STDOUT, s, args);
    va_end(args);
    return ret;
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
