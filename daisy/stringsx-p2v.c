/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "dis.h"
#include "vliw.h"

/************************************************************************
 *									*
 *				xlate_lsx_p2v				*
 *				-------------				*
 *									*
 ************************************************************************/

int xlate_lsx_p2v (ptip, ins, ins_ptr, end, opcode)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *end;
OPCODE1  *opcode;
{
   int	    earliest;
   OPCODE2  *opcode_out;
   TIP	    *ins_tip;
   TIP	    *tip = *ptip;

   opcode_out = set_operands (opcode, ins, opcode->b.format);

   opcode_out->op.renameable[RT & (~OPERAND_BIT)] = (ins >> 21) & 0x1F;
   opcode_out->op.renameable[RZ & (~OPERAND_BIT)] = (ins >> 16) & 0x1F;
   opcode_out->op.renameable[RB & (~OPERAND_BIT)] = (ins >> 11) & 0x1F;

   earliest = get_earliest_time (tip, opcode_out, 0);
   if (earliest < tip->vliw->time) earliest = tip->vliw->time;
   *ptip = insert_op (opcode_out, tip, earliest, &ins_tip);

   if (ins_ptr == end) return 4;
   else return 0;
}

/************************************************************************
 *									*
 *				xlate_lscbx_p2v				*
 *				---------------				*
 *									*
 ************************************************************************/

int xlate_lscbx_p2v (ptip, ins, ins_ptr, end, opcode)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *end;
OPCODE1  *opcode;
{
   int	    earliest;
   OPCODE2  *opcode_out;
   TIP	    *ins_tip;
   TIP	    *tip = *ptip;

   opcode_out = set_operands (opcode, ins, opcode->b.format);

   opcode_out->op.renameable[RT & (~OPERAND_BIT)] = (ins >> 21) & 0x1F;
   opcode_out->op.renameable[RZ & (~OPERAND_BIT)] = (ins >> 16) & 0x1F;
   opcode_out->op.renameable[RB & (~OPERAND_BIT)] = (ins >> 11) & 0x1F;

   earliest = get_earliest_time (tip, opcode_out, 0);
   if (earliest < tip->vliw->time) earliest = tip->vliw->time;
   *ptip = insert_op (opcode_out, tip, earliest, &ins_tip);

   if (ins_ptr == end) return 4;
   else return 0;
}

/************************************************************************
 *									*
 *				xlate_stsx_p2v				*
 *				--------------				*
 *									*
 ************************************************************************/

int xlate_stsx_p2v (ptip, ins, ins_ptr, end, opcode)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *end;
OPCODE1  *opcode;
{
   int      gpr;
   int	    earliest;
   OPCODE2  *opcode_out;
   TIP	    *ins_tip;
   TIP	    *tip = *ptip;

   opcode_out = set_operands (opcode, ins, opcode->b.format);

   opcode_out->op.renameable[RS & (~OPERAND_BIT)] = (ins >> 21) & 0x1F;
   opcode_out->op.renameable[RZ & (~OPERAND_BIT)] = (ins >> 16) & 0x1F;
   opcode_out->op.renameable[RB & (~OPERAND_BIT)] = (ins >> 11) & 0x1F;

   earliest = get_earliest_time (tip, opcode_out, 0);
   if (earliest < tip->vliw->time) earliest = tip->vliw->time;

   /* Since stsx potentially reads every GPR, all PPC architected
      registers must contain their latest value.  For example, if
      r3 maps to r63 at the currently last VLIW, another VLIW must
      be added so that the latest value of r3 is in r3, not just r63.
   */
   for (gpr = 0; gpr < NUM_PPC_GPRS; gpr++) {
      if (earliest < tip->gpr_rename[gpr]->time)
          earliest = tip->gpr_rename[gpr]->time;
   }

   *ptip = insert_op (opcode_out, tip, earliest, &ins_tip);

   if (ins_ptr == end) return 4;
   else return 0;
}
