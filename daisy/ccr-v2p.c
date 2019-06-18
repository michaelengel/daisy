/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*======================================================================*
 *									*
 * Routines here are to handle mappings opcodes explicitly reading	*
 * or writing any portion of the condition code register.		*
 *									*
 =======================================================================*/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "dis_tbl.h"
#include "resrc_map.h"
#include "resrc_list.h"
#include "resrc_offset.h"
#include "gen_resrc.h"

extern RESRC_MAP  real_resrc;
extern RESRC_MAP  *resrc_map[];
extern PPC_OP_FMT *ppc_op_fmt[MAX_PPC_FORMATS];

/************************************************************************
 *									*
 *				xlate_expl_ccr_op			*
 *				-----------------			*
 *									*
 * Returns:  TRUE  if "op" explicitly uses any part of the condition	*
 *	           code register and we generate simulation code here.	*
 *	     FALSE otherwise.						*
 *									*
 ************************************************************************/

int xlate_expl_ccr_op (op, ccr_shad_vec, gpr_wr)
OPCODE2  *op;
unsigned *ccr_shad_vec;		/* Output whenever CR bit/field is set */
int	 *gpr_wr;		/* Output on mfcr, o.w. undefined */
{
   /* If "op" does not have any explicit CCR operands, we don't handle it */
   if (!explicit_ccr_use (op)) return FALSE;

   switch (op->b->op_num) {
						/* 1-bit operands */
      case OP_CREQV:
	 cr_op_v2p (op, 0x7FFFF238, ccr_shad_vec);
	 break;

      case OP_CRAND:
	 cr_op_v2p (op, 0x7FFFF038, ccr_shad_vec);
	 break;

      case OP_CRXOR:
	 cr_op_v2p (op, 0x7FFFF278, ccr_shad_vec);
	 break;

      case OP_CROR:
	 cr_op_v2p (op, 0x7FFFF378, ccr_shad_vec);
	 break;

      case OP_CRANDC:
	 cr_op_v2p (op, 0x7FFFF078, ccr_shad_vec);
	 break;

      case OP_CRNAND:
	 cr_op_v2p (op, 0x7FFFF3B8, ccr_shad_vec);
	 break;

      case OP_CRORC:
	 cr_op_v2p (op, 0x7FFFF338, ccr_shad_vec);
	 break;

      case OP_CRNOR:
	 cr_op_v2p (op, 0x7FFFF0F8, ccr_shad_vec);
	 break;

						/* 4-bit field operands */
      case OP_MCRF:
	 mcrf_v2p (op, ccr_shad_vec);
	 break;

      case OP_MCRXR:
	 mcrxr_v2p (op, ccr_shad_vec);
	 break;

      case OP_MCRFS:
	 mcrfs_v2p (op, ccr_shad_vec);
	 break;

      case OP_CMPI:
      case OP_CMPLI:
	 cmp_v2p (op, TRUE, ccr_shad_vec);
	 break;

      case OP_CMP:
      case OP_CMPL:
	 cmp_v2p (op, FALSE, ccr_shad_vec);
	 break;

      case OP_FCMPU:
      case OP_FCMPO:
	 fcmp_v2p (op, ccr_shad_vec);
	 break;

						/* Whole CCR operand */
      case OP_MFCR:
	 *gpr_wr = mfcr_v2p (op);
	 break;

      case OP_MTCRF:
	 mtcrf_v2p (op, ccr_shad_vec);
	 break;

      case OP_MTCRF2:
	 mtcrf2_v2p (op, ccr_shad_vec);
	 break;

      default:
	 fprintf (stderr, "Unexpected opcode (%d) in \"xlate_expl_ccr_op\"\n",
		  op->b->op_num);
	 exit (1);
   }

   return TRUE;
}

/************************************************************************
 *									*
 *				cr_op_v2p				*
 *				---------				*
 *									*
 * Handle the VLIW operations,						*
 *									*
 *	CRNOR, CRANDC, CRXOR, CRNAND, CRAND, CREQV, CRORC, CROR		*
 *									*
 * which have the form (for example),					*
 *									*
 *	CRAND	9,24,18
 *									*
 * meaning bits 24 and 18 of the Condition Register should be AND'ed	*
 * and saved in bit 9 of the Condition Register.  This operation is	*
 * mapped to								*
 *									*
 *	l	r31,CRF6_OFFSET(r13)	# CR6 = 24/4			*
 *	l	r30,CRF4_OFFSET(r13)	# CR4 = 18/4			*
 *	rlinm	r30,r30,2,0,31		# Align bit 18 with bit 24	*
 *					# 24%4 = 0, 18%4 = 2		*
 *					# ==> Shift 24 left by 2	*
 *					#				*
 *					# %4:       0  1  2  3		*
 *					#	    ----------		*
 *					# Bit Posn: 3  2  1  0		*
 *									*
 *	and	r31,r31,r30		# Do the CRand operation	*
 *	l	r30,CRF2_OFFSET(r13)	# CR2 = 9/4			*
 *	rlimi	r30,r31,31,1,1		# Align bit 18 with bit 9	*
 *					# 31 = (9%4 - 18%4) % 32	*
 *	st	r30,CRF2_OFFSET(r13)	#				*
 *									*
 * NOTE 1:  CC? is stored in the upper 4 bits (corresponding to cr0 on	*
 *	    PPC).  Thus these upper 4 bits must be aligned.		*
 *									*
 ************************************************************************/

