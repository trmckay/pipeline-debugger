#include "cli.h"
#include "data.h"
#include "debug.h"
#include "types.h"
#include "util.h"
#include "pollLib.h"
#include "serial.h"
#include <pwd.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// DESCRIPTION: takes the command as a string, and applies it to the serial port
// RETURNS: 0 for success, non-zero for error
int parse_cmd(char *line, target_t *tg) {
    char *cmd, *s_a1, *s_a2;
    int ec;
    word_t pc;

    // not strtok'd line for later use
    char line_copy[strlen(line) + 1];
    strcpy(line_copy, line);

    cmd = strtok(line, " ");
    word_t a1, a2;

    s_a1 = strtok(NULL, " ");
    s_a2 = strtok(NULL, " ");

    // connection test
    if (match_strs(cmd, CTEST_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage t <number>\n");
            return EXIT_FAILURE;
        }
        if ((a1 = get_num(tg->variables, s_a1)) < 1) {
            fprintf(stderr, "Error: usage: t <number>\n");
            return EXIT_FAILURE;
        }
        return connection_test(tg->serial_port, a1, 1, 0);
    }

    // pause
    if (match_strs(cmd, PAUSE_TOKEN)) {
        printf("Pause MCU\n");
        if (!(ec = mcu_pause(tg->serial_port, &pc)))
            printf("pc = 0x%02X\n", pc);
        return ec;
    }

    // resume
    if (match_strs(cmd, RESUME_TOKEN)) {
        printf("Resume MCU\n");
        return (mcu_resume(tg->serial_port));
    }

    // program
    if (match_strs(cmd, PROGRAM_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage: pr <mem.bin> [-f]\n");
            return EXIT_FAILURE;
        }
        if ((ec = mcu_pause(tg->serial_port, &pc)))
            return ec;
        if ((ec = mcu_program(tg->serial_port, s_a1, 1)))
            return ec;
        if ((ec = mcu_reset(tg->serial_port)))
            return ec;
        return mcu_resume(tg->serial_port);
    }

    // step
    if (match_strs(cmd, STEP_TOKEN)) {
        printf("Step\n");
        if ((ec = mcu_pause(tg->serial_port, &pc))) {
            return ec;
        }
        return mcu_step(tg->serial_port);
    }

    // reset
    if (match_strs(cmd, RESET_TOKEN)) {
        printf("Reset MCU\n");
        if ((ec = mcu_pause(tg->serial_port, &pc)))
            return ec;
        if ((ec = mcu_reset(tg->serial_port)))
            return ec;
        return mcu_resume(tg->serial_port);
    }

    // status (not implemented)
    if (match_strs(cmd, STATUS_TOKEN)) {
        printf("Request MCU status\n");
        fprintf(stderr, "Warning: not implemented\n");
        int s, err;
        err = mcu_status(tg->serial_port, &s);
        printf("Status: %d\n", s);
        return err;
    }

    // add breakpoint
    if (match_strs(cmd, BPADD_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage: b <pc>\n");
            return EXIT_FAILURE;
        }
        a1 = get_num(tg->variables, s_a1);

        for (int i = 0; i < tg->bp_cap; i++) {
            if (tg->breakpoints[i] < 0) {
                tg->breakpoints[i] = a1;
                printf("Add breakpoint %d @ pc = 0x%08X\n", i, a1);
                return mcu_add_breakpoint(tg->serial_port, a1);
            }
        }
        fprintf(stderr, "Error: max number of breakpoints reached\n");
        return EXIT_FAILURE;
    }

    // delete breakpoint
    if (match_strs(cmd, BPDEL_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage: d <bp-num>\n");
            return EXIT_FAILURE;
        }
        a1 = get_num(tg->variables, s_a1);
        if (a1 < tg->bp_cap && tg->breakpoints[a1] >= 0) {
            printf("Delete breakpoint %d @ pc = 0x%08X\n", a1,
                   (word_t)tg->breakpoints[a1]);
            int err = mcu_rm_breakpoint(tg->serial_port, tg->breakpoints[a1]);
            tg->breakpoints[a1] = -1;
            return err;
        } else {
            fprintf(stderr, "Error: breakpoint does not exist\n");
            return EXIT_FAILURE;
        }
    }

    // clear breakpoints
    if (match_strs(cmd, BPCLR_TOKEN)) {
        printf("Clear breakpoints\n");
        for (int i = 0; i < tg->bp_cap; i++) {
            if (tg->breakpoints[i] >= 0) {
                printf("Delete breakpoint %d @ pc = 0x%08X\n", i,
                       (word_t)tg->breakpoints[i]);
                if (mcu_rm_breakpoint(tg->serial_port, tg->breakpoints[i]))
                    return EXIT_FAILURE;
                tg->breakpoints[i] = -1;
            }
        }
        return EXIT_SUCCESS;
    }

    // list breakpoints
    if (match_strs(cmd, BPLIST_TOKEN)) {
        int none = 1;
        printf("List breakpoints\n");
        for (int i = 0; i < tg->bp_cap; i++) {
            if (tg->breakpoints[i] > 0)
                none = 0;
        }
        if (none) {
            printf("No breakpoints set\n");
            return EXIT_SUCCESS;
        }
        printf("NUM  |  PC\n");
        for (int i = 0; i < tg->bp_cap; i++) {
            if (tg->breakpoints[i] > 0)
                printf(" %d   |  0x%08X\n", i, (word_t)tg->breakpoints[i]);
        }
        return EXIT_SUCCESS;
    }

    // read register file
    if (match_strs(cmd, REG_RD_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage: rr <reg>\n");
            return EXIT_FAILURE;
        }
        a1 = parse_register_addr(tg->variables, s_a1);
        if (a1 < 0 || a1 > RF_SIZE) {
            fprintf(stderr, "Error: address out of range\n");
            return EXIT_FAILURE;
        }
        if (mcu_pause(tg->serial_port, &pc)) {
            fprintf(stderr, "Error: failed to pause MCU\n");
            return EXIT_FAILURE;
        }
        word_t r;
        int err;
        if (!(err = mcu_reg_read(tg->serial_port, a1, &r)))
            printf("x%d = %d (0x%08X)\n", a1, r, r);
        return err;
    }

    // write register file
    if (match_strs(cmd, REG_WR_TOKEN)) {
        if (s_a1 == NULL || s_a2 == NULL) {
            fprintf(stderr, "Error: usage: rw <reg> <data>\n");
            return EXIT_FAILURE;
        }
        a1 = parse_register_addr(tg->variables, s_a1);
        if (a1 < 0 || a1 > RF_SIZE) {
            fprintf(stderr, "Error: address out of range\n");
            return EXIT_FAILURE;
        }
        if (mcu_pause(tg->serial_port, &pc)) {
            fprintf(stderr, "Error: failed to pause MCU\n");
            return EXIT_FAILURE;
        }
        a2 = get_num(tg->variables, s_a2);
        int err;
        if (!(err = mcu_reg_write(tg->serial_port, a1, a2)))
            printf("x%d <- %d (0x%08X)\n", a1, a2, a2);
        return err;
    }

    // read word from memory
    if (match_strs(cmd, MEM_RD_W_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage: d <pc>\n");
            return EXIT_FAILURE;
        }
        if (mcu_pause(tg->serial_port, &pc)) {
            fprintf(stderr, "Error: failed to pause MCU\n");
            return EXIT_FAILURE;
        }
        a1 = get_num(tg->variables, s_a1);
        if (a1 < 0 || a1 > 0xFFFFFFFF) {
            fprintf(stderr, "Error: address out of range\n");
            return EXIT_FAILURE;
        }
        word_t r;
        int err;
        if (!(err = mcu_mem_read_word(tg->serial_port, a1, &r)))
            printf("MEM[0x%08X] = %d (0x%08X)\n", a1, r, r);
        return err;
    }

    // write word to memory
    if (match_strs(cmd, MEM_WR_W_TOKEN)) {
        if (s_a1 == NULL || s_a2 == NULL) {
            fprintf(stderr, "Error: usage: mww <addr> <data>\n");
            return EXIT_FAILURE;
        }
        a1 = get_num(tg->variables, s_a1);
        a2 = get_num(tg->variables, s_a2);
        if (a1 < 0 || a1 > 0xFFFFFFFF) {
            fprintf(stderr, "Error: address out of range\n");
            return EXIT_FAILURE;
        }
        if (mcu_pause(tg->serial_port, &pc)) {
            fprintf(stderr, "Error: failed to pause MCU\n");
            return EXIT_FAILURE;
        }
        int err;
        if (!(err = mcu_mem_write_word(tg->serial_port, a1, a2)))
            printf("MEM[0x%08X] <- %d (0x%08X)\n", a1, a2, a2);
        return err;
    }

    // read byte from memory
    if (match_strs(cmd, MEM_RD_B_TOKEN)) {
        if (s_a1 == NULL) {
            fprintf(stderr, "Error: usage: d <pc>\n");
            return EXIT_FAILURE;
        }
        if (mcu_pause(tg->serial_port, &pc)) {
            fprintf(stderr, "Error: failed to pause MCU\n");
            return EXIT_FAILURE;
        }
        a1 = get_num(tg->variables, s_a1);
        byte_t r;
        int err;
        if (!(err = mcu_mem_read_byte(tg->serial_port, a1, &r)))
            printf("MEM[0x%08X] = %d (0x%04X)\n", a1, r, r);
        return err;
    }

    // write a byte to memory
    if (match_strs(cmd, MEM_WR_B_TOKEN)) {
        if (s_a1 == NULL || s_a2 == NULL) {
            fprintf(stderr, "Error: usage: mww <addr> <data>\n");
            return EXIT_FAILURE;
        }
        if ((a1 = get_num(tg->variables, s_a1)) < 0) {
            fprintf(stderr, "Error: address must be positive integer\n");
        }
        if (mcu_pause(tg->serial_port, &pc)) {
            fprintf(stderr, "Error: failed to pause MCU\n");
            return EXIT_FAILURE;
        }
        a2 = get_num(tg->variables, s_a2);
        int err;
        if (!(err = mcu_mem_write_byte(tg->serial_port, a1, a2)))
            printf("MEM[0x%08X] <- %d (0x%04X)\n", a1, a2, a2);
        return err;
    }

    // print unrecognized cmd msg and return error
    INVLD_CMD(line_copy);
    return EXIT_FAILURE;
}

