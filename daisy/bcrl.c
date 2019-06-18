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
#include "resrc_list.h"
#include "dis_tbl.h"

extern OPCODE1 *ppc_opcode[];

static OPCODE2 m2spr_bcrl;

/************************************************************************
 *									*
 *				fixup_bcrl				*
 *				----------				*
 *									*
 * Branch-and-link instructions are always converted to plain branches	*
 * with the link register explicitly loaded before the branch.  This	*
 * causes a problem for BCR instructions because the link register	*
 * must also be loaded with the branch target before the branch.  To	*
 * overcome this problem, we introduce a second link register LR2, and	*
 * move the value in the original link register LR to it before the	*
 * branch.  The return address from the branch-and-link is still	*
 * explicitly loaded into the original LR (after LR2 has been set).	*
 *									*
 * Returns:  BR_INDIR_LR2, the type of branch to which opcode is	*
 *	     converted.							*
 *									*
 ************************************************************************/

int fixup_bcrl (ptip, opcode, p_ins)
TIP	 **ptip;
OPCODE2  *opcode;
unsigned *p_ins;
{
   int		  i;
   int		  earliest;
   int		  num_rd = opcode->op.num_rd;
   unsigned short *rd = opcode->op.rd;
   unsigned	  ins = *p_ins;
   TIP		  *tip = *ptip;
   TIP		  *ins_tip;

   /* 1st:  Patch the BCRL instruction to make it BCR2 */
   for (i = 0; i < num_rd; i++)
      if (rd[i] == LR) {  rd[i] = LR2;    break;  }

   opcode->b = &ppc_opcode[OP_BCR2]->b;

   ins &= 0xFFFFF800;
   ins |= 0x00000022;

   opcode->op.ins = ins;
   *p_ins         = ins;

   /* 2nd:  Put in an M2SPR instruction moving LR to LR2 */
   earliest = get_earliest_time (tip, &m2spr_bcrl, FALSE);
   if (earliest < tip->vliw->time) earliest = tip->vliw->time;

   /* NOTE:  Since this routine should be called before brlink()
	     and since earliest is no earlier than the current
	     time, the move from LR to LR2 should always occur
	     before LR is overwritten with the return address.
   */
   *ptip = insert_op (&m2spr_bcrl, tip, earliest, &ins_tip);

   return BR_INDIR_LR2;
}

/************************************************************************
 *									*
 *				init_fixup_bcrl				*
 *				---------------				*
 *									*
 * Make "m2spr" instruction moving LR to LR2 for use in "fixup_bcrl".	*
 *									*
 ************************************************************************/

init_fixup_bcrl ()
{
   static unsigned short _lr = LR;
   static RESULT	 _lr2;
   static unsigned char  w_expl = SPR_T & (~OPERAND_BIT);
   static unsigned char  r_expl = SPR_F & (~OPERAND_BIT);

   m2spr_bcrl.b = &ppc_opcode[OP_M2SPR]->b;

   assert         (ppc_opcode[OP_M2SPR]->op.num_wr == 1);
   _lr2.latency =  ppc_opcode[OP_M2SPR]->op.wr_lat[0];
   _lr2.reg     =  LR2;

   m2spr_bcrl.op.ins    = 0x7D4803A8;
   m2spr_bcrl.op.num_wr = 1;
   m2spr_bcrl.op.num_rd = 1;
   m2spr_bcrl.op.wr     = &_lr2;
   m2spr_bcrl.op.rd     = &_lr;
   m2spr_bcrl.op.wr_explicit = &w_expl;
   m2spr_bcrl.op.rd_explicit = &r_expl;
   m2spr_bcrl.op.srcdest_common = -1;
}

