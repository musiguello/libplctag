
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

#include <tests/simulator/ab.h>
#include <tests/simulator/slice.h>

#define CIP_CMD_FORWARD_CLOSE        ((uint8_t)0x4E)
#define CIP_CMD_FORWARD_OPEN         ((uint8_t)0x54)
#define CIP_CMD_FORWARD_OPEN_EX      ((uint8_t)0x5B)

typedef struct {
    /* CM Service Request - Connection Manager */
    uint8_t cm_service_code;        /* 0x54 Forward Open Request, 0x5B FO Extended. */
    uint8_t cm_req_path_size;       /* size in words of path, next field */
    slice_s cm_req_path;            /* Connection manager path: 0x20,0x06,0x24,0x01 for CM, instance 1*/

    /* Forward Open Params */
    uint8_t secs_per_tick;          /* seconds per tick */
    uint8_t timeout_ticks;          /* timeout = srd_secs_per_tick * src_timeout_ticks */
    uint32_t orig_to_targ_conn_id;  /* 0, returned by target in reply. */
    uint32_t targ_to_orig_conn_id;  /* what is _our_ ID for this connection, use ab_connection ptr as id ? */
    uint16_t conn_serial_number;    /* our connection ID/serial number */
    uint16_t orig_vendor_id;        /* our unique vendor ID */
    uint32_t orig_serial_number;    /* our unique serial number */
    uint8_t conn_timeout_multiplier;/* timeout = mult * RPI */
    uint8_t reserved[3];            /* reserved, set to 0 */
    uint32_t orig_to_targ_rpi;      /* us to target RPI - Request Packet Interval in microseconds */
    uint32_t orig_to_targ_conn_params; /* some sort of identifier of what kind of PLC we are??? */
    uint32_t targ_to_orig_rpi;      /* target to us RPI, in microseconds */
    uint32_t targ_to_orig_conn_params; /* some sort of identifier of what kind of PLC the target is ??? */
    uint8_t transport_class;        /* ALWAYS 0xA3, server transport, class 3, application trigger */
    uint8_t path_size;              /* size of connection path in 16-bit words
                                     * connection path from MSG instruction.
                                     *
                                     * EG LGX with 1756-ENBT and CPU in slot 0 would be:
                                     * 0x01 - backplane port of 1756-ENBT
                                     * 0x00 - slot 0 for CPU
                                     * 0x20 - class
                                     * 0x02 - MR Message Router
                                     * 0x24 - instance
                                     * 0x01 - instance #1.
                                     */
    slice_s path;
} forward_open_request_s;

#define FORWARD_OPEN_REQ_SIZE_BASE      (38)   /* need to add lengths of paths too. */
#define FORWARD_OPEN_REQ_EX_SIZE_BASE   (42)   /* need to add lengths of paths too. */


typedef struct {
    uint8_t resp_service_code;      /* returned as 0xD4 or 0xDB */
    uint8_t reserved1;               /* returned as 0x00? */
    uint8_t general_status;         /* 0 on success */
    uint8_t status_size;            /* number of 16-bit words of extra status, 0 if success */
    uint32_t orig_to_targ_conn_id;  /* target's connection ID for us, save this. */
    uint32_t targ_to_orig_conn_id;  /* our connection ID back for reference */
    uint16_t conn_serial_number;    /* our connection ID/serial number from request */
    uint16_t orig_vendor_id;        /* our unique vendor ID from request*/
    uint32_t orig_serial_number;    /* our unique serial number from request*/
    uint32_t orig_to_targ_api;      /* Actual packet interval, microsecs */
    uint32_t targ_to_orig_api;      /* Actual packet interval, microsecs */
    uint8_t app_data_size;          /* size in 16-bit words of send_data at end */
    uint8_t reserved2;
} forward_open_response_s;

#define FORWARD_OPEN_RESP_SIZE (30)

extern slice_s handle_forward_open_request(context_s *context, slice_s raw_request);

