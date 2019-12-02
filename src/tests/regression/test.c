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

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include "lib/libplctag.h"
#include "tests/regression/logix.h"
#include "tests/regression/util.h"

#define TIMEOUT (1000)

typedef void (*plc_emulator)(int sock);

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



int main(int argc, char **argv)
{
    int test_count = 0;
    int test_count_pass = 0;
    int test_count_fail = 0;

    (void)argc;
    (void)argv;

    /* set up child signal handler first. */
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
    int32_t tag = PLCTAG_ERR_CREATE;
    pid_t parent_pid = getpid();
    pid_t emulator_pid = 0;

    do {
        /* do this first to make sure that we do not have the flag set. */
        sigcont_received = 0;

        /* fork off child */
        emulator_pid = fork();
        if(emulator_pid < 0) {
            /* wat? this is fatal as it means something is really wrong. */
            error("Unable to fork emulator process!");
        } else if (emulator_pid == 0) {
            int client_sock = 0;
            int listen_sock = 0;
            socklen_t size = sizeof(struct sockaddr_in);
            struct sockaddr_in client_addr;

            /* boilerplate for all child emulator processes. */

            /* first set the SIGINT flag before we set up the signal handler. */
            sigint_received = 0;
            setup_sigint_handler();

            /* set up socket. */
            rc = init_socket(&listen_sock);
            if(rc != PLCTAG_STATUS_OK) {
                info("Unable to open socket!");

                /* let the parent process know that we are ready */
                kill(parent_pid, SIGCONT);

                /* run until done. */
                while(!sigint_received) {
                    util_sleep_ms(10);
                }

                exit(1);
            }

            /* let the parent process know that we are ready */
            kill(parent_pid, SIGCONT);

            /*
             * Accept a client connection.  We only do one at a time.
             * Get the client information.
             */
            memset(&client_addr, 0, sizeof client_addr);
            client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &size);
            if (client_sock == -1) {
                info("accept() failed! errno=%d\n", errno);

                /* close the listening */
                if(listen_sock > 0) {
                    close(listen_sock);
                }

                /* let the parent process know that we are ready */
                kill(parent_pid, SIGCONT);

                /* run until done. */
                while(!sigint_received) {
                    util_sleep_ms(10);
                }

                exit(1);
            }

            /* run the main loop for the emulator. */
            while(!sigint_received) {
                test->emulator(client_sock);
                util_sleep_ms(1);
            }

            /* close the sockets now that we are done. */
            close(client_sock);
            close(listen_sock);
        } else {
            /*
             * parent test process.
             *
             * Wait for the child to be ready.
             */
            while(!sigcont_received) {
                util_sleep_ms(10);
            }

            /* from the parent side, run the test. */

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
        }
    } while(0);

    /* clean up the tag if needed. */
    if(tag > 0) {
        plc_tag_destroy(tag);
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

