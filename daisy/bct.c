/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*======================================================================*
 * Handle BCT's and other branches that decrement the PowerPC CTR.	*
 *======================================================================*/

/************************************************************************
 * The BO field of a PPC conditional branch instruc is specified as	*
 *									*
 * 1x1xx   Branch ALWAYS						*
 *									*
 * 001xx   Branch if (BI == 0)						*
 * 011xx   Branch if (BI == 1)						*
 *									*
 * 0000x   Decr CTR, then branch if (decremented CTR != 0  and  BI == 0)*
 * 0001x   Decr CTR, then branch if (decremented CTR == 0  and  BI == 0)*
 *									*
 * 0100x   Decr CTR, then branch if (decremented CTR != 0  and  BI == 1)*
 * 0101x   Decr CTR, then branch if (decremented CTR == 0  and  BI == 1)*
 *									*
 * 1x00x   Decr CTR, then branch if (decremented CTR != 0)		*
 * 1x01x   Decr CTR, then branch if (decremented CTR == 0)		*
 *----------------------------------------------------------------------*
 * Alternatively the bits can be viewed as follows:			*
 *									* 
 *  X    X    X    X    X						*
 *  A    A    A    A    A						*
 *  |    |    |    |    |						*
 *  |    |    |    |    Unused						*
 *  |    |    |    |							*
 *  |    |    |    0: Branch if CTR != 0				*
 *  |    |    |    1: Branch if CTR == 0				*
 *  |    |    |								*
 *  |    |    0: CTR Pay Attn and Decrement				*
 *  |    |    1: CTR Ignore						*
 *  |    |								*
 *  |    0: Branch if BI == 0						*
 *  |    1: Branch if BI == 1						*
 *  |									*
 *  0: BI Pay Attn							*
 *  1: BI Ignore							*
 ************************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "dis_tbl.h"
#include "vliw.h"
#include "rd_config.h"
#include "vliw-macros.h"
#include "unify.h"

/* Indices of bits in 4-bit Condition Register Field set by "cmp" */
#define LT_BIT					0
#define GT_BIT					1
#define EQ_BIT					2
#define OV_BIT					3

#define MAX_BCTS_PER_PAGE			1024
#define MAX_BCT_CRS_PER_PAGE			 512

static unsigned	      bct_cnt;
static unsigned	      bct_cr_cnt;
static unsigned short cal_rd[MAX_BCTS_PER_PAGE];
static unsigned short cmpi_rd[MAX_BCTS_PER_PAGE];
static unsigned short cr_rd[2*MAX_BCT_CRS_PER_PAGE];
static RESULT	      cal_wr[MAX_BCTS_PER_PAGE];
static RESULT	      cmpi_wr[4*MAX_BCTS_PER_PAGE];
static RESULT	      cr_wr[MAX_BCT_CRS_PER_PAGE];

static OPCODE2 op_cal;
static OPCODE2 op_cmpi;
static OPCODE2 op_cr;

static int     cal_latency;
static int     cmpi_latency;
static int     cr_latency;

static TIP     *ins_cmpi_tip;
static int     vliw_ccr_save;

static UNIFY   *cal_unify;
static UNIFY   *cmpi_unify;

extern UNIFY      *insert_unify;        /* Non-0 if unify instead of insert */
extern OPCODE1     *ppc_opcode[];
extern unsigned	   *curr_ins_addr;
extern int	   curr_rename_reg;
extern int	   curr_rename_reg_valid;
extern int	   num_gprs;
extern int	   num_fprs;
extern int	   num_ccbits;

short get_cal_bump (TIP *);
void  gen_cal_ctr_dec (void);
void  gen_compound_cond (int, int, int, int);
void  patch_bc (OPCODE2 *, int, int);

