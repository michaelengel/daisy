/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "offset.h"
#include "dis.h"
#include "dis_tbl.h"

/************************************************************************
 *									*
 *				get_offset				*
 *				----------				*
 *									*
 * Return a 32-bit value, whose upper 4 bits indicate the type of	*
 *        offset, and whose lower 28 bits indicate the value of the	*
 *	  offset. (It is up to the user to sign extend when necessary.)	*
 *									*
 ************************************************************************/

unsigned get_offset (op)
OPCODE2 *op;
{
   unsigned rval;

   switch (op->b->op_num) {

      case OP_RLMI:		    /* 10 bits */
      case OP_RLMI_:
      case OP_RLNM:
      case OP_RLNM_:
         rval = (MB_ME << 28)         | ((op->op.ins >> 1) & 0x03FF);
         break;

      case OP_RLIMI:		    /* 15 bits */
      case OP_RLIMI_:
      case OP_RLINM:
      case OP_RLINM_:
         rval = (SH_MB_ME << 28)      | ((op->op.ins >> 1) & 0x7FFF);
         break;

      case OP_TI:		    /* 16 bits */
      case OP_MULI:
      case OP_SFI:
      case OP_DOZI:
      case OP_CMPI:
      case OP_AI:
      case OP_AI_:
      case OP_CAL:
         rval = (ALU_SIGNED16 << 28)  | (op->op.ins        & 0xFFFF);
         break;

      case OP_LBZ:
      case OP_LHZ:
      case OP_LHA:
      case OP_L:
      case OP_LM:

      case OP_LBZU:
      case OP_LHZU:
      case OP_LHAU:
      case OP_LU:
 
      case OP_LFS:
      case OP_LFD:
      case OP_LFSU:
      case OP_LFDU:
         rval = (LD_SIGNED16 << 28)   | (op->op.ins        & 0xFFFF);
         break;

      case OP_STB:
      case OP_STH:
      case OP_ST:
      case OP_STM:

      case OP_STBU:
      case OP_STHU:
      case OP_STU:

      case OP_STFS:
      case OP_STFD:
      case OP_STFSU:
      case OP_STFDU:
         rval = (ST_SIGNED16 << 28)   | (op->op.ins        & 0xFFFF);
         break;

      case OP_CMPLI:
      case OP_ANDIL:
      case OP_ORIL:
      case OP_XORIL:
         rval = (ALU_UNSIGNED16 << 28) | (op->op.ins       & 0xFFFF);
         break;

      case OP_CAU:
      case OP_ANDIU:
      case OP_ORIU:
      case OP_XORIU:
         rval = (UP_UNSIGNED16 << 28)  | (op->op.ins       & 0xFFFF);
         break;

      case OP_BC:
      case OP_BCA:
      case OP_BCL:
      case OP_BCLA:
         rval = (BC_SIGNED14_16 << 28) | (op->op.ins       & 0xFFFC);
         break;

      case OP_B:
      case OP_BA:
      case OP_BL:
      case OP_BLA:
         rval = (B_SIGNED24_26 << 28)  | (op->op.ins       & 0x03FFFFFC);
         break;

      default:
	 rval = (NO_IMMEDS << 28);
	 break;
   }

   return rval;
}
