/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*
   Taken from (1) POWER Processor Architecture Version 1.52
   February 15, 1990, and positive page numbers refer to
   this document, and (2) "The PowerPC Architecture:  A Specification
   for a New Family of RISC Processors", May 1994 (2nd Edition),
   by Cathy May, Ed Silha, Rick Simpson, and Hank Warren.
   Negative page numbers refer to this document.
*/

#include "dis.h"
#include "dis_tbl.h"
#include "opcode_fmt.h"
#include "latency.h"

OPCODE1 for_ops[] = {
{18,E1, 0,	32,	2,	OP_B,		"b",		12, 0, 0, 0, 0,
1, {IAR}, 1, {INSADR}, {1}},

{18,E1,	2,	32,	2,	OP_BA,		"ba",		12, 0, 0, 0, 0,
1, {IAR}, 0, {NOTHING}, {1}},

{18,E1,	1,	32,	2,	OP_BL,		"bl",		12, 0, 0, 0, 0,
2, {IAR, LR}, 1, {INSADR}, {1, 1}},

{18,E1,	3,	32,	2,	OP_BLA,		"bla",		12, 0, 0, 0, 0,
2, {IAR, LR}, 0, {1,1}},

{16,E1,	0,	16,	2,	OP_BC,		"bc",		12, 0, 0, 0, 0,
2, {IAR, BO}, 3, {INSADR, BI, BO}, {L1,L1}},

{16,E1,	2,	16,	2,	OP_BCA,		"bca",		12, 0, 0, 0, 0,
2, {IAR, BO}, 2, {BI, BO}, {L1,L1}},

{16,E1,	1,	16,	2,	OP_BCL,		"bcl",		12, 0, 0, 0, 0,
3, {IAR, BO, LR},  3, {INSADR, BI, BO}, {L1,L1,L1}},

{16,E1,	3,	16,	2,	OP_BCLA,	"bcla",		12, 0, 0, 0, 0,
3, {IAR, BO, LR},  2, {BI, BO}, {L1,L1,L1}},

{19,E1,	32,	81,	11,	OP_BCR,		"bcr",		13, 0, 0, 0, 0,
2, {IAR, BO}, 3, {LR, BI, BO}, {L1,L1}},

{19,E1,	33,	81,	11,	OP_BCRL,	"bcrl",		13, 0, 0, 0, 0,
3, {IAR, BO, LR}, 3, {LR, BI, BO}, {L1,L1,L1}},

{19,E1,	34,	81,	11,	OP_BCR2,	"bcr2",		13, 0, 0, 0, 0,
2, {IAR, BO}, 3, {LR2, BI, BO}, {L1,L1}},

{19,E1,	35,	81,	11,	OP_BCR2L,	"bcr2l",	13, 0, 0, 0, 0,
3, {IAR, BO, LR}, 3, {LR2, BI, BO}, {L1,L1,L1}},

{19,E1,	1056,	81,	11,	OP_BCC,		"bcc",		13, 0, 0, 0, 0,
2, {IAR, BO}, 3, {CTR, BI, BO}, {L1,L1}},

{19,E1,	1057,	81,	11,	OP_BCCL,	"bccl",		13, 0, 0, 0, 0,
3, {IAR, BO, LR},  3, {CTR, BI, BO}, {L1,L1,L1}},

{17,E2,	0,	48,	2,	OP_SVC,		"svc",		14, 0, 0, 0, 0,
5, {IAR, CTR, EE, PR, MSRFE}, 16, {MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
			          MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
   {L2,  L2,  L2, L2, L2}},

{17,E2,	1,	48,	2,	OP_SVCL,	"svcl",		14, 0, 0, 0, 0,
6, {IAR, CTR, LR, EE, PR, MSRFE}, 16, {MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
				      MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
   {L2,  L2,  L2, L2, L2, L2}},

{17,E2,	2,	49,	2,	OP_SVCA,	"svca",		14, 0, 1, 0, 0,
5, {IAR, CTR, EE, PR, MSRFE}, 16, {MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
			          MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
   {L2,  L2,  L2, L2, L2}},

{17,E2,	3,	49,	2,	OP_SVCLA,	"svcla",	14, 0, 1, 0, 0,
6, {IAR, CTR, LR, EE, PR, MSRFE}, 16, {MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
			              MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
   {L2,  L2,  L2, L2, L2, L2}},

{19,E3,	164,	82,	11,	OP_RFSVC,	"rfsvc",	14, 1, 0, 0, 0,
17, {IAR, MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
          MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31}, 2, {CTR, LR},
   {L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3}},

{19,E3,	165,	82,	11,	OP_RFSVC,	"rfsvc",	14, 1, 0, 0, 0,
18, {IAR, LR, MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
              MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31}, 2, {CTR, LR},
   {L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3,L3}},

{19,E4,	100,	82,	11,	OP_RFI,		"rfi",		15, 1, 0, 0, 0,
18, {IAR, MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
          MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31}, 2, {SRR0, SRR1},
   {L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4}},

{19,E4,	101,	82,	11,	OP_RFI,		"rfi",		15, 1, 0, 0, 0,
18, {IAR, LR, MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
              MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31}, 2,{SRR0,SRR1},
   {L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4,L4}},

{19,E5,	0,	83,	11,	OP_MCRF,	"mcrf",		15, 0, 0, 0, 0,
1, {BF}, 1, {BFA}, {L5}},
{19,E5,	1,	83,	11,	OP_MCRF,	"mcrf",		15, 0, 0, 0, 0,
2, {BF, LR}, 1, {BFA}, {L5,L5}},

{19,E6,	578,	80,	11,	OP_CREQV,	"creqv",	16, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	579,	80,	11,	OP_CREQV,	"creqv",	16, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	514,	80,	11,	OP_CRAND,	"crand",	16, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	515,	80,	11,	OP_CRAND,	"crand",	16, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	386,	80,	11,	OP_CRXOR,	"crxor",	16, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	387,	80,	11,	OP_CRXOR,	"crxor",	16, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	898,	80,	11,	OP_CROR,	"cror",		16, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	899,	80,	11,	OP_CROR,	"cror",		16, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	258,	80,	11,	OP_CRANDC,	"crandc",	17, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	259,	80,	11,	OP_CRANDC,	"crandc",	17, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	450,	80,	11,	OP_CRNAND,	"crnand",	17, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	451,	80,	11,	OP_CRNAND,	"crnand",	17, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	834,	80,	11,	OP_CRORC,	"crorc",	17, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	835,	80,	11,	OP_CRORC,	"crorc",	17, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{19,E6,	 66,	80,	11,	OP_CRNOR,	"crnor",	17, 0, 0, 0, 0,
1, {BT}, 2, {BA, BB}, {L6}},
{19,E6,	 67,	80,	11,	OP_CRNOR,	"crnor",	17, 0, 0, 0, 0,
2, {BT, LR}, 2, {BA, BB}, {L6,L6}},

{34,E7,	 0,	0,	0,	OP_LBZ,		"lbz",		20, 0, 0, 0, 0,
1, {RT}, 3, {RZ, DCACHE, MEM}, {L7}},
{31,E7,	 174,	193,	11,	OP_LBZX,	"lbzx",		20, 0, 0, 0, 0,
1, {RT}, 4, {RZ, RB, DCACHE, MEM}, {L7}},
{31,E7,	 175,	193,	11,	OP_LBZX,	"lbzx",		20, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 4, {RZ, RB, DCACHE, MEM},
   {L7, C7,  C7,  C7,  C7}},

{40,E7,	 0,	0,	0,	OP_LHZ,		"lhz",		21, 0, 0, 0, 0,
1, {RT}, 4, {RZ, DCACHE, MEM, AL}, {L7}},
{31,E7,	 558,	193,	11,	OP_LHZX,	"lhzx",		21, 0, 0, 0, 0,
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7}},
{31,E7,	 559,	193,	11,	OP_LHZX,	"lhzx",		21, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, C7,  C7,  C7,  C7}},

{42,E7,	 0,	0,	0,	OP_LHA,		"lha",		22, 0, 0, 0, 0,
1, {RT}, 4, {RZ, DCACHE, MEM, AL}, {L7}},
{31,E7,	 686,	193,	11,	OP_LHAX,	"lhax",		22, 0, 0, 0, 0,
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7}},
{31,E7,	 687,	193,	11,	OP_LHAX,	"lhax",		22, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, C7,  C7,  C7,  C7}},

{32,E7,	 0,	0,	0,	OP_L,		"l",		23, 0, 0, 0, 0,
1, {RT}, 4, {RZ, DCACHE, MEM, AL}, {L7}},
{31,E7,	 46,	193,	11,	OP_LX,		"lx",		23, 0, 0, 0, 0,
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7}},
{31,E7,	 47,	193,	11,	OP_LX,		"lx",		23, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, C7,  C7,  C7,  C7}},
{31,E7,	 40,	193,	11,	OP_LWARX,	"lwarx",       -77, 0, 0, 0, 0,
2, {RT, ATOMIC_RES}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7, L7}},

{31,E7,	 1580,	193,	11,	OP_LHBRX,	"lhbrx",	24, 0, 0, 0, 0,
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7}},
{31,E7,	 1581,	193,	11,	OP_LHBRX,	"lhbrx",	24, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, C7,  C7,  C7,  C7}},
{31,E7,	 1068,	193,	11,	OP_LBRX,	"lbrx",		24, 0, 0, 0, 0,
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7}},
{31,E7,	 1069,	193,	11,	OP_LBRX,	"lbrx",		24, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, C7,  C7,  C7,  C7}},

{46,E8,	 0,	1,	0,	OP_LM,		"lm",		25, 0, 0, 1, 0,
1, {RTM}, 4, {RZ, DCACHE, MEM, AL}, {L8}},

{38,E9,	 0,	5,	0,	OP_STB,		"stb",		26, 0, 0, 0, 0,
2, {DCACHE, MEM}, 2, {RS, RZ}, {L9, L9}},
{31,E9,	 430,	71,	11,	OP_STBX,	"stbx",		26, 0, 0, 0, 0,
2, {DCACHE, MEM}, 3, {RS, RZ, RB}, {L9, L9}},
{31,E9,	 431,	71,	11,	OP_STBX,	"stbx",		26, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 3, {RS, RZ, RB},
   {L9,     L9,  C9,  C9,  C9,  C9}},

{44,E9,	 0,	5,	0,	OP_STH,		"sth",		27, 0, 0, 0, 0,
2, {DCACHE, MEM}, 3, {RS, RZ, AL}, {L9, L9}},
{31,E9,	 814,	71,	11,	OP_STHX,	"sthx",		27, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {L9, L9}},
{31,E9,	 815,	71,	11,	OP_STHX,	"sthx",		27, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {L9,     L9,  C9,  C9,  C9,  C9}},

{36,E9,	 0,	5,	0,	OP_ST,		"st",		28, 0, 0, 0, 0,
2, {DCACHE, MEM}, 3, {RS, RZ, AL}, {L9, L9}},
{31,E9,	 302,	71,	11,	OP_STX,		"stx",		28, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {L9, L9}},
{31,E9,	 303,	71,	11,	OP_STX,		"stx",		28, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {L9,     L9,  C9,  C9,  C9,  C9}},
{31,E9,	 301,	71,	11,	OP_STWCX_,	"stwcx.",      -78, 0, 0, 0, 0,
2, {DCACHE, MEM}, 5, {RS, RZ, RB, AL, ATOMIC_RES}, {L9, L9}},

