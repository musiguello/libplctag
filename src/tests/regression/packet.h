/***************************************************************************
 *   Copyright (C) 2019 by Kyle Hayes                                      *
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
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <stddef.h>
#include "lib/libplctag2.h"
#include <tests/regression/slice.h>
#include <tests/regression/socket.h>


/*
 * packet routines, like printf/scanf
 *
 * Simple format:
 *
 * The template string is composed of elements separated by spaces.  Each element
 * can be either an immediate value or be copied in/out of a variable in the
 * varargs section.   Spaces and tabs are considered to be whitespace and are
 * ignored.
 *
 * Element types:
 *
 * =XX - if packing, this pushes the byte described by the hex digits into the output buffer slice.
 *       When unpacking, this matches a byte value in the input buffer slice.
 *
 * Input/Output:
 *
 * %U1 - When unpacking, copy a 1-byte unsigned integer into the respective argument passed
 *       in the varargs part.  When packing, copy a 1-byte unsigned integer from the respective
 *       varargs argument into the passed buffer slice.
 *
 * %L2 - When unpacking, copy a little endian 2-byte unsigned integer into the respective argument passed
 *       in the varargs part.  When packing, copy a little endian 2-byte unsigned integer from the respective
 *       varargs argument into the passed buffer slice.
 *
 * %B2 - As for L2, but with big endian 2-byte unsigned integer.
 *
 * %L4 - As for L2, but with little endian 4-byte unsigned integer.
 *
 * %B4 - As for L2, but with big endian 4-byte unsigned integer.
 *
 * %L8 - As for L2, but with little endian 8-byte unsigned integer.
 *
 * %B8 - As for L2, but with big endian 8-byte unsigned integer.
 *
 * Return Values:
 *
 * The pack_slice routine returns a slice covering the entire new part that was packed in the output
 * slice.   When there is an error to report, an error slice is returned.  Errors:
 *    PLCTAG_ERR_OUT_OF_BOUNDS - returned when the output data would be longer than the output buffer slice.
 *
 * The unpack_slice() function returns a slice covering all the data in the input buffer that was processed.
 * If the slice was unable to process all arguments, it will return an error slice.
 *    PLCTAG_ERR_TOO_SHORT - returned when there is insufficient data in the input buffer to process the
 *                           entire template.
 *
 * Both routines will return PLCTAG_ERR_UNSUPPORTED if they find a template element that does not match one
 * of the elements above.
 *
 * Example Patterns:
 *
 * A pack pattern template that puts out a 28-byte slice with a single 4-byte little-endian inserted at the
 * fifth through eighth bytes.
 *
 * =65 =00 =04 =00 %L4             =00 =00
 * =00 =00 =00 =00 =00 =00 =00 =00 =00 =00
 * =00 =00 =00 =00 =01 =00 =00 =00
 *
 * An unpack pattern template that matches a template with three fields to pull out.
 *
 * =70 =00 %L2     %L4             =00 =00
 * =00 =00 =00 =00 =00 =00 =00 =00 =00 =00
 * =00 =00 =00 =00 =00 =00 =00 =00 =01 =00
 * =02 =00 =a1 =00 =04 =00 %L4
 * =b1 =00 %L2
 *
 */

extern slice_s pack_slice(slice_s outbuf, const char *tmpl, ...);
extern slice_s unpack_slice(slice_s inbuf, const char *tmpl, ...);

/* useful for debugging. */
extern void slice_dump(slice_s s);

