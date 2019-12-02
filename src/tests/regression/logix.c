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


#include <signal.h>
#include "tests/regression/util.h"
#include "tests/regression/logix.h"
#include "lib/libplctag.h"

static const char *register_session_request_string = "65 00 04 00 00 00 00 00 00 00 "
                                                     "00 00 00 00 00 00 00 00 00 00 "
                                                     "00 00 00 00 01 00 00 00 ";

static const char *register_session_response_string = "65 00 04 00 <1 <1 <1 <1 00 00 "
                                                      "00 00 00 00 00 00 00 00 00 00 "
                                                      "00 00 00 00 01 00 00 00 ";

static const char *forward_open_ex_request_string = "6f 00 44 00 =1 =1 =1 =1 00 00 "
                                                    "00 00 >2 >2 ?? ?? ?? ?? ?? ?? "
                                                    "00 00 00 00 00 00 00 00 01 00 "
                                                    "02 00 00 00 00 00 b2 00 34 00 "
                                                    "5b 02 20 06 24 01 0a 05 00 00 "
                                                    "00 00 e8 ca 2a 2a b0 df 3d f3 "
                                                    "45 43 50 21 01 00 00 00 40 42 "
                                                    "0f 00 a2 0f 00 42 40 42 0f 00 "
                                                    "a2 0f 00 42 a3 03 01 04 20 02 "
                                                    "24 01 ";
/*

PC -> PLC
6f 00 44 00 =1 =1 =1 =1 00 00
00 00 >2 >2 >2 >2 >2 >2 >2 >2
00 00 00 00 00 00 00 00 01 00
02 00 00 00 00 00 b2 00 34 00
5b 02 20 06 24 01 0a 05 00 00
00 00 >4 >4 >4 >4 >5 >5 3d f3
45 43 50 21 01 00 00 00 40 42
0f 00 a2 0f 00 42 40 42 0f 00
a2 0f 00 42 a3 03 01 04 20 02
24 01

?1 = session handle
?2 = session id
?4 = PC connection ID
?5 = PC connection sequence ID

PLC -> PC
6f 00 2e 00 <1 <1 <1 <1 00 00
00 00 <2 <2 <2 <2 <2 <2 <2 <2
00 00 00 00 00 00 00 00 00 00
02 00 00 00 00 00 b2 00 1e 00
db 00 00 00 <3 <3 <3 <3 <4 <4
<4 <4 <5 <5 3d f3 45 43 50 21
40 42 0f 00 40 42 0f 00 00 00

?3 = PLC connection ID, made up.

PC -> PLC
70 00 2e 00 =1 =1 =1 =1 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 01 00
02 00 a1 00 04 00 =3 =3 =3 =3
b1 00 >6 >6 01 00 52 08 91 0d
54 65 73 74 44 49 4e 54 41 72
72 61 79 00 01 00 00 00 00 00

?6 = PC connection sequence ID

PLC -> PC
70 00 20 00 <1 <1 <1 <1 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
02 00 a1 00 04 00 <4 <4 <4 <4
b1 00 <7 <7 01 00 d2 00 00 00
c4 00 <8 <8 <8 <8

?7 = PLC connection sequence ID. Made up.
?8 = DINT data from PLC.

PC->PLC
6f 00 28 00 =1 =1 =1 =1 00 00
00 00 =2 =2 =2 =2 =2 =2 =2 =2
00 00 00 00 00 00 00 00 01 00
02 00 00 00 00 00 b2 00 18 00
4e 02 20 06 24 01 0a 05 =5 =5
3d f3 45 43 50 21 03 54 01 04
20 02 24 01

PLC->PC
6f 00 1e 00 <1 <1 <1 <1 00 00
00 00 <2 <2 <2 <2 <2 <2 <2 <2
00 00 00 00 00 00 00 00 00 00
02 00 00 00 00 00 b2 00 0e 00
ce 00 00 00 <5 <5 3d f3 45 43
50 21 00 00

 */

static const char *forward_open_response_string = "6f 00 2e 00 ?? ?? ?? ?? 00 00 "
                                                  "00 00 ?? ?? ?? ?? ?? ?? ?? ?? "
                                                  "00 00 00 00 00 00 00 00 00 00 "
                                                  "02 00 00 00 00 00 b2 00 1e 00 "
                                                  "db 00 00 00 15 40 f4 ff e8 ca "
                                                  "2a 2a b0 df 3d f3 45 43 50 21 "
                                                  "40 42 0f 00 40 42 0f 00 00 00 ";




/* this is called repeatedly. */
int logix_emulator(pid_t parent)
{
    /* run until done. */
    util_sleep_ms(10);
}
