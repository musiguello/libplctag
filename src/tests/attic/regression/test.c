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
#include "lib/libplctag2.h"
#include "tests/regression/logix.h"
#include "tests/regression/packet.h"
#include "tests/regression/util.h"

#define TIMEOUT (1000)
#define BUF_SIZE (100)

typedef slice_s (*plc_emulator)(slice_s inbut, slice_s outbuf);

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
        .tag_string = "protocol=ab-eip&gateway=127.0.0.1&path=1,5&cpu=lgx&elem_size=4&elem_count=3&name=TestDINTArray&debug=4",
        .result = PLCTAG_STATUS_OK,
        .tag_size = 12
    },
    { .test_name = NULL, .tag_string = NULL, .result = PLCTAG_ERR_UNSUPPORTED }
};

static int run_test(test_entry *test);
static int run_emulator(pid_t parent_pid, test_entry *test);
static pid_t startup_emulator(pid_t parent_pid, test_entry *test);
static pid_t startup_tag_test(pid_t parent_pid, test_entry *test);
static int run_tag_test(pid_t parent_pid, test_entry *test);
//static void error_cleanup(int listen_sock, int client_sock);
static slice_s peek_data(int client_sock, slice_s input_buf);
static slice_s get_packet(slice_s possible_packet);
static int send_data(int client_sock, slice_s output_packet);


int main(int argc, char **argv)
{
    int test_count = 0;
    int test_count_pass = 0;
    int test_count_fail = 0;

    (void)argc;
    (void)argv;

    /* set up child signal handler first. */
    sigint_received = 0;
    setup_sigint_handler();

    /* run the tests. */
    test_entry *test = tests;
    while(test && test->test_name) {
        int rc = PLCTAG_STATUS_OK;

        test_count++;

        rc = run_test(test);
        if(rc == test->result) {
            info("Test #%d %s: Succeeded.", test_count, test->test_name);
            test_count_pass++;
        } else {
            info("Test #%d %s: Failed: %s (%d)!", test_count, test->test_name, plc_tag_decode_error(rc), rc);
            test_count_fail++;
        }

        test++;
    }

    info("Total tests: %d", test_count);
    info("\tPassing: %d", test_count_pass);
    info("\tFailing: %d", test_count_fail);

    /* only success if there are no failures. */
    return test_count_fail;
}


/*
 * Set up two processes to run the different parts of the test.
 * One runs the emulator.  One runs the tag test.
 *
 * We use processes in order to isolate memory corruption problems
 * and other crash bugs.
 */

int run_test(test_entry *test)
{
    pid_t parent_pid = getpid();
    pid_t emulator_pid = 0;
    pid_t tag_test_pid = 0;
    int emulator_status = PLCTAG_STATUS_OK;
    int tag_test_status = PLCTAG_STATUS_OK;

    /* clear the signal flag before we fork. */
    sigint_received = 0;

    /* start up the emulator first.  This waits for the emulator to be ready. */
    info("Starting up emulator process.");
    emulator_pid = startup_emulator(parent_pid, test);
    if(emulator_pid < 0) {
        /* whoops! */
        error("Unable to start up emulator process!");
    }

    /* wait for the emulator to spin up. */
    info("Waiting for emulator to start up.");
    while(!sigint_received) {
        util_sleep_ms(10);
    }

    /*
     * FIXME - this could be a race condition.  The emulator
     * process could fire this before we set it here.
     *
     * However, even if we miss another SIGINT from the emulator,
     * we will eventually time out in the tag test process.  So this
     * will terminate eventually.
     */
    sigint_received = 0;

    info("Starting up tag test process.");
    tag_test_pid = startup_tag_test(parent_pid, test);
    if(tag_test_pid < 0) {
        /* also bad. */
        error("Unable to start up tag test process!");
    }

    /*
     * Wait until the test completes.
     *
     * This could be either the tag test completing or the
     * emulator crashing.
     */
    info("Waiting for test to complete.");
    while(!sigint_received) {
        util_sleep_ms(10);
    }

    info("Shutting down test.");
    /* terminate the processes.  Just in case. */
    kill(emulator_pid, SIGINT);
    kill(tag_test_pid, SIGINT);

    /* Get the final status from each process. */
    waitpid(emulator_pid, &emulator_status, 0);
    waitpid(tag_test_pid, &tag_test_status, 0);

    if(emulator_status != PLCTAG_STATUS_OK) {
        info("Emulator quit with status %s (%d)!", plc_tag_decode_error(emulator_status), emulator_status);
    }

    return tag_test_status;
}


