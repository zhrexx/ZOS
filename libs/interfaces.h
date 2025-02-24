#ifndef INTERFACES_H
#define INTERFACES_H

#include "types.h"
#include "memory.h"
#include "kernel.h"
#include "stdarg.h"

// ------------------------------------------
// Interfaces
// ------------------------------------------
static void int_to_str(int value, char *str) {
    char temp[20];
    int i = 0, j = 0;

    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    if (value < 0) {
        str[j++] = '-';
        value = -value;
    }

    while (value > 0) {
        temp[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        str[j++] = temp[--i];
    }

    str[j] = '\0';
}

static void float_to_str(float value, char *str) {
    int int_part = (int)value;
    float frac_part = value - int_part;
    char int_str[20];
    char frac_str[20];

    int_to_str(int_part, int_str);

    frac_part *= 100;
    int frac_int = (int)frac_part;
    int_to_str(frac_int, frac_str);

    int i = 0, j = 0;
    while (int_str[i] != '\0') {
        str[j++] = int_str[i++];
    }
    str[j++] = '.';
    i = 0;
    while (frac_str[i] != '\0') {
        str[j++] = frac_str[i++];
    }
    str[j] = '\0';
}


size_t strlen(const char *s) {
    size_t c = 0;
    while (s[c] != '\0') {
        c++;
    }
    return c;
}

char *str_format(const char *fmt, va_list args) {
    int fmt_index = 0;
    size_t total_length = 0;

    va_list args_copy;
    va_copy(args_copy, args);

    while (fmt[fmt_index] != '\0') {
        if (fmt[fmt_index] == '%' && fmt[fmt_index + 1] != '\0') {
            switch (fmt[fmt_index + 1]) {
                case 's': {
                    char *arg = va_arg(args_copy, char *);
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

    char *buffer = (char *)aarena_alloc(&arena, total_length + 1);
    if (buffer == NULL) {
        return NULL;
    }

    fmt_index = 0;
    int buffer_index = 0;
    while (fmt[fmt_index] != '\0') {
        if (fmt[fmt_index] == '%' && fmt[fmt_index + 1] != '\0') {
            switch (fmt[fmt_index + 1]) {
                case 's': {
                    char *arg = va_arg(args, char *);
                    while (*arg != '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                case 'd': {
                    char temp[20];
                    int_to_str(va_arg(args, int), temp);
                    char *arg = temp;
                    while (*arg != '\0') {
                        buffer[buffer_index++] = *arg++;
                    }
                    break;
                }
                case 'f': {
                    char temp[32];
                    float_to_str((float)va_arg(args, double), temp);
                    char *arg = temp;
                    while (*arg != '\0') {
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


#endif