{31,E9,	 1836,	71,	11,	OP_STHBRX,	"sthbrx",	29, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {L9, L9}},
{31,E9,	 1837,	71,	11,	OP_STHBRX,	"sthbrx",	29, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {L9,     L9,  C9,  C9,  C9,  C9}},
{31,E9,	 1324,	71,	11,	OP_STBRX,	"stbrx",	29, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {L9, L9}},
{31,E9,	 1325,	71,	11,	OP_STBRX,	"stbrx",	29, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {L9,     L9,  C9,  C9,  C9,  C9}},

{47,E10, 0,	4,	0,	OP_STM,		"stm",		30, 0, 0, 1, 0,
2, {DCACHE, MEM}, 3, {RSM, RZ, AL}, {L10, L10}},

{35,E7,	 0,	0,	0,	OP_LBZU,	"lbzu",		31, 0, 0, 0, 0,
2, {RT, RZ}, 3, {RZ, DCACHE, MEM}, {L7,U7}},
{31,E7,	 238,	193,	11,	OP_LBZUX,	"lbzux",	31, 0, 0, 0, 0,
2, {RT, RZ}, 4, {RZ, RB, DCACHE, MEM}, {L7,U7}},
{31,E7,	 239,	193,	11,	OP_LBZUX,	"lbzux",	31, 0, 0, 0, 0,
6, {RT, RZ, CR0, CR1, CR2, CR3}, 4, {RZ, RB, DCACHE, MEM},
   {L7, U7, C7,  C7,  C7,  C7}},

{41,E7,	 0,	0,	0,	OP_LHZU,	"lhzu",		32, 0, 0, 0, 0,
2, {RT, RZ}, 4, {RZ, DCACHE, MEM, AL}, {L7,U7}},
{31,E7,	 622,	193,	11,	OP_LHZUX,	"lhzux",	32, 0, 0, 0, 0,
2, {RT, RZ}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7,U7}},
{31,E7,	 623,	193,	11,	OP_LHZUX,	"lhzux",	32, 0, 0, 0, 0,
6, {RT, RZ, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, U7, C7,  C7,  C7,  C7}},

{43,E7,	 0,	0,	0,	OP_LHAU,	"lhau",		33, 0, 0, 0, 0,
2, {RT, RZ}, 4, {RZ, DCACHE, MEM, AL}, {L7,U7}},
{31,E7,	 750,	193,	11,	OP_LHAUX,	"lhaux",	33, 0, 0, 0, 0,
2, {RT, RZ}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7,U7}},
{31,E7,	 751,	193,	11,	OP_LHAUX,	"lhaux",	33, 0, 0, 0, 0,
6, {RT, RZ, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, U7, C7,  C7,  C7,  C7}},

{33,E7,	 0,	0,	0,	OP_LU,		"lu",		34, 0, 0, 0, 0,
2, {RT, RZ}, 4, {RZ, DCACHE, MEM, AL}, {L7,U7}},
{31,E7,	 110,	193,	11,	OP_LUX,		"lux",		34, 0, 0, 0, 0,
2, {RT, RZ}, 5, {RZ, RB, DCACHE, MEM, AL}, {L7,U7}},
{31,E7,	 111,	193,	11,	OP_LUX,		"lux",		34, 0, 0, 0, 0,
6, {RT, RZ, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L7, U7, C7,  C7,  C7,  C7}},

{39,E9,	 0,	5,	0,	OP_STBU,	"stbu",		35, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 2, {RS, RZ}, {U9, L9, L9}},
{31,E9,	 494,	71,	11,	OP_STBUX,	"stbux",	35, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 3, {RS, RZ, RB}, {U9, L9, L9}},
{31,E9,	 495,	71,	11,	OP_STBUX,	"stbux",	35, 0, 0, 0, 0,
7, {RZ, DCACHE, MEM, CR0, CR1, CR2, CR3}, 3, {RS, RZ, RB},
   {U9, L9,     L9,  C9,  C9,  C9,  C9}},

{45,E9,	 0,	5,	0,	OP_STHU,	"sthu",		36, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 3, {RS, RZ, AL}, {U9, L9, L9}},
{31,E9,	 878,	71,	11,	OP_STHUX,	"sthux",	36, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {U9, L9, L9}},
{31,E9,	 879,	71,	11,	OP_STHUX,	"sthux",	36, 0, 0, 0, 0,
7, {RZ, DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {U9, L9,     L9,  C9,  C9,  C9,  C9}},

{37,E9,	 0,	5,	0,	OP_STU,		"stu",		37, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 3, {RS, RZ, AL}, {U9, L9, L9}},
{31,E9,	 366,	71,	11,	OP_STUX,	"stux",		37, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {U9, L9, L9}},
{31,E9,	 367,	71,	11,	OP_STUX,	"stux",		37, 0, 0, 0, 0,
7, {RZ, DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {U9, L9,     L9,  C9,  C9,  C9,  C9}},

/*** For now, lsx is always serializing, so give it a simpler template ***/
{31,E11, 1066,	193,	11,	OP_LSX,	"lsx",			38, 0, 0, 1, 0,
#ifdef WHY_WOULD_WE_WANT_THIS_IF_WE_CANT_SCHEDULE_LSCBX_OPS_IN_VLIW
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L11}},
#else
32, {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,R11,R12,R13,R14,R15,
     R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31},
11, {RZ, RB, DCACHE, MEM, XER25, XER26, XER27, XER28, XER29, XER30, XER31},
    {L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11}},
#endif

{31,E11, 1067,	193,	11,	OP_LSX,	"lsx",			38, 0, 0, 1, 0,
#ifdef WHY_WOULD_WE_WANT_THIS_IF_WE_CANT_SCHEDULE_LSCBX_OPS_IN_VLIW
5, {RT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL}},
#else
36, {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,R11,R12,R13,R14,R15,
     R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31,
     CR0, CR1, CR2, CR3},
11, {RZ, RB, DCACHE, MEM, XER25, XER26, XER27, XER28, XER29, XER30, XER31},
    {L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     C11,C11,C11,C11}},
#endif

{31,E12, 1194,	72,	11,	OP_LSI,		"lsi",		38, 0, 0, 1, 0,
1, {RTS}, 3, {RZ, DCACHE, MEM}, {L12}},
{31,E12, 1195,	72,	11,	OP_LSI,		"lsi",		38, 0, 0, 1, 0,
5, {RTS, CR0, CR1, CR2, CR3}, 3, {RZ, DCACHE, MEM}, {L12,C12,C12,C12,C12}},

/*** For now, lscbx is always serializing, so give it a simpler template ***/
{31,E11, 554,	193,	11,	OP_LSCBX,	"lscbx",	39, 0, 0, 1, 0,
#ifdef WHY_WOULD_WE_WANT_THIS_IF_WE_CANT_SCHEDULE_LSCBX_OPS_IN_VLIW
1, {RT}, 5, {RZ, RB, DCACHE, MEM, AL}, {2}},
#else
39, {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,R11,R12,R13,R14,R15,
     R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31,
     XER25, XER26, XER27, XER28, XER29, XER30, XER31},
19, {RZ, RB, DCACHE, MEM, XER25, XER26, XER27, XER28, XER29, XER30, XER31,
     XER16, XER17, XER18, XER19, XER20, XER21, XER22, XER23},
    {L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     X11,X11,X11,X11,X11,X11,X11}},
#endif

{31,E11, 555,	193,	11,	OP_LSCBX_,	"lscbx.",	39, 0, 0, 1, 0,
#ifdef WHY_WOULD_WE_WANT_THIS_IF_WE_CANT_SCHEDULE_LSCBX_OPS_IN_VLIW
5, {RT,  CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L11, C11, C11, C11, C11}},
#else
43, {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,R11,R12,R13,R14,R15,
     R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31,
     XER25, XER26, XER27, XER28, XER29, XER30, XER31, CR0, CR1, CR2, CR3},
19, {RZ, RB, DCACHE, MEM, XER25, XER26, XER27, XER28, XER29, XER30, XER31,
     XER16, XER17, XER18, XER19, XER20, XER21, XER22, XER23},
    {L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,L11,
     X11,X11,X11,X11,X11,X11,X11,C11,C11,C11,C11}},
#endif

/*** For now, stsx is always serializing, so give it a simpler template ***/
{31,E13, 1322,	71,	11,	OP_STSX,	"stsx",		40, 0, 0, 1, 0,
#ifdef WHY_WOULD_WE_WANT_THIS_IF_WE_CANT_SCHEDULE_LSCBX_OPS_IN_VLIW
2, {DCACHE, MEM}, 4, {RS, RZ, RB, AL}, {L13,L13}},
#else
 3, {MQ, DCACHE, MEM},
39, {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,R11,R12,R13,R14,R15,
     R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31,
     XER25, XER26, XER27, XER28, XER29, XER30, XER31},
    {M13,L13,    L13}},
#endif

{31,E13, 1323,	71,	11,	OP_STSX,	"stsx",		40, 0, 0, 1, 0,
#ifdef WHY_WOULD_WE_WANT_THIS_IF_WE_CANT_SCHEDULE_LSCBX_OPS_IN_VLIW
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL}, 
   {L13,    L13, C13, C13, C13, C13}},
#else
 7, {MQ, DCACHE, MEM, CR0, CR1, CR2, CR3},
39, {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10,R11,R12,R13,R14,R15,
     R16,R17,R18,R19,R20,R21,R22,R23,R24,R25,R26,R27,R28,R29,R30,R31,
     XER25, XER26, XER27, XER28, XER29, XER30, XER31},
    {M13,L13,    L13, C13, C13, C13, C13}},
#endif

{31,E14, 1450,	73,	11,	OP_STSI,	"stsi",		40, 0, 0, 1, 0,
3, {MQ, DCACHE, MEM}, 2, {RSS, RZ}, {1,L14,L14}},
{31,E14, 1451,	73,	11,	OP_STSI,	"stsi",		40, 0, 0, 1, 0,
7, {MQ, DCACHE, MEM, CR0, CR1, CR2, CR3}, 2, {RSS, RZ},
   {M14,L14,    L14, C14, C14, C14, C14}},

{14,E15, 0,	0,	0,	OP_CAL,		"cal",		41, 0, 0, 0, 0,
1, {RT}, 1, {RZ}, {L15}},

{15,E15, 0,	2,	0,	OP_CAU,		"cau",		41, 0, 0, 0, 0,
1, {RT}, 1, {RZ}, {L15}},

{31,E15, 532,	 64,	11,	OP_CAX,		"cax",		41, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L15}},
{31,E15, 533,	 64,	11,	OP_CAX_,	"cax.",		41, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L15, C15, C15, C15, C15}},
{31,E15, 1556,	 64,	11,	OP_CAXO,	"caxo",		41, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L15, O15, O15}},
{31,E15,1557,	 64,	11,	OP_CAXO_,	"caxo.",	41, 0, 0, 0, 0,
7, {RT, SO, OV, CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L15,O15,O15,C15, C15, C15, C15}},

#ifdef IGNORE_CA_FOR_AI
{12,E15, 0,	6,	0,	OP_AI,		"ai",		42, 0, 0, 0, 0,
1, {RT}, 1, {RA}, {L15}},
{13,E15, 0,	6,	0,	OP_AI_,		"ai.",		42, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {RA}, {L15, C15, C15, C15, C15}},
#else
{12,E15, 0,	6,	0,	OP_AI,		"ai",		42, 0, 0, 0, 0,
2, {RT, CA}, 1, {RA}, {L15, O15}},
{13,E15, 0,	6,	0,	OP_AI_,		"ai.",		42, 0, 0, 0, 0,
6, {RT, CA, CR0, CR1, CR2, CR3}, 1, {RA}, {L15, O15, C15, C15, C15, C15}},
#endif
{8,E15,	 0,	6,	0,	OP_SFI,		"sfi",		42, 0, 0, 0, 0,
2, {RT, CA}, 1, {RA}, {L15, O15}},

