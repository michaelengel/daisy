/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "dis_tbl.h"
#include "vliw.h"
#include "rd_config.h"
#include "unify.h"

extern int	   no_unify;
extern int         do_combining;		/* Boolean */
extern OPCODE1     *ppc_opcode[];
extern UNIFY       *insert_unify;      /* Non-0 if unify instead of insert   */

extern int	   cluster_resrc[];    /* Bool: Reesource assoc with	     */
				       /*       clusters/whole machine	     */
static REG_RENAME  dummy_rename;
static int	   muli_latency;

int combine_induc_var         (TIP **, OPCODE2 *, unsigned *, TIP **);
int combine_add_induc_var     (TIP **, OPCODE2 *, unsigned *, TIP **);
int combine_ai_muli_induc_var (TIP **, OPCODE2 *, unsigned *, int (), TIP **, int);
int combine_shifti_induc_var  (TIP **, OPCODE2 *, unsigned *, TIP **);

static int addsub_disp (int, int);
static int mul_disp (int, int);
static unsigned get_muli_ins (unsigned, unsigned, OPCODE2 **);

/************************************************************************
 *									*
 *				combine_induc_var			*
 *				-----------------			*
 *									*
 * If we are in a loop with an induction variable, do combining.	*
 *									*
 * Returns:  0 if combining not done and caller should schedule op	*
 *	     1 if combining done, and caller need not  schedule op	*
 *	     ALSO update ptip to indicate current tip			*
 *									*
 ************************************************************************/

int combine_induc_var (ptip, op, ins_ptr, p_ins_tip)
TIP	 **ptip;
OPCODE2  *op;
unsigned *ins_ptr;
TIP	 **p_ins_tip;
{
   int rval;

   no_unify = TRUE;
   rval = combine_induc_var_main (ptip, op, ins_ptr, p_ins_tip);
   no_unify = FALSE;

   return rval;
}

int combine_induc_var_main (ptip, op, ins_ptr, p_ins_tip)
TIP	 **ptip;
OPCODE2  *op;
unsigned *ins_ptr;
TIP	 **p_ins_tip;
{
   if (!do_combining) {
      op->op.mq = FALSE;
      return 0;
   }

   switch (op->b->op_num) {
      case OP_CAL:
	 return combine_ai_muli_induc_var (ptip, op, ins_ptr, addsub_disp,
					   p_ins_tip, RZ);
      case OP_AI:
      case OP_AI_:
	 return combine_ai_muli_induc_var (ptip, op, ins_ptr, addsub_disp,
					   p_ins_tip, RA);
      case OP_CAX:
      case OP_CAX_:
      case OP_A:
      case OP_A_:
	 return combine_add_induc_var     (ptip, op, ins_ptr, p_ins_tip);

      case OP_MULI:
	 return combine_ai_muli_induc_var (ptip, op, ins_ptr, mul_disp,
					   p_ins_tip, RA);
      case OP_RLINM:
      case OP_RLINM_:
	 return combine_shifti_induc_var  (ptip, op, ins_ptr, p_ins_tip);

      default:
	 op->op.mq = FALSE;
	 return 0;
   }
}

/************************************************************************
 *									*
 *				combine_add_induc_var			*
 *				---------------------			*
 *									*
 * If we are in a loop with an induction variable updated by a CAX or	*
 * ADD instruction with no immediate operand, but where the update	*
 * value is loop invariant, do combining.  For example with		*
 * "CAX   r7,r7,r8", use combining so that the update does not become a	*
 * needless serializer, e.g.						*
 *									*
 *		CAX   r7,  r7, r8					*
 *		MULI  r8_2,r8, 2					*
 *		MULI  r8_3,r8, 3					*
 *		MULI  r8_4,r8, 4					*
 *		------------------					*
 *		CAX   r7,   r7, r8		; New VLIW		*
 *		CAX   r7',  r7, r8_2					*
 *		CAX   r7'', r7, r8_3					*
 *		CAX   r7''',r7, r8_4					*
 *									*
 * NOTE the MULI instructions must not set MQ unlike the Power version.	*
 *									*
 * Returns:  0 if combining not done and caller should schedule op	*
 *	     1 if combining done, and caller need not  schedule op	*
 *									*
 ************************************************************************/