pid_t startup_emulator(pid_t parent_pid, test_entry *test)
{
    pid_t emulator_pid = 0;

    emulator_pid = fork();
    if(emulator_pid == 0) {
        /* child process, this will run the emulator. */

        /* first set the SIGINT flag before we set up the signal handler. */
        sigint_received = 0;
        setup_sigint_handler();

        /* run the emulator until we are signaled otherwise */
        exit(run_emulator(parent_pid, test));
    } else if(emulator_pid < 0) {
        /* wat? this is fatal as it means something is really wrong. */
        error("Unable to fork emulator process!");
    } /* parent process. */

    return emulator_pid;
}


pid_t startup_tag_test(pid_t parent_pid, test_entry *test)
{
    pid_t tag_test_pid = 0;

    tag_test_pid = fork();
    if(tag_test_pid == 0) {
        /* child process, this will run the emulator. */

        /* first set the SIGINT flag before we set up the signal handler. */
        sigint_received = 0;
        setup_sigint_handler();

        /* run the emulator until we are signaled otherwise */
        exit(run_tag_test(parent_pid, test));
    } else if(tag_test_pid < 0) {
        /* wat? this is fatal as it means something is really wrong. */
        info("Unable to fork tag test process!");
    } /* parent process. */

    return tag_test_pid;
}



/*
 * boilerplate for all emulator processes.  This opens a
 * listening socket, waits for a connection and then
 * calls the emulator code with whatever we read from the socket.
 */

int run_emulator(pid_t parent_pid, test_entry *test)
{
    int rc = PLCTAG_STATUS_OK;
    int listen_sock = 0;
    int client_sock = 0;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = 0;
    uint8_t buf_bytes[BUF_SIZE];
    slice_s buf = slice_make(buf_bytes, sizeof(buf_bytes));

    /* fake exceptions */
    do {
        /* open the listening socket. */
        rc = init_socket(&listen_sock);
        if(rc != PLCTAG_STATUS_OK) {
            if(listen_sock > 0) {
                close(listen_sock);
            }

            break;
        }

        /* send the parent a signal indicating that we are ready. */
        kill(parent_pid, SIGINT);

        /* wait for a client connection. */
        memset(&client_addr, 0, sizeof client_addr);
        client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock < 0) {
            info("accept() failed! errno=%d\n", errno);

            if(listen_sock > 0) {
                close(listen_sock);
            }

            rc = PLCTAG_ERR_BAD_CONNECTION;
            break;
        }

        while(rc == PLCTAG_STATUS_OK && !sigint_received) {
            slice_s possible_packet;
            slice_s input_packet;

            /* get data from the socket. */
            possible_packet = peek_data(client_sock, buf);
            if(slice_err(possible_packet)) {
                rc = slice_err(possible_packet);
                break;
            }

            info("possible_packet=");
            slice_dump(possible_packet);

            /* We might have more or less data than required, get exactly the packet data. */
            input_packet = get_packet(possible_packet);
            if(input_packet.len > 0) {
                info("input_packet=");
                slice_dump(input_packet);

                slice_s output_packet = test->emulator(input_packet, buf);

                /* is there something to output? */
                if(output_packet.len > 0) {
                    info("output_packet=");
                    slice_dump(output_packet);

                    rc = send_data(client_sock, output_packet);
                    if(rc) {
                        break;
                    }
                }

                /* purge data from the OS socket buffer. */
                info("Purging %d bytes from the socket buffer.", input_packet.len);
                rc = (int)recv(client_sock, input_packet.data, (size_t)input_packet.len, 0);
                if(rc != input_packet.len) {
                    info("Recv did not purge all buffered data! rc=%d", rc);
                    rc = PLCTAG_ERR_READ;
                    break;
                } else {
                    rc = PLCTAG_STATUS_OK;
                }
            }
        }
    } while(0);

    if(client_sock > 0) {
        close(client_sock);
    }

    if(listen_sock > 0) {
        close(listen_sock);
    }

    return rc;
}



