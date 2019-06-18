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

extern int do_combining;		/* Boolean */
extern int extra_resrc[];
extern int no_unify;

unsigned get_cal_ins (int, unsigned, unsigned, OPCODE2  **);

/************************************************************************
 *									*
 *				xlate_update_p2v			*
 *				----------------			*
 *									*
 * Translate PPC LOAD-UPDATE or STORE-UPDATE op into two instructions,	*
 * a LOAD or STORE followed by an update.				*
 *									*
 * Return values should match those of "xlate_opcode_p2v" in main.c	*
 *									*
 * NOTE:  We don't want the CAL/CAX at the current time, as it will	*
 *        confuse the way the simulator handles parallel semantics, e.g.*
 *									*
 *	 	st	r3,4(r4)					*
 *		cal	r4,4(r4)					*
 *		st	r3,8(r4)					*
 *		cal	r4,8(r4)					*
 *									*
 *	 should use the original value of r4 for the latter two ops.	*
 ************************************************************************/

int xlate_update_p2v (ptip, opcode, ins, ins_ptr, start, end)
TIP	 **ptip;
OPCODE1  *opcode;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int		is_cal;
   int		is_ld;
   int		is_flt;
   int		rval;
   int		earliest;
   int		orig_disp;
   int		lim_disp;
   int		disp;
   int		in_range;
   int		no_update;
   int		rb_rt_lux = FALSE;
   unsigned 	pre_lx_rb_avail;
   unsigned 	post_lx_rb_avail;
   unsigned	post_lx_rb_clust;
   unsigned	ra;
   unsigned	rb;
   unsigned	rt;
   unsigned	mem_ins;
   unsigned	cal_ins;
   unsigned	*gpr_wr_addr;
   OPCODE1	*op1_mem;
   OPCODE2	*op2_mem;
   OPCODE1	*op1_cal;
   OPCODE2	*op2_cal;
   OPCODE2	*ins_mem_op;
   OPCODE2	*ins_cal_op;
   OPERAND_OUT	*verop;
   REG_RENAME   vpre_lx_rb_gpr_rename;
   REG_RENAME   *pre_lx_rb_gpr_rename;
   REG_RENAME   *post_lx_rb_gpr_rename;
   TIP		*tip = *ptip;
   TIP		*ins_tip;
   TIP		*ver_tip;
   TIP		*old_tip;
   VLIW		*new_vliw;

   /* Things get confusing when combining LD/ST updates */
   no_unify = TRUE;

   if (ins_ptr == end) rval = 4;
   else		       rval = 0;

   switch (opcode->b.op_num) {
      case OP_LBZU:   mem_ins = (ins & 0x03FFFFFF) | ( 34 << 26);
		      is_cal = TRUE;	is_ld = TRUE;    is_flt = FALSE;
		      break;

      case OP_LBZUX:  mem_ins = (ins & 0xFFFFF801) | ( 87 <<  1);
		      is_cal = FALSE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;

      case OP_LHZU:   mem_ins = (ins & 0x03FFFFFF) | ( 40 << 26);
		      is_cal = TRUE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;

      case OP_LHZUX:  mem_ins = (ins & 0xFFFFF801) | (279 <<  1);
		      is_cal = FALSE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;

      case OP_LHAU:   mem_ins = (ins & 0x03FFFFFF) | ( 42 << 26);
		      is_cal = TRUE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;

      case OP_LHAUX:  mem_ins = (ins & 0xFFFFF801) | (343 <<  1);
		      is_cal = FALSE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;

      case OP_LU:     mem_ins = (ins & 0x03FFFFFF) | ( 32 << 26);
		      is_cal = TRUE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;

      case OP_LUX:    mem_ins = (ins & 0xFFFFF801) | ( 23 <<  1);
		      is_cal = FALSE;	is_ld = TRUE;	  is_flt = FALSE;
		      break;


      case OP_STBU:   mem_ins = (ins & 0x03FFFFFF) | ( 38 << 26);
		      is_cal = TRUE;	is_ld = FALSE;	  is_flt = FALSE;
		      break;

      case OP_STBUX:  mem_ins = (ins & 0xFFFFF801) | (215 <<  1);
		      is_cal = FALSE;	is_ld = FALSE;	  is_flt = FALSE;
		      break;

      case OP_STHU:   mem_ins = (ins & 0x03FFFFFF) | ( 44 << 26);
		      is_cal = TRUE;	is_ld = FALSE;	  is_flt = FALSE;
		      break;

      case OP_STHUX:  mem_ins = (ins & 0xFFFFF801) | (407 <<  1);
		      is_cal = FALSE;	is_ld = FALSE;	  is_flt = FALSE;
		      break;

      case OP_STU:    mem_ins = (ins & 0x03FFFFFF) | ( 36 << 26);
		      is_cal = TRUE;	is_ld = FALSE;	  is_flt = FALSE;
		      break;

      case OP_STUX:   mem_ins = (ins & 0xFFFFF801) | (151 <<  1);
		      is_cal = FALSE;	is_ld = FALSE;	  is_flt = FALSE;
		      break;


      case OP_LFSU:   mem_ins = (ins & 0x03FFFFFF) | ( 48 << 26);
		      is_cal = TRUE;	is_ld = TRUE;	  is_flt = TRUE;
		      break;

      case OP_LFSUX:  mem_ins = (ins & 0xFFFFF801) | (535 <<  1);
		      is_cal = FALSE;	is_ld = TRUE;	  is_flt = TRUE;
		      break;

      case OP_LFDU:   mem_ins = (ins & 0x03FFFFFF) | ( 50 << 26);
		      is_cal = TRUE;	is_ld = TRUE;	  is_flt = TRUE;
		      break;

      case OP_LFDUX:  mem_ins = (ins & 0xFFFFF801) | (599 <<  1);
		      is_cal = FALSE;	is_ld = TRUE;	  is_flt = TRUE;
		      break;


      case OP_STFSU:  mem_ins = (ins & 0x03FFFFFF) | ( 52 << 26);
		      is_cal = TRUE;	is_ld = FALSE;	  is_flt = TRUE;
		      break;

      case OP_STFSUX: mem_ins = (ins & 0xFFFFF801) | (663 <<  1);
		      is_cal = FALSE;	is_ld = FALSE;	  is_flt = TRUE;
		      break;

      case OP_STFDU:  mem_ins = (ins & 0x03FFFFFF) | ( 54 << 26);
		      is_cal = TRUE;	is_ld = FALSE;	  is_flt = TRUE;
		      break;

      case OP_STFDUX: mem_ins = (ins & 0xFFFFF801) | (727 <<  1);
		      is_cal = FALSE;	is_ld = FALSE;	  is_flt = TRUE;
		      break;

      default:	      assert (1 == 0);				  break;
   }

   ra = (ins >> 16) & 0x1F;
   rt = (ins >> 21) & 0x1F;		/* Can also be FRT */

   /* Check if we have ins like  lux r3,r6,r3.  If not careful when
      break into lx  r3,r6,r3 / cax r6,r6,r3, the "cax" will get
      the incorrect updated value from the "lx".
   */
   if (is_ld  &&  !is_cal  &&  !is_flt) {
      rb = (ins >> 11) & 0x1F;
      if (rb == rt) {
         rb_rt_lux	    = TRUE;
	 extra_resrc[ALU]   = 1;
	 pre_lx_rb_avail    = tip->avail[rb];

	 if (!tip->gpr_rename[rb-R0]) pre_lx_rb_gpr_rename = 0;
         else {
	    vpre_lx_rb_gpr_rename = *tip->gpr_rename[rb-R0];
	    pre_lx_rb_gpr_rename  = &vpre_lx_rb_gpr_rename;
	 }
      }
   }

   orig_disp = (int) ((short) (ins & 0xFFFF));
   lim_disp  = (tip->mem_update_cnt[ra] + tip->vliw->mem_update_cnt[ra] + 1) *
	       orig_disp;

   /* Make sure displacement allows even double-precision operand */
   in_range  = (lim_disp >= -32768)  &&  (lim_disp <= 32760);
   no_update = ((ra == 0)  ||  (is_ld  &&  ra == rt  &&  !is_flt));

   op1_mem = get_opcode (mem_ins);
   op2_mem = set_operands (op1_mem, mem_ins, op1_mem->b.format);

   gpr_wr_addr = tip->gpr_writer[ra];
   if (gpr_wr_addr != NO_GPR_WRITERS  &&  gpr_wr_addr != MULT_GPR_WRITERS  &&
       gpr_wr_addr == ins_ptr - 1     &&  in_range  && !no_update &&  is_cal &&
       do_combining) {

      /* Make sure scheduler does not serialize waiting for lastest RA  */
      /* Make sure scheduler uses RA and not some renamed version of it */

      patch_gpr_rename_for_pre_combine (tip, ra);

      /* Save "tip" to compare to "ver_tip" below */
      old_tip = tip;

      /******* Schedule the memory op at the earliest place *******/
      if (!sched_ld_before_st (&tip, op2_mem, ins_ptr, FALSE, &ins_tip,
			       &ver_tip, &verop)) {
	 earliest = get_earliest_time (tip, op2_mem, FALSE);
	 if (!sched_ppc_rz0_op (&tip, op2_mem, earliest, &ins_tip))
            tip = insert_op (op2_mem, tip, earliest, &ins_tip);

	 ma_store (tip, op2_mem);	/* Save for alias analysis if STORE */
      }
      else if (ver_tip  &&  ver_tip == old_tip) {

         /* Modify LD-VER displacement to reflect the proper iteration of RA */
	 disp = (1 + ver_tip->mem_update_cnt[ra]) * orig_disp;
	 verop->ins &= 0xFFFF0000;
	 verop->ins |= disp & 0xFFFF;
      }

      /* Modify the LD/ST displacement to reflect the proper iteration of RA */
      disp = (1 + tip->vliw->mem_update_cnt[ra] + tip->mem_update_cnt[ra]  -
	      ins_tip->vliw->mem_update_cnt[ra]) * orig_disp;
      ins_mem_op = ins_tip->op->op;
      ins_mem_op->op.ins &= 0xFFFF0000;
      ins_mem_op->op.ins |= disp & 0xFFFF;

      /* If overwrote source in ins like lux r3,r6,r3, then make info
         on r3 be that before the lux, so cax references correct val.
      */
      if (rb_rt_lux) {
         extra_resrc[ALU]       = 0;
	 post_lx_rb_gpr_rename  = tip->gpr_rename[rb-R0];
	 post_lx_rb_avail       = tip->avail[rb];
	 post_lx_rb_clust       = tip->cluster[rb];
	 tip->gpr_rename[rb-R0] = pre_lx_rb_gpr_rename;
	 tip->avail[rb]	        = pre_lx_rb_avail;
      }

      /******** Schedule the CAL/CAX op at the earliest place *******/
      cal_ins = get_cal_ins (is_cal, ins, ra, &op2_cal);
      earliest = get_earliest_time (tip, op2_cal, FALSE);
      tip = insert_op (op2_cal, tip, earliest, &ins_tip);

      /* If overwrote source in ins like lux r3,r6,r3, then restore
         info on r3 to be that after the lux, i.e. r3 has loaded val.
      */
      if (rb_rt_lux) {
	 tip->gpr_rename[rb-R0] = post_lx_rb_gpr_rename;
	 tip->avail[rb]	        = post_lx_rb_avail;
	 tip->cluster[rb]       = post_lx_rb_clust;
      }

      /* Modify the displacement to reflect the current iteration of RA */
      disp = (1 + tip->vliw->mem_update_cnt[ra] + tip->mem_update_cnt[ra]++
	      - ins_tip->vliw->mem_update_cnt[ra]) * orig_disp;
      ins_cal_op = ins_tip->op->op;
      ins_cal_op->op.ins &= 0xFFFF0000;
      ins_cal_op->op.ins |= disp & 0xFFFF;
   }
   else {
      
      if (!sched_ld_before_st (&tip, op2_mem, ins_ptr, TRUE, &ins_tip,
			       &ver_tip, &verop)) {
	 earliest = get_earliest_time (tip, op2_mem, FALSE);
         if (!sched_ppc_rz0_op (&tip, op2_mem, earliest, &ins_tip))
            tip = insert_op (op2_mem, tip, earliest, &ins_tip);
      }

      ma_store (tip, op2_mem);		/* Save for alias analysis if STORE */

      /* Don't do update in two cases.  Update is also not done if the
         LOAD/STORE causes an alignment interrupt or a data storage
         interrupt, be we will ignore that for now.
      */
      if      (ra == 0)	{
	 extra_resrc[ALU] = 0;
	 *ptip = tip;
	 no_unify = FALSE;
	 return rval;
      }
      else if (is_ld  &&  ra == rt  &&  !is_flt) {
	 extra_resrc[ALU] = 0;
	 *ptip = tip;
	 no_unify = FALSE;
	 return rval;
      }

      /* If overwrote source in ins like lux r3,r6,r3, then make info
         on r3 be that before the lux, so cax references correct val.
      */
      if (rb_rt_lux) {
         extra_resrc[ALU]       = 0;
	 post_lx_rb_gpr_rename  = tip->gpr_rename[rb-R0];
	 post_lx_rb_avail       = tip->avail[rb];
	 post_lx_rb_clust       = tip->cluster[rb];
	 tip->gpr_rename[rb-R0] = pre_lx_rb_gpr_rename;
	 tip->avail[rb]	        = pre_lx_rb_avail;
      }

      cal_ins = get_cal_ins (is_cal, ins, ra, &op2_cal);
      earliest = get_earliest_time (tip, op2_cal, FALSE);

      if (is_cal) {
	 tip = insert_op (op2_cal, tip, earliest, &ins_tip);
	 tip->mem_update_cnt[ra]++;
      }
      else if (!combine_induc_var (&tip, op2_cal, ins_ptr, &ins_tip))
	 tip = insert_op (op2_cal, tip, earliest, &ins_tip);

      /* If overwrote source in ins like lux r3,r6,r3, then restore
         info on r3 to be that after the lux, i.e. r3 has loaded val.
      */
      if (rb_rt_lux) {
	 tip->gpr_rename[rb-R0] = post_lx_rb_gpr_rename;
	 tip->avail[rb]	        = post_lx_rb_avail;
	 tip->cluster[rb]       = post_lx_rb_clust;
      }
   }

   no_unify = FALSE;
   *ptip = tip;
   return rval;
}

/************************************************************************
 *									*
 *				get_cal_ins				*
 *				-----------				*
 *									*
 ************************************************************************/

static unsigned get_cal_ins (is_cal, ins, ra, op2_cal)
int	 is_cal;
unsigned ins;
unsigned ra;
OPCODE2  **op2_cal;
{
   unsigned cal_ins;
   OPCODE1  *op1_cal;

   if (is_cal) cal_ins = (ins & 0x03FFFFFF) | ( 14 << 26);      /* CAL */
   else        cal_ins = (ins & 0xFFFFF801) | (266 <<  1);	/* CAX */

   cal_ins &= 0xFC1FFFFF;			   /* Replace RT by RA */
   cal_ins |= ra << 21;

   op1_cal = get_opcode (cal_ins);
   *op2_cal = set_operands (op1_cal, cal_ins, op1_cal->b.format);

   return cal_ins;
}