int combine_add_induc_var (ptip, op, ins_ptr, p_ins_tip)
TIP	 **ptip;
OPCODE2  *op;
unsigned *ins_ptr;
TIP	 **p_ins_tip;
{
   int	       earliest;
   int	       orig_disp;
   int	       lim_disp;
   int	       disp;
   int	       in_range;
   int	       muli_avail;
   int	       induc_avail_save;
   int	       dest_field;
   int	       induc_var_avail_save;
   int	       induc_var_clust_save;
   int	       clust_num;
   int	       resrc_needed[NUM_VLIW_RESRC_TYPES+1];
   unsigned    ins = op->op.ins;
   unsigned    muli_ins;
   unsigned    rt;
   unsigned    ra;
   unsigned    rb;
   unsigned    si;
   unsigned    induc_var;
   unsigned    invariant;
   unsigned    invar_mult_reg;
   unsigned    *gpr_wr_addr;
   OPCODE2     *ins_cal_op;
   OPCODE2     *add_op;
   OPCODE2     *muli_op1;
   OPCODE2     *muli_op2;
   OPCODE2     *i_unify_op;
   TIP	       *tip = *ptip;
   TIP	       *ins_tip;
   REG_RENAME  *induc_var_save;

   rt = op->op.renameable[RT & (~OPERAND_BIT)];
   ra = op->op.renameable[RA & (~OPERAND_BIT)];
   rb = op->op.renameable[RB & (~OPERAND_BIT)];
/*
   rt = (ins >> 21) & 0x1F;
   ra = (ins >> 16) & 0x1F;
   rb = (ins >> 11) & 0x1F;
*/
   /* Check if this ADD/CAX is an induction variable update, and if so,
      whether RA or RB contains the induction variable.
   */
   if      (ra == rt) {induc_var = ra;   invariant = rb;}
   else if (rb == rt) {induc_var = rb;   invariant = ra;}
   else return 0;

   gpr_wr_addr = tip->gpr_writer[induc_var-R0];
   if (tip->gpr_writer[induc_var-R0] == NO_GPR_WRITERS) {
      if (tip->gpr_writer[invariant] != NO_GPR_WRITERS) return 0;
      else {
	 earliest = get_earliest_time (tip, op, FALSE);
	 tip = insert_op (op, tip, earliest, &ins_tip);

	 if (tip == ins_tip) dest_field = RT;
	 else		     dest_field = RA;

	 tip->mem_update_cnt[induc_var-R0]++;

	 *ptip = tip;
	 *p_ins_tip = ins_tip;
	 return 1;
      }
   }
   else if (tip->gpr_writer[induc_var] != ins_ptr - 1)    return 0;
   else if (tip->gpr_writer[invariant] != NO_GPR_WRITERS) return 0;

   /* We have an induction variable that we've seen before */
   else {		
      /* Insert operation at earliest possible point in group */
      muli_ins = get_muli_ins (induc_var, invariant, &muli_op1);
      induc_var_save	   = tip->gpr_rename[induc_var-R0];
      induc_var_avail_save = tip->avail[induc_var];
      induc_var_clust_save = tip->cluster[induc_var];

      tip = insert_op (muli_op1, tip, 0, &ins_tip);
      muli_op2 = ins_tip->op->op;

      if (insert_unify) i_unify_op = insert_unify->op;
      else		i_unify_op = 0;

      /* Delete muli commit.  This is not completely efficient,
	 since the non-architected register may be marked busy longer
	 than it really is.
      */
      get_needed_resrcs (tip->op->op, resrc_needed);
      clust_num = tip->op->cluster;
      decr_resrc_usage (tip, tip->vliw->resrc_cnt, resrc_needed, clust_num);

      tip->num_ops--;
      tip->op = tip->op->next;
	 
      /* IF muli result is not available before the induction variable,
	 THEN delete muli and its commit and treat CAX/ADD as normal instruc
	 ELSE keep muli result and insert CAX/ADD instruction with muli result
      */
      muli_avail = ins_tip->vliw->time + muli_latency;

      /* Want to check the time COMMIT is available, not the original val */
      assert (induc_var >= R0  &&  induc_var < R0 + NUM_PPC_GPRS);
      if (muli_avail >= tip->gpr_rename[induc_var]->time) {

         tip->gpr_rename[induc_var-R0] = induc_var_save;
         tip->avail[induc_var]       = induc_var_avail_save;
	 tip->cluster[induc_var]     = induc_var_clust_save;

	 /* Delete muli op itself */
	 if (ins_tip != tip) {

	    /* Don't delete if used by somebody else */
	    if (ins_tip->op != i_unify_op) {

	       get_needed_resrcs (ins_tip->op->op, resrc_needed);
	       clust_num = ins_tip->op->cluster;
	       decr_resrc_usage (ins_tip, ins_tip->vliw->resrc_cnt,
				 resrc_needed, clust_num);

	       ins_tip->num_ops--;
	       ins_tip->op = ins_tip->op->next;
	    }
	 }
	 return 0;
      }
      else {
	 /* Make sure scheduler does not serialize waiting for lastest RT */
	 /* Make sure scheduler uses RT and not some renamed version of it */

         patch_gpr_rename_for_pre_combine (tip, induc_var);

	 /* Insert CAX/ADD instruction */
	 tip = insert_op (op, tip, muli_avail, &ins_tip);
	 if (tip == ins_tip) dest_field = RT;
	 else		     dest_field = RA;

	 /* Patch muli to have proper immediate multiplicand */
	 si = 1 + tip->vliw->mem_update_cnt[induc_var-R0]   +
		        tip->mem_update_cnt[induc_var-R0]++ -
	      ins_tip->vliw->mem_update_cnt[induc_var-R0];
	 muli_op2->op.ins |= si;

	 /* Patch CAX/ADD to use reg w/ proper mult of invariant from muli */
	 add_op = ins_tip->op->op;
	 invar_mult_reg = muli_op2->op.wr[0].reg;
	 if      (add_op->op.rd[0] == R0 + invariant)
	    add_op->op.rd[0] = invar_mult_reg;
	 else if (add_op->op.rd[1] == R0 + invariant)
	    add_op->op.rd[1] = invar_mult_reg;
	 else assert (1 == 0);

	 if (invariant == ra)
	      add_op->op.renameable[RA & (~OPERAND_BIT)] = invar_mult_reg;
	 else add_op->op.renameable[RB & (~OPERAND_BIT)] = invar_mult_reg;

	 *ptip = tip;
	 *p_ins_tip = ins_tip;
	 return 1;
      }
   }
}

