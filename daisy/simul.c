/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#pragma alloca

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "dis.h"
#include "dis_tbl.h"
#include "stats.h"
#include "resrc_map.h"
#include "resrc_offset.h"
#include "hash.h"
#include "rd_config.h"
#include "gen_resrc.h"
#include "cache.h"
#include "debug.h"
#include "simul.h"

/* Max # of VLIW paths in translated program.  If too small, incr this val */
#define MAX_PATHS			0x100000	/*    1 Mbyte */

#define MAX_XLATE_PAIRS_PER_PAGE	16384

/* Max # of instrucs needed to regroup after SVC call.  See "xlate_svc_v2p" */
#define NUM_SVC_RTN_INS			64

#define SIMUL_MEM_SIZE			0x10000000
#define MAX_TOTAL_GROUPS		0x20000

typedef struct {
   TIP	    *tip;
   unsigned *br_addr;
} XLPAIR;

typedef struct {
   unsigned short skip_ops;
   unsigned short other_ops;
   OPCODE2	  *ops[MAX_OPS_PER_VLIW];
} SPEC_OPS;

static XLPAIR   xlate_pair[MAX_XLATE_PAIRS_PER_PAGE];
static unsigned xlate_pair_cnt;
static unsigned page_simul_code_size;

       unsigned	 total_vliw_ops;
       unsigned  curr_spec_size;
       unsigned  curr_spec_addr;

/* Leave space (+3) for branch back to start via CTR if reuse simul mem */
/*     unsigned    xlate_mem[XLATE_MEM_WORDS+3];*/
       unsigned    *xlate_mem;
       unsigned    *xlate_mem_ptr;
       unsigned    *xlate_mem_end;
       unsigned	   consec_ins_needed;

extern int	 num_gprs;
extern int	 num_fprs;
extern int	 num_ccbits;
extern int	 bytes_per_vliw;
extern int	 num_ins_in_loadver_fail_simgen;
extern int	 recompiling;

extern OPCODE1	 illegal_opcode;
extern RESRC_MAP *resrc_map[];
extern unsigned  *prog_start;
extern unsigned  *prog_end;
extern char	 *info_dir;
extern char	 *daisy_progname;
extern int	 trace_xlated_pgm;
extern int	 trace_all_br;		/* Both branch-and-link and non */
extern int	 trace_lib;		/* Trace lib fcns or only user code */
extern int	 do_cache_simul;
extern int	 do_cache_trace;
extern int	 max_dyn_pages_action;
extern unsigned  max_dyn_page_cnt;

       unsigned  total_groups;
       unsigned  group_cnt[MAX_TOTAL_GROUPS];

       unsigned  perf_cnts[MAX_PATHS];
static unsigned  path_num;	/* For globally numbering all paths */
static unsigned  vliw_num;	/* For globally numbering all VLIWs */
static unsigned	 group_entry_vliw_num;
static VLIW	 *group_entry_vliw;

static unsigned  trace_br_upper;
static unsigned  trace_br_lower;
static unsigned  find_data_upper;
static unsigned  find_data_lower;
static unsigned  find_ins_upper;
static unsigned  find_ins_lower;
static unsigned  num_ins_in_xlate_offpage_indir    = 26;
static unsigned  num_ins_in_xlate_offpage_cnt_code = 10;
static unsigned  vliw_addr;
static unsigned	 full_trips_thru_simul_mem;
static unsigned	 page_simul_code_size;
static unsigned  *xl_br_addr;
static unsigned  svc_rtn_code[NUM_SVC_RTN_INS];

void	 handle_illegal_raw    (unsigned, unsigned);
void	 hit_entry_pt_lim1_raw (void);
void	 hit_entry_pt_lim2_raw (void);
void     xlate_entry_raw       (void);
void	 trace_br_raw    (void);
void	 find_data_raw   (void);
void	 find_instr_raw  (void);
unsigned *xlate_skip_v2p (TIP *);
unsigned *dump		 (unsigned);
unsigned *bump_group_cnt (unsigned *);

/************************************************************************
 *									*
 *				init_simul				*
 *				----------				*
 *									*
 ************************************************************************/

init_simul ()
{
   PTRGL *trace_br   = (PTRGL *) trace_br_raw;
   PTRGL *pfind_data = (PTRGL *) find_data_raw;
   PTRGL *pfind_ins  = (PTRGL *) find_instr_raw;

   instr_init	       (daisy_progname, info_dir);
   init_cnts_and_timer (daisy_progname, &path_num, perf_cnts);

   if (do_cache_simul) init_cache_simul (daisy_progname);
   if (do_cache_trace) trace_init       (daisy_progname);

   if (do_cache_simul  ||  do_cache_trace) {
      find_data_upper = ((unsigned) pfind_data->func) >> 16;
      find_data_lower = ((unsigned) pfind_data->func)  & 0xFFFF;
      find_ins_upper  = ((unsigned) pfind_ins->func)  >> 16;
      find_ins_lower  = ((unsigned) pfind_ins->func)   & 0xFFFF;
   }

   trace_br_upper = ((unsigned) trace_br->func) >> 16;
   trace_br_lower = ((unsigned) trace_br->func)  & 0xFFFF;

   xlate_mem = get_obj_mem (SIMUL_MEM_SIZE);

   vliw_addr = 0;
   full_trips_thru_simul_mem = 0;
   consec_ins_needed = 1;
   xlate_mem_end = &xlate_mem[XLATE_MEM_WORDS];
}

/************************************************************************
 *									*
 *			add_vliw_xlate_pair				*
 *			-------------------				*
 *									*
 * Add the (tip, xl_addr) pair to the list of VLIW / PPC translation	*
 * points.  There should be one such pair for each direct branch from	*
 * one VLIW to another.							*
 *									*
 ************************************************************************/

add_vliw_xlate_pair (tip, xl_addr)
TIP *tip;
unsigned *xl_addr;
{
   /* If assert fails, increase MAX_XLATE_PAIRS_PER_PAGE */
   assert (xlate_pair_cnt < MAX_XLATE_PAIRS_PER_PAGE);

   xlate_pair[xlate_pair_cnt].tip     = tip;
   xlate_pair[xlate_pair_cnt].br_addr = xl_addr;

   xlate_pair_cnt++;
}

/************************************************************************
 *									*
 *			patch_vliw_xlate_pairs				*
 *			----------------------				*
 *									*
 * Patch VLIW tip branches to point to the translated code for the	*
 * target VLIW.								*
 *									*
 ************************************************************************/

patch_vliw_xlate_pairs ()
{
   int	    offset;
   unsigned i;
   unsigned *targ_addr = -1;
   TIP	    *targ_tip;
   XLPAIR  *xp;

   for (i = 0; i < xlate_pair_cnt; i++) {
      xp = &xlate_pair[i];
      offset = ((char *) xp->tip->vliw->xlate) - ((char *) xp->br_addr);

      /* Make sure within range, and that keeping on 4-byte boundary */
      assert ((offset & 0x00000003) == 0 &&
	     ((offset & 0xFC000000) == 0 ||
	      (offset & 0xFC000000) == 0xFC000000));

      /* Clear upper 6 bits, lower 2 bits of offset before putting in BR ins */
      offset &= 0x03FFFFFC;

      *(xp->br_addr) |= offset;
   }

   /* Reset the count for the next page */
   xlate_pair_cnt = 0;
   page_simul_code_size = 0;
}

/************************************************************************
 *									*
 *				vliw_to_ppc_group			*
 *				------------------			*
 *									*
 * Translate the tree of VLIW instructions starting with "vliw" into	*
 * PPC simulation code.  The passed "vliw" normally corresponds to the	*
 * entry point encountered for the current page.  Translation proceeds	*
 * in each path of the tree of VLIW instructions until a tip branch	*
 * (i.e an unconditional branch) is encountered.			*
 *									*
 ************************************************************************/

