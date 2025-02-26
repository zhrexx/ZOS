// TODO: FileSystem > Load programs from disk
// TODO: Drivers
#include "msstd.h"

#define K_VERSION "1.0"
#define K_SHELL_SYMBOL "$ "

void calculator() {
    printf("First Number: ");
    char *fnum_str = fgets_dcc(256);
    int fnum = atoi(fnum_str);
    printf("Second Number: ");
    char *snum_str = fgets_dcc(256);
    int snum = atoi(snum_str);
    printf("(+|-|*|/) ");
    char *symbol_str = fgets_dcc(256);
    char symbol = symbol_str[0];
    if (symbol == '\0') {
        printf("ERROR: No symbol specified\n");
        return;
    }
    printf("\n");
    int r;
    switch (symbol) {
        case '+': r = fnum + snum; break;
        case '-': r = fnum - snum; break;
        case '*': r = fnum * snum; break;
        case '/': 
            if (snum == 0) {
                printf("ERROR: Division by zero\n");
                return;
            }
            r = fnum / snum; 
            break;
        default:
            printf("ERROR: Unknown symbol: %c\n", symbol);
            return;
    }
    printf("Result: %d\n", r);
}

uint64_t get_cpu_speed(void) {
    uint64_t start, end;
    uint64_t cycles;
    start = rdtsc();
    for (volatile int i = 0; i < 1000000; i++) {}
    end = rdtsc();
    cycles = end - start;
    return cycles;
}


void shell_run() {
    printf(K_SHELL_SYMBOL);
    char *cmd = fgets_dcc(256);
    while (cmd != NULL) {
        if (cmd[0] == '\0') {
            cmd = fgets_dcc(256);
            printf(K_SHELL_SYMBOL);
            continue;
        }
        if (strcmp(cmd, "exit") == 0) {
            kernel_shutdown();
        } else if (strcmp(cmd, "clear") == 0) {
            kernel_clear_screen();
            printf("ZOS %s\n", K_VERSION);
            kernel_change_color("default");
        } else if (strcmp(cmd, "author") == 0) {
            printf("| Project: ZOS\n| Author: zhrexx\n");
        } else if (strncmp(cmd, "color ", 6) == 0) {
            kernel_change_color(cmd+6);            
        } else if (strncmp(cmd, "calc", 4) == 0) {
            calculator();
        } else if (strcmp(cmd, "help") == 0) {
            printf("| ZOS Help                    |\n");
            printf("| calc - A simple Calculator  |\n");
            printf("| color <str> - Set a color   |\n");
            printf("| clear - clears the screen   |\n");
            printf("| exit - shutdowns the PC     |\n");
        } else if (strcmp(cmd, "infload") == 0) {
            return;
        } else if (strcmp(cmd, "time") == 0) {
            printf("%s\n", time_now());
        } else {
            printf("Unknown Command: %s\n", cmd);
        }
        printf(K_SHELL_SYMBOL);
        cmd = fgets_dcc(256);
    }
}

void kernel_main(unsigned int magic, unsigned int* mboot_info) {
    (void) magic, (void)mboot_info;
    kernel_clear_screen();
    printf("ZOS %s\n", K_VERSION);
    printf("%s\n", time_now());
    shell_run();
    kernel_clear_screen();
    printf("Infinite Loading\nIf you wanna shutdown click 'q'\n");
    uint64_t cps = get_cpu_speed();
    for (size_t i = 0; 1 ;i++) {
        if (getchar_nb() == 'q') {
            break;
        }

        kernel_display_spinner(10, VGA_WIDTH/2-1, i);
        kernel_delay(cps*2);
    }

    kernel_shutdown();
}







