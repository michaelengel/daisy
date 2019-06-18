/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#pragma alloca

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "dis_tbl.h"
#include "resrc_list.h"
#include "vliw.h"
#include "rd_config.h"
#include "bitvec-macros.h"
#include "vliw-macros.h"
#include "unify.h"

/* These are per page */
#define MAX_CCR_F_COMMITS		2048
#define MAX_CCR_B_COMMITS		512
#define MAX_FPR_COMMITS			8192
#define MAX_GPR_COMMITS			8192
#define MAX_CA_RENAMES		        2148

#define NUM_FMR_TYPES			6

OPCODE2 *latest_vliw_op;

extern int loophdr_bytes_per_path;

extern int store_latency;	/* If 0, need N*(N-1)/2 compares with LOADS */

extern int num_gprs;
extern int num_fprs;
extern int num_ccbits;

extern int reorder_cc_ops;			/* Boolean */

extern int num_resrc[];

extern int num_clusters;	       /* 0 ==> Not have clustered machine   */
extern int cross_cluster_penalty;
extern int cluster_resrc[];	       /* Bool: Reesource assoc with	     */
				       /*       clusters/whole machine	     */

extern unsigned *curr_ins_addr;
extern OPCODE1  *ppc_opcode[];

extern int	curr_rename_reg;		/* Used in brlink.c, rz0.c */
extern int	curr_rename_reg_valid;

       int	unusable_cluster;	     /* (-1) if all clusters usable */
       int      fits_cluster;
       UNIFY    *insert_unify;	      /* Non-zero if unify instead of insert */

       REG_RENAME     *map_mem[MAX_PATHS_PER_PAGE][NUM_PPC_GPRS_C+NUM_PPC_FPRS+NUM_PPC_CCRS];
       MEMACC	      *stores_mem[MAX_PATHS_PER_PAGE][NUM_MA_HASHVALS];
       unsigned	      *gpr_wr_mem[MAX_PATHS_PER_PAGE][NUM_PPC_GPRS_C];
       unsigned short avail_mem[MAX_PATHS_PER_PAGE][NUM_MACHINE_REGS];
       int	      avail_mem_cnt;
       int	      *resrc_needed;
       int	      no_unify;		  /* Turn off unify optimization */

       int	      main_resrc_needed[NUM_VLIW_RESRC_TYPES+1];
       int	      commit_resrc_needed[NUM_VLIW_RESRC_TYPES+1];
       int	      extra_resrc[NUM_VLIW_RESRC_TYPES+1];

static unsigned short commit_ccr_b_src[2*MAX_CCR_B_COMMITS];
static unsigned short commit_ccr_f_src[MAX_CCR_F_COMMITS];
static unsigned short commit_fpr_src[MAX_FPR_COMMITS];
static unsigned short commit_gpr_src[2*MAX_GPR_COMMITS];
static RESULT	      commit_ccr_b_dest[MAX_CCR_B_COMMITS];
static RESULT	      commit_ccr_f_dest[MAX_CCR_F_COMMITS];
static RESULT	      commit_fpr_dest[14*MAX_FPR_COMMITS];
static RESULT	      commit_gpr_dest[MAX_GPR_COMMITS];
static REG_RENAME     ca_rename[MAX_CA_RENAMES];

static unsigned short *pcommit_ccr_b_src;
static unsigned short *pcommit_ccr_f_src;
static unsigned short *pcommit_fpr_src;
static unsigned short *pcommit_gpr_src;
static RESULT	      *pcommit_ccr_b_dest;
static RESULT	      *pcommit_ccr_f_dest;
static RESULT	      *pcommit_fpr_dest;
static RESULT	      *pcommit_gpr_dest;


static VLIW	vliw_mem[MAX_VLIWS_PER_PAGE];
static TIP	tip_mem[MAX_TIPS_PER_PAGE];
static BASE_OP	base_op_mem[MAX_BASE_OPS_PER_PAGE];
static int	vliw_cnt;		   /* # VLIW's used so far on page */
static int	tip_cnt;		   /* # TIP's used so far on page  */
static int	base_op_cnt;
static int      ca_rename_cnt;		   /* Number of CA renames on page */
static int      doing_commit;

/* 
   Could merge these with 6 corresponding statics in "opcode_fmt_fcn.c"
   if memory space is tight
*/
static OPCODE2	      vliw_ops_mem[MAX_BASE_OPS_PER_PAGE];
static int	      vliw_ops_cnt;
static RESULT	      wr_area[MAX_PAGE_MOD_RESRC];
static unsigned short rd_area[MAX_PAGE_USE_RESRC];
static RESULT	      *ptr_wr = wr_area;
static unsigned short *ptr_rd = rd_area;

static OPCODE2  commit_gpr_op;
static OPCODE2  commit_gpr_c_op;
static OPCODE2  commit_gpr_o_op;
static OPCODE2  commit_gpr_co_op;
static OPCODE2  commit_fpr_op[NUM_FMR_TYPES];
static OPCODE2  commit_ccr_b_op;
static OPCODE2  commit_ccr_f_op;

unsigned renameable[NUM_WORDS_RESRCS];

VLIW *append_new_vliw (TIP *, int);
TIP  *get_tip_at_time (TIP *, int);
TIP  *add_op_to_tip   (OPCODE2 *, TIP *, TIP *, int, int);
TIP  *schedule_commit (int, int, int, int, TIP *, int, int, int);
TIP  *rename_output_regs (VLIW *, OPCODE2 *, TIP *, TIP *, OPERAND_OUT *, OPERAND_OUT *, int);
OPCODE2 *alloc_vliw_op (OPCODE2 *, int, int);
TIP *unify_try (OPCODE2 *, TIP *, int, TIP **);
TIP  *commit_update (TIP *, TIP *, OPCODE2 *, int, int, int, int,
		     int *, int *);

/************************************************************************
 *									*
 *				vliw_init				*
 *				---------				*
 *									*
 ************************************************************************/

vliw_init ()
{
   read_config    ();
   set_renameable ();

   resrc_needed = main_resrc_needed;
}

/************************************************************************
 *									*
 *				vliw_init_page				*
 *				--------------				*
 *									*
 ************************************************************************/

vliw_init_page ()
{
   int i;

   vliw_init_page_for_simul ();

   tip_cnt		= 0;
   vliw_cnt		= 0;
   vliw_ops_cnt		= 0;
   base_op_cnt		= 0;
   avail_mem_cnt	= 0;
   ca_rename_cnt	= 0;
   ptr_wr		= wr_area;
   ptr_rd		= rd_area;

   pcommit_gpr_dest   = commit_gpr_dest;
   pcommit_gpr_src    = commit_gpr_src;
   pcommit_fpr_dest   = commit_fpr_dest;
   pcommit_fpr_src    = commit_fpr_src;
   pcommit_ccr_b_dest = commit_ccr_b_dest;
   pcommit_ccr_b_src  = commit_ccr_b_src;
   pcommit_ccr_f_dest = commit_ccr_f_dest;
   pcommit_ccr_f_src  = commit_ccr_f_src;
}

/************************************************************************
 *									*
 *			vliw_init_page_for_simul			*
 *			------------------------			*
 *									*
 ************************************************************************/

vliw_init_page_for_simul ()
{
   int i;

   /* The time to clear this should not really be counted in the scheduling
      time as the "commit" value is used only in generating simulation code.
   */
   for (i = 0; i < base_op_cnt; i++)
      base_op_mem[i].commit = FALSE;
}

/************************************************************************
 *									*
 *				set_renameable				*
 *				--------------				*
 *									*
 * Mark all PPC architected GPR's, FPR's, and CCR's as renameable.	*
 * Leave everything else as unrenameable (0).				*
 *									*
 * (Perhaps we should wait on the CCR's?)				*
 *									*
 ************************************************************************/

set_renameable ()
{
   int i;
   int index;

   assert ((num_gprs   & 0x1F) == 0);	/* Number of regs is mult of 32 */
   assert ((num_fprs   & 0x1F) == 0);
   assert ((num_ccbits & 0x1F) == 0);

   assert ((R0  & 0x1F) == 0);		/* Regs begin at mult of 32	*/
   assert ((FP0 & 0x1F) == 0);
   assert ((CR0 & 0x1F) == 0);

   assert (CTR == R32);
   index = R0 >> 5;
   renameable[index++] = 0xFFFFFFFF;
   renameable[index++] = 0x00000001;	/* Assume CTR = R32 is renameable */
   for (i = 64; i < num_gprs; i += 32, index++)
      renameable[index] = 0;

   index = FP0 >> 5;
   renameable[index++] = 0xFFFFFFFF;
   for (i = 32; i < num_fprs; i += 32, index++)
      renameable[index] = 0;

   index = CR0 >> 5;
   renameable[index++] = 0xFFFFFFFF;
   for (i = 32; i < num_ccbits; i += 32, index++)
      renameable[index] = 0;

   set_bit (renameable, CA);
#ifdef ACCESS_RENAMED_OV_RESULT
   set_bit (renameable, OV);
#endif
}

/************************************************************************
 *									*
 *				get_vliw				*
 *				--------				*
 *									*
 * Return a new VLIW with only an entry tip and no ops on it.		*
 *									*
 ************************************************************************/