/************************************************************************
 *									*
 *				handle_bcond				*
 *				------------				*
 *									*
 * If "op" contains any dependence on the CTR, as with a BCT, convert	*
 * the CTR operations and compare to explicit operations and modify	*
 * "op" so that it depends only on a non-(PPC) architected bit in	*
 * the condition register.  In other words, conditional branches may	*
 * only depend on bits in the condition register, not explicitly on	*
 * the CTR.								*
 *									*
 * RETURNS:  (A) 0 if branch does not depend on CTR and nothing done,	*
 *	         1 otherwise, plus changes to "op" noted in (B)		*
 *									*
 *	 (B) Opcode for normal branch handler to schedule.  This opcode	*
 *	     is just the input "op" with appropriate changes:  The	*
 *           returned opcode does not depend on or change the CTR.  If	*
 *	     this holds for the original instruction, the original	*
 *	     instruction is returned unmodified.  Otherwise the changes	*
 *	     to CTR are done explicitly, and a non-(PPC) architected	*
 *	     condition register bit is set to reflect the result.  If	*
 *	     need be this bit is combined with the BI bit to yield a	*
 *	     single bit that combined with the BI bit to yield a single	*
 *	     bit that the returned instruction branches on.		*
 *									*
 * Example:	bct <target>   ==>  cal  CTR,-1(CTR)			*
 *				    cmpi ccb,CTR,0			*
 *				    bc   12,ccb,<target>		*
 *									*
 * NOTES:  (1) CTR is assumed to be R32 in the VLIW architecture,	*
 *	       hence "cal" and "cmpi" can be done.			*
 *									*
 *	   (2) Since CTR is R32, renaming can be done so that "cal" and	*
 *	       "cmpi" can be done in parallel and can use combining:	*
 *									*
 * Example:	bct <target>   ==>  cal  CTR',-1(CTR)	   # VLIW1	*
 *				    cmpi ccb,CTR,1			*
 *									*
 *				    copy CTR,CTR'	   # VLIW2	*
 *				    bc   12,ccb,<target>		*
 *									*
 ************************************************************************/

int handle_bcond (ptip, op)
TIP     **ptip;
OPCODE2 *op;
{
   int		vliw_ccr;
   int		earliest;
   int		cal_in_reg;
   unsigned	ins        = op->op.ins;
   unsigned	bo         = (ins >> 21) & 0x1F;
   int		ctr_sense  = bo &  2;
   int		ctr_ignore = bo &  4;
   int		bi_sense   = bo &  8;
   int		bi_ignore  = bo & 16;
   short	cal_bump;
   TIP		*tip = *ptip;
   TIP		*cal_tip;
   TIP		*earliest_cmpi_tip;
   TIP		*ins_tip;
   TIP		*ins_cal_tip;

   if (ctr_ignore) return 0;			/* No special handling here */

   op_cal.op.orig_addr  = curr_ins_addr;
   op_cmpi.op.orig_addr = curr_ins_addr;
   op_cr.op.orig_addr   = curr_ins_addr;

   gen_cal_ctr_dec ();
   if (!combine_induc_var (&tip, &op_cal, curr_ins_addr+1, &ins_cal_tip)) {
      earliest = get_earliest_time (tip, &op_cal, FALSE);
      tip = insert_op (&op_cal, tip, earliest, &ins_cal_tip);
      cal_unify = insert_unify;
   }

   cal_tip = tip;
   cal_bump = get_cal_bump (ins_cal_tip);
   assert (cal_bump < 0);

   vliw_ccr = gen_cmpi_ctr (tip, ins_cal_tip, -cal_bump, &earliest_cmpi_tip);
   vliw_ccr_save = vliw_ccr;
   tip=insert_op (&op_cmpi, tip, earliest_cmpi_tip->vliw->time, &ins_cmpi_tip);
   cmpi_unify = insert_unify;

   fix_cmpi (ins_cmpi_tip, ins_cal_tip, cal_tip);

   if (!bi_ignore) {				/* BCT or BCF with cond	    */
      gen_compound_cond (bi_sense, ctr_sense, vliw_ccr,
			 op->op.renameable[BI & (~OPERAND_BIT)]);

      earliest = get_earliest_time (tip, &op_cr, FALSE);
      tip = insert_op (&op_cr, tip, earliest, &ins_tip);

      ctr_sense = 2;				/* If BI set, take branch */
   }

   patch_bc (op, vliw_ccr, ctr_sense);

   *ptip = tip;
   return 1;
}

/************************************************************************
 *									*
 *			    update_ccbit_for_bct			*
 *			    --------------------			*
 *									*
 * Should be called after the actual branch that used to be a BCT is	*
 * scheduled, so that we know the full range in which the non-arch	*
 * condition code bit holding the CTR comparison is needed.		*
 *									*
 ************************************************************************/

update_ccbit_for_bct (tip)
TIP *tip;
{
   update_reg_info3 (tip, ins_cmpi_tip, vliw_ccr_save,
		     ins_cmpi_tip->vliw->time + cmpi_latency);
}

