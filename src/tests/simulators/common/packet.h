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
#include <lib/libplctag2.h>
#include <tests/simulator/slice.h>

#define EIP_REGISTER_SESSION     ((uint16_t)0x0065)
#define EIP_UNREGISTER_SESSION   ((uint16_t)0x0066)
#define EIP_UNCONNECTED_DATA     ((uint16_t)0x006F)
#define EIP_CONNECTED_SEND       ((uint16_t)0x0070)


#define CIP_CMD_RESPONSE             ((uint8_t)0x80)
#define CIP_CMD_PCCC_EXECUTE         ((uint8_t)0x4B)
#define CIP_CMD_FORWARD_CLOSE        ((uint8_t)0x4E)
#define CIP_CMD_FORWARD_OPEN         ((uint8_t)0x54)
#define CIP_CMD_FORWARD_OPEN_EX      ((uint8_t)0x5B)
#define CIP_CMD_READ                 ((uint8_t)0x4C)
#define CIP_CMD_WRITE                ((uint8_t)0x4D)
#define CIP_CMD_READ_FRAG            ((uint8_t)0x52)
#define CIP_CMD_WRITE_FRAG           ((uint8_t)0x53)

#define CIP_CMD_OK                   ((uint8_t)0x80)

#define CIP_STATUS_OK               ((uint8_t)0)
#define CIP_STATUS_FRAG             ((uint8_t)0x06)

/* CPF Item Types */
#define CPF_ITEM_NAI ((uint16_t)0x0000) /* NULL Address Item */
#define CPF_ITEM_CAI ((uint16_t)0x00A1) /* connected address item */
#define CPF_ITEM_CDI ((uint16_t)0x00B1) /* connected data item */
#define CPF_ITEM_UDI ((uint16_t)0x00B2) /* Unconnected data item */

#define CIP_SYMBOLIC_SEGMENT  ((uint8_t)0x91)
#define CIP_NUMERIC_SEGMENT_ONE_BYTE  ((uint8_t)0x28)
#define CIP_NUMERIC_SEGMENT_TWO_BYTES  ((uint8_t)0x29)
#define CIP_NUMERIC_SEGMENT_FOUR_BYTES  ((uint8_t)0x2A)


inline static uint16_t get_uint16_le(slice_s input_buf, int offset) {
    uint16_t res = 0;

    if((offset + 2) <= input_buf.len) {
        res = ((uint16_t)slice_at(res_buf, offset) + (uint16_t)(slice_at(res_buf, offset + 1) << 8));
    }

    return res;
}


inline static uint32_t get_uint32_le(slice_s input_buf, int offset) {
    uint32_t res = 0;

    if((offset + 4) <= input_buf.len) {
        res =  (uint32_t)(slice_at(res_buf, offset))
             + (uint32_t)(slice_at(res_buf, offset + 1) << 8)
             + (uint32_t)(slice_at(res_buf, offset + 2) << 16)
             + (uint32_t)(slice_at(res_buf, offset + 3) << 24);
    }

    return res;
}


inline static uint64_t get_uint64_le(slice_s input_buf, int offset) {
    uint64_t res = 0;

    if((offset + 4) <= input_buf.len) {
        res =  ((uint64_t)slice_at(res_buf, offset))
             + ((uint64_t)slice_at(res_buf, offset + 1) << 8)
             + ((uint64_t)slice_at(res_buf, offset + 2) << 16)
             + ((uint64_t)slice_at(res_buf, offset + 3) << 24)
             + ((uint64_t)slice_at(res_buf, offset + 3) << 32)
             + ((uint64_t)slice_at(res_buf, offset + 3) << 40)
             + ((uint64_t)slice_at(res_buf, offset + 3) << 48)
             + ((uint64_t)slice_at(res_buf, offset + 3) << 56);
    }

    return res;
}




/* Common packet types */
typedef struct {
    uint16_t command;
    uint16_t length;
    uint32_t session_handle;
    int32_t status;
    uint64_t sender_context;
    uint32_t options;
    slice_s payload;
} eip_header;
#define EIP_HEADER_SIZE (24) /* 24 bytes */

/* CPF unconnected message. */
typedef struct {
    uint16_t item_count;
    uint16_t nai_item_type;
    uint16_t nai_item_length;
    uint16_t udi_item_type;
    uint16_t udi_item_length;
    slice_s payload;
} common_packet_header_unconn;


/* CPF connected message. */
typedef struct {
    uint16_t item_count;
    uint16_t cai_item_type;
    uint16_t cai_item_length;
    uint32_t targ_conn_id;
    uint16_t cdi_item_type;
    uint16_t cdi_item_length;
    uint16_t conn_seq_num; /* warning: counted in the length! */
    slice_s payload;
} common_packet_header_conn;


extern eip_header read_eip_packet(int sock_fd, slice_s input_buffer);