{31,E15, 20,	64,	11,	OP_A,		"a",		43, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, RB}, {L15, O15}},
{31,E15, 21,	64,	11,	OP_A_,		"a.",		43, 0, 0, 0, 0,
6, {RT, CA, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L15, O15, C15, C15, C15, C15}},
{31,E15, 1044,	64,	11,	OP_AO,		"ao",		43, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, RB}, {L15, O15, O15, O15}},
{31,E15, 1045,	64,	11,	OP_AO_,		"ao.",		43, 0, 0, 0, 0,
8, {RT, CA, SO, OV, CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L15,O15,O15,O15,C15, C15, C15, C15}},

{31,E16, 276,	64,	11,	OP_AE,		"ae",		43, 0, 0, 0, 0,
2, {RT, CA}, 3, {RA, RB, CA}, {L16, O16}},
{31,E16, 277,	64,	11,	OP_AE_,		"ae.",		43, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, C16, C16, C16, C16}},
{31,E16, 1300,	64,	11,	OP_AEO,		"aeo",		43, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 3, {RA, RB, CA}, {L16, O16, O16, O16}},
{31,E16, 1301,	64,	11,	OP_AEO_,	"aeo.",		43, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},

{31,E15, 16,	64,	11,	OP_SF,		"sf",		43, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, RB}, {L15, O15}},
{31,E15, 17,	64,	11,	OP_SF_,		"sf.",		43, 0, 0, 0, 0,
6, {RT, CA, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L15, O15, C15, C15, C15, C15}},
{31,E15, 1040,	64,	11,	OP_SFO,		"sfo",		43, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, RB}, {1,1,1,1}},
{31,E15, 1041,	64,	11,	OP_SFO_,	"sfo.",		43, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L15, O15, O15, O15, C15, C15, C15, C15}},

{31,E16, 272,	64,	11,	OP_SFE,		"sfe",		43, 0, 0, 0, 0,
2, {RT, CA}, 3, {RA, RB, CA}, {L16, O16}},
{31,E16, 273,	64,	11,	OP_SFE_,	"sfe.",		43, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, C16, C16, C16, C16}},
{31,E16, 1296,	64,	11,	OP_SFEO,	"sfeo",		43, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 3, {RA, RB, CA}, {L16, O16, O16, O16}},
{31,E16, 1297,	64,	11,	OP_SFEO_,	"sfeo.",	43, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},

{31,E15, 80,	64,	11,	OP_SUBF,	"subf",	       -83, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L15}},
{31,E15, 81,	64,	11,	OP_SUBF_,	"subf.",       -83, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L15, C15, C15, C15, C15}},
{31,E15, 1104,	64,	11,	OP_SUBFO,	"subfo",       -83, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L15, O15, O15}},
{31,E15, 1105,	64,	11,	OP_SUBFO_,	"subfo.",      -83, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L15, O15, O15, C15, C15, C15, C15}},

{31,E16, 468,	76,	11,	OP_AME,		"ame",		44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, CA}, {L16, O16}},

{31,E16, 469,	76,	11,	OP_AME_,	"ame.",		44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1492,	76,	11,	OP_AMEO,	"ameo",		44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, CA}, {L16, O16, O16, O16}},

{31,E16, 1493,	76,	11,	OP_AMEO_,	"ameo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},


{31,E16, 404,	76,	11,	OP_AZE,		"aze",		44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, CA}, {L16, O16}},

{31,E16, 405,	76,	11,	OP_AZE_,	"aze.",		44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1428,	76,	11,	OP_AZEO,	"azeo",		44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, CA}, {L16, O16, O16, O16}},

{31,E16, 1429,	76,	11,	OP_AZEO_,	"azeo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},


{31,E16, 464,	76,	11,	OP_SFME,	"sfme",		44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, CA}, {L16, O16}},

{31,E16, 465,	76,	11,	OP_SFME_,	"sfme.",	44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1488,	76,	11,	OP_SFMEO,	"sfmeo",	44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, CA}, {L16, O16, O16, O16}},

{31,E16, 1489,	76,	11,	OP_SFMEO_,	"sfmeo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},


{31,E16, 400,	76,	11,	OP_SFZE,	"sfze",		44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, CA}, {L16, O16}},

{31,E16, 401,	76,	11,	OP_SFZE_,	"sfze.",	44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1424,	76,	11,	OP_SFZEO,	"sfzeo",	44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, CA}, {L16, O16, O16, O16}},

{31,E16, 1425,	76,	11,	OP_SFZEO_,	"sfzeo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},


{9,E15,	 0,	6,	0,	OP_DOZI,	"dozi",		45, 0, 0, 0, 0,
1, {RT}, 1, {RA}, {L15}},


{31,E15, 528,	64,	11,	OP_DOZ,		"doz",		45, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L15}},

{31,E15, 529,	64,	11,	OP_DOZ_,	"doz.",		45, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L15, C15, C15, C15, C15}},

{31,E15,1552,	64,	11,	OP_DOZO,	"dozo",		45, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L15, O15, O15}},

{31,E15,1553,	64,	11,	OP_DOZO_,	"dozo.",	45, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L15, O15, O15, C15, C15, C15, C15}},

{31,E15, 720,	76,	11,	OP_ABS,		"abs",		46, 0, 0, 0, 0,
1, {RT}, 1, {RA}, {L15}},

{31,E15, 721,	76,	11,	OP_ABS_,	"abs.",		46, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {RA}, {L15, C15, C15, C15, C15}},

{31,E15,1744,	76,	11,	OP_ABSO,	"abso",		46, 0, 0, 0, 0,
3, {RT, SO, OV}, 1, {RA}, {L15, O15, O15}},

{31,E15,1745,	76,	11,	OP_ABSO_,	"abso.",	46, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 1, {RA},
   {L15, O15, O15, C15, C15, C15, C15}},


{31,E15, 208,	76,	11,	OP_NEG,		"neg",		46, 0, 0, 0, 0,
1, {RT}, 1, {RA}, {L15}},

{31,E15, 209,	76,	11,	OP_NEG_,	"neg.",		46, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {RA}, {L15, C15, C15, C15, C15}},

{31,E15,1232,	76,	11,	OP_NEGO,	"nego",		46, 0, 0, 0, 0,
3, {RT, SO, OV}, 1, {RA}, {L15, O15, O15}},

{31,E15,1233,	76,	11,	OP_NEGO_,	"nego.",	46, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 1, {RA},
   {L15, O15, O15, C15, C15, C15, C15}},


{31,E15, 976,	76,	11,	OP_NABS,	"nabs",		46, 0, 0, 0, 0,
1, {RT}, 1, {RA}, {L15}},

{31,E15, 977,	76,	11,	OP_NABS_,	"nabs.",	46, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {RA}, {L15, C15, C15, C15, C15}},

{31,E15,2000,	76,	11,	OP_NABSO,	"nabso",	46, 0, 0, 0, 0,
3, {RT, SO, OV}, 1, {RA}, {L15, O15, O15}},

{31,E15,2001,	76,	11,	OP_NABSO_,	"nabso.",	46, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 1, {RA},
   {L15, O15, O15, C15, C15, C15, C15}},


{31,E17, 214,	64,	11,	OP_MUL,		"mul",		47, 0, 0, 0, 0,
2, {RT, MQ}, 2, {RA, RB}, {L17, M17}},

{31,E17, 215,	64,	11,	OP_MUL_,	"mul.",		47, 0, 0, 0, 0,
6, {RT,  MQ,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L17, M17, C17, C17, C17, C17}},

{31,E17,1238,	64,	11,	OP_MULO,	"mulo",		47, 0, 0, 0, 0,
4, {RT, MQ, SO, OV}, 2, {RA, RB}, {L17, M17, O17, O17}},

{31,E17,1239,	64,	11,	OP_MULO_,	"mulo.",	47, 0, 0, 0, 0,
8, {RT,  MQ,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L17, M17, O17, O17, C17, C17, C17, C17}},


{7, E18, 0,	6,	0,	OP_MULI,	"muli",		47, 0, 0, 0, 0,
2, {RT, MQ}, 1, {RA}, {L18, M18}},


{31,E19, 470,	64,	11,	OP_MULS,	"muls",		47, 0, 0, 0, 0,
2, {RT, MQ}, 2, {RA, RB}, {L19, M19}},

{31,E19, 471,	64,	11,	OP_MULS_,	"muls.",	47, 0, 0, 0, 0,
6, {RT,  MQ,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, M19, C19, C19, C19, C19}},

{31,E19,1494,	64,	11,	OP_MULSO,	"mulso",	47, 0, 0, 0, 0,
4, {RT, MQ, SO, OV}, 2, {RA, RB}, {L19, M19, O19, O19}},

{31,E19,1495,	64,	11,	OP_MULSO_,	"mulso.",	47, 0, 0, 0, 0,
8, {RT,  MQ,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, M19, O19, O19, C19, C19, C19, C19}},


{31,E19, 150,	64,	11,	OP_MULHW,	"mulhw",       -92, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L19}},

{31,E19, 151,	64,	11,	OP_MULHW_,	"mulhw.",      -92, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, C19, C19, C19, C19}},

{31,E19,  22,	64,	11,	OP_MULHWU,	"mulhwu",      -93, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L19}},

{31,E19,  23,	64,	11,	OP_MULHWU_,	"mulhwu.",     -93, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, C19, C19, C19, C19}},

{31,E19, 466,	64,	11,	OP_MULLD,	"mulld",       -90, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L19}},

{31,E19, 467,	64,	11,	OP_MULLD_,	"mulld.",      -90, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, C19, C19, C19, C19}},

{31,E19,1490,	64,	11,	OP_MULLDO,	"mulldo",      -90, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L19, O19, O19}},

{31,E19,1491,	64,	11,	OP_MULLDO_,	"mulldo.",     -90, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, O19, O19, C19, C19, C19, C19}},

{31,E19, 146,	64,	11,	OP_MULHD,	"mulhd",       -91, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L19}},

{31,E19, 147,	64,	11,	OP_MULHD_,	"mulhd.",      -91, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, C19, C19, C19, C19}},

{31,E19,  18,	64,	11,	OP_MULHDU,	"mulhdu",      -92, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L19}},

{31,E19,  19,	64,	11,	OP_MULHDU_,	"mulhdu.",     -92, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L19, C19, C19, C19, C19}},

{31,E20, 662,	64,	11,	OP_DIV,		"div",		48, 0, 0, 0, 0,
2, {RT, MQ}, 3, {RA, RB, MQ}, {L20, M20}},

{31,E20, 663,	64,	11,	OP_DIV_,	"div.",		48, 0, 0, 0, 0,
6, {RT,  MQ,  CR0, CR1, CR2, CR3}, 3, {RA, RB, MQ},
   {L20, M20, C20, C20, C20, C20}},

{31,E20, 1686,	64,	11,	OP_DIVO,	"divo",		48, 0, 0, 0, 0,
4, {RT, MQ, SO, OV}, 3, {RA, RB, MQ}, {L20, M20, O20, O20}},

{31,E20, 1687,	64,	11,	OP_DIVO_,	"divo.",	48, 0, 0, 0, 0,
8, {RT,  MQ,  SO,  OV,  CR0, CR1, CR2, CR3}, 3, {RA, RB, MQ},
   {L20, M20, O20, O20, C20, C20, C20, C20}},