vliw_to_ppc_group (vliw, group)
VLIW *vliw;
int  group;
{
   int      i, j;
   int	    num_vliws;
   VLIW     *vliw_list[MAX_VLIWS_PER_PAGE];
   SPEC_OPS spec_ops;
   unsigned curr_vliw_num; 
   unsigned *bump_addr;
   OPCODE2  *path_ops[MAX_OPS_PER_VLIW];
   char	    name[16];
   unsigned gpr_shad_vec[2]   = {0, 0};
   unsigned fpr_shad_vec[2]   = {0, 0};
   unsigned ccr_shad_vec[2]   = {0, 0};
   unsigned xer_shad_vec[2]   = {0, 0};	      /* Specul XER   with each GPR */
   unsigned fpscr_shad_vec[2] = {0, 0};	      /* Specul FPSCR with each FPR */

   assert (num_gprs   == 64);	     /* If more, gpr_shad_vec must increase */
   assert (num_fprs   == 64);	     /* If more, fpr_shad_vec must increase */
   assert (num_ccbits == 64);	     /* If more, ccr_shad_vec must increase */

   assert (total_groups < MAX_TOTAL_GROUPS);
   bump_addr = bump_group_cnt (&group_cnt[total_groups]);
   total_groups++;
   group_entry_vliw     = vliw;
   group_entry_vliw_num = vliw_num + 1;

   num_vliws = make_vliw_list (vliw, vliw_list, MAX_VLIWS_PER_PAGE, group);

   for (i = 0; i < num_vliws; i++) {

      /* If this assert fails, then XLATE_MEM_WORDS should be increased
         to allow larger programs (with more code/text) to be translated.
	 --THIS USED TO BE THE CASE.  Now that we can reuse simulation
         memory (see the "dump ()" routine), failure indicates a more
         serious problem.
      */
      assert (xlate_mem_ptr < xlate_mem + XLATE_MEM_WORDS);
      if (!recompiling) assert (vliw_list[i]->xlate == 0);

      if (xlate_mem_ptr < xlate_mem_end) {
	 if (i == 0) vliw_list[i]->xlate = bump_addr;
	 else        vliw_list[i]->xlate = xlate_mem_ptr;
      }
      else				 vliw_list[i]->xlate = xlate_mem;

      spec_ops.skip_ops   = 0;
      spec_ops.other_ops  = 0;
      curr_vliw_num       = vliw_num++;

      if (do_cache_simul  ||  do_cache_trace)
	 ins_cache_ref (vliw_addr, find_ins_upper, find_ins_lower);

      tip_to_ppc (vliw_list[i]->entry, curr_vliw_num, 0, &spec_ops, path_ops,
		  0, 0, path_num, group,
		  gpr_shad_vec, fpr_shad_vec, ccr_shad_vec, 
		  xer_shad_vec, fpscr_shad_vec);

      /* Should "curr_spec_size" be rounded up to a multiple of 32 or 64? */
      curr_spec_size = 4 * ((2 * spec_ops.skip_ops + 1) + spec_ops.other_ops);
      curr_spec_addr += curr_spec_size;		/* After round up if done */
      vliw_addr      += bytes_per_vliw;

      sprintf (name, "V%u", curr_vliw_num+1);
      save_specified_ins (name, curr_vliw_num, spec_ops.ops,
			  spec_ops.other_ops, spec_ops.skip_ops);
   }
}

/************************************************************************
 *									*
 *				tip_to_ppc				*
 *				----------				*
 *									*
 ************************************************************************/

tip_to_ppc (tip, vliw_index, depth, spec_ops, path_ops, path_cnt, orig_ops,
	    path1_of_vliw, group,
	    gpr_shad_vec_in, fpr_shad_vec_in, ccr_shad_vec_in, 
	    xer_shad_vec_in, fpscr_shad_vec_in)
