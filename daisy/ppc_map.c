/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "resrc_map.h"
#include "resrc_offset.h"
#include "dis.h"
#include "resrc_list.h"
#include "vliw.h"
#include "dis_tbl.h"
#include "cacheconst.h"
#include "gen_resrc.h"

/* Codes used to define moves to/from CTR or Link Register or Link Reg 2 */
#define NO_LR_CTR_MOVE			0
#define FROM_LR				1
#define TO_LR				2
#define TO_LR2				3
#define FROM_CTR			4
#define TO_CTR				5

extern int	   num_gprs;
extern int	   num_fprs;
extern int	   num_ccbits;
extern RESRC_MAP   real_resrc;
extern RESRC_MAP   *resrc_map[];
extern PPC_OP_FMT  *ppc_op_fmt[MAX_PPC_FORMATS];

static int intmap;
static int fltmap;
static short xl_reg_map[NUM_MACHINE_REGS];   /* Resource used by xlated ins */
unsigned extract_operand_val (unsigned, OPERAND *);

/************************************************************************
 *									*
 *				map_to_ppc_regs				*
 *				---------------				*
 *									*
 * Find a real PPC register for each register in the instruction	*
 * that is not always kept in a PPC register, e.g. R13-R31.		*
 *									*
 ************************************************************************/

map_to_ppc_regs (opcode, ins)
OPCODE2  *opcode;
unsigned ins;
{
   intmap = 31;			/* Start with registers r31 and fp31 */
   fltmap = 31;

   init_xl_reg_map   (opcode, ins);
   map_template_regs (opcode, ins);
}

/************************************************************************
 *									*
 *				init_xl_reg_map				*
 *				---------------				*
 *									*
 ************************************************************************/

init_xl_reg_map (opcode, ins)
OPCODE2  *opcode;
unsigned ins;
{
   int		num;
   int		op_num;
   int		type;
   int		num_operands;
   unsigned	val;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;
   RESRC_MAP    *resrc;

   assert (opcode->op.ins == ins);

   curr_format  = ppc_op_fmt[opcode->b->format];
   num_operands = curr_format->num_operands;

   xl_reg_map[MQ] = NO_MAP;      /* MQ  properly initialized if its explicit */
   for (num = 0; num < num_operands; num++) {
      op_num  = curr_format->ppc_asm_map[num];
      operand = &curr_format->list[op_num];
      type    = operand->type;

      switch (type) {
	 case RT:
	 case RS:
	 case RA:
	 case RB:
	 case RZ:
	 case FRT:
	 case FRS:
	 case FRA:
	 case FRB:
	 case FRC:
	    val = opcode->op.renameable[type & (~OPERAND_BIT)];
	    xl_reg_map[val] = MAP_UNINIT;
	    break;

	 case SPR_F:
	 case SPR_T:
	    val = extract_operand_val (ins, operand);
            if (val == CTR) xl_reg_map[CTR] = MAP_UNINIT;
	    break;

	 default:
	    break;
      }
   }

   if (opcode->b->op_num == OP_MULI  &&  opcode->op.mq) {
/*    assert (resrc_map[MQ] == &real_resrc); */	   /* Now MQ = R33 and real */
      xl_reg_map[MQ] = MAP_UNINIT;
   }
}

/************************************************************************
 *									*
 *				map_template_regs			*
 *				-----------------			*
 *									*
 * Find a real PPC register for each register in the instruction	*
 * that is not always kept in a PPC register, i.e. R13-R31.		*
 *									*
 * This routine is simplified in that it only looks at registers	*
 * appearing explicitly in the opcode.  This routine should be		*
 * sufficient when translating PPC to PPC except for lm, stm, lsi,	*
 * stsi, etc. instructions.						*
 *									*
 * NOTE:  Assumes that R0 to R31 are numbered consecutively.		*
 * NOTE:  Assumes that registers mapped to real PPC registers are	*
 *	  consecutive.
 *									*
 ************************************************************************/

