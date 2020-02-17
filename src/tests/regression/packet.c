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

#include <stdarg.h>
#include <stdio.h>
#include <tests/regression/packet.h>
#include <tests/regression/util.h>
#include <tests/regression/slice.h>


/*
 *  * Example Patterns:
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
 */


typedef enum {
    INT_TYPE_ERROR,
    INT_TYPE_U1,
    INT_TYPE_L2,
    INT_TYPE_B2,
    INT_TYPE_L4,
    INT_TYPE_B4,
    INT_TYPE_L8,
    INT_TYPE_B8
} int_type_t;


static int from_hex(const char *hex_digits);
static int_type_t get_int_type(const char *type_chars);
static int pack_val(slice_s outbuf, int out_index, int_type_t int_type, va_list args);
static int unpack_val(slice_s inbuf, int in_index, int_type_t int_type, va_list args);



slice_s pack_slice(slice_s outbuf, const char *tmpl, ...)
{
    va_list args;
    int out_index = 0;

    va_start(args, tmpl);

    /* process template. */
    while (*tmpl != '\0') {
        if (*tmpl == '=') {
            /* insert byte given by hex digits. */
            int val = 0;

            tmpl++;

            val = from_hex(tmpl);

            if(val < 0) {
                return slice_make_err(val);
            } else {
                /* valid hex. */
                int rc = slice_at_put(outbuf, out_index, (uint8_t)val);
                if(rc < 0) {
                    info("Unable to insert into template at position %d!", out_index);
                    return slice_make_err(rc);
                }

                /* ready the next output element index. */
                out_index++;
                tmpl += 2;
            }
        } else if(*tmpl == '%') {
            /* Insert bytes provided by the passed argument in the specified size and endian-ness. */
            int_type_t int_type;

            /* bump past the initial '%' character. */
            tmpl++;

            /* what type and size is this? */
            int_type = get_int_type(tmpl);
            if(int_type == INT_TYPE_ERROR) {
                return slice_make_err(PLCTAG_ERR_BAD_DATA);
            } else {
                out_index = pack_val(outbuf, out_index, int_type, args);
                if(out_index < 0) {
                    return slice_make_err(out_index);
                } else {
                    /* bump past the type. */
                    tmpl += 2;
                }
            }
        } else if(*tmpl == ' ' || *tmpl == '\t') {
            /* eat whitespace */
            tmpl++;
        } else {
            info("Unsupported template element type '%c'", *tmpl);
            return slice_make_err(PLCTAG_ERR_UNSUPPORTED);
        }
    }

    /* clean up the varargs */
    va_end(args);

    /*
     * if we got here, everything went smoothly, so return the slice representing the
     * newly created, packed data.
     */

    return slice_trunc(outbuf, out_index);
}


slice_s unpack_slice(slice_s inbuf, const char *tmpl, ...)
{
    va_list args;
    int in_index = 0;

    va_start(args, tmpl);

    info("Starting unpack_slice().");
    info("input template=%s",tmpl);
    info("input slice=");
    slice_dump(inbuf);

    /* process template. */
    while (*tmpl != '\0') {
        if (*tmpl == '=') {
            /* match byte given by hex digits. */
            int val = 0;

            tmpl++;

            val = from_hex(tmpl);

            if(val < 0) {
                return slice_make_err(val);
            } else {
                /* valid hex. */
                int rc = slice_at(inbuf, in_index);
                if(rc < 0) {
                    info("Unable to read from input buffer at position %d!", in_index);
                    return slice_make_err(rc);
                }

                /* now match */
                if(rc != val) {
                    info("\t%x(%d) != %x!", rc, in_index, val);
                    return slice_make_err(PLCTAG_ERR_NO_MATCH);
                } else {
                    info("\t%x(%d) = %x", rc, in_index, val);
                }

                /* ready the next input element index. */
                in_index++;
                tmpl += 2;
            }
        } else if(*tmpl == '%') {
            /* Copy out bytes provided by the passed argument in the specified size and endian-ness. */
            int_type_t int_type;

            /* bump past the initial '%' character. */
            tmpl++;

            /* what type and size is this? */
            int_type = get_int_type(tmpl);
            if(int_type == INT_TYPE_ERROR) {
                return slice_make_err(PLCTAG_ERR_BAD_DATA);
            } else {
                in_index = unpack_val(inbuf, in_index, int_type, args);
                if(in_index < 0) {
                    info("Error unpacking bytes!");
                    return slice_make_err(in_index);
                } else {
                    /* bump past the type. */
                    tmpl += 2;
                }
            }
        } else if(*tmpl == ' ' || *tmpl == '\t') {
            /* eat whitespace */
            tmpl++;
        } else {
            info("Unsupported template element type '%c'", *tmpl);
            return slice_make_err(PLCTAG_ERR_UNSUPPORTED);
        }
    }

    /* clean up the varargs */
    va_end(args);

    /*
     * if we got here, everything went smoothly, so return the slice representing the
     * newly created, packed data.
     */

    return slice_make(inbuf.data, in_index);
}


