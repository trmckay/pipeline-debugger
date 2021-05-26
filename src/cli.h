#ifndef CLI_H
#define CLI_H

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define RESET "\x1b[0m"

#define MAX_BREAK_PTS 8
#define MAX_VAR_COUNT 256

#define MAX_RECEIVE_LEN 1024

#define MAX_INPUT_LEN 1024

#define REL_CONFIG_PATH "/.config/rvdb/config"

#define CTEST_TOKEN "t"
#define PAUSE_TOKEN "p"
#define RESUME_TOKEN "r"
#define PROGRAM_TOKEN "pr"
#define STEP_TOKEN "s"
#define RESET_TOKEN "rst"
#define STATUS_TOKEN "st"
#define BPADD_TOKEN "b"
#define BPDEL_TOKEN "del"
#define BPCLR_TOKEN "bc"
#define BPLIST_TOKEN "bl"
#define REG_RD_TOKEN "rr"
#define REG_WR_TOKEN "rw"
#define MEM_RD_W_TOKEN "mrw"
#define MEM_WR_W_TOKEN "mww"
#define MEM_RD_B_TOKEN "mrb"
#define MEM_WR_B_TOKEN "mwb"

#define X0 "zero"
#define X1 "ra"
#define X2 "sp"
#define X3 "gp"
#define X4 "tp"
#define X5 "t0"
#define X6 "t1"
#define X7 "t2"
#define X8 "s0"
#define X9 "s1"
#define X10 "a0"
#define X11 "a1"
#define X12 "a2"
#define X13 "a3"
#define X14 "a4"
#define X15 "a5"
#define X16 "a6"
#define X17 "a7"
#define X18 "s2"
#define X19 "s3"
#define X20 "s4"
#define X21 "s5"
#define X22 "s6"
#define X23 "s7"
#define X24 "s8"
#define X25 "s9"
#define X26 "s10"
#define X27 "s11"
#define X28 "t3"
#define X29 "t4"
#define X30 "t5"
#define X31 "t6"

#define HELP_MSG                                                               \
    "RISC-V UART Debugger (rvdb) v1.4 | Trevor McKay "                         \
    "<trmckay@calpoly.edu>\n\n"                                                \
    "USAGE\n"                                                                  \
    "    rvdb [device]\n\n"                                                    \
    "MORE INFO\n"                                                              \
    "    man rvdb\n"

#define HELP() printf(HELP_MSG)

#define INVLD_CMD(L)                                                           \
    fprintf(stderr,                                                            \
            "Error: unrecognized command\n"                                    \
            "Execute external shell commands with a leading '!'.\n"            \
            "Example: '! %s' or '!%s'\n"                                       \
            "Enter 'h' for more information.\n",                               \
            (L), (L));

void restore_term(int serial_port);
void debug_cli(char *path, int serial_port);

#endif
