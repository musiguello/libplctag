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
#include <tests/simulator/ab.h>
#include <tests/simulator/slice.h>

#define EIP_REGISTER_SESSION     ((uint16_t)0x0065)
#define EIP_UNREGISTER_SESSION   ((uint16_t)0x0066)
#define EIP_UNCONNECTED_DATA     ((uint16_t)0x006F)
#define EIP_CONNECTED_DATA       ((uint16_t)0x0070)

/* Common packet types */
typedef struct {
    uint16_t command;
    uint16_t length;
    uint32_t session_handle;
    int32_t status;
    uint64_t sender_context;
    uint32_t options;
    slice_s payload;
} eip_packet_s;
#define EIP_HEADER_SIZE (24) /* 24 bytes */

extern slice_s read_eip_packet(int sock, slice_s input_buf);
extern slice_s dispatch_eip_packet(context_s *context, slice_s raw_eip_request);
extern slice_s marshall_eip_response(context_s *context, slice_s output_buf, eip_packet_s *eip_packet);