{31,E20, 726,	64,	11,	OP_DIVS,	"divs",		48, 0, 0, 0, 0,
2, {RT, MQ}, 2, {RA, RB}, {L20, M20}},

{31,E20, 727,	64,	11,	OP_DIVS_,	"divs.",	48, 0, 0, 0, 0,
6, {RT,  MQ,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, M20, C20, C20, C20, C20}},

{31,E20, 1750,	64,	11,	OP_DIVSO,	"divso",	48, 0, 0, 0, 0,
4, {RT, MQ, SO, OV}, 2, {RA, RB}, {L20, M20, O20, O20}},

{31,E20, 1751,	64,	11,	OP_DIVSO_,	"divso.",	48, 0, 0, 0, 0,
8, {RT,  MQ,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, M20, O20, O20, C20, C20, C20, C20}},


{31,E20, 978,	64,	11,	OP_DIVD,	"divd",	       -94, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L20}},

{31,E20, 979,	64,	11,	OP_DIVD_,	"divd.",       -94, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, C20, C20, C20, C20}},

{31,E20, 2002,	64,	11,	OP_DIVDO,	"divdo",       -94, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L20, O20, O20}},

{31,E20, 2003,	64,	11,	OP_DIVDO_,	"divdo.",      -94, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, O20, O20, C20, C20, C20, C20}},


{31,E20, 982,	64,	11,	OP_DIVW,	"divw",	       -95, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L20}},

{31,E20, 983,	64,	11,	OP_DIVW_,	"divw.",       -95, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, C20, C20, C20, C20}},

{31,E20, 2006,	64,	11,	OP_DIVWO,	"divwo",       -95, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L20, O20, O20}},

{31,E20, 2007,	64,	11,	OP_DIVWO_,	"divwo.",      -95, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, O20, O20, C20, C20, C20, C20}},

{31,E20, 914,	64,	11,	OP_DIVD,	"divdu",       -96, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L20}},

{31,E20, 915,	64,	11,	OP_DIVD_,	"divdu.",      -96, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, C20, C20, C20, C20}},

{31,E20, 1938,	64,	11,	OP_DIVDO,	"divduo",      -96, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L20, O20, O20}},

{31,E20, 1939,	64,	11,	OP_DIVDO_,	"divduo.",     -96, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, O20, O20, C20, C20, C20, C20}},


{31,E20, 918,	64,	11,	OP_DIVWU,	"divwu",       -97, 0, 0, 0, 0,
1, {RT}, 2, {RA, RB}, {L20, M20}},

{31,E20, 919,	64,	11,	OP_DIVWU_,	"divwu.",      -97, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, M20, C20, C20, C20, C20}},

{31,E20, 1942,	64,	11,	OP_DIVWUO,	"divwuo",      -97, 0, 0, 0, 0,
3, {RT, SO, OV}, 2, {RA, RB}, {L20, M20, O20, O20}},

{31,E20, 1943,	64,	11,	OP_DIVWUO_,	"divwuo.",     -97, 0, 0, 0, 0,
7, {RT,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, RB},
   {L20, M20, O20, O20, C20, C20, C20, C20}},


{11,E21, 0,	7,	0,	OP_CMPI,	"cmpi",		49, 0, 0, 0, 0,
1, {BF}, 1, {RA}, {L21}},
{10,E21, 0,	8,	0,	OP_CMPLI,	"cmpli",	49, 0, 0, 0, 0,
1, {BF}, 1, {RA}, {L21}},
{31,E21, 0,	74,	11,	OP_CMP,		"cmp",		49, 0, 0, 0, 0,
1, {BF}, 2, {RA, RB}, {L21}},
{31,E21, 1,	74,	11,	OP_CMP,		"cmp",		49, 0, 0, 0, 0,
5, {BF, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L21, C21, C21, C21, C21}},
{31,E21, 64,	74,	11,	OP_CMPL,	"cmpl",		49, 0, 0, 0, 0,
1, {BF}, 2, {RA, RB}, {L21}},
{31,E21, 65,	74,	11,	OP_CMPL,	"cmpl",		49, 0, 0, 0, 0,
5, {BF, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L21, C21, C21, C21, C21}},

{3,E22,	 0,	3,	0,	OP_TI,		"ti",		50, 0, 0, 0, 0,
1, {IAR}, 1, {RA}, {L22}},
{31,E22, 8,	69,	11,	OP_T,		"t",		50, 0, 0, 0, 0,
1, {IAR}, 2, {RA, RB}, {L22}},
{31,E22, 9,	69,	11,	OP_T,		"t",		50, 0, 0, 0, 0,
5, {IAR, CR0, CR1, CR2, CR3}, 2, {RA, RB}, {L22, C22, C22, C22, C22}},


{28,E23, 0,	9,	0,	OP_ANDIL,	"andil.",	51, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 1, {RS}, {L23, C23, C23, C23, C23}},

{29,E23, 0,	9,	0,	OP_ANDIU,	"andiu.",	51, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 1, {RS}, {L23, C23, C23, C23, C23}},

{31,E23, 56,	75,	11,	OP_AND,		"and",		51, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 57,	75,	11,	OP_AND_,	"and.",		51, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{24,E23, 0,	9,	0,	OP_ORIL,	"oril",		51, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L23}},

{25,E23, 0,	9,	0,	OP_ORIU,	"oriu",		52, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L23}},

{31,E23, 888,	75,	11,	OP_OR,		"or",		52, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 889,	75,	11,	OP_OR_,		"or.",		52, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


/* Synthetic non-PPC ops for committing ops like AI that set CA or OV */
{31,E23, 890,	75,	11,	OP_OR_C,	"or_c",		52, 0, 0, 0, 0,
2, {RA, CA},     2, {RS, RB}, {L23, O23}},

{31,E23, 892,	75,	11,	OP_OR_O,	"or_o",		52, 0, 0, 0, 0,
3, {RA, OV, SO}, 2, {RS, RB}, {L23, O23, O23}},

{31,E23, 894,	75,	11,	OP_OR_CO,	"or_co",	52, 0, 0, 0, 0,
4, {RA, CA, OV, SO}, 2, {RS, RB}, {L23, O23, O23, O23}},


{26,E23, 0,	9,	0,	OP_XORIL,	"xoril",	52, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L23}},

{27,E23, 0,	9,	0,	OP_XORIU,	"xoriu",	52, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L23}},

{31,E23, 632,	75,	11,	OP_XOR,		"xor",		53, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 633,	75,	11,	OP_XOR_,	"xor.",		53, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{31,E23, 120,	75,	11,	OP_ANDC,	"andc",		53, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 121,	75,	11,	OP_ANDC_,	"andc.",	53, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{31,E23, 568,	75,	11,	OP_EQV,		"eqv",		53, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 569,	75,	11,	OP_EQV_,	"eqv.",		53, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{31,E23, 824,	75,	11,	OP_ORC,		"orc",		53, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 825,	75,	11,	OP_ORC_,	"orc.",		53, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{31,E23, 248,	75,	11,	OP_NOR,		"nor",		54, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 249,	75,	11,	OP_NOR_,	"nor.",		54, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{31,E23, 1844,	194,	11,	OP_EXTS,	"exts",		54, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L23}},

{31,E23, 1845,	194,	11,	OP_EXTS_,	"exts.",	54, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 1, {RS}, {L23, C23, C23, C23, C23}},


{31,E23, 1908,	194,	11,	OP_EXTSB,	"extsb",      -112, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L23}},

{31,E23, 1909,	194,	11,	OP_EXTSB_,	"extsb.",     -112, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 1, {RS}, {L23, C23, C23, C23, C23}},


{31,E23, 952,	75,	11,	OP_NAND,	"nand",		54, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L23}},

{31,E23, 953,	75,	11,	OP_NAND_,	"nand.",	54, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L23, C23, C23, C23, C23}},


{31,E24, 52,	194,	11,	OP_CNTLZ,	"cntlz",	54, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L24}},

{31,E24, 53,	194,	11,	OP_CNTLZ_,	"cntlz.",	54, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 1, {RS}, {L24, C24, C24, C24, C24}},


{20,E25, 0,	163,	1,	OP_RLIMI,	"rlimi",	56, 0, 0, 0, 0,
1, {RA}, 2, {RS, RA}, {L25}},
{20,E25, 1,	163,	1,	OP_RLIMI_,	"rlimi.",	56, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RA}, {L25, C25, C25, C25, C25}},

{21,E25, 0,	161,	1,	OP_RLINM,	"rlinm",	56, 0, 0, 0, 0,
1, {RA}, 1, {RS}, {L25}},
{21,E25, 1,	161,	1,	OP_RLINM_,	"rlinm.",	56, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 1, {RS}, {L25, C25, C25, C25, C25}},

{22,E25, 0,	162,	1,	OP_RLMI,	"rlmi",		56, 0, 0, 0, 0,
1, {RA}, 3, {RS, RA, RB}, {L25}},
{22,E25, 1,	162,	1,	OP_RLMI_,	"rlmi.",	56, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 3, {RS, RA, RB}, {L25, C25, C25, C25, C25}},

{23,E25, 0,	160,	1,	OP_RLNM,	"rlnm",		56, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L25}},
{23,E25, 1,	160,	1,	OP_RLNM_,	"rlnm.",	56, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L25, C25, C25, C25, C25}},

{31,E25, 1074,	195,	11,	OP_RRIB,	"rrib",		57, 0, 0, 0, 0,
1, {RA}, 3, {RS, RA, RB}, {L25}},
{31,E25, 1075,	195,	11,	OP_RRIB_,	"rrib.",	57, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 3, {RS, RA, RB}, {L25, C25, C25, C25, C25}},

{31,E25, 58,	75,	11,	OP_MASKG,	"maskg",	57, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L25}},
{31,E25, 59,	75,	11,	OP_MASKG_,	"maskg.",	57, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L25, C25, C25, C25, C25}},

{31,E25, 1082,	195,	11,	OP_MASKIR,	"maskir",	57, 0, 0, 0, 0,
1, {RA}, 3, {RS, RA, RB}, {L25}},
{31,E25, 1083,	195,	11,	OP_MASKIR,	"maskir.",	57, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 3, {RS, RA, RB}, {L25, C25, C25, C25, C25}},

{31,E26, 48,	75,	11,	OP_SL,		"sl",		58, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L26}},
{31,E26, 49,	75,	11,	OP_SL_,		"sl.",		58, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, C26, C26, C26, C26}},

{31,E26, 1072,	75,	11,	OP_SR,		"sr",		58, 0, 0, 0, 0,
1, {RA}, 2, {RS, RB}, {L26}},
{31,E26, 1073,	75,	11,	OP_SR_,		"sr.",		58, 0, 0, 0, 0,
5, {RA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, C26, C26, C26, C26}},

