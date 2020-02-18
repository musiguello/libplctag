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


#include <stdlib.h>
#include <tests/simulator/handle_register_session.h>
#include <tests/simulator/utils.h>

#define SESSION_REQ_SIZE (28)


static int validate_register_session_request(eip_packet_s req);
static slice_s marshall_register_session_response(slice_s output_buf, uint32_t session_handle);



slice_s handle_register_session(context_s *context, eip_packet_s req)
{
    int rc = PLCTAG_STATUS_OK;
    slice_s result;

    rc = validate_register_session_request(req);
    if(rc != PLCTAG_STATUS_OK) {
        result = slice_make_err(rc);
    } else {
        context->session_handle = (uint32_t)rand();

        result = marshall_register_session_response(context->buffer, context->session_handle);
    }

    return result;
}

int validate_register_session_request(eip_packet_s req)
{
    uint16_t req_version;
    uint16_t req_option_flags;

    /* check each field of the header for validity. */
    if(req.command != EIP_REGISTER_SESSION) {
        info("Register session request has wrong command %x!", (int)req.command);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* packet length other than the header must be 4 bytes. */
    if(req.length != 4) {
        info("Register session request has wrong length parameter %d!", (int)req.length);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* now check the payload. */
    if(req.payload.len != 4) {
        info("Register session request has wrong payload length %d!", (int)req.length);
        return PLCTAG_ERR_BAD_DATA;
    }

    /* packet session handle must be zero. */
    if(req.session_handle != 0) {
        info("Register session request session handle must be zero, was %x!", (int)req.session_handle);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* packet status must be zero. */
    if(req.status != 0) {
        info("Register session request status must be zero, was %x!", (int)req.status);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* packet sender context must be zero. */
    if(req.sender_context != 0U) {
        info("Register session request session context must be zero!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* packet options must be zero. */
    if(req.options != 0) {
        info("Register session request header options must be zero!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* check the payload contents. */
    req_version = get_uint16_le(req.payload, 0);
    req_option_flags = get_uint16_le(req.payload, 2);

    /* requested version must be one. */
    if(req_version != 0) {
        info("Register session request version must be one!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* request option flags must be zero. */
    if(req_option_flags != 0) {
        info("Register session request option flags must be zero!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    return PLCTAG_STATUS_OK;
}



slice_s marshall_register_session_response(slice_s output_buf, uint32_t session_handle)
{
    /*  Marshal a session request response packet:
            uint16_t command;
            uint16_t length;
            uint32_t session_handle;
            uint32_t status;
            uint64_t sender_context;
            uint32_t options;

              payload
            uint16_t eip_version;
            uint16_t option_flags;
    */

    /* command */
    set_uint16_le(output_buf, 0, EIP_REGISTER_SESSION);

    /* packet length */
    set_uint16_le(output_buf, 2, 4); /* MAGIC - size of the payload. */

    /* session handle returned to the client. */
    set_uint32_le(output_buf, 4, session_handle);

    /* status is zero for success. */
    set_uint32_le(output_buf, 8, 0);

    /* session context is zero. */
    set_uint64_le(output_buf, 12, 0U);

    /* options are zero */
    set_uint32_le(output_buf, 20, 0);

    /* EIP version is one */
    set_uint16_le(output_buf, 24, 1);

    /* option flags are zero */
    set_uint16_le(output_buf, 26, 0);

    return slice_trunc(output_buf, SESSION_REQ_SIZE);
}
