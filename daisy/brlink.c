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

#define MAX_BRLINKS_PER_PAGE			1024

static unsigned	      brlink_cnt;
static RESULT	      cau_wr[MAX_BRLINKS_PER_PAGE];
static RESULT	      oril_wr[MAX_BRLINKS_PER_PAGE];
static unsigned short oril_rd[MAX_BRLINKS_PER_PAGE];
static unsigned short mtlr_rd[MAX_BRLINKS_PER_PAGE];

static OPCODE2 op_cau;
static OPCODE2 op_oril;
static OPCODE2 op_mtlr;

static int cau_latency;
static int oril_latency;
static int mtlr_latency; 

extern OPCODE1     *ppc_opcode[];
extern int	   curr_rename_reg;
extern int	   curr_rename_reg_valid;
extern int	   num_gprs;
extern unsigned	   *curr_ins_addr;
extern UNIFY       *insert_unify;     /* Non-zero if unify instead of insert */

/************************************************************************
 *									*
 *				init_branch_and_link			*
 *				--------------------			*
 *									*
 * Setup the constant part of the opcodes "CAU", "ORIL", and "MTLR"	*
 * used by "branch_and_link".						*
 *									*
 ************************************************************************/

init_branch_and_link ()
{
   static RESULT         _lr;
   static unsigned char  cau_w_expl     =  RT    & (~OPERAND_BIT);
   static unsigned char  oril_w_expl    =  RA    & (~OPERAND_BIT);
   static unsigned char  mtlr_w_expl    =  SPR_T & (~OPERAND_BIT);
   static unsigned char  cau_r_expl[2]  = {RA    & (~OPERAND_BIT),
					   UI    & (~OPERAND_BIT)};
   static unsigned char  oril_r_expl[2] = {RS    & (~OPERAND_BIT),
					   UI    & (~OPERAND_BIT)};
   static unsigned char  mtlr_r_expl    =  RS    & (~OPERAND_BIT);

   op_cau.b	    = &ppc_opcode[OP_CAU]->b;
   op_cau.op.num_wr = 1;
   op_cau.op.num_rd = 0;
   op_cau.op.wr_explicit = &cau_w_expl;
   op_cau.op.rd_explicit =  cau_r_expl;
   op_cau.op.srcdest_common = -1;
   cau_latency = get_max_latency1 (ppc_opcode[OP_CAU]);

   op_oril.b	     = &ppc_opcode[OP_ORIL]->b;
   op_oril.op.num_wr = 1;
   op_oril.op.num_rd = 1;
   op_oril.op.wr_explicit = &oril_w_expl;
   op_oril.op.rd_explicit =  oril_r_expl;
   op_oril.op.srcdest_common = -1;
   oril_latency = get_max_latency1 (ppc_opcode[OP_ORIL]);

   _lr.reg	     = LR;
   op_mtlr.b	     = &ppc_opcode[OP_MTSPR]->b;
   op_mtlr.op.ins    = 0x7C0803A6;
   op_mtlr.op.num_wr = 1;
   op_mtlr.op.num_rd = 1;
   op_mtlr.op.wr     = &_lr;
   op_mtlr.op.wr_explicit = &mtlr_w_expl;
   op_mtlr.op.rd_explicit = &mtlr_r_expl;
   op_mtlr.op.srcdest_common = -1;
   mtlr_latency = get_max_latency1 (ppc_opcode[OP_MTSPR]);

   init_brlink_latencies ();
}

/************************************************************************
 *									*
 *				init_brlink_latencies			*
 *				---------------------			*
 *									*
 ************************************************************************/

static init_brlink_latencies ()
{
   int    i;
   int	  cau_latency;
   int	  or_latency;

   assert (ppc_opcode[OP_MTSPR]->op.num_wr == 1);
   op_mtlr.op.wr[0].latency = ppc_opcode[OP_MTSPR]->op.wr_lat[0];

   assert (ppc_opcode[OP_CAU]->op.num_wr   == 1);
   assert (ppc_opcode[OP_ORIL]->op.num_wr  == 1);
   cau_latency = ppc_opcode[OP_CAU]->op.wr_lat[0];
   or_latency  = ppc_opcode[OP_ORIL]->op.wr_lat[0];

   for (i = 0; i < MAX_BRLINKS_PER_PAGE; i++) {
       cau_wr[i].latency = cau_latency;
      oril_wr[i].latency =  or_latency;
   }

}

/************************************************************************
 *									*
 *				init_page_brlink			*
 *				----------------			*
 *									*
 ************************************************************************/

init_page_brlink ()
{
   op_cau.op.wr  = cau_wr;
   op_oril.op.wr = oril_wr;
   op_oril.op.rd = oril_rd;
   op_mtlr.op.rd = mtlr_rd;
   brlink_cnt    = 0;
}