VLIW *get_vliw ()
{
   int  i, j;
   VLIW *vliw;
   TIP  *tip;

   vliw	 = &vliw_mem[vliw_cnt++];
   assert (vliw_cnt <= MAX_VLIWS_PER_PAGE);

   /* Non-cluster resources start at 0 and increment until limit is reached */
    for (i = 0; i < NUM_VLIW_RESRC_TYPES+1; i++)
      vliw->resrc_cnt[i] = 0;

   /* All VLIW's have at least 1 leaf branch */
   vliw->resrc_cnt[BRLEAF] = 1;

   /* Cluster resources start at the number of resources available in
      cluster, then are incremented until no resource are available.
   */
   for (i = 0; i < num_clusters; i++)
      for (j = 0; j < NUM_VLIW_RESRC_TYPES+1; j++)
	 vliw->cluster[i][j] = 0;

   tip   = &tip_mem[tip_cnt++];
   assert (tip_cnt <= MAX_TIPS_PER_PAGE);

   vliw->entry   = tip;
   vliw->xlate   = 0;			/* So know if xlated to PPC simcode */
   vliw->visited = 0;
   tip->vliw     = vliw;		/* Belong to the overall VLIW */
   tip->prev     = 0;			/* 1st tip of VLIW */
   tip->orig_ops = 0;
   tip->num_ops  = 0;			/* No ops in list yet */
   tip->op       = 0;
   tip->br_addr  = 0;

   mem_clear (tip->mem_update_cnt, 
	      NUM_PPC_GPRS_C * sizeof (tip->mem_update_cnt[0]));

   return vliw;
}

/************************************************************************
 *									*
 *				get_tip					*
 *				-------					*
 *									*
 * Return pointer to a TIP.  TIP is not guaranteed to be initialized.	*
 *									*
 ************************************************************************/

TIP *get_tip ()
{
   TIP *tip = &tip_mem[tip_cnt++];
   assert (tip_cnt <= MAX_TIPS_PER_PAGE);
   tip->br_addr  = 0;
   tip->orig_ops = 0;

   return tip;
}

/************************************************************************
 *									*
 *				reset_tip_avail				*
 *				---------------				*
 *									*
 * Sets to 0 the "avail" time for all resources in each tip used on	*
 * current page.  This is typically called after completing translation	*
 * on the current page, so as to be ready for the next page.		*
 *									*
 ************************************************************************/

reset_tip_avail ()
{
   int i;

   for (i = 0; i < tip_cnt; i++)
      mem_clear (tip_mem[i].avail, NUM_MACHINE_REGS*sizeof(tip_mem[0].avail));
}

/************************************************************************
 *									*
 *				init_reg_avail				*
 *				--------------				*
 *									*
 * Make available all VLIW registers that do coincide with PPC		*
 * registers, i.e. r32-r63, fp32-fp63, and cc32-cc63 are made		*
 * available.  This is typically called at serialization points in	*
 * the code such as "bl" instructions and their immediate successor.	*
 *									*
 ************************************************************************/

init_reg_avail (tip)
TIP *tip;
{
   set_all_bits (tip->gpr_vliw, num_gprs);
   set_all_bits (tip->fpr_vliw, num_fprs);
   set_all_bits (tip->ccr_vliw, num_ccbits);

   tip->gpr_vliw[0] = 0;	  /* PPC registers are never available */
   tip->fpr_vliw[0] = 0;
   tip->ccr_vliw[0] = 0;

#ifndef CTR_IS_NOT_R32
   tip->gpr_vliw[1] = 0xFFFFFFFE; /* R32, the PPC CTR is never available */
#endif

#ifndef MQ_IS_NOT_R33
   tip->gpr_vliw[1] &= 0xFFFFFFFD; /* R33, the Power MQ is never available */
#endif
}

/************************************************************************
 *									*
 *				get_datadep_earliest			*
 *				--------------------			*
 *									*
 * Looking only at data dependences, what is the earliest "op" can go	*
 * on the path ended by "tip"?						*
 *									*
 ************************************************************************/

int get_datadep_earliest (tip, op)
TIP	*tip;
OPCODE2 *op;
{
   int		  i;
   int	          max_time = 0;
   int		  num_rd   = op->op.num_rd;
   unsigned short *rd      = op->op.rd;
   unsigned short *avail   = tip->avail;

   for (i = 0; i < num_rd; i++)
      if (tip->avail[rd[i]] > max_time) max_time = tip->avail[rd[i]];

   return max_time;
}

/************************************************************************
 *									*
 *				get_earliest_time			*
 *				-----------------			*
 *									*
 * Return the latest time that any of the operands of "op" are		*
 * available along the path containing "vliw".  Also check whether	*
 * any of the resources written by the instruction are serializing,	*
 * i.e. whether any resources written are not renameable.		*
 *									*
 ************************************************************************/

int get_earliest_time (tip, op, ignore_mem)
TIP	*tip;
OPCODE2 *op;
int	ignore_mem;	/* Boolean:  TRUE if moving LOADS above STORES */
{
   int i;
   int reg;
   int rz_reg;
   int num_rd;
   int num_wr;
   int max_time;
   int avail_time;
   int wrote_ca;
   int non_renameable_dest;
   int renameable_wr_cnt;
   int set_avail_time;
   unsigned short *rd;
   RESULT	  *wr;
   unsigned short *avail;

   /* There can be no registers/resources read:  Examples: lil    r3,12 */
   /*						           mtfsb1 30	*/
   if (op->op.num_rd == 0) max_time = 0;
   else if (is_mftb_op (op)) return tip->vliw->time;
   else {

      num_rd = op->op.num_rd;
      rd = op->op.rd;
      avail = tip->avail;

      /* We can't schedule operations like RLIMI, RLMI, RRIB, and MASKIR
         until the committed RA value is available.  This is because RA
         is used as both a source and destination register (and cannot
         be renamed to avoid this).
      */
      if (op->op.srcdest_common < 0) max_time = -1;
      else {
	 assert (op->op.srcdest_common >= R0  &&
		 op->op.srcdest_common <  R0 + NUM_PPC_GPRS_C);
	 max_time = tip->gpr_rename[op->op.srcdest_common - R0]->time;
	 if (max_time < tip->vliw->time) max_time = tip->vliw->time;
      }

      if (op->op.srcdest_common >= 0) {
	 assert (op->op.srcdest_common >= R0  &&
		 op->op.srcdest_common <  R0 + NUM_PPC_GPRS_C);
	 max_time = tip->gpr_rename[op->op.srcdest_common - R0]->time;
	 if (max_time < tip->vliw->time) max_time = tip->vliw->time;
      }
      else if (is_daisy_rz_op (op)) {
	 /* Assumes DAISY RZ ops are a subset of PowerPC */
	 rz_reg = op->op.renameable[RZ & (~OPERAND_BIT)];
         if (rz_reg != R0) max_time = get_rz_avail_time (tip, rz_reg);
         else max_time = -1;
      }
      else max_time = -1;

      avail_time = 0;	   /* O.w. uninit on  l r5,4(0) */
      for (i = 0; i < num_rd; i++) {
	 reg = rd[i];

	 /* MFCR and MCRF must wait for input bits to be committed */
	 if (op->b->op_num == OP_MFCR  ||  op->b->op_num == OP_MCRF) {
	    if (!tip->ccr_rename[reg - CR0]) avail_time = 0;
	    else avail_time = tip->ccr_rename[reg - CR0]->time;
	 }
	 else if (is_bit_set (renameable, reg)) {
	    avail_time = avail[reg];
	    if (!reorder_cc_ops && (reg >= CR0 && reg < CR0 + num_ccbits))
	       if (tip->vliw->time > avail_time) avail_time = tip->vliw->time;
	 }
	 else { /* LOAD-VERIFY ==> Can ignore memory availability when sched */

	    if      (!ignore_mem)	      set_avail_time = TRUE;
	    else if (!is_simple_load_op (op)) set_avail_time = TRUE;
	    else if (!is_mem_operand (reg))   set_avail_time = TRUE;
            else			      set_avail_time = FALSE;

	    if (set_avail_time) avail_time = avail[reg];
	 }
         if (avail_time > max_time) max_time = avail_time;
      }

      /* The CA or OV bits of the XER may be available early for ops like
         SFE.  However, if we are doing an MFSPR, then we must wait for
         the bits to be committed back to the XER.
      */
      if (op->b->op_num == OP_MFSPR  &&  (rd[0] >= XER0  &&  rd[0] < XER31)) {
         if (max_time < tip->ca_rename.time) max_time = tip->ca_rename.time;
#ifdef ACCESS_RENAMED_OV_RESULT
         if (max_time < tip->ov_rename.time) max_time = tip->ov_rename.time;
#endif
      }

      if (max_time >= tip->vliw->time) return max_time;
   }

   /* (1) If the op has non-renameable destinations, it must go at the end.
          (The CA, OV, and SO bits can be renamed in conjunction with their
          non-architected GPR.  For example, "ai r62,r3,r4" would place
          CA in a bit associated with r62 instead of in the XER.  When r62
	  is committed to a PPC architected register such as r5, the CA bit
	  with r62 is copied to the CA bit in the XER.  For operations that
	  set OV and SO, the OV is copied like CA at commit time, while SO
	  in the XER is or'ed with the OV bit.

          The same is true for bits 0-19 of the FPSCR, which are set by
	  floating point arithmetic ops.

      (2) For now, we can rename at most one destination.  If there are
	  more, the operation must be placed at the end (with no renaming).
   */
   wrote_ca = FALSE;
   num_wr = op->op.num_wr;
   wr = op->op.wr;
   if      (renameable_fp_arith_op (op)) {}
#ifdef SCHED_MEMOPS_IN_ORDER
   else if (is_memop (op)) return tip->vliw->time;
#endif
   else if (!is_condreg_field (wr, num_wr)) {
      non_renameable_dest = FALSE;
      renameable_wr_cnt = 0;
      for (i = 0; i < num_wr; i++) {
         reg = wr[i].reg;

	 if (!reorder_cc_ops  &&  (reg >= CR0  &&  reg < CR0 + num_ccbits)) {
            non_renameable_dest = TRUE;
	    break;
	 }

         if (is_bit_set (renameable, reg)) {
	    renameable_wr_cnt++;
	    if (reg == CA) wrote_ca = TRUE;
	 }
         else if (reg != CA  &&  reg != OV  &&  reg != SO) {
	    non_renameable_dest = TRUE;
	    break;
	 }
      }
      if (non_renameable_dest  ||  (
          renameable_wr_cnt > 1  &&  !(renameable_wr_cnt == 2  &&  wrote_ca)))
         max_time = tip->vliw->time;
   }
   else if (!reorder_cc_ops) max_time = tip->vliw->time;

   assert (max_time != -1);
   return max_time;
}