/************************************************************************
 *									*
 *				combine_ai_muli_induc_var		*
 *				-------------------------		*
 *									*
 * If we are in a loop with an induction variable updated by an ADD	*
 * or MULTIPLY type instruction with immediate operand, do combining.	*
 * For example with "CAL   r7,r7,1", use combining so that the r7	*
 * update does not become a needless serializer, e.g.			*
 *									*
 *		CAL   r7',  r7,2					*
 *		CAL   r7'', r7,3					*
 *		CAL   r7''',r7,4					*
 *		...							*
 * Returns:  0 if combining not done and caller should schedule op	*
 *	     1 if combining done, and caller need not  schedule op	*
 *									*
 ************************************************************************/

int combine_ai_muli_induc_var (ptip, op, ins_ptr, disp_func, p_ins_tip, type)
TIP	 **ptip;
OPCODE2  *op;
unsigned *ins_ptr;
int	 (*disp_func) ();
TIP	 **p_ins_tip;
int	 type;				/* E.g. RZ for "cal" and RA for "ai" */
{
   int	    earliest;
   int	    orig_disp;
   int	    lim_disp;
   int	    disp;
   int	    in_range;
   int	    dest_field;
   unsigned ins = op->op.ins;
   unsigned ra;
   unsigned rt;
   unsigned *gpr_wr_addr;
   OPCODE2  *ins_cal_op;
   TIP	    *tip = *ptip;
   TIP	    *ins_tip;

   ra = op->op.renameable[type & (~OPERAND_BIT)];
   rt = op->op.renameable[RT   & (~OPERAND_BIT)];
/*
   ra = (ins >> 16) & 0x1F;
   rt = (ins >> 21) & 0x1F;
*/
   if (ra != rt) return 0;

   /* Check if we are doing a LIL type instruction, i.e. RA/RZ = 0 */
   if (op->b->op_num == OP_CAL  &&  op->op.num_rd == 0)  return 0;

   orig_disp = (int) ((short) (ins & 0xFFFF));
   lim_disp  = disp_func (tip->vliw->mem_update_cnt[ra] + 1 +
			        tip->mem_update_cnt[ra], orig_disp);

   /* Make sure displacement allows even double-precision operand */
   in_range  = (lim_disp >= -32768)  &&  (lim_disp <= 32760);
   if (!in_range) return 0;

   gpr_wr_addr = tip->gpr_writer[ra];
   if (tip->gpr_writer[ra] == NO_GPR_WRITERS) {
      earliest = get_earliest_time (tip, op, FALSE);
      tip = insert_op (op, tip, earliest, &ins_tip);

      if (tip == ins_tip) dest_field = RT;
      else		  dest_field = RA;

      tip->mem_update_cnt[ra]++;

      *ptip = tip;
      *p_ins_tip = ins_tip;
      return 1;
   }
   else if (tip->gpr_writer[ra] != ins_ptr - 1) return 0;

   /* We have an induction variable that we've seen before */
   else {		
      /* Make sure scheduler does not serialize waiting for lastest RA */
      /* Make sure scheduler uses RA and not some renamed version of it */

      patch_gpr_rename_for_pre_combine (tip, ra);

      earliest = get_earliest_time (tip, op, FALSE);
      tip = insert_op (op, tip, earliest, &ins_tip);
      if (tip == ins_tip) dest_field = RT;
      else		  dest_field = RA;

      /* Modify the displacement to reflect the current iteration of RA */
      disp = disp_func (tip->vliw->mem_update_cnt[ra]+tip->mem_update_cnt[ra]++
			+ 1 - ins_tip->vliw->mem_update_cnt[ra], orig_disp);

      ins_cal_op = ins_tip->op->op;
      ins_cal_op->op.ins &= 0xFFFF0000;
      ins_cal_op->op.ins |= disp & 0xFFFF;

      *ptip = tip;
      *p_ins_tip = ins_tip;
      return 1;
   }
}

