/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>

#include "vliw.h"
#include "dis.h"

/************************************************************************
 *									*
 *				xlate_addic_0				*
 *				-------------				*
 *									*
 * The Tobey compiler sometimes produces the op, "addic.  Rx,Ry,0"	*
 * to move Ry to Rx and set CR0 appropriately.  This serializes		*
 * under DAISY, so we expand it to:					*
 *									*
 *	       cmpi   0,Ry,0						*
 *	       addic  Rx,Ry,0						*
 *									*
 * In this way, the 3 results, Rx, cr0, and CA can all be renamed and	*
 * the ops scheduled as early as data dependences allow.  Note that we	*
 * cannot do this transformation with arbitrary constants, N, because	*
 * of finite precision rollover effects.  For example if Ry=2^31 - 1	*
 * and N=1, the sum is -2^31, which is LESS THAN 0.  However 2^31 - 1	*
 * is GREATER THAN -N=-1.						*
 *									*
 ************************************************************************/

int xlate_addic_0 (ptip, opcode, ins, ins_ptr, start, end, status)
TIP	 **ptip;
OPCODE1  *opcode;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
int	 *status;
{
   int	    ra;
   unsigned cmpi_ins;
   unsigned ai_ins;
   OPCODE2  *opcode_out;

   if (opcode->b.primary   != 13) return FALSE;	   /* Status does not matter */
   else if ((ins & 0xFFFF) !=  0) return FALSE;

   ra = (ins >> 16) & 0x1F;

   cmpi_ins = 0x2C000000 | (ra << 16);
   opcode = get_opcode (cmpi_ins);
   opcode_out = set_operands (opcode, cmpi_ins, opcode->b.format);
   *status = xlate_opcode_p2v (ptip, opcode_out, cmpi_ins, ins_ptr,
			       start, end);

   ai_ins = (ins & 0x3FFFFFF) | (12 << 26);
   opcode = get_opcode (ai_ins);
   opcode_out = set_operands (opcode, ai_ins, opcode->b.format);
   *status = xlate_opcode_p2v (ptip, opcode_out, ai_ins, ins_ptr,
			       start, end);

   return TRUE;
}