int run_tag_test(pid_t parent_pid, test_entry *test)
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

    /* notify the parent process that we are done. */
    kill(parent_pid, SIGINT);

    return rc;
}



slice_s peek_data(int client_sock, slice_s input_buf)
{
        int bytes_transfered = (int)recv(client_sock, input_buf.data, (size_t)input_buf.len, MSG_PEEK);

        if(bytes_transfered < 0) {
            return slice_make_err(PLCTAG_ERR_READ);
        }

        return slice_make(input_buf.data, bytes_transfered);
}


slice_s get_packet(slice_s possible_packet)
{
    int packet_len = 0;

    /* AB packets start with a 24-byte header. */
    if(possible_packet.len < 24) {
        return slice_make_err(PLCTAG_ERR_TOO_SMALL);
    }

    /* FIXME - this should be factored out somewhere else. */

    /* get the packet length. */
    packet_len = slice_at(possible_packet, 2) + (slice_at(possible_packet, 3) << 8);

    if(possible_packet.len >= (packet_len + 24)) {
        /* there is a full packet here. */
        return slice_make(possible_packet.data, packet_len + 24);
    }

    return slice_make_err(PLCTAG_ERR_TOO_SMALL);
}


int send_data(int client_sock, slice_s output_packet)
{
    int bytes_to_send = output_packet.len;
    int bytes_sent = 0;
    int rc = 0;

    do {
        rc = (int)send(client_sock, output_packet.data + bytes_sent, (size_t)bytes_to_send, MSG_NOSIGNAL);
        if(rc < 0) {
            info("Error sending data to the client!  errno=%d", errno);
            return PLCTAG_ERR_WRITE;
        }

        bytes_sent += rc;
        bytes_to_send = output_packet.len - bytes_sent;
    } while(bytes_to_send > 0);

    return PLCTAG_STATUS_OK;
}












/* OLD */

//int run_test_old(test_entry *test)
//{
//    int rc = PLCTAG_STATUS_OK;
//    pid_t parent_pid = getpid();
//    pid_t emulator_pid = 0;
//    pid_t tag_test_pid = 0;
//
//    /* fork off child to run the test. */
//    emulator_pid = fork();
//    if(emulator_pid < 0) {
//        /* wat? this is fatal as it means something is really wrong. */
//        error("Unable to fork emulator process!");
//    } else if (emulator_pid == 0) {
//        /* child process, this will run the emulator. */
//        /* first set the SIGINT flag before we set up the signal handler. */
//        sigint_received = 0;
//        setup_sigint_handler();
//
//        run_emulator(parent_pid, test);
//    } else {
//        /* parent process.  This will run the tag test. */
//        while(!sigcont_received) {
//            util_sleep_ms(10);
//        }
//
//        /* from the parent side, run the test. */
//        rc = test_tag(test);
//    }
//
//    /* kill the child process if there is one. */
//    if(emulator_pid > 0) {
//        int status = 0;
//
//        /* signal the child emulator process to terminate. */
//        kill(emulator_pid, SIGINT);
//
//        /* wait for it to die. */
//        waitpid(emulator_pid, &status, 0);
//    }
//
//    return rc;
//}



