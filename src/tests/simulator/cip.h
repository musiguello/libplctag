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

/* CIP commands. */
#define CIP_CMD_RESPONSE                ((uint8_t)0x80)
#define CIP_CMD_PCCC_EXECUTE            ((uint8_t)0x4B)
#define CIP_CMD_READ                    ((uint8_t)0x4C)
#define CIP_CMD_WRITE                   ((uint8_t)0x4D)
#define CIP_CMD_FORWARD_CLOSE           ((uint8_t)0x4E)
#define CIP_CMD_READ_FRAG               ((uint8_t)0x52)
#define CIP_CMD_WRITE_FRAG              ((uint8_t)0x53)
#define CIP_CMD_FORWARD_OPEN            ((uint8_t)0x54)
#define CIP_CMD_FORWARD_OPEN_EX         ((uint8_t)0x5B)

#define CIP_CMD_OK                      ((uint8_t)0x80)


/* CIP status, only important ones. */
#define CIP_STATUS_OK                   ((uint8_t)0)
#define CIP_STATUS_FRAG                 ((uint8_t)0x06)

/* CIP name encoding constants. */
#define CIP_SYMBOLIC_SEGMENT            ((uint8_t)0x91)
#define CIP_NUMERIC_SEGMENT_ONE_BYTE    ((uint8_t)0x28)
#define CIP_NUMERIC_SEGMENT_TWO_BYTES   ((uint8_t)0x29)
#define CIP_NUMERIC_SEGMENT_FOUR_BYTES  ((uint8_t)0x2A)

/* CIP IOI path encoding constants */
#define CIP_IOI_CLASS_1B                ((uint8_t)0x20)
#define CIP_IOI_INSTANCE_1B             ((uint8_t)0x24)

/* other CIP defines */
#define CIP_TRANSPORT_EXPLICIT          ((uint8_t)0xA3)  /* Class 3, application trigger. */


