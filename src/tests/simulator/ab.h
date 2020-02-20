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
#include <tests/simulator/slice.h>

typedef int8_t BOOLEAN;
typedef int8_t SINT;
typedef int16_t INT;
typedef int32_t DINT;
typedef float REAL;
typedef uint64_t BOOL_ARRAY_ELEMENT;

typedef enum {
    AB_DINT = 0,
    AB_INT = 1,
    AB_REAL = 2,
    AB_BOOL = 3,
    AB_BOOL_ARRAY = 4,
} tag_type_t;

typedef struct tag_s {
    char tag_name[41];
    int dimensions[3];
    tag_type_t tag_type;
    union {
        DINT dint_val[];
        INT int_val[];
        REAL real_val[];
        BOOLEAN bool_val;
        BOOL_ARRAY_ELEMENT bool_array_val[];
    } val;
} tag_s;

typedef struct {
    int done;

    int client_sock;
    slice_s buffer;
    uint16_t max_request_size;
    uint16_t max_response_size;

    uint32_t session_handle;
    uint64_t client_session_context;

    uint32_t client_connection_id;
    uint16_t client_connection_seq;
    uint32_t server_connection_id;
    uint16_t server_connection_seq;

    tag_s **tags;
} context_s;



/* CPF Item Types */
#define CPF_ITEM_CAI ((uint16_t)0x00A1) /* connected address item */
#define CPF_ITEM_CDI ((uint16_t)0x00B1) /* connected data item */


