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
#include <tests/simulator/connected_request.h>
#include <tests/simulator/utils.h>


static int unmarshall_and_validate_connected_request(context_s *context, slice_s raw_cpf_packet, co_cpf_s *cpf_packet);
static slice_s dispatch_cip_request(context_s *context, slice_s raw_cip_packet);



slice_s handle_connected_request(context_s *context, slice_s raw_request)
{
    int rc = PLCTAG_STATUS_OK;
    slice_s result;
    co_cpf_s cpf_packet;

    /* make sure that we set all the data to a known state first. */
    memset(&cpf_packet, 0, sizeof(cpf_packet));

    rc = unmarshall_and_validate_connected_request(context, raw_request, &cpf_packet);
    if(rc != PLCTAG_STATUS_OK) {
        info("handle_connected_request() failed to validate incoming connected request packet with error %s!", plc_tag_decode_error(rc));
        result = slice_make_err(rc);
    } else {
        /* get the rest of the packet, but note that the connection sequence ID is not actually part of the CIP packet! */
        slice_s raw_cip_packet = slice_remainder(raw_request, CO_CPF_HEADER_SIZE + 2);
        info("handle_connected_request() validated incoming connected request.  Now dispatching CIP payload.");

        result = dispatch_cip_request(context, raw_cip_packet);
    }

    return result;
}



#define BUMP(n) do { offset += (n); } while(0);