TIP	 *tip;
unsigned vliw_index;
int	 depth;			/* How many skips before this tip ? */
SPEC_OPS *spec_ops;
OPCODE2  **path_ops;
int	 path_cnt;
int	 orig_ops;
unsigned path1_of_vliw;		/* So can count num paths in each VLIW */
int	 group;
unsigned gpr_shad_vec_in[2];	/* 1 bit per GPR => 1 if latest in shad */
unsigned fpr_shad_vec_in[2];	/* 1 bit per FPR => 1 if latest in shad */
unsigned ccr_shad_vec_in[2];	/* 1 bit per CCR => 1 if latest in shad */
unsigned xer_shad_vec_in[2];	/* 1 bit per GPR/XER => 1 if latest in shad */
unsigned fpscr_shad_vec_in[2];	/* 1 bit per FPR/FPSCR =>1 if latest in shad */
{
   int	    i;
   int	    index;
   int	    spec_cnt;
   int	    reg_wr;
   int	    had_illegal = FALSE;
   int	    mq_write;
   int	    num_cycles;
   unsigned curr_path_num;
   unsigned gpr_shad_vec[2];
   unsigned fpr_shad_vec[2];
   unsigned ccr_shad_vec[2];
   unsigned xer_shad_vec[2];		/* Specul XER   with each GPR */
   unsigned fpscr_shad_vec[2];		/* Specul FPSCR with each FPR */
   unsigned *patch_addr;
   BASE_OP  *bop;
   OPCODE2  *op;
   int      commit_op[MAX_OPS_PER_VLIW];	/* Boolean Array */

   assert (num_gprs == 64);	      /* If more, gpr_shad_vec must increase */
   gpr_shad_vec[0] = gpr_shad_vec_in[0];
   gpr_shad_vec[1] = gpr_shad_vec_in[1];
   xer_shad_vec[0] = xer_shad_vec_in[0];
   xer_shad_vec[1] = xer_shad_vec_in[1];

   assert (num_fprs == 64);	      /* If more, fpr_shad_vec must increase */
   fpr_shad_vec[0]   =   fpr_shad_vec_in[0];
   fpr_shad_vec[1]   =   fpr_shad_vec_in[1];
   fpscr_shad_vec[0] = fpscr_shad_vec_in[0];
   fpscr_shad_vec[1] = fpscr_shad_vec_in[1];

   assert (num_ccbits == 64);	      /* If more, ccr_shad_vec must increase */
   ccr_shad_vec[0] = ccr_shad_vec_in[0];
   ccr_shad_vec[1] = ccr_shad_vec_in[1];

   assert (tip->vliw->group == group);

   total_vliw_ops += tip->num_ops + 1;	/* The 1 is for the tip branch */
   orig_ops += tip->orig_ops;
   spec_cnt = spec_ops->other_ops;
   bop = tip->op;
   for (i = 0; i < tip->num_ops; i++, path_cnt++, bop = bop->next) {
      op = bop->op;
      spec_ops->ops[spec_cnt++] = op;
      path_ops[path_cnt]        = op;
      commit_op[path_cnt]       = bop->commit;
   }
   spec_ops->other_ops = spec_cnt;

   /* The ops in the list were in reverse order.  Output code in fwd order */
   for (i = 0, index = path_cnt-1; i < tip->num_ops; i++, index--) {
      op = path_ops[index];

      /* If in illegal opcodes, programmer must have known that conditional
	 branch would always be taken.  Stop since this would lead to
	 garbage anyway and may confuse us in the process.  The
	 "xlate_illegal_p2v" sets up the "page" field to hold the original
	 address of the offending instruction.
      */
      if      (op->b->illegal) {
	 xlate_illegal_v2p  (op->b->page, op->op.ins);
	 had_illegal = TRUE;
      }
      else if (is_string_indir (op))   xlate_string_indir (op);
      else {
	 reg_wr = xlate_opcode_v2p (op, ccr_shad_vec, xer_shad_vec, 
				    fpscr_shad_vec, &mq_write);

         if (op->op.verif)
	    load_verify_v2p (op, gpr_shad_vec, fpr_shad_vec, ccr_shad_vec,
			         xer_shad_vec, fpscr_shad_vec);
	 else {
	    if (reg_wr >= 0) update_shad_writes (gpr_shad_vec, fpr_shad_vec,
						 ccr_shad_vec, reg_wr);

	    if (mq_write)    update_shad_writes (gpr_shad_vec, fpr_shad_vec,
						 ccr_shad_vec, MQ);
	 }
      }
   }

   /* Check if we are tracing branches, and if so, dump appropriate code */
   dump_simul_trace_code (tip, trace_br_upper, trace_br_lower);

   if (tip->br_type != BR_ONPAGE) {

      if (!had_illegal) 
         assert (tip->left == 0); /* Non (DIRECT/ONPAGE) branches are leaves */

      wr_shad_gprs   (gpr_shad_vec);
      wr_shad_fprs   (fpr_shad_vec);
      wr_shad_ccrs   (ccr_shad_vec);
      wr_shad_xers   (xer_shad_vec);
      wr_shad_fpscrs (fpscr_shad_vec);
      curr_path_num = path_num++;
      instr_bump_leaf_cnt (curr_path_num);
      if (!had_illegal) xlate_leaf_br_v2p (tip);

      instr_save_path_ins (curr_path_num, path_ops, path_cnt, orig_ops,
			   vliw_index, curr_path_num - path1_of_vliw, depth,
			   group_entry_vliw, group_entry_vliw_num, 
			   tip->br_addr, tip, tip->vliw->time + 1);
   }
   else {
      if (tip->left == 0) {
	 curr_path_num = path_num++;

	 if (!had_illegal) {
	    /* Leaf target should be VLIW entry tip */
	    assert (tip->right == tip->right->vliw->entry);

	    /* If assertion fails, increase MAX_PATHS */
	    assert (curr_path_num < MAX_PATHS);
	 }

         wr_shad_gprs   (gpr_shad_vec);
         wr_shad_fprs   (fpr_shad_vec);
         wr_shad_ccrs   (ccr_shad_vec);
         wr_shad_xers   (xer_shad_vec);
         wr_shad_fpscrs (fpscr_shad_vec);
         instr_bump_leaf_cnt (curr_path_num);

	 if (!had_illegal) xlate_leaf_br_v2p (tip);

	 /* For each group exit, we save the number of cycles executed
	    from the start of the group.  Combining this information,
	    with the number of times each exit is executed, we can
	    determine which paths in the program account for the bulk
	    of execution time.
	 */
	 if      (tip->right->vliw->group != tip->vliw->group)
	      num_cycles = tip->vliw->time + 1;
	 else if (tip->right->vliw == group_entry_vliw)
	      num_cycles = tip->vliw->time + 1;
	 else num_cycles = 0;

	 instr_save_path_ins (curr_path_num, path_ops, path_cnt, orig_ops,
			      vliw_index, curr_path_num - path1_of_vliw, depth,
			      group_entry_vliw, group_entry_vliw_num, 
			      tip->br_addr, tip, num_cycles);
	 
	 if (!had_illegal) {
	    /* Translate the VLIW we jump to if it not translated already. */
            /* NOTE:  This is the same check as in "xlate_onpage_v2p".     */
	    if (tip->right->vliw->xlate == 0)
	       add_vliw_xlate_pair (tip->right, xl_br_addr);
	 }
      }
      else {
	 spec_ops->skip_ops++;

         assert (!had_illegal);
	 if (!had_illegal) {
	    /* Skip fall-through should be in same VLIW */
	    assert (tip->vliw == tip->left->vliw);

            /* Skip target should be in same VLIW */
/*	    assert (tip->vliw == tip->right->vliw);*/
	    if (tip->vliw != tip->right->vliw) {
		printf ("\nPLEASE ENTER DBX (via \"dbx -a procnum\") and debug me\n");
		printf ("\ntip       = 0x%08x, tip->right       = 0x%08x\n",
			tip, tip->right);
		printf ("\ntip->vliw = 0x%08x, tip->right->vliw = 0x%08x\n\n",
			tip->vliw, tip->right->vliw);
		while (1);
	    }
	 }

	 patch_addr = xlate_skip_v2p (tip);
	 tip_to_ppc  (tip->left,  vliw_index, depth+1, spec_ops, path_ops,
		      path_cnt, orig_ops, path1_of_vliw, group,
		      gpr_shad_vec, fpr_shad_vec, ccr_shad_vec,
		      xer_shad_vec, fpscr_shad_vec);

	 patch_skip (patch_addr, xlate_mem_ptr);

	 tip_to_ppc  (tip->right, vliw_index, depth+1, spec_ops, path_ops,
		      path_cnt, orig_ops, path1_of_vliw, group,
		      gpr_shad_vec, fpr_shad_vec, ccr_shad_vec,
		      xer_shad_vec, fpscr_shad_vec);
      }
   }
}

/************************************************************************
 *									*
 *				dump					*
 *				----					*
 *									*
 * Dump translated code to translated code buffer.			*
 *									*
 * NOTE: Could make this a macro for efficiency.			*
 *									*
 ************************************************************************/

unsigned *dump (ins)
unsigned ins;
{
   int	    offset;
   unsigned *xmp;
   unsigned *rval;
   unsigned upper;
   unsigned lower;

   /* Simulation code memory has to be at least large enough to handle
      the simulation code from a single PPC page.  If this assert fails,
      increase XLATE_MEM_WORDS.
   */
   assert (page_simul_code_size++ < XLATE_MEM_WORDS);

   if (xlate_mem_ptr >= xlate_mem_end - consec_ins_needed) {

      /* Branch back to the start of translation memory -- If need less
	 than 26 (signed) bits, go directly, otherwise through CTR.
      */
      offset = ((char *) xlate_mem) - (char *) xlate_mem_ptr;
      if (offset >= -0x2000000) {
         *xlate_mem_ptr = 0x48000000 | (offset & 0x3FFFFFC);
      }
      else assert (1 == 0);	   /* Lots of branches break in this case */

      /* Invalidate all hash entries to the end of the translation memory */
      for (xmp = xlate_mem_ptr; xmp < xlate_mem_end; xmp++)
         invalidate_hash_castouts (xmp);

      xlate_mem_ptr  = &xlate_mem[0];
      full_trips_thru_simul_mem++;
   }

   invalidate_hash_castouts (xlate_mem_ptr);

   rval = xlate_mem_ptr;
   *xlate_mem_ptr++ = ins;

   return rval;
}

/************************************************************************
 *									*
 *				get_xlated_code_size			*
 *				--------------------			*
 *									*
 * How many bytes of simulation code did we generate in total?		*
 *									*
 ************************************************************************/

unsigned get_xlated_code_size ()
{
   return ((xlate_mem_ptr - xlate_mem) +
	   (full_trips_thru_simul_mem * XLATE_MEM_WORDS))
	  * sizeof (xlate_mem[0]);
}

/************************************************************************
 *									*
 *				xlate_string_indir			*
 *				------------------			*
 *									*
 ************************************************************************/

xlate_string_indir (op)
OPCODE2  *op;
{
   if      (op->b->op_num == OP_LSCBX)  xlate_lscbx_v2p (op);
   else if (op->b->op_num == OP_LSCBX_) xlate_lscbx_v2p (op);
   else if (op->b->op_num == OP_LSX)    xlate_lsx_v2p   (op);
   else if (op->b->op_num == OP_STSX)   xlate_stsx_v2p  (op);
   else    assert (1 == 0);
}

/************************************************************************
 *									*
 *				xlate_illegal_v2p			*
 *				-----------------			*
 *									*
 * Called when compiling illegal opcode.  Causes "handle_illegal_raw"	*
 * and "handle_illegal" routines to be invoked if this instruction	*
 * is actually executed.  The "handle_illegal_raw" routine merely sets	*
 * up the TOC register so that "handle_illegal" can properly access the	*
 * format string for "fprintf".						*
 *									*
 ************************************************************************/