/************************************************************************
 *									*
 *			    gen_cal_ctr_dec				*
 *			    ---------------				*
 *									*
 * Fill in fields in "op_cal" for counter decrement instruction:	*
 *									*
 *		cal  r32,-1(r32)					*
 *									*
 ************************************************************************/

static void gen_cal_ctr_dec ()
{
   assert (bct_cnt < MAX_BCTS_PER_PAGE);

   op_cal.op.wr   = &cal_wr[bct_cnt];
   op_cal.op.rd   = &cal_rd[bct_cnt];

   op_cal.op.wr[0].reg = CTR;
   op_cal.op.rd[0]     = CTR;

   op_cal.op.renameable[RZ & (~OPERAND_BIT)] = CTR;
   op_cal.op.renameable[RT & (~OPERAND_BIT)] = CTR;
}

/************************************************************************
 *									*
 *			    get_cal_bump				*
 *			    ------------				*
 *									*
 * Get the immediate field of the newly inserted "cal r32,-1(r32)"	*
 * instruction.  In other words, find out if combining has been done	*
 * which changed the "-1" to a different value.  We assume that the	*
 * "cal" instruction is the last inserted on "tip".			*
 *									*
 ************************************************************************/

static short get_cal_bump (tip)
TIP *tip;
{
   if (insert_unify) {
      assert (insert_unify->op->b->op_num == OP_CAL);

      return insert_unify->op->op.ins & 0xFFFF;
   }
   else {
      assert (tip->num_ops >= 1);
      assert (tip->op->op->b->op_num == OP_CAL);

      return tip->op->op->op.ins & 0xFFFF;
   }
}

/************************************************************************
 *									*
 *			    gen_cmpi_ctr				*
 *			    ------------				*
 *									*
 * Fill in fields in "op_cmpi" for counter compare instruction:		*
 *									*
 *		cmpi  <bf>,r32,cal_bump  (if in same tip as "cal" ins	*
 *		cmpi  <bf>,r32,0         (if not)			*
 *									*
 * As part of this, find a non-PPC architected condition code register	*
 * to hold the result, and find the earliest tip at or after the "cal"	*
 * instruction in which the "cmpi" may be placed.			*
 *									*
 * RETURNS:  The (non PPC architected) condition code in which the	*
 *	     "cmpi" result is placed.					*
 *									*
 ************************************************************************/

static int gen_cmpi_ctr (orig_tip, ins_cal_tip, cal_bump, earliest_tip)
TIP   *orig_tip;
TIP   *ins_cal_tip;
short cal_bump;
TIP   **earliest_tip;
{
   unsigned	  ins;
   unsigned short vliw_ccr;

   assert (bct_cnt < MAX_BCTS_PER_PAGE);

   ins  = op_cmpi.op.ins;
   ins &= 0xFFFF0000;
   ins |= cal_bump & 0xFFFF;
   op_cmpi.op.ins = ins;

   op_cmpi.op.wr = &cmpi_wr[4 * bct_cnt];
   op_cmpi.op.rd = &cmpi_rd[bct_cnt];
   bct_cnt++;

   vliw_ccr = get_bc_cmpi_start (orig_tip, ins_cal_tip, earliest_tip);

   op_cmpi.op.rd[0]     = INSADR; /* Dummy always avail, change to CTR later */
   op_cmpi.op.wr[0].reg = vliw_ccr + LT_BIT;
   op_cmpi.op.wr[1].reg = vliw_ccr + GT_BIT;
   op_cmpi.op.wr[2].reg = vliw_ccr + EQ_BIT;
   op_cmpi.op.wr[3].reg = vliw_ccr + OV_BIT;

   op_cmpi.op.renameable[BF & (~OPERAND_BIT)] = vliw_ccr;
   op_cmpi.op.renameable[RA & (~OPERAND_BIT)] = INSADR;	       /* Dummy ... */

   return vliw_ccr;
}

/************************************************************************
 *									*
 *			    get_bc_cmpi_start				*
 *			    -----------------				*
 *									*
 * Determine the earliest time at which "op_cau" may be placed,		*
 * accounting for whether a VLIW register is available, and return	*
 * the available VLIW register, and the "first_tip" at which it may	*
 * be used.								*
 *									*
 ************************************************************************/