/************************************************************************
 *									*
 *				get_rz_avail_time			*
 *				-----------------			*
 *									*
 * Let OP be an op such as "cal" with an RZ field.  Let the RZ register	*
 * be "rz".  Then if through copy propagation, "rz" at some point maps	*
 * to 0, there is a problem.  For example "cal  r5,4(r9)" would become	*
 * "cal r5,4(r0)", i.e. "lil r5,0" if r9 mapped to r0 at some point	*
 * and the "cal" were inserted there.  Hence if this occurs, the op	*
 * cannot be placed until the VLIW in which the register after r0 in	*
 * the copy chain is available.  This could be the VLIW in which r9	*
 * itself becomes for instance.						*
 *									*
 ************************************************************************/

int get_rz_avail_time (tip, reg)
TIP *tip;
int reg;
{
   REG_RENAME *mapping;
   REG_RENAME *prev_mapping;

   /* reg should not be R0:  This is an RZ depending on the register
      in the RZ field, and were it 0, this register should be empty.
   */
   assert (reg > R0  &&  reg < R0 + num_gprs);

   prev_mapping = tip->gpr_rename[reg - R0];
   if (!prev_mapping) return tip->avail[reg];

   for (mapping = prev_mapping->prev; mapping; mapping = mapping->prev) {
      if (mapping->vliw_reg == 0) return prev_mapping->time;
      prev_mapping = mapping;
   }

   return tip->avail[reg];
}

/************************************************************************
 *									*
 *				insert_op				*
 *				---------				*
 *									*
 * Insert op at earliest place where input operands are ready and	*
 * resources exist to execute it.  This involves 3 steps as indicated	*
 * below:								*
 *									*
 * (1)									*
 * if (earliest < curr_tip->vliw->time)					*
 *    make list of VLIW tips						*
 *    try to fit from earliest to immediately before current		*
 *    if fit found, put op there with renaming.  SUCCESS		*
 *    else earliest = curr_tip->vliw->time				*
 *									*
 * (2)									*
 * if (earliest == curr_tip->vliw->time)				*
 *    try to fit in current						*
 *    if fits, Put op in current with no renaming, 			*
 *	       Put commit in current.  SUCCESS				*
 *    else earliest = curr_tip->vliw->time + 1				*
 *									*
 * (3)									*
 * if (earliest > curr_tip->vliw->time)					*
 *    curr_time = curr_tip->vliw->time					*
 *    while (earliest > curr_time)					*
 *       add new VLIW with curr_time + 1				*
 *       curr_time++							*
 *    put op in most recently added with no renaming.  SUCCESS		*
 *    curr_tip = most recently added entry				*
 *									*
 * RETURNS:  The last TIP along this path.  (On entry to this		*
 *	     routine the last TIP should be "curr_tip".  Hence		*
 *           the last TIP returned will most often be "curr_tip".	*
 *           The exception is when "opcode" must be scheduled 		*
 *           beyond the current last TIP.)  The TIP at which the	*
 *	     operation is inserted is also returned via the parameter	*
 *	     "ins_tip".							*
 *									*
 ************************************************************************/

TIP *insert_op (opcode, curr_tip, earliest_in, ins_tip)
OPCODE2 *opcode;
TIP	*curr_tip;
int	earliest_in;
TIP	**ins_tip;
{
   int     i;
   int	   limit;
   int	   earliest = earliest_in;
   int	   same_clust;
   int     list_size;
   int     time;
   int	   curr_time;
   int	   ca_ext_op;
   int	   did_unify;
   TIP     *rtn_tip;
   TIP     **tip_list;
   VLIW	   *new_vliw;

   /* Sequentialize STORES to avoid problems with anti & output dependences */
   if (is_store_op (opcode)) {
      if (earliest < curr_tip->vliw->time) earliest = curr_tip->vliw->time;
   }
   else if (!no_unify) {
      curr_tip = unify_try (opcode, curr_tip, earliest, ins_tip);
      if (*ins_tip) return curr_tip;
   }

   curr_time = curr_tip->vliw->time;

   if (ca_ext_op = is_carry_ext_op (opcode)) {
      extra_resrc[ALU]   = 1;
      extra_resrc[ISSUE] = 1;
   }

   get_needed_resrcs (opcode, resrc_needed);

   /**************** (1) ****************/
   if (earliest < curr_time) {
      tip_list = alloca((curr_time - earliest) * sizeof (tip_list[0]));
      make_tip_list (curr_tip, earliest, curr_time - 1, tip_list);

      list_size = curr_time - earliest;

      /* With clustering, scheduling at earliest times requires
         (1) the latest available inputs be in the same cluster, and
         (2) opcode fits in that cluster
         *  (1) must be generalized further if cross-cluster penalties
            can be greater than 1:  all inputs finishing within
            (cross_cluster_penalty-1) of the latest finisher must be in
	    the same cluster.
         ** Note that all inputs in the same cluster is a subcase of (1).
      */
      if (list_size < cross_cluster_penalty) limit = list_size;
      else				     limit = cross_cluster_penalty;

      for (i = 0; i < limit; i++) {
	 if (opcode_fits (opcode, tip_list[i], curr_tip, TRUE, TRUE,
			  resrc_needed))  break;
      }

      /* Same cluster not required for these times */
      if (i == limit) for (     ; i < list_size; i++) {
	 if (opcode_fits (opcode, tip_list[i], curr_tip, TRUE, FALSE,
			  resrc_needed)) break;
      }

      if (i == list_size) earliest = curr_time;
      else {
	 /* SUCCESS:  Fit found.  Put op at tip_list[i] with renaming,
		      i.e. the commit is at "curr_tip" or after.
	 */
	 curr_tip = add_op_to_tip (opcode, tip_list[i], curr_tip, TRUE,
				   ca_ext_op);
	 *ins_tip = tip_list[i];
	 unify_add_op (*ins_tip, (*ins_tip)->op->op);
      }
   }

   /**************** (2) ****************/
   if (earliest == curr_time) {
      same_clust = ((get_datadep_earliest (curr_tip, opcode) + 
		     cross_cluster_penalty) > curr_time);

      if (opcode_fits (opcode, curr_tip, curr_tip, FALSE, same_clust,
		       resrc_needed)) {
	 curr_tip = add_op_to_tip (opcode, curr_tip, curr_tip, FALSE,
				   ca_ext_op);
	 *ins_tip = curr_tip;
      }
      else earliest = curr_time + 1;
   }

   /**************** (3) ****************/
   if (earliest > curr_time) {

      /* If not all inputs are in the same cluster, we must wait some more */
      if (latest_inputs_same_cluster (curr_tip, opcode) < 0) {
         if (earliest < earliest_in + cross_cluster_penalty)
             earliest = earliest_in + cross_cluster_penalty;
      }

      for (time = curr_time + 1; time <= earliest; time++) {
	 new_vliw = append_new_vliw (curr_tip, time);
	 curr_tip = new_vliw->entry;
      }
      rtn_tip = add_op_to_tip (opcode, curr_tip, curr_tip, FALSE, ca_ext_op);
      assert (rtn_tip == curr_tip);
      *ins_tip = curr_tip;
   }

   if (ca_ext_op) {
      extra_resrc[ALU]   = 0;
      extra_resrc[ISSUE] = 0;
   }

   return curr_tip;
}

/************************************************************************
 *									*
 *				add_op_to_tip				*
 *				-------------				*
 *									*
 * Insert op at current tip.						*
 *									*
 * Assumes that caller has checked that sufficient resources exist	*
 * to execute "ppc_op" here, and that all input operands are ready.	*
 *									*
 * CREATE new opcode "vliw_op" copied from "ppc_op".  An operation	*
 * "ppc_op" may appear on more than one VLIW path and have its		*
 * operands named differently on the different paths.			*
 *									*
 ************************************************************************/

TIP *add_op_to_tip (ppc_op, tip_op, tip_commit, do_renaming, ca_ext_op)
OPCODE2 *ppc_op;
TIP	*tip_op;
TIP	*tip_commit;			/* Commit done here or later */
int	do_renaming;			/* Boolean */
int     ca_ext_op;		        /* Boolean -- AE, SFE, SFZE, etc */
{ 
   int	       i;
   int	       latency;
   int	       expl;
   int	       reg;
   int	       ca_explicit;
   int	       num_wr;
   int	       done_time;
   int	       curr_cluster;
   int	       cr_field = !is_cr_bit_op (ppc_op);
   int	       *save_resrc_needed;
   OPCODE2     *vliw_op;
   VLIW	       *vliw    = tip_op->vliw;
   OPERAND_OUT *v_operand;
   OPERAND_OUT *p_operand;
   BASE_OP     *base_op = &base_op_mem[base_op_cnt++];

   assert (base_op_cnt <= MAX_BASE_OPS_PER_PAGE);
   curr_cluster = fits_cluster;

   if (!ca_ext_op) ca_explicit = FALSE;
   else		   ca_explicit = is_ca_input_explicit (tip_op, tip_commit);

   /* NOTE:  We allocate "vliw_op" unnecessarily when recursively invoked
	     with commit operation.
   */
   vliw_op	  = alloc_vliw_op (ppc_op, do_renaming, ca_explicit);
   latest_vliw_op = vliw_op;

   v_operand = &vliw_op->op;
   p_operand = &ppc_op->op;

   /* If no resources read for ins (e.g. LIL), "ppc_map.c" can get
      confused unless the RZ field is 0, so make sure it is.
   */
   if (ppc_op->op.num_rd == 0) v_operand->renameable[RZ & (~OPERAND_BIT)] = 0;

   fix_renamed_input_regs (vliw, tip_commit, p_operand, v_operand);

   /* Set the dest registers and commit */
   num_wr = p_operand->num_wr;
   if (!do_renaming) {
      for (i = 0; i < num_wr; i++) {
         reg     = p_operand->wr[i].reg;
         latency = p_operand->wr[i].latency;

	 if (reg == MEM  ||  reg == DCACHE)
	      done_time = vliw->time + store_latency;
	 else done_time = vliw->time + latency;

	 if (reg == CA) {
	    tip_commit->ca_rename.vliw_reg = CA;
	    tip_commit->ca_rename.time     = done_time;
	    tip_commit->ca_rename.prev     = 0;
	 }

	 update_reg_info (tip_op, tip_op, reg, -1, done_time, done_time);
      }
   }
   else {
      save_resrc_needed = resrc_needed;
      resrc_needed      = commit_resrc_needed;
      tip_commit = rename_output_regs (vliw, vliw_op, tip_op, tip_commit, 
				       p_operand, v_operand, cr_field);
      resrc_needed      = save_resrc_needed;
   }

   /* Pass ppc_op because it has "rd_vec" and "wr_vec" set unlike vliw_op,
      and "rd_vec" and "wr_vec" are used to determine the op type.
   */
   incr_resrc_usage (tip_op, vliw->resrc_cnt, resrc_needed, curr_cluster);
   do_reg_cluster_assignments (tip_commit, ppc_op, curr_cluster);

   /* Put the operation into the list of operations for this TIP unless it
      is a non-trap branch.   A non-trap branch will be the br_ins for 
      a TIP.
   */
   if (!is_branch (ppc_op)  ||  is_trap (ppc_op)) {
      base_op->op      = vliw_op;
      base_op->next    = tip_op->op;
      base_op->cluster = curr_cluster;
      tip_op->op       = base_op;
      tip_op->num_ops++;
   }

   return tip_commit;
}