/************************************************************************
 *									*
 *				branch_and_link				*
 *				---------------				*
 *									*
 * Handle branches that set the link register.  Since the link register	*
 * is maintained separately from the PPC link register, and since	*
 * none of our branches set the link register implicitly, the address	*
 * that would normally go in the PPC link register in a branch and	*
 * link must be explicitly comptuted and stored in the link register.	*
 * This is done with 3 instructions:					*
 *									*
 *	cau	r32,BRANCH_ADDR+4(r0)	# if bcrl instruction		*
 *	oril	r32,r32,BRANCH_ADDR+4	# BRANCH_ADDR is in original	*
 *      mtlr    LR,r32			# program			*
 *									*
 * We may have multiple branch_and_links in one VLIW instruction, each	*
 * with different targets and different return addresses.  To deal with	*
 * this, multiple templates are kept for the 3 instructions (cau, oril,	*
 * and mtlr).  These templates are used in sequence and should be	*
 * numerous enough to guarantee that a unique one is always available	*
 * for each "branch-and-link" in a VLIW instruction.			*
 *									*
 ************************************************************************/

TIP *branch_and_link (orig_tip, br_addr)
TIP	 *orig_tip;
unsigned *br_addr;
{
   int	    earliest;
   int	    orig_num_ops;
   int	    cau_num_ops;
   int	    oril_num_ops;
   int	    vliw_gpr;
   int	    cau_gpr;
   int	    oril_gpr;
   unsigned *link_addr = br_addr + 1;
   unsigned upper = ((unsigned) link_addr) >> 16;
   unsigned lower = ((unsigned) link_addr)  & 0xFFFF;
   TIP	    *ins_tip;
   TIP	    *cau_ins_tip;
   TIP	    *cau_tip;
   TIP	    *oril_tip;
   TIP	    *mtlr_tip;

   op_cau.op.ins  = 0x3C000000 | upper;
   op_oril.op.ins = 0x60000000 | lower;

   orig_num_ops = orig_tip->num_ops;

   assert (brlink_cnt < MAX_BRLINKS_PER_PAGE);

   op_cau.op.wr     = &cau_wr[brlink_cnt];
   op_oril.op.wr    = &oril_wr[brlink_cnt];
   op_oril.op.rd    = &oril_rd[brlink_cnt];
   op_mtlr.op.rd    = &mtlr_rd[brlink_cnt];

   vliw_gpr = get_brlink_start (orig_tip, &ins_tip);

   op_cau.op.orig_addr  = curr_ins_addr;
   op_oril.op.orig_addr = curr_ins_addr;
   op_mtlr.op.orig_addr = curr_ins_addr;

   op_cau.op.wr[0].reg			      = vliw_gpr;
   op_cau.op.renameable[RT & (~OPERAND_BIT)]  = vliw_gpr;
   cau_tip  = insert_op (&op_cau, orig_tip, ins_tip->vliw->time, &ins_tip);
   cau_ins_tip = ins_tip;

   /* If this the CAU is unified with the CAU from another path to
      the "bl", we may have to use its destination.
   */
   if (insert_unify) cau_gpr = insert_unify->op->op.wr[0].reg;
   else		     cau_gpr = vliw_gpr;

   op_oril.op.rd[0]			      = cau_gpr;
   op_oril.op.wr[0].reg			      = vliw_gpr;
   op_oril.op.renameable[RS & (~OPERAND_BIT)] = cau_gpr;
   op_oril.op.renameable[RA & (~OPERAND_BIT)] = vliw_gpr;

   cau_num_ops = cau_tip->num_ops;
   oril_tip = insert_op (&op_oril, cau_tip,
			 ins_tip->vliw->time + cau_latency, &ins_tip);

   oril_num_ops = oril_tip->num_ops;

   /* If this the ORIL is unified with the ORIL from another path to
      the "bl", we may have to use its destination.
   */
   if (insert_unify) oril_gpr = insert_unify->op->op.wr[0].reg;
   else		     oril_gpr = vliw_gpr;

   op_mtlr.op.rd[0]			      = oril_gpr;
   op_mtlr.op.renameable[RS & (~OPERAND_BIT)] = oril_gpr;

   /* Don't do MOVE_TO_LINKREG before branch or something might overwrite it */
   if (orig_tip->vliw->time > ins_tip->vliw->time + oril_latency)
        earliest = orig_tip->vliw->time;
   else earliest = ins_tip->vliw->time + oril_latency;

   mtlr_tip = insert_op (&op_mtlr, oril_tip, earliest, &ins_tip);
   update_reg_info2 (mtlr_tip, cau_ins_tip, oril_gpr,
		     cau_ins_tip->vliw->time + cau_latency);

   brlink_cnt++;

   return mtlr_tip;
}

/************************************************************************
 *									*
 *			    get_brlink_start				*
 *			    ----------------				*
 * Determine the earliest time at which "op_cau" may be placed,		*
 * accounting for whether a VLIW register is available, and return	*
 * the available VLIW register, and the "first_tip" at which it may	*
 * be used.								*
 *									*
 ************************************************************************/

int get_brlink_start (last_tip, pfirst_tip)
TIP	*last_tip;
TIP	**pfirst_tip;
{
   int earliest;
   TIP *first_tip;

   earliest = last_tip->vliw->time - cau_latency - oril_latency;
   if (earliest < 0) earliest = 0;

   op_cau.op.wr[0].reg = R1;	/* Pick random renameable PPC register */

   while (TRUE) {
      first_tip = get_tip_at_time (last_tip, earliest);
      assert (first_tip);

      if (set_reg_for_op (&op_cau, first_tip, last_tip)) {
	 assert (curr_rename_reg_valid == TRUE);
	 curr_rename_reg_valid = FALSE;
	 *pfirst_tip = first_tip;
	 return curr_rename_reg;
      }
      else earliest++;
   }
}
