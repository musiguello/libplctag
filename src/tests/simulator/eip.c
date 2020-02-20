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

#include <lib/libplctag2.h>
#include <tests/simulator/eip.h>
#include <tests/simulator/register_session.h>
#include <tests/simulator/socket.h>
#include <tests/simulator/unconnected_request.h>
#include <tests/simulator/utils.h>


static int unmarshal_and_validate_eip_request(context_s *context, slice_s raw_eip_request, eip_packet_s *eip_packet);


slice_s read_eip_packet(int sock, slice_s input_buf)
{
    int bytes_left_to_read = EIP_HEADER_SIZE;
    slice_s tmp_in_buf = input_buf;
    int bytes_read = 0;
    slice_s result = slice_make_err(PLCTAG_ERR_BAD_DATA);

    /* get at least 24 bytes. */
    do {
        tmp_in_buf = socket_read(sock, slice_trunc(tmp_in_buf, bytes_left_to_read));

        if(slice_err(tmp_in_buf)) {
            info("ERROR: Unable to read socket: %s.", plc_tag_decode_error(slice_err(tmp_in_buf)));
            return tmp_in_buf;
        }

        /* update the bytes read and recreate the input buffer */
        bytes_read += tmp_in_buf.len;
        tmp_in_buf = slice_remainder(input_buf, bytes_read);

        /* Do we have enough bytes for the payload length?  If so, process it. */
        if(bytes_read >= 4) {
            /* get the packet length, it is the second 16-bit word in the data. */
            uint16_t packet_len = get_uint16_le(input_buf, 2);
            bytes_left_to_read = (24 + packet_len) - bytes_read;
            info("read_eip_packet() got sufficient bytes, %d, to calculate real packet length, %d.", bytes_read, packet_len);
        }
    } while(bytes_left_to_read > 0);

    /* all good, build the result slice and return it. */
    result = slice_trunc(input_buf, bytes_read);

    info("read_eip_packet() got packet:");
    slice_dump(result);

    return result;
}



slice_s dispatch_eip_packet(context_s *context, slice_s raw_eip_packet)
{
    int rc;
    slice_s result;
    eip_packet_s eip_packet;

    rc = unmarshal_and_validate_eip_request(context, raw_eip_packet, &eip_packet);

    if(rc == PLCTAG_STATUS_OK) {
        switch(eip_packet.command) {
            case EIP_REGISTER_SESSION:
                result = handle_register_session(context, eip_packet.payload);
                break;

            case EIP_UNCONNECTED_DATA:
                result = handle_unconnected_request(context, eip_packet.payload);
                break;


            default:
                info("Unsupported EIP packet type %x!", eip_packet.command);
                result = slice_make_err(PLCTAG_ERR_UNSUPPORTED);
                break;
        }
    } else {
        info("dispatch_eip_packet(): Error unmarshalling or validating EIP packet, %s!", plc_tag_decode_error(rc));
        result = slice_make_err(rc);
    }

    return result;
}





slice_s marshall_eip_response(context_s *context, slice_s output_buf, eip_packet_s *eip_packet)
{
    (void) context;

    /* just write out the fields.   Do a length check first. */
    if(output_buf.len < (EIP_HEADER_SIZE + eip_packet->length)) {
        info("marshall_eip_response(): Insufficient space (%d bytes) in the output buffer for %d bytes of header and payload!", output_buf.len, (EIP_HEADER_SIZE + eip_packet->length));
        return slice_make_err(PLCTAG_ERR_TOO_SMALL);
    }

    set_uint16_le(output_buf, 0, eip_packet->command);
    set_uint16_le(output_buf, 2, eip_packet->length);
    set_uint32_le(output_buf, 4, eip_packet->session_handle);
    set_uint32_le(output_buf, 8, (uint32_t)eip_packet->status);
    set_uint64_le(output_buf, 12, eip_packet->sender_context);
    set_uint32_le(output_buf, 20, eip_packet->options);

    return slice_trunc(output_buf, EIP_HEADER_SIZE);
}




int unmarshal_and_validate_eip_request(context_s *context, slice_s raw_eip_request, eip_packet_s *eip_packet)
{
    /* check the slice size.  Must be at least the EIP header size. */
    if(raw_eip_request.len < EIP_HEADER_SIZE) {
        info("unmarshal_and_validate_eip_request(): raw packet is too small for a full EIP header, got size %d!", raw_eip_request.len);
        return PLCTAG_ERR_TOO_SMALL;
    }

    /* unmarshall the data in the packet. */
    eip_packet->command = get_uint16_le(raw_eip_request, 0);
    eip_packet->length = get_uint16_le(raw_eip_request, 2);
    eip_packet->session_handle = get_uint32_le(raw_eip_request, 4);
    eip_packet->status = (int32_t)get_uint32_le(raw_eip_request, 8);
    eip_packet->sender_context = get_uint64_le(raw_eip_request, 12);
    eip_packet->options = get_uint32_le(raw_eip_request, 20);

    /* separate out the payload. */
    eip_packet->payload = slice_remainder(raw_eip_request, EIP_HEADER_SIZE);

    info("unmarshal_and_validate_eip_request(): EIP packet type %x received with length %d and payload length of %d  (actual %d).",
            eip_packet->command,
            raw_eip_request.len,
            eip_packet->length,
            eip_packet->payload.len);

    /* packet length other than the header must match the length parameter. */
    if(eip_packet->length != eip_packet->payload.len) {
        info("EIP request has length parameter %d, does not match actual payload length of %d!", (int)eip_packet->length, (int)eip_packet->payload.len);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* check the session handle.  This must be zero only for session setup requests. */
    if(eip_packet->command == EIP_REGISTER_SESSION && eip_packet->session_handle != 0) {
        info("Register session request session handle must be zero, was %x!", (int)eip_packet->session_handle);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* check the session handle.  This must be present in all but session setup requests. */
    if(eip_packet->command != EIP_REGISTER_SESSION && eip_packet->session_handle != context->session_handle) {
        info("Non-register session EIP request session handle must be %x, was %x!", (int)context->session_handle, (int)eip_packet->session_handle);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* packet status must be zero. */
    if(eip_packet->status != 0) {
        info("Register session request status must be zero, was %x!", (int)eip_packet->status);
        /* FIXME - translate these into PLCTAG_ERR_XXX codes. */
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* the sender context should be zero in some circumstances */
    if(eip_packet->sender_context != (uint64_t)0) {
        if(eip_packet->command == EIP_REGISTER_SESSION || eip_packet->command == EIP_CONNECTED_DATA) {
            info("Session context must be zero!");
            return PLCTAG_ERR_BAD_PARAM;
        }
    }

    /* packet options must be zero. */
    if(eip_packet->options != 0) {
        info("EIP request header options must be zero but was %d!", (int)eip_packet->options);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* save some data for later. */
    context->client_session_context = eip_packet->sender_context;

    return PLCTAG_STATUS_OK;
}