/************************************************************************
 *									*
 *				fix_renamed_input_regs			*
 *				----------------------			*
 *									*
 * Patch inputs to operation to reflect any commits that have been	*
 * done in this VLIW.  In other words, since we are using parallel	*
 * semantics, if earlier in this VLIW we have "COPY  R5,R63", and	*
 * we now have the op, "CAL  R4,0(R5)", it should be changed to		*
 * "CAL  R4,0(R63)".							*
 *									*
 ************************************************************************/

fix_renamed_input_regs (vliw, tip_commit, p_operand, v_operand)
VLIW	    *vliw;
TIP	    *tip_commit;
OPERAND_OUT *p_operand;
OPERAND_OUT *v_operand;
{
   int	       i;
   int	       expl;
   int	       reg;
   int	       num_rd;

   /* Indicate the source registers */
   num_rd = p_operand->num_rd;
   for (i = 0; i < num_rd; i++) {
      reg = p_operand->rd[i];

      if (is_bit_set (renameable, reg)) {

         /* At "tip_op", the "rename_reg" still refers to whatever
	    old value it had.  Thereafter, its availability is the
	    time at which the current op finishes and writes its
	    result to "rename_reg".  Likewise "reg" is mapped to
	    "rename_reg" after "tip_op".  At "tip_op", its old value
	    may still be read.
         */
	 reg = get_mapping (tip_commit, reg, vliw->time);

	 expl = v_operand->rd_explicit[i];
	 if (expl >= 0) v_operand->renameable[expl] = reg;
      }
      v_operand->rd[i] = reg;
   }
}

/************************************************************************
 *									*
 *				rename_output_regs			*
 *				------------------			*
 *									*
 *									*
 * NOTE: If the destination is a 4-bit conditition register field, as	*
 *	 with "cmp", each of the 4 bits will be a separate "wr"		*
 *	 operand.  Make sure here and below with "renaming_ok" that we	*
 *	 only rename the 4-bit field once (when we encounter the bit	*
 *	 whose value mod 4 is zero).					*
 *									*
 ************************************************************************/

TIP *rename_output_regs (vliw, op, tip_op, tip_commit, p_operand, v_operand,
			 cr_field)
VLIW	    *vliw;
OPCODE2	    *op;
TIP	    *tip_op;
TIP	    *tip_commit;
OPERAND_OUT *p_operand;
OPERAND_OUT *v_operand;
int	    cr_field;
{
   int	       i;
   int	       reg;
   int	       expl;
   int	       num_wr;
   int	       renaming_ok;
   int	       rename_reg;
   int	       gpr_rename_reg;
   int	       done_time;
   int	       latency;
   int	       commit_latency;
   int	       fp_arith_op;			/* Boolean */
   REG_RENAME  *ca_ren;

   num_wr = p_operand->num_wr;
   fp_arith_op = renameable_fp_arith_op (op);

   /* Used by ops like "addic" which also set CA ==> Can be sure
      that the GPR used for CA renaming is correct.
   */
   gpr_rename_reg = -1;		    

   for (i = 0; i < num_wr; i++) {
      reg     = p_operand->wr[i].reg;
      latency = p_operand->wr[i].latency;

      if (reg == MEM || reg == DCACHE)
	   done_time = vliw->time + store_latency;
      else done_time = vliw->time + latency;

      if (!(cr_field  &&  (reg >= CR0  &&  reg < CR0 + num_ccbits)))
			       renaming_ok = TRUE;
      else if ((reg & 3) == 0) renaming_ok = TRUE;
      else		       renaming_ok = FALSE;

      /* The CA, OV, and SO bits are unique in that they can be renamed,
         but only in conjunction with the GPR receiving  the "main"
	 result of the operation, with the result that CA, OV, and SO
         values are committed in the same operation (e.g. or_c) as the
	 GPR result is committed.  Since operations writing CA, OV, and SO
         can be renamed, they can be moved up and must have data structures
         updated from when the operation is performed until its results
         are committed.
      */
      if (reg == CA  ||  reg == OV  ||  reg == SO) {
	 v_operand->wr[i].reg     = reg;
	 v_operand->wr[i].latency = latency;

	 /* It is important in some applications (e.g. killtime from
            m88ksim in SPEC95) to access the CA value before it is
	    committed to the XER.  The SFE instruction benefits for example.
	    Thus, we track when CA is available and the non-architected
	    GPR in which it is available.
	 */
	 if (reg == CA) {
	    assert (gpr_rename_reg >= 0);
	    assert (ca_rename_cnt < MAX_CA_RENAMES);
	    ca_ren = &ca_rename[ca_rename_cnt++];

	    tip_commit->ca_rename.vliw_reg = CA;
	    tip_commit->ca_rename.time = tip_commit->vliw->time+commit_latency;
	    tip_commit->ca_rename.prev = ca_ren;

	    ca_ren->vliw_reg	       = gpr_rename_reg;
	    ca_ren->time	       = done_time;
	    ca_ren->prev	       = 0;

	    tip_commit->avail[reg]     = done_time;
	 }
	 else {
	    commit_latency	   = commit_gpr_dest[0].latency;
	    tip_commit->avail[reg] = tip_commit->vliw->time + commit_latency;
	 }
      }
      else if ((reg < FP0  ||  reg >= FP0 + num_fprs) && fp_arith_op) {
	 v_operand->wr[i].reg     = reg;
	 v_operand->wr[i].latency = latency;
         commit_latency = commit_fpr_dest[0].latency;
         tip_commit->avail[reg] = tip_commit->vliw->time + commit_latency;
      }
      else if (!is_bit_set (renameable, reg)) {
	 v_operand->wr[i].reg     = reg;
	 v_operand->wr[i].latency = latency;
	 update_reg_info2 (tip_op, tip_op, reg, done_time);
      }
      else if (!renaming_ok) {
	 /* This is for bits 1, 2, and 3 of a condition register field.
	    Bit 0 is handled in the else clause below.
	 */
	 rename_reg = curr_rename_reg + (reg & 3);
	 v_operand->wr[i].reg     = rename_reg;
	 v_operand->wr[i].latency = latency;
         commit_latency = commit_ccr_f_dest[0].latency;
	 update_reg_info (tip_commit, tip_op, reg, rename_reg, done_time,
			  tip_commit->vliw->time + commit_latency);
      }
      else {
	 assert (curr_rename_reg_valid == TRUE);
	 rename_reg = curr_rename_reg;
	 curr_rename_reg_valid = FALSE;

	 v_operand->wr[i].reg     = rename_reg;
	 v_operand->wr[i].latency = latency;
	 expl = v_operand->wr_explicit[i];
	 if (expl >= 0) v_operand->renameable[expl] = rename_reg;

	 tip_commit = commit_update (tip_commit, tip_op, op, reg, rename_reg,
				     done_time, cr_field, 
				     &gpr_rename_reg, &commit_latency);
      }
   }

   return tip_commit;
}

/************************************************************************
 *									*
 *				unify_try				*
 *				---------				*
 *									*
 ************************************************************************/