/************************************************************************
 *									*
 *				addsub_disp				*
 *				-----------				*
 *									*
 ************************************************************************/

static int addsub_disp (iternum, orig_disp)
int iternum;		/* Numbered 1,2,3, ... */
int orig_disp;
{
   return iternum * orig_disp;
}

/************************************************************************
 *									*
 *				addsub_disp				*
 *				-----------				*
 *									*
 ************************************************************************/

static int mul_disp (iternum, orig_disp)
int iternum;		/* Numbered 1,2,3, ... */
int orig_disp;
{
   int i;
   int rval = orig_disp;

   for (i = 1; i < iternum; i++)
      rval *= orig_disp;

   return rval;
}

/************************************************************************
 *									*
 *				combine_shifti_induc_var		*
 *				------------------------		*
 *									*
 * If we are in a loop with an induction variable updated by a logical	*
 * shift instruction with immediate operand, do combining.  For example	*
 * with "RLINM   r7,r7,1,0,30", use combining so that the r7 update	*
 * does not become a needless serializer, e.g.				*
 *									*
 *		RLINM   r7',  r7,2,0,29					*
 *		RLINM   r7'', r7,3,0,28					*
 *		RLINM   r7''',r7,4,0,27					*
 *		...							*
 * Returns:  0 if combining not done and caller should schedule op	*
 *	     1 if combining done, and caller need not  schedule op	*
 *									*
 ************************************************************************/

