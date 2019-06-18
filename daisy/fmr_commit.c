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
#include "resrc_map.h"
#include "resrc_offset.h"
#include "gen_resrc.h"

extern int num_fprs;

/************************************************************************
 *									*
 *				save_fpscr_on_specul			*
 *				--------------------			*
 *									*
 * Save FPSCR to a scratch location if we have speculative operation	*
 * which sets it.  Return the FPR to which the speculative result is	*
 * written.								*
 *									*
 ************************************************************************/

int save_fpscr_on_specul (op)
OPCODE2 *op;
{
   int fpr_ext;

   if (!renameable_fp_arith_op (op)) return -1;

   fpr_ext = op->op.renameable[FRT & (~OPERAND_BIT)];
   if (fpr_ext < FP0 + NUM_PPC_FPRS  ||  fpr_ext >= FP0 + num_fprs)
      return -1;

   dump (0xFFE0048E);			/* mffs fp31	 */
   dump (0xDBE00000 | (RESRC_PTR << 16) | FPSCR_SCR_OFFSET);
					/* stfd	 fp31,FPSCR_SCR_OFFSET(r13) */

   return fpr_ext;
}

/************************************************************************
 *									*
 *			handle_result_fpscr_on_specul			*
 *			-----------------------------			*
 *									*
 * (1) Put the FPSCR into the extender bits associated with "fpr_ext".	*
 * (2) Restore the FPSCR to its value before this instruction.		*
 *									*
 ************************************************************************/

handle_result_fpscr_on_specul (fpr_ext, fpscr_shad_vec)
int	 fpr_ext;
unsigned *fpscr_shad_vec;
{
   int fpr_ext_off;

   set_bit (fpscr_shad_vec, fpr_ext);

   fpr_ext_off = SHDFP0_FPSCR_OFFSET + SIMFPSCR_SIZE * (fpr_ext - FP0);
   dump (0xFFE0048E);			/* mffs  fp31			    */
   dump (0xDBE00000 | (RESRC_PTR << 16) | fpr_ext_off);	
					/* stfd  fp31,FPR_FPSCR_EXT(r13)    */

					/* lfd   fp31,FPSCR_SCR_OFFSET(r13) */
   dump (0xCBE00000 | (RESRC_PTR << 16) | FPSCR_SCR_OFFSET);	
   dump (0xFDFEFD8E);			/* mtfsf 0xFF,fp31		    */
}

/************************************************************************
 *									*
 *				handle_fmrX_commit			*
 *				------------------			*
 *									*
 * Returns 0 if not a FMR commit (that sets FPSCR), non-zero otherwise.	*
 *									*
 * (1) Put in fp31 the current FPSCR contents.				*
 * (2) Move fp31 to a scratch 8-byte memory location.			*
 * (3) Copy to r31 the low 4-bytes of the scratch memory location.	*
 * (4) Put in r30 the FPSCR from the speculative op being committed.	*
 * (5) Create in r29 a mask for sticky exception bits being committed.	*
 * (6) AND mask in r29 with r30 to obtain desired set of sticky bits.	*
 * (7) OR any set sticky bits into the current FPSCR in r31.		*
 * (8) Set bits 15-19 of current FPSCR directly with those from result.	*
 * (9) Put the result (in r31) back to scratch memory location.		*
 *(10) Load scratch memory location into fp31.				*
 *(11) Set bits 0-19 of the FPSCR with the updated values.		*
 *									*
 ************************************************************************/

int handle_fmrX_commit (op)
OPCODE2 *op;
{
   unsigned or_mask;
   int      fpr_ext;
   int      fpr_ext_off;

   /* Set mask to indicate sticky bits to set in FPSCR (upper 16 bits of 32) */
   switch (op->b->op_num) {
      case OP_FMR1:  or_mask = 0xFB86;	break;	/* Bits 0-4,6-8,     13-14 */
      case OP_FMR2:  or_mask = 0xFB16;	break;	/* Bits 0-4,6-7,11,  13-14 */
      case OP_FMR3:  or_mask = 0xFB96;	break;	/* Bits 0-4,6-8,11 , 13-14 */
      case OP_FMR4:  or_mask = 0xFF66;	break;	/* Bits 0-5,6-7,9-10,13-14 */
      case OP_FMR5:  or_mask = 0xFB06;	break;	/* Bits 0-4,6-7,     13-14 */
      default:				return 0;
   }

   dump (0xFFE0048E);			/* mffs fp31	     */
   dump (0xDBE00000 | (RESRC_PTR << 16) | FPSCR_SCR_OFFSET);
					/* stfd	fp31,FPSCR_SCR_OFFSET(r13) */
   dump (0x83E00000 | (RESRC_PTR << 16) | (FPSCR_SCR_OFFSET+4));
					/* l	r31,(FPSCR_SCR_OFFSET+4)(r13)*/

   fpr_ext     = op->op.renameable[FRB & (~OPERAND_BIT)];
   fpr_ext_off = FP0_FPSCR_OFFSET + SIMFPSCR_SIZE * (fpr_ext - FP0);
   dump (0x83C00000 | (RESRC_PTR << 16) | (fpr_ext_off+4));	
					/* l    r30,FPR_FPSCR_EXT(r13) */

   /* Want to OR-in bits 0-4, 6-8, and 13-14.  Create mask for this.  */
   dump (0x3FA00000 | or_mask);		/* cau  r29,or_mask	      */
   dump (0x7FBDF038);			/* and  r29,r29,r30	      */
   dump (0x7FFFEB78);			/* or   r31,r31,r29	      */

   /* We want to set bits 15-19.  Do this directly.		      */
					/* rlimi r31,r30,0,15,19      */

   dump (0x93E00000 | (RESRC_PTR << 16) | (FPSCR_SCR_OFFSET+4));
					/* st	r31,(FPSCR_SCR_OFFSET+4)(r13)*/

   dump (0xCBE00000 | (RESRC_PTR << 16) | FPSCR_SCR_OFFSET);
					/* lfd	fp31,FPSCR_SCR_OFFSET(r13)   */

   dump (0xFDF0FD8E);			/* mtfsf 0xF8,fp31	     */

   return 1;
}

