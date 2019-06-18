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
#include "dis_tbl.h"

extern int num_gprs;
extern int curr_rename_reg;
extern int curr_rename_reg_valid;
extern int commit_resrc_needed[];

static void set_ry_reg (OPCODE2 *, int);

/************************************************************************
 *									*
 *				sched_rlwimi				*
 *				------------				*
 *									*
 * Expand   rlwimi  ra,rs,sh,mb,me   into the following 3 ops:		*
 *									*
 *	    rlinm   ry,rs,sh,mb,me		 ; ry = field to insert *
 *	    rlinm   ra,ra,0,(me+1)%32,(mb-1)%32  ; Clr ins field in ra  *
 *	    or	    ra,ra,ry			 ; Insert ry into ra	*
 *									*
 * Notes:  ry is non-PowerPC architected register, i.e. >= 34.		*
 *	   The 2nd rlinm must be scheduled after th first in case	*
 *	   ra == rs.							*
 *									*
 ************************************************************************/

int sched_rlwimi (ptip, op, p_ins_tip)
TIP     **ptip;
OPCODE2 *op;
TIP     **p_ins_tip;
{
   int		  ra;
   int		  rs;
   int	          or_earliest;
   int		  mask_reg;
   TIP		  *t;
   TIP		  *tip;
   TIP		  *mask_tip;
   TIP		  *mask_ins_tip;
   TIP		  *invmask_tip;
   TIP		  *invmask_ins_tip;
   TIP		  *or_tip;
   TIP		  *or_ins_tip;
   unsigned short ra_avail, ry_avail;
   unsigned short save_avail;
   unsigned char  save_clust;
   unsigned	  mb, inv_mb;
   unsigned	  me, inv_me;
   unsigned				       rlwimi_ins;
   unsigned				       mask_ins;
   unsigned				       invmask_ins;
   unsigned				       or_ins;
   OPCODE1	  *mask_op1,   *invmask_op1,  *or_op1;
   OPCODE2	  *mask_op,    *invmask_op,   *or_op;
   REG_RENAME     *save_rename;

   if (op->b->op_num != OP_RLIMI  &&  op->b->op_num != OP_RLIMI_) return FALSE;

   ra = op->op.renameable[RA & (~OPERAND_BIT)];
   rs = op->op.renameable[RS & (~OPERAND_BIT)];
   rlwimi_ins = op->op.ins;

   tip = *ptip;

   /* Use arbitrary register (R1) as dummy to in which to put ry */
   save_avail  = tip->avail[R1];
   save_clust  = tip->cluster[R1];
   save_rename = tip->gpr_rename[R1 - R0];

   /* rlinm r1,rs,sh,mb,me */
   mask_ins = 0x54010000 | ((rs-R0) << 21) | (rlwimi_ins & 0xFFFE);
   mask_op1 = get_opcode (mask_ins);
   mask_op  = set_operands (mask_op1, mask_ins, mask_op1->b.format);
   mask_tip = insert_op (mask_op, tip, tip->avail[rs], &mask_ins_tip);
   mask_reg = fix_ry_reg (mask_tip, mask_ins_tip, mask_op);

   /* rlinm ra,ra,0,(me+1)%32,(mb-1)%32 */
   mb = (op->op.ins >> 6) & 0x1F;
   me = (op->op.ins >> 1) & 0x1F;
   inv_mb = (me + 1) & 0x1F;
   inv_me = (mb - 1) & 0x1F;
   invmask_ins = 0x54000000 |((ra-R0) << 21) |((ra-R0) << 16) |
			      (inv_mb <<  6) | (inv_me <<  1);
   invmask_op1 = get_opcode (invmask_ins);
   invmask_op  = set_operands (invmask_op1, invmask_ins, 
				    invmask_op1->b.format);
   invmask_tip = insert_op (invmask_op, mask_tip, tip->avail[ra],
			   &invmask_ins_tip);

   /* Create new op OR'ing r1 and ra into ra.  From above, r1 will map to ry */
   or_ins  = 0x7C000378 | ((ra-R0) << 21) | ((ra-R0) << 16) | ((R1-R0) << 11);
   or_op1  = get_opcode (or_ins);
   or_op   = set_operands (or_op1, or_ins, or_op1->b.format);

   ra_avail = invmask_ins_tip->vliw->time + invmask_op->op.wr[0].latency;
   ry_avail =    mask_ins_tip->vliw->time +    mask_op->op.wr[0].latency;
   or_earliest = (ra_avail > ry_avail) ? ra_avail : ry_avail;

   or_tip = insert_op (or_op, invmask_tip, or_earliest, &or_ins_tip);

   /* Make sure reg containing ry is marked used everywhere it should */
   for (t = or_ins_tip; ; t = t->prev) {
      if (t != or_ins_tip)   clr_bit (t->gpr_vliw, mask_reg-R0);
      if (t == mask_ins_tip) break;
   }

   /* Restore the real information about R1 */
   or_tip->avail[R1]	     = save_avail;
   or_tip->cluster[R1]	     = save_clust;
   or_tip->gpr_rename[R1 - R0] = save_rename;

   *ptip      = or_tip;
   *p_ins_tip = or_ins_tip;

   return TRUE;
}

/************************************************************************
 *									*
 *				fix_ry_reg				*
 *				----------				*
 *									*
 * Patch the result of the mask operation to be in a non-architected	*
 * register, and delete any superfluous commit operation of that is	*
 * generated.  Make sure that R1 (the dummy reg) maps to the non-	*
 * architected register.						*
 *									*
 * Returns:  The non-architected register holding the mask field.	*
 *									*
 ************************************************************************/

static int fix_ry_reg (mask_tip, mask_ins_tip, mask_op)
TIP     *mask_tip;
TIP     *mask_ins_tip;
OPCODE2 *mask_op;
{
   int mask_reg;
   int mask_clust;

   /* Were we able to load ry prior to the last VLIW? */
   if (mask_ins_tip != mask_tip) {
      /* Delete the commit operation, and set resources appropriately */
      mask_tip->num_ops--;
      mask_clust   = mask_tip->op->cluster;
      mask_tip->op = mask_tip->op->next;
      decr_resrc_usage (mask_tip, mask_tip->vliw->resrc_cnt,
			commit_resrc_needed, mask_clust);

      assert (mask_tip->gpr_rename[R1-R0]->prev != 0);
      mask_reg = mask_tip->gpr_rename[R1-R0]->prev->vliw_reg;

      /* Always get ry from the non-architected register */
      mask_tip->gpr_rename[R1-R0] = mask_tip->gpr_rename[R1-R0]->prev;
   }
   else {
      /* Get a non-PPC architected register for the result.        */
      set_reg_for_op (mask_op, mask_ins_tip, mask_tip);
      assert (curr_rename_reg_valid);
      curr_rename_reg_valid = FALSE;

      /* Change the register referenced to be non-PPC-architected. */
      assert (mask_tip->gpr_rename[R1-R0]->prev == 0);
      mask_reg = mask_tip->gpr_rename[R1-R0]->vliw_reg = curr_rename_reg;
      set_ry_reg (mask_tip->op->op, mask_reg);
   }

   return mask_reg;
}

/************************************************************************
 *									*
 *				set_ry_reg				*
 *				----------				*
 *									*
 ************************************************************************/

static void set_ry_reg (op, reg)
OPCODE2 *op;
int     reg;
{
   assert (op->b->op_num == OP_RLINM);

   op->op.wr[0].reg = reg;

   op->op.renameable[RA & (~OPERAND_BIT)] = reg;
}