/* helper functions */

int from_hex(const char *hex_digits)
{
    int result = 0;

    info("from_hex(%c%c)",hex_digits[0], hex_digits[1]);

    for(int digit_count=0; digit_count < 2; digit_count++) {
        switch(hex_digits[digit_count]) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                result = (result << 4) + (hex_digits[digit_count] - '0');
                break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                result = (result << 4) + 10 + (hex_digits[digit_count] - 'a');
                break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                result = (result << 4) + 10 + (hex_digits[digit_count] - 'A');
                break;
            case 0:
                /* unexpectedly short string. */
                info("Expected hex digit but found end of string!");
                return PLCTAG_ERR_TOO_SMALL;
                break;
            default:
                /* unexpected character */
                info("Expected hex digit but found other character!");
                return PLCTAG_ERR_BAD_DATA;
                break;
        }
    }

    return result;
}


int_type_t get_int_type(const char *type_chars)
{
    if(*type_chars == 'L') {
        type_chars++;

        switch(*type_chars) {
            case '2': return INT_TYPE_L2; break;
            case '4': return INT_TYPE_L4; break;
            case '8': return INT_TYPE_L8; break;
            default: break;
        }
    } else if(*type_chars == 'B') {
        type_chars++;

        switch(*type_chars) {
            case '2': return INT_TYPE_B2; break;
            case '4': return INT_TYPE_B4; break;
            case '8': return INT_TYPE_B8; break;
            default: break;
        }
    } else if(*type_chars) {
        type_chars++;

        if(*type_chars == '1') {
            return INT_TYPE_U1;
        }
    }

    info("Unsupported int type '%c'!", *type_chars);

    return INT_TYPE_ERROR;
}