{31,E26, 304,	75,	11,	OP_SLQ,		"slq",		59, 0, 0, 0, 0,
2, {RA, MQ}, 2, {RS, RB}, {L26, M26}},
{31,E26, 305,	75,	11,	OP_SLQ_,	"slq.",		59, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 1328,	75,	11,	OP_SRQ,		"srq",		59, 0, 0, 0, 0,
2, {RA, MQ}, 2, {RS, RB}, {L26, M26}},
{31,E26, 1329,	75,	11,	OP_SRQ_,	"srq.",		59, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 368,	67,	11,	OP_SLIQ,	"sliq",		60, 0, 0, 0, 0,
2, {RA, MQ}, 1, {RS}, {L26, M26}},
{31,E26, 369,	67,	11,	OP_SLIQ_,	"sliq.",	60, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 1, {RS}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 496,	67,	11,	OP_SLLIQ,	"slliq",	60, 0, 0, 0, 0,
2, {RA, MQ}, 2, {RS, MQ}, {L26, M26}},
{31,E26, 497,	67,	11,	OP_SLLIQ_,	"slliq.",	60, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 2, {RS, MQ}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 1392,	67,	11,	OP_SRIQ,	"sriq",		60, 0, 0, 0, 0,
2, {RA, MQ}, 1, {RS}, {L26, M26}},
{31,E26, 1393,	67,	11,	OP_SRIQ_,	"sriq.",	60, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 1, {RS}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 1520,	67,	11,	OP_SRLIQ,	"srliq",	60, 0, 0, 0, 0,
2, {RA, MQ}, 2, {RS, MQ}, {L26, M26}},
{31,E26, 1521,	67,	11,	OP_SRLIQ_,	"srliq.",	60, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 2, {RS, MQ}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 432,	75,	11,	OP_SLLQ,	"sllq",		61, 0, 0, 0, 0,
2, {RA, MQ}, 3, {RS, RB, MQ}, {L26, M26}},
{31,E26, 433,	75,	11,	OP_SLLQ_,	"sllq.",	61, 0, 0, 0, 0,
6, {RA,  MQ,  CR0, CR1, CR2, CR3}, 3, {RS, RB, MQ},
   {L26, M26, C26, C26, C26, C26}},

{31,E26, 1456,	75,	11,	OP_SRLQ,	"srlq",		61, 0, 0, 0, 0,
2, {RA, MQ}, 3, {RS, RB, MQ}, {L26, M26}},
{31,E26, 1457,	75,	11,	OP_SRLQ_,	"srlq.",	61, 0, 0, 0, 0,
6, {RA,  MQ,  CR0, CR1, CR2, CR3}, 3, {RS, RB, MQ},
   {L26, M26, C26, C26, C26, C26}},

{31,E26, 306,	75,	11,	OP_SLE,		"sle",		62, 0, 0, 0, 0,
2, {RA, MQ}, 2, {RS, RB}, {L26, M26}},
{31,E26, 307,	75,	11,	OP_SLE_,	"sle.",		62, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 434,	75,	11,	OP_SLEQ,	"sleq",		62, 0, 0, 0, 0,
2, {RA, MQ}, 3, {RS, RB, MQ}, {L26, M26}},
{31,E26, 435,	75,	11,	OP_SLEQ_,	"sleq.",	62, 0, 0, 0, 0,
6, {RA,  MQ,  CR0, CR1, CR2, CR3}, 3, {RS, RB, MQ},
   {L26, M26, C26, C26, C26, C26}},

{31,E26, 1330,	75,	11,	OP_SRE,		"sre",		62, 0, 0, 0, 0,
2, {RA, MQ}, 2, {RS, RB}, {L26, M26}},
{31,E26, 1331,	75,	11,	OP_SRE_,	"sre.",		62, 0, 0, 0, 0,
6, {RA, MQ, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, M26, C26, C26, C26, C26}},

{31,E26, 1458,	75,	11,	OP_SREQ,	"sreq",		62, 0, 0, 0, 0,
2, {RA, MQ}, 3, {RS, RB, MQ}, {L26, M26}},
{31,E26, 1459,	75,	11,	OP_SREQ_,	"sreq.",	62, 0, 0, 0, 0,
6, {RA,  MQ,  CR0, CR1, CR2, CR3}, 3, {RS, RB, MQ},
   {L26, M26, C26, C26, C26, C26}},

{31,E26, 1648,	67,	11,	OP_SRAI,	"srai",		63, 0, 0, 0, 0,
2, {RA, CA}, 1, {RS}, {L26, O26}},
{31,E26, 1649,	67,	11,	OP_SRAI_,	"srai.",	63, 0, 0, 0, 0,
6, {RA, CA, CR0, CR1, CR2, CR3}, 1, {RS}, {L26, O26, C26, C26, C26, C26}},

{31,E26, 1584,	75,	11,	OP_SRA,		"sra",		63, 0, 0, 0, 0,
2, {RA, CA}, 2, {RS, RB}, {L26, O26}},
{31,E26, 1585,	75,	11,	OP_SRA_,	"sra.",		63, 0, 0, 0, 0,
6, {RA, CA, CR0, CR1, CR2, CR3}, 2, {RS, RB}, {L26, O26, C26, C26, C26, C26}},

{31,E26, 1904,	67,	11,	OP_SRAIQ,	"sraiq",	64, 0, 0, 0, 0,
3, {RA, MQ, CA}, 1, {RS}, {L26, M26, O26}},
{31,E26, 1905,	67,	11,	OP_SRAIQ_,	"sraiq.",	64, 0, 0, 0, 0,
7, {RA, MQ, CA, CR0, CR1, CR2, CR3}, 1, {RS}, {L26, O26, C26, C26, C26, C26}},

{31,E26, 1840,	75,	11,	OP_SRAQ,	"sraq",		64, 0, 0, 0, 0,
3, {RA, MQ, CA}, 2, {RS, RB}, {L26, M26, O26}},
{31,E26, 1841,	75,	11,	OP_SRAQ_,	"sraq.",	64, 0, 0, 0, 0,
7, {RA,  MQ,  CA,  CR0, CR1, CR2, CR3}, 2, {RS, RB}, 
   {L26, M26, O26, C26, C26, C26, C26}},

{31,E26, 1842,	75,	11,	OP_SREA,	"srea",		65, 0, 0, 0, 0,
3, {RA, MQ, CA}, 2, {RS, RB}, {L26, M26, O26}},
{31,E26, 1843,	75,	11,	OP_SREA_,	"srea.",	65, 0, 0, 0, 0,
7, {RA,  MQ,  CA,  CR0, CR1, CR2, CR3}, 2, {RS, RB},
   {L26, M26, O26, C26, C26, C26, C26}},

{31,E27, 934,	68,	11,	OP_MTSPR,	"mtspr",	66, 0, 0, 0, 0,
1, {SPR_T}, 1, {RS}, {L27}},
{31,E27, 935,	68,	11,	OP_MTSPR,	"mtspr",	66, 0, 0, 0, 0,
5, {SPR_T, CR0, CR1, CR2, CR3}, 1, {RS}, {L27}},

{31,E27, 936,  196,	11,	OP_M2SPR,	"m2spr",	66, 0, 0, 0, 0,
1, {SPR_T}, 1, {SPR_F}, {L27}},
{31,E27, 937,  196,	11,	OP_M2SPR,	"m2spr",	66, 0, 0, 0, 0,
5, {SPR_T, CR0, CR1, CR2, CR3}, 1, {SPR_F}, {L27}},

{31,E28, 678,	77,	11,	OP_MFSPR,	"mfspr",	66, 0, 0, 0, 0,
1, {RT}, 1, {SPR_F}, {L28}},
{31,E28, 679,	77,	11,	OP_MFSPR,	"mfspr",	66, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {SPR_F}, {L28}},

{31,E28, 742,   77,	11,	OP_MFTB,	"mftb",	      -352, 0, 0, 0, 0,
1, {RT}, 1, {SPR_F}, {L28}},
{31,E28, 743,   77,	11,	OP_MFTB,	"mftb",	      -352, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {SPR_F}, {L28}},

{31,E29, 288,	96,	11,	OP_MTCRF,	"mtcrf",	67, 0, 0, 0, 0,
1, {FXM}, 1, {RS}, {L29}},
{31,E29, 289,	96,	11,	OP_MTCRF,	"mtcrf",	67, 0, 0, 0, 0,
5, {FXM, CR0, CR1, CR2, CR3}, 1, {RS}, {L29, C29, C29, C29, C29}},

{31,E29, 290,  197,	11,	OP_MTCRF2,	"mtcrf2",	67, 0, 0, 0, 0,
1, {BF2}, 1, {RB}, {L29}},
{31,E29, 291,  197,	11,	OP_MTCRF2,	"mtcrf2",	67, 0, 0, 0, 0,
5, {BF2, CR0, CR1, CR2, CR3}, 1, {RB}, {L29, C29, C29, C29, C29}},

{31,E30, 38,	78,	11,	OP_MFCR,	"mfcr",		67, 0, 0, 0, 0,
1, {RT}, 32, {CR0, CR1, CR2, CR3, CR4, CR5, CR6, CR7,
	      CR8, CR9, CR10,CR11,CR12,CR13,CR14,CR15,
	      CR16,CR17,CR18,CR19,CR20,CR21,CR22,CR23,
	      CR24,CR25,CR26,CR27,CR28,CR29,CR30,CR31},
   {L30}},
{31,E30, 39,	78,	11,	OP_MFCR,	"mfcr",		67, 0, 0, 0, 0,
1, {RT, CR0, CR1, CR2, CR3}, 32, {CR0, CR1, CR2, CR3, CR4, CR5, CR6, CR7,
				  CR8, CR9, CR10,CR11,CR12,CR13,CR14,CR15,
				  CR16,CR17,CR18,CR19,CR20,CR21,CR22,CR23,
				  CR24,CR25,CR26,CR27,CR28,CR29,CR30,CR31},
   {L30, C30, C30, C30, C30}},

{31,E31, 1024,	66,	11,	OP_MCRXR,	"mcrxr",	67, 0, 0, 0, 0,
5, {BF, XER0, XER1, XER2, XER3}, 4, {XER0, XER1, XER2, XER3},
   {L31, X31, X31,  X31,  X31}},
{31,E31, 1025,	66,	11,	OP_MCRXR,	"mcrxr",	67, 0, 0, 0, 0,
9, {BF, XER0, XER1, XER2, XER3, CR0, CR1, CR2, CR3},
4, {XER0, XER1, XER2, XER3},
   {L31, X31, X31,  X31,  X31,  C31, C31, C31, C31}},

