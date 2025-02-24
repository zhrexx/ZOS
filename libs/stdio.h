#ifndef STDIO_H
#define STDIO_H

#include "types.h"
#include "memory.h" 
#include "interfaces.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int shift = 0, ctrl = 0, alt = 0;

int getchar(void) {
    while (!(inb(0x64) & 1));

    uint8_t scancode = inb(0x60); 

    static char scancode_map[256] = {
        0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
        0,  '\\','z','x','c','v','b','n','m',',','.','/',
        0,   0,   0,  ' ',
        0,   0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,   0,   0,  0,  0,   0,  0,  0,  0,  0,   0,  0,   0,  0,  0,  0,
        0,   0,   0,  0,  0,   0,  0,  0,  0,  0,   0,  0,   0,  0,  0,  0,
        0,   0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 0,  0,  0,  0,  0,  0,
        [0x2A] = 0, [0x36] = 0, [0x1D] = 0, [0x9D] = 0, [0x38] = 0, [0xB8] = 0
    };

    if (scancode == 0x2A || scancode == 0x36) {
        shift = 1;
        return 0;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift = 0;
        return 0;
    }

    if (scancode == 0x1D) {
        ctrl = 1;
        return 0;
    }
    if (scancode == 0x9D) {
        ctrl = 0;
        return 0;
    }

    if (scancode == 0x38) {
        alt = 1;
        return 0;
    }
    if (scancode == 0xB8) {
        alt = 0;
        return 0;
    }

    if (scancode & 0x80) {
        return 0;
    }

    char key = scancode_map[scancode];

    if (shift) {
        if (key >= 'a' && key <= 'z') {
            key -= 32; 
        } else if (key == '1') {
            key = '!';
        } else if (key == '2') {
            key = '@';
        } else if (key == '3') {
            key = '#';
        }
    }

    if (ctrl) {
        if (key >= 'a' && key <= 'z') {
            key -= 32;
        }
    }

    return key;
}

int is_shift_pressed(void) {
    return shift;
}

int is_ctrl_pressed(void) {
    return ctrl;
}

int is_alt_pressed(void) {
    return alt;
}

char *fgets(int size) {
    if (size <= 1) return NULL;
    char *buffer = (char *)aarena_alloc(&arena, size);
    if (!buffer) return NULL;

    int i = 0;
    while (i < size - 1) {
        char c = getchar();
        
        if (c == '\n' || c == '\r') break;
        
        if (c == '\b' && i > 0) { 
            i--;
            kernel_clean_latest_char();
        } else if (c && c != '\b') {
            buffer[i++] = c;
        }
    }

    buffer[i] = '\0';
    return buffer;
}

// dcc = Display clicked character
int getchar_dcc(void) {
    char x = getchar();
    if (x != '\b') {
        printf("%c", x);
    }
    return x;
}

char *fgets_dcc(int size) {
    if (size <= 1) return NULL;
    char *buffer = (char *)aarena_alloc(&arena, size);
    if (!buffer) return NULL;

    int i = 0;
    while (i < size - 1) {
        char c = getchar_dcc();
        
        if (c == '\n' || c == '\r') break;
        
        if (c == '\b' && i > 0) {
            i--;
            kernel_clean_latest_char();
        } else if (c && c != '\b') {
            buffer[i++] = c;
        }
    }

    buffer[i] = '\0';
    return buffer;
}

#endif // STDIO_H