map_template_regs (opcode, ins)
OPCODE2  *opcode;
unsigned ins;
{
   int		num;
   int		op_num;
   int		type;
   int		num_operands;
   unsigned	val;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;
   RESRC_MAP    *resrc;

   assert (opcode->op.ins == ins);

   curr_format  = ppc_op_fmt[opcode->b->format];
   num_operands = curr_format->num_operands;

   for (num = 0; num < num_operands; num++) {
      op_num  = curr_format->ppc_asm_map[num];
      operand = &curr_format->list[op_num];
      type    = operand->type;

      switch (type) {
	 case RT:
	 case RS:
	 case RA:
	 case RB:
	 case RZ:
	    val = opcode->op.renameable[type & (~OPERAND_BIT)];
/*	    val = R0 + (int) ((ins >> operand->shift) & operand->mask);*/

	    resrc = resrc_map[val];
	    if      (operand->dest)	   xl_reg_map[val] = R0 + intmap--;
	    else if (resrc != &real_resrc) xl_reg_map[val] = R0 + intmap--;
	    else if (xl_reg_map[val] == MAP_UNINIT)
					   xl_reg_map[val] = NO_MAP;
	    break;

	 case FRT:
	 case FRS:
	 case FRA:
	 case FRB:
	 case FRC:
	    val = opcode->op.renameable[type & (~OPERAND_BIT)];
	    resrc = resrc_map[val];
	    if      (operand->dest)	   xl_reg_map[val] = FP0 + fltmap--;
	    else if (resrc != &real_resrc) xl_reg_map[val] = FP0 + fltmap--;
	    else if (xl_reg_map[val] == MAP_UNINIT)
					   xl_reg_map[val] = NO_MAP;
	    break;

	 case SPR_F:
	 case SPR_T:
	    val = extract_operand_val (ins, operand);
            if (val == CTR) xl_reg_map[CTR] = R0 + intmap--;
	    break;

	 case RTM:
	 case RSM:
	    fprintf (stderr, "Got unexpected operand for lm/stm instruction in \"map_template_regs\"\n");
	    exit (1);

	 case RTS:
	 case RSS:
	    fprintf (stderr, "Got unexpected operand for lsi/stsi instruction in \"map_template_regs\"\n");
	    break;

	 default:
	    break;
      }
   }

   /* Kludge:  Combining for "A RT,RA,RB" and "CAX RT,RA,RB" uses the
      "MULI" instruction to compute multiples of RB, if RB is loop
      invariant and RT=RA, or vice-versa with RA and RB switched.
      The Power "MULI" instruction has an undefined effect on the MQ
      register.  We assume that there is a MULI instruction in the
      VLIW for which this is not the case.  Hence we must save and
      restore MQ during our simulation.  We just put it in one of
      the PPC GPR's, e.g. r29 for the "MULI" instruction.
   */
   if (opcode->b->op_num == OP_MULI  &&  opcode->op.mq) {
/*    assert (resrc_map[MQ] == &real_resrc); */	   /* Now MQ = R33 and real */
      xl_reg_map[MQ] = R0 + intmap--;
   }
}

/************************************************************************
 *									*
 *				load_to_ppc_regs			*
 *				----------------			*
 *									*
 * Load mapped registers into real PPC registers in preparation		*
 * for operation, e.g.							*
 *									*
 *	           l  r(mapped),R_ORIG(r13)				*
 *									*
 ************************************************************************/

load_to_ppc_regs (opcode)
OPCODE2 *opcode;
{
   int		  i;
   int		  val;
   int		  map;
   int		  do_load;
   int		  rd_reg;
   int		  num_inputs = opcode->op.num_rd;
   unsigned short *rd = opcode->op.rd;
   RESRC_MAP      *resrc;

   for (i = 0; i < num_inputs; i++) {

      rd_reg = rd[i];

      if      (rd_reg >= R0   &&  rd_reg < R0  + num_gprs)   {
         if (xl_reg_map[rd_reg] != NO_MAP  &&	/* MFMQ comes from real MQ */
	     !(opcode->b->op_num == OP_MFSPR  &&  rd_reg == MQ)) {
	    val = xl_reg_map[rd_reg] - R0;
	    resrc = resrc_map[rd_reg];
	    if (resrc != &real_resrc)
	       dump (0x80000000 | (val<<21) | (RESRC_PTR<<16) | resrc->offset);
	    else dump (0x60000000 | (val << 16) | ((rd_reg - R0) << 21));
	 }
      }
      else if (rd_reg >= FP0  &&  rd_reg < FP0 + num_fprs)   {
         if (xl_reg_map[rd_reg] != NO_MAP) {
	    val = xl_reg_map[rd_reg] - FP0;
	    resrc = resrc_map[rd_reg];
	    if (resrc != &real_resrc)
	       dump (0xC8000000 | (val<<21) | (RESRC_PTR<<16) | resrc->offset);
	    else dump (0xFC000090 | (val << 21) | ((rd_reg - FP0) << 11));
	 }
      }
      else if (rd_reg >= CR0  &&  rd_reg < CR0 + num_ccbits) assert (1 == 0);
   }

   /* Move MQ to safe GPR.  See comment at end of "map_template_regs" */
   if (opcode->b->op_num == OP_MULI  &&  opcode->op.mq)
      dump (0x7C0002A6 | ((xl_reg_map[MQ] - R0) << 21));
}

