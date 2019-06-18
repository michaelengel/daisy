/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "vliw.h"

extern int num_gprs;
extern int num_fprs;
extern int do_split_record;

/************************************************************************
 *									*
 *				split_record				*
 *				------------				*
 *									*
 *   The complex ops (other than add/subf) that	have the record bit	*
 *   and set the condition code at a late cycle:			*
 *									*
 *       op.   r3=							*
 *									*
 *   will probably be broken up into					*
 *									*
 *       op    r3=							*
 *       cmpi  cr0=r3,0							*
 *									*
 *   which is identical to op. r3=.					*
 *									*
 * For floating point record ops, the appropriate sequence is		*
 *									*
 *       op    fp1=							*
 *       fmr.  fp1,fp1							*
 *									*
 ************************************************************************/

int split_record (ptip, op, ins_ptr, start, end)
TIP	**ptip;
OPCODE2 *op;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    cmp_reg;
   int	    valid_cmp;				/* Boolean */
   int	    earliest;
   unsigned main_ins;
   unsigned cmp_ins;
   OPCODE1  *cmp_op1;
   OPCODE2  *cmp_op2;
   OPCODE1  *main_op1;
   OPCODE2  *main_op2;
   TIP	    *ins_tip;

   if (!do_split_record)           return FALSE;
   if (!is_complex_record_op (op)) return FALSE;

   if      (is_store_op (op)) {		/* Comparison based on src	    */
      assert (1 == 0);			/* Don't expect to encounter this   */
      cmp_reg = get_cmp_src_reg  (op->op.rd, op->op.num_rd, &valid_cmp);
   }
   else if (!is_idiv_op (op)) {		/* Comparison based on dest	    */
      cmp_reg = get_cmp_dest_reg (op->op.wr, op->op.num_wr, &valid_cmp);
   }
   else					/* Comparison based on MQ (remaind) */
      cmp_reg = MQ;

   if (!valid_cmp) return FALSE;

   /* Construct the main operation -- without the record bit set */
   if (op->b->primary == 13) main_ins = op->op.ins & 0xFBFFFFFF; /* ai.-> ai */
   else			     main_ins = op->op.ins & 0xFFFFFFFE;
   main_op1 = get_opcode (main_ins);
   main_op2 = set_operands (main_op1, main_ins, main_op1->b.format);

   /* Basic op may be combinable or update, let xlate_opcode_p2v handle it. */
   xlate_opcode_p2v (ptip, main_op2, main_ins, ins_ptr, start, end);


   /* Construct the cmpi / fmr. operation */
   if (cmp_reg >= R0  &&  cmp_reg < R0 + num_gprs) {
      cmp_ins = 0x2C000000  | (cmp_reg << 16);			  /* cmpi */
      cmp_op1 = get_opcode (cmp_ins);
   }
   else {
      cmp_ins = 0xFC000091 | (cmp_reg << 21) | (cmp_reg << 11);	  /* fmr. */
      cmp_op1 = get_opcode (cmp_ins);
   }
   cmp_op2 = set_operands (cmp_op1, cmp_ins, cmp_op1->b.format);

   earliest = get_earliest_time (*ptip, cmp_op2, FALSE);
   *ptip = insert_op (cmp_op2, *ptip, earliest, &ins_tip);

   return TRUE;
}

/************************************************************************
 *									*
 *				get_cmp_src_reg				*
 *				---------------				*
 *									*
 * If there is only a single GPR or FPR source register, return it	*
 * and make "valid_cmp" TRUE.  Otherwise make "valid_cmp" FALSE.	*
 *									*
 ************************************************************************/

static int get_cmp_src_reg (rd, num_rd, valid_cmp)
unsigned short	*rd;
int		num_rd;
int		*valid_cmp;	/* Bool:  TRUE if only one GPR/FPR src reg */
{
   int i;
   int rtn_reg;
   int num_valid = 0;

   for (i = 0; i < num_rd; i++) {
      if (rd[i] >= R0  && rd[i] < R0 + num_gprs) {
	 rtn_reg = rd[i];
	 num_valid++;
      }
      else if (rd[i] >= FP0  && rd[i] < FP0 + num_fprs) {
	 rtn_reg = rd[i];
	 num_valid++;
      }
   }

   if (num_valid == 1) *valid_cmp = TRUE;
   else		       *valid_cmp = FALSE;

   return rtn_reg;
}

/************************************************************************
 *									*
 *				get_cmp_dest_reg			*
 *				----------------			*
 *									*
 * If there is only a single GPR or FPR destination register, return it	*
 * and make "valid_cmp" TRUE.  Otherwise make "valid_cmp" FALSE.	*
 *									*
 ************************************************************************/

static int get_cmp_dest_reg (wr, num_wr, valid_cmp)
RESULT *wr;
int    num_wr;
int    *valid_cmp;	/* Bool:  TRUE if only one GPR/FPR dest reg */
{
   int i;
   int rtn_reg;
   int num_valid = 0;

   for (i = 0; i < num_wr; i++) {
      if (wr[i].reg >= R0  && wr[i].reg < R0 + num_gprs) {
	 rtn_reg = wr[i].reg;
	 num_valid++;
      }
      else if (wr[i].reg >= FP0  && wr[i].reg < FP0 + num_fprs) {
	 rtn_reg = wr[i].reg;
	 num_valid++;
      }
   }

   if (num_valid == 1) *valid_cmp = TRUE;
   else		       *valid_cmp = FALSE;

   return rtn_reg;
}