xlate_illegal_v2p (ins_ptr, ins)
unsigned *ins_ptr;
unsigned ins;
{
   PTRGL    *ptrgl = (PTRGL *) handle_illegal_raw;
   unsigned offset = (unsigned) (((char *) ins_ptr) - (char *) prog_start);
   unsigned upper  = ((unsigned) ptrgl->func) >> 16;
   unsigned lower  = ((unsigned) ptrgl->func)  & 0xFFFF;

   /* We can destroy r3, r4, (and LR) because we do not return */
   dump (0x3C000000 |(3 << 21) |( 0 << 16) | upper); /* cau  r3,r0,<UPPER> */
   dump (0x60000000 |(3 << 21) |( 3 << 16) | lower); /* oril r3,r3,<LOWER> */
   dump (0x7C0003A6 |(3 << 21) |( 8 << 16));         /* mtspr LR,r3        */

   upper = offset >> 16;
   lower = offset  & 0xFFFF;
   dump (0x3C000000 |(3 << 21) |( 0 << 16) | upper); /* cau  r3,r0,<UPPER> */
   dump (0x60000000 |(3 << 21) |( 3 << 16) | lower); /* oril r3,r3,<LOWER> */

   upper  = ins >> 16;
   lower  = ins  & 0xFFFF;
   dump (0x3C000000 |(4 << 21) |( 0 << 16) | upper); /* cau  r4,r0,<UPPER> */
   dump (0x60000000 |(4 << 21) |( 4 << 16) | lower); /* oril r4,r4,<LOWER> */

   dump (0x4E800020);				     /* bcr handle_illegal */
}

/************************************************************************
 *									*
 *				xlate_opcode_v2p			*
 *				----------------			*
 *									*
 * RETURNS:  GPR or FPR written, if this occurs, -1 o.w.		*
 *									*
 ************************************************************************/

int xlate_opcode_v2p (op, ccr_shad_vec, xer_shad_vec, fpscr_shad_vec, mq_write)
OPCODE2  *op;
unsigned *ccr_shad_vec;
unsigned *xer_shad_vec;
unsigned *fpscr_shad_vec;
int	 *mq_write;			/* Output */
{
   int reg_wr = -1;
   int gpr_ext;
   int fpr_ext;
   int ca_saved;			/* Boolean */

   assert (!is_branch (op)  ||  is_trap (op));

   if (!xlate_expl_ccr_op (op, ccr_shad_vec, &reg_wr)) {

      gpr_ext  = save_xer_on_specul   (op);
      fpr_ext  = save_fpscr_on_specul (op);
      ca_saved = get_ca_extender_for_specul (op, gpr_ext);

      map_to_ppc_regs  (op, op->op.ins);
      load_to_ppc_regs (op);
      xlate_non_branch (op, op->op.ins);
      reg_wr = store_from_ppc_regs (op, ccr_shad_vec, mq_write);

      if (do_cache_simul  ||  do_cache_trace)
	 data_cache_ref (op, find_data_upper, find_data_lower);

      switch (op->b->op_num) {
	 case OP_OR_C:	handle_ca_commit    (op);	break;
	 case OP_OR_O:	handle_ov_commit    (op);	break;
	 case OP_OR_CO:	handle_ca_ov_commit (op);	break;
	 default:
	    if (!handle_fmrX_commit (op)) {   /* Returns if not FPSCR commit */

	       assert (!(gpr_ext >= 0  &&  fpr_ext >= 0));
	       if      (gpr_ext >= 0)
	          handle_result_xer_on_specul   (gpr_ext, xer_shad_vec);
	       else if (fpr_ext >= 0)
	          handle_result_fpscr_on_specul (fpr_ext, fpscr_shad_vec);
	       else if (ca_saved)
		  /* No need for this if call handle_result_xer_on_specul */
	          restore_xer_from_ca_read ();	
	       break;
	    }
      }
   }

   return reg_wr;
}

/************************************************************************
 *									*
 *				save_xer_on_specul			*
 *				------------------			*
 *									*
 * Save the XER to a scratch location if we have speculative operation	*
 * which sets it.  Return the GPR to which the speculative result is	*
 * written.								*
 *									*
 ************************************************************************/

int save_xer_on_specul (op)
OPCODE2 *op;
{
   int gpr_ext;
   int gpr_ca = sets_gpr_ca_bit (op);
   int gpr_ov = sets_gpr_ov_bit (op);

   if      (gpr_ca >= 0) gpr_ext = gpr_ca;
   else if (gpr_ov >= 0) gpr_ext = gpr_ov;
   else return -1;

   dump (0x7FE102A6);			/* mfspr r31,XER		 */
   dump (0x93E00000 | (RESRC_PTR << 16) | XER_SCR_OFFSET);
					/* st	 r31,XER_SCR_OFFSET(r13) */

   return gpr_ext;
}

/************************************************************************
 *									*
 *			handle_result_xer_on_specul			*
 *			---------------------------			*
 *									*
 * (1) Put the XER into the extender bits associated with "gpr_ext".	*
 * (2) Restore the XER to its value before this instruction.		*
 *									*
 ************************************************************************/

handle_result_xer_on_specul (gpr_ext, xer_shad_vec)
int	 gpr_ext;
unsigned *xer_shad_vec;
{
   int gpr_ext_off;

   set_bit (xer_shad_vec, gpr_ext);

   gpr_ext_off = SHDR0_XER_OFFSET + SIMXER_SIZE * (gpr_ext - R0);
   dump (0x7FE102A6);			/* mfspr r31,XER	         */
   dump (0x93E00000 | (RESRC_PTR << 16) | gpr_ext_off);	
					/* st    r31,GPR_XER_EXT(r13)    */

					/* l     r31,XER_SCR_OFFSET(r13) */
   dump (0x83E00000 | (RESRC_PTR << 16) | XER_SCR_OFFSET);	
   dump (0x7FE103A6);			/* mtspr XER,r31		 */
}

/************************************************************************
 *									*
 *			get_ca_extender_for_specul			*
 *			--------------------------			*
 *									*
 * If this "op" reads the CA extender bit of the GPR for use in an	*
 * extended precision calculation (e.g. AE, SFE, etc.), then save	*
 * the current XER and move the CA extender bit to the XER.		*
 *									*
 * Returns TRUE if the old CA value should be restored to the XER.	*
 *									*
 ************************************************************************/

int get_ca_extender_for_specul (op, save_gpr)
OPCODE2 *op;
int     save_gpr;
{
   int gpr_ext;
   int gpr_ext_off;

   /* If this op does not explicitly read CA from an extender, do nothing */
   if (get_ca_ppc_opcode (op->b->op_num) == op->b->op_num) return FALSE;

   gpr_ext     = op->op.renameable[GCA & (~OPERAND_BIT)];
   gpr_ext_off = R0_XER_OFFSET + SIMXER_SIZE * (gpr_ext - R0);

   dump (0x7FE102A6);			/* mfspr r31,XER	     */
   dump (0x83C00000 | (RESRC_PTR << 16) | gpr_ext_off);	
					/* l    r30,GPR_XER_EXT(r13) */

   /* save_xer_on_specul already saves old value if save_gpr >= 0    */
   if (save_gpr < 0)		        
      dump (0x93E00000 | (RESRC_PTR << 16) | XER_SCR_OFFSET);
					/* st	 r31,XER_SCR_OFFSET(r13) */

   dump (0x53DF0084);			/* rlimi r31,r30,0,2,2	     */
   dump (0x7FE103A6);			/* mtspr XER,r31	     */

   return TRUE;
}

/************************************************************************
 *									*
 *			restore_xer_from_ca_read			*
 *			------------------------			*
 *									*
 ************************************************************************/

restore_xer_from_ca_read ()
{
					/* l     r31,XER_SCR_OFFSET(r13) */
   dump (0x83E00000 | (RESRC_PTR << 16) | XER_SCR_OFFSET);	
   dump (0x7FE103A6);			/* mtspr XER,r31		 */
}


/************************************************************************
 *									*
 *				handle_ca_commit			*
 *				----------------			*
 *									*
 * (1) Put in r31 the XER contents in GPR.				*
 * (2) Put in r30 the XER from the speculative op being committed.	*
 * (3) Insert the CA bit (2) from r30 to r31.				*
 * (4) Put the updated r31 back in the XER.				*
 *									*
 ************************************************************************/