int get_bc_cmpi_start (last_tip, ins_cal_tip, pfirst_tip)
TIP *last_tip;
TIP *ins_cal_tip;
TIP **pfirst_tip;
{
   int earliest;
   TIP *first_tip;

   earliest = ins_cal_tip->vliw->time;

   op_cmpi.op.wr[0].reg = CRF1+LT_BIT;/* Random renameable PPC CC reg field */
   op_cmpi.op.wr[1].reg = CRF1+GT_BIT;
   op_cmpi.op.wr[2].reg = CRF1+EQ_BIT;
   op_cmpi.op.wr[3].reg = CRF1+OV_BIT;

   while (TRUE) {
      first_tip = get_tip_at_time (last_tip, earliest);
      assert (first_tip);

      if (set_reg_for_op (&op_cmpi, first_tip, last_tip)) {
	 assert (curr_rename_reg_valid == TRUE);
	 curr_rename_reg_valid = FALSE;
	 *pfirst_tip = first_tip;
	 return curr_rename_reg;
      }
      else earliest++;
   }
}

/************************************************************************
 *									*
 *				fix_cmpi				*
 *				--------				*
 *									*
 * Patch the immediate value for "cmpi" if need be as well as the RA	*
 * register being compared.						*
 *									*
 ************************************************************************/

fix_cmpi (cmpi_ins_tip, cal_ins_tip, cal_commit_tip)
TIP *cmpi_ins_tip;
TIP *cal_ins_tip;
TIP *cal_commit_tip;
{
   int in_reg;
   int cmpi_time;

   if (cmpi_ins_tip == cal_ins_tip) {
     /* "cmpi" with "cal" in VLIW:  Change RA for "cmpi" to "cal" input */
     if (cal_unify)  in_reg =cal_unify->op->op.renameable[RZ & (~OPERAND_BIT)];
     else in_reg=cal_ins_tip->op->next->op->op.renameable[RZ & (~OPERAND_BIT)];
   }
   else {
      /* "cmpi" after "cal":  Change immediate in "cmpi" to 0 */
      fix_cmpi_immed (cmpi_ins_tip);

      /* "cmpi" after "cal":  Change RA for "cmpi" to "cal" result */
      if (cal_unify) in_reg =cal_unify->op->op.renameable[RT & (~OPERAND_BIT)];
      else    in_reg = cal_ins_tip->op->op->op.renameable[RT & (~OPERAND_BIT)];
      assert (!(in_reg >= SHDR0  &&  in_reg < SHDR0 + num_gprs));
      /* in_reg -= SHDR0; */

      /* If "cal" result renamed and already committed to CTR, use CTR */
      if (cmpi_unify) cmpi_time = cmpi_unify->set_time;
      else	      cmpi_time = cmpi_ins_tip->vliw->time;

      if (cal_commit_tip->vliw->time < cmpi_time) in_reg = CTR;
   }

   if (!cmpi_unify) fix_cmpi_ctr_in (cmpi_ins_tip, in_reg);
}

/************************************************************************
 *									*
 *				fix_cmpi_immed				*
 *				--------------				*
 *									*
 * Change immediate in "cmpi" to 0					*
 *									*
 ************************************************************************/

static fix_cmpi_immed (tip)
TIP *tip;
{

   if (cmpi_unify) {
      assert  (cmpi_unify->op->b->op_num == OP_CMPI);
      assert ((cmpi_unify->op->op.ins & 0x0000FFFF) == 0);
   }
   else {
      assert (tip->num_ops >= 1);
      assert (tip->op->op->b->op_num == OP_CMPI);

      tip->op->op->op.ins &= 0xFFFF0000;
   }
}

/************************************************************************
 *									*
 *				fix_cmpi_ctr_in				*
 *				---------------				*
 *									*
 * Routine called if "cmpi" instruction inserted in same VLIW that	*
 * decrements the CTR via a "cal" instruction.  If so, "cmpi" should	*
 * read the value of the CTR from the same place "cal" does, not from	*
 * the (rename) register to which "cal" writes the new value of CTR.	*
 *									*
 ************************************************************************/

int fix_cmpi_ctr_in (tip, correct_reg)
TIP *tip;
int correct_reg;
{
   OPCODE2 *op;

   op = tip->op->op;

   assert (op->op.rd[0] == INSADR);	/* Check for dummy place holders */
   assert (op->op.renameable[RA & (~OPERAND_BIT)] == INSADR);

   op->op.rd[0] = correct_reg;
   op->op.renameable[RA & (~OPERAND_BIT)] = correct_reg;

   return correct_reg;
}