{31,E32, 292,	79,	11,	OP_MTMSR,	"mtmsr",	67, 0, 0, 0, 0,
32, {MSR0, MSR1, MSR2, MSR3, MSR4, MSR5, MSR6, MSR7,
     MSR8, MSR9, MSR10,MSR11,MSR12,MSR13,MSR14,MSR15,
     MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
     MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
 1, {RS},
    {L32, L32, L32, L32, L32, L32, L32, L32, 
     L32, L32, L32, L32, L32, L32, L32, L32, 
     L32, L32, L32, L32, L32, L32, L32, L32, 
     L32, L32, L32, L32, L32, L32, L32, L32}}, 

{31,E32, 293,	79,	11,	OP_MTMSR,	"mtmsr",	67, 0, 0, 0, 0,
36, {MSR0, MSR1, MSR2, MSR3, MSR4, MSR5, MSR6, MSR7,
     MSR8, MSR9, MSR10,MSR11,MSR12,MSR13,MSR14,MSR15,
     MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
     MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31,
     CR0,  CR1,  CR2,  CR3},
 1, {RS},
    {L32, L32, L32, L32, L32, L32, L32, L32, 
     L32, L32, L32, L32, L32, L32, L32, L32, 
     L32, L32, L32, L32, L32, L32, L32, L32, 
     L32, L32, L32, L32, L32, L32, L32, L32, C32, C32, C32, C32}}, 

{31,E33, 166,	78,	11,	OP_MFMSR,	"mfmsr",	67, 0, 0, 0, 0,
1, {RT}, 32, {MSR0, MSR1, MSR2, MSR3, MSR4, MSR5, MSR6, MSR7,
	      MSR8, MSR9, MSR10,MSR11,MSR12,MSR13,MSR14,MSR15,
	      MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
	      MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
   {L33}},

{31,E33, 167,	78,	11,	OP_MFMSR,	"mfmsr",	67, 0, 0, 0, 0,
 5, {RT, CR0, CR1, CR2, CR3}, 
   32, {MSR0, MSR1, MSR2, MSR3, MSR4, MSR5, MSR6, MSR7,
	MSR8, MSR9, MSR10,MSR11,MSR12,MSR13,MSR14,MSR15,
	MSR16,MSR17,MSR18,MSR19,MSR20,MSR21,MSR22,MSR23,
	MSR24,MSR25,MSR26,MSR27,MSR28,MSR29,MSR30,MSR31},
   {L33, C33, C33, C33, C33}},

{48,E34, 0,	10,	0,	OP_LFS,		"lfs",		82, 0, 0, 0, 0,
1, {FRT}, 4, {RZ, DCACHE, MEM, AL}, {L34}},
{31,E34, 1070,	176,	11,	OP_LFSX,	"lfsx",		82, 0, 0, 0, 0,
1, {FRT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L34}},
{31,E34, 1071,	176,	11,	OP_LFSX,	"lfsx",		82, 0, 0, 0, 0,
5, {FRT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L34, C34, C34, C34, C34}},

{50,E34, 0,	10,	0,	OP_LFD,	"lfd",		83, 0, 0, 0, 0,
1, {FRT}, 4, {RZ, DCACHE, MEM, AL}, {L34}},
{31,E34, 1198,	176,	11,	OP_LFDX,	"lfdx",		83, 0, 0, 0, 0,
1, {FRT}, 5, {RZ, RB, DCACHE, MEM, AL}, {L34}},
{31,E34, 1199,	176,	11,	OP_LFDX,	"lfdx",		83, 0, 0, 0, 0,
5, {FRT, CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L34, C34, C34, C34, C34}},

{49,E34,   0,	10,	0,	OP_LFSU,	"lfsu",		84, 0, 0, 0, 0,
2, {FRT, RZ}, 4, {RZ, DCACHE, MEM, AL}, {L34, U34}},
{31,E34, 1134,	176,	11,	OP_LFSUX,	"lfsux",	84, 0, 0, 0, 0,
2, {FRT, RZ}, 5, {RZ, RB, DCACHE, MEM, AL}},
{31,E34, 1135,	176,	11,	OP_LFSUX,	"lfsux",	84, 0, 0, 0, 0,
6, {FRT, RZ,  CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L34, U34, C34, C34, C34, C34}},

{51,E34,  0,	10,	0,	OP_LFDU,	"lfdu",		85, 0, 0, 0, 0,
2, {FRT, RZ}, 4, {RZ, DCACHE, MEM, AL}, {L34, U34}},
{31,E34, 1262,	176,	11,	OP_LFDUX,	"lfdux",	85, 0, 0, 0, 0,
2, {FRT, RZ}, 5, {RZ, RB, DCACHE, MEM, AL}, {L34, U34}},
{31,E34, 1263,	176,	11,	OP_LFDUX,	"lfdux",	85, 0, 0, 0, 0,
6, {FRT, RZ,  CR0, CR1, CR2, CR3}, 5, {RZ, RB, DCACHE, MEM, AL},
   {L34, U34, C34, C34, C34, C34}},

{52,E35, 0,	11,	0,	OP_STFS,	"stfs",		87, 0, 0, 0, 0,
2, {DCACHE, MEM}, 3, {FRS, RZ, AL}, {L35, L35}},
{31,E35, 1326,	177,	11,	OP_STFSX,	"stfsx",	87, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {FRS, RZ, RB, AL}, {L35, L35}},
{31,E35, 1327,	177,	11,	OP_STFSX,	"stfsx",	87, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {FRS, RZ, RB, AL},
   {L35,    U35, C35, C35, C35, C35}},
{31,E35, 1966,	177,	11,	OP_STFIWX,	"stfiwx",     -198, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {FRS, RZ, RB, AL}, {L35, L35}},

{54,E35,  0,	11,	0,	OP_STFD,	"stfd",		88, 0, 0, 0, 0,
2, {DCACHE, MEM}, 3, {FRS, RZ, AL}, {L35, L35}},
{31,E35, 1454,	177,	11,	OP_STFDX,	"stfdx",	88, 0, 0, 0, 0,
2, {DCACHE, MEM}, 4, {FRS, RZ, RB, AL}, {L35, L35}},
{31,E35, 1455,	177,	11,	OP_STFDX,	"stfdx",	88, 0, 0, 0, 0,
6, {DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {FRS, RZ, RB, AL},
   {L35,    U35, C35, C35, C35, C35}},

{53,E35, 0,	11,	0,	OP_STFSU,	"stfsu",	89, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 3, {FRS, RZ, AL}, {U35, L35, L35}},
{31,E35, 1390,	177,	11,	OP_STFSUX,	"stfsux",	89, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 4, {FRS, RZ, RB, AL}, {U35, L35, L35}},
{31,E35, 1391,	177,	11,	OP_STFSUX,	"stfsux",	89, 0, 0, 0, 0,
7, {RZ, DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {U35, L35,   U35, C35, C35, C35, C35}},

{55,E35,  0,	11,	0,	OP_STFDU,	"stfdu",	90, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 3, {FRS, RZ, AL}, {U35, L35, L35}},
{31,E35, 1518,	177,	11,	OP_STFDUX,	"stfdux",	90, 0, 0, 0, 0,
3, {RZ, DCACHE, MEM}, 4, {FRS, RZ, RB, AL}, {U35, L35, L35}},
{31,E35, 1519,	177,	11,	OP_STFDUX,	"stfdux",	90, 0, 0, 0, 0,
7, {RZ, DCACHE, MEM, CR0, CR1, CR2, CR3}, 4, {RS, RZ, RB, AL},
   {U35, L35,   U35, C35, C35, C35, C35}},

{63,E36, 144,	178,	11,	OP_FMR,		"fmr",		91, 0, 0, 0, 0,
1, {FRT}, 1, {FRB}, {L36}},
{63,E36, 145,	178,	11,	OP_FMR_,	"fmr.",		91, 0, 0, 0, 0,
5, {FRT, CR4, CR5, CR6, CR7}, 1, {FRB}, {L36, C36, C36, C36, C36}},

{63,E36, 146,	178,	11,	OP_FMR1,	"fmr1",		91, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI}, 1, {FRB}, 
    {L36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36}},

{63,E36, 148,	178,	11,	OP_FMR2,	"fmr2",		91, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXIMZ}, 1, {FRB}, 
    {L36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36}},

{63,E36, 150,	178,	11,	OP_FMR3,	"fmr3",		91, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ}, 
 1, {FRB}, 
    {L36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36}},

{63,E36, 152,	178,	11,	OP_FMR4,	"fmr4",		91, 0, 0, 0, 0,
15, {FRT, C,  FL, FG, FE, FU, FR, FI, OX, UX, ZX, XX, VXSNAN, VXIDI, VXZDZ},
 1, {FRB}, 
    {L36, F36,F36,F36,F36,F36,F36,F36,F36,F36,F36,F36,F36,    F36,   F36}},

{63,E36, 154,	178,	11,	OP_FMR5,	"fmr5",		91, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN}, 1, {FRB}, 
    {L36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36, F36}},

{63,E36, 528,	178,	11,	OP_FABS,	"fabs",		91, 0, 0, 0, 0,
1, {FRT}, 1, {FRB}, {L36}},
{63,E36, 529,	178,	11,	OP_FABS_,	"fabs.",	91, 0, 0, 0, 0,
5, {FRT, CR4, CR5, CR6, CR7}, 1, {FRB}, {L36, C36, C36, C36, C36}},

{63,E36, 80,	178,	11,	OP_FNEG,	"fneg",		91, 0, 0, 0, 0,
1, {FRT}, 1, {FRB}, {L36}},
{63,E36, 81,	178,	11,	OP_FNEG_,	"fneg.",	91, 0, 0, 0, 0,
5, {FRT, CR4, CR5, CR6, CR7}, 1, {FRB}, {L36, C36, C36, C36, C36}},

{63,E36, 272,	178,	11,	OP_FNABS,	"fnabs",	91, 0, 0, 0, 0,
1, {FRT}, 1, {FRB}, {L36}},
{63,E36, 273,	178,	11,	OP_FNABS_,	"fnabs.",	91, 0, 0, 0, 0,
5, {FRT, CR4, CR5, CR6, CR7}, 1, {FRB}, {L36, C36, C36, C36, C36}},


{63,E37, 42,	145,	6,	OP_FA,		"fa",		92, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37}},

{63,E37, 43,	145,	6,	OP_FA_,		"fa.",		92, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37,
     C37, C37, C37, C37}},


{59,E37, 42,	145,	6,	OP_FADDS,	"fadds",      -179, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37}},

{59,E37, 43,	145,	6,	OP_FADDS_,	"fadds.",     -179, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37,
     C37, C37, C37, C37}},


{63,E37, 40,	145,	6,	OP_FS,		"fs",		93, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37}},

{63,E37, 41,	145,	6,	OP_FS_,		"fs.",		93, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37,
     C37, C37, C37, C37}},

{59,E37, 40,	145,	6,	OP_FSUBS,	"fsubs",      -180, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37}},

{59,E37, 41,	145,	6,	OP_FSUBS_,	"fsubs.",     -180, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37, F37,
     C37, C37, C37, C37}},

{63,E38, 50,	146,	6,	OP_FM,		"fm",		93, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXIMZ},
 8, {FRA, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{63,E38, 51,	146,	6,	OP_FM_,		"fm.",		93, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXIMZ,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},

{59,E38, 50,	146,	6,	OP_FMULS,	"fmuls",      -181, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXIMZ},
 8, {FRA, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{59,E38, 51,	146,	6,	OP_FMULS_,	"fmuls.",     -181, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXIMZ,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},

{63,E39, 36,	145,	6,	OP_FD,		"fd",		94, 0, 0, 0, 0,
15, {FRT, C,   FL, FG, FE, FU, FR,  FI, OX, UX, ZX, XX, VXSNAN, VXIDI, VXZDZ},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39,F39,F39}},

{63,E39, 37,	145,	6,	OP_FD_,		"fd.",		94, 0, 0, 0, 0,
19, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, ZX, XX, VXSNAN, VXIDI, VXZDZ,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39,
     C39, C39, C39, C39}},


{59,E39, 36,	145,	6,	OP_FDIVS,	"fdivs",      -182, 0, 0, 0, 0,
15, {FRT, C,   FL, FG, FE, FU, FR,  FI, OX, UX, ZX, XX, VXSNAN, VXIDI, VXZDZ},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39,F39,F39}},

{59,E39, 37,	145,	6,	OP_FDIVS_,	"fdivs.",     -182, 0, 0, 0, 0,
19, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, ZX, XX, VXSNAN, VXIDI, VXZDZ,
     CR4, CR5, CR6, CR7},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39,
     C39, C39, C39, C39}},


{63,E40, 24,	178,	11,	OP_FRSP,	"frsp",		94, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN},
 8, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40}},

{63,E40, 25,	178,	11,	OP_FRSP_,	"frsp.",	94, 0, 0, 0, 0,
16, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, CR4, CR5, CR6, CR7},
 8, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40,
     C40, C40, C40, C40}},