handle_ca_commit (op)
OPCODE2 *op;
{
   int gpr_ext = op->op.renameable[RB & (~OPERAND_BIT)];
   int gpr_ext_off = R0_XER_OFFSET + SIMXER_SIZE * (gpr_ext - R0);

   assert (op->b->op_num == OP_OR_C);

   dump (0x7FE102A6);			/* mfspr r31,XER	     */
   dump (0x83C00000 | (RESRC_PTR << 16) | gpr_ext_off);	
					/* l    r30,GPR_XER_EXT(r13) */

   dump (0x53DF0084);			/* rlimi r31,r30,0,2,2	     */
   dump (0x7FE103A6);			/* mtspr XER,r31	     */
}

/************************************************************************
 *									*
 *				handle_ov_commit			*
 *				----------------			*
 *									*
 * (1) Put in r31 the XER contents in GPR.				*
 * (2) Put in r30 the XER from the speculative op being committed.	*
 * (3) Insert the OV bit (1) from r30 to r31.				*
 * (4) Rotate r30 right by 1, so that OV aligned with SO (bit 0).	*
 * (5) Or OV from speculative op to SO from XER.			*
 * (6) Insert updated Summary Overflow (SO) in r31.			*
 * (7) Put the updated r31 back in the XER.				*
 *									*
 ************************************************************************/

handle_ov_commit (op)
OPCODE2 *op;
{
   int gpr_ext = op->op.renameable[RB & (~OPERAND_BIT)];
   int gpr_ext_off = R0_XER_OFFSET + SIMXER_SIZE * (gpr_ext - R0);

   assert (op->b->op_num == OP_OR_O);

   dump (0x7FE102A6);			/* mfspr r31,XER	     */
   dump (0x83C00000 | (RESRC_PTR << 16) | gpr_ext_off);	
					/* l    r30,GPR_XER_EXT(r13) */

   dump (0x53DF0042);			/* rlimi r31,r30,0,1,1	     */
   dump (0x57DEF83E);			/* rlinm r30,r30,31,0,31     */
   dump (0x7FDEFB78);			/* or    r30,r30,r31	     */
   dump (0x53DF0000);			/* rlimi r31,r30,0,0,0	     */
   dump (0x7FE103A6);			/* mtspr XER,r31	     */
}

/************************************************************************
 *									*
 *				handle_ca_ov_commit			*
 *				-------------------			*
 *									*
 * (1) Put in r31 the XER contents in GPR.				*
 * (2) Put in r30 the XER from the speculative op being committed.	*
 * (3) Insert the OV and CA bits (1 and 2) from r30 to r31.		*
 * (4) Rotate r30 right by 1, so that OV aligned with SO (bit 0).	*
 * (5) Or OV from speculative op to SO from XER.			*
 * (6) Insert updated Summary Overflow (SO) in r31.			*
 * (7) Put the updated r31 back in the XER.				*
 *									*
 ************************************************************************/

handle_ca_ov_commit (op)
OPCODE2 *op;
{
   int gpr_ext = op->op.renameable[RB & (~OPERAND_BIT)];
   int gpr_ext_off = R0_XER_OFFSET + SIMXER_SIZE * (gpr_ext - R0);

   assert (op->b->op_num == OP_OR_CO);

   dump (0x7FE102A6);			/* mfspr r31,XER	     */
   dump (0x83C00000 | (RESRC_PTR << 16) | gpr_ext_off);	
					/* l    r30,GPR_XER_EXT(r13) */

   dump (0x53DF0044);			/* rlimi r31,r30,0,1,2	     */
   dump (0x57DEF83E);			/* rlinm r30,r30,31,0,31     */
   dump (0x7FDEFB78);			/* or    r30,r30,r31	     */
   dump (0x53DF0000);			/* rlimi r31,r30,0,0,0	     */
   dump (0x7FE103A6);			/* mtspr XER,r31	     */
}

/************************************************************************
 *									*
 *				xlate_skip_v2p				*
 *				--------------				*
 *									*
 * Translate skips ("bc" instructions) as follows:			*
 *									*
 *			bc	L2					*
 *	L1:		b	FALL_THRU				*
 *	L2:		b	TARGET					*
 *	FALL_THRU:							*
 *									*
 * Returns the L2 address.  This unconditional branch can be filled in	*
 * (via "patch_skip" below) with real target address when it is known.	*
 *									*
 ************************************************************************/

unsigned *xlate_skip_v2p (tip)
TIP   *tip;
{
   int	    commit_time;
   unsigned offset;
   unsigned bi_crf0;
   unsigned bo = (tip->br_ins >> 21) & 0x1F;
   unsigned bi = tip->br_condbit - CR0;
   unsigned *rval;

   offset  = CRF0_OFFSET + SIMCRF_SIZE * (bi >> 2);
   bi_crf0 = bi & 3;
   dump (0x83E00000 | (RESRC_PTR<<16) | offset);	/* l  r31,bi>>4(r13) */
   dump (0x7FE80120);					/* mtcrf 0x80,r31    */

   consec_ins_needed = 3;
   dump (0x40000008 | (bo << 21) | (bi_crf0 << 16));	/*     bc L2         */
   consec_ins_needed = 1;
   dump (0x48000008);					/*     b  FALL_THRU  */
   rval = dump (0x48000000);				/* L2: b  TARGET     */

   return rval;						/* b TARGET instruc */
}

/************************************************************************
 *									*
 *				patch_skip				*
 *				----------				*
 *									*
 ************************************************************************/

patch_skip (br_addr, targ_addr)
unsigned *br_addr;
unsigned *targ_addr;
{
   if (targ_addr >= xlate_mem_end) targ_addr = xlate_mem;
   (*br_addr) |= (((char *) targ_addr) - ((char *) br_addr)) & 0x3FFFFFC;
}

/************************************************************************
 *									*
 *				xlate_leaf_br_v2p			*
 *				-----------------			*
 *									*
 ************************************************************************/

xlate_leaf_br_v2p (tip)
TIP *tip;
{
   switch (tip->br_type) {
      case BR_ONPAGE:
         xlate_onpage_v2p (tip);
	 break;

      case BR_OFFPAGE:
	 xlate_offpage_v2p (tip->br_ins, tip->right);
	 break;

      case BR_SVC:
         xlate_svc_v2p (tip);
	 break;

      case BR_TRAP:
	 xlate_trap_v2p ();
	 break;

      case BR_INDIR_LR:
         xlate_indir_v2p (tip->br_ins, FALSE, FALSE, FALSE);
	 break;

      case BR_INDIR_LR2:
         xlate_indir_v2p (tip->br_ins, FALSE, FALSE, TRUE);
	 break;

      case BR_INDIR_CTR:
         xlate_indir_v2p (tip->br_ins, TRUE, FALSE, FALSE);
	 break;

      case BR_INDIR_RFSVC:
         xlate_indir_v2p (tip->br_ins, FALSE, FALSE, FALSE);
	 break;

      case BR_INDIR_RFI:
         xlate_indir_v2p (tip->br_ins, FALSE, TRUE, FALSE);
	 break;

      default:
         fprintf (stderr, "Unknown type of leaf branch (%d)\n", tip->br_type);
	 exit (1);
   }
}

/************************************************************************
 *									*
 *				xlate_onpage_v2p			*
 *				----------------			*
 *									*
 * NO LONGER TRUE:							*
 * VLIW's in a "group" generally fall thru from one to the next in	*
 * the PPC simcode without need of an explicit branch.  There are	*
 * two exceptions:							*
 *									*
 *   (1) The successor instruction has already been emitted (the	*
 *	 unusual case since the VLIW tree instructions themselves	*
 *	 usually form a tree).						*
 *									*
 *   (2) The sucessor instruction is in another group.  (In this case	*
 *	 the offset is filled in later by "patch_vliw_ppc_group".)	*
 *									*
 ************************************************************************/