/************************************************************************
 *									*
 *				gen_compound_cond			*
 *				-----------------			*
 *									*
 * Handle conditionals which branch on both the decremented counter	*
 * and some bit (BI) in the condition code register.  Combine the value	*
 * of the BI bit with the counter comparison already in "vliw_ccr".	*
 * Place the result in bit "vliw_ccr", i.e. if "vliw_ccr" is 1, the	*
 * branch will be taken, otherwise it will not.				*
 *									*
 ************************************************************************/

static void gen_compound_cond (bi_sense, ctr_sense, vliw_ccr, ppc_ccr)
int bi_sense;
int ctr_sense;
int vliw_ccr;
int ppc_ccr;
{
   int ba;
   int bb;
   int op_num;
   int order = 0;		/* 0 => CEQ,CEQ,BI   1 => CEQ,BI,CEQ */

   if      ( bi_sense == 0)
      if   (ctr_sense == 0) { op_num = OP_CRNOR;   op_cr.op.ins = 0x4C000042; }
      else                  { op_num = OP_CRANDC;  op_cr.op.ins = 0x4C000102; }
   else if (ctr_sense == 0) { op_num = OP_CRANDC;  op_cr.op.ins = 0x4C000102;
						   order = 1;		      }
   else			    { op_num = OP_CRAND;   op_cr.op.ins = 0x4C000202; }

   op_cr.b = &ppc_opcode[op_num]->b;

   if (order) {
      ba = ppc_ccr;
      bb = vliw_ccr + EQ_BIT;
   }
   else {
      ba = vliw_ccr + EQ_BIT;
      bb = ppc_ccr;
   }

   assert (bct_cr_cnt < MAX_BCT_CRS_PER_PAGE);
   op_cr.op.wr = &cr_wr[bct_cr_cnt];
   op_cr.op.rd = &cr_rd[2 * bct_cr_cnt];
   bct_cr_cnt++;

   /* Read into BA and BB, the values of vliw_ccr and ppc_ccr (BI) */
   op_cr.op.rd[0] = ba;
   op_cr.op.rd[1] = bb;
   op_cr.op.renameable[BA & (~OPERAND_BIT)] = ba;
   op_cr.op.renameable[BB & (~OPERAND_BIT)] = bb;

   /* Write result to vliw_ccr (BT), a non-PPC architected CCR bit */
   op_cr.op.wr[0].reg = vliw_ccr + EQ_BIT;
   op_cr.op.renameable[BT & (~OPERAND_BIT)] = vliw_ccr + EQ_BIT;
}

/************************************************************************
 *									*
 *				patch_bc				*
 *				--------				*
 *									*
 * 1. Change BO field to 4=br-if-false (BCT) or 12=br-if-true (BCF)	*
 * 2. Add dependence on non-architected bit that patch code sets in CR	*
 * 3. Take out any compound dependence on a bit in the CR		*
 * 4. Take out read and write dependences on CTR			*
 *									*
 ************************************************************************/

static void patch_bc (op, vliw_ccr, ctr_sense)
OPCODE2 *op;
int	vliw_ccr;
int	ctr_sense;
{
   int		  i;
   int		  num_wr = op->op.num_wr--;	/* No CTR write */
   int		  num_rd = op->op.num_rd--;	/* No CTR read  */
   RESULT	  *wr    = op->op.wr;
   unsigned short *rd    = op->op.rd;
   unsigned	  ins    = op->op.ins;

   ins &= 0xFC1FFFFF;	      /* 1. Replace BO field with 4 or 12 */
   if (ctr_sense) ins |= (12 << 21);
   else	          ins |= ( 4 << 21);
   op->op.ins = ins;
			      /* 2. Change bit branched on to cmpi result */
   op->op.renameable[BI & (~OPERAND_BIT)] = vliw_ccr + EQ_BIT;

   /* 2,3. Find CR bit (BI) in list of items read, replace it with vliw_ccr */
   for (i = 0; i < num_rd; i++) {
      if (rd[i] >= CR0  &&  rd[i] < CR0 + num_ccbits) {
	 rd[i] = vliw_ccr + EQ_BIT;
	 break;
      }
   }
   assert (i < num_rd);

   /* 4a. Find the CTR in list of items read */
   for (i = 0; i < num_rd; i++)
      if (rd[i] == CTR) break;

   assert (i < num_rd);

   /* 4b. Slide everything past the CTR down 1 */
   for (; i < num_rd - 1; i++)
      rd[i] = rd[i + 1];

   /* 4c. Find the CTR in list of items written */
   for (i = 0; i < num_wr; i++)
      if (wr[i].reg == CTR) break;

   assert (i < num_wr);

   /* 4d. Slide everything past the CTR down 1 */
   for (; i < num_wr - 1; i++)
      wr[i].reg = wr[i + 1].reg;
}