cr_op_v2p (op, gpr_ins, ccr_shad_vec)
OPCODE2  *op;
unsigned gpr_ins;	/* If orig ins is "crand  BT,BA,BB",   */
			/*    gpr_ins  is "  and  r31,r31,r30" */
unsigned *ccr_shad_vec;
{
   int	    crf_set_before;
   unsigned ins = op->op.ins;
   unsigned shift;
   unsigned bt;
   unsigned ba;
   unsigned bb;
   unsigned bt_offset;
   unsigned ba_offset;
   unsigned bb_offset;
   unsigned bt_posn;
   unsigned ba_posn;
   unsigned bb_posn;

   bt = op->op.renameable[BT & (~OPERAND_BIT)] - CR0;
   ba = op->op.renameable[BA & (~OPERAND_BIT)] - CR0;
   bb = op->op.renameable[BB & (~OPERAND_BIT)] - CR0;

   crf_set_before = is_range_set (ccr_shad_vec, bt & 0xFFFFFFFC, bt | 3);
   set_bit (ccr_shad_vec, bt);

   ba_offset = CRF0_OFFSET + SIMCRF_SIZE * (ba >> 2);
   bb_offset = CRF0_OFFSET + SIMCRF_SIZE * (bb >> 2);
   dump (0x83E00000 |(RESRC_PTR<<16)| ba_offset);/* l r31,CCR-BA_OFFSET(r13) */
   dump (0x83C00000 |(RESRC_PTR<<16)| bb_offset);/* l r30,CCR-BB_OFFSET(r13) */

   ba_posn = ba & 3;
   bb_posn = bb & 3;
   if	   (ba_posn > bb_posn) shift = 32 - (ba_posn - bb_posn);
   else if (bb_posn > ba_posn) shift =       bb_posn - ba_posn;
   else			       shift = 0;

   if (shift != 0)
      dump (0x57DE003E | shift << 11);		/* rlinm r30,r30,shift,0,31  */

   dump (gpr_ins);				/* and   r31,r31,r30 (e.g.)  */

   if (crf_set_before) bt_offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bt >> 2);
   else		       bt_offset =    CRF0_OFFSET + SIMCRF_SIZE * (bt >> 2);

   dump (0x83C00000 |(RESRC_PTR<<16)| bt_offset);/* l r30,CCR-BT_OFFSET(r13) */

   bt_posn = bt & 3;
   if	   (bt_posn > ba_posn) shift = 32 - (bt_posn - ba_posn);
   else if (ba_posn > bt_posn) shift =       ba_posn - bt_posn;
   else			       shift = 0;

   bt_offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bt >> 2);
				      /* rlimi r30,r31,shift,bt_posn,bt_posn */
   dump (0x53FE0000 | (shift << 11) | (bt_posn << 6) | (bt_posn << 1));
   dump (0x93C00000 |(RESRC_PTR<<16)| bt_offset);/* st r30,CCR-BT_OFFSET(r13)*/
}

/************************************************************************
 *									*
 *				mcrf_v2p				*
 *				--------				*
 *									*
 * Handle the MCRF instruction, e.g. "MCRF  CR3,CR6".			*
 *									*
 *	l	r31,CRF3_OFFSET(r13)					*
 *	st	r31,CRF6_OFFSET(r13)					*
 *									*
 ************************************************************************/

