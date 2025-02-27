// TODO: FileSystem > Load programs from disk
// TODO: Drivers
#include "msstd.h"

#define K_VERSION 1.0
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
                kernel_panic("division by zero\n");
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

void number_game() {
    while (1) {
        uint32_t rnum = kernel_rand_range(1, 100);
        printf("Enter a number: ");
        const char *epswd = fgets_dcc(100);
        int enumd = atoi(epswd);
        printf("\n");
        
        if (rnum == (uint32_t)enumd) {
            break;
        }
        kernel_clear_screen();
        printf("Wrong number guessed it was %d\n", rnum);
    }
    printf("Number guessed\n");
}

int get_day_of_week(int year, int month) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int k = year % 100;
    int j = year / 100;
    int day = (1 + 13 * (month + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (day + 6) % 7;
}

int get_days_in_month(int year, int month) {
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        } else {
            return 28;
        }
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

void print_calendar(int year, int month) {
    printf("     %d-%02d\n", year, month);
    printf(" Su Mo Tu We Th Fr Sa\n");

    int days_in_month = get_days_in_month(year, month);
    int starting_day = get_day_of_week(year, month);

    for (int i = 0; i < starting_day; i++) {
        printf("   ");
    }

    struct Day d = kernel_localtime(kernel_time().seconds);
    int current_day = d.day;
    int current_month = d.month;
    int current_year = d.year;

    for (int day = 1; day <= days_in_month; day++) {
        if (year == current_year && month == current_month && day == current_day) {
                printf("[%2d] ", day);
        } else {
            printf("%2d ", day);
        }
        if ((starting_day + day) % 7 == 0) {
            printf("\n");
        }
    }

    printf("\n");
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
            printf("ZOS %f\n", K_VERSION);
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
            printf("| time - prints current time  |\n");
            printf("| timer - a simple timer      |\n");
            printf("| cal - a simple calender     |\n");
            printf("| numgame - a simple game     |\n");
            printf("| exit - shutdowns the PC     |\n");
        } else if (strcmp(cmd, "infload") == 0) {
            return;
        } else if (strcmp(cmd, "time") == 0) {
            printf("%s\n", time_now());
        } else if (strcmp(cmd, "timer") == 0) {
            struct time_info prev = kernel_time();
            int c = 0;
            int running = 1;
            uint64_t cps = get_cpu_speed();
            printf("Timer. Press 'q' to exit.\n");
            while (running) {
                struct time_info ti = kernel_time();
                if (ti.seconds != prev.seconds) {
                    c++;
                    printf("%d\n", c);
                    term_row--;
                    prev = ti;
                }
                if (getchar_nb() == 'q') {
                    term_row++;
                    kernel_clean_latest_line();
                    kernel_clean_latest_line();
                    running = 0;
                }
                kernel_delay(cps / 1000);
            }
        } else if (strcmp(cmd, "cal") == 0) {
            struct Day d = kernel_localtime(kernel_time().seconds);
            print_calendar(d.year, d.month);
        } else if (strcmp(cmd, "numgame") == 0) {
            number_game();
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
    printf("ZOS %f\n", K_VERSION);
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