int unmarshal_and_validate_connected_request(context_s *context, slice_s raw_cpf_packet, co_cpf_s *cpf_packet)
{
    int offset = 0;

    (void)context;
;
    if(raw_cpf_packet.len < CO_CPF_HEADER_SIZE) {
        info("unmarshal_and_validate_connected_request() CPF packet must have at least %d bytes of data.", CO_CPF_HEADER_SIZE);
        return PLCTAG_ERR_TOO_SMALL;
    }

    /* unpack the prefix on the CPF body */
    cpf_packet->interface_handle = get_uint32_le(raw_cpf_packet, offset);  BUMP(4);
    cpf_packet->router_timeout = get_uint16_le(raw_cpf_packet, offset); BUMP(2);

    if(cpf_packet->interface_handle != (uint32_t)0) {
        info("unmarshal_and_validate_connected_request(): interface handle should be zero but was %x!", (int)cpf_packet->interface_handle);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(cpf_packet->router_timeout != (uint16_t)1) {
        info("unmarshal_and_validate_connected_request(): router timeout should be 1 but was %x!", (int)cpf_packet->router_timeout);
        return PLCTAG_ERR_BAD_PARAM;
    }


    /* unpack the CPF body. */

    cpf_packet->item_count = get_uint16_le(raw_cpf_packet, offset); BUMP(2);

    if(cpf_packet->item_count != (uint16_t)2) {
        info("unmarshal_and_validate_connected_request() we can only handle CPF requests with two items but this request has %d items!", (int)cpf_packet->item_count);
        return PLCTAG_ERR_UNSUPPORTED;
    }

    /* the connected address item is two 16-bit not zero. */
    cpf_packet->cai_item_type = get_uint16_le(raw_cpf_packet, offset); BUMP(2);
    cpf_packet->cai_item_length = get_uint16_le(raw_cpf_packet, offset); BUMP(2);

    if(cpf_packet->nai_item_type != CPF_ITEM_CAI) {
        info("unmarshal_and_validate_connected_request() connected address type must be %x, got %x!", CPF_ITEM_CAI, (int)cpf_packet->cai_item_type);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(cpf_packet->nai_item_length != (uint16_t)4) {
        info("unmarshal_and_validate_connected_request() connected address length must be 4, got %x!", (int)cpf_packet->cai_item_length);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* get the connection ID */
    cpf_packet->connection_id = get_uint32_le(raw_cpf_packet, offset); BUMP(4);

    if(cpf_packet->connection_id != context->server_connection_id) {
        info("unmarshal_and_validate_connected_request(): connection ID should match %x but got %x!", context->server_connection_id, (int)cpf_packet->connection_id);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* Now get the data items.   This is a type and a length. */
    cpf_packet->cdi_item_type = get_uint16_le(raw_cpf_packet, offset); BUMP(2);
    cpf_packet->cdi_item_length = get_uint16_le(raw_cpf_packet, offset); BUMP(2);

    if(cpf_packet->cdi_item_type != CPF_ITEM_CDI) {
        info("unmarshal_and_validate_connected_request() connected data type, %x, expected, got %x!", CPF_ITEM_CDI, (int)cpf_packet->udi_item_type);
        return PLCTAG_ERR_UNSUPPORTED;
    }

    if(cpf_packet->cdi_item_length <= (uint16_t)0) {
        info("unmarshal_and_validate_connected_request() connected data length must be greater than zero, got %d!", (int)cpf_packet->cdi_item_length);
        return PLCTAG_ERR_UNSUPPORTED;
    }

    /* get the connection sequence .  */
    cpf_packet->connection_seq = get_uint16_le(raw_cpf_packet, offset); BUMP(2);
    context->client_connection_seq = cpf_packet->connection_seq;

    /* double check the payload length against the actual length. */
    if(cpf_packet->cdi_item_length != (raw_cpf_packet.len - UC_CPF_HEADER_SIZE)) {
        info("unmarshal_and_validate_connected_request() CIP packet length must be %d, got %d!", (int)(raw_cpf_packet.len - UC_CPF_HEADER_SIZE), (int)cpf_packet->udi_item_length);

        if(cpf_packet->udi_item_length < (raw_cpf_packet.len - UC_CPF_HEADER_SIZE)) {
            return PLCTAG_ERR_TOO_SMALL;
        } else {
            return PLCTAG_ERR_TOO_LARGE;
        }
    }

    return PLCTAG_STATUS_OK;
}



slice_s dispatch_cip_request(context_s *context, slice_s raw_cip_packet)
{
    uint8_t cip_command;
    slice_s result;

    /* the first byte of the payload is the CIP operation/command. */
    cip_command = slice_at(raw_cip_packet, 0);

    switch(cip_command) {
        case CIP_CMD_READ:
            /* fall through */
        case CIP_CMD_READ_FRAG:
            result = handle_read_request(context, raw_cip_packet);
            break;

        default:
            info("dispatch_cip_command(): CIP command type %x is not supported!", cip_command);
            result = slice_make_err(PLCTAG_ERR_UNSUPPORTED);
            break;
    }

    return result;
}



slice_s marshall_unconnected_response(context_s *context, slice_s output_buf, uc_cpf_s *uc_resp)
{
    eip_packet_s eip_packet;
    slice_s tmp_out;
    slice_s header;
    int offset = 0;

    /* set default values of zero. */
    memset(&eip_packet, 0, sizeof eip_packet);

    eip_packet.command = EIP_UNCONNECTED_DATA;
    eip_packet.length = uc_resp->udi_item_length + UC_CPF_HEADER_SIZE;
    eip_packet.session_handle = context->session_handle;

    header = marshall_eip_response(context, output_buf, &eip_packet);
    if(slice_err(header) != PLCTAG_STATUS_OK) {
        info("marshall_unconnected_response(): Unable to marshal EIP header, error %s!", plc_tag_decode_error(slice_err(header)));
        return header;
    }

    /* build an output buffer for our own use. */
    tmp_out = slice_remainder(output_buf, header.len);

    /* first set up the EIP header. */
    if(tmp_out.len < UC_CPF_HEADER_SIZE) {
        info("marshall_unconnected_response() output buffer has insufficient space for CPF fields!");
        return slice_make_err(PLCTAG_ERR_TOO_SMALL);
    }

    set_uint32_le(tmp_out, offset, /*uc_resp->interface_handle*/ (uint32_t)0); BUMP(4);
    set_uint16_le(tmp_out, offset, /*uc_resp->router_timeout*/ (uint16_t)1); BUMP(2);
    set_uint16_le(tmp_out, offset, uc_resp->item_count); BUMP(2);
    set_uint16_le(tmp_out, offset, uc_resp->nai_item_type); BUMP(2);
    set_uint16_le(tmp_out, offset, uc_resp->nai_item_length); BUMP(2);
    set_uint16_le(tmp_out, offset, uc_resp->udi_item_type); BUMP(2);
    set_uint16_le(tmp_out, offset, uc_resp->udi_item_length); BUMP(2);

    /* return the slice of all that has been marshalled so far. */
    return slice_trunc(output_buf, header.len + UC_CPF_HEADER_SIZE);
}