xlate_onpage_v2p (tip)
TIP *tip;
{
   int      diff;
   unsigned *br_addr;

   if (tip->right->vliw->xlate != 0) {
      br_addr = dump (0x48000000);			/* b PREV_XLATION */
      diff  = ((char *) tip->right->vliw->xlate) - ((char *) br_addr);
      diff &= 0x3FFFFFC;
      (*br_addr) |= diff;
   }
   else xl_br_addr = dump (0x48000000);		/* Fill in offset later */
}

/************************************************************************
 *									*
 *				xlate_offpage_v2p			*
 *				-----------------			*
 *									*
 * Dump code to branch indirectly offpage.  The pointer we jump to	*
 * points to the translation routine if the offpage entry point is not	*
 * yet translated, and to the translated code if the offpage entry	*
 * point has been translated.						*
 *									*
 * NOTE:  We must use r27 because r30 and r31 may be set by the		*
 *	  branch_and_link routine.  The use of r27 match that in	*
 *	  xlate_entry_raw in r13.s					*
 *									*
 ************************************************************************/

xlate_offpage_v2p (ins, target_addr)
unsigned ins;
unsigned *target_addr;
{
   unsigned lower;
   unsigned upper;

#ifdef DEBUG_SHOW_TARGET   
   fprintf (stdout, "Ins %08x: Target 0x%08x is offpage\n", ins, target_addr);
#endif

   /* Put in r27 the target address in the untranslated code */
   upper = ((unsigned) target_addr) >> 16;
   lower = ((unsigned) target_addr) &  0xFFFF;

   dump (0x3C000000 |(27 << 21) |( 0 << 16)| upper); /* cau  r27,r0,<UPPER>  */
   dump (0x60000000 |(27 << 21) |(27 << 16)| lower); /* oril r27,r27,<LOWER> */

   xlate_offpage_indir (ins, BR_OFFPAGE);
}

/************************************************************************
 *									*
 *				xlate_indir_v2p				*
 *				---------------				*
 *									*
 * Dump code to branch indirectly offpage.  The pointer we jump to	*
 * points to the translation routine if the offpage entry point is not	*
 * yet translated, and to the translated code if the offpage entry	*
 * point has been translated.						*
 *									*
 * NOTE:  We must use r27  because r30 and r31 may be set by		*
 *	  the branch_and_link routine.  The use	of r27 must		*
 *	  match that in xlate_entry_raw in r13.s			*
 *									*
 ************************************************************************/

xlate_indir_v2p (ins, use_ctr, use_srr0, use_lr2)
unsigned ins;
unsigned use_ctr;
unsigned use_srr0;
unsigned use_lr2;
{
   int lr;
   int offset;
   int br_type;

#ifdef DEBUG_SHOW_TARGET   
   fprintf (stdout, "Ins 0x%08x Target is indirect\n", ins);
#endif

   if      (use_ctr) { offset = CTR_OFFSET;  br_type = BR_INDIR_CTR; }
   else if (use_lr2) { offset = LR2_OFFSET;  br_type = BR_INDIR_LR2; }
   else		     { offset = LR_OFFSET;   br_type = BR_INDIR_LR;  }

   if (!use_ctr)
      if (use_srr0)
	   dump (0x7C0002A6 | (27<<21) | (26<<16));  /* mfspr r27,SRR0       */
      else dump (0x80000000 | (27<<21) | (RESRC_PTR<<16) | offset);
						     /* l r27,LR(r13)        */
#ifdef CTR_IS_NOT_R32
   else    dump (0x7C0002A6 | (27<<21) | (9<<16));   /* mfspr r27,CTR        */
#else						     /* l r27,CTR(r13)       */
      else dump (0x80000000 | (27<<21) | (RESRC_PTR<<16) | offset);
#endif

   xlate_offpage_indir (ins, br_type);
}

/************************************************************************
 *									*
 *				xlate_offpage_indir			*
 *				-------------------			*
 *									*
 * Dump code to branch indirectly offpage.  The pointer we jump to	*
 * points to the translation routine if the offpage entry point is not	*
 * yet translated, and to the translated code if the offpage entry	*
 * point has been translated.						*
 *									*
 * ASSUMES: r27 has already been loaded with address to branch to	*
 *	    in original code.						*
 *									*
 * WARNING:  If this the number of instrucs in this code is modified	*
 *	     "load_verify_v2p" should also be changed so that its	*
 *	     branch-around offset will be correct. OLD:  See loadver.c	*
 *									*
 ************************************************************************/

xlate_offpage_indir (ins, br_type)
unsigned ins;
int	 br_type;
{
   int	    offset;
   unsigned cond;
   PTRGL    *ptrgl;
   unsigned upper;
   unsigned lower;

   assert (num_ins_in_xlate_offpage_indir    == 26);
   assert (num_ins_in_xlate_offpage_cnt_code == 10);

   if	   (br_type == BR_INDIR_CTR) offset = INDIR_CTR_CNT;
   else if (br_type == BR_INDIR_LR)  offset = INDIR_LR_CNT;
   else if (br_type == BR_INDIR_LR2) offset = INDIR_LR2_CNT;
   else if (br_type == BR_OFFPAGE)   offset = OFFPAGE_CNT;
   else				     offset = -1;

   if (offset >= 0) {
     dump (0x83ED0000 | offset);	   /* l    r31,offset(r13) */
     dump (0x3BFF0001);			   /* cal  r31,1(r31)	   */
     dump (0x93ED0000 | offset);	   /* st   r31,offset(r13) */

     if (max_dyn_page_cnt != NO_DYN_PAGE_CNT) {
       if      (max_dyn_pages_action==1) ptrgl=(PTRGL *) hit_entry_pt_lim1_raw;
       else if (max_dyn_pages_action==2) ptrgl=(PTRGL *) hit_entry_pt_lim2_raw;
       else				 ptrgl=(PTRGL *) xlate_entry_raw;

       upper  = ((unsigned) ptrgl->func) >> 16;
       lower  = ((unsigned) ptrgl->func)  & 0xFFFF;

       dump (0x83ED0000 | DYNPAGE_CNT);	   /* l    r31,DYNPAGE_CNT(r13)     */
       dump (0x3BFF0001);		   /* cal  r31,1(r31)	            */
       dump (0x93ED0000 | DYNPAGE_CNT);	   /* st   r31,DYNPAGE_CNT(r13)     */
       dump (0x83CD0000 | DYNPAGE_LIM);	   /* l    r30,DYNPAGE_LIM(r13)     */
       dump (0x7C1FF040);		   /* cmpl 0,r31,r30                */

       consec_ins_needed = 5;
       dump (0x40820014);		   /* bne  below_lim		    */
       consec_ins_needed = 1;

       dump (0x3FC00000 | upper);	   /* cau  r30,r0,hit_entry_pt_lim  */
       dump (0x63DE0000 | lower);	   /* oril r30,r30,---------------> */
       dump (0x7FC803A6);		   /* mtlr r30			    */
       dump (0x4E800020);		   /* bcr  ALWAYS		    */
     /*				below_lim:				    */
     }
   }
   else {				      /* 3 NOPS so have const # ops */
     dump (0x7C000378);
     dump (0x7C000378);
     dump (0x7C000378);

     if (max_dyn_page_cnt != NO_DYN_PAGE_CNT) {
       dump (0x7C000378); dump (0x7C000378);  /* 10 NOPS so have const # ops */
       dump (0x7C000378); dump (0x7C000378);
       dump (0x7C000378); dump (0x7C000378);
       dump (0x7C000378); dump (0x7C000378);
       dump (0x7C000378); dump (0x7C000378);
     }
   }

   /* Make sure we don't corrupt CC register */
   dump (0x7fe00026);				     /* mfcr r31	     */
   dump (0x93ed0000|CC_SAVE);			     /*   st r31,CC_SAVE(r13)*/

/*	st	r27,PAGE_ENTRY(r13)		# r27 = Address branching to */
   dump (0x936D0000 | PAGE_ENTRY);
/*	l	r29,HASH_TABLE(r13)		# r29 = &hash_table[0]	     */
   dump (0x83AD0000 | HASH_TABLE);
/*	rlinm	r30,r27,0,16,29			# r30=((addr/4) & HMASK) * 4 */
   dump (0x577E043A | ((30-HASH_TABLE_LOG) << 6));
/*	lx	r30,r29,r30			# r30 = entry		     */
   dump (0x7FDDF02E);
/*	cmpi	0,r30,0				# entry == 0?		     */
   dump (0x2C1E0000);
/*	l	r29,XLATE_RAW(r13)		# r29 = address to jump to   */
   dump (0x83AD0000 | XLATE_RAW);
/*	mtspr	LR,r29				# LR  = address to jump to   */
   dump (0x7FA803A6);
/*	bc	12,2,DO_BRANCH			# Goto xlator if entry == 0  */
   consec_ins_needed = 15;
   dump (0x41820038);
   consec_ins_needed = 1;

/*						# Peel first part of loop    */
/*	l	r29,0(r30)			# r29 = entry->addr	     */
   dump (0x83be0000);
/*	cmpl	0,r29,r27			# entry->addr == addr	     */
   dump (0x7C1DD840);
/*	bc	12,2,LOAD_XLATE_LABEL		# break if entry->addr == addr */
   dump (0x4182001C);

/* LOOP:
/*	l	r30,8(r30)			# r30 = entry = entry->next  */
   dump (0x83DE0008);
/*	cmpi	1,r30,0				# entry->next == 0?	     */
   dump (0x2C9E0000);
/*	bc	12,6,DO_BRANCH			# break if entry->addr == addr */
   dump (0x41860018);
/*	l	r29,0(r30)			# r29 = entry->addr	     */
   dump (0x83BE0000);
/*	cmpl	0,r29,r27			# entry->addr == addr	     */
   dump (0x7C1DD840);
/*	bc	4,2,LOOP			# loop if entry->next != 0   */
   dump (0x4082FFEC);

/* LOAD_XLATE_LABEL:
/*	l	r30,4(r30)			# r30 = xlated addr	     */
   dump (0x83DE0004);
/*	mtspr	LR,r30				# LR  = xlated addr	     */
   dump (0x7FC803A6);

/* DO_BRANCH:							             */

   /* Restore condition register					     */
/*						# r30 = entry		     */
   dump (0x83ed0000|CC_SAVE);			      /* l  r31,CC_SAVE(r13) */
   dump (0x7feff120);				      /* mtcr   r31	     */

   dump (0x4E800020);				      /* bcr (UNCOND) */
}