TIP *unify_try (op, tip_commit, earliest, unify_tip)
OPCODE2	    *op;
TIP	    *tip_commit;
int	    earliest;
TIP	    **unify_tip;
{
   int	       i;
   int	       reg;
   int	       num_wr;
   int	       renaming_ok;
   int	       gpr_rename_reg;
   int	       done_time;
   int	       latency;
   int	       commit_latency;
   int	       cr_field;
   UNIFY       *unify;
   VLIW	       *vliw;
   TIP	       *tip_op;
   OPERAND_OUT *p_operand;
   REG_RENAME  *ca_ren;

   if (! (unify = unify_find (tip_commit, op, earliest))) {
      *unify_tip = 0;
      if (!doing_commit) insert_unify = 0;
      return tip_commit;
   }
   else {
      insert_unify = unify;
      *unify_tip   = unify->tip;
      tip_op       = unify->tip;
      vliw         = tip_op->vliw;
      p_operand    = &op->op;
      cr_field     = !is_cr_bit_op (op);
   }

   /* Used by ops like "addic" which also set CA ==> Can be sure
      that the GPR used for CA renaming is correct.
   */
   gpr_rename_reg = -1;		    

   num_wr = p_operand->num_wr;
   for (i = 0; i < num_wr; i++) {
      reg     = p_operand->wr[i].reg;
      latency = p_operand->wr[i].latency;

      if (reg == MEM || reg == DCACHE)
	   done_time = vliw->time + store_latency;
      else done_time = vliw->time + latency;

      if (!(cr_field  &&  (reg >= CR0  &&  reg < CR0 + num_ccbits)))
			       renaming_ok = TRUE;
      else if ((reg & 3) == 0) renaming_ok = TRUE;
      else		       renaming_ok = FALSE;

      if (reg == CA  ||  reg == OV  ||  reg == SO) {
	 if (reg == CA) {
	    assert (gpr_rename_reg >= 0);
	    assert (ca_rename_cnt < MAX_CA_RENAMES);
	    ca_ren = &ca_rename[ca_rename_cnt++];

	    tip_commit->ca_rename.vliw_reg = CA;
	    tip_commit->ca_rename.time = tip_commit->vliw->time+commit_latency;
	    tip_commit->ca_rename.prev = ca_ren;

	    ca_ren->vliw_reg	       = gpr_rename_reg;
	    ca_ren->time	       = done_time;
	    ca_ren->prev	       = 0;

	    tip_commit->avail[reg]     = done_time;
	 }
	 else {
	    commit_latency	   = commit_gpr_dest[0].latency;
	    tip_commit->avail[reg] = tip_commit->vliw->time + commit_latency;
	 }
      }
      else if (!renaming_ok) {
	 /* This is for bits 1, 2, and 3 of a condition register field.
	    Bit 0 is handled in the else clause below.
	 */
         commit_latency = commit_ccr_f_dest[0].latency;
	 update_reg_info (tip_commit, tip_op, reg, unify->dest + (reg & 3),
			  done_time,
			  tip_commit->vliw->time + commit_latency);
      }
      else {
	 /* Sometimes we don't want to do a commit and update, as with
	    CAU, ORIL for a branch-and-link.
	 */
	 if (is_bit_set (renameable, reg)) tip_commit =
	    commit_update (tip_commit, tip_op, op, reg, unify->dest, done_time,
			   cr_field, &gpr_rename_reg, &commit_latency);
      }
   }

   return tip_commit;
}

/************************************************************************
 *									*
 *				commit_update				*
 *				-------------				*
 *									*
 * Commit "rename_reg" to "reg" and update the register info from	*
 * "tip_commit" to "tip_op".						*
 *									*
 ************************************************************************/

TIP *
commit_update (tip_commit, tip_op, op, reg, rename_reg, done_time, cr_field,
	       p_gpr_rename_reg, p_commit_latency)
TIP	*tip_commit;
TIP	*tip_op;
OPCODE2	*op;
int	reg;
int	rename_reg;
int	done_time;
int	cr_field;
int	*p_gpr_rename_reg;		  /* Output */
int	*p_commit_latency;		  /* Output */
{
   int gpr_rename_reg;
   int commit_latency;
   int ca_set   = sets_ppc_ca_bit     (op);
   int ov_set   = sets_ppc_ov_bit     (op);
   int fmr_type = get_fmr_commit_type (op);

   tip_commit = schedule_commit (reg, rename_reg, ca_set, ov_set, tip_commit,
				 done_time, cr_field, fmr_type);

   /* Get the commit latency */
   if      (reg >= R0   &&  reg < R0 + num_gprs) {
      commit_latency = commit_gpr_dest[0].latency;
      *p_gpr_rename_reg = rename_reg;
   }
   else if (reg >= FP0  &&  reg < FP0 + num_fprs)
      commit_latency = commit_ccr_f_dest[0].latency;
   else if (reg >= CR0  &&  reg < CR0 + num_ccbits) {
      commit_latency = commit_ccr_b_dest[0].latency;
      assert (commit_latency == commit_ccr_f_dest[0].latency);
   }

   /* We require that the commit latency be one because of
      copy propagation.  A value is assumed to be continuously
      available from its definition (plus latency) until its
      final commit.  If a commit takes more than one cycle,
      then its (non-PPC-architected) source register may be
      used for something else, making the continuity assumption
      false, with the consequence that something may erroneously
      try to read the commit value out of the source register
      after it has been destroyed.  If we ever have a machine
      with non-unit commit latencies, then we will have to
      change either copy propagation or the way values are
      committed.
   */
   assert (commit_latency == 1);
   update_reg_info (tip_commit, tip_op, reg, rename_reg, done_time,
		    tip_commit->vliw->time + commit_latency);

   *p_commit_latency = commit_latency;

   return tip_commit;
}

/************************************************************************
 *									*
 *				init_schedule_commit			*
 *				--------------------			*
 *									*
 * Set up the constant part of the commit ops, so they need not be	*
 * done every time we do a commit.					*
 *									*
 ************************************************************************/

init_schedule_commit ()
{
   int i;

   static unsigned char  ccr_f_w_expl    = BF  & (~OPERAND_BIT);
   static unsigned char  ccr_f_r_expl    = BFA & (~OPERAND_BIT);
   static unsigned char  ccr_b_w_expl    =  BT & (~OPERAND_BIT);
   static unsigned char  ccr_b_r_expl[2] = {BA & (~OPERAND_BIT),
					    BB & (~OPERAND_BIT)};

   static unsigned char  fpr_w_expl      = FRT & (~OPERAND_BIT);
   static unsigned char  fpr_r_expl      = FRB & (~OPERAND_BIT);
   static unsigned char  gpr_w_expl      =  RA & (~OPERAND_BIT);
   static unsigned char  gpr_r_expl[2]   = {RS & (~OPERAND_BIT),
					    RB & (~OPERAND_BIT)};

   commit_gpr_op.b		  = &ppc_opcode[OP_OR]->b;
   commit_gpr_op.op.num_wr	  = 1;
   commit_gpr_op.op.num_rd	  = 2;
   commit_gpr_op.op.wr_explicit   = &gpr_w_expl;
   commit_gpr_op.op.rd_explicit   = gpr_r_expl;
   commit_gpr_op.op.ins		  = 0x7C000378;

   commit_gpr_c_op.b		  = &ppc_opcode[OP_OR_C]->b;
   commit_gpr_c_op.op.num_wr	  = 2;
   commit_gpr_c_op.op.num_rd	  = 2;
   commit_gpr_c_op.op.wr_explicit = &gpr_w_expl;
   commit_gpr_c_op.op.rd_explicit = gpr_r_expl;
   commit_gpr_c_op.op.ins	  = 0x7C000378;

   commit_gpr_o_op.b		  = &ppc_opcode[OP_OR_O]->b;
   commit_gpr_o_op.op.num_wr	  = 3;
   commit_gpr_o_op.op.num_rd	  = 2;
   commit_gpr_o_op.op.wr_explicit = &gpr_w_expl;
   commit_gpr_o_op.op.rd_explicit = gpr_r_expl;
   commit_gpr_o_op.op.ins	  = 0x7C000378;

   commit_gpr_co_op.b		   = &ppc_opcode[OP_OR_CO]->b;
   commit_gpr_co_op.op.num_wr	   = 4;
   commit_gpr_co_op.op.num_rd	   = 2;
   commit_gpr_co_op.op.wr_explicit = &gpr_w_expl;
   commit_gpr_co_op.op.rd_explicit = gpr_r_expl;
   commit_gpr_co_op.op.ins	   = 0x7C000378;

   for (i = 0; i < NUM_FMR_TYPES; i++) {
      commit_fpr_op[i].op.num_wr      = 1;
      commit_fpr_op[i].op.num_rd      = 1;
      commit_fpr_op[i].op.wr_explicit = &fpr_w_expl;
      commit_fpr_op[i].op.rd_explicit = &fpr_r_expl;
      commit_fpr_op[i].op.ins	      = 0xFC000090;
   }

   /* Have no guarantee that OP_FMR, OP_FMR1, OP_FMR2, ... are consecutive */
   commit_fpr_op[0].b = &ppc_opcode[OP_FMR]->b;
   commit_fpr_op[1].b = &ppc_opcode[OP_FMR1]->b;
   commit_fpr_op[2].b = &ppc_opcode[OP_FMR2]->b;
   commit_fpr_op[3].b = &ppc_opcode[OP_FMR3]->b;
   commit_fpr_op[4].b = &ppc_opcode[OP_FMR4]->b;
   commit_fpr_op[5].b = &ppc_opcode[OP_FMR5]->b;

   commit_ccr_b_op.b		  = &ppc_opcode[OP_CROR]->b;
   commit_ccr_b_op.op.num_wr	  = 1;
   commit_ccr_b_op.op.num_rd	  = 2;
   commit_ccr_b_op.op.wr_explicit = &ccr_b_w_expl;
   commit_ccr_b_op.op.rd_explicit = ccr_b_r_expl;
   commit_ccr_b_op.op.ins	  = 0x4C000382;

   commit_ccr_f_op.b		  = &ppc_opcode[OP_MCRF]->b;
   commit_ccr_f_op.op.num_wr	  = 1;
   commit_ccr_f_op.op.num_rd	  = 1;
   commit_ccr_f_op.op.wr_explicit = &ccr_f_w_expl;
   commit_ccr_f_op.op.rd_explicit = &ccr_f_r_expl;
   commit_ccr_f_op.op.ins	  = 0x4C000000;

   init_commit_latencies ();
}

/************************************************************************
 *									*
 *				init_commit_latencies			*
 *				---------------------			*
 *									*
 * For simplicity, we assume that for GPR commits, that the time to	*
 * commit CA, OV, and SO to the XER is the same as the time to commit	*
 * to the integer register.  If this is not true, an assert fail	*
 * should occur, and the code can be fixed up to handle the more	*
 * general case.  (Basically, GPR commit latencies would have to be set	*
 * at each commit instruction, instead of once at program		*
 * initialization time.  Thus, we would lose a bit of speed in		*
 * executing the program.)						*
 *									*
 ************************************************************************/

