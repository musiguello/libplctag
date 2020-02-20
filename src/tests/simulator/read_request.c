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


#include <tests/simulator/ab.h>
#include <tests/simulator/cip.h>
#include <tests/simulator/connected_request.h>
#include <tests/simulator/read_request.h>
#include <tests/simulator/utils.h>

typedef struct {
    slice_s tag_name;
    int dimensions[3];
    int fragmented;
    uint32_t offset;
} cip_read_request_s;


static int unmarshal_and_validate_read_request(context_s *context, slice_s raw_request, cip_read_request_s *req);
static slice_s marshal_read_response(context_s *context, slice_s output_buf, cip_read_request_s *req);



slice_s handle_read_request(context_s *context, slice_s raw_request)
{
    cip_read_request_s req;
    int rc;

    rc = unmarshal_and_validate_read_request(context, raw_request, &fo_req);
    /* FIXME - handle non-existent tags. */
    if(rc != PLCTAG_STATUS_OK) {
        info("handle_forward_open_request(): failed to validate or unmarshall forward open request, %s!", plc_tag_decode_error(rc));
        return slice_make_err(rc);
    }

    return marshal_read_response(context, context->buffer, &fo_req);
}