{63,E38, 58,	144,	6,	OP_FMA,		"fma",		95, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{63,E38, 59,	144,	6,	OP_FMA_,	"fma.",		95, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{59,E38, 58,	144,	6,	OP_FMADDS,	"fmadds",     -183, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{59,E38, 59,	144,	6,	OP_FMADDS_,	"fmadds.",    -183, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{63,E38, 56,	144,	6,	OP_FMS,		"fms",		95, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{63,E38, 57,	144,	6,	OP_FMS_,	"fms.",		95, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{59,E38, 56,	144,	6,	OP_FMSUBS,	"fmsubs",     -184, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{59,E38, 57,	144,	6,	OP_FMSUBS_,	"fmsubs.",    -184, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{63,E38, 62,	144,	6,	OP_FNMA,	"fnma",		96, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{63,E38, 63,	144,	6,	OP_FNMA_,	"fnma.",	96, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{59,E38, 62,	144,	6,	OP_FNMADDS,	"fnmadds",    -185, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{59,E38, 63,	144,	6,	OP_FNMADDS_,	"fnmadds.",   -185, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{63,E38, 60,	144,	6,	OP_FNMS,	"fnms",		96, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{63,E38, 61,	144,	6,	OP_FNMS_,	"fnms.",	96, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{59,E38, 60,	144,	6,	OP_FNMSUBS,	"fnmsubs",    -186, 0, 0, 0, 0,
14, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38}},

{59,E38, 61,	144,	6,	OP_FNMSUBS_,	"fnmsubs.",   -186, 0, 0, 0, 0,
18, {FRT, C, FL, FG, FE, FU, FR, FI, OX, UX, XX, VXSNAN, VXISI, VXIMZ,
     CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38, F38,
     C38, C38, C38, C38}},


{63,E40, 28,	178,   11,	OP_FCTIW,	"fctiw",      -189, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXCVI},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40}},

{63,E40, 29,	178,   11,	OP_FCTIW_,	"fctiw.",     -189, 0, 0, 0, 0,
16, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXCVI,
     CR4, CR5, CR6, CR7},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40,
     C40, C40, C40, C40}},


{63,E40, 30,	178,   11,	OP_FCTIWZ,	"fctiwz",     -190, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXCVI},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40}},


{63,E40, 31,	178,   11,	OP_FCTIWZ_,	"fctiwz.",    -190, 0, 0, 0, 0,
16, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXCVI,
     CR4, CR5, CR6, CR7},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40, F40,
     C40, C40, C40, C40}},


{59,E39, 48,	178,	6,	OP_FRES,	"fres",       -200, 0, 0, 0, 0,
13, {FRT, C, FL, FG, FE, FU, FR, FI, FX, OX, UX, ZX, VXSNAN},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39}},


{59,E39, 49,	178,	6,	OP_FRES_,	"fres.",      -200, 0, 0, 0, 0,
17, {FRT, C, FL, FG, FE, FU, FR, FI, FX, OX, UX, ZX, VXSNAN,
     CR4, CR5, CR6, CR7},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39, F39,
     C39, C39, C39, C39}},


{63,E52, 44,	178,	6,	OP_FSQRT,	"fsqrt",      -198, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXSQRT},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52}},


{63,E52, 45,	178,	6,	OP_FSQRT_,	"fsqrt.",     -198, 0, 0, 0, 0,
16, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXSQRT,
     CR4, CR5, CR6, CR7},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52,
     C52, C52, C52, C52}},


{59,E52, 44,	178,	6,	OP_FSQRTS,	"fsqrts",     -199, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXSQRT},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52}},


{59,E52, 45,	178,	6,	OP_FSQRTS_,	"fsqrts.",    -199, 0, 0, 0, 0,
16, {FRT, C, FL, FG, FE, FU, FR, FI, FX, XX, VXSNAN, VXSQRT,
     CR4, CR5, CR6, CR7},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52,
     C52, C52, C52, C52}},


{63,E52, 52,	178,	6,	OP_FSQRTE,	"fsqrte",     -201, 0, 0, 0, 0,
12, {FRT, C, FL, FG, FE, FU, FR, FI, FX, ZX, VXSNAN, VXSQRT},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52}},


{63,E52, 53,	178,	6,	OP_FSQRTE_,	"fsqrte.",    -201, 0, 0, 0, 0,
16, {FRT, C, FL, FG, FE, FU, FR, FI, FX, ZX, VXSNAN, VXSQRT,
     CR4, CR5, CR6, CR7},
 7, {FRB, VE, OE, UE, XE, RN0, RN1},
    {L52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52, F52,
     C52, C52, C52, C52}},


{63,E53, 46,	144,	6,	OP_FSEL,	"fsel",       -202, 0, 0, 0, 0,
 1, {FRT},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L53}},

{63,E53, 47,	144,	6,	OP_FSEL_,	"fsel.",      -202, 0, 0, 0, 0,
 5, {FRT, CR4, CR5, CR6, CR7},
 9, {FRA, FRB, FRC, VE, OE, UE, XE, RN0, RN1},
    {L53, C53, C53, C53, C53}},


{63,E41, 0,	179,	11,	OP_FCMPU,	"fcmpu",	97, 0, 0, 0, 0,
 6, {BF, FL, FG, FE, FU, VXVC, VXSNAN},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L41, F41, F41, F41, F41, F41, F41}},

{63,E41, 1,	179,	11,	OP_FCMPU,	"fcmpu",	97, 0, 0, 0, 0,
10, {BF,  CR4, CR5, CR6, CR7, FL, FG, FE, FU, VXVC, VXSNAN},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L41, C41, C41, C41, C41, F41, F41, F41, F41, F41, F41}},


{63,E41,  64,	179,	11,	OP_FCMPO,	"fcmpo",	97, 0, 0, 0, 0,
 6, {BF, FL, FG, FE, FU, VXVC, VXSNAN},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L41, F41, F41, F41, F41, F41, F41}},

{63,E41,  65,	179,	11,	OP_FCMPO,	"fcmpo",	97, 0, 0, 0, 0,
10, {BF, CR4, CR5, CR6, CR7, FL, FG, FE, FU, VXVC, VXSNAN},
 8, {FRA, FRB, VE, OE, UE, XE, RN0, RN1},
    {L41, C41, C41, C41, C41, F41, F41, F41, F41, F41, F41}},


{63,E42, 1166,	180,	11,	OP_MFFS,	"mffs",		98, 0, 0, 0, 0,
 1, {FRT},
32, {FPSCR0, FPSCR1, FPSCR2, FPSCR3, FPSCR4, FPSCR5, FPSCR6, FPSCR7,
     FPSCR8, FPSCR9, FPSCR10,FPSCR11,FPSCR12,FPSCR13,FPSCR14,FPSCR15,
     FPSCR16,FPSCR17,FPSCR18,FPSCR19,FPSCR20,FPSCR21,FPSCR22,FPSCR23,
     FPSCR24,FPSCR25,FPSCR26,FPSCR27,FPSCR28,FPSCR29,FPSCR30,FPSCR31},
    {L42}},

{63,E42, 1167,	180,	11,	OP_MFFS_,	"mffs.",	98, 0, 0, 0, 0,
 5, {FRT, CR4, CR5, CR6, CR7},
32, {FPSCR0, FPSCR1, FPSCR2, FPSCR3, FPSCR4, FPSCR5, FPSCR6, FPSCR7,
     FPSCR8, FPSCR9, FPSCR10,FPSCR11,FPSCR12,FPSCR13,FPSCR14,FPSCR15,
     FPSCR16,FPSCR17,FPSCR18,FPSCR19,FPSCR20,FPSCR21,FPSCR22,FPSCR23,
     FPSCR24,FPSCR25,FPSCR26,FPSCR27,FPSCR28,FPSCR29,FPSCR30,FPSCR31},
    {L42, C42, C42, C42, C42}},


{63,E43, 128,	181,	11,	OP_MCRFS,	"mcrfs",	98, 0, 0, 0, 0,
2, {BF, BFB}, 1, {BFB}, /* As dictated by BFB, reset FX, OX, UX, ZX, XX, */ 
   {L43, L43}},		/*     VXSNAN, VXISI, VXIDI, VXZDZ, VXIMZ, VXVC  */

{63,E43, 129,	181,	11,	OP_MCRFS,	"mcrfs",	98, 0, 0, 0, 0,
6, {BF, BFB,  CR4, CR5, CR6, CR7}, 1, {BFB},
   {L43, L43, C43, C43, C43, C43}},                    /* As dictated by BFB,
          reset FX, OX, UX, ZX, XX, VXSNAN, VXISI, VXIDI, VXZDZ, VXIMZ, VXVC */

{63,E44, 1422,	112,	11,	OP_MTFSF,	"mtfsf",	99, 0, 0, 0, 0,
1, {FLM}, 1, {FRB}, {L44}},
{63,E44, 1423,	112,	11,	OP_MTFSF_,	"mtfsf.",	99, 0, 0, 0, 0,
5, {FLM, CR4, CR5, CR6, CR7}, 1, {FRB}, {L44, C44, C44, C44, C44}},

{63,E44, 1424,	198,	11,	OP_MTFSF2,	"mtfsf2",	99, 0, 0, 0, 0,
1, {BFF2}, 1, {FRB}, {L44}},
{63,E44, 1425,	198,	11,	OP_MTFSF2_,	"mtfsf2.",	99, 0, 0, 0, 0,
5, {BFF2, CR4, CR5, CR6, CR7}, 1, {FRB}, {L44, C44, C44, C44, C44}},

{63,E44,  268,	182,	11,	OP_MTFSFI,	"mtfsfi",	99, 0, 0, 0, 0,
1, {BFF}, 0, {0}, {L44}},
{63,E44,  269,	182,	11,	OP_MTFSFI_,	"mtfsfi.",	99, 0, 0, 0, 0,
5, {BFF, CR4, CR5, CR6, CR7}, 0, {0}, {L44, C44, C44, C44, C44}},

{63,E44,  76,	183,	11,	OP_MTFSB1,	"mtfsb1",      100, 0, 0, 0, 0,
1, {BTF}, 0, {0}, {L44}},
{63,E44,  77,	183,	11,	OP_MTFSB1_,	"mtfsb1.",     100, 0, 0, 0, 0,
1, {BTF, CR4, CR5, CR6, CR7}, 0, {0}, {L44, C44, C44, C44, C44}},

{63,E44, 140,	183,	11,	OP_MTFSB0,	"mtfsb0",      100, 0, 0, 0, 0,
1, {BTF}, 0, {0}, {L44}},
{63,E44, 141,	183,	11,	OP_MTFSB0_,	"mtfsb0.",     100, 0, 0, 0, 0,
1, {BTF, CR4, CR5, CR6, CR7}, 0, {0}, {L44, C44, C44, C44, C44}},

{31,E45, 1636,	193,	11,	OP_RAC,		"rac",	       124, 1, 0, 0, 0,
2, {RT, RZ}, 4, {RZ, RB, TLB, PFT}, {L45, L45}},
{31,E45, 1637,	193,	11,	OP_RAC_,	"rac.",	       124, 1, 0, 0, 0,
6, {RT,  RZ,  CR0, CR1, CR2, CR3}, 4, {RZ, RB, TLB, PFT},
   {L45, L45, C45, C45, C45, C45}},

{31,E46, 612,	65,	11,	OP_TLBI,	"tlbi",	       124, 1, 0, 0, 0,
2, {RZ, TLB}, 3, {RZ, RB, TLB}, {L46, L46}},
{31,E46, 613,	65,	11,	OP_TLBI,	"tlbi",	       124, 1, 0, 0, 0,
6, {RZ, TLB, CR0, CR1, CR2, CR3}, 3, {RZ, RB, TLB},
   {L46, L46, C46, C46, C46, C46}},

{31,E46, 740,	82,	11,	OP_TLBIA,	"tlbia",      -445, 1, 0, 0, 0,
1, {TLB}, 0, {1}, {L46}},

{31,E46,1132,	82,	11,	OP_TLBSYNC,	"tlbsync",    -445, 1, 0, 0, 0,
1, {TLB}, 1, {TLB}, {L46}},