init_commit_latencies ()
{
   int    i;
   int	  or_latency;
   int	  or_c_latency;
   int	  or_o_latency;
   int	  or_co_latency;
   int	  ca_latency;
   int	  ov_latency;
   int	  so_latency;
   int	  fmr_latency[NUM_FMR_TYPES];
   int	  cror_latency;
   int    mcrf_latency;

   /**************       GPR's      **************/
   assert (ppc_opcode[OP_OR]->op.num_wr    == 1);
   assert (ppc_opcode[OP_OR_C]->op.num_wr  == 2);
   assert (ppc_opcode[OP_OR_O]->op.num_wr  == 3);
   assert (ppc_opcode[OP_OR_CO]->op.num_wr == 4);

   or_latency    = ppc_opcode[OP_OR]->op.wr_lat[0];
   or_c_latency  = ppc_opcode[OP_OR_C]->op.wr_lat[0];
   or_o_latency  = ppc_opcode[OP_OR_O]->op.wr_lat[0];
   or_co_latency = ppc_opcode[OP_OR_CO]->op.wr_lat[0];
   assert (or_latency == or_c_latency == or_o_latency == or_co_latency);

   ca_latency = ppc_opcode[OP_OR_C]->op.wr_lat[1];
   assert (or_c_latency == ca_latency);

   ov_latency = ppc_opcode[OP_OR_CO]->op.wr_lat[1];
   so_latency = ppc_opcode[OP_OR_CO]->op.wr_lat[2];
   assert (or_o_latency == ov_latency == so_latency);

   ca_latency = ppc_opcode[OP_OR_CO]->op.wr_lat[1];
   ov_latency = ppc_opcode[OP_OR_CO]->op.wr_lat[2];
   so_latency = ppc_opcode[OP_OR_CO]->op.wr_lat[3];
   assert (or_co_latency == ca_latency == ov_latency == so_latency);

   for (i = 0; i < MAX_GPR_COMMITS; i++)
      commit_gpr_dest[i].latency = or_latency;

   /**************       FPR's      **************/
   assert (ppc_opcode[OP_FMR]->op.num_wr == 1);
   fmr_latency[0] = ppc_opcode[OP_FMR]->op.wr_lat[0];
   fmr_latency[1] = ppc_opcode[OP_FMR1]->op.wr_lat[0];
   fmr_latency[2] = ppc_opcode[OP_FMR2]->op.wr_lat[0];
   fmr_latency[3] = ppc_opcode[OP_FMR3]->op.wr_lat[0];
   fmr_latency[4] = ppc_opcode[OP_FMR4]->op.wr_lat[0];
   fmr_latency[5] = ppc_opcode[OP_FMR5]->op.wr_lat[0];
   for (i = 1; i < NUM_FMR_TYPES; i++)
      assert (fmr_latency[i] == fmr_latency[0]);

   chk_other_fmr_latencies (OP_FMR1, fmr_latency[0]);
   chk_other_fmr_latencies (OP_FMR2, fmr_latency[0]);
   chk_other_fmr_latencies (OP_FMR3, fmr_latency[0]);
   chk_other_fmr_latencies (OP_FMR4, fmr_latency[0]);
   chk_other_fmr_latencies (OP_FMR5, fmr_latency[0]);

   for (i = 0; i < MAX_FPR_COMMITS; i++)
      commit_fpr_dest[i].latency = fmr_latency[0];

   /**************     CCR Bits     **************/
   assert (ppc_opcode[OP_CROR]->op.num_wr == 1);
   cror_latency = ppc_opcode[OP_CROR]->op.wr_lat[0];
   for (i = 0; i < MAX_CCR_B_COMMITS; i++)
      commit_ccr_b_dest[i].latency = cror_latency;

   /************** CCR 4-bit Fields **************/
   assert (ppc_opcode[OP_MCRF]->op.num_wr == 1);
   mcrf_latency = ppc_opcode[OP_MCRF]->op.wr_lat[0];

   for (i = 0; i < MAX_CCR_F_COMMITS; i++)
      commit_ccr_f_dest[i].latency = mcrf_latency;
}

/************************************************************************
 *									*
 *				chk_other_fmr_latencies			*
 *				-----------------------			*
 *									*
 ************************************************************************/

chk_other_fmr_latencies (op_num, exp_latency)
int op_num;
int exp_latency;
{
   int i;
   int num_wr = ppc_opcode[op_num]->op.num_wr;

   for (i = 0; i < num_wr; i++)
      assert (ppc_opcode[op_num]->op.wr_lat[i] == exp_latency);
}

/************************************************************************
 *									*
 *				schedule_commit				*
 *				---------------				*
 *									*
 * Create a commit instruction to copy "src" (the renamed value) to	*
 * "dest", the architected PPC register.  Invoke "insert_op" in	*
 * (indirectly) mutually recursive fashion to find the earliest place	*
 * the commit can be placed.  (There may not be room in the current	*
 * last VLIW on this path, in which case, additional VLIW's must be	*
 * added.)								*
 *									*
 ************************************************************************/

TIP *schedule_commit (dest, src, ca_set, ov_set, tip, done_time, field_commit,
		      fmr_type)
int dest;
int src;
int ca_set;
int ov_set;
TIP *tip;
int done_time;
int field_commit;	/* Boolean:  Only for cond regs -> 4-bits, not 1 */
int fmr_type;		/* OP_FMR, OP_FMR1, OP_FMR2, ... */
{
   int	   earliest;
   OPCODE2 *commit_op;
   TIP     *rtn_tip;
   TIP     *dummy_tip;

   if      (is_gpr (dest)) {		/* Assumes commit is OR */
      assert (is_gpr (src));

      if      (ca_set)
	 if   (ov_set) commit_op = &commit_gpr_co_op;
         else          commit_op = &commit_gpr_c_op;
      else if (ov_set) commit_op = &commit_gpr_o_op;
      else	       commit_op = &commit_gpr_op;

      /* If fail assert, increase MAX_GPR_COMMITS.  
	 NOTE:  This assert also covers commit_op.op.rd.
      */
      commit_op->op.wr = pcommit_gpr_dest;
      commit_op->op.rd = pcommit_gpr_src;

      pcommit_gpr_dest += commit_op->op.num_wr;
      pcommit_gpr_src  += 2;
      assert (pcommit_gpr_dest < commit_gpr_dest + MAX_GPR_COMMITS);

      if      (ca_set)
	 if   (ov_set) { commit_op->op.wr[1].reg = CA;
		         commit_op->op.wr[2].reg = OV;
		         commit_op->op.wr[3].reg = SO; }
         else          { commit_op->op.wr[1].reg = CA; }
      else if (ov_set) { commit_op->op.wr[1].reg = OV;
		         commit_op->op.wr[2].reg = SO; }

      commit_op->op.renameable[RA & (~OPERAND_BIT)] = dest;
      commit_op->op.renameable[RS & (~OPERAND_BIT)] = src;
      commit_op->op.renameable[RB & (~OPERAND_BIT)] = src;
      commit_op->op.wr[0].reg = dest;
      commit_op->op.rd[0]     = src;
      commit_op->op.rd[1]     = src;
   }
   else if (is_fpr (dest)) {		/* Assumes commit is FMR */
      assert (is_fpr (src));

      /* If fail assert, increase MAX_FPR_COMMITS.  
	 NOTE:  This assert also covers commit_fpr_op.op.rd.
      */
      switch (fmr_type) {
	 case OP_FMR:	commit_op = &commit_fpr_op[0];  break;
	 case OP_FMR1:	commit_op = &commit_fpr_op[1];  break;
	 case OP_FMR2:	commit_op = &commit_fpr_op[2];  break;
	 case OP_FMR3:	commit_op = &commit_fpr_op[3];  break;
	 case OP_FMR4:	commit_op = &commit_fpr_op[4];  break;
	 case OP_FMR5:	commit_op = &commit_fpr_op[5];  break;
	 default:	assert (1 == 0);	        break;
      }

      commit_op->op.wr  = pcommit_fpr_dest;
      pcommit_fpr_dest += commit_op->op.num_wr;
      commit_op->op.rd  = pcommit_fpr_src++;
      assert (pcommit_fpr_src < commit_fpr_src + MAX_FPR_COMMITS);

      set_fp_commit_fpscr (commit_op, fmr_type);

      commit_op->op.renameable[FRT & (~OPERAND_BIT)] = dest;
      commit_op->op.renameable[FRB & (~OPERAND_BIT)] = src;
      commit_op->op.wr[0].reg = dest;
      commit_op->op.rd[0]     = src;
   }
   else if (is_ccbit (dest)) {		/* Assumes commit is CROR or MCRF */
      assert (is_ccbit (src));

      /* If fail assert, increase MAX_CCR_?_COMMITS.  
	 NOTE:  This assert also covers commit_ccr_op.op.rd.
      */
      if (field_commit) {

	 commit_op = &commit_ccr_f_op;
	 commit_op->op.wr = pcommit_ccr_f_dest++;
	 commit_op->op.rd = pcommit_ccr_f_src++;
         assert (commit_ccr_f_dest < commit_ccr_f_dest+MAX_CCR_F_COMMITS);

	 commit_ccr_f_op.op.renameable[BF  & (~OPERAND_BIT)] = dest;
	 commit_ccr_f_op.op.renameable[BFA & (~OPERAND_BIT)] = src;
	 commit_ccr_f_op.op.wr[0].reg = dest;
	 commit_ccr_f_op.op.rd[0]     = src;
      }
      else {				/* Committing 1-bit of CR */
	 commit_op = &commit_ccr_b_op;
	 commit_op->op.wr = pcommit_ccr_b_dest++;
	 commit_op->op.rd = pcommit_ccr_b_src;    pcommit_ccr_b_src += 2;
         assert (commit_ccr_b_dest < commit_ccr_b_dest+MAX_CCR_B_COMMITS);

	 commit_ccr_b_op.op.renameable[BT & (~OPERAND_BIT)] = dest;
	 commit_ccr_b_op.op.renameable[BA & (~OPERAND_BIT)] = src;
	 commit_ccr_b_op.op.renameable[BB & (~OPERAND_BIT)] = src;
	 commit_ccr_b_op.op.wr[0].reg = dest;
	 commit_ccr_b_op.op.rd[0]     = src;
	 commit_ccr_b_op.op.rd[1]     = src;
      }
   }
   else assert (1 == 0);

   /* Insert the commit.  Note that this is mutually recursive.  We
      stop because the time for the commit is the time of "tip" or
      the time at which the result is available to commit, whichever
      is later.  Consequently no renaming will be done, and we
      will not be reinvoked.  

      NOTE:  Commit time can be later than that of the current tip if
             the machine has latencies greater than one.
   */
   if (tip->vliw->time < done_time) earliest = done_time;
   else				    earliest = tip->vliw->time;

   doing_commit = TRUE;
   rtn_tip = insert_op (commit_op, tip, earliest, &dummy_tip);
   doing_commit = FALSE;

   assert (rtn_tip->num_ops >= 1);
   rtn_tip->op->commit = TRUE;

   return rtn_tip;
}