/************************************************************************
 *									*
 *				xlate_non_branch			*
 *				----------------			*
 *									*
 * Generate non-branching PowerPC instruction with mapped registers	*
 * substituting for original PowerPC registers before translation.	*
 * Note that we keep the link register independently of the PowerPC	*
 * link register.							*
 *									*
 * NOTE: Despite its name, this routine also handles TRAP instructions.	*
 *									*
 ************************************************************************/

xlate_non_branch (opcode, ins)
OPCODE2  *opcode;
unsigned ins;
{
   int		num;
   int		op_num;
   int		type;
   int		num_operands;
   int		code;
   unsigned	val;
   unsigned	spr_val;
   unsigned     lr_val;
   unsigned	reg;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;

   assert (opcode->op.ins == ins);

   curr_format  = ppc_op_fmt[opcode->b->format];
   num_operands = curr_format->num_operands;

   code = NO_LR_CTR_MOVE;
   for (num = 0; num < num_operands; num++) {
      op_num  = curr_format->ppc_asm_map[num];
      operand = &curr_format->list[op_num];
      type    = operand->type;

      if      (type == SPR_F) {
	 val = extract_operand_val (ins, operand);
	 if     ((spr_val = spr_f (val)) == LR) { code = FROM_LR;   break; }
      }
      else if (type == SPR_T) {
	 val = extract_operand_val (ins, operand);
	 if     ((spr_val = spr_t (val)) == LR) { code = TO_LR;     break; }
	 else if (spr_val == LR2) {
	    /* Only time we expect this is on LR to LR2 move */
	    assert (op_num == 0);
	    assert (curr_format->list[1].type == SPR_F);
	    lr_val = spr_f ((ins >> 16) & 0x1F);
	    assert (lr_val == LR);
	    code = TO_LR2;
	    break;
	 }
      }

      switch (type) {
	 case RT:
	 case RS:
	 case RA:
	 case RB:
	 case RZ:
	    val = opcode->op.renameable[type & (~OPERAND_BIT)];
/*	    val = R0 + (int) ((ins >> operand->shift) & operand->mask);*/

	    reg = get_gpr_for_op (val, operand->dest);

	    if (type == RZ  &&  reg == R0  &&  !is_daisy_rz_op (opcode))
	       reg = put_r0_in_scratch (opcode);

	    ins &= ~(operand->mask << operand->shift);
	    ins |=  reg		   << operand->shift;

	    break;

	 case FRT:
	 case FRS:
	 case FRA:
	 case FRB:
	 case FRC:
	    val = opcode->op.renameable[type & (~OPERAND_BIT)];
/*	    val = FP0 + (int) ((ins >> operand->shift) & operand->mask);*/

	    reg = get_fpr_for_op (val, operand->dest);

	    ins &= ~(operand->mask << operand->shift);
	    ins |=  reg		   << operand->shift;

	    break;

	 case RTM:
	 case RTS:
	 case RSM:
	 case RSS:
	    fprintf (stderr, "Instructions lm, lsi, stm, and stsi not yet supported\n");
	    exit (1);

	 default:
	    break;
      }
   }

   if (!handle_lr_move (opcode, code)) dump (ins);
}

/************************************************************************
 *									*
 *				handle_lr_move				*
 *				--------------				*
 *									*
 * Specially handle mtspr/mfspr moves to and from LR and LR2.		*
 * Aside from R13-R63, FP24-FP63, CR0-CR15, and the shadow regs,	*
 * these are the only registers which do not use the actual PowerPC	*
 * registers.  (See the file "ppc_map_tbl.c" for the official list of	*
 * which registers do not use their real PowerPC counterpart in		*
 * simulation.								*
 *									*
 * NOTE:  This routine is very PowerPC specific.			*
 *									*
 ************************************************************************/