void receive_otter_input(int serial_port) {
    char buffer[MAX_RECEIVE_LEN];

    if (read_string(serial_port, buffer) != 0) {
        return;
    }

    printf("OTTER: %s\n", buffer);
}

void readFromStdin(char *buffer) {
    int len;

    if (fgets(buffer, MAX_INPUT_LEN, stdin) == NULL) {
        perror("fgets");
        exit(-1);
    }

    // remove \n
    len = strlen(buffer);
    buffer[len-1] = '\0';
}

// DESCRIPTION: launches a debugger command line interface (a la GDB)
//   on the device at the designated serial port
void debug_cli(char *path, int serial_port) {
    char line[MAX_INPUT_LEN];
    int err = 0;

    // create and populate a hash table of variables
    char *keys[MAX_VAR_COUNT];
    word_t values[MAX_VAR_COUNT];
    ht_t *vars_ht = g_hash_table_new(g_str_hash, g_str_equal);

    // get path to config file
    const char *home_dir = getenv("HOME");
    char config_path[strlen(home_dir) + strlen(REL_CONFIG_PATH) + 1];
    strcpy(config_path, home_dir);
    strcat(config_path, REL_CONFIG_PATH);
    // populate variables from file
    int vc = populate_vars(vars_ht, keys, values, config_path);

    if (vc > 0) {
        printf("\nUsing variables:\n");
        for (int i = 0; i < vc; i++) {
            printf("  %s = 0x%08X (%d)\n", keys[i], values[i], values[i]);
        }
    }

    // Array of breakpoints (also should be tracked in the module)
    // -1 = none
    // positive int = PC of breakpoint
    // only hardware breakpoints are supported right now
    int64_t bps[MAX_BREAK_PTS] = {-1, -1, -1, -1, -1, -1, -1, -1};

    // create a packed structure for target
    target_t tg;
    tg.serial_port = serial_port;
    tg.variables = vars_ht;
    tg.paused = 0;
    tg.breakpoints = bps;
    tg.bp_cap = MAX_BREAK_PTS;
    tg.pipe = 0;

    printf("\n" CYAN "UART Debugger/Receiver\n" RESET);
    printf("Enter 'h' for usage details.\n");

    // create a pollset to poll stdin and the serial port
    setupPollSet();
    addToPollSet(STDIN_FILENO);
    addToPollSet(serial_port);

    // run until EOD is read
    while (1) {
        // prompt
        printf("\nrvdb @ %s", path);
        if (tg.paused)
            printf(" (paused)\n");
        else
            printf("\n");

        if (err)
            printf(RED "$ " RESET);
        else
            printf(GREEN "$ " RESET);
        fflush(stdout); // flush stdout to display the prompt
        memset(line, 0, MAX_INPUT_LEN);

        if (pollCall(-1) == STDIN_FILENO) {
            // User has entered input
            readFromStdin(line);

            if (line[0] == '!') {
                err = 0;
                system(line + 1);
            }

            // help message
            else if (match_strs(line, "h")) {
                HELP_MSG;
                err = 0;
            }

            // quit/exit
            else if (match_strs(line, "q")) {
                ht_destroy(vars_ht, keys, vc);
                return;
            } else if (match_strs(line, "exit")) {
                ht_destroy(vars_ht, keys, vc);
                return;
            } else {
                err = parse_cmd(line, &tg);
            }

        } else {
            // Otter is talking
            receive_otter_input(serial_port);
        }

        
    }
}
