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
#include "tests/regression/util.h"
#include "tests/regression/logix.h"
#include "lib/libplctag.h"


int logix_emulator(pid_t parent_pid)
{
    /* first set the SIGINT flag before we set up the signal handler. */
    sigint_received = 0;

    setup_sigint_handler();

    /* let the parent process know that we are ready */
    kill(parent_pid, SIGCONT);

    /* run until done. */
    while(!sigint_received) {
        util_sleep_ms(10);
    }

    return PLCTAG_STATUS_OK;
}
