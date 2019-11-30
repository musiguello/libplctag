/***************************************************************************
 *   Copyright (C) 2019 by Kyle Hayes                                      *
 *   Author Kyle Hayes  kyle.hayes@gmail.com                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include "tests/regression/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include "lib/libplctag.h"


static int calculate_pattern_byte_length(const char *pattern);

/*
 * This contains the utilities used by the test harness.
 */



int util_sleep_ms(int ms)
{
    struct timeval tv;

    tv.tv_sec = ms/1000;
    tv.tv_usec = (ms % 1000)*1000;

    return select(0,NULL,NULL,NULL, &tv);
}


/*
 * time_ms
 *
 * Return the current epoch time in milliseconds.
 */
int64_t util_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv,NULL);

    return  ((int64_t)tv.tv_sec*1000)+ ((int64_t)tv.tv_usec/1000);
}



/*
 * Logging routines.
 */

void error(const char *templ, ...)
{
    va_list va;

    /* print it out. */
    va_start(va,templ);
    vfprintf(stderr,templ,va);
    va_end(va);
    fprintf(stderr,"\n");

    exit(1);
}



void info(const char *templ, ...)
{
    va_list va;

    /* print it out. */
    va_start(va,templ);
    vfprintf(stderr,templ,va);
    va_end(va);
    fprintf(stderr,"\n");
}

/*
 * SIGCONT support
 */

volatile sig_atomic_t sigcont_received = 0;

static void SIGCONT_handler(int not_used)
{
    (void)not_used;

    sigcont_received = 1;
}


void setup_sigcont_handler(void)
{
    struct sigaction SIGCONT_act;

    /* set up child signal handler first. */
    memset(&SIGCONT_act, 0, sizeof(SIGCONT_act));
    SIGCONT_act.sa_handler = SIGCONT_handler;
    sigaction(SIGCONT, &SIGCONT_act, NULL);
}


volatile sig_atomic_t sigint_received = 0;

static void SIGINT_handler(int not_used)
{
    (void)not_used;

    sigint_received = 1;
}


void setup_sigint_handler(void)
{
    struct sigaction SIGINT_act;

    /* set up child signal handler first. */
    memset(&SIGINT_act, 0, sizeof(SIGINT_act));
    SIGINT_act.sa_handler = SIGINT_handler;
    sigaction(SIGINT, &SIGINT_act, NULL);
}

<<<<<<< HEAD


/* socket handling */
#define PORT    44818 /* Port to listen on */
#define BACKLOG     10  /* Passed to listen() */

int init_socket(int *sock)
{
    int reuseaddr = 1;
    struct sockaddr_in listen_addr;

    /* open a TCP socket using IPv4 protocol */
    *sock = socket (PF_INET, SOCK_STREAM, 0);
    if (*sock == -1) {
        info("socket() failed errno=%d.\n", errno);
        return PLCTAG_ERR_OPEN;
    }

    /* Enable the socket to reuse the address */
    if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
        info("setsockopt() failed! errno=%d", errno);
        return PLCTAG_ERR_OPEN;
    }

    /* describe where we want to listen: on any network. */
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(PORT);
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(*sock, (struct sockaddr *) &listen_addr, sizeof (listen_addr)) < 0) {
        info("bind() failed! errno=%d", errno);
        return PLCTAG_ERR_OPEN;
    }

    /* Listen */
    if (listen(*sock, BACKLOG) == -1) {
        info("listen() failed! errno=%d", errno);
        return PLCTAG_ERR_OPEN;
    }

    return PLCTAG_STATUS_OK;
}

/*
 * Packets are specified with a string in hex with ?? characters in
 * the following format:
 *
 *   af 01 bc ?? ?? 02 69 ...
 *
 * Hex digits are matched against the incoming bytes in the socket.
 *
 * Any entry that is '**' is copied into the passed buffer and not matched.
 *
 * Any entry that is '??' is ignored.
 *
 * Patterns must be compiled before use.
 *
 * The socket is read first with MSG_PEEK. If there is a match of all bytes
 * that are not copied, and the number of bytes to be copied is exactly the size
 * of the passed buffer, then PLCTAG_STATUS_OK is returned and the message is
 * fully read from the socket to remove the data.
 *
 * If the packet is not matched, then the message is not read and PLCTAG_ERR_NOT_FOUND
 * is returned.
 *
 * If a socket error occurs, PLCTAG_ERR_READ is returned.
 */

#define BUF_SIZE (1024)

