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

#pragma once

typedef struct {
    int len;
    uint8_t *data;
} slice_s;

static slice_s make_slice(uint8_t *data, int len) { return (slice_s){.len = len, .data = data}; }

/*
 * match the pattern against the bytes in the buffer.  The pattern
 * may have bytes that are skipped.   If the entire pattern matches,
 * then the length of the pattern, in bytes, is returned.
 *
 * If the match fails because there is insufficient data in the buffer,
 * then PLCTAG_ERR_TOO_SHORT is returned.
 *
 * If any match byte does not match the data in the buffer, then PLCTAG_ERR_NOT_FOUND is
 * returned.
 *
 * The patterns are strings of a form such as this:
 *
 * 6f 00 44 00 =1 =1 =1 =1 00 00
 * 00 00 >2 >2 >2 >2 >2 >2 >2 >2
 * 00 00 00 00 00 00 00 00 01 00
 * 02 00 00 00 00 00 b2 00 34 00
 * 5b 02 20 06 24 01 0a 05 00 00
 * 00 00 >4 >4 >4 >4 >5 >5 3d f3
 * 45 43 50 21 01 00 00 00 40 42
 * 0f 00 a2 0f 00 42 40 42 0f 00
 * a2 0f 00 42 a3 03 01 04 20 02
 * 24 01
 *
 * Elements that are two hex digits are used to match the corresponding byte in the buffer.
 * Elements that are not hex digits are skipped during a match.
 *
 */
extern int match_pattern(slice_s buf, const char *pattern);

/*
 * Take a pattern in the form above for match_pattern() and turn it into binary byte values
 * in the passed slice.
 *
 * If there is insufficient space in the slice, PLCTAG_ERR_TOO_LONG is returned.
 *
 * If all goes well, the return values is the number of bytes copied to the slice.
 *
 * Pattern elements that are not hex digits are converted to 0xFF bytes in the target slice.
 */
extern int copy_pattern(slice_s buf, const char *pattern);