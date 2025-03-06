#ifndef STDIO_H
#define STDIO_H

#include "types.h"
#include "memory.h" 
#include "interfaces.h"

//--------------------------

#define KEY_UP      1000
#define KEY_DOWN    1001
#define KEY_LEFT    1002
#define KEY_RIGHT   1003

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static int shift = 0, ctrl = 0, alt = 0;
static int layout = 0;
static int extended = 0;

static char qwerty_map[256] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/',
    0,   0,   0,  ' ',
};

static char qwerty_shift_map[256] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,   '|','Z','X','C','V','B','N','M','<','>','?',
    0,   0,   0,  ' ',
};

static char qwertz_map[256] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','\'','\b',
    '\t','q','w','e','r','t','z','u','i','o','p','[','+','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','#',
    0,   '<','y','x','c','v','b','n','m',',','.','/',
    0,   0,   0,  ' ',
};

static char qwertz_shift_map[256] = {
    0,  27, '!','"','#','$','%','&','/','(',')','=','_','`','\b',
    '\t','Q','W','E','R','T','Z','U','I','O','P','{','*','\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','\'',
    0,   '>','Y','X','C','V','B','N','M',';',':','_',
    0,   0,   0,  ' ',
};

void set_keyboard_layout(int new_layout) {
    layout = new_layout;
}

int get_keyboard_layout(void) {
    return layout;
}

int getchar(void) {
    while (!(inb(0x64) & 1));

    uint8_t scancode = inb(0x60);
    
    if (scancode == 0xE0) {
        extended = 1;
        return 0;
    }
    
    if (extended) {
        extended = 0;
        switch (scancode) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            case 0xAA:
            case 0xC8: case 0xD0: case 0xCB: case 0xCD:
                return 0;
        }
    }

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

    if (alt && ctrl && scancode == 0x21) {
        layout = 0;
        return 0;
    }

    if (alt && ctrl && scancode == 0x22) {
        layout = 1;
        return 0;
    }

    if (scancode & 0x80) {
        return 0;
    }

    char key;
    if (layout == 0) {
        key = shift ? qwerty_shift_map[scancode] : qwerty_map[scancode];
    } else {
        key = shift ? qwertz_shift_map[scancode] : qwertz_map[scancode];
    }

    if (ctrl) {
        if (key >= 'a' && key <= 'z') {
            key = key - 'a' + 1;
        } else if (key >= 'A' && key <= 'Z') {
            key = key - 'A' + 1;
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
    char *buffer = (char *)malloc(size);
    if (!buffer) return NULL;

    int i = 0;
    int cursor_pos = 0;
    
    while (i < size - 1) {
        int c = getchar();
        
        if (c == KEY_LEFT && cursor_pos > 0) {
            cursor_pos--;
            printf("\033[D");
            continue;
        }
        if (c == KEY_RIGHT && cursor_pos < i) {
            cursor_pos++;
            printf("\033[C");
            continue;
        }
        
        if (c == '\n' || c == '\r') break;
        
        if (c == '\b' && cursor_pos > 0) {
            cursor_pos--;
            i--;
            kernel_clean_latest_char();
        } else if (c > 0 && c < 1000) {
            if (cursor_pos < i) {
                for (int j = i; j > cursor_pos; j--) {
                    buffer[j] = buffer[j-1];
                }
                buffer[cursor_pos] = c;
                i++;
                cursor_pos++;
                
                for (int j = cursor_pos - 1; j < i; j++) {
                    printf("%c", buffer[j]);
                }
                
                printf("\033[%dD", i - cursor_pos);
            } else {
                buffer[cursor_pos] = c;
                i++;
                cursor_pos++;
            }
        }
    }

    buffer[i] = '\0';
    return buffer;
}

int getchar_dcc(void) {
    int x = getchar();
    if (x > 0 && x < 128 && x != '\b') {
        printf("%c", x);
    }
    return x;
}

char *fgets_dcc(int size) {
    if (size <= 1) return NULL;
    char *buffer = (char *)malloc(size);
    if (!buffer) return NULL;

    int i = 0;
    int cursor_pos = 0;
    
    while (i < size - 1) {
        int c = getchar_dcc();
        
        if (c == KEY_LEFT && cursor_pos > 0) {
            cursor_pos--;
            printf("\033[D");
            continue;
        }
        if (c == KEY_RIGHT && cursor_pos < i) {
            cursor_pos++;
            printf("\033[C");
            continue;
        }
        
        if (c == '\n' || c == '\r') break;
        
        if (c == '\b' && cursor_pos > 0) {
            cursor_pos--;
            i--;
            kernel_clean_latest_char();
        } else if (c > 0 && c < 1000) {
            if (cursor_pos < i) {
                for (int j = i; j > cursor_pos; j--) {
                    buffer[j] = buffer[j-1];
                }
                buffer[cursor_pos] = c;
                i++;
                cursor_pos++;
                
                for (int j = cursor_pos - 1; j < i; j++) {
                    printf("%c", buffer[j]);
                }
                
                printf("\033[%dD", i - cursor_pos);
            } else {
                buffer[cursor_pos] = c;
                i++;
                cursor_pos++;
            }
        }
    }

    buffer[i] = '\0';
    return buffer;
}

int getchar_nb(void) {
    if (!(inb(0x64) & 1)) {
        return -1;
    }
    
    uint8_t scancode = inb(0x60);
    
    if (scancode == 0xE0) {
        extended = 1;
        return 0;
    }
    
    if (extended) {
        extended = 0;
        switch (scancode) {
            case 0x48: return KEY_UP;
            case 0x50: return KEY_DOWN;
            case 0x4B: return KEY_LEFT;
            case 0x4D: return KEY_RIGHT;
            case 0xAA:
            case 0xC8: case 0xD0: case 0xCB: case 0xCD:
                return 0;
        }
    }
    
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

    if (alt && ctrl && scancode == 0x21) {
        layout = 0;
        return 0;
    }

    if (alt && ctrl && scancode == 0x22) {
        layout = 1;
        return 0;
    }
    
    if (scancode & 0x80) {
        return 0;
    }
    
    char key;
    if (layout == 0) {
        key = shift ? qwerty_shift_map[scancode] : qwerty_map[scancode];
    } else {
        key = shift ? qwertz_shift_map[scancode] : qwertz_map[scancode];
    }

    if (ctrl) {
        if (key >= 'a' && key <= 'z') {
            key = key - 'a' + 1;
        } else if (key >= 'A' && key <= 'Z') {
            key = key - 'A' + 1;
        }
    }
    
    return key;
}

#endif
