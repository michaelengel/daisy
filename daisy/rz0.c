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

static void set_xor_clr_reg (OPCODE2 *, int);

/************************************************************************
 *									*
 *				sched_ppc_rz0_op			*
 *				----------------			*
 *									*
 ************************************************************************/

int sched_ppc_rz0_op (ptip, op, earliest_in, p_ins_tip)
TIP     **ptip;
OPCODE2 *op;
int     earliest_in;
TIP     **p_ins_tip;
{
   int		  rz;
   int	          earliest;
   int	          earliest_dep;
   int		  lit_reg;
   int		  lit_clust;
   TIP		  *t;
   TIP		  *tip;
   TIP		  *lit_tip;
   TIP		  *lit_ins_tip;
   TIP		  *rz_tip;
   TIP		  *rz_ins_tip;
   unsigned short save_avail;
   unsigned char  save_clust;
   unsigned		       rz_ins;
   OPCODE1	  *lit_op1,   *rz_op1;
   OPCODE2	  *lit_op,    *rz_op;
   REG_RENAME     *save_rename;

   rz = op->op.renameable[RZ & (~OPERAND_BIT)];

   if (rz != R0)				        return FALSE;
   if (! (is_ppc_rz_op (op)  &&  !is_daisy_rz_op (op))) return FALSE;

   tip = *ptip;

   /* Use arbitrary register (R1) as dummy to in which to put literal 0 */
   save_avail  = tip->avail[R1];
   save_clust  = tip->cluster[R1];
   save_rename = tip->gpr_rename[R1 - R0];

   lit_op1 = get_opcode (0x7C210A78);		     /* xor r1,r1,r1 */
   lit_op  = set_operands (lit_op1, 0x7C210A78, lit_op1->b.format);

   lit_tip = insert_op (lit_op, tip, 0, &lit_ins_tip);

   /* Were we able to load the literal value prior to the last VLIW? */
   if (lit_ins_tip != lit_tip) {
      /* Delete the commit operation, and set resources appropriately */
      lit_tip->num_ops--;
      lit_clust   = lit_tip->op->cluster;
      lit_tip->op = lit_tip->op->next;
      lit_tip->vliw->cluster[lit_clust][ISSUE]++;
      lit_tip->vliw->cluster[lit_clust][MEMOP]++;

      assert (lit_tip->gpr_rename[R1-R0]->prev != 0);
      lit_reg = lit_tip->gpr_rename[R1-R0]->prev->vliw_reg;

      /* Always get the literal value from the non-architected register */
      lit_tip->gpr_rename[R1-R0] = lit_tip->gpr_rename[R1-R0]->prev;
   }
   else {
      /* Get a non-PPC architected register for the result.        */
      set_reg_for_op (lit_op, lit_ins_tip, lit_tip);
      assert (curr_rename_reg_valid);
      curr_rename_reg_valid = FALSE;

      /* Change the register referenced to be non-PPC-architected. */
      assert (lit_tip->gpr_rename[R1-R0]->prev == 0);
      lit_reg = lit_tip->gpr_rename[R1-R0]->vliw_reg = curr_rename_reg;
      set_xor_clr_reg (lit_tip->op->op, lit_reg);
   }

   /* Create a new op referencing r1.  From above, r1 will map to lit_reg */
   rz_ins  = op->op.ins;
   rz_ins |= (R1 - R0) << 16;	     /* Put R1 in the RZ field (which was 0) */
   rz_op1  = get_opcode (rz_ins);
   rz_op   = set_operands (rz_op1, rz_ins, rz_op1->b.format);

   if (earliest_in < 0)
      earliest = get_earliest_time (lit_tip, rz_op, FALSE);
   else {
      earliest_dep = lit_ins_tip->vliw->time + lit_op->op.wr[0].latency;
      if (earliest_dep > earliest_in) earliest = earliest_dep;
      else			      earliest = earliest_in;
   }

   rz_tip = insert_op (rz_op, lit_tip, earliest, &rz_ins_tip);

   /* Make sure reg containing literal 0 is marked used everywhere it should */
   /* This can be overly pessimistic, e.g. when no LOAD-VER is needed, but   */
   /* this situation should arise sufficiently infrequently that we won't    */
   /* worry about the wasted register.                                       */
   for (t = rz_tip; ; t = t->prev) {
      if (t != rz_tip)  clr_bit (t->gpr_vliw, lit_reg-R0);
      if (t == lit_tip) break;
   }

   /* Restore the real information about R1 */
   rz_tip->avail[R1]	       = save_avail;
   rz_tip->cluster[R1]	       = save_clust;
   rz_tip->gpr_rename[R1 - R0] = save_rename;

   *ptip      = rz_tip;
   *p_ins_tip = rz_ins_tip;

   return TRUE;
}

/************************************************************************
 *									*
 *				set_xor_clr_reg				*
 *				---------------				*
 *									*
 ************************************************************************/

static void set_xor_clr_reg (op, reg)
OPCODE2 *op;
int     reg;
{
   assert (op->b->op_num == OP_XOR);

   op->op.wr[0].reg = reg;
   op->op.rd[0]     = reg;
   op->op.rd[1]     = reg;

   op->op.renameable[RA & (~OPERAND_BIT)] = reg;
   op->op.renameable[RS & (~OPERAND_BIT)] = reg;
   op->op.renameable[RB & (~OPERAND_BIT)] = reg;
}