mcrf_v2p (op, ccr_shad_vec)
OPCODE2  *op;
unsigned *ccr_shad_vec;
{
   unsigned ins = op->op.ins;
   unsigned bf;
   unsigned bfa;
   unsigned bf_offset;
   unsigned bfa_offset;
   unsigned bf_posn;
   unsigned bfa_posn;

   bf  = op->op.renameable[BF  & (~OPERAND_BIT)] - CR0;
   bfa = op->op.renameable[BFA & (~OPERAND_BIT)] - CR0;

   set_bit (ccr_shad_vec, bf);

   bf_offset  = SHDCRF0_OFFSET + SIMCRF_SIZE * (bf  >> 2);
   bfa_offset =    CRF0_OFFSET + SIMCRF_SIZE * (bfa >> 2);
   dump (0x83E00000 |(RESRC_PTR<<16)| bfa_offset); /*  l r31,BFA_OFFSET(r13) */
   dump (0x93E00000 |(RESRC_PTR<<16)| bf_offset);  /* st r31,BF_OFFSET(r13)  */
}

/************************************************************************
 *									*
 *				mcrxr_v2p				*
 *				---------				*
 *									*
 * Handle the MCRXR instruction, e.g. "MCRXR  CR4".			*
 *									*
 *	mcrxr   cr0			# So as to Clear XER0-3		*
 *	mfspr   r31,XER							*
 *	st	r31,CRF4_OFFSET(r13)					*
 *									*
 ************************************************************************/

mcrxr_v2p (op, ccr_shad_vec)
OPCODE2  *op;
unsigned *ccr_shad_vec;
{
   unsigned ins = op->op.ins;
   unsigned bf;
   unsigned bf_offset;
   unsigned bf_posn;

   assert (resrc_map[XER0] == &real_resrc);

   bf = op->op.renameable[BF  & (~OPERAND_BIT)] - CR0;
   set_bit (ccr_shad_vec, bf);

   bf_offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bf >> 2);
   dump (0x7C000400);				  /* mcrxr cr0 */
   dump (0x7FE102A6);				  /* mfspr r31,XER */
   dump (0x93E00000 |(RESRC_PTR<<16)| bf_offset); /* st    r31,BF_OFFSET(r13)*/
}

/************************************************************************
 *									*
 *				mcrfs_v2p				*
 *				---------				*
 *									*
 * Handle the MCRFS instruction, e.g. "MCRFS  CR4,FPSCR0".		*
 *									*
 *	mcrfs   cr4,fpscr0						*
 *	mfcr	r31							*
 *	rlinm	r31,r31,4*CR4,0,3					*
 *	st	r31,CRF4_OFFSET(r13)					*
 *									*
 ************************************************************************/

mcrfs_v2p (op, ccr_shad_vec)
OPCODE2  *op;
unsigned *ccr_shad_vec;
{
   unsigned ins = op->op.ins;
   unsigned bf;
   unsigned bfb;
   unsigned bf_offset;
   unsigned shift;

   bf  = op->op.renameable[BF  & (~OPERAND_BIT)] - CR0;
   bfb = (ins >> 18) & 7;

   set_bit (ccr_shad_vec, bf);

   dump (ins);					  /* mcrfs cr4,fpscr0 */
   dump (0x7FE00026);				  /* mfcr  r31        */

   if (bfb != 0) {
      shift = bfb << 2;
      dump (0x57FF0006 | (shift << 11));	  /* rlinm r31,r31,4*CR4,0,3 */
   }

   bf_offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bf >> 2);
   dump (0x93E00000 |(RESRC_PTR<<16)| bf_offset); /* st  r31,CRF4_OFFSET(r13)*/
}

/************************************************************************
 *									*
 *				cmp_v2p					*
 *				-------					*
 *									*
 * Handle the 4 integer compare instructions (CMPI, CMPLI, CMP, CMPL),	*
 * e.g. "cmpi  cr5,r8,45"						*
 *									*
 *	CMPI  cr1,r8,45			# Use CR1 in case Rc=1		*
 *	MFCR  r31							*
 *	RLINM r31,r31,4,0,3						*
 *	ST    r31,CRF5_OFFSET(r13)					*
 *									*
 ************************************************************************/

cmp_v2p (op, immed, ccr_shad_vec)
OPCODE2  *op;
int	 immed;			/* Boolean:  TRUE if immediate compare */
unsigned *ccr_shad_vec;
{
   unsigned ins = op->op.ins;
   unsigned ra;
   unsigned rb;
   unsigned reg;
   unsigned bf;
   unsigned bf_offset;

   bf	     = op->op.renameable[BF  & (~OPERAND_BIT)] - CR0;
   bf_offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bf >> 2);

   set_bit (ccr_shad_vec, bf);

   map_to_ppc_regs  (op, ins);
   load_to_ppc_regs (op);

   ra = op->op.renameable[RA & (~OPERAND_BIT)];
   reg = get_gpr_for_op (ra, FALSE);

   ins &= 0xFC60FFFF;		/* Use CR1 for result & appropriate GPR src */
   ins |= (1 << 23);
   ins |= reg << 16;

   if (!immed) {
      rb   = op->op.renameable[RB & (~OPERAND_BIT)];
      reg  = get_gpr_for_op (rb, FALSE);
      ins &= 0xFFFF07FF;			/* Use appropriate GPR src */
      ins |= reg << 11;
   }

   dump (ins);
   dump (0x7FE00026);				  /* mfcr  r31               */
   dump (0x57FF2006);				  /* rlinm r31,r31,4,0,3     */
   dump (0x93E00000 |(RESRC_PTR<<16)| bf_offset); /* st  r31,CRF5_OFFSET(r13)*/
}

