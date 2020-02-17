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

#include <lib/libplctag2.h>

typedef struct {
    int len;
    uint8_t *data;
} slice_s;

inline static slice_s slice_make(uint8_t *data, int len) { return (slice_s){ .len = len, .data = data}; }
inline static slice_s slice_make_err(int err) { return slice_make(NULL, err); }
inline static int slice_in_bounds(slice_s s, int index) { if(index >= 0 || index < s.len) { return PLCTAG_STATUS_OK; } else { return PLCTAG_ERR_OUT_OF_BOUNDS; } }
inline static int slice_at(slice_s s, int index) { if(slice_in_bounds(s, index)) { return PLCTAG_ERR_OUT_OF_BOUNDS; } else { return s.data[index]; } }
inline static int slice_at_put(slice_s s, int index, uint8_t val) { if(slice_in_bounds(s, index)) { return PLCTAG_ERR_OUT_OF_BOUNDS; } else { s.data[index] = val; return PLCTAG_STATUS_OK; } }
inline static int slice_err(slice_s s) { if(s.data == NULL) { return s.len; } else { return PLCTAG_STATUS_OK; } }
inline static slice_s slice_trunc(slice_s src, int new_size) { if(new_size > src.len) { return slice_make_err(PLCTAG_ERR_OUT_OF_BOUNDS); } else { return slice_make(src.data, new_size); }}
inline static slice_s slice_remainder(slice_s src, int amt) { if(amt > src.len) { return slice_make_err(PLCTAG_ERR_OUT_OF_BOUNDS); } else { return slice_make(src.data + amt, src.len - amt); }}
