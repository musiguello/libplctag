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
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "lib/libplctag.h"
#include "tests/regression/logix.h"
#include "tests/regression/util.h"

#define TIMEOUT (1000)
#define BUF_SIZE (1024)

typedef int (*plc_emulator)(uint8_t *buf, int data_size);

typedef struct {
    const char *test_name;
    const plc_emulator emulator;
    const char *tag_string;
    const int result;
    const int tag_size;
} test_entry;


static test_entry tests[] = {
    {
        .test_name = "Test Logix DINT tag read.  Expect PLCTAG_STATUS_OK.",
        .emulator = (plc_emulator)logix_emulator,
        .tag_string = "protocol=ab-eip&gateway=127.0.0.1&path=1,5&cpu=lgx&elem_size=4&elem_count=3&name=TestDINTArray",
        .result = PLCTAG_STATUS_OK,
        .tag_size = 12
    },
    { .test_name = NULL, .tag_string = NULL, .result = PLCTAG_ERR_UNSUPPORTED }
};

static int run_test(test_entry *test);
static void run_emulator(pid_t parent_pid, test_entry *test);
static int test_tag(test_entry *test);
static void error_cleanup(int listen_sock, int client_sock);


int main(int argc, char **argv)
{
    int test_count = 0;
    int test_count_pass = 0;
    int test_count_fail = 0;

    (void)argc;
    (void)argv;

    /* set up child signal handler first. */
    sigcont_received = 0;
    setup_sigcont_handler();

    /* run the tests. */

    test_entry *test = tests;
    while(test && test->test_name) {
        test_count++;

        if(run_test(test) == PLCTAG_STATUS_OK) {
            test_count_pass++;
        } else {
            test_count_fail++;
        }

        test++;
    }

    info("Total tests: %d", test_count);
    info("\tPassing: %d", test_count_pass);
    info("\tFailing: %d", test_count_fail);

    return 0;
}



int run_test(test_entry *test)
{
    int rc = PLCTAG_STATUS_OK;
    pid_t parent_pid = getpid();
    pid_t emulator_pid = 0;

    /* fork off child to run the test. */
    emulator_pid = fork();
    if(emulator_pid < 0) {
        /* wat? this is fatal as it means something is really wrong. */
        error("Unable to fork emulator process!");
    } else if (emulator_pid == 0) {
        /* child process, this will run the emulator. */
        /* first set the SIGINT flag before we set up the signal handler. */
        sigint_received = 0;
        setup_sigint_handler();

        run_emulator(parent_pid, test);
    } else {
        /* parent process.  This will run the tag test. */
        while(!sigcont_received) {
            util_sleep_ms(10);
        }

        /* from the parent side, run the test. */
        rc = test_tag(test);
    }

    /* kill the child process if there is one. */
    if(emulator_pid > 0) {
        int status = 0;

        /* signal the child emulator process to terminate. */
        kill(emulator_pid, SIGINT);

        /* wait for it to die. */
        waitpid(emulator_pid, &status, 0);
    }

    return rc;
}


void run_emulator(pid_t parent_pid, test_entry *test)
{
    int rc = PLCTAG_STATUS_OK;
    int client_sock = 0;
    int listen_sock = 0;
    socklen_t size = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;

    /* boilerplate for all child emulator processes. */

    /* set up socket. */
    rc = init_socket(&listen_sock);
    if(rc != PLCTAG_STATUS_OK) {
        info("Unable to open socket!");

        /* let the parent process know that we are ready */
        kill(parent_pid, SIGCONT);

        error_cleanup(listen_sock, client_sock);
    }

    /* let the parent process know that we are ready */
    kill(parent_pid, SIGCONT);

    /*
     * Accept a client connection.  We only do one at a time.
     * Get the client information.
     */
    memset(&client_addr, 0, sizeof client_addr);
    client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &size);
    if (client_sock < 0) {
        info("accept() failed! errno=%d\n", errno);

        error_cleanup(listen_sock, client_sock);
    }

    /* run the main loop for the emulator. */
    while(!sigint_received) {
        uint8_t buf[BUF_SIZE];
        int bytes_read = 0;
        int bytes_processed = 0;

        /* peek at the socket buffer. */
        bytes_read = (int)recv(client_sock,buf, BUF_SIZE, MSG_PEEK);
        if(bytes_read < 0) {
            /* error reading! */
            info("Unable to read socket, errno=%d", errno);

            error_cleanup(listen_sock, client_sock);
        }

        bytes_processed = test->emulator(buf, bytes_read);
        if(bytes_processed > 0) {
            /* we matched, now pull the bytes out of the socket buffer. */
            bytes_read = (int)recv(client_sock, buf, (size_t)bytes_processed, 0);
            if(bytes_read < 0) {
                /* error reading! */
                info("Unable to read matched bytes out of the socket, errno=%d", errno);

                error_cleanup(listen_sock, client_sock);
            }
        }

        util_sleep_ms(1);
    }

    /* close the sockets now that we are done. */
    close(client_sock);
    close(listen_sock);
}


int test_tag(test_entry *test)
{
    int rc = PLCTAG_STATUS_OK;
    int32_t tag = 0;

    do {
        /* create the tag for the test. */
        tag = plc_tag_create(test->tag_string, TIMEOUT);
        if(tag < 0) {
            info("\tUnable to create tag!  Status: %s", plc_tag_decode_error(tag));
            rc = PLCTAG_ERR_CREATE;
            break;
        }

        rc = plc_tag_read(tag, TIMEOUT);
        if(rc != test->result) {
            info("\tRead failed with status %s.", plc_tag_decode_error(rc));
            rc = PLCTAG_ERR_READ;
            break;
        }

        /* only check the size if the read succeeded. */
        if(rc == PLCTAG_STATUS_OK) {
            rc = plc_tag_get_size(tag);
            if(rc != test->tag_size) {
                info("\tTag size does not match! Tag is %d bytes but expected %d bytes.", rc, test->tag_size);
                rc = PLCTAG_ERR_NO_MATCH;
                break;
            }
        }
    } while(0);

    /* clean up the tag if needed. */
    if(tag > 0) {
        plc_tag_destroy(tag);
    }

    return rc;
}



void error_cleanup(int listen_sock, int client_sock)
{
    /* close the listening */
    if(listen_sock > 0) {
        close(listen_sock);
    }

    if(client_sock > 0) {
        close(client_sock);
    }

    /* wait until done. */
    while(!sigint_received) {
        util_sleep_ms(10);
    }

    exit(1);
}
