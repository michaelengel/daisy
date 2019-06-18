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

/************************************************************************
 *									*
 *				del_dead_tip_ops			*
 *				----------------			*
 *									*
 * Delete any writes to the same register in the same tip (assuming	*
 * that the previous write to the same register had only that register	*
 * as a destination.)  This routine should typically be invoked after	*
 * each operation is scheduled.						*
 *									*
 ************************************************************************/

del_dead_tip_ops (tip, reg, old_avail, first_old_op)
TIP     *tip;
int     reg;
int     old_avail;     /* Time reg was available before we scheduled this op */
BASE_OP *first_old_op; /* First op prior to any primitives for curr PPC op   */
{
   int     clust;
   BASE_OP *bop;
   BASE_OP *old_bop;
   OPCODE2 *op;
   int     resrc_needed[NUM_VLIW_RESRC_TYPES+1];

   /* Negative means that the primary destination of this register
      is something other than a PowerPC architected GPR, FPR, or CCR.
   */
   if (old_avail < 0)	      return;
   if (!(old_bop = tip->op))  return;	     /* No ops on tip */

   if (tip->gpr_rename[reg]->time == old_avail) {

      /* A PowerPC op may be expanded to multiple VLIW primitives.
         Make sure we start looking for operations to delete prior
         to any of the VLIW primitives for the newly inserted PowerPC op.
      */
      for (;  old_bop->next != first_old_op; old_bop = old_bop->next) {
         if      (!old_bop->next)        return;
         else if (old_bop->op->op.verif) return;  /* Inserted ins was VERIFY */
      }

      if         (old_bop->op->op.verif) return;  /* Inserted ins was VERIFY */
      for (bop = first_old_op;  bop;  bop = bop->next) {
	 op = bop->op;

	 /* We can't delete anything before a LOAD-VERIFY because registers
	    will not have the correct value for the recovery code if the
	    LOAD-VERIFY fails.
	 */
	 if      (op->op.verif) break;	  
	 else if (op->op.num_wr == 1) {
	    if (op->op.wr[0].reg == reg) {

	       /* Delete this dead op, and increase resources accordingly */
	       tip->num_ops--;
	       clust = bop->cluster;
	       get_needed_resrcs (op, resrc_needed);
	       decr_resrc_usage (tip, tip->vliw->resrc_cnt, resrc_needed,
				 clust);
	       old_bop->next = bop->next;

	       break;
	    }
	 }
	 old_bop = bop;
      }
   }
}

/************************************************************************
 *									*
 *				get_dest_avail_time			*
 *				-------------------			*
 *									*
 * This routine is generally invoked prior to scheduling an operation.	*
 * If the op to be scheduled has only a single register destination,	*
 * the availability time of that register is returned.  This is useful	*
 * after scheduling the op for quickly determining whether the op	*
 * killed a write to the same destination in the same tip, and hence	*
 * whether the earlier op can be deleted (thus freeing up resources).	*
 *									*
 * Returns:  If op has a single PowerPC architected GPR, FPR, or CCR	*
 *	     dest reg, its avail time is returned.			*
 *	     O.w. -1 is returned.					*
 *									*
 ************************************************************************/

int get_dest_avail_time (tip, op, p_reg, prior_op)
TIP     *tip;
OPCODE2 *op;
int     *p_reg;
BASE_OP **prior_op;
{
   int reg;

   if (op->op.num_wr == 0) return -1;
   else {
      /* We assume that the first register is the primary destination */
      reg       = op->op.wr[0].reg;
      *prior_op = tip->op;

      if      (reg >=  R0  &&  reg <  R0 + NUM_PPC_GPRS) {
	 *p_reg = reg;
	 return tip->gpr_rename[reg-R0]->time;
      }
      else if (reg >= FP0  &&  reg < FP0 + NUM_PPC_FPRS) {
	 *p_reg = reg;
	 return tip->fpr_rename[reg-FP0]->time;
      }
      else if (reg >= CR0  &&  reg < CR0 + NUM_PPC_CCRS) {
	 *p_reg = reg;
	 return tip->ccr_rename[reg-CR0]->time;
      }
      else return -1;
   }
}
