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
#include <string.h>
#include <tests/simulator/eip.h>
#include <tests/simulator/register_session.h>
#include <tests/simulator/utils.h>

#define SESSION_REQ_SIZE (4)


slice_s handle_register_session(context_s *context, slice_s raw_request)
{
    eip_packet_s eip_packet;
    uint16_t req_version;
    uint16_t req_option_flags;
    slice_s result;
    slice_s header;
    slice_s tmp_output;

    /* there is not much to do here for validation.  Just do it inline. */
    /* check the payload contents. */
    if(raw_request.len != SESSION_REQ_SIZE) {
        info("handle_register_session(): Session request malformed.  Payload should be 4 bytes but was %d bytes!", (int)raw_request.len);
        return slice_make_err(PLCTAG_ERR_BAD_DATA);
    }

    req_version = get_uint16_le(raw_request, 0);
    req_option_flags = get_uint16_le(raw_request, 2);

    /* requested version must be one. */
    if(req_version != 1) {
        info("handle_register_session(): Register session request version must be one but was %d!", (int)req_version);
        return slice_make_err(PLCTAG_ERR_BAD_PARAM);
    }

    /* request option flags must be zero. */
    if(req_option_flags != 0) {
        info("handle_register_session(): Register session request option flags must be zero but was %x!", (int)req_option_flags);
        return slice_make_err(PLCTAG_ERR_BAD_PARAM);
    }

    /* carry on with making our response. */

    info("handle_register_session() validated incoming session request.  Now building response packet.");

    /* we will set up the outgoing packet.  Set default values. */
    memset(&eip_packet, 0, sizeof eip_packet);

    eip_packet.command = EIP_REGISTER_SESSION;
    eip_packet.length = SESSION_REQ_SIZE;

    /* we need a session handle.   Make a random value.  Store it for later. */
    context->session_handle = (uint32_t)rand();
    eip_packet.session_handle = context->session_handle;

    /* marshal the EIP header. */
    header = marshall_eip_response(context, context->buffer, &eip_packet);
    if(slice_err(header) != PLCTAG_STATUS_OK) {
        info("handle_register_session(): Error, %s, marshalling EIP header!", plc_tag_decode_error(slice_err(header)));
        return header;
    }

    /* header contains the part of the output buffer that has the EIP header in it. */
    tmp_output = slice_remainder(context->buffer, header.len);

    /* FIXME - should check to make sure that there is buffer space! */

    /* marshall our fields. */
    set_uint16_le(tmp_output, 0, req_version);
    set_uint16_le(tmp_output, 2, req_option_flags);

    /* build a packet slice for output. */
    result = slice_trunc(context->buffer, header.len + SESSION_REQ_SIZE);

    return result;
}


