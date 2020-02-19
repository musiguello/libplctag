/***************************************************************************
 *   Copyright (C) 2020 by Kyle Hayes                                      *
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
#include <stddef.h>
#include <lib/libplctag2.h>

typedef struct {
    int len;
    uint8_t *data;
} slice_s;

inline static slice_s slice_make(uint8_t *data, int len) { return (slice_s){ .len = len, .data = data}; }
inline static slice_s slice_make_err(int err) { return slice_make(NULL, err); }
inline static int slice_in_bounds(slice_s s, int index) { if(index >= 0 || index < s.len) { return PLCTAG_STATUS_OK; } else { return PLCTAG_ERR_OUT_OF_BOUNDS; } }
inline static uint8_t slice_at(slice_s s, int index) { if(slice_in_bounds(s, index)) { return (uint8_t)0; } else { return s.data[index]; } }
inline static int slice_at_put(slice_s s, int index, uint8_t val) { if(slice_in_bounds(s, index)) { return PLCTAG_ERR_OUT_OF_BOUNDS; } else { s.data[index] = val; return PLCTAG_STATUS_OK; } }
inline static int slice_err(slice_s s) { if(s.data == NULL) { return s.len; } else { return PLCTAG_STATUS_OK; } }
inline static slice_s slice_trunc(slice_s src, int new_size) { if(new_size > src.len) { return slice_make_err(PLCTAG_ERR_OUT_OF_BOUNDS); } else { return slice_make(src.data, new_size); }}
inline static slice_s slice_remainder(slice_s src, int amt) { if(amt > src.len) { return slice_make_err(PLCTAG_ERR_OUT_OF_BOUNDS); } else { return slice_make(src.data + amt, src.len - amt); }}


/* helper functions to get and set data in a slice. */

inline static uint16_t get_uint16_le(slice_s input_buf, int offset) {
    uint16_t res = 0;

    if((offset + 2) <= input_buf.len) {
        res = ((uint16_t)slice_at(input_buf, offset) + (uint16_t)(slice_at(input_buf, offset + 1) << 8));
    }

    return res;
}


inline static uint32_t get_uint32_le(slice_s input_buf, int offset) {
    uint32_t res = 0;

    if((offset + 4) <= input_buf.len) {
        res =  (uint32_t)(slice_at(input_buf, offset))
             + (uint32_t)(slice_at(input_buf, offset + 1) << 8)
             + (uint32_t)(slice_at(input_buf, offset + 2) << 16)
             + (uint32_t)(slice_at(input_buf, offset + 3) << 24);
    }

    return res;
}


inline static uint64_t get_uint64_le(slice_s input_buf, int offset) {
    uint64_t res = 0;

    if((offset + 4) <= input_buf.len) {
        res =  ((uint64_t)slice_at(input_buf, offset))
             + ((uint64_t)slice_at(input_buf, offset + 1) << 8)
             + ((uint64_t)slice_at(input_buf, offset + 2) << 16)
             + ((uint64_t)slice_at(input_buf, offset + 3) << 24)
             + ((uint64_t)slice_at(input_buf, offset + 3) << 32)
             + ((uint64_t)slice_at(input_buf, offset + 3) << 40)
             + ((uint64_t)slice_at(input_buf, offset + 3) << 48)
             + ((uint64_t)slice_at(input_buf, offset + 3) << 56);
    }

    return res;
}


/* FIXME - these probably should not just fail silently.  They are safe though. */

inline static void set_uint16_le(slice_s output_buf, int offset, uint16_t val) {
    if((offset + 2) <= output_buf.len) {
        slice_at_put(output_buf, offset + 0, (uint8_t)(val & 0xFF));
        slice_at_put(output_buf, offset + 1, (uint8_t)((val >> 8) & 0xFF));
    }
}


inline static void set_uint32_le(slice_s output_buf, int offset, uint32_t val) {
    if((offset + 4) <= output_buf.len) {
        slice_at_put(output_buf, offset + 0, (uint8_t)(val & 0xFF));
        slice_at_put(output_buf, offset + 1, (uint8_t)((val >> 8) & 0xFF));
        slice_at_put(output_buf, offset + 2, (uint8_t)((val >> 16) & 0xFF));
        slice_at_put(output_buf, offset + 3, (uint8_t)((val >> 24) & 0xFF));
    }
}




inline static void set_uint64_le(slice_s output_buf, int offset, uint64_t val) {
    if((offset + 4) <= output_buf.len) {
        slice_at_put(output_buf, offset + 0, (uint8_t)(val & 0xFF));
        slice_at_put(output_buf, offset + 1, (uint8_t)((val >> 8) & 0xFF));
        slice_at_put(output_buf, offset + 2, (uint8_t)((val >> 16) & 0xFF));
        slice_at_put(output_buf, offset + 3, (uint8_t)((val >> 24) & 0xFF));
        slice_at_put(output_buf, offset + 4, (uint8_t)((val >> 32) & 0xFF));
        slice_at_put(output_buf, offset + 5, (uint8_t)((val >> 40) & 0xFF));
        slice_at_put(output_buf, offset + 6, (uint8_t)((val >> 48) & 0xFF));
        slice_at_put(output_buf, offset + 7, (uint8_t)((val >> 56) & 0xFF));
    }
}