/************************************************************************
 *									*
 *				xlate_svc_v2p				*
 *				-------------				*
 *									*
 *	1) Move into the real PPC link register the address of a piece	*
 *	   of code ("svc_rtn_code") dumped by "daisy" that does a hash	*
 *	   table lookup of LR(r13) and jumps to the previously		*
 *	   translated code or the translator as appropriate.		*
 *									*
 *	2) Dump out the svc instruction (which does not have		*
 *	   the link bit set).						*
 *									*
 ************************************************************************/

xlate_svc_v2p (tip)
TIP *tip;
{
   int		   first_time;
   unsigned	   upper;
   unsigned	   lower;
   static int	   already_called = FALSE;

   if (!already_called) {
      first_time     = TRUE;
      already_called = TRUE;
   }
   else first_time   = FALSE;

   /* Move the 32-bit condition register from its memory mapped location
      to the real condition register.
   */
   ccr_setup_v2p ();

   upper = ((unsigned) &svc_rtn_code[0]) >> 16;
   lower = ((unsigned) &svc_rtn_code[0]) &  0xFFFF;

   dump (0x3FE00000 | upper);		/* cau	r31,r0,UPPER  */
   dump (0x63FF0000 | lower);		/* oril	r31,r31,LOWER */
   dump (0x7FE803A6);			/* mtlr	r31	      */

   /* Make sure we don't have the link bit of the SVC on */
   dump (tip->br_ins & 0xFFFFFFFE);

   /* We don't want this flushed because random pieces of code can refer
      to it, so we dump it to a special area and flush it.
   */
   if (first_time) setup_kernel_rtn ();
}

/************************************************************************
 *									*
 *				setup_kernel_rtn			*
 *				----------------			*
 *									*
 ************************************************************************/

setup_kernel_rtn ()
{
   unsigned *save_xlate_mem_ptr;
   unsigned *save_xlate_mem_end;

   save_xlate_mem_ptr = xlate_mem_ptr;
   save_xlate_mem_end = xlate_mem_end;
   xlate_mem_ptr = svc_rtn_code;
   xlate_mem_end = svc_rtn_code + NUM_SVC_RTN_INS;

   /* Make sure LINK BIT not set in ins. */
   xlate_indir_v2p (0, FALSE, FALSE, FALSE);
   assert (xlate_mem_ptr - svc_rtn_code <= NUM_SVC_RTN_INS);

   flush_cache (svc_rtn_code, xlate_mem_ptr);
   xlate_mem_ptr = save_xlate_mem_ptr;
   xlate_mem_end = save_xlate_mem_end;
}

/************************************************************************
 *									*
 *				xlate_trap_v2p				*
 *				--------------				*
 *									*
 ************************************************************************/

xlate_trap_v2p ()
{
   assert (1 == 0);			/* Don't expect to be called */
}

/************************************************************************
 *									*
 *				ccr_setup_v2p				*
 *				-------------				*
 *									*
 * Move the memory mapped 32-bit condition register (i.e. CR0-CR7) to	*
 * the real condition register.						*
 *									*
 *	l	r31,CRF0_OFFSET(r13)					*
 *									*
 *	l	r30,CRF1_OFFSET(r13)	# Do 7 pairs of instructions	*
 *	rlimi	r31,r30,28,4,7		# like this			*
 *									*
 *	l	r30,CRF2_OFFSET(r13)	# Next pair			*
 *	rlimi	r31,r30,24,8,11						*
 *	...								*
 *	mtcrf	0xFF,r31		# Put the constructed val in CR	*
 *									*
 ************************************************************************/

ccr_setup_v2p ()
{
   int	     i;
   unsigned  shift;
   unsigned  start;
   unsigned  offset;

   dump (0x83E00000 | (RESRC_PTR<<16) | CRF0_OFFSET);     /* l r31,CRF0(r13) */

   for (i = 1, shift  = 28, start  = 4, offset  = CRF1_OFFSET;
	i < 8; 
	i++,   shift -= 4,  start += 4, offset += 4) {
						   
      dump (0x83C00000 | (RESRC_PTR<<16) | offset);       /* l r30,CRF?(r13) */
      dump (0x53DF0000 | (shift << 11) |/* rlimi r31,r30,shift,start,start+3 */
	   (start << 6)| ((start+3) << 1));
   }

   dump (0x7FEFF120);					  /* mtcrf 0xFF,r31  */
}

/************************************************************************
 *									*
 *				update_shad_writes			*
 *				------------------			*
 *									*
 ************************************************************************/

update_shad_writes (gpr_shad_vec, fpr_shad_vec, ccr_shad_vec, reg_wr)
unsigned *gpr_shad_vec;
unsigned *fpr_shad_vec;
unsigned *ccr_shad_vec;
int	 reg_wr;
{

   if	   (reg_wr >=    R0  &&  reg_wr <    R0 + num_gprs)
      set_bit (gpr_shad_vec, reg_wr - R0);

   else if (reg_wr >=   FP0  &&  reg_wr <   FP0 + num_fprs)
      set_bit (fpr_shad_vec, reg_wr - FP0);

   else if (reg_wr >=   CR0  &&  reg_wr <   CR0 + num_ccbits)
      set_bit (ccr_shad_vec, reg_wr - CR0);

   else assert (1 == 0);
}

