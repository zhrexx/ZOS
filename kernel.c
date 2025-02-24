#include "msstd.h"

#define K_SHELL_SYMBOL "$ "

void shell_run();

void kernel_main(unsigned int magic, unsigned int* mboot_info) {
    (void) magic, (void) mboot_info;
    kernel_clear_screen();
    printf("Welcome to the ZOS\n");
    
    shell_run();
    while(1) __asm__("hlt");
}


void shell_run() {
    printf(K_SHELL_SYMBOL);
    char *cmd = fgets_dcc(256);
    while (cmd != NULL) {
        printf(K_SHELL_SYMBOL);
        if (cmd[0] == '\0') {
            goto s_end;
        }
        if (strcmp(cmd, "exit") == 0) {
            kernel_exit();            
        } else if (strcmp(cmd, "clear") == 0) {
            kernel_clear_screen();
            printf(K_SHELL_SYMBOL);
        } else {
            printf("Unknown Command: %s\n", cmd);
        }
s_end:
        cmd = fgets_dcc(256);
    }
}
