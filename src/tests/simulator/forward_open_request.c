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
#include <tests/simulator/cip.h>
#include <tests/simulator/forward_open_request.h>
#include <tests/simulator/unconnected_request.h>
#include <tests/simulator/utils.h>


#define FO_PARAM_SECS_PER_TICK              ((uint8_t)0x0a)
#define FO_PARAM_TIMEOUT_TICKS              ((uint8_t)0x05)
#define FO_PARAM_CONN_SERIAL_NUMBER         ((uint16_t)0x3420)
#define FO_PARAM_VENDOR_ID                  ((uint16_t)0xF33D)
#define FO_PARAM_ORIG_SERIAL_NUMBER         ((uint32_t)0x21504345)
#define FO_PARAM_CONN_TIMEOUT_MULTIPLIER    ((uint8_t)0x01)
#define FO_PARAM_RPI                        ((uint32_t)0x000f4240)  /* 1000000 */
#define FO_PARAM_CONN_PARAMS                ((uint16_t)0x43F8)
#define FO_PARAM_CONN_PARAMS_EX             ((uint32_t)0x42000fa2)


static uint8_t message_router_path[] = { 0x01,              /* backplane port of network module */
                                         0x06,              /* slot 6 for CPU */
                                         CIP_IOI_CLASS_1B,  /* class */
                                         0x02,              /* MR Message Router class */
                                         CIP_IOI_INSTANCE_1B, /* instance */
                                         0x01               /* instance #1 */
                                       };

#define MESSAGE_ROUTER_PATH_SIZE  ((int)(sizeof(message_router_path)/sizeof(message_router_path[0])))


static int unmarshall_and_validate_forward_open_request(context_s *context, slice_s raw_request, forward_open_request_s *fo_req);
static slice_s marshall_foward_open_response(context_s *context, slice_s output_buf, forward_open_request_s *fo_req);


slice_s handle_forward_open_request(context_s *context, slice_s raw_request)
{
    forward_open_request_s fo_req;
    int rc;

    rc = unmarshall_and_validate_forward_open_request(context, raw_request, &fo_req);
    if(rc != PLCTAG_STATUS_OK) {
        info("handle_forward_open_request(): failed to validate or unmarshall forward open request, %s!", plc_tag_decode_error(rc));
        return slice_make_err(rc);
    }

    return marshall_foward_open_response(context, context->buffer, &fo_req);
}


#define BUMP(n) do { offset += (n); } while(0);