/************************************************************************
 *									*
 *				init_bct				*
 *				--------				*
 *									*
 * Setup the constant part of the opcodes "CAL", "CMPI", and "CR???"	*
 * used by BCT and BCT with condition.					*
 *									*
 ************************************************************************/

init_bct ()
{
   static unsigned char  cal_w_expl     =  RT    & (~OPERAND_BIT);
   static unsigned char  cmpi_w_expl    =  BF    & (~OPERAND_BIT);
   static unsigned char  cr_w_expl      =  BT    & (~OPERAND_BIT);
   static unsigned char  cal_r_expl[2]  = {RZ    & (~OPERAND_BIT),
					    D    & (~OPERAND_BIT)};
   static unsigned char  cmpi_r_expl[2] = {RA    & (~OPERAND_BIT),
					   SI    & (~OPERAND_BIT)};
   static unsigned char  cr_r_expl[2]   = {BA    & (~OPERAND_BIT),
					   BB    & (~OPERAND_BIT)};

   assert (CTR == R32);

   /* cal  R32,-1(R32) */
   op_cal.b	    = &ppc_opcode[OP_CAL]->b;
   op_cal.op.ins    = 0x3800FFFF;
   op_cal.op.num_wr = 1;
   op_cal.op.num_rd = 1;
   op_cal.op.wr_explicit = &cal_w_expl;
   op_cal.op.rd_explicit =  cal_r_expl;
   op_cal.op.srcdest_common = -1;
   cal_latency = get_max_latency1 (ppc_opcode[OP_CAL]);

   /* cmpi <bf>,R32,1 */
   op_cmpi.b	     = &ppc_opcode[OP_CMPI]->b;
   op_cmpi.op.ins    = 0x2C000000;
   op_cmpi.op.num_wr = 4;
   op_cmpi.op.num_rd = 1;
   op_cmpi.op.wr_explicit = &cmpi_w_expl;
   op_cmpi.op.rd_explicit =  cmpi_r_expl;
   op_cmpi.op.srcdest_common = -1;
   cmpi_latency = get_max_latency1 (ppc_opcode[OP_CMPI]);

   /* cr_op <bt>,<ba>,<bb>,    where op = NOR, ANDC, AND */
   op_cr.op.num_wr = 1;
   op_cr.op.num_rd = 2;
   op_cr.op.wr_explicit = &cr_w_expl;
   op_cr.op.rd_explicit =  cr_r_expl;
   op_cr.op.srcdest_common = -1;
   cal_latency = get_max_latency1 (ppc_opcode[OP_CRNOR]);

   init_bct_latencies ();
}

/************************************************************************
 *									*
 *				init_bct_latencies			*
 *				------------------			*
 *									*
 ************************************************************************/

static init_bct_latencies ()
{
   int    i;
   int	  cal_latency;
   int	  cmpi_latency;
   int	  cr_latency;

   assert (ppc_opcode[OP_CAL]->op.num_wr  == 1);
   assert (ppc_opcode[OP_CMPI]->op.num_wr == 1);
   assert (ppc_opcode[OP_NOR]->op.num_wr  == 1);
   cal_latency  = ppc_opcode[OP_CAL]->op.wr_lat[0];
   cmpi_latency = ppc_opcode[OP_CMPI]->op.wr_lat[0];
   cr_latency = ppc_opcode[OP_NOR]->op.wr_lat[0];

   for (i = 0; i < MAX_BCTS_PER_PAGE; i++)
      cal_wr[i].latency = cal_latency;

   for (i = 0; i < 4*MAX_BCTS_PER_PAGE; i++)
      cmpi_wr[i].latency = cmpi_latency;

   for (i = 0; i < MAX_BCT_CRS_PER_PAGE; i++)
      cr_wr[i].latency = cr_latency;
}

/************************************************************************
 *									*
 *				init_page_bct				*
 *				-------------				*
 *									*
 ************************************************************************/

init_page_bct ()
{
   op_cal.op.wr  = cal_wr;
   op_cal.op.rd  = cal_rd;
   op_cmpi.op.wr = cmpi_wr;
   op_cmpi.op.rd = cmpi_rd;
   op_cr.op.wr   = cr_wr;
   op_cr.op.rd   = cr_rd;

   bct_cnt    = 0;
   bct_cr_cnt = 0;
}
