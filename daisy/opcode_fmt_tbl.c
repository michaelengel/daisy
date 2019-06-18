/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          � Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "opcode_fmt.h"

/* Opcode Forms:	
			Primary		Extended	Example (page #)
			-------		--------	----------------
	D   (0-11)	0-5		None
	B   (16)	0-5		None
	I   (32)	0-5		None
	SC  (48-49)	0-5		None
	X   (64-79,)	0-5		21-30
	    (192,)
	    (180-183)
	XL  (80-83)	0-5		21-30
	XFX (96)	0-5		21-30
	XFL (112)	0-5		21-30
	XO  (128-129)	0-5		21-30
	A   (144-146)	0-5		26-30
	M   (160-161)	0-5		None


	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+-------------------------------+
D   0  |   OPCD    |   RT    |   RZ    |		D	       |  p.20
       +-----------+---------+---------+-------------------------------+
    1  |   OPCD    |   RTM   |   RZ    |		D	       |  p.25
       +-----------+---------+---------+-------------------------------+
    2  |   OPCD    |   RT    |   RA    |		UI	       |  p.41
       +-----------+-----+---+---------+-------------------------------+
    3  |   OPCD    |   TO    |   RA    |		SI	       |  p.50
       +-----------+---------+---------+-------------------------------+
    4  |   OPCD    |   RSM   |   RZ    |		D	       |  p.30
       +-----------+---------+---------+-------------------------------+
    5  |   OPCD    |   RS    |   RZ    |		D	       |  p.26
       +-----------+---------+---------+-------------------------------+
    6  |   OPCD    |   RT    |   RA    |		SI	       |  p.42
       +-----------+-----+---+---------+-------------------------------+
    7  |   OPCD    | BF  |###|   RA    |		SI	       |  p.49
       +-----------+-----+---+---------+-------------------------------+
    8  |   OPCD    | BF  |###|   RA    |		UI	       |  p.49
       +-----------+---------+---------+-------------------------------+
    9  |   OPCD    |   RS    |   RA    |		UI	       |  p.51
       +-----------+---------+---------+-------------------------------+
    10 |   OPCD    |   FRT   |   RZ    |		D	       |  p.82
       +-----------+---------+---------+-------------------------------+
    11 |   OPCD    |   FRS   |   RZ    |		D	       |  p.87
       +-----------+---------+---------+-------------------------------+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+---------------------------+-+-+
B   16 |   OPCD    |   BO    |   BI    |	   BD		   |A|L|  p.12
       +-----------+---------+---------+---------------------------+-+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+-----------------------------------------------+-+-+
I   32 |   OPCD    |			  LI			   |A|L|  p.12
       +-----------+-----------------------------------------------+-+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+-------+---------------+---+-+-+
SC  48 |   OPCD    |#########|#########|  FL1  |      LEV      |FL2|S|L|  p.14
       +-----------+---------+---------+-------+---------------+---+-+-+
    49 |   OPCD    |#########|#########|	     SV		   |S|L|  p.14
       +-----------+---------+---------+---------------------------+-+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+---------+-------------------+-+
X   64 |   OPCD    |   RT    |   RA    |   RB	 |	  EO	     |R|  p.43
       +-----------+---------+---------+---------+-------------------+-+
    65 |   OPCD    |#########|   RA    |   RB	 |	  EO	     |R|  p.124
       +-----------+-----+---+---------+---------+-------------------+-+
    66 |   OPCD    | BF  |###|#########|#########|	  EO	     |R|  p.67
       +-----------+-----+---+---------+---------+-------------------+-+
    67 |   OPCD    |   RS    |   RA    |   SH	 |	  EO	     |R|  p.60
       +-----------+---------+---------+---------+-------------------+-+
    68 |   OPCD    |   RS    |   SPR   |#########|	  EO	     |R|  p.66
       +-----------+---------+---------+---------+-------------------+-+
    69 |   OPCD    |   TO    |   RA    |   RB	 |	  EO	     |R|  p.50
       +-----------+---------+-+-------+---------+-------------------+-+
    70 |   OPCD    |   RS    |#| SR    |#########|	  EO	     |R|  p.125
       +-----------+---------+-+-------+---------+-------------------+-+
    71 |   OPCD    |   RS    |   RZ    |   RB	 |	  EO	     |R|  p.26
       +-----------+---------+---------+---------+-------------------+-+
    72 |   OPCD    |   RTS   |   RZ    |   NB	 |	  EO	     |R|  p.38
       +-----------+---------+---------+---------+-------------------+-+
    73 |   OPCD    |   RSS   |   RZ    |   NB	 |	  EO	     |R|  p.40
       +-----------+-----+---+---------+---------+-------------------+-+
    74 |   OPCD    | BF  |###|   RA    |   RB	 |	  EO	     |R|  p.49
       +-----------+-----+---+---------+---------+-------------------+-+
    75 |   OPCD    |   RS    |   RA    |   RB	 |	  EO	     |R|  p.51
       +-----------+---------+---------+---------+-------------------+-+
    76 |   OPCD    |   RT    |   RA    |#########|	  EO	     |R|  p.44
       +-----------+---------+---------+---------+-------------------+-+
    77 |   OPCD    |   RT    |   SPR   |#########|	  EO	     |R|  p.66
       +-----------+---------+---------+---------+-------------------+-+
    78 |   OPCD    |   RT    |#########|#########|	  EO	     |R|  p.67
       +-----------+---------+---------+---------+-------------------+-+
    79 |   OPCD    |   RS    |#########|#########|	  EO	     |R|  p.67
       +-----------+---------+---------+---------+-------------------+-+
   176 |   OPCD    |   FRT   |   RZ    |   RB    |	  EO	     |R|  p.82
       +-----------+---------+---------+---------+-------------------+-+
   177 |   OPCD    |   FRS   |   RZ    |   RB    |	  EO	     |R|  p.87
       +-----------+---------+---------+---------+-------------------+-+
   178 |   OPCD    |   FRT   |#########|   FRB   |	  EO	     |R|  p.91
       +-----------+-----+---+---------+---------+-------------------+-+
   179 |   OPCD    | BF  |###|   FRA   |   FRB   |	  EO	     |R|  p.97
       +-----------+-----+---+---------+---------+-------------------+-+
   180 |   OPCD    |   FRT   |#########|#########|	  EO	     |R|  p.98
       +-----------+-----+---+-----+---+---------+-------------------+-+
   181 |   OPCD    | BF  |###| BFB |###|#########|	  EO	     |R|  p.98
       +-----------+-----+---+-----+---+-------+-+-------------------+-+
   182 |   OPCD    | BFF |###|#########|  I    |#|	  EO	     |R|  p.99
       +-----------+-----+---+---------+-------+-+-------------------+-+
   183 |   OPCD    |   BTF   |#########|#########|	  EO	     |R|  p.100
       +-----------+---------+-+-------+---------+-------------------+-+
   192 |   OPCD    |   RT    |#| SR    |#########|	  EO	     |R|  p.125
       +-----------+---------+-+-------+---------+-------------------+-+
   193 |   OPCD    |   RT    |   RZ    |   RB	 |	  EO	     |R|  p.20
       +-----------+---------+---------+---------+-------------------+-+
   194 |   OPCD    |   RS    |   RA    |#########|	  EO	     |R|  p.54
       +-----------+---------+---------+---------+-------------------+-+
   195 |   OPCD    |   RS    |   RA    |   RB	 |	  EO	     |R|  p.57
       +-----------+---------+---------+---------+-------------------+-+
   196 |   OPCD    |  SPR_T  |  SPR_F  |#########|	  EO	     |R|  new
       +-----------+---------+---------+---------+-------------------+-+
   197 |   OPCD    | BF2 |###| BFEX|###|    RB   |	  EO	     |R|  new
       +-----------+-----+---+-----+---+---------+-------------------+-+
   198 |   OPCD    | BFF2|###| BFEX|###|   FRB   |	  EO	     |R|  new
       +-----------+-----+---+-----+---+---------+-------------------+-+
   199 |   OPCD    |   RT    |#########|   RB	 |	  EO	     |R| p.442P
       +-----------+---------+---------+---------+---------+---------+-+
   200 |   OPCD    |   RT    |   RA    |   RB	 |   GCA   |  EO     |R|  p.43
       +-----------+---------+---------+---------+---------+---------+-+
   201 |   OPCD    |   RT    |   RA    |   GCA	 |	  EO	     |R|  p.44
       +-----------+---------+---------+---------+-------------------+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+---------+-------------------+-+
XL  80 |   OPCD    |   BT    |   BA    |   BB	 |	  EO	     |L|  p.16
       +-----------+---------+---------+---------+-------------------+-+
    81 |   OPCD    |   BO    |   BI    |#########|	  EO	     |L|  p.13
       +-----------+---------+---------+---------+-------------------+-+
    82 |   OPCD    |#########|#########|#########|	  EO	     |L|  p.14
       +-----------+-----+---+-----+---+---------+-------------------+-+
    83 |   OPCD    | BF  |###| BFA |###|#########|	  EO	     |L|  p.15
       +-----------+-----+---+-----+---+---------+-------------------+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+-----------------+-+-------------------+-+
XFX 96 |   OPCD    |   RS    |        FXM      |#|	  EO	     |R|  p.67
       +-----------+---------+-----------------+-+-------------------+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+-+---------------+-+---------+-------------------+-+
XFL 112|   OPCD    |#|      FLM      |#|   FRB   |	  EO	     |R|  p.99
       +-----------+-+---------------+-+---------+-------------------+-+

************************
 OBSOLETE:  128 and 129
************************
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+---------+--+----------------+-+
XO  128|   OPCD    |   RT    |   RA    |   RB	 |OE|	   EO'	     |R|
       +-----------+---------+---------+---------+--+----------------+-+
    129|   OPCD    |   RT    |   RA    |#########|OE|	   EO'	     |R|
       +-----------+---------+---------+---------+--+----------------+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+---------+---------+---------+-+
A   144|   OPCD    |   FRT   |   FRA   |   FRB	 |   FRC   |   XO    |R|  p.95
       +-----------+---------+---------+---------+---------+---------+-+
    145|   OPCD    |   FRT   |   FRA   |   FRB	 |#########|   XO    |R|  p.92
       +-----------+---------+---------+---------+---------+---------+-+
    146|   OPCD    |   FRT   |   FRA   |#########|   FRC   |   XO    |R|  p.93
       +-----------+---------+---------+---------+---------+---------+-+

	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-----------+---------+---------+---------+---------+---------+-+
M   160|   OPCD    |   RS    |   RA    |   RB	 |   MB    |   ME    |R|  p.56
       +-----------+---------+---------+---------+---------+---------+-+
    161|   OPCD    |   RS    |   RA    |   SH	 |   MB    |   ME    |R|  p.56
       +-----------+---------+---------+---------+---------+---------+-+
    162|   OPCD    |   RS    |   RA    |   RB	 |   MB    |   ME    |R|  p.56
       +-----------+---------+---------+---------+---------+---------+-+
    163|   OPCD    |   RS    |   RA    |   SH	 |   MB    |   ME    |R|  p.56
       +-----------+---------+---------+---------+---------+---------+-+
*/

