/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "dis.h"
#include "dis_tbl.h"
#include "rd_config.h"

extern int do_copyprop;

static OPCODE2 *copy_to_or (int, int);

/************************************************************************
 *									*
 *				sched_copy_op				*
 *				-------------				*
 *									*
 * Returns: TRUE  if sched "op" as a copy, and caller need not sched op	*
 *	    FALSE otherwise,		   and caller should   sched op	*
 *									*
 * ALSO updates ptip to indicate current tip.				*
 *									*
 ************************************************************************/

int sched_copy_op (ptip, op, ins, earliest, p_ins_tip)
TIP	 **ptip;
OPCODE2  *op;
unsigned ins;
int	 earliest;		/* If negative, we should compute here */
TIP	 **p_ins_tip;		/* Output */
{
   int		src;
   int		dest;
   int		rval;
   int		avail_time;
   int	        is_ai_copy = FALSE;     /* Boolean */
   int		gpr_copy   = FALSE;	/* Boolean */
   int		fpr_copy   = FALSE;	/* Boolean */
   TIP		*tip	   = *ptip;
   TIP		*ins_tip   = *ptip;
   REG_RENAME	*mapping;

   if      (!do_copyprop) return FALSE;

   *p_ins_tip = ins_tip;

   gpr_copy = is_gpr_copy (op, ins, &src, &dest, &is_ai_copy);

   if (!gpr_copy) fpr_copy = is_fpr_copy (op, ins, &src, &dest);
   else if (is_ai_copy) 
      return handle_ai_copy (ptip, op, ins, earliest, src, dest, p_ins_tip);
   else {
      fpr_copy = FALSE;

      /* We standardly use OR as the COMMIT operation.  In order not
	 to confuse other code, convert non-OR GPR COPY's to OR GPR COPY's.
      */
      if (op->b->op_num != OP_OR  &&  !is_ai_copy) op = copy_to_or (src, dest);
   }
	    
   if (gpr_copy  ||  fpr_copy) {

      if (dest == src) return TRUE;		/* COPY's to self are NOP's */

      if (earliest < 0) {
	 earliest = get_earliest_time (tip, op, FALSE);
	 if (tip->vliw->time > earliest) earliest = tip->vliw->time;
      }
      tip = insert_op (op, tip, earliest, &ins_tip);
      tip->op->commit = TRUE;		  /* COPY acts just like a COMMIT */

      if (gpr_copy) {
	 mapping       = tip->gpr_rename[dest - R0];
         assert (mapping->prev == 0);

	 /* If the source has no definition in this group, i.e. we are using
	    the value at entry to the group, make the source map to itself
	    with availability at time 0.
	 */
	 if (!tip->gpr_rename[src - R0])  set_mapping (tip, src, src, 0, TRUE);
	 mapping->prev = tip->gpr_rename[src -  R0];
      }
      else {
	 mapping       = tip->fpr_rename[dest - FP0];
         assert (mapping->prev == 0);

	 if (!tip->fpr_rename[src - FP0]) set_mapping (tip, src, src, 0, TRUE);
	 mapping->prev = tip->fpr_rename[src - FP0];
      }

      for (; mapping; mapping = mapping->prev) avail_time = mapping->time;

      tip->avail[dest] = avail_time;
      rval = TRUE;
   }
   else rval = FALSE;

   *ptip        = tip;
   *p_ins_tip   = ins_tip;

   return rval;
}

/************************************************************************
 *									*
 *				handle_ai_copy				*
 *				--------------				*
 *									*
 * Returns: TRUE always.  (Assumes that "op" is an "ai rx,ry,0".	*
 *									*
 * ALSO updates ptip to indicate current tip.				*
 *									*
 ************************************************************************/

int handle_ai_copy (ptip, op, ins, earliest, src, dest, p_ins_tip)
TIP	 **ptip;
OPCODE2  *op;
unsigned ins;
int	 earliest;		/* If negative, we should compute here */
int	 src;
int	 dest;
TIP	 **p_ins_tip;		/* Output */
{
   int		avail_time;
   int		save_avail;
   TIP		*tip	   = *ptip;
   TIP		*ins_tip   = *ptip;
   REG_RENAME	*mapping;
   REG_RENAME	*save_mapping;

   if (earliest < 0) earliest = get_earliest_time (tip, op, FALSE);

   if (dest == src) {
      save_avail   = tip->avail[src];
      save_mapping = tip->gpr_rename[src-R0];
   }

   tip = insert_op (op, tip, earliest, &ins_tip);

   /* Two cases:  ai scheduled prior to current VLIW and not. */
   mapping = tip->gpr_rename[dest - R0];
   if (mapping->prev != 0) mapping = mapping->prev;
   assert (mapping->prev == 0);

   /* Even if the dest == src, we must do the operation, in order
      to clear the CA bit.  An update is also needed to restore the 
      the GPR destination mapping and availability time to their
      original values.
   */
   if (dest == src) {
      tip->avail[src]	    = save_avail;
      tip->gpr_rename[src-R0] = save_mapping;
   }
   else {
      /* If the source has no definition in this group, i.e. we are using
         the value at entry to the group, make the source map to itself
         with availability at time 0.
      */
      if (!tip->gpr_rename[src - R0])  set_mapping (tip, src, src, 0, TRUE);
      mapping->prev = tip->gpr_rename[src -  R0];
 
      for (; mapping; mapping = mapping->prev) avail_time = mapping->time;

      tip->avail[dest] = avail_time;
   }

   *ptip        = tip;
   *p_ins_tip   = ins_tip;

   return TRUE;
}

