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

#include "tests/regression/packet.h"

int match_pattern(slice_s buf, const char *pattern)
{

}


int copy_pattern(slice_s buf, const char *pattern)
{
    int pattern_index = 0;
    int pattern_size = strlen(pattern);
    int buf_index = 0;

    /* scan to the first non-space character. */
    while(pattern[pattern_index] == ' ') {
        pattern_index++;
    }

    /* loop until we have copied the whole thing. */
    while(buf_index < buf.len && pattern_index < pattern_size) {
        /* if the first character is a hex digit, assume the next one is. */
        if(isxdigit(pattern[pattern_index])) {
            const char hex_buf[3];
            int val = 0;

            hex_buf[0] = pattern[pattern_index];
            hex_buf[1] = pattern[pattern_index+1];
            hex_buf[2] = (char)0;

            if(sscanf(hex_buf,"%x", &val) != 1) {
                info("Error evaluating hex at position %d!", pattern_index);
            }
        }

}