int pack_val(slice_s outbuf, int out_index, int_type_t int_type, va_list args)
{
    int ret = out_index;

    switch(int_type) {
        case INT_TYPE_U1:
        /* single bytes do not have any endian definition. */
            if(slice_in_bounds(outbuf, out_index) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                slice_at_put(outbuf, out_index, (uint8_t)va_arg(args, int));
                ret = out_index + 1;
            }

            break;

        case INT_TYPE_L2:
            /* 2-byte little endian unsigned int. */
            if(slice_in_bounds(outbuf, out_index + 1) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint16_t val = (uint16_t)va_arg(args, int);

                slice_at_put(outbuf, out_index, (uint8_t)(val & 0xFF));
                slice_at_put(outbuf, out_index + 1, (uint8_t)((val >> 8) & 0xFF));

                ret = out_index + 2;
            }

            break;

        case INT_TYPE_B2:
            /* 2-byte big endian unsigned int. */
            if(slice_in_bounds(outbuf, out_index + 1) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint16_t val = (uint16_t)va_arg(args, int);

                slice_at_put(outbuf, out_index, (uint8_t)((val >> 8) & 0xFF));
                slice_at_put(outbuf, out_index + 1, (uint8_t)(val & 0xFF));

                ret = out_index + 2;
            }

            break;

        case INT_TYPE_L4:
            /* 4-byte little endian unsigned int. */
            if(slice_in_bounds(outbuf, out_index + 3) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint32_t val = va_arg(args, uint32_t);

                slice_at_put(outbuf, out_index, (uint8_t)(val & 0xFF));
                slice_at_put(outbuf, out_index + 1, (uint8_t)((val >> 8) & 0xFF));
                slice_at_put(outbuf, out_index + 2, (uint8_t)((val >> 16) & 0xFF));
                slice_at_put(outbuf, out_index + 3, (uint8_t)((val >> 24) & 0xFF));

                ret = out_index + 4;
            }

            break;

        case INT_TYPE_B4:
            /* 4-byte big endian unsigned int. */
            if(slice_in_bounds(outbuf, out_index + 3) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint32_t val = va_arg(args, uint32_t);

                slice_at_put(outbuf, out_index, (uint8_t)((val >> 24) & 0xFF));
                slice_at_put(outbuf, out_index + 1, (uint8_t)((val >> 16) & 0xFF));
                slice_at_put(outbuf, out_index + 2, (uint8_t)((val >> 8) & 0xFF));
                slice_at_put(outbuf, out_index + 3, (uint8_t)(val & 0xFF));

                ret = out_index + 4;
            }

            break;

        case INT_TYPE_L8:
            /* 8-byte little endian unsigned int. */
            if(slice_in_bounds(outbuf, out_index + 7) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint64_t val = va_arg(args, uint64_t);

                slice_at_put(outbuf, out_index, (uint8_t)(val & 0xFF));
                slice_at_put(outbuf, out_index + 1, (uint8_t)((val >> 8) & 0xFF));
                slice_at_put(outbuf, out_index + 2, (uint8_t)((val >> 16) & 0xFF));
                slice_at_put(outbuf, out_index + 3, (uint8_t)((val >> 24) & 0xFF));
                slice_at_put(outbuf, out_index + 4, (uint8_t)((val >> 32) & 0xFF));
                slice_at_put(outbuf, out_index + 5, (uint8_t)((val >> 40) & 0xFF));
                slice_at_put(outbuf, out_index + 6, (uint8_t)((val >> 48) & 0xFF));
                slice_at_put(outbuf, out_index + 7, (uint8_t)((val >> 56) & 0xFF));

                ret = out_index + 8;
            }

            break;

        case INT_TYPE_B8:
            /* 8-byte big endian unsigned int. */
            if(slice_in_bounds(outbuf, out_index + 7) != PLCTAG_STATUS_OK) {
                info("Buffer insertion out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint64_t val = va_arg(args, uint64_t);

                slice_at_put(outbuf, out_index, (uint8_t)((val >> 56) & 0xFF));
                slice_at_put(outbuf, out_index + 1, (uint8_t)((val >> 48) & 0xFF));
                slice_at_put(outbuf, out_index + 2, (uint8_t)((val >> 40) & 0xFF));
                slice_at_put(outbuf, out_index + 3, (uint8_t)((val >> 32) & 0xFF));
                slice_at_put(outbuf, out_index + 4, (uint8_t)((val >> 24) & 0xFF));
                slice_at_put(outbuf, out_index + 5, (uint8_t)((val >> 16) & 0xFF));
                slice_at_put(outbuf, out_index + 6, (uint8_t)((val >> 8) & 0xFF));
                slice_at_put(outbuf, out_index + 7, (uint8_t)(val & 0xFF));

                ret = out_index + 8;
            }

            break;

        case INT_TYPE_ERROR:
            info("Error type passed!");
            ret = PLCTAG_ERR_BAD_PARAM;
            break;

        default:
            info("Unsupported type! Type = %d.", int_type);
            ret = PLCTAG_ERR_UNSUPPORTED;
            break;
    }

    return ret;
}


int unpack_val(slice_s inbuf, int in_index, int_type_t int_type, va_list args)
{
    int ret = in_index;

    switch(int_type) {
        case INT_TYPE_U1:
            /* single bytes do not have any endian definition. */
            if(slice_in_bounds(inbuf, in_index) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint8_t *var = va_arg(args, uint8_t *);

                *var = (uint8_t)slice_at(inbuf, in_index);

                ret = in_index + 1;
            }

            break;

        case INT_TYPE_L2:
            /* 2-byte little endian unsigned int. */
            if(slice_in_bounds(inbuf, in_index + 1) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint16_t var = 0;

                var =  (uint16_t)((uint16_t)slice_at(inbuf, in_index)
                    + ((uint16_t)slice_at(inbuf, in_index + 1) << 8));

                *(va_arg(args, uint16_t *)) = var;

                ret = in_index + 2;
            }

            break;

        case INT_TYPE_B2:
            /* 2-byte big endian unsigned int. */
            if(slice_in_bounds(inbuf, in_index + 1) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint16_t var = 0;

                var = (uint16_t)(((uint16_t)slice_at(inbuf, in_index) << 8)
                    +  (uint16_t)slice_at(inbuf, in_index + 1));

                *(va_arg(args, uint16_t *)) = var;

                ret = in_index + 2;
            }

            break;

        case INT_TYPE_L4:
            /* 4-byte little endian unsigned int. */
            if(slice_in_bounds(inbuf, in_index + 3) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint32_t var = 0;

                var =  (uint32_t)slice_at(inbuf, in_index)
                    + ((uint32_t)slice_at(inbuf, in_index + 1) << 8)
                    + ((uint32_t)slice_at(inbuf, in_index + 2) << 16)
                    + ((uint32_t)slice_at(inbuf, in_index + 3) << 24);

                *(va_arg(args, uint32_t *)) = var;

                ret = in_index + 4;
            }

            break;

        case INT_TYPE_B4:
            /* 4-byte big endian unsigned int. */
            if(slice_in_bounds(inbuf, in_index + 3) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint32_t var = 0;

                var = ((uint32_t)slice_at(inbuf, in_index)     << 24)
                    + ((uint32_t)slice_at(inbuf, in_index + 1) << 16)
                    + ((uint32_t)slice_at(inbuf, in_index + 2) << 8)
                    +  (uint32_t)slice_at(inbuf, in_index + 3);

                *(va_arg(args, uint32_t *)) = var;

                ret = in_index + 4;
            }

            break;

        case INT_TYPE_L8:
            /* 8-byte little endian unsigned int. */
            if(slice_in_bounds(inbuf, in_index + 7) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint64_t var = 0;

                var =  (uint64_t)slice_at(inbuf, in_index)
                    + ((uint64_t)slice_at(inbuf, in_index + 1) << 8)
                    + ((uint64_t)slice_at(inbuf, in_index + 2) << 16)
                    + ((uint64_t)slice_at(inbuf, in_index + 3) << 24)
                    + ((uint64_t)slice_at(inbuf, in_index + 4) << 32)
                    + ((uint64_t)slice_at(inbuf, in_index + 5) << 40)
                    + ((uint64_t)slice_at(inbuf, in_index + 6) << 48)
                    + ((uint64_t)slice_at(inbuf, in_index + 7) << 56);

                *(va_arg(args, uint64_t *)) = var;

                ret = in_index + 8;
            }

            break;

        case INT_TYPE_B8:
            /* 8-byte big endian unsigned int. */
            if(slice_in_bounds(inbuf, in_index + 7) != PLCTAG_STATUS_OK) {
                info("Buffer read out of bounds!");
                return PLCTAG_ERR_OUT_OF_BOUNDS;
            } else {
                uint64_t var = 0;

                var = ((uint64_t)slice_at(inbuf, in_index)     << 56)
                    + ((uint64_t)slice_at(inbuf, in_index + 1) << 48)
                    + ((uint64_t)slice_at(inbuf, in_index + 2) << 40)
                    + ((uint64_t)slice_at(inbuf, in_index + 3) << 32)
                    + ((uint64_t)slice_at(inbuf, in_index + 4) << 24)
                    + ((uint64_t)slice_at(inbuf, in_index + 5) << 16)
                    + ((uint64_t)slice_at(inbuf, in_index + 6) << 8)
                    +  (uint64_t)slice_at(inbuf, in_index + 7);

                *(va_arg(args, uint64_t *)) = var;

                ret = in_index + 8;
            }

            break;

        case INT_TYPE_ERROR:
            info("Error type passed!");
            ret = PLCTAG_ERR_BAD_PARAM;
            break;

        default:
            info("Unsupported type! Type = %d.", int_type);
            ret = PLCTAG_ERR_UNSUPPORTED;
            break;
    }

    return ret;
}

