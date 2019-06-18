/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_dis
#define H_dis

#include "opcode_fmt.h"

#define NUM_PRIMARY_OPCODES		64
#define MAX_DIFF_EXT_SIZES		2

#define MAX_WR_PER_OP			48
#define MAX_RD_PER_OP			48

#define MAX_PAGE_MOD_RESRC		(MAX_RPAGE_ENTRIES*64)
#define MAX_PAGE_USE_RESRC		(MAX_RPAGE_ENTRIES*64)

#define MAX_EXPLICITS			8	/* In any PPC instruc */

#define BCOND_PRIMARY			16

typedef struct {
   int num;
   int list[MAX_DIFF_EXT_SIZES];
} EXTSIZE;

EXTSIZE extsize_list[NUM_PRIMARY_OPCODES];

typedef struct {
   double cnt;
   int    opcode;
} OPCNT;

typedef struct {
   unsigned char  primary;
   unsigned char  exec_time;	 /* 1 => Fully Pipelined, Latency => No-pipe */
   unsigned short ext;
   unsigned char  format;
   unsigned char  extsize;
   unsigned short op_num;
   char		  *name;
   int		  page;
   unsigned	  priv   : 1;
   unsigned	  sa     : 1;
   unsigned	  mult   : 1;		/* Is lm, stm, lsi, or stsi instruc */
   unsigned	  illegal: 1;
} OPCODE_BASE;

typedef struct {
   unsigned short reg;
   unsigned short latency;
} RESULT;

typedef struct {
   unsigned	  num_wr;
   unsigned       wr[MAX_WR_PER_OP];
   unsigned	  num_rd;
   unsigned	  rd[MAX_RD_PER_OP];
   unsigned       wr_lat[MAX_WR_PER_OP];      /* Latency till avail in VLIW */
} OPERAND_IN;

/* The "wr_explicit" and "rd_explicit" fields allow immediate determination
   whether an operand in "wr" or "rd" is an explicit operand in the 
   instruction, for example RT, RA, FRT, etc.  Since "wr_explicit" and
   "rd_explicit" are "char's" OPERAND_BIT is not set.  If an operand
   is not explicit in the instruction, "wr_explicit" and "rd_explicit"
   contain -1.
*/
typedef struct {
   unsigned	  num_wr;
   RESULT	  *wr;
   unsigned char  *wr_explicit;
   unsigned	  num_rd;
   unsigned short *rd;
   unsigned char  *rd_explicit;
   unsigned	  ins;			/* Original instruction */
   unsigned	  orig_addr;		/* Original ins address */
   int		  srcdest_common;	/* Non-neg if a src/dest must be same
					   as with RLIMI.  Contains val of
					   non-renameable register
					*/
   unsigned	  mq    : 1;		/* 1 if simul should preserve MQ, */
   unsigned	  verif : 1;		/* 1 if LOAD is a LOAD-VERIFY ins */
   unsigned short renameable[NUM_RENAMEABLE_OPERANDS];
} OPERAND_OUT;

typedef struct {
   OPCODE_BASE    b;
   OPERAND_IN	  op;
} OPCODE1;

typedef struct {
   OPCODE_BASE    *b;
   OPERAND_OUT	  op;
} OPCODE2;

void set_extsize_list (void);
void check_ascending (void);
OPCODE1 *get_opcode (unsigned);
int opcode_cmp (OPCODE1 *, OPCODE1 *);

#endif
