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

