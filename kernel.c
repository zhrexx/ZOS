// TODO: FileSystem > Load programs from disk
// TODO: Drivers
#include "msstd.h"
// #include "drivers/mfs.h"

#define K_VERSION 1.0
#define K_SHELL_SYMBOL "$ "

void shell_run();

void kernel_main(unsigned int magic, unsigned int* mboot_info) {
    (void) magic, (void) mboot_info;
    kernel_clear_screen();
    printf("ZOS %f\n", K_VERSION);
    printf("Loading...");

    // TODO: Real loading 
    int loading_in_progress = 1;
    int f = 0;
    while (loading_in_progress) {
        kernel_display_spinner(1, 12, f++);
        kernel_delay(20000000);
        if (f == 100) {
            loading_in_progress = 0;
        }
    }

    kernel_clear_screen();    
    shell_run();
    while(1) __asm__("hlt");
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
            kernel_exit();            
        } else if (strcmp(cmd, "clear") == 0) {
            kernel_clear_screen();
            kernel_change_color("default");
        } else if (strcmp(cmd, "author") == 0) {
            printf("| Project: ZOS\n| Author: zhrexx\n");
        } else if (strncmp(cmd, "color ", 6) == 0) {
            kernel_change_color(cmd+6);            
        } else {
            printf("Unknown Command: %s\n", cmd);
        }
        
        printf(K_SHELL_SYMBOL);
        cmd = fgets_dcc(256);
    }
}