PPC_OP_FMT ppc_op_fmt_base[] = {
{  0,-1, 3, {0, 2, 1}, {{RT, 21, 1, 0, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {D,   0, 0, 1, 0xFFFF}}},

{  1,-1, 3, {0, 2, 1}, {{RTM,21, 1, 0, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {D,   0, 0, 1, 0xFFFF}}},

{  2,-1, 3, {0, 1, 2}, {{RT, 21, 1, 0, 0x1F},
		        {RZ, 16, 0, 1, 0x1F},
		        {UI,  0, 0, 1, 0xFFFF}}},

{  3,-1, 3, {0, 1, 2}, {{TO, 21, 0, 1, 0x1F}, 
		        {RA, 16, 0, 1, 0x1F},
		        {SI,  0, 0, 1, 0xFFFF}}},

{  4,-1, 3, {0, 2, 1}, {{RSM,21, 0, 1, 0x1F},
		        {RZ, 16, 0, 1, 0x1F},
		        {D,   0, 0, 1, 0xFFFF}}},

{  5,-1, 3, {0, 2, 1}, {{RS, 21, 0, 1, 0x1F},
		        {RZ, 16, 0, 1, 0x1F},
		        {D,   0, 0, 1, 0xFFFF}}},

{  6,-1, 3, {0, 1, 2}, {{RT, 21, 1, 0, 0x1F},
		        {RA, 16, 0, 1, 0x1F},
		        {SI,  0, 0, 1, 0xFFFF}}},

{  7,-1, 3, {0, 1, 2}, {{BF, 23, 1, 0, 0x07},
		        {RA, 16, 0, 1, 0x1F},
		        {SI,  0, 0, 1, 0xFFFF}}},

{  8,-1, 3, {0, 1, 2}, {{BF, 23, 1, 0, 0x07},
		        {RA, 16, 0, 1, 0x1F},
		        {UI,  0, 0, 1, 0xFFFF}}},

{  9,-1, 3, {1, 0, 2}, {{RS, 21, 0, 1, 0x1F},
		        {RA, 16, 1, 0, 0x1F},
		        {UI,  0, 0, 1, 0xFFFF}}},

{ 10,-1, 3, {0, 2, 1}, {{FRT,21, 1, 0, 0x1F},
		        {RZ, 16, 0, 1, 0x1F},
		        {D,   0, 0, 1, 0xFFFF}}},

{ 11,-1, 3, {0, 2, 1}, {{FRS,21, 0, 1, 0x1F},
		        {RZ, 16, 0, 1, 0x1F},
		        {D,   0, 0, 1, 0xFFFF}}},

{ 16,-1, 3, {0, 1, 2}, {{BO, 21, 1, 1, 0x1F},
		        {BI, 16, 0, 1, 0x1F},
		        {BD,  2, 0, 1, 0x3FFF}}},

{ 32,-1, 1, {0},       {{LI,  2, 0, 1, 0xFFFFFF}}},

{ 48,-1, 3, {1, 0, 2}, {{FL1,12, 0, 1, 0x0F},
		        {LEV, 4, 0, 1, 0xFF},
		        {FL2, 2, 0, 1, 0x03}}},

{ 49,-1, 1, {0},       {{SV,  2, 0, 1, 0x3FFF}}},

{ 64,-1, 3, {0, 1, 2}, {{RT, 21, 1, 0, 0x1F}, 
		        {RA, 16, 0, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{ 65,-1, 2, {0, 1},    {{RA, 16, 0, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{ 66,-1, 1, {0},       {{BF, 23, 1, 0, 0x07}}},

{ 67,-1, 3, {1, 0, 2}, {{RS, 21, 0, 1, 0x1F}, 
		        {RA, 16, 1, 0, 0x1F}, 
		        {SH, 11, 0, 1, 0x1F}}},

{ 68,-1, 2, {1, 0},    {{RS, 21, 0, 1, 0x1F}, 
		        {SPR_T,16, 1, 0, 0x1F}}},

{ 69,-1, 3, {0, 1, 2}, {{TO, 21, 0, 1, 0x1F}, 
		        {RA, 16, 0, 1, 0x1F},
		        {RB, 11, 0, 1, 0x1F}}},

{ 70,-1, 2, {1, 0},    {{RS, 21, 0, 1, 0x1F}, 
		        {SR, 16, 1, 0, 0x0F}}},

{ 71,-1, 3, {0, 1, 2}, {{RS, 21, 0, 1, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{ 72,-1, 3, {0, 1, 2}, {{RTS,21, 1, 0, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {NB, 11, 0, 1, 0x1F}}},

{ 73,-1, 3, {0, 1, 2}, {{RSS,21, 0, 1, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {NB, 11, 0, 1, 0x1F}}},

{ 74,-1, 3, {0, 1, 2}, {{BF, 23, 1, 0, 0x07},
		        {RA, 16, 0, 1, 0x1F},
		        {RB, 11, 0, 1, 0x1F}}},

{ 75,-1, 3, {1, 0, 2}, {{RS, 21, 0, 1, 0x1F}, 
		        {RA, 16, 1, 0, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{ 76,-1, 2, {0, 1},    {{RT, 21, 1, 0, 0x1F}, 
		        {RA, 16, 0, 1, 0x1F}}},

{ 77,-1, 2, {0, 1},    {{RT, 21, 1, 0, 0x1F}, 
		        {SPR_F,16, 0, 1, 0x1F}}},

{ 78,-1, 1, {0},       {{RT, 21, 1, 0, 0x1F}}},

{ 79,-1, 1, {0},       {{RS, 21, 0, 1, 0x1F}}},

{176,-1, 3, {0, 1, 2}, {{FRT,21, 1, 0, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{177,-1, 3, {0, 1, 2}, {{FRS,21, 0, 1, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{178,-1, 2, {0, 1},    {{FRT,21, 1, 0, 0x1F}, 
		        {FRB,11, 0, 1, 0x1F}}},

{179,-1, 3, {0, 1, 2}, {{BF, 23, 1, 0, 0x07}, 
		        {FRA,16, 0, 1, 0x1F}, 
		        {FRB,11, 0, 1, 0x1F}}},

{180,-1, 1, {0},       {{FRT,21, 1, 0, 0x1F}}}, 

{181,-1, 2, {0, 1},    {{BF, 23, 1, 0, 0x07}, 
		        {BFB,18, 0, 1, 0x07}}},

{182,-1, 2, {0, 1},    {{BFF, 23, 0, 1, 0x07}, 
		        {I,   12, 0, 1, 0x0F}}},

{183,-1, 1, {0},       {{BTF, 21, 0, 1, 0x1F}}},

{192,-1, 2, {0, 1},    {{RT, 21, 1, 0, 0x1F}, 
		        {SR, 16, 0, 1, 0x0F}}},

{193,-1, 3, {0, 1, 2}, {{RT, 21, 1, 0, 0x1F}, 
		        {RZ, 16, 0, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{194,-1, 2, {1, 0},    {{RS, 21, 0, 1, 0x1F}, 
		        {RA, 16, 1, 0, 0x1F}}},

{195, 1, 3, {1, 0, 2}, {{RS, 21, 0, 1, 0x1F}, 
		        {RA, 16, 1, 1, 0x1F}, 
		        {RB, 11, 0, 1, 0x1F}}},

{196,-1, 2, {0, 1},    {{SPR_T,21, 1, 0, 0x1F}, 
		        {SPR_F,16, 0, 1, 0x1F}}},

{197,-1, 3, {0, 1, 2}, {{BF2,  23, 1, 0, 0x07},
		        {BFEX, 18, 0, 1, 0x07},
		        {RB,   11, 0, 1, 0x1F}}},

{198,-1, 2, {0, 1, 2}, {{BFF2, 23, 1, 0, 0x07},
		        {BFEX, 18, 0, 1, 0x07},
		        {FRB,  11, 0, 1, 0x1F}}},

{199,-1, 2, {0, 1}, {{RT, 21, 1, 0, 0x1F}, 
		     {RB, 11, 0, 1, 0x1F}}},


{200,-1, 4, {0, 1, 2, 3}, {{RT,  21, 1, 0, 0x1F}, 
		           {RA,  16, 0, 1, 0x1F}, 
		           {RB,  11, 0, 1, 0x1F},
		           {GCA,  6, 0, 1, 0x1F}}},

{201,-1, 3, {0, 1, 2},    {{RT,  21, 1, 0, 0x1F}, 
		           {RA,  16, 0, 1, 0x1F}, 
		           {GCA, 11, 0, 1, 0x1F}}},

{ 80,-1, 3, {0, 1, 2}, {{BT, 21, 1, 0, 0x1F}, 
		        {BA, 16, 0, 1, 0x1F}, 
		        {BB, 11, 0, 1, 0x1F}}},

{ 81,-1, 2, {0, 1},    {{BO, 21, 1, 1, 0x1F},
		        {BI, 16, 0, 1, 0x1F}}},

{ 82,-1, 0},

{ 83,-1, 2, {0, 1},    {{BF, 23, 1, 0, 0x07}, 
		        {BFA,18, 0, 1, 0x07}}},

{ 96,-1, 2, {1, 0},    {{RS, 21, 0, 1, 0x1F}, 
		        {FXM,12, 1, 0, 0xFF}}},

{112,-1, 2, {0, 1},    {{FLM,17, 0, 1, 0xFF}, 
		        {FRB,11, 1, 0, 0x1F}}},

{144,-1, 4, {0, 1, 2, 3}, {{FRT, 21, 1, 0, 0x1F}, 
		           {FRA, 16, 0, 1, 0x1F}, 
		           {FRB, 11, 0, 1, 0x1F},
		           {FRC,  6, 0, 1, 0x1F}}},

{145,-1, 3, {0, 1, 2}, {{FRT, 21, 1, 0, 0x1F}, 
		        {FRA, 16, 0, 1, 0x1F}, 
		        {FRB, 11, 0, 1, 0x1F}}},

{146,-1, 3, {0, 1, 2}, {{FRT, 21, 1, 0, 0x1F}, 
		        {FRA, 16, 0, 1, 0x1F}, 
		        {FRC,  6, 0, 1, 0x1F}}},

{160,-1, 5, {1, 0, 2, 3, 4}, {{RS, 21, 0, 1, 0x1F}, 
			      {RA, 16, 1, 0, 0x1F}, 
			      {RB, 11, 0, 1, 0x1F}, 
			      {MB,  6, 0, 1, 0x1F}, 
			      {ME,  1, 0, 1, 0x1F}}},

{161,-1, 5, {1, 0, 2, 3, 4}, {{RS, 21, 0, 1, 0x1F}, 
			      {RA, 16, 1, 0, 0x1F}, 
			      {SH, 11, 0, 1, 0x1F}, 
			      {MB,  6, 0, 1, 0x1F}, 
			      {ME,  1, 0, 1, 0x1F}}},

{162, 1, 5, {1, 0, 2, 3, 4}, {{RS, 21, 0, 1, 0x1F}, 
			      {RA, 16, 1, 1, 0x1F}, 
			      {RB, 11, 0, 1, 0x1F}, 
			      {MB,  6, 0, 1, 0x1F}, 
			      {ME,  1, 0, 1, 0x1F}}},

{163, 1, 5, {1, 0, 2, 3, 4}, {{RS, 21, 0, 1, 0x1F}, 
			      {RA, 16, 1, 1, 0x1F}, 
			      {SH, 11, 0, 1, 0x1F}, 
			      {MB,  6, 0, 1, 0x1F}, 
			      {ME,  1, 0, 1, 0x1F}}}
};

int num_ppc_fmts = sizeof (ppc_op_fmt_base) / sizeof (ppc_op_fmt_base[0]);

