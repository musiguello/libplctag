
#pragma once

#include <lib/libplctag.h>

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