/************************************************************************
 *									*
 *				set_fp_commit_fpscr			*
 *				-------------------			*
 *									*
 * Put in the "wr" field of "commit_op" all the bits that this type	*
 * of fp commit sets in the FPSCR.					*
 *									*
 ************************************************************************/

set_fp_commit_fpscr (commit_op, fmr_type)
OPCODE2 *commit_op;
int	fmr_type;
{
   int	    i;
   int	    op_num;
   int	    num_wr;
   unsigned *wr;

   num_wr = ppc_opcode[fmr_type]->op.num_wr;
   wr     = ppc_opcode[fmr_type]->op.wr;

   /* The template has all the real destinations except the first (FRT), */
   /* which is symbolic.						 */
   for (i = 1; i < num_wr; i++)
      commit_op->op.wr[i].reg = wr[i];
}

/************************************************************************
 *									*
 *				make_tip_list				*
 *				-------------				*
 *									*
 * Roughly speaking, put in "list", the set of tips encountered when	*
 * moving up the VLIW chain from "start_tip".  Actually, only the first	*
 * tip encountered in a VLIW is placed in "list".   This first tip in	*
 * a VLIW is a leaf tip since we are moving up the chain.  Since the	*
 * caller normally wants the list of tips ordered from "earliest" to	*
 * "latest", tips are placed in "list" in this order.			*
 *									*
 * NOTE 1:  The "start_tip" and any tips in its VLIW are not placed in	*
 *	    the list.							*
 *									*
 * NOTE 2:  The caller is responsible for insuring that "list" is large	*
 *	    enough to hold all of the elements.				*
 *									*
 ************************************************************************/

make_tip_list (start_tip, earliest, latest, list)
TIP *start_tip;
int earliest;
int latest;
TIP **list;
{
   int  index = latest - earliest;
   TIP  *tip;
   VLIW *prev_vliw = start_tip->vliw;

   for (tip = start_tip;  tip;  tip = tip->prev) {
      if (tip->vliw->time < earliest) break;
      else if (tip->vliw != prev_vliw) {
	 list[index--] = tip;
	 prev_vliw = tip->vliw;
      }
   }
}

/************************************************************************
 *									*
 *				get_tip_at_time				*
 *				---------------				*
 *									*
 * Traverse up the tree from "start_tip" until a tip is reached whose	*
 * VLIW has time "time".  Return that tip.  If no such tip is found,	*
 * return 0.								*
 *									*
 ************************************************************************/

TIP *get_tip_at_time (start_tip, time)
TIP *start_tip;
int time;
{
   TIP  *tip;
   TIP  *rtn_tip   = start_tip;
   VLIW *prev_vliw = start_tip->vliw;

   for (tip = start_tip;  tip;  tip = tip->prev) {
      if (tip->vliw->time < time) break;
      else if (tip->vliw != prev_vliw) {
	 prev_vliw = tip->vliw;
         rtn_tip   = tip;
      }
   }

   if (rtn_tip->vliw->time == time) return rtn_tip;
   else				    return 0;
}

/************************************************************************
 *									*
 *				append_new_vliw				*
 *				---------------				*
 *									*
 * Create a new VLIW and branch to it from "tip".  Assign this new	*
 * VLIW time "time".  Copy the map, avail, and group information from	*
 * the VLIW of "tip".  Update the map and avail information with any	*
 * updates between the entry of the VLIW and "tip".			*
 *									*
 ************************************************************************/

VLIW *append_new_vliw (tip, time)
TIP *tip;
int time;
{
   int		 i;
   TIP		 *entry;
   VLIW		 *vliw;
   VLIW		 *orig_vliw = tip->vliw;
   unsigned char *pred_tip_update_cnt;
   short	 *pred_vliw_update_cnt;
   short	 *succ_vliw_update_cnt;

   vliw = get_vliw ();

   vliw->time = time;
   vliw->group = orig_vliw->group;

   entry = vliw->entry;
   entry->prev     = tip;
   entry->prob     = tip->prob;
   entry->tp	   = tip->tp;
   tip->left       = 0;				/* Signals Leaf */
   tip->right      = entry;
   tip->br_ins     = 0x48000000;		/* b UNCOND */
   tip->br_type    = BR_ONPAGE;

   set_tip_regs (tip, entry, FALSE);

   pred_tip_update_cnt  = tip->mem_update_cnt;
   pred_vliw_update_cnt = tip->vliw->mem_update_cnt;
   succ_vliw_update_cnt = entry->vliw->mem_update_cnt;
   for (i = 0; i < NUM_PPC_GPRS_C; i++)
      succ_vliw_update_cnt[i] = pred_vliw_update_cnt[i] +
				pred_tip_update_cnt[i];

   return vliw;
}

/************************************************************************
 *									*
 *				set_tip_regs				*
 *				------------				*
 *									*
 * Set (1) all renaming registers available for use, (2) "avail" info,	*
 * (4) "mapping" info based on going from "pred_tip" to "succ_tip", and	*
 * (5) which instruction wrote which PPC GPR.				*
 *									*
 ************************************************************************/

set_tip_regs (pred_tip, succ_tip, cond_br_targ)
TIP *pred_tip;
TIP *succ_tip;
int cond_br_targ;		/* Boolean */
{
   int i;

   /* (1) All non-PPC-architected registers available for renaming */
   init_reg_avail (succ_tip);

   succ_tip->last_store  = pred_tip->last_store;
   succ_tip->stm_r1_time = pred_tip->stm_r1_time;
   succ_tip->ca_rename   = pred_tip->ca_rename;
#ifdef ACCESS_RENAMED_OV_RESULT
   succ_tip->ov_rename   = pred_tip->ov_rename;
#endif

   /* (2) Copy "avail" info from "pred_tip" to "succ_tip" */
   /* (3) Copy register mapping info from "pred_tip" to "succ_tip" */
   /* (4) The writers of each GPR are initially the same as for predecessor */
   if (!cond_br_targ) {
      succ_tip->avail      = pred_tip->avail;
      succ_tip->gpr_rename = pred_tip->gpr_rename;
      succ_tip->fpr_rename = pred_tip->fpr_rename;
      succ_tip->ccr_rename = pred_tip->ccr_rename;
      succ_tip->stores     = pred_tip->stores;
      succ_tip->gpr_writer = pred_tip->gpr_writer;
      succ_tip->loophdr    = pred_tip->loophdr;
      succ_tip->cluster    = pred_tip->cluster;
   }
   else {
      assert (avail_mem_cnt < MAX_PATHS_PER_PAGE);

      /* Do availability of machine resources */
      succ_tip->avail = avail_mem[avail_mem_cnt];
      mem_copy (succ_tip->avail, pred_tip->avail, 
		NUM_MACHINE_REGS * sizeof (pred_tip->avail[0]));

      /* Do register and mapping and STORES */
      succ_tip->gpr_rename = map_mem[avail_mem_cnt];
      succ_tip->fpr_rename = succ_tip->gpr_rename + NUM_PPC_GPRS_C;
      succ_tip->ccr_rename = succ_tip->fpr_rename + NUM_PPC_FPRS;
      succ_tip->stores     = stores_mem[avail_mem_cnt];
      succ_tip->loophdr    = (char *) get_loophdr_mem ();
      succ_tip->cluster    = (char *) get_cluster_mem ();

      mem_copy (succ_tip->gpr_rename, pred_tip->gpr_rename, 
		NUM_PPC_GPRS_C * sizeof (pred_tip->gpr_rename[0]));

      mem_copy (succ_tip->fpr_rename, pred_tip->fpr_rename, 
		NUM_PPC_FPRS   * sizeof (pred_tip->fpr_rename[0]));

      mem_copy (succ_tip->ccr_rename, pred_tip->ccr_rename, 
		NUM_PPC_CCRS   * sizeof (pred_tip->ccr_rename[0]));

      mem_copy (succ_tip->stores, pred_tip->stores,
		NUM_MA_HASHVALS * sizeof (pred_tip->stores[0]));

      mem_copy (succ_tip->loophdr, pred_tip->loophdr,
		loophdr_bytes_per_path);

      mem_copy (succ_tip->cluster, pred_tip->cluster,
		NUM_MACHINE_REGS * sizeof (pred_tip->cluster[0]));

      /* Do GPR writers */
      succ_tip->gpr_writer = gpr_wr_mem[avail_mem_cnt];
      mem_copy (succ_tip->gpr_writer, pred_tip->gpr_writer,
		NUM_PPC_GPRS_C * sizeof (succ_tip->gpr_writer[0]));

      avail_mem_cnt++;
   }
}

/************************************************************************
 *									*
 *				update_reg_info				*
 *				---------------				*
 *									*
 * Update the "commit", "avail", and "map" info for the registers in	*
 * the system.  Do so along the path up the tree from "tip_commit" to	*
 * "tip_op".								*
 *									*
 ************************************************************************/