static int handle_lr_move (opcode, code)
OPCODE2 *opcode;
int     code;
{
  unsigned	srcreg;
  unsigned	destreg;
  unsigned	tempreg;
  unsigned	ctr_reg;
  RESRC_MAP    *resrc;

  switch (code) {
    case TO_LR2:
      dump (0x83E00000 | (RESRC_PTR<<16) | LR_OFFSET);   /* l   r31,LR(r13)  */
      dump (0x93E00000 | (RESRC_PTR<<16) | LR2_OFFSET);  /* st  r31,LR2(r13) */
      return TRUE;
   
    case TO_LR:
      srcreg = opcode->op.renameable[RS & (~OPERAND_BIT)];

      resrc = resrc_map[srcreg];
      tempreg = get_gpr_for_op (srcreg, FALSE);

      dump (0x90000000			|	  /* st  r31,LR(r13)      */
	    (tempreg << 21)		|
	    (RESRC_PTR         << 16)	|
	    resrc_map[LR]->offset);

      return TRUE;
   
    case FROM_LR:
      destreg = opcode->op.renameable[RT & (~OPERAND_BIT)];
      resrc = resrc_map[destreg];
      assert (xl_reg_map[destreg] != NO_MAP);
      tempreg = xl_reg_map[destreg] - R0;

      dump (0x80000000			|	/* l   r31,LR(r13)      */
	    (tempreg << 21)		|
	    (RESRC_PTR         << 16)	|
	     resrc_map[LR]->offset);

      return TRUE;
   
   default:
      return FALSE;
  }
}

/************************************************************************
 *									*
 *				get_gpr_for_op				*
 *				--------------				*
 *									*
 ************************************************************************/

int get_gpr_for_op (in_reg, dest)
int   in_reg;
int   dest;				/* Boolean */
{
   if (xl_reg_map[in_reg] == NO_MAP)          return in_reg		- R0;
   else if (dest)		              return xl_reg_map[in_reg]	- R0;
   else if (resrc_map[in_reg] != &real_resrc) return xl_reg_map[in_reg]	- R0;
   else					      return in_reg		- R0;
}

/************************************************************************
 *									*
 *				get_fpr_for_op				*
 *				--------------				*
 *									*
 ************************************************************************/

int get_fpr_for_op (in_reg, dest)
int   in_reg;
int   dest;				/* Boolean */
{
   if (xl_reg_map[in_reg] == NO_MAP)          return in_reg		- FP0;
   else if (dest)		              return xl_reg_map[in_reg]	- FP0;
   else if (resrc_map[in_reg] != &real_resrc) return xl_reg_map[in_reg]	- FP0;
   else					      return in_reg		- FP0;
}

/************************************************************************
 *									*
 *			     store_from_ppc_regs			*
 *			     -------------------			*
 *									*
 * Store into mapped registers from real PPC registers following	*
 * operation.								*
 *									*
 * RETURNS:  GPR, FPR written, if this occurs, -1 o.w.			*
 *									*
 ************************************************************************/

int store_from_ppc_regs (opcode, ccr_shad_vec, mq_write)
OPCODE2  *opcode;
unsigned *ccr_shad_vec;
int	 *mq_write;			/* Output */
{
   int		  i;
   int		  rval;
   int		  offset;
   int		  num_outputs = opcode->op.num_wr;
   int		  condreg_bitcnt = 0;
   unsigned	  st_reg;
   unsigned	  wr_reg;
   unsigned	  tmpreg;
   RESULT	  *wr = opcode->op.wr;

   rval = -1;				/* Did not have GPR destination */
   *mq_write = FALSE;
   for (i = 0; i < num_outputs; i++) {
      wr_reg = wr[i].reg;

      if (wr_reg >= R0  &&  wr_reg < R0 + num_gprs) {

	 if (wr_reg == MQ) {		/* mfspr  r(mapped),MQ */
	    *mq_write = TRUE;
	    st_reg = R0 + intmap--;
	    dump (0x7C0002A6 | ((st_reg - R0) << 21));
	 }
         else {
	    assert (xl_reg_map[wr_reg] != NO_MAP);
	    st_reg = xl_reg_map[wr_reg];
	 }

	 if (!(rval >= 0 && wr_reg == MQ)) rval = wr_reg;
	 if (opcode->op.verif) offset = GPR_V_OFFSET;
	 else		       offset = SHDR0_OFFSET + SIMGPR_SIZE*(wr_reg-R0);

	 /* Store the value into the mapped register */
	 /* st  r(mapped),R_ORIG(r13) */
	 dump (0x90000000 | ((st_reg-R0) << 21) | (RESRC_PTR << 16) | offset);
      }
      else if (wr_reg >= FP0  &&  wr_reg < FP0 + num_fprs) {
	 assert (xl_reg_map[wr_reg] != NO_MAP);

	 rval = wr_reg;
	 if (opcode->op.verif) offset = FPR_V_OFFSET;
	 else		       offset = SHDFP0_OFFSET+SIMFPR_SIZE*(wr_reg-FP0);

	 /* Store the value into the mapped register */
	 /* st  r(mapped),FP_ORIG(r13) */
	 dump (0xD8000000 | ((xl_reg_map[wr_reg] - FP0) << 21) |
	       (RESRC_PTR << 16) | offset);
      }
      else if (wr_reg >= CR0  &&  wr_reg < CR0 + num_ccbits) {
	 assert (wr_reg >= CR0  &&  wr_reg <= CR7);
	 condreg_bitcnt++;

	 if ((wr_reg & 3) == 0) {
	    tmpreg = R0 + intmap--;

	    dump (0x7C000026 | (tmpreg<<21));	     /* MFCR  tmpreg */
	    if (wr_reg == CR4) {		/* rlinm tmpreg,tmpreg,4,0,3 */
	       dump (0x54002006 | (tmpreg<<21) | (tmpreg<<16));
	    }

	    set_bit (ccr_shad_vec, wr_reg - CR0);
	    offset = SHDCRF0_OFFSET + SIMCRF_SIZE * ((wr_reg - CR0) / 4);

	    dump (0x90000000 | (tmpreg<<21) | (RESRC_PTR<<16) | offset);
	 }
      }
   }

   /* Make sure we see full bit fields */
   assert (condreg_bitcnt == 0  ||  condreg_bitcnt == 4);

   /* Restore MQ from safe GPR.  See comment at end of "map_template_regs" */
   if (opcode->b->op_num == OP_MULI  &&  opcode->op.mq)
      dump (0x7C0003A6 | (xl_reg_map[MQ] << 21));

   return rval;
}

