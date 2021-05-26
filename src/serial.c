// Raw I/O UART transciever
// Updated by Trevor McKay
//
// Based on code by Keefe Johnson:
//     https://github.com/KeefeJ/otter_debugger
//
// Original terminal config code from:
//     https://www.gnu.org/software/libc/manual/html_node/Noncanon-Example.html

#include "serial.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

term_sa saved_attributes;

/* open_serial
 *
 * DESCRIPTION
 * Opens a raw input/output serial connection for transferring bytes
 * at a time.
 *
 * RETURNS
 * Nothing; saves serial port FD to int *serial_port.
 *
 * ARGUMENTS
 * char *path: string of the path to the serial port;
 *     something like /dev/ttyX
 * int *serial_port: pointer to an integer which should be allocated in
 *     the main stack frame or dynamically
 */

void restore_term(int serial_port) {
    printf("Restoring serial port settings... ");
    tcsetattr(serial_port, TCSANOW, &saved_attributes);
    printf("restored\n");
    printf("Closing port... ");
    close(serial_port);
    printf("closed\n");
}

int open_serial(char *path, int *serial_port) {
    term_sa saved_attributes;

    struct termios tattr;

    if ((*serial_port = open(path, O_RDWR)) == -1) {
        fprintf(stderr, "Error: open(%s): %s\n", path, strerror(errno));
        return 1;
    }

    /* Make sure the port is a terminal. */
    if (!isatty(*serial_port)) {
        fprintf(stderr, "Error: file is not a terminal\n");
        return 2;
    }

    /* Save the terminal attributes so we can restore them later. */
    tcgetattr(*serial_port, &saved_attributes);

    // Set the funny terminal modes.
    tcgetattr(*serial_port, &tattr);
    tattr.c_oflag &= ~OPOST; // raw output
    tattr.c_lflag &=
        ~(ICANON | ECHO | ECHOE | ISIG | ECHONL | IEXTEN); // raw input
    tattr.c_cflag &= ~(CSIZE | PARENB | CSTOPB);           // 8N1 ...
    tattr.c_cflag |= (CS8 | CLOCAL | CREAD); // ... and enable without ownership
    // more raw input, and no software flow control
    tattr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                       ICRNL | IXON | IXOFF | IXANY);
    tattr.c_cc[VMIN] = MIN_BYTES;
    tattr.c_cc[VTIME] =
        INTER_BYTE_TIMEOUT;    // allow up to 1.0 secs between bytes received
    cfsetospeed(&tattr, BAUD); // set baud rate
    tcsetattr(*serial_port, TCSAFLUSH, &tattr);

    return 0;
}

// send a word to the device and return 0 if successful
int send_word(int serial_port, word_t w) {
    ssize_t bw;
    w = htonl(w);

    bw = write(serial_port, &w, 4);

    if (bw == -1) {
        perror("write(serial)");
        return 1;
    }
    if (bw != 4) {
        fprintf(stderr, "Error: wrote only %ld of 4 bytes\n", bw);
        return 1;
    }

    return 0;
}

// read a byte from thhe device and return 0 if successful
int recv_byte(int serial_port, byte_t *byte) {
    ssize_t br;

    br = read(serial_port, byte, 1);

    if (br == -1) {
        perror("read(serial_port)");
        exit(-1);
    }
    if (br == 0) {
        fprintf(stderr, "Error: read 0 bytes from serial_port\n");
        return 1;
    }
    return 0;
}

// read a word from the device and return 0 if successful
int recv_word(int serial_port, word_t *word) {
    word_t w;
    ssize_t br;
    w = 0;

    br = read(serial_port, &w, 4);

    if (br == -1) {
        perror("read(serial_port)");
        return 1;
    }
    if (br != 4) {
        fprintf(stderr, "Error: read only %ld of 4 bytes: 0x%08X\n", br,
                ntohl(w));
        return 1;
    }

    *word = ntohl(w);
    return 0;
}

// wait for a readable byte until timeout
int wait_readable(int serial_port, int msec) {
    int r;
    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(serial_port, &set);
    timeout.tv_sec = msec / 1000;
    timeout.tv_usec = (msec % 1000) * 1000;

    r = select(serial_port + 1, &set, NULL, NULL, &timeout);

    if (r == -1) {
        perror("select");
        return 0;
    }

    return FD_ISSET(serial_port, &set);
}

// read a word after the specified timeout
// if it's not ready yet, the read fails
// return 0 on success
int read_word(int serial_port, word_t *word) {
    word_t r;
    if (wait_readable(serial_port, TIMEOUT_MSEC)) {
        recv_word(serial_port, &r);
        *word = r;
        return 0;
    }
    return 1;
}

// reads bytes from the serial port until it reaches a null terminator
// this depends on the otter programmer sending a 0 to not get stuck, could be improved to timeout
// returns 0 on success
int read_string(int serial_port, char *data) {
    int i;
    byte_t byte;
    i = 0;

    do {
        if (recv_byte(serial_port, &byte) != 0) {
            return 1;
        }
        data[i++] = (char)byte;
    } while(byte != '\0');

    return 0;
}
