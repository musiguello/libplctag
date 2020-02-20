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

/* standard CPF connected message with two items. */
typedef struct {
    /* seems to be a prefix on each CPF header. */
    uint32_t interface_handle;      /* ALWAYS 0 */
    uint16_t router_timeout;        /* in seconds */

    /* the CPF body */
    uint16_t item_count;
    uint16_t cai_item_type;
    uint16_t cai_item_length;
    uint32_t connection_id;
    uint16_t cdi_item_type;
    uint16_t cdi_item_length;
    uint16_t connection_seq;
    slice_s payload;
} co_cpf_s;

#define CO_CPF_HEADER_SIZE (20) /* does not include the connection sequence! */

#define CPF_ITEM_CAI ((uint16_t)0x00A1) /* connected address item */
#define CPF_ITEM_CDI ((uint16_t)0x00B1) /* connected data item */


extern slice_s handle_connected_request(context_s *context, slice_s raw_request);
extern slice_s marshall_connected_response(context_s *context, slice_s output_buf, uc_cpf_s *uc_resp);