int unmarshall_and_validate_forward_open_request(context_s *context, slice_s raw_request, forward_open_request_s *fo_req)
{
    int offset = 0;

    /* check the size.  This is tricky because there are variable length fields. */
    if(raw_request.len < (FORWARD_OPEN_REQ_SIZE_BASE + 4)) {
        info("unmarshall_and_validate_forward_open_request(): Payload is insufficient for forward open request data!");
        return PLCTAG_ERR_TOO_SMALL;
    }

    /* start decoding the connection manager command and path.*/
    fo_req->cm_service_code = slice_at(raw_request, offset); BUMP(1);
    fo_req->cm_req_path_size = slice_at(raw_request, offset); BUMP(1);  /* path len in 16-bit words. */

    /* get the path to the target object. */
    fo_req->cm_req_path = slice_trunc(slice_remainder(raw_request, offset), fo_req->cm_req_path_size * 2);
    BUMP(fo_req->cm_req_path_size * 2);

    /* get the Forward Open parameters. */
    fo_req->secs_per_tick = slice_at(raw_request, offset); BUMP(1);
    fo_req->timeout_ticks = slice_at(raw_request, offset); BUMP(1);

    fo_req->orig_to_targ_conn_id = get_uint32_le(raw_request, offset); BUMP(4);
    fo_req->targ_to_orig_conn_id = get_uint32_le(raw_request, offset); BUMP(4);
    fo_req->conn_serial_number = get_uint16_le(raw_request, offset); BUMP(2);
    fo_req->orig_vendor_id = get_uint16_le(raw_request, offset); BUMP(2);
    fo_req->orig_serial_number = get_uint32_le(raw_request, offset); BUMP(4);
    fo_req->conn_timeout_multiplier = (uint8_t)slice_at(raw_request, offset); BUMP(1);
    fo_req->reserved[0] = slice_at(raw_request, offset); BUMP(1);
    fo_req->reserved[1] = slice_at(raw_request, offset); BUMP(1);
    fo_req->reserved[2] = slice_at(raw_request, offset); BUMP(1);
    fo_req->orig_to_targ_rpi = get_uint32_le(raw_request, offset); BUMP(4);

    /* the old command has 16-bit params.  The new one has 32-bit params. */
    if(fo_req->cm_service_code == CIP_CMD_FORWARD_OPEN) {
        fo_req->orig_to_targ_conn_params = get_uint16_le(raw_request, offset); BUMP(2);
    } else {
        fo_req->orig_to_targ_conn_params = get_uint32_le(raw_request, offset); BUMP(4);
    }

    fo_req->targ_to_orig_rpi = get_uint32_le(raw_request, offset); BUMP(4);

    if(fo_req->cm_service_code == CIP_CMD_FORWARD_OPEN) {
        fo_req->targ_to_orig_conn_params = get_uint16_le(raw_request, offset); BUMP(2);
    } else {
        fo_req->targ_to_orig_conn_params = get_uint32_le(raw_request, offset); BUMP(4);
    }

    fo_req->transport_class = slice_at(raw_request, offset); BUMP(1);
    fo_req->path_size = slice_at(raw_request, offset); BUMP(1);

    /* get the path to the CPU */
    fo_req->path = slice_trunc(slice_remainder(raw_request, offset), fo_req->path_size * 2);
    BUMP(fo_req->path_size * 2);

    /* now we can validate the fields. */

    /* first the size */
    if(offset > raw_request.len) {
        info("unmarshall_and_validate_forward_open_request(): Payload after parsing is insufficient for forward open request data!");
        return PLCTAG_ERR_TOO_SMALL;
    }

    if(fo_req->cm_req_path_size != 2) {
        info("unmarshall_and_validate_forward_open_request(): path length to the CM is not two!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* check the path: Connection manager path: 0x20,0x06,0x24,0x01  */
    if(slice_at(fo_req->cm_req_path,0) != 0x20 || slice_at(fo_req->cm_req_path,1) != 0x06 || slice_at(fo_req->cm_req_path,2) != 0x24 || slice_at(fo_req->cm_req_path,3) != 0x01) {
        info("unmarshall_and_validate_forward_open_request(): path to the CM is not valid!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->secs_per_tick != (uint8_t)FO_PARAM_SECS_PER_TICK) {
        info("unmarshall_and_validate_forward_open_request(): secs per tick param should be %d but was %d!", FO_PARAM_SECS_PER_TICK, fo_req->secs_per_tick);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->timeout_ticks != (uint8_t)FO_PARAM_TIMEOUT_TICKS) {
        info("unmarshall_and_validate_forward_open_request(): timeout ticks param should be %d but was %d!", FO_PARAM_TIMEOUT_TICKS, fo_req->timeout_ticks);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->orig_to_targ_conn_id != 0) {
        info("unmarshall_and_validate_forward_open_request(): originator to target connection ID param should be zero but was %d!", fo_req->orig_to_targ_conn_id);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* this comes from the client so anything other than zero is good. */
    if(fo_req->targ_to_orig_conn_id == 0) {
        info("unmarshall_and_validate_forward_open_request(): originator to target connection ID param should not be zero but was!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* store that away in the context for later. */
    context->client_connection_id = fo_req->targ_to_orig_conn_id;

    /* make one up for us. */
    context->server_connection_id = (uint32_t)rand();

    if(fo_req->conn_serial_number != FO_PARAM_CONN_SERIAL_NUMBER) {
        info("unmarshall_and_validate_forward_open_request(): connection serial number param should be %d but was %d!", FO_PARAM_CONN_SERIAL_NUMBER, fo_req->conn_serial_number);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->orig_vendor_id != FO_PARAM_VENDOR_ID) {
        info("unmarshall_and_validate_forward_open_request(): vendor ID param should be %d but was %d!", FO_PARAM_VENDOR_ID, fo_req->orig_vendor_id);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->orig_serial_number != FO_PARAM_ORIG_SERIAL_NUMBER) {
        info("unmarshall_and_validate_forward_open_request(): originator serial number param should be %d but was %d!", FO_PARAM_ORIG_SERIAL_NUMBER, fo_req->orig_serial_number);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->conn_timeout_multiplier != FO_PARAM_CONN_TIMEOUT_MULTIPLIER) {
        info("unmarshall_and_validate_forward_open_request(): connection timeout multiplier param should be %d but was %d!", FO_PARAM_CONN_TIMEOUT_MULTIPLIER, fo_req->conn_timeout_multiplier);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->reserved[0] != 0) {
        info("unmarshall_and_validate_forward_open_request(): reserved byte 0 should be zero but was %d!", fo_req->reserved[0]);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->reserved[1] != 0) {
        info("unmarshall_and_validate_forward_open_request(): reserved byte 1 should be zero but was %d!", fo_req->reserved[0]);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->reserved[2] != 0) {
        info("unmarshall_and_validate_forward_open_request(): reserved byte 2 should be zero but was %d!", fo_req->reserved[0]);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->orig_to_targ_rpi != FO_PARAM_RPI) {
        info("unmarshall_and_validate_forward_open_request(): originator to target RPI param should be %d but was %d!", FO_PARAM_RPI, fo_req->orig_to_targ_rpi);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->cm_service_code == CIP_CMD_FORWARD_OPEN) {
        if(fo_req->orig_to_targ_conn_params != (uint32_t)FO_PARAM_CONN_PARAMS) {
            info("unmarshall_and_validate_forward_open_request(): originator to target connection param should be %d but was %d!", FO_PARAM_CONN_PARAMS, fo_req->orig_to_targ_conn_params);
            return PLCTAG_ERR_BAD_PARAM;
        }
    } else {
        if(fo_req->orig_to_targ_conn_params != FO_PARAM_CONN_PARAMS_EX) {
            info("unmarshall_and_validate_forward_open_request(): originator to target connection param should be %d but was %d!", FO_PARAM_CONN_PARAMS_EX, fo_req->orig_to_targ_conn_params);
            return PLCTAG_ERR_BAD_PARAM;
        }
    }

    if(fo_req->targ_to_orig_rpi != FO_PARAM_RPI) {
        info("unmarshall_and_validate_forward_open_request(): target to originator RPI param should be %d but was %d!", FO_PARAM_RPI, fo_req->targ_to_orig_rpi);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(fo_req->cm_service_code == CIP_CMD_FORWARD_OPEN) {
        if(fo_req->targ_to_orig_conn_params != (uint32_t)FO_PARAM_CONN_PARAMS) {
            info("unmarshall_and_validate_forward_open_request(): target to originator connection param should be %d but was %d!", FO_PARAM_CONN_PARAMS, fo_req->targ_to_orig_conn_params);
            return PLCTAG_ERR_BAD_PARAM;
        }
    } else {
        if(fo_req->targ_to_orig_conn_params != FO_PARAM_CONN_PARAMS_EX) {
            info("unmarshall_and_validate_forward_open_request(): target to originator connection param should be %d but was %d!", FO_PARAM_CONN_PARAMS_EX, fo_req->targ_to_orig_conn_params);
            return PLCTAG_ERR_BAD_PARAM;
        }
    }

    if(fo_req->transport_class != (uint8_t)CIP_TRANSPORT_EXPLICIT) {
        info("unmarshall_and_validate_forward_open_request(): transport class param should be %d but was %d!", CIP_TRANSPORT_EXPLICIT, fo_req->transport_class);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /*
     * validate the path to the message router in the PLC CPU module.  For the purposes of this
     * simulation, the path is fixed.
     */

    if(fo_req->path_size * 2 != (uint8_t)MESSAGE_ROUTER_PATH_SIZE) {
        info("unmarshall_and_validate_forward_open_request(): message router path param should be %d bytes long but was %d!", MESSAGE_ROUTER_PATH_SIZE, fo_req->transport_class);
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* check the path itself. */
    for(int i=0; i < MESSAGE_ROUTER_PATH_SIZE; i++) {
        if(slice_at(fo_req->path, i) != message_router_path[i]) {
            info("unmarshall_and_validate_forward_open_request(): message router path byte %d should be %d byte but was %d!", i, message_router_path[i] , slice_at(fo_req->path, i));
            return PLCTAG_ERR_BAD_PARAM;
        }
    }


    /* all tests passed. */

    return PLCTAG_STATUS_OK;
}




slice_s marshall_foward_open_response(context_s *context, slice_s output_buf, forward_open_request_s *fo_req)
{
    uc_cpf_s cpf_resp;
    slice_s tmp_out;
    slice_s cpf_header;
    int offset = 0;

    /* set default values of zero. */
    memset(&cpf_resp, 0, sizeof cpf_resp);

    cpf_resp.item_count = 2;
    cpf_resp.nai_item_type = CPF_ITEM_NAI;
    cpf_resp.nai_item_length = 0;
    cpf_resp.udi_item_type = CPF_ITEM_UDI;
    cpf_resp.udi_item_length = FORWARD_OPEN_RESP_SIZE;

    /* first set up the CPF header. */
    cpf_header = marshall_unconnected_response(context, output_buf, &cpf_resp);
    if(slice_err(cpf_header) != PLCTAG_STATUS_OK) {
        info("marshall_foward_open_response(): Unable to marshal CPF header, error %s!", plc_tag_decode_error(slice_err(cpf_header)));
        return cpf_header;
    }

    /* build an output buffer for our own use. */
    tmp_out = slice_remainder(output_buf, cpf_header.len);
    if(tmp_out.len < FORWARD_OPEN_RESP_SIZE) {
        info("marshall_foward_open_response() output buffer has insufficient space for forward open response fields!");
        return slice_make_err(PLCTAG_ERR_TOO_SMALL);
    }

//    uint8_t resp_service_code;      /* returned as 0xD4 or 0xDB */
//    uint8_t reserved1;               /* returned as 0x00? */
//    uint8_t general_status;         /* 0 on success */
//    uint8_t status_size;            /* number of 16-bit words of extra status, 0 if success */
//    uint32_t orig_to_targ_conn_id;  /* target's connection ID for us, save this. */
//    uint32_t targ_to_orig_conn_id;  /* our connection ID back for reference */
//    uint16_t conn_serial_number;    /* our connection ID/serial number from request */
//    uint16_t orig_vendor_id;        /* our unique vendor ID from request*/
//    uint32_t orig_serial_number;    /* our unique serial number from request*/
//    uint32_t orig_to_targ_api;      /* Actual packet interval, microsecs */
//    uint32_t targ_to_orig_api;      /* Actual packet interval, microsecs */
//    uint8_t app_data_size;          /* size in 16-bit words of send_data at end */
//    uint8_t reserved2;

    slice_at_put(tmp_out, offset, (fo_req->cm_service_code | CIP_CMD_OK)); BUMP(1);
    slice_at_put(tmp_out, offset, 0); BUMP(1);
    slice_at_put(tmp_out, offset, 0); BUMP(1);
    slice_at_put(tmp_out, offset, 0); BUMP(1);
    set_uint32_le(tmp_out, offset, context->client_connection_id); BUMP(4);
    set_uint32_le(tmp_out, offset, context->server_connection_id); BUMP(4);
    set_uint16_le(tmp_out, offset, FO_PARAM_CONN_SERIAL_NUMBER); BUMP(2);
    set_uint16_le(tmp_out, offset, FO_PARAM_VENDOR_ID); BUMP(2);
    set_uint32_le(tmp_out, offset, FO_PARAM_ORIG_SERIAL_NUMBER); BUMP(4);
    set_uint32_le(tmp_out, offset, FO_PARAM_RPI); BUMP(4);
    set_uint32_le(tmp_out, offset, FO_PARAM_RPI); BUMP(4);
    slice_at_put(tmp_out, offset, 0); BUMP(1);
    slice_at_put(tmp_out, offset, 0); BUMP(1);

    /* return the slice of all that has been marshalled so far. */
    return slice_trunc(output_buf, cpf_header.len + FORWARD_OPEN_RESP_SIZE);
}