update_reg_info (tip_commit, tip_op, reg, rename_reg, done_time, commit_avail)
TIP *tip_commit;
TIP *tip_op;
int reg;
int rename_reg;
int done_time;
int commit_avail;
{
   int	      index;
   TIP	      *tip;
   REG_RENAME *commit_info;

   for (tip = tip_commit; ; tip = tip->prev) {

      /* The rename register is in use [tip_op, tip_commit), i.e. it
	 cannot be used again in tip_op, but it can be used in tip_commit.
      */
      if (tip != tip_commit) {
	 if      (is_gpr   (reg)) clr_bit (tip->gpr_vliw, rename_reg-R0);
	 else if (is_fpr   (reg)) clr_bit (tip->fpr_vliw, rename_reg-FP0);
	 else if (is_ccbit (reg)) clr_bit (tip->ccr_vliw, rename_reg-CR0);
	 else assert (1 == 0);
      }

      if (tip == tip_op) break;
   }

   /* Setup availability and commit info for the actual commit of the
      current operation to "reg".
   */
   if (tip_commit != tip_op) tip_commit->avail[rename_reg] = done_time;
   tip_commit->avail[reg] = done_time;

   if      (is_gpr   (reg)) {
      if (reg < R0 + NUM_PPC_GPRS_C) {
	 /* Is result renamed and committed? */
	 if (rename_reg < 0)
	    set_mapping (tip_commit, reg,        reg, commit_avail, TRUE);
	 else {
	    set_mapping (tip_commit, rename_reg, reg, done_time,    TRUE);
	    set_mapping (tip_commit, reg,	 reg, commit_avail, FALSE);
	 }

	 if      (tip_commit->gpr_writer[reg] == (unsigned *) NO_GPR_WRITERS)
	    tip_commit->gpr_writer[reg] = curr_ins_addr;
	 else if (tip_commit->gpr_writer[reg] != curr_ins_addr)
	    tip_commit->gpr_writer[reg] = (unsigned *) MULT_GPR_WRITERS;
      }
   }
   else if (is_fpr   (reg)) {
      assert (reg < FP0 + NUM_PPC_FPRS);
      /* Is result renamed and committed? */
      if (rename_reg < 0)
	 set_mapping (tip_commit, reg,	      reg, commit_avail, TRUE);
      else {
	 set_mapping (tip_commit, rename_reg, reg, done_time,    TRUE);
	 set_mapping (tip_commit, reg,        reg, commit_avail, FALSE);
      }
   }
   else if (is_ccbit (reg)) {
      /* If convert BCT to branch on cond-code, cond-code is in non-arch reg */
      if (reg < CR0 + NUM_PPC_CCRS) {
	 /* Is result renamed and committed? */
	 if (rename_reg < 0)
	    set_mapping (tip_commit, reg,        reg, commit_avail, TRUE);
	 else {
	    set_mapping (tip_commit, rename_reg, reg, done_time,    TRUE);
	    set_mapping (tip_commit, reg,	 reg, commit_avail, FALSE);
	 }
      }
   }
}

/************************************************************************
 *									*
 *				update_reg_info2			*
 *				----------------			*
 *									*
 * Update the "avail" times, and the availibility for use of		*
 * non-architected VLIW registers.  This is normally called for		*
 * synthetic ops (such as from "brlink.c") which use only		*
 * non-architected VLIW registers.  Do so along the path up the tree	*
 * from "tip_commit" to "tip_op".					*
 *									*
 ************************************************************************/

update_reg_info2 (tip_commit, tip_op, reg, done_time)
TIP *tip_commit;
TIP *tip_op;
int reg;
int done_time;
{
   int index;
   TIP *tip;

   for (tip = tip_commit; ; tip = tip->prev) {

      /* The "reg" is in use [tip_op, tip_commit), i.e. it cannot 
	 be used again in tip_op, but it can be used in tip_commit.
      */
      if (tip != tip_commit) {
	 if      (is_gpr   (reg)) clr_bit (tip->gpr_vliw, reg-R0);
	 else if (is_fpr   (reg)) clr_bit (tip->fpr_vliw, reg-FP0);
	 else if (is_ccbit (reg)) clr_bit (tip->ccr_vliw, reg-CR0);
	 else assert (1 == 0);
      }

      /* At "tip_op", the "reg" still refers to whatever old value
	 it had.  Thereafter, its availability is the time at which the
	 current op finishes and writes its result to "reg".
      */
      if (tip == tip_op) break;
   }

   tip_commit->avail[reg] = done_time;
}

/************************************************************************
 *									*
 *				update_reg_info3			*
 *				----------------			*
 *									*
 * Update the "avail" times, and the availibility for use of		*
 * non-architected VLIW registers.  This is normally called from	*
 * "update_ccbit_for_bct" in "bct.c" to indicate the usage of the	*
 * destination condition register field.				*
 *									*
 ************************************************************************/

update_reg_info3 (tip_commit, tip_op, cr_field, done_time)
TIP *tip_commit;
TIP *tip_op;
int cr_field;
int done_time;
{
   int index;
   TIP *tip;

   assert (is_ccbit (cr_field));

   for (tip = tip_commit; ; tip = tip->prev) {
      if (num_ccbits <= 64)
	 clr_bit_range_word (tip->ccr_vliw, cr_field-CR0, cr_field-CR0+3);
      else clr_bit_range    (tip->ccr_vliw, cr_field-CR0, cr_field-CR0+3);

      if (tip == tip_op) break;
   }

   tip_commit->avail[cr_field] = done_time;
}

/************************************************************************
 *									*
 *				is_tip_pred				*
 *				-----------				*
 *									*
 * Returns:  TRUE if "qtip" is predecessor (not necessarily immediate)	*
 *	     of qtip, o.w.  FALSE					*
 *									*
 ************************************************************************/

int is_tip_pred (qtip, curr_tip)
TIP *qtip;
TIP *curr_tip;
{
   int  start_group = curr_tip->vliw->group;
   TIP *tip	    = curr_tip;

   do {
      assert (tip->vliw->group == start_group);

      if ((tip = tip->prev) == qtip) return TRUE;
   } while (tip);

   return FALSE;
}

#ifdef USE_REG_CLASSIFY_FUNCS

/************************************************************************
 *									*
 *				is_gpr					*
 *				------					*
 *									*
 ************************************************************************/

int is_gpr (reg)
int reg;
{
   if (reg >= R0  &&  reg < R0 + num_gprs) return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *				is_fpr					*
 *				------					*
 *									*
 ************************************************************************/

int is_fpr (reg)
int reg;
{
   if (reg >= FP0  &&  reg < FP0 + num_fprs) return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *				is_ccbit				*
 *				--------				*
 *									*
 ************************************************************************/

int is_ccbit (reg)
int reg;
{
   if (reg >= CR0  &&  reg < CR0 + num_ccbits) return TRUE;
   else return FALSE;
}
#endif

/************************************************************************
 *									*
 *				get_base_op				*
 *				-----------				*
 *									*
 ************************************************************************/

BASE_OP *get_base_op ()
{
   BASE_OP *base_op;

   base_op = &base_op_mem[base_op_cnt++];
   assert (base_op_cnt <= MAX_BASE_OPS_PER_PAGE);

   return base_op;
}

/************************************************************************
 *									*
 *				alloc_vliw_op				*
 *				-------------				*
 *									*
 ************************************************************************/

OPCODE2 *alloc_vliw_op (ppc_op, do_renaming, ca_explicit)
OPCODE2 *ppc_op;
int	do_renaming;
int     ca_explicit;	   /* TRUE if AE, SFE, etc and using CA in GPR */
{
   int	       i;
   int	       r_num_rd;
   int	       v_opnum;
   OPCODE2     *vliw_op;
   OPCODE_BASE *v_b;
   OPERAND_OUT *v_operand;
   OPERAND_OUT *p_operand;

   vliw_op	         = &vliw_ops_mem[vliw_ops_cnt++];
   assert (vliw_ops_cnt <= MAX_BASE_OPS_PER_PAGE);

   v_operand = &vliw_op->op;
   p_operand = &ppc_op->op;

   r_num_rd      = p_operand->num_rd;
   vliw_op->b    = ppc_op->b;
   *v_operand    = *p_operand;
   v_operand->rd = ptr_rd;
   ptr_rd       += r_num_rd;

   if (p_operand->orig_addr == 0) 
      v_operand->orig_addr = curr_ins_addr;

   /* If we get the CA from something other than the XER, we must
      make that something explicit in AE, SFE, SFZE, etc ops.
      That is we must use a special form of those instructions
      with an additional operand.  The additional operand requires
      an additional op slot in the VLIW.
   */
   if (ca_explicit) {
      v_opnum    = get_ca_ext_opcode (ppc_op->b->op_num);
      vliw_op->b = &ppc_opcode[v_opnum]->b;

      /* Make the reading of CA explicit */     
      for (i = 0; i < r_num_rd; i++)
         if (p_operand->rd[i] == CA) break;

      /* CA must be one of the inputs to this instruction */
      assert (i < r_num_rd);
      v_operand->rd_explicit[i] = GCA;
      v_operand->renameable[GCA & (~OPERAND_BIT)] = CA;
   }

   if (do_renaming) {
      v_operand->wr = ptr_wr;
      ptr_wr	   += p_operand->num_wr;
   }

   assert (ptr_rd <= &rd_area[MAX_PAGE_USE_RESRC]);
   assert (ptr_wr <= &wr_area[MAX_PAGE_MOD_RESRC]);

   return vliw_op;
}

/************************************************************************
 *									*
 *				is_ca_input_explicit			*
 *				--------------------			*
 *									*
 * Returns TRUE if the CA bit is available in a GPR extender at the	*
 *	   time of "tip_op", or FALSE if CA should be obtained from	*
 *	   the XER.							*
 *									*
 ************************************************************************/

int is_ca_input_explicit (tip_op, tip_commit)
TIP *tip_op;
TIP *tip_commit;
{
   if (!tip_commit->ca_rename.prev)				return FALSE;
   else if (tip_op->vliw->time  >=  tip_commit->ca_rename.time) return FALSE;
   else								return TRUE;
}