{31,E47, 420,	70,	11,	OP_MTSR,	"mtsr",	       125, 1, 0, 0, 0,
1, {SR}, 1, {RS}, {L47}},
{31,E47, 421,	70,	11,	OP_MTSR,	"mtsr",	       125, 1, 0, 0, 0,
5, {SR, CR0, CR1, CR2, CR3}, 1, {RS}, {L47, C47, C47, C47, C47}},


{31,E47, 484,	71,	11,	OP_MTSRI,	"mtsri",       125, 1, 0, 0, 0,
17, {RZ,SR0,SR1,SR2,SR3,SR4,SR5,SR6,SR7,SR8,SR9,SR10,SR11,SR12,SR13,SR14,SR15},
 3, {RS, RZ, RB},
    {L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47}},

{31,E47, 485,	71,	11,	OP_MTSRI,	"mtsri",       125, 1, 0, 0, 0,
21, {RZ,  CR0, CR1, CR2, CR3,
     SR0,SR1,SR2,SR3,SR4,SR5,SR6,SR7,SR8,SR9,SR10,SR11,SR12,SR13,SR14,SR15},
 3, {RS, RZ, RB},
    {L47, C47, C47, C47, C47,
     L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47,L47}},


{31,E48,1190,	192,	11,	OP_MFSR,	"mfsr",	       125, 0, 0, 0, 0,
1, {RT}, 1, {SR}, {L48}},
{31,E48,1191,	192,	11,	OP_MFSR,	"mfsr",	       125, 0, 0, 0, 0,
5, {RT, CR0, CR1, CR2, CR3}, 1, {SR}, {L48, C48, C48, C48, C48}},

{31,E48,1254,	193,	11,	OP_MFSRI,	"mfsri",       125, 0, 0, 0, 0,
 2, {RT, RZ}, 
18, {RZ, RB,
     SR0,SR1,SR2,SR3,SR4,SR5,SR6,SR7,SR8,SR9,SR10,SR11,SR12,SR13,SR14,SR15},
    {L48, L48}},

{31,E48,1318,	199,	11,	OP_MFSRIN,	"mfsrin",     -442, 0, 0, 0, 0,
 1, {RT}, 
17, {RB,
     SR0,SR1,SR2,SR3,SR4,SR5,SR6,SR7,SR8,SR9,SR10,SR11,SR12,SR13,SR14,SR15},
    {L48}},


{31,E48,1255,	193,	11,	OP_MFSRI,	"mfsri",       125, 0, 0, 0, 0,
  6, {RT,  RZ,  CR0, CR1, CR2, CR3}, 
 18, {RZ, RB,
      SR0,SR1,SR2,SR3,SR4,SR5,SR6,SR7,SR8,SR9,SR10,SR11,SR12,SR13,SR14,SR15},
     {L48, L48, C48, C48, C48, C48}},

{31,E49, 2028,	65,	11,	OP_DCLZ,	"dclz",	       126, 1, 0, 0, 0,
3, {RZ, DCACHE, TLB}, 3, {RZ, RB, DR}, {L49, L49, L49}},
{31,E49, 2029,	65,	11,	OP_DCLZ,	"dclz",	       126, 1, 0, 0, 0,
7, {RZ, DCACHE, TLB, CR0, CR1, CR2, CR3}, 3, {RZ, RB, DR},
   {L49, L49,   L49, C49, C49, C49, C49}},

{31,E49,  556,	65,	11,	OP_DCBT,	"dcbt",	      -346, 0, 0, 0, 0,
2, {DCACHE, TLB}, 3, {RZ, RB, DR}, {L49, L49}},
{31,E49,  492,	65,	11,	OP_DCBTST,	"dcbtst",     -347, 0, 0, 0, 0,
2, {DCACHE, TLB}, 3, {RZ, RB, DR}, {L49, L49}},
{31,E49,  108,	65,	11,	OP_DCBST,	"dcbst",      -348, 0, 0, 0, 0,
2, {MEM, TLB}, 4, {RZ, RB, DR, DCACHE}, {L49, L49}},
{31,E49,  172,	65,	11,	OP_DCBF,	"dcbf",       -349, 0, 0, 0, 0,
3, {DCACHE, MEM, TLB}, 4, {RZ, RB, DR, DCACHE}, {L49, L49, L49}},
{31,E49,  940,	65,	11,	OP_DCBI,	"dcbi",       -439, 1, 0, 0, 0,
2, {DCACHE, TLB}, 3, {RZ, RB, DR}, {L49, L49}},

{31,E49,  336,	65,	11,	OP_CLF,		"clf",	       127, 0, 0, 0, 0,
5, {RZ, DCACHE, ICACHE, MEM, TLB}, 3, {RZ, RB, DR}, {L49, L49, L49, L49, L49}},
{31,E49,  337,	65,	11,	OP_CLF,		"clf",	       127, 0, 0, 0, 0,
9, {RZ, DCACHE, ICACHE, MEM, TLB, CR0, CR1, CR2, CR3}, 3, {RZ, RB, DR},
   {L49, L49,   L49,    L49, L49, C49, C49, C49, C49}},

{31,E49, 1004,	65,	11,	OP_CLI,		"cli",	       127, 1, 0, 0, 0,
3, {RZ, DCACHE, ICACHE, TLB}, 3, {RZ, RB, DR}, {L49, L49, L49, L49}},
{31,E49, 1005,	65,	11,	OP_CLI,		"cli",	       127, 1, 0, 0, 0,
7, {RZ, DCACHE, ICACHE, TLB, CR0, CR1, CR2, CR3}, 3, {RZ, RB, DR},
   {L49, L49,   L49,    L49, L49, C49, C49, C49, C49}},

{31,E49, 1260,	65,	11,	OP_DCLST,	"dclst",       128, 0, 0, 0, 0,
4, {RZ, DCACHE, MEM, TLB}, 3, {RZ, RB, DR}, {L49, L49, L49, L49}},
{31,E49, 1261,	65,	11,	OP_DCLST,	"dclst",       128, 0, 0, 0, 0,
8, {RZ, DCACHE, MEM, TLB, CR0, CR1, CR2, CR3}, 3, {RZ, RB, DR},
   {L49, L49,   L49, L49, C49, C49, C49, C49}},

{31,E50, 1196,	82,	11,	OP_DCS,		"dcs",	       128, 0, 0, 0, 0,
0, {1}, 2, {DCACHE, MEM}, {L50}},
{31,E50, 1197,	82,	11,	OP_DCS,		"dcs",	       128, 0, 0, 0, 0,
4, {CR0, CR1, CR2, CR3}, 2, {DCACHE, MEM}, {C50, C50, C50, C50}},

{19,E50,  300,	82,	11,	OP_ICS,		"ics",	       128, 0, 0, 0, 0,
1, {ICACHE}, 2, {ICACHE, MEM}, {L50}},
{19,E50,  301,	82,	11,	OP_ICS,		"ics",	       128, 0, 0, 0, 0,
2, {ICACHE, LR}, 2, {ICACHE, MEM}, {L50, L50}},

{19,E50, 1964,	82,	11,	OP_ICBI,	"icbi",	      -345, 0, 0, 0, 0,
1, {ICACHE}, 1, {ICACHE}, {L50}},

{31,E51, 1062,	76,	11,	OP_CLCS,	"clcs",	       129, 0, 0, 0, 0,
1, {RT}, 2, {RA, ICACHE, DCACHE}, {L51}},
{31,E51, 1063,	76,	11,	OP_CLCS,	"clcs",	       129, 0, 0, 0, 0,
5, {RT,  CR0, CR1, CR2, CR3}, 2, {RA, ICACHE, DCACHE},
   {L51, C51, C51, C51, C51}},

{19,E50, 1708,	82,	11,	OP_EIEIO,	"eieio",      -350, 0, 0, 0, 0,
1, {MEM}, 1, {MEM}, {L50}},


/* The following are not real PowerPC opcodes and do not fit in 32 bits.
   They do allow the CA input to be explicitly specified (so as to allow
   it to come from a GPR extender).
 */
{31,E16, 278,  200,	11,	OP_EAE,		"eae",		43, 0, 0, 0, 0,
2, {RT, CA}, 3, {RA, RB, CA}, {L16, O16}},
{31,E16, 279,  200,	11,	OP_EAE_,	"eae.",		43, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, C16, C16, C16, C16}},
{31,E16, 1302, 200,	11,	OP_EAEO,	"eaeo",		43, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 3, {RA, RB, CA}, {L16, O16, O16, O16}},
{31,E16, 1303, 200,	11,	OP_EAEO_,	"eaeo.",	43, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},

{31,E16, 274,  200,	11,	OP_ESFE,	"esfe",		43, 0, 0, 0, 0,
2, {RT, CA}, 3, {RA, RB, CA}, {L16, O16}},
{31,E16, 275,  200,	11,	OP_ESFE_,	"esfe.",	43, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, C16, C16, C16, C16}},
{31,E16, 1298, 200,	11,	OP_ESFEO,	"esfeo",	43, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 3, {RA, RB, CA}, {L16, O16, O16, O16}},
{31,E16, 1299, 200,	11,	OP_ESFEO_,	"esfeo.",	43, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 3, {RA, RB, CA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},

{31,E16, 472,  201,	11,	OP_EAME,	"eame",		44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, GCA}, {L16, O16}},

{31,E16, 473,  201,	11,	OP_EAME_,	"eame.",	44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1496, 201,	11,	OP_EAMEO,	"eameo",	44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, GCA}, {L16, O16, O16, O16}},

{31,E16, 1497, 201,	11,	OP_EAMEO_,	"eameo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},


{31,E16, 406,  201,	11,	OP_EAZE,	"eaze",		44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, GCA}, {L16, O16}},

{31,E16, 407,  201,	11,	OP_EAZE_,	"eaze.",	44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1430, 201,	11,	OP_EAZEO,	"eazeo",	44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, GCA}, {L16, O16, O16, O16}},

{31,E16, 1431, 201,	11,	OP_EAZEO_,	"eazeo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},

{31,E16, 462,  201,	11,	OP_ESFME,	"esfme",	44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, GCA}, {L16, O16}},

{31,E16, 463,  201,	11,	OP_ESFME_,	"esfme.",	44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1486, 201,	11,	OP_ESFMEO,	"esfmeo",	44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, GCA}, {L16, O16, O16, O16}},

{31,E16, 1487, 201,	11,	OP_ESFMEO_,	"esfmeo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},


{31,E16, 402,  201,	11,	OP_ESFZE,	"esfze",	44, 0, 0, 0, 0,
2, {RT, CA}, 2, {RA, GCA}, {L16, O16}},

{31,E16, 403,  201,	11,	OP_ESFZE_,	"esfze.",	44, 0, 0, 0, 0,
6, {RT,  CA,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, C16, C16, C16, C16}},

{31,E16, 1426, 201,	11,	OP_ESFZEO,	"esfzeo",	44, 0, 0, 0, 0,
4, {RT, CA, SO, OV}, 2, {RA, GCA}, {L16, O16, O16, O16}},

{31,E16, 1427, 201,	11,	OP_ESFZEO_,	"esfzeo.",	44, 0, 0, 0, 0,
8, {RT,  CA,  SO,  OV,  CR0, CR1, CR2, CR3}, 2, {RA, GCA},
   {L16, O16, O16, O16, C16, C16, C16, C16}},

};

int for_ops_elem = sizeof (for_ops) / sizeof (for_ops[0]);
OPCODE1 *ppc_opcode[NUM_PPC_OPCODE_SLOTS];  /* This is sorted in dis.c */