typedef enum {
    BYTE_MATCH,
    BYTE_COPY,
    BYTE_IGNORE
} byte_mask_t;



int match_packet(int sock, compiled_pattern_s *pattern, uint8_t *copy_buf, int copy_buf_size)
{
    int rc = PLCTAG_STATUS_OK;
    uint8_t buf[BUF_SIZE];
    ssize_t bytes_read = 0;
    int bytes_copied = 0;

    /* get the bytes. */
    bytes_read = recv(sock, buf, BUF_SIZE, MSG_PEEK);
    if(bytes_read < 0) {
        /* error reading! */
        info("Unable to read socket, errno=%d", errno);
        return PLCTAG_ERR_READ;
    }

    /* if there is no match on the length then we did not match at all. */
    if(bytes_read >= pattern->packet_size) {
        return PLCTAG_ERR_NOT_FOUND;
    }

    /* we have at least the right number of bytes. */
    for(int i=0; i < pattern->packet_size; i++) {
        switch(pattern->match_type[i]) {
            case BYTE_MATCH:
                if(buf[i] != pattern->match_bytes[i]) {
                    /* no match */
                    return PLCTAG_ERR_NOT_FOUND;
                }
                break;

            case BYTE_COPY:
                if(bytes_copied > copy_buf_size) {
                    info("Attempt to copy more bytes than allowed, %d.", copy_buf_size);
                    return PLCTAG_ERR_TOO_LARGE;
                }

                copy_buf[bytes_copied] = buf[i];
                bytes_copied++;
                break;

            case BYTE_IGNORE:
                /* nothing to do */
                break;
        }
    }

    /* we matched, now pull the bytes out of the socket buffer. */
    bytes_read = recv(sock, buf, pattern->packet_size, 0);
    if(bytes_read < 0) {
        /* error reading! */
        info("Unable to read matched bytes out of the socket, errno=%d", errno);
        return PLCTAG_ERR_READ;
    }

    return rc;
}


int compile_pattern(const char *pattern, compiled_pattern_s **compiled_pattern)
{
    uint8_t match_bytes[BUF_SIZE];
    uint8_t match_type[BUF_SIZE];
    char hex_digits[3];
    int digit_index = 0;
    int copy_index = 0;
    int ignore_index = 0;
    int pattern_index = 0;
    int byte_index;

    while(pattern && pattern[pattern_index]) {
        /* handle match sections */
        if(isxdigit(pattern[pattern_index])) {
            hex_digits[digit_index] = pattern[pattern_index];
            digit_index++;
        }

        if(digit_index >= 2) {
            int val = 0;

            hex_digits[2] = (char)0;
            digit_index = 0;

            if(sscanf(hex_digits,"%x", &val) != 2) {
                info("Error parsing hex digits at position %d in pattern!", pattern_index);
                return PLCTAG_ERR_BAD_PARAM;
            }

            match_bytes[byte_index] = (uint8_t)val;
            match_type[byte_index] = BYTE_MATCH;
            byte_index++;
        }

        /* handle copy sections */
        if(pattern[pattern_index] == '*') {
            copy_index++;
        }

        if(copy_index >= 2) {
            copy_index = 0;
            match_bytes[byte_index] = 0;
            match_type[byte_index] = BYTE_COPY;
            byte_index++;
        }

        /* handle ignore sections */
        if(pattern[pattern_index] == '?') {
            ignore_index++;
        }

        if(ignore_index >= 2) {
            ignore_index = 0;
            match_bytes[byte_index] = 0;
            match_type[byte_index] = BYTE_IGNORE;
            byte_index++;
        }


        if(byte_index > BUF_SIZE) {
            info("Pattern compiles to size larger than buffer!");
            return PLCTAG_ERR_TOO_LARGE;
        }

        pattern_index++;
    }

    *compiled_pattern = (compiled_pattern_s *)calloc(1, sizeof(*compiled_pattern) + (byte_index * 2));
    if(! *compiled_pattern) {
        error("Unable to allocate memory for a compiled pattern!");
    }

    (*compiled_pattern)->packet_size = byte_index;
    (*compiled_pattern)->match_bytes = (uint8_t*)(*compiled_pattern + 1);
    (*compiled_pattern)->match_type = (uint8_t*)(*compiled_pattern + 1) + byte_index;

    memcpy((*compiled_pattern)->match_bytes, match_bytes, byte_index);
    memcpy((*compiled_pattern)->match_type, match_type, byte_index);

    return byte_index;
}
=======
>>>>>>> Adding test harness.
