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

#pragma once

#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <stdlib.h>

extern int util_sleep_ms(int ms);
extern int64_t util_time_ms(void);
extern void error(const char *templ, ...);
extern void info(const char *templ, ...);

/* signal helpers. */
extern volatile sig_atomic_t sigcont_received;
extern void setup_sigcont_handler(void);

extern volatile sig_atomic_t sigint_received;
extern void setup_sigint_handler(void);

/* socket handling */
extern int init_socket(int *sock);

/* matching */

typedef struct {
    int packet_size;
    uint8_t *match_bytes;
    uint8_t *match_type;
} compiled_pattern_s;

extern int compile_pattern(const char *pattern, compiled_pattern_s **compiled_pattern);
extern int match_packet(int sock, compiled_pattern_s *pattern, uint8_t *copy_buf, int copy_buf_size);
