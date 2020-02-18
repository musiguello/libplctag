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


#include <string.h>
#include <tests/simulator/packet.h>
#include <tests/simulator/slice.h>
#include <tests/simulator/socket.h>
#include <tests/simulator/utils.h>


eip_packet_s read_eip_packet(int sock, slice_s input_buf)
{
    int bytes_left_to_read = EIP_HEADER_SIZE;
    slice_s tmp_in_buf = input_buf;
    int bytes_read = 0;
    eip_packet_s result;

    memset(&result, 0, sizeof(result));

    /* get at least 24 bytes. */
    do {
        tmp_in_buf = socket_read(sock, tmp_in_buf);

        if(slice_err(tmp_in_buf)) {
            info("ERROR: Unable to read socket: %s.", plc_tag_decode_error(slice_err(tmp_in_buf)));
            result.status = slice_err(tmp_in_buf);
            return result;
        }

        /* update the bytes read and the input buffer */
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

    tmp_in_buf = slice_trunc(input_buf, bytes_read);

    info("read_eip_packet() got packet:");
    slice_dump(tmp_in_buf);

    /* unmarshall the data in the packet. */
    result.command = get_uint16_le(tmp_in_buf, 0);
    result.length = get_uint16_le(tmp_in_buf, 2);
    result.session_handle = get_uint32_le(tmp_in_buf, 4);
    result.status = (int32_t)get_uint32_le(tmp_in_buf, 8);
    result.sender_context = get_uint64_le(tmp_in_buf, 12);
    result.options = get_uint32_le(tmp_in_buf, 20);

    /* separate out the payload. */
    result.payload = slice_remainder(tmp_in_buf, EIP_HEADER_SIZE);

    return result;
}