/************************************************************************
 *									*
 *				wr_shad_gprs				*
 *				------------				*
 *									*
 * Invoke at terminal tip of VLIW to copy values from shadow registers	*
 * of VLIW GPR's to the GPR's themselves.  This is so that the original	*
 * value is available throughout the VLIW, in accordance with parallel	*
 * execution semantics.							*
 *									*
 * Returns:  Number of instructions generated to write shadow registers	*
 *									*
 ************************************************************************/

int wr_shad_gprs (gpr_shad_vec)
unsigned *gpr_shad_vec;
{
   int	     i, j;
   int	     reg;
   int	     num_reg_groups;
   int	     cnt  = 0;
   unsigned  mask = 1;

   num_reg_groups = num_gprs / WORDBITS;

   /* Expect num_gprs to be integer multiple of WORDBITS, i.e. of 32 */
   assert (num_reg_groups * WORDBITS == num_gprs);

   /* Write any necessary shadow registers for R0-R63 (the GPRS) */
   reg = 0;
   for (i = 0; i < num_reg_groups; i++) {
      mask = 1;
      for (j = 0; j < WORDBITS; j++, mask <<= 1, reg++)
         if (gpr_shad_vec[i] & mask) cnt = wr_shad_gpr (reg, cnt);
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_gpr				*
 *				-----------				*
 *									*
 * Returns:  Number of instructions generated to write shadow register	*
 *									*
 ************************************************************************/

static int wr_shad_gpr (gpr_num, cnt)
int gpr_num;
int cnt;
{
   int	     offset;
   unsigned  ld_reg;
   RESRC_MAP *nonshad_resrc;

   nonshad_resrc = resrc_map[R0 + gpr_num];
   if (nonshad_resrc != &real_resrc) ld_reg = 31;
   else				     ld_reg = gpr_num;

   /* Load the value from the shadow register */
   cnt++;
   offset = SHDR0_OFFSET + SIMGPR_SIZE * gpr_num;
   dump    (0x80000000 | (ld_reg << 21) | (RESRC_PTR << 16) | offset);
						/*    l  r31,SHAD(r13) */

   /* Store the value into the regular integer reg if need be */
   if (nonshad_resrc != &real_resrc) {
      cnt++;
      dump (0x90000000 | (ld_reg << 21) | (RESRC_PTR << 16) |
	    nonshad_resrc->offset);		/*    st  r31,SHAD(r13) */
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_fprs				*
 *				------------				*
 *									*
 * Invoke at terminal tip of VLIW to copy values from shadow registers	*
 * of VLIW FPR's to the FPR's themselves.  This is so that the original	*
 * value is available throughout the VLIW, in accordance with parallel	*
 * execution semantics.							*
 *									*
 * Returns:  Number of instructions generated to write shadow registers	*
 *									*
 ************************************************************************/

int wr_shad_fprs (fpr_shad_vec)
unsigned *fpr_shad_vec;
{
   int	     i, j;
   int	     reg;
   int	     num_reg_groups;
   int	     cnt  = 0;
   unsigned  mask = 1;

   num_reg_groups = num_fprs / WORDBITS;

   /* Expect num_fprs to be integer multiple of WORDBITS, i.e. of 32 */
   assert (num_reg_groups * WORDBITS == num_fprs);

   /* Write any necessary shadow registers for FP0-FP63 (the FPRS) */
   reg = 0;
   for (i = 0; i < num_reg_groups; i++) {
      mask = 1;
      for (j = 0; j < WORDBITS; j++, mask <<= 1, reg++)
         if (fpr_shad_vec[i] & mask) cnt = wr_shad_fpr (reg, cnt);
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_fpr				*
 *				-----------				*
 *									*
 * Returns:  Number of instructions generated to write shadow register	*
 *									*
 ************************************************************************/

static int wr_shad_fpr (fpr_num, cnt)
int fpr_num;
int cnt;
{
   int	     offset;
   unsigned  ld_reg;
   RESRC_MAP *nonshad_resrc;

   nonshad_resrc = resrc_map[   FP0 + fpr_num];
   if (nonshad_resrc != &real_resrc) ld_reg = 31;
   else				     ld_reg = fpr_num;

   /* Load the value from the shadow register */
   cnt++;
   offset = SHDFP0_OFFSET + SIMFPR_SIZE * fpr_num;
   dump    (0xC8000000 | (ld_reg << 21) | (RESRC_PTR << 16) | offset);
						/*    lfd  r31,SHAD(r13) */

   /* Store the value into the regular integer reg if need be */
   if (nonshad_resrc != &real_resrc) {
      cnt++;
      dump (0xD8000000 | (ld_reg << 21) | (RESRC_PTR << 16) |
	    nonshad_resrc->offset);		/*    stfd r31,SHAD(r13) */
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_ccrs				*
 *				------------				*
 *									*
 * Invoke at terminal tip of VLIW to copy values from shadow registers	*
 * of VLIW CCR's to the CCR's themselves.  This is so that the original	*
 * value is available throughout the VLIW, in accordance with parallel	*
 * execution semantics.							*
 *									*
 * Returns:  Number of instructions generated to write shadow registers	*
 *									*
 * NOTE:     This could be made more efficient by keeping one bit per	*
 *	     CR Field instead of one bit per CR bit.  However, this	*
 *	     keeps things simpler and easier to maintain for now.	*
 *									*
 ************************************************************************/

int wr_shad_ccrs (ccr_shad_vec)
unsigned *ccr_shad_vec;
{
   int	     i, j;
   int	     reg;
   int	     num_reg_groups;
   int	     cnt  = 0;
   unsigned  mask = 1;

   num_reg_groups = num_ccbits / WORDBITS;

   /* Expect num_fprs to be integer multiple of WORDBITS, i.e. of 32 */
   assert (num_reg_groups * WORDBITS == num_ccbits);

   /* Write any necessary shadow registers for CR0-CR63 (the CCRBITS) */
   reg = 0;
   for (i = 0; i < num_reg_groups; i++) {
      mask = 1;
      for (j = 0; j < WORDBITS; j++, mask <<= 1, reg++)
         if (ccr_shad_vec[i] & mask) cnt = wr_shad_ccr (reg, cnt);
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_ccr				*
 *				-----------				*
 *									*
 * Returns:  Number of instructions generated to write shadow register	*
 *									*
 ************************************************************************/

static int wr_shad_ccr (ccr_num, cnt)
int ccr_num;
int cnt;
{
   int	     offset;
   RESRC_MAP *nonshad_resrc;

   nonshad_resrc = resrc_map[   CR0 + ccr_num];
   assert (nonshad_resrc != &real_resrc);

   /* Load the value from the shadow register */
   offset = SHDCRF0_OFFSET + SIMCRF_SIZE * (ccr_num / 4);
   cnt++;					/*    l  r31,SHAD(r13) */
   dump (0x80000000 | (31 << 21) | (RESRC_PTR << 16) | offset);

   /* Store the value into the regular cond reg */
   cnt++;					/*    st  r31,SHAD(r13) */
   dump (0x90000000 | (31 << 21) | (RESRC_PTR << 16) | nonshad_resrc->offset);

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_xers				*
 *				------------				*
 *									*
 * Invoke at terminal tip of VLIW to copy values from shadow registers	*
 * of VLIW GPR XER extenders to the GPR XER extenders themselves.  This	*
 * is so that the original value is available throughout the VLIW, in	*
 * accordance with parallel execution semantics.			*
 *									*
 * Returns:  Number of instructions generated to write shadow registers	*
 *									*
 ************************************************************************/

int wr_shad_xers (xer_shad_vec)
unsigned *xer_shad_vec;
{
   int	     i, j;
   int	     reg;
   int	     num_reg_groups;
   int	     cnt  = 0;
   unsigned  mask = 1;

   num_reg_groups = num_gprs / WORDBITS;

   /* Expect num_gprs to be integer multiple of WORDBITS, i.e. of 32 */
   assert (num_reg_groups * WORDBITS == num_gprs);

   /* Write any necessary shadow registers for R0-R63 (the GPRS) */
   reg = 0;
   for (i = 0; i < num_reg_groups; i++) {
      mask = 1;
      for (j = 0; j < WORDBITS; j++, mask <<= 1, reg++)
         if (xer_shad_vec[i] & mask) cnt = wr_shad_xer (reg, cnt);
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_xer				*
 *				-----------				*
 *									*
 * Returns:  Number of instructions generated to write shadow register	*
 *									*
 ************************************************************************/

static int wr_shad_xer (gpr_num, cnt)
int gpr_num;
int cnt;
{
   int	     offset;
   int	     sh_offset;

   /* Load the value from the shadow ext bits */
   sh_offset = SHDR0_XER_OFFSET + SIMXER_SIZE * gpr_num;
   cnt++;					/*    l  r31,SHAD(r13) */
   dump (0x80000000 | (31 << 21) | (RESRC_PTR << 16) | sh_offset);

   /* Store the value into the regular XER extender bits */
   offset = R0_XER_OFFSET + SIMXER_SIZE * gpr_num;
   cnt++;					/*    st  r31,SHAD(r13) */
   dump (0x90000000 | (31 << 21) | (RESRC_PTR << 16) | offset);

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_fpscrs				*
 *				--------------				*
 *									*
 * Invoke at terminal tip of VLIW to copy values from shadow registers	*
 * of VLIW FPR FPSCR extenders to the FPR FPSCR extenders themselves.	*
 * This is so that the original value is available throughout the VLIW,	*
 * in accordance with parallel execution semantics.			*
 *									*
 * Returns:  Number of instructions generated to write shadow registers	*
 *									*
 ************************************************************************/

int wr_shad_fpscrs (fpscr_shad_vec)
unsigned *fpscr_shad_vec;
{
   int	     i, j;
   int	     reg;
   int	     num_reg_groups;
   int	     cnt  = 0;
   unsigned  mask = 1;

   num_reg_groups = num_fprs / WORDBITS;

   /* Expect num_fprs to be integer multiple of WORDBITS, i.e. of 32 */
   assert (num_reg_groups * WORDBITS == num_fprs);

   /* Write any necessary shadow registers for FP0-FP63 (the FPRS) */
   reg = 0;
   for (i = 0; i < num_reg_groups; i++) {
      mask = 1;
      for (j = 0; j < WORDBITS; j++, mask <<= 1, reg++)
         if (fpscr_shad_vec[i] & mask) cnt = wr_shad_fpscr (reg, cnt);
   }

   return cnt;
}

/************************************************************************
 *									*
 *				wr_shad_fpscr				*
 *				-------------				*
 *									*
 * Returns:  Number of instructions generated to write shadow register	*
 *									*
 ************************************************************************/

static int wr_shad_fpscr (fpr_num, cnt)
int fpr_num;
int cnt;
{
   int	     offset;
   int	     sh_offset;

   assert (SIMFPSCR_SIZE == 8);

   /* Load the value from the shadow ext bits 32-63 */
   sh_offset = SHDFP0_FPSCR_OFFSET + SIMFPSCR_SIZE * fpr_num + 4;
   cnt++;					/*    l  r31,SHAD(r13) */
   dump (0x80000000 | (31 << 21) | (RESRC_PTR << 16) | sh_offset);

   /* Store the value into the regular XER extender bits 32-63 */
   offset = FP0_FPSCR_OFFSET + SIMFPSCR_SIZE * fpr_num + 4;
   cnt++;					/*    st  r31,SHAD(r13) */
   dump (0x90000000 | (31 << 21) | (RESRC_PTR << 16) | offset);

   return cnt;
}

/************************************************************************
 *									*
 *				put_r0_in_scratch			*
 *				-----------------			*
 *									*
 * This routine copies R0 to a scratch register, and is used when the	*
 * RA (RZ in our nomenclature) field of "op" is 0, and DAISY intends	*
 * this to mean [r0], not literal 0.					*
 *									*
 ************************************************************************/

static int put_r0_in_scratch (op)
OPCODE2 *op;
{
   int scratch = R0 + intmap--;

   assert (is_ppc_rz_op (op));

   dump (0x7C000378 | (scratch << 16));		/* OR  scratch,r0,r0 */

   return scratch;
}

/************************************************************************
 *									*
 *				ins_cache_ref				*
 *				-------------				*
 *									*
 * For Instruction fetches, generate call to the data cache simulator.	*
 *									*
 ************************************************************************/

ins_cache_ref (address, find_ins_upper, find_ins_lower)
unsigned address;
unsigned find_ins_upper;
unsigned find_ins_lower;
{
   dump (0x3FE00000 | find_ins_upper);	     /* liu   r31,find_instr_UPPER  */
   dump (0x63FF0000 | find_ins_lower);	     /* oril  r31,r31,      _LOWER  */
   dump (0x7FE803A6);			     /* mtlr  r31		    */
   dump (0x3FE00000 | address >> 16);	     /* liu   r31,    address_UPPER */
   dump (0x63FF0000 | address &  0xFFFF);    /* oril  r31,r31,address_LOWER */
   dump (0x4E800021);			     /* bcrl			    */
}

/************************************************************************
 *									*
 *				data_cache_ref				*
 *				--------------				*
 *									*
 * For LOADS and STORES, generate a call to the data cache simulator.	*
 *									*
 ************************************************************************/

data_cache_ref (opcode, find_data_upper, find_data_lower)
OPCODE2  *opcode;
unsigned find_data_upper;
unsigned find_data_lower;
{
   int		num;
   int		op_num;
   int		type;
   int		num_operands;
   int		acctype;
   int		offset;
   int	        daisy_rz;
   unsigned	ins = opcode->op.ins;
   unsigned	val;
   unsigned	rz_reg;
   unsigned	rb_reg;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;

   if	   (is_load_op  (opcode)) acctype = CLOAD;
   else if (is_store_op (opcode)) acctype = CSTORE;
   else				  return;

   /* Below, we assume that the RZ value always means [RZ], never 0 */
   assert (! (daisy_rz = is_daisy_rz_op (opcode)));

   curr_format  = ppc_op_fmt[opcode->b->format];
   num_operands = curr_format->num_operands;

   rz_reg = NUM_PPC_GPRS+1;		/* Set to illegal values */
   rb_reg = NUM_PPC_GPRS+1;
   for (num = 0; num < num_operands; num++) {
      op_num  = curr_format->ppc_asm_map[num];
      operand = &curr_format->list[op_num];
      type    = operand->type;

      switch (type) {
	 case RZ:
	    val	   = opcode->op.renameable[type & (~OPERAND_BIT)];
	    rz_reg = get_gpr_for_op (val, operand->dest);
	    break;

	 case RB:
	    val = opcode->op.renameable[type & (~OPERAND_BIT)];
	    rb_reg = get_gpr_for_op (val, operand->dest);
	    break;

	 default:
	    break;
      }
   }

   /* All LOADS and STORES use RA|0 (i.e. our RZ) */ 
   assert (rz_reg >= 0);

   dump (0x3FE00000 | find_data_upper);		/* liu   r31,find_data_UPPER */
   dump (0x63FF0000 | find_data_lower);		/* oril  r31,r31,     _LOWER */
   dump (0x7FE803A6);				/* mtlr  r31		     */

   offset = ins & 0xFFFF;
   if (rb_reg < NUM_PPC_GPRS)
      if (rz_reg == 0  &&  daisy_rz)
	   ins = 0x3BE00000 | (rb_reg << 16);	      /* "cal" with RB as RZ */
      else ins = 0x7FE00214 | (rz_reg << 16) | (rb_reg << 11);      /* "cax" */
   else    ins = 0x3BE00000 | (rz_reg << 16) | offset;		    /* "cal" */

   dump (ins);					/* cal   r31,MEM_ADDRESS */
   dump (0x3BC00000 | acctype);			/* lil   r30,ACCESS_TYPE */
   dump (0x4E800021);				/* bcrl			 */
}