/************************************************************************
 *									*
 *				is_gpr_copy				*
 *				-----------				*
 *									*
 * Returns: TRUE  if "op" is a GPR copy, and indicate the src and dest,	*
 *	    FALSE otherwise.						*
 *									*
 * The following opcodes have forms which are recognized as COPYS:	*
 *									*
 *	CAL, OR, ORIL, ORIU, XORIL, XORIU, RLINM			*
 *									*
 * Note that AI is not in the list.  This is because it also sets CA.	*
 *									*
 ************************************************************************/

int is_gpr_copy (op, ins, src, dest, is_ai_copy)
OPCODE2  *op;
unsigned ins;
int      *src;
int      *dest;
int	 *is_ai_copy;
{
   *is_ai_copy = FALSE;

   if      (op->b->op_num == OP_CAL		    && 
	   (ins & 0xFFFF) == 0			    &&
	    op->op.num_rd != 0) {
      *src  = op->op.renameable[RZ & (~OPERAND_BIT)];
      *dest = op->op.renameable[RT & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_OR		    && 
	    op->op.renameable[RS & (~OPERAND_BIT)] ==
	    op->op.renameable[RB & (~OPERAND_BIT)]) {
      *src  = op->op.renameable[RS & (~OPERAND_BIT)];
      *dest = op->op.renameable[RA & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_ORIL		    && 
	   (ins & 0xFFFF) == 0) {
      *src  = op->op.renameable[RS & (~OPERAND_BIT)];
      *dest = op->op.renameable[RA & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_ORIU		    && 
	   (ins & 0xFFFF) == 0) {
      *src  = op->op.renameable[RS & (~OPERAND_BIT)];
      *dest = op->op.renameable[RA & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_XORIL		    && 
	   (ins & 0xFFFF) == 0) {
      *src  = op->op.renameable[RS & (~OPERAND_BIT)];
      *dest = op->op.renameable[RA & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_XORIU		    && 
	   (ins & 0xFFFF) == 0) {
      *src  = op->op.renameable[RS & (~OPERAND_BIT)];
      *dest = op->op.renameable[RA & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_RLINM		    && 
	   (ins & 0xFFFF) == 0x3E) {
      *src  = op->op.renameable[RS & (~OPERAND_BIT)];
      *dest = op->op.renameable[RA & (~OPERAND_BIT)];
      return TRUE;
   }
   else if (op->b->op_num == OP_AI		    && 
	   (ins & 0xFFFF) == 0) {
      *src  = op->op.renameable[RA & (~OPERAND_BIT)];
      *dest = op->op.renameable[RT & (~OPERAND_BIT)];
      *is_ai_copy = TRUE;
      return TRUE;
   }
   else return FALSE;
}

/************************************************************************
 *									*
 *				is_fpr_copy				*
 *				-----------				*
 *									*
 * Returns: TRUE  if "op" is an FPR copy, and indicate the src and dest,*
 *	    FALSE otherwise.						*
 *									*
 * FMR is the only opcodes which is recognized as a COPY.		*
 *									*
 ************************************************************************/

int is_fpr_copy (op, ins, src, dest)
OPCODE2 *op;
unsigned ins;
int     *src;
int     *dest;
{
   if (op->b->op_num == OP_FMR) {
      *src  = op->op.renameable[FRB & (~OPERAND_BIT)];
      *dest = op->op.renameable[FRT & (~OPERAND_BIT)];
      return TRUE;
   }
   else return FALSE;
}

/************************************************************************
 *									*
 *				copy_to_or				*
 *				----------				*
 *									*
 * We standardly use OR as the COMMIT operation.  In order not		*
 * to confuse other code, convert non-OR GPR COPY's to OR GPR COPY's.	*
 *									*
 ************************************************************************/

static OPCODE2 *copy_to_or (src, dest)
int src;
int dest;
{
   OPCODE1  *op;
   OPCODE2  *op_rtn;
   unsigned or_ins = 0x7C000378;

   if (src < NUM_PPC_GPRS) {
      or_ins |= src  << 21;
      or_ins |= src  << 11;
   }
   if (dest < NUM_PPC_GPRS) {
      or_ins |= dest << 16;
   }

   op = get_opcode (or_ins);
   op_rtn = set_operands (op, or_ins, op->b.format);

   if (src >= NUM_PPC_GPRS) {
      op_rtn->op.renameable[RS & (~OPERAND_BIT)] = src;
      op_rtn->op.renameable[RB & (~OPERAND_BIT)] = src;
      op_rtn->op.rd[0] = src;
      op_rtn->op.rd[1] = src;
   }
   if (dest >= NUM_PPC_GPRS) {
      op_rtn->op.renameable[RA & (~OPERAND_BIT)] = dest;
      op_rtn->op.wr[0].reg = dest;
   }

   return op_rtn;
}
