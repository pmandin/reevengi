/*
	RE1 EVT
	Room events?

	Copyright (C) 2013	Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
 
#ifndef RDT_EVT_H
#define RDT_EVT_H 1

/*
00:1:inc value
01:1:store 1
02:1:store 2,some copies
03:1:store 2
04:4+2:call function, 04 NN (N from 0 to 3) MM
05:1+3:call function, 05 NN MM PP
06:NN:exec script at inst+2, 06 NN ....
07:NN:exec script bytecode:07 NN ....
08:2:08 NN
09:2:09 NN

80:o1 :1:nop?
81:o2 :1
82:o3 :1
83:o4 :1
84:o5 :4
85:o6 :2
86:o7 :1
87:o8 :2
88:o9 :2
89:o10:
8a:o11:
8b:o12:1

f6:1
f7:1
f8:1,??,word used to init stuff
f9:0 or 3
fa:4
fb:1 or variable
fc:variable
fd:script bytescodes, variable
fe:1:nop
ff:0:exit event: return value in eax[2] ?

0xf6	1		process stuff, continue with 0xf7 stuff
0xf7	1		on condition, continue with 0xfe stuff
0xf8	?		continue with 0xf9 stuff
0xf9	3		on condition, continue with 0xfe stuff
0xfa 	4		process stuff
0xfb 	1		can jump to other address
0xfc	ptr[1].b	process stuff, use ptr[2].w, add ptr[1].b, store curptr
0xfd	1		execute SCD stuff, jump to other address
*/

/*--- Defines ---*/

/*--- External types ---*/

struct room_s;

/*--- Types ---*/

/*--- Functions ---*/


#endif /* RDT_EVT_H */