/************************************************************************
 *									*
 *				fcmp_v2p				*
 *				--------				*
 *									*
 * Handle the 2 floating point compare instructions (FCMPU and FCMPO)	*
 * e.g. "fcmpo  cr3,fp8,fp9"						*
 *									*
 *	FCMPO  cr0,fp8,fp9						*
 *	MFCR   r31							*
 *	ST     r31,CRF3_OFFSET(r13)					*
 *									*
 ************************************************************************/

fcmp_v2p (op, ccr_shad_vec)
OPCODE2  *op;
unsigned *ccr_shad_vec;
{
   unsigned ins = op->op.ins;
   unsigned fra;
   unsigned frb;
   unsigned reg;
   unsigned bf;
   unsigned bf_offset;

   bf	     = op->op.renameable[BF  & (~OPERAND_BIT)] - CR0;
   bf_offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bf >> 2);

   set_bit (ccr_shad_vec, bf);

   map_to_ppc_regs  (op, ins);
   load_to_ppc_regs (op);

   fra = op->op.renameable[FRA & (~OPERAND_BIT)];
   reg = get_fpr_for_op (fra, FALSE);

   ins &= 0xFC6007FF;		/* Use CR0 for result & appropriate FPR srcs */
   ins |= reg << 16;

   frb  = op->op.renameable[FRB & (~OPERAND_BIT)];
   reg  = get_fpr_for_op (frb, FALSE);
   ins |= reg << 11;

   dump (ins);
   dump (0x7FE00026);				  /* mfcr r31 */
   dump (0x93E00000 |(RESRC_PTR<<16)| bf_offset); /* st  r31,CRF3_OFFSET(r13)*/
}

/************************************************************************
 *									*
 *				mfcr_v2p				*
 *				--------				*
 *									*
 * Handle the MFCR instruction, e.g. "MFCR  R17"			*
 *									*
 *	l	r31,CRF0_OFFSET(r13)					*
 *									*
 *	l	r30,CRF1_OFFSET(r13)	# Do 7 pairs of instructions	*
 *	rlimi	r31,r30,28,4,7		# like this			*
 *									*
 *	l	r30,CRF2_OFFSET(r13)	# Next pair			*
 *	rlimi	r31,r30,24,8,11						*
 *	...								*
 *	st	r31,R17_OFFSET(r13)	# By "store_from_ppc_reg"	*
 *									*
 ************************************************************************/

int mfcr_v2p (op)
OPCODE2 *op;
{
   int	     i;
   unsigned  rt;
   unsigned  shift;
   unsigned  start;
   unsigned  shadreg;
   unsigned  destreg;
   unsigned  offset;
   unsigned  st_offset;
   RESRC_MAP *resrc;

   rt = op->op.renameable[RT & (~OPERAND_BIT)];
   st_offset = SHDR0_OFFSET + SIMGPR_SIZE * (rt - R0);

   destreg = 31;
						   /* l r31,CRF0_OFFSET(r13) */
   dump (0x80000000 | (destreg<<21) | (RESRC_PTR<<16) | CRF0_OFFSET);  

   for (i = 1, shift  = 28, start  = 4, offset  = CRF1_OFFSET;
	i < 8; 
	i++,   shift -= 4,  start += 4, offset += SIMCRF_SIZE) {
						   /* l r30,CRF?_OFFSET(r13) */
      dump (0x83C00000 | (RESRC_PTR<<16) | offset); 

					/* rlimi r31,r30,shift,start,start+3 */
      dump (0x53C00000 | (destreg << 16) | (shift << 11) |
	    (start << 6) | ((start+3) << 1));
   }

					/*    st r31,SHADR17_OFFSET(r13)     */
   dump (0x93E00000 | (RESRC_PTR<<16) | st_offset);

   return rt;
}