/************************************************************************
 *									*
 *				load_verify_v2p				*
 *				---------------				*
 *									*
 * Produce simulation code for LOAD-VERIFY instructions.  This code	*
 * assumes that "store_from_ppc_regs" in "ppc_map.c" has already placed	*
 * the reloaded value in GPR_VERIFY or FPR_VERIFY as appropriate.	*
 * We compare this value to the speculative value (which should be in	*
 * a non-architected (for PPC) register.  If they are different, the	*
 * code produced by "xlate_page_offdir" is executed.  If they are the	*
 * same, the xlate_page_offdir code is branched around.  Below is a	*
 * template of the code for a LOAD-VERIFY of an integer quantity.	*
 *									*
 *	l     r31,SPEC_LD_REG(r13)					*
 *	l     r30,VERIF_LD(r13)						*
 *	cmp   r30,r31							*
 *	beq   L1:							*
 *									*
 *	l     r31,LD_VER_FAIL_CNT(r13)					*
 *	cal   r31,1(r31)						*
 *	st    r31,LD_VER_FAIL_CNT(r13)					*
 *									*
 *	liu   r27,    ppc_addr_of_ld_UPPER				*
 *	oril  r27,r27,ppc_addr_of_ld_LOWER				*
 *	<xlate_offpage_indir () code>					*
 *  L1:									*
 *									*
 * WARNING:  We must pay attention to the number of instructions	*
 *	     produced by "xlate_page_offdir".  If it changes, the	*
 *	     branch around offset also changes.				*
 *									*
 ************************************************************************/

load_verify_v2p (op, gpr_shad_vec, fpr_shad_vec, ccr_shad_vec, 
		 xer_shad_vec, fpscr_shad_vec)
OPCODE2  *op;
unsigned *gpr_shad_vec;
unsigned *fpr_shad_vec;
unsigned *ccr_shad_vec;
unsigned *xer_shad_vec;
unsigned *fpscr_shad_vec;
{
   int	    orig_dest = get_gpr_fpr_dest (op);
   unsigned upper     = op->op.orig_addr >> 16;
   unsigned lower     = op->op.orig_addr  & 0xFFFF;
   unsigned shad_writes;
   unsigned span_dist;
   unsigned *br_addr;

   assert (op->op.verif);
   assert (num_ins_in_loadver_fail_simgen == 3);

   if	   (orig_dest >= R0   &&  orig_dest < R0  + num_gprs) {
      dump (0x83E00000 | (RESRC_PTR << 16) |
	    resrc_map[orig_dest]->offset);		     /* l  r31,..    */
      dump (0x83C00000 | (RESRC_PTR << 16) | GPR_V_OFFSET);  /* l  r30,..    */
      dump (0x7C1EF800);				  /* cmp   0,r30,r31 */
   }
   else if (orig_dest >= FP0  &&  orig_dest < FP0 + num_fprs) {
#ifndef USE_FLT_COMP_ON_LD_VER
      dump (0x83E00000 | (RESRC_PTR << 16) |
	    resrc_map[orig_dest]->offset);		     /* l  r31,..    */
      dump (0x83C00000 | (RESRC_PTR << 16) | FPR_V_OFFSET);  /* l  r30,..    */
      dump (0x7C1EF800);				  /* cmp   0,r30,r31 */
      dump (0x83E00000 | (RESRC_PTR << 16) |
	   (resrc_map[orig_dest]->offset + 4));		     /* l  r31,..    */
      dump (0x83C00000 | (RESRC_PTR << 16) |(FPR_V_OFFSET+4));/*l  r30,..    */
      dump (0x7C9EF800);				  /* cmp   1,r30,r31 */
      dump (0x4C423202);				  /* crand 2,2,6     */
#else
      dump (0xCBE00000 | (RESRC_PTR << 16) |		     
	    resrc_map[orig_dest]->offset);		    /* lfd  fp31 ..  */
      dump (0xCBC00000 | (RESRC_PTR << 16) | FPR_V_OFFSET); /* lfd  fp30,..  */
      dump (0xFFA0048E);				/* mffs fp29         */
      dump (0xFC1EF800);				/* fcmpu 0,fp30,fp31 */
      dump (0xFDFEED8E);				/* mtfsf 255,fp29    */
#endif
   }
   else assert (1 == 0);

   /* Leave space for branch -- not generate till we know # of bytes
      it must branch around.  Since code may wrap around, we have to
      allow full 26-bit offset.
   */
   consec_ins_needed = 3;
   dump (0x41820008);
   consec_ins_needed = 1;
   dump (0x48000008);
   br_addr = dump (0x00000000);		/* Put out stub.  Fill in below */

   shad_writes  = wr_shad_gprs     (gpr_shad_vec);
   shad_writes += wr_shad_fprs     (fpr_shad_vec);
   shad_writes += wr_shad_ccrs     (ccr_shad_vec);
   shad_writes += wr_shad_xers     (xer_shad_vec);
   shad_writes += wr_shad_fpscrs (fpscr_shad_vec);

   span_dist = 4 * (shad_writes + num_ins_in_loadver_fail_simgen + 6);

   dump (0x83E00000 | (RESRC_PTR<<16) | LD_VER_FAIL_CNT);  /*   l r31,FAIL.. */
   dump (0x3BFF0001);					   /* cal r31,1(r31) */
   dump (0x93E00000 | (RESRC_PTR<<16) | LD_VER_FAIL_CNT);  /*  st r31,FAIL.. */

   dump (0x3F600000 | upper);		   	     /* liu   r27,ppc_upper */
   dump (0x637B0000 | lower);			     /* oril  r27,ppc_lower */

   loadver_fail_simgen ();

   /* Check if dump routine wrapped back to start of simul memory */
   if      (xlate_mem_ptr < br_addr) 
      span_dist = (unsigned) (((char *) xlate_mem_ptr) - (char *) br_addr);
   else if (xlate_mem_ptr == xlate_mem_end) 
      span_dist = (unsigned) (((char *) xlate_mem) - (char *) br_addr);

   *br_addr = 0x48000000 | (span_dist & 0x3FFFFFC);		   /* beq L1 */
}

/************************************************************************
 *									*
 *				bump_group_cnt				*
 *				--------------				*
 *									*
 * Emit code to bump the number of times the current group has been	*
 * executed.  (Should be done at the start of each group.)		*
 *									*
 ************************************************************************/

static unsigned *bump_group_cnt (pcnt)
unsigned *pcnt;
{
   unsigned upper = ((unsigned) pcnt) >> 16;
   unsigned lower = ((unsigned) pcnt) &  0xFFFF;
   unsigned *rval = xlate_mem_ptr;

   dump (0x3FE00000 | upper);			/* cau	r31,0,  upper */
   dump (0x63FF0000 | lower);			/* oril r31,r31,lower */
   dump (0x83DF0000);				/* l	r30,0(r31)    */
   dump (0x3BDE0001);				/* cal  r30,1(r30)    */
   dump (0x93DF0000);				/* st	r30,0(r31)    */

   return rval;
}

/************************************************************************
 *									*
 *				simul_reexec_info			*
 *				-----------------			*
 *									*
 ************************************************************************/

simul_reexec_info (fd, fcn)
int fd;
int (*fcn) ();			/* read or write */
{
   unsigned cnts_mem_size;
   unsigned simul_mem_size;

   assert (fcn (fd, &path_num, sizeof (path_num)) == sizeof (path_num));
   assert (fcn (fd, &vliw_num, sizeof (vliw_num)) == sizeof (vliw_num));
   assert (fcn (fd, &xlate_mem_ptr, sizeof (xlate_mem_ptr)) == 
	   sizeof (xlate_mem_ptr));

   cnts_mem_size = path_num * sizeof (perf_cnts[0]);
   assert (fcn (fd, perf_cnts, cnts_mem_size) == cnts_mem_size);

   simul_mem_size = (xlate_mem_ptr - xlate_mem) * sizeof (xlate_mem[0]);
   assert (fcn (fd, xlate_mem, simul_mem_size) == simul_mem_size);

   flush_cache (xlate_mem, xlate_mem_ptr);
}