combine_shifti_induc_var (ptip, op, ins_ptr, p_ins_tip)
TIP	 **ptip;
OPCODE2  *op;
unsigned *ins_ptr;
TIP	 **p_ins_tip;
{
   int	    earliest;
   int	    lim_shift;
   int	    disp;
   int	    in_range;
   int	    orig_shift_size;
   int	    shift_size;
   int	    shift_left;
   unsigned ins = op->op.ins;
   unsigned inserted_ins;
   unsigned ra;
   unsigned rs;
   unsigned sh;
   unsigned mb;
   unsigned me;
   unsigned *gpr_wr_addr;
   OPCODE2  *ins_shift_op;
   TIP	    *tip = *ptip;
   TIP	    *ins_tip;

   /* Make sure we have RLINM */
   assert ((ins >> 26) == 21);

   rs = op->op.renameable[RS & (~OPERAND_BIT)];
   ra = op->op.renameable[RA & (~OPERAND_BIT)];
/*
   rs = (ins >> 21) & 0x1F;
   ra = (ins >> 16) & 0x1F;
*/
   if (ra != rs) return 0;

   sh = (ins >> 11) & 0x1F;
   mb = (ins >>  6) & 0x1F;
   me = (ins >>  1) & 0x1F;

   if      (mb ==  0  &&  me == 31 - sh) {
      shift_left      = TRUE;
      orig_shift_size = sh;
   }
   else if (me == 31  &&  mb == 32 - sh) {
      shift_left      = FALSE;
      orig_shift_size = 32 - sh;
   }
   else return 0;

   lim_shift = (tip->mem_update_cnt[ra] + tip->vliw->mem_update_cnt[ra] + 1) *
	      orig_shift_size;

   /* Make sure can fit maximum resulting shift size */
   in_range  = (lim_shift >= 0)  &&  (lim_shift <= 31);
   if (!in_range) return 0;

   gpr_wr_addr = tip->gpr_writer[ra];
   if (tip->gpr_writer[ra] == NO_GPR_WRITERS) {
      earliest = get_earliest_time (tip, op, FALSE);
      tip = insert_op (op, tip, earliest, &ins_tip);

      tip->mem_update_cnt[ra]++;

      *ptip = tip;
      *p_ins_tip = ins_tip;
      return 1;
   }
   else if (tip->gpr_writer[ra] != ins_ptr - 1) return 0;

   /* We have an induction variable that we've seen before */
   else {		
      /* Make sure scheduler does not serialize waiting for lastest RA */
      /* Make sure scheduler uses RA and not some renamed version of it */

      patch_gpr_rename_for_pre_combine (tip, ra);

      earliest = get_earliest_time (tip, op, FALSE);
      tip = insert_op (op, tip, earliest, &ins_tip);

      /* Modify the shift to reflect the current iteration of RA */
      shift_size = (tip->vliw->mem_update_cnt[ra] + tip->mem_update_cnt[ra]++
		    + 1 - ins_tip->vliw->mem_update_cnt[ra]) * orig_shift_size;

      ins_shift_op = ins_tip->op->op;
      inserted_ins = ins_shift_op->op.ins;
      if (shift_left) {
	 inserted_ins &= 0xFFFF07C1;		  /* Keep MB and record bit */
	 inserted_ins |=       shift_size  << 11;
	 inserted_ins |= (31 - shift_size) <<  1;
      }
      else {
	 inserted_ins &= 0xFFFF003F;		  /* Keep ME and record bit */
	 inserted_ins |= (32 - shift_size) << 11;
	 inserted_ins |=       shift_size  <<  6;
      }

      ins_shift_op->op.ins = inserted_ins;

      *ptip = tip;
      *p_ins_tip = ins_tip;
      return 1;
   }
}

/************************************************************************
 *									*
 *			patch_gpr_rename_for_pre_combine		*
 *			--------------------------------		*
 *									*
 ************************************************************************/

patch_gpr_rename_for_pre_combine (tip, ra)
TIP *tip;
int ra;
{
   dummy_rename.vliw_reg  = ra;
   dummy_rename.time      = --tip->avail[ra];
   dummy_rename.prev      = 0;

   tip->gpr_rename[ra-R0] = &dummy_rename;
}

/************************************************************************
 *									*
 *				get_muli_ins				*
 *				-----------				*
 *									*
 * The caller must fill in the immediate multiplier.			*
 *									*
 ************************************************************************/

static unsigned get_muli_ins (rt, ra, op2_muli)
unsigned rt;
unsigned ra;
OPCODE2  **op2_muli;
{
   unsigned muli_ins;
   OPCODE1  *op1_muli;

   muli_ins = 0x1C000000 | (rt << 21) | (ra << 16);

   op1_muli = get_opcode (muli_ins);
   *op2_muli = set_operands (op1_muli, muli_ins, op1_muli->b.format);
   (*op2_muli)->op.mq = TRUE;

   return muli_ins;
}

/************************************************************************
 *									*
 *				init_combining				*
 *				--------------				*
 *									*
 ************************************************************************/

init_combining ()
{
   muli_latency = get_max_latency1 (ppc_opcode[OP_MULI]);
}