/************************************************************************
 *									*
 *				mtcrf_v2p				*
 *				---------				*
 *									*
 * Handle the MTCRF instruction, e.g. "MTCRF  255,R17"			*
 *									*
 *	l	r31,R17_OFFSET(r13)					*
 *									*
 *	rlimi	r30,r31,0,0,3		# Do 8 pairs of instructions	*
 *	st	r30,CRF0_OFFSET(r13)	# like this			*
 *									*
 *	rlimi	r30,r31,4,0,3		# Next pair			*
 *	st	r30,CRF1_OFFSET(r13)					*
 *	...								*
 *									*
 ************************************************************************/

mtcrf_v2p (op, ccr_shad_vec)
OPCODE2  *op;
unsigned *ccr_shad_vec;
{
   int	     i;
   unsigned  ins = op->op.ins;
   unsigned  fxm = (ins >> 12) & 0xFF;
   unsigned  mask;
   unsigned  rs;
   unsigned  shift;
   unsigned  srcreg;
   unsigned  offset;
   RESRC_MAP *resrc;

   rs = op->op.renameable[RS & (~OPERAND_BIT)];
   resrc = resrc_map[rs];

   if (resrc == &real_resrc) srcreg = rs;
   else {
      srcreg = 31;				   /* l  r31,R17_OFFSET(r13) */
      dump (0x83E00000 | (RESRC_PTR<<16) | resrc->offset);
   }

   for (i = 0, shift  = 0, mask   = 0x80, offset  = SHDCRF0_OFFSET;
	i < 8; 
	i++,   shift += 4, mask >>= 1,    offset += SIMCRF_SIZE) {
					
      if (mask & fxm) {
	 set_bit (ccr_shad_vec, shift);		  /* rlimi r30,r31,shift,0,3 */
	 dump (0x501E0006 | (srcreg   <<21) | (shift<<11));
	 dump (0x93C00000 | (RESRC_PTR<<16) | offset); 
      }						  /* st r30,CRF?_OFFSET(r13) */
   }
}

/************************************************************************
 *									*
 *				mtcrf2_v2p				*
 *				----------				*
 *									*
 * Handle the MTCRF2 instruction, e.g. "MTCRF2  11,5,R17"		*
 * i.e. put bits 20-23 of R17 in CR11 (i.e. field 11).			*
 *									*
 *	l	r31,R17_OFFSET(r13)					*
 *									*
 *	rlimi	r30,r31,0,0,3		#				*
 *	st	r30,CRFi_OFFSET(r13)	#				*
 *									*
 ************************************************************************/

mtcrf2_v2p (op, ccr_shad_vec)
OPCODE2  *op;
unsigned *ccr_shad_vec;
{
   unsigned  ins = op->op.ins;
   unsigned  bf2;
   unsigned  rb;
   unsigned  bfex;
   unsigned  shift;
   unsigned  srcreg;
   unsigned  offset;
   RESRC_MAP *resrc;

   bf2  = op->op.renameable[BF2  & (~OPERAND_BIT)] - CR0;
   bfex = (ins >> 18) & 7;

   set_bit (ccr_shad_vec, bf2);

   rb = op->op.renameable[RB & (~OPERAND_BIT)];
   resrc = resrc_map[rb];

   if (resrc == &real_resrc) srcreg = rb;
   else {
      srcreg = 31;				   /* l  r31,R17_OFFSET(r13) */
      dump (0x83E00000 | (RESRC_PTR<<16) | resrc->offset);
   }

   shift  = 4 * bfex;
   offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (bf2 >> 2);
						  /* rlimi r30,r31,shift,0,3 */
   dump (0x501E0006 | (srcreg   <<21) | (shift<<11));
   dump (0x93C00000 | (RESRC_PTR<<16) | offset);  /* st r30,CRF?_OFFSET(r13) */
}

/************************************************************************
 *									*
 *				explicit_ccr_use			*
 *				----------------			*
 *									*
 * Returns:  TRUE  if "op" explicitly uses any part of the condition	*
 *	           code register.					*
 *	     FALSE otherwise.						*
 *									*
 ************************************************************************/

int explicit_ccr_use (op)
OPCODE2 *op;
{
   int		num;
   int		op_num;
   int		type;
   int		num_operands;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;

   if (op->b->op_num == OP_MFCR  ||  op->b->op_num == OP_MTCRF) return TRUE;

   curr_format  = ppc_op_fmt[op->b->format];
   num_operands = curr_format->num_operands;

   for (num = 0; num < num_operands; num++) {
      op_num  = curr_format->ppc_asm_map[num];
      operand = &curr_format->list[op_num];
      type    = operand->type;

      switch (type) {
	 case BT:
	 case BA:
	 case BB:
	 case BI:	return TRUE;

	 case BF:
	 case BF2:
	 case BFA:	return TRUE;

	 default:	break;
      }
   }

   return FALSE;
}
