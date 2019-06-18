/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <a.out.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <float.h>

#include "vliw.h"
#include "contin.h"
#include "dis.h"
#include "dis_tbl.h"
#include "resrc_map.h"
#include "resrc_offset.h"
#include "stats.h"
#include "hash.h"
#include "branch.h"

#define XLATE_ENTRY_STK_SIZE		0x20000		/*  512 kbytes */

#define MAX_SERIAL_PTS_PER_PAGE		1024
#define MAX_GROUPS_PER_PAGE		4096
#define MAX_CURR_WD_BYTES		256
#define NUM_KERNEL_SVC_XLATE_INS	8

/* Two addresses found empirically for "_exit" pointer glue.
   Actual routine is in kernel.
*/
#define _EXIT_PTRGL_ADDR1	0xD00D14C4
#define _EXIT_PTRGL_ADDR2	0XD0011AD8


typedef struct {
   unsigned *ppc;
   VLIW	    *vliw;
} HASHPAIR;

extern OPCODE1	   illegal_opcode;
extern PPC_OP_FMT  *ppc_op_fmt[MAX_PPC_FORMATS];

extern int	   operand_val[NUM_PPC_OPERAND_TYPES];
extern char	   operand_set[NUM_PPC_OPERAND_TYPES];
extern unsigned	   perf_cnts[];
extern unsigned	   lsx_area[];
extern HASH_ENTRY  **hash_table;
extern OPCODE2	   *latest_vliw_op;
/* extern unsigned xlate_mem[];*/
extern unsigned    *xlate_mem;
extern unsigned    *xlate_mem_ptr;
extern int	   dump_bkpt_on;
extern int	   do_daisy_comp;
extern int	   trace_xlated_pgm;
extern int	   unroll_while_ilp_better;
extern int	   do_cache_simul;
extern int	   do_cache_trace;
extern int	   num_invoc_till_native;
extern int	   extra_invoc_till_native;
extern int	   xlated_pgm_has_segv_handler;
extern int	   do_vliw_dump;
extern int	   use_cont_cnt;
extern int	   use_join_cnt;
extern int	   save_curr_xlation;
extern int	   use_prev_xlation;
extern int	   use_pdf_cnts;
extern int	   use_pdf_prob;
extern int	   pdf_0cnt_offpage;
extern int	   aixver;
extern int	   errno;
extern char	   *errno_text[];
extern unsigned    max_dyn_page_cnt;

extern int	   doing_load_mult;	/* ld_motion.c uses to stop motion */

extern int	   inline_ucond_br;
extern int	   recompiling;

extern REG_RENAME     *map_mem[][NUM_PPC_GPRS_C+NUM_PPC_FPRS+NUM_PPC_CCRS];
extern MEMACC	      *stores_mem[][NUM_MA_HASHVALS];
extern unsigned	      *gpr_wr_mem[][NUM_PPC_GPRS_C];
extern unsigned short avail_mem[][NUM_MACHINE_REGS];
extern int	      avail_mem_cnt;

extern int	   max_till_ser_loop;
extern int	   max_till_ser_other;
extern int	   max_unroll_loop;
extern int	   max_duplic_other;
extern int	   max_ins_per_group;		/* 0 or neg ==> No limit */
extern int	   num_skips;
extern int	   num_ccbits;

extern unsigned	   rpagesize_mask;
extern unsigned	   rpage_entries;

extern double	   loop_back_prob;
extern double	   negint_cmp_prob;
extern double	   if_then_prob;

/* # Times point in PPC page STARTS CONTINUATION / TRANSLATED TOTAL:
   Following two counts refer only to the current translation of the
   page.  If parts of the page were translated from some other entry
   point, we do not know it, as the counts are discarded after each
   page translation.
*/
static unsigned	   char cont_xlate_cnt[MAX_RPAGE_ENTRIES];
static unsigned	   char total_xlate_cnt[MAX_RPAGE_ENTRIES];

/* Bit Vector to check if we have visited a branch before on this page */
static unsigned    char visited_br[MAX_RPAGE_ENTRIES/8];

/* 2-bit saturating counter to check whether each point in the page is
   a join point.  Each time we encounter a branch, we bump by 1, the 
   counter for its target address (assuming it is onpage).  If (1) the
   instruction preceding the target is not an unconditional branch, and
   (2) the target is not the first instruction of the page, then the
   counter is bumped again.  Whenever the counter for a location is
   greater than or equal to 2, the location is a join.
*/
static unsigned    char join_cnt[MAX_RPAGE_ENTRIES];

static HASHPAIR	   hash_table_page_list[MAX_SERIAL_PTS_PER_PAGE];
static unsigned	   hash_table_page_cnt;
static VLIW	   *vliw_page[MAX_RPAGE_ENTRIES];
static unsigned	   *xlate_addr[MAX_RPAGE_ENTRIES];
static unsigned	   *kernel_svc_xlate;
static int	   updated_orig_ops;
static int	   prog_start_diff;   /* Curr prog_start - Earlier Run start */
       unsigned	   *xlate_entry_raw_ptr;
       unsigned	   xlate_entry_cnt;
       unsigned	   total_ppc_ops;	/* NOTE: May xlate single op mult 
						 times.  Each xlation counted
						 here so total may be inflated.
					 */
       unsigned    *prog_start;
       unsigned	   *prog_end;
       unsigned    textins;
       unsigned	   *curr_ins_addr;
       unsigned	   *curr_entry_addr;

static unsigned	   branch_type;

       char	   *daisy_progname;
static char	   daisy_progname_buff[256];

       char     curr_wd[MAX_CURR_WD_BYTES];
static char     daisy_vliw_file[256];
static int	group_ppc_ins_cnt[MAX_GROUPS_PER_PAGE];
static unsigned curr_group;
static unsigned svc_instr_addr;	      /* Normally 0x3600 or 0x37D0 */

       unsigned char r13_area[R13_AREA_SIZE];
static unsigned xlate_entry_stack[XLATE_ENTRY_STK_SIZE];

unsigned *get_target (unsigned, unsigned char *, OPCODE2 *, unsigned *);
unsigned *xlate_entry        (unsigned *);
unsigned *xlate_stack_setup  (void);
unsigned *xlate_entry_raw    (unsigned *);
unsigned *loadver_fail_raw   (unsigned *);
unsigned *instrum_code	     (PTRGL *, unsigned, unsigned, int);
unsigned reverse_br_cond     (unsigned);
void	 handle_illegal      (unsigned,   unsigned);
void	 finish_daisy_compile (int);
void	 finish_daisy_compile_raw (void);
PTRGL *load_prog_to_exec     (char *, unsigned, char *, char *);
unsigned *get_svc_target     (unsigned, OPCODE2 *);
unsigned *in_atexit	     (unsigned *);
unsigned *in__exit	     (unsigned *);
VLIW	 *chk_prev_xlation   (unsigned *, int *);
TIP   *handle_prev_xlation   (XLATE_POINT *, VLIW *);
TIP   *handle_serialize	     (XLATE_POINT *, unsigned *, int);
TIP    *add_tip_to_vliw	     (XLATE_POINT *);
TIP *init_continuation_page  (unsigned *);
TIP *init_xlate_page	     (unsigned *);
TIP *xlate_illegal_p2v	     (TIP *, OPCODE1 *, unsigned *, unsigned);
TIP *handle_onpage	     (TIP *, OPCODE2 *, unsigned, unsigned *,
			      unsigned *, unsigned *, unsigned);
TIP *handle_offind	     (int, TIP *, OPCODE2 *, unsigned, unsigned *,
			      unsigned *, unsigned *, unsigned);
TIP *set_link_tip	     (TIP *);
TIP *set_bcl_tip	     (TIP *);
TIP *add_leaf_br_tip	     (TIP *, int, int);
int reopen_dump_file	     (void);

/************************************************************************
 *									*
 *				main					*
 *				----					*
 *									*
 * Read the text section of a (fully-linked) executable, and		*
 * translate all code on the entry page reachable from the entry	*
 * point.  Initialize data structures so that future pages and entry	*
 * points may be translated when "xlate_entry" is invoked by calls from	*
 * the translated code.							*
 *									*
 * This routine is "main2" because the real "main" is in assembly	*
 * language and adds the stack value as a 4th parameter	and then calls	*
 * us.  This is so that we can invoke the translated code with the same	*
 * stack value as the original code (or close to it).  This is not	*
 * needed for correctness, but can be an aid in debugging when		*
 * comparing to the untranslated version.				*
 *									*
 * NOTE:  We use "read" not "fread" below because "fread" invokes	*
 *	  malloc.  As described in the header to the routine,		*
 *	  "malloc_tbl_mem" in "dis.c", this is undesirable.		*
 *									*
 ************************************************************************/

main2 (argc, argv, env, sp_val)
int	 argc;
char	 *argv[];
char	 *env;
unsigned sp_val;
{
   int		  fd;
   unsigned	  n;
   struct filehdr hdr;
   AOUTHDR	  auxhdr;
   PTRGL	  *ptrgl;
   int		  (*fptr) ();
   int		  num_flags;
   char		  buff[256];

   init_errno_text ();

   if (argc < 2) print_usage (argv[0], "Incorrect # of Arguments");

   num_flags = handle_flags (argc, argv);

   fd = open (argv[num_flags + 1], O_RDONLY, 0);
   if (fd == -1) {
      sprintf (buff, "Could not open input file, \"%s\"", argv[num_flags+1]);
      print_usage (argv[0], buff);
   }
   else {
      strcpy (daisy_progname_buff, argv[num_flags + 1]);
      daisy_progname = daisy_progname_buff;
   }

   n = read (fd, &hdr, sizeof (hdr));
   assert (n == sizeof (hdr));

   n = read (fd, &auxhdr, sizeof (auxhdr));
   assert (n == sizeof (auxhdr));
   textins = auxhdr.tsize / 4;
   close (fd);

   assert (getwd (curr_wd) == curr_wd);

   ptrgl = load_prog_to_exec (argv[num_flags+1], textins, argv[0], buff);

   /* Ignore segmentation violations from speculation */
   if (num_invoc_till_native != 0) zeromem ();
   init_everything ();
   init_svc        ();
   xlate_mem_ptr = xlate_mem;

   /* Might want to allow pdf only with "use_prev_xlation" */
   if (use_prev_xlation) read_reexec_info ();
   if (use_pdf_cnts  ||  use_pdf_prob) read_pdf (daisy_progname);

   if (do_daisy_comp  &&  do_vliw_dump) init_dump_file ();
   if (trace_xlated_pgm) init_trace_br_xlated ();

/* atexit		     (finish_daisy_compile);*/
   setup_regs		     (argc - 1 - num_flags, &argv[num_flags+1], env);

   if (!use_prev_xlation) {
     xlate_stack_setup	     ();
     if (do_daisy_comp && num_invoc_till_native != 0) xlate_entry (ptrgl->func);
     else					   no_xlate_setup (ptrgl->func);
   }

   ptrgl->func = xlate_mem;
   fptr = (int) ptrgl;
   fptr (argc - 1 - num_flags, &argv[num_flags+1], env, sp_val);
}

/************************************************************************
 *									*
 *				finish_daisy_compile			*
 *				--------------------			*
 *									*
 * This routine expects to be called from the translated code from	*
 * 0x3600 ("svc_instr" in /unix), the entry to kernel service routines	*
 * in AIX/PPC.  This routine is only invoked when _exit() service	*
 * routine is called.  Hence we finish this routine by re-calling	*
 * _exit() with the exit code of the application program that was just	*
 * translated.  See the routine "dump_exit_chk" below.			*
 *									*
 ************************************************************************/

void finish_daisy_compile (exit_code)
int exit_code;
{
   if (dump_bkpt_on) finish_bkpt ();
   instr_wrapup ();			/* Write VLIW ins descrip to disk   */
   finish_vliw_simul ();		/* Write counts to disk		    */
   if (save_curr_xlation) save_reexec_info ();	/* Write info to save.daisy */
   if (do_cache_simul) prnt_cachestat ();
   if (do_cache_trace) trace_done     ();
   dump_loadver_summary ();
   do_stats ();

   _exit (exit_code);
}

/************************************************************************
 *									*
 *				load_prog_to_exec			*
 *				-----------------			*
 *									*
 * Load the program to execute into memory and leave pointers to its	*
 * start and end.							*
 *									*
 ************************************************************************/

PTRGL *load_prog_to_exec (progname, num_ins, dis_progname, buff)
char	 *progname;
unsigned num_ins;
char	 *dis_progname;
char	 *buff;
{
   PTRGL *fptr;

   fptr = load (progname, 1, 0);

   if (!fptr) {
      sprintf (buff, "Load of target program, \"%s\" failed.  Errno = %d:\n\t%s",
	       progname, errno, errno_text[errno]);
      print_usage (dis_progname, buff);
   }

   prog_start = fptr->func;
   prog_end   = ((unsigned *) fptr->func) + num_ins;

   return fptr;
}

/************************************************************************
 *									*
 *				xlate_stack_setup			*
 *				-----------------			*
 *									*
 * Precede the real translated code with stack setup to original value.	*
 *									*
 ************************************************************************/

unsigned *xlate_stack_setup ()
{
   unsigned *rval = xlate_mem_ptr;

   dump (0x38260000);				/* cal r1,0(r6) */
   return rval;
}

/************************************************************************
 *									*
 *				no_xlate_setup				*
 *				--------------				*
 *									*
 * Setup to do unconditional branch to untranslated code.  We do this	*
 * because the stack value must is set to match the initial stack	*
 * value of the translated version, and after the stack is properly	*
 * set, we must jump back to the untranslated code.			*
 *									*
 ************************************************************************/

no_xlate_setup (entry)
unsigned entry;
{
   dump (0x3FE00000 | (entry >> 16));	    /* liu   r31,      entry_HIGH */
   dump (0x63FF0000 | (entry & 0xFFFF));    /* oril  r31, r31, entry_LOW  */
   dump (0x7FE803A6);			    /* mtspr LR,  r31		  */
   dump (0x4E800020);			    /* bcr			  */

   flush_cache (xlate_mem, xlate_mem_ptr);
}

/************************************************************************
 *									*
 *				xlate_entry				*
 *				-----------				*
 *									*
 * Translate points on "entry"'s page that are reachable from "entry".	*
 *									*
 ************************************************************************/

unsigned *xlate_entry (entry)
unsigned *entry;
{
   int		i;
   int		serialize;
   int		force_serialize;
   int		targ_offset;
   int		kernel_svc_entry;				  /* Boolean */
   int		status;	 /* 0=keep xlating, 1=condbr, 2=UCbranch, 4=page end */
   PTRGL	*ptrgl = (PTRGL *) xlate_entry_raw;
   int		num_ops_on_tip;
   unsigned	curr_index;
   unsigned	*text;
   unsigned	*p_xlate;
   unsigned	*ppc_src;
   unsigned	*ins_ptr;
   unsigned	ins;
   unsigned	*start;
   unsigned	*end;
   unsigned	*rval;
   unsigned	*entry1;
   unsigned	offset_in_page;
   OPCODE1	*opcode;
   OPCODE2	*opcode_out;
   VLIW		*prev_xlation;
   TIP		*prev_tip;
   TIP		*curr_tip;
   TIP		*dummy_tip;
   XLATE_POINT	*xlate_point;

/* if (rval = in_atexit (entry)) return rval;*/
/* if (rval = in__exit  (entry)) return entry;*/
   kernel_svc_entry = kernel_svc_call (entry);

   xlate_entry_cnt++;

   /* If we have invoked "xlate_entry" at least "num_invok_till_native"
      times, then instead of translating, finish executing the program
      natively.  This can be used with binary search to track down
      difficult bugs in large programs.  For example if "xlate_entry"
      is invoked 100 times before the translated program crashes, then
      set "num_invoc_till_native" to 50.  If it does not crash, then
      increase "num_invoc_till_native" to 75, otherwise reduce it to
      25.  In this way, the entry point with the original error can
      quickly be isolated.
   */
   if (num_invoc_till_native >= 0)
      if (xlate_entry_cnt > num_invoc_till_native + extra_invoc_till_native) {
	 rval = dump_to_native ();
	 exec_unxlated_code (entry, rval);
      }

   /* Do this on every translation, because some programs (like the jove
      editor) setup their handler and bomb on our speculative load seg viols
   */
   if (xlated_pgm_has_segv_handler) setup_segv_handler ();

   if (dump_bkpt_on) dump_bkpt (entry);

   curr_entry_addr = entry1 = entry;
   rval = xlate_mem_ptr;
   xlate_entry_raw_ptr = ptrgl->func;

   start = (unsigned *) (((unsigned) entry) & (~rpagesize_mask));
   end   = start + rpage_entries;

   /* Points to start of real translated code */
   dummy_tip = init_xlate_page (entry);

   assert (rpage_entries % WORDBITS == 0);

   while (xlate_point = get_highest_prio_cont ()) {

      prev_tip  = xlate_point->source;		/* In VLIW code   */
      ppc_src   = xlate_point->ppc_src;		/* In original code */
      entry     = xlate_point->target;		/* In original code */
      serialize = xlate_point->serialize;

      if (!serialize) force_serialize = 
	 xlated_enough (ppc_src, entry, prev_tip, &prev_xlation,
			xlate_point->targ_type);
      else prev_xlation = chk_prev_xlation (entry, &force_serialize);

      if (prev_xlation) {
	 curr_tip = handle_prev_xlation (xlate_point, prev_xlation);
	 continue;
      }
      else if (serialize  ||  force_serialize) {
	   curr_tip = handle_serialize (xlate_point, entry, force_serialize);
	   if (curr_tip == 0) continue;
      }
      else curr_tip = add_tip_to_vliw  (xlate_point);

      ins_ptr = entry;
      
      num_ops_on_tip = 0;
      curr_index = (((unsigned) ins_ptr) & rpagesize_mask) >> 2;
      while (TRUE) {

	 curr_ins_addr = ins_ptr;

         /* In words, not bytes */
         offset_in_page = (((unsigned) ins_ptr) & rpagesize_mask) /
		     sizeof (unsigned);
	 total_xlate_cnt[offset_in_page]++;
	 set_bit (curr_tip->loophdr, offset_in_page);

         if (entry >= prog_start  &&  entry < prog_end)
	    ins = *(ins_ptr + prog_start_diff);
	 else ins = *ins_ptr;

	 /* If this is a join point that does not start the tip,
	    convert it to an unconditional branch to this location.
	    In this way, we can serialize here if need be.
         */
	 if (use_join_cnt  &&  num_ops_on_tip > 0  &&
	     join_cnt[curr_index] > 1) ins = 0x48000000;

	 curr_index++;
	 num_ops_on_tip++;

	 ins_ptr++;
	 total_ppc_ops++;
	 group_ppc_ins_cnt[curr_tip->vliw->group]++;

         opcode = get_opcode (ins);
	 doing_load_mult = FALSE;	/* To be set o.w. by xlate_mult_p2v */

	 /* If in illegal opcodes, programmer must have known that conditional
	    branch would always be taken.  Stop since this would lead to
	    garbage anyway and may confuse us in the process.
	 */
	 if	 (opcode == &illegal_opcode) {
	    curr_tip = xlate_illegal_p2v (curr_tip, opcode, ins_ptr - 1, ins);
	    status = 2;		/* Make illegal status like uncond branch */
	 }
	 else if (is_multiple (opcode)) {
	   status = xlate_mult_p2v    (&curr_tip, opcode, ins, ins_ptr,
				       start, end);
	   curr_tip->orig_ops++;
	 }
	 else if (is_ld_st_update (opcode)) {
	   status = xlate_update_p2v  (&curr_tip, opcode, ins, ins_ptr,
				       start, end);
	   curr_tip->orig_ops++;
	 }
	 else if (xlate_addic_0 (&curr_tip, opcode, ins, ins_ptr,
				 start, end, &status)) curr_tip->orig_ops++;
         else {
	    opcode_out = set_operands (opcode, ins, opcode->b.format);
	    status     = xlate_opcode_p2v (&curr_tip, opcode_out, ins, ins_ptr,
					   start, end);
	    if (!updated_orig_ops) curr_tip->orig_ops++;
	 }

	 if (status) break;
      }

   /* Have we completed the last instruction on the page?  Put a branch
      to the next page if page did not end with a branch ("status & 3")
      (in which case the offpage stuff is handled in "handle_onpage"
      and "handle_offind").

      The "handle_offind" routine looks at the link bit of "ins"
      to see whether "bl" type code should be produced.  It should
      not in this case.
   */
      if (ins_ptr == end  &&  !(status & 3)) {
         ins        = 0x48000004;			/* b NEXT_INS */
	 opcode     = get_opcode (ins);
	 opcode_out = set_operands (opcode, ins, opcode->b.format);
	 branch_type = NON_SUPERVISOR;
	 handle_offind (BR_OFFPAGE, curr_tip, opcode_out, ins & (~LINK_BIT),
			ins_ptr - 1, end, ins_ptr, TRUE);
      }
   }

   assert (dummy_tip->left == 0);

   /******************* For debugging ******************/
   if (do_vliw_dump) vliw_dump ();

   /* If we just translated the kernel service entry point (0x3600
      or "svc_instr" in PPC/AIX), then spit out code to jump to
      "finish_daisy_compile" if the service requested is _exit()
   */
   if (kernel_svc_entry) dump_exit_chk ();

   vliw_to_ppc_simcode ();
   flush_cache (rval, xlate_mem_ptr);

   add_page_hash_entries (kernel_svc_entry);
   hash_table_page_cnt = 0;

   /* If after executing this page we are to execute code natively,
      clear all hash table entries, so that all attempts to go offpage
      will endup going through us (xlate_entry).  Since we can't access
      any old translations, we might as well reclaim the simulation
      memory while we're at it.
   */
   if (num_invoc_till_native >= 0)
      if (xlate_entry_cnt >= num_invoc_till_native) {
	 clr_all_hash_entries ();
         xlate_mem_ptr = xlate_mem;
      }

   return rval; 
}

/************************************************************************
 *									*
 *				init_xlate_page				*
 *				---------------				*
 *									*
 ************************************************************************/

TIP *init_xlate_page (entry)
unsigned *entry;
{
   int i;
   int size;
   TIP *dummy_tip;

   dummy_tip = init_continuation_page (entry);
   init_opcode_ptrs   ();
   vliw_init_page     ();
   ma_init_page	      ();
   init_page_brlink   ();
   init_page_bct      ();
   init_page_regmap   ();
   tree_pos_init_page ();
   unify_init_page    ();
   page_verif_loophdr_mem ();		/* For debugging purposes only */
   page_verif_cluster_mem ();		/* For debugging purposes only */

   for (i = 0; i < rpage_entries; i++) {
      cont_xlate_cnt[i]  = 0;
      total_xlate_cnt[i] = 0;
      vliw_page[i]       = 0;
      join_cnt[i]        = 0;
   }

   size = rpage_entries / 8;
   for (i = 0; i < size; i++)
      visited_br[i] = 0;	/* Clear the bit vector of visited branches */

   /* Make sure each group of VLIW's is set to have 0 instructions to start */
   for (i = 0; i < curr_group; i++)
      group_ppc_ins_cnt[i] = 0;

   curr_group = 0;

   return dummy_tip;
}

/************************************************************************
 *									*
 *				in_atexit				*
 *				---------				*
 *									*
 * Check if the current entry point is the translator itself, in	*
 * particular if it is the "finish_daisy_compile" function.  If so	*
 * return a pointer to an assembly language routine which sets up	*
 * registers for "finish_daisy_compile".  Otherwise return 0.		*
 *									*
 ************************************************************************/

unsigned *in_atexit (entry)
unsigned *entry;
{
   unsigned *exit_rtn_locn;
   unsigned *exit_rtn_locn_xlated;
   PTRGL    *finish_daisy_compile_ds	 = (PTRGL *) finish_daisy_compile;
   PTRGL    *finish_daisy_compile_raw_ds = (PTRGL *) finish_daisy_compile_raw;

   /* Let's not translate the translator for now */
   if (entry != finish_daisy_compile_ds->func) return 0;

   /* Setup LR2 with the address where "finish_daisy_compile" should return */
   exit_rtn_locn = *((unsigned *) &r13_area[LR_OFFSET]);
   exit_rtn_locn_xlated = get_xlated_addr (exit_rtn_locn);
   assert (exit_rtn_locn_xlated);
   *((unsigned *) &r13_area[LR2_OFFSET]) = exit_rtn_locn_xlated;

   return finish_daisy_compile_raw_ds->func;
}

/************************************************************************
 *									*
 *				in__exit				*
 *				--------				*
 *									*
 * Check if the current entry point is the "_exit" routine which is	*
 * called when "exit" finishes and is called directly by some programs.	*
 * If so, invoke the "finish_daisy_compile" function by returning a	*
 * pointer to an assembly language routine which sets up registers	*
 * for "finish_daisy_compile".  Otherwise return 0.			*
 *									*
 ************************************************************************/

unsigned *in__exit (entry)
unsigned *entry;
{
   /* Let's not translate the translator for now */
   if (entry == _EXIT_PTRGL_ADDR1  ||  entry == _EXIT_PTRGL_ADDR2) {
      finish_daisy_compile (0);
      return entry;
   }
   else return 0;
}

/************************************************************************
 *									*
 *				kernel_svc_call				*
 *				---------------				*
 *									*
 * Returns TRUE if kernel service call, FALSE otherwise.		*
 *									*
 * This is very PPC/AIX dependent.  It could also be made into a	*
 * macro for efficiency.						*
 *									*
 ************************************************************************/

int kernel_svc_call (entry)
unsigned entry;
{
   /* Expect svc_instr_addr to be 0x3600 or 0x37D0 */
   if (entry == svc_instr_addr) return TRUE;
   else				return FALSE;
}

/************************************************************************
 *									*
 *				dump_exit_chk				*
 *				-------------				*
 *									*
 * Spit out code at the start of the translation of "svc_instr" in	*
 * /unix (Normally 0x3600 or 0x37D0) to check if the _exit() kernel	*
 * service is being called.  The _exit() call has r2=13 in AIX 3.2	*
 * and r2=23 in AIX 4.1, and r2=34 in AIX 4.3.  If this is a call to	*
 * _exit(), then go to our "finish_daisy_compile" routine instead.	*
 * When done, "finish_daisy_compile", should invoke _exit() with the 	*
 * proper return code (passed in r3).					*
 *									*
 ************************************************************************/

dump_exit_chk ()
{
   PTRGL      *finish_ds = (PTRGL *) finish_daisy_compile_raw;
   unsigned   func       = finish_ds->func;

   kernel_svc_xlate = xlate_mem_ptr;

   dump (0x7FC00026);			/*	  mfcr r30		     */
   if      (aixver >= 300  &&  aixver < 400)
        dump (0x2C02000D);		/*        cmpi r2,13		     */
   else if (aixver >= 400  &&  aixver < 420)
        dump (0x2C020017);		/*        cmpi r2,23		     */
   else if (aixver >= 420  &&  aixver < 430)
        dump (0x2C020019);		/*        cmpi r2,25		     */
   else dump (0x2C020022);		/*        cmpi r2,34		     */
   dump (0x40820014);			/*        bne  real_3600_xlate       */
   dump (0x3FE00000 | (func>>16));	/*        liu  r31,    finish >> 16  */
   dump (0x63FF0000 | (func&0xFFFF));	/*        oril r31,r31,finish&0xFFFF */
   dump (0x7FE803A6);			/*        mtlr r31		     */
   dump (0x4E800020);			/*        bcr			     */
					/* real_3600_xlate:		     */
   dump (0x7FCFF120);			/*	  mtcrf 255,r30		     */
}

/************************************************************************
 *									*
 *				chk_prev_xlation			*
 *				----------------			*
 *									*
 * Check if we have already translated starting from the target.  If	*
 * so, return the first VLIW of the translation. Otherwise return NULL.	*
 *									*
 ************************************************************************/

VLIW *chk_prev_xlation (entry, force_serialize)
unsigned *entry;
int      *force_serialize;
{
   unsigned offset_in_page;
   unsigned cnt;
   unsigned cont_cnt;			/* # Times starts continuation */
   unsigned total_cnt;			/* # Times xlated in total     */

   /* In words, not bytes */
   offset_in_page = (((unsigned) entry) & rpagesize_mask) / sizeof (unsigned);

   /* If group is bigger than allowed size, serialize */
   cont_cnt  = cont_xlate_cnt[offset_in_page];
   total_cnt = total_xlate_cnt[offset_in_page];
   if (use_cont_cnt) cnt = cont_cnt;
   else		     cnt = total_cnt;

   /* We should be wary about serializing branch-and-links, but we
      have to do it sometimes, or we can get stuck when translating
      an infinite recursion, e.g. recur (x) { recur (x+1); }
   */
   if (cnt >= max_till_ser_loop + 2*max_unroll_loop) *force_serialize = TRUE;
   else						     *force_serialize = FALSE;
   
   cont_xlate_cnt[offset_in_page] = cont_cnt + 1;
   return vliw_page[offset_in_page];
}

/************************************************************************
 *									*
 *				xlated_enough				*
 *				-------------				*
 *									*
 * Check if we have already visited the current "entry" a sufficient	*
 * ("max_unroll") number of times.  If so and we have previously	*
 * translated starting from "entry" then set "prev_xlation" to the VLIW	*
 * at the start	of the previous translation and return TRUE.  Otherwise	*
 * bump the number of times we have visited this point and return FALSE.*
 *									*
 ************************************************************************/

int xlated_enough (ppc_src, target, prev_tip, prev_xlation, targ_type)
unsigned ppc_src;
unsigned target;
TIP	 *prev_tip;
VLIW	 **prev_xlation;		/* Output	*/
int	 targ_type;
{
   int	    max_till_ser;
   int	    max_duplic;
   unsigned offset_in_page;
   unsigned cnt;
   unsigned cont_cnt;		/* # Times starts continuation */
   unsigned total_cnt;		/* # Times xlated in total     */
   unsigned joinpt_cnt;		/* # of ways to reach this pt from onpage */

   /* In words, not bytes */
   offset_in_page = (target & rpagesize_mask) / sizeof (unsigned);

   assert (max_unroll_loop  >= 2); /* Move to where max_unroll_loop  read */
   assert (max_duplic_other >= 2); /* Move to where max_duplic_other read */

   /* If group is bigger than allowed size, serialize */
   cont_cnt   = cont_xlate_cnt[offset_in_page];
   total_cnt  = total_xlate_cnt[offset_in_page];
   joinpt_cnt = join_cnt[offset_in_page];

   /* We serialize differently on loops and diamond join points */
   if (is_bit_set (prev_tip->loophdr, offset_in_page)) {
      joinpt_cnt   = 2;		/* Treat all loop headers as join points */
      max_till_ser = max_till_ser_loop;
      max_duplic   = max_unroll_loop;
   }
   else {
      max_till_ser = max_till_ser_other;
      max_duplic   = max_duplic_other;
   }

   if      (use_cont_cnt)  cnt = cont_cnt;
   else if (!use_join_cnt) cnt = total_cnt;
   else {
      if (joinpt_cnt > 1)  cnt = total_cnt;
      else		   cnt = 0;		/* Don't serialize at non-joins */
   }

   /* We should be very wary about serializing unconditional branches,
      but we have to do it sometimes, or we can get stuck when translating
      an infinite loop, e.g. while (1);
   */
   if (inline_ucond_br  &&  targ_type == UNCOND_TARG) {
      if (cnt < max_till_ser + 2 * max_duplic) {
         cont_xlate_cnt[offset_in_page] = cont_cnt + 1;
         *prev_xlation = 0;
	 return FALSE;
      }
   }

   if (max_ins_per_group > 0) {
      if (group_ppc_ins_cnt[prev_tip->vliw->group] > max_ins_per_group) {
	 *prev_xlation = vliw_page[offset_in_page];
	 return TRUE;
      }
   }

   if (cnt >= max_till_ser) {

      if (cnt >= max_till_ser + max_duplic) {
         if (unroll_while_ilp_better  &&
	     cnt < max_till_ser + 4*max_duplic) {
	    if (ilp_increasing (prev_tip, target)) {
	       cont_xlate_cnt[offset_in_page] = cont_cnt + 1;
	       *prev_xlation = 0;
	       return FALSE;
	    }
	 }
         *prev_xlation = vliw_page[offset_in_page];
	 return TRUE;
      }
      else {
	 cont_xlate_cnt[offset_in_page] = cont_cnt + 1;
         *prev_xlation = 0;
	 if (cnt != max_till_ser)	     return FALSE;
	 else if (vliw_page[offset_in_page]) return FALSE;
	 else				     return TRUE;
      }
   }
   else {
      *prev_xlation = 0;
      cont_xlate_cnt[offset_in_page] = ++cont_cnt;
      if (target <= ppc_src) 
	 if (!vliw_page[offset_in_page]) return TRUE;  /* If loop, peel 1st */
	 else				 return FALSE; /* iter, then unroll */
      else				 return FALSE;
   }
}

/************************************************************************
 *									*
 *				handle_prev_xlation			*
 *				-------------------			*
 *									*
 * Fixup place that branched here to branch to previously translated	*
 * VLIW code (except for the fall-thru from an unconditional BRLINK	*
 * which will go to the previously translated code via the hash table.)	*
 *									*
 * RETURNS:  the entry tip to previously translated code.		*
 *									*
 ************************************************************************/

TIP *handle_prev_xlation (xlate_point, prev_xlation)
XLATE_POINT *xlate_point;
VLIW	    *prev_xlation;
{
   TIP *curr_tip;
   TIP *prev_tip;

   prev_tip = xlate_point->source;
   curr_tip = prev_xlation->entry;

   if      (xlate_point->targ_type == CDFALL_THRU)
      prev_tip = add_leaf_br_tip (prev_tip, FALSE, BR_ONPAGE);

   else if (xlate_point->targ_type == COND_TARG) {
      prev_tip = add_leaf_br_tip (prev_tip, TRUE, BR_ONPAGE);
      free_loophdr_mem (xlate_point->loophdr);
      free_cluster_mem (xlate_point->cluster);
   }

   else assert (xlate_point->targ_type == UNCOND_TARG  ||
	        xlate_point->targ_type == UCFALL_THRU  ||
	        xlate_point->targ_type == CDLFALL_THRU);

   if (xlate_point->targ_type !=  UCFALL_THRU  &&
       xlate_point->targ_type != CDLFALL_THRU) {
      prev_tip->right = curr_tip;
      prev_tip->left  = 0;				/* Since leaf */
   }

   free_loophdr_mem (prev_tip->loophdr);
   free_cluster_mem (prev_tip->cluster);

   return prev_xlation->entry;
}

/************************************************************************
 *									*
 *				handle_serialize			*
 *				----------------			*
 *									*
 * Handle the case of a VLIW which serializes execution, such as the	*
 * VLIW at a procedure return, or a VLIW which is jumped to from	*
 * another page.  Make the probability of the first tip of any		*
 * serializer VLIW be 1 -- nothing can percolate past the start tip in	*
 * any case.								*
 *									*
 * RETURNS:  Tip to which operations should be appended.  NULL is	*
 *	     returned if this entry point has already been translated.	*
 *									*
 ************************************************************************/

TIP *handle_serialize (xlate_point, entry, force_serialize)
XLATE_POINT *xlate_point;
unsigned    *entry;
int	    force_serialize; /* Bool: Code not need serial, but seen k times */
{
   int	    i;
   char	    br_type;
   unsigned offset_in_page;
   unsigned xlated_addr;
   TIP	    *curr_tip;
   TIP	    *prev_tip;
   VLIW	    *new_vliw;

   /* The target of an unconditional branch (that is not "bl") should
      not be treated as a serialization point.  It inhibits parallelism
      and breaks up the natural flow of the code.
   */
   if (inline_ucond_br  &&  xlate_point->targ_type == UNCOND_TARG  &&  
       !force_serialize)
      return xlate_point->source;

   /* If we already have translated from this entry point (during a
      previous page translation, then don't do it again unless recompiling.
   */
   xlated_addr = get_xlated_addr (entry);
   if (xlated_addr != xlate_entry_raw_ptr  &&  !recompiling) {
      curr_tip = (TIP *) entry;
      br_type = BR_OFFPAGE;
   }
   else {

      br_type = BR_ONPAGE;

      /* In words, not bytes */
      offset_in_page=(((unsigned) entry) & rpagesize_mask) / sizeof (unsigned);

      new_vliw = get_vliw ();
      assert (hash_table_page_cnt < MAX_SERIAL_PTS_PER_PAGE);
      hash_table_page_list[hash_table_page_cnt].ppc  = entry;
      hash_table_page_list[hash_table_page_cnt].vliw = new_vliw;
      hash_table_page_cnt++;

      curr_tip = new_vliw->entry;
      curr_tip->prev     = 0;		/* Not go to pred of serializer VLIW */

      /* Set numbering so can quickly tell ancestor tips in group */
      tree_pos_group_start (&curr_tip->tp);

      if (use_pdf_cnts) curr_tip->prob = DBL_MAX/2;  /* We use cnts with PDF */
      else	        curr_tip->prob = 1.00;	/*             prob without  */
   }

   prev_tip = xlate_point->source;	/* However, pred must get here */

   /* If serializing conditional fall-thru, must add new tip with
      unconditional branch to this new serialized VLIW.
   */
   if	   (xlate_point->targ_type == CDFALL_THRU) {
      prev_tip = add_leaf_br_tip (prev_tip, FALSE, BR_ONPAGE);
      prev_tip->right   = curr_tip;
      prev_tip->br_type = br_type;
   }
   else if (xlate_point->targ_type == COND_TARG) {
      if (force_serialize)
	 prev_tip = add_leaf_br_tip (prev_tip, TRUE, BR_ONPAGE);
      prev_tip->right   = curr_tip;
      prev_tip->br_type = br_type;

      /* "add_to_xlate_list" allocated memory for loophdr, cluster. */
      free_loophdr_mem (xlate_point->loophdr);
      free_cluster_mem (xlate_point->cluster);
   }
   else if (xlate_point->targ_type == UNCOND_TARG) {
      prev_tip->left    = 0;		/* If b AROUND, must set to 0 */
      prev_tip->right   = curr_tip;
      prev_tip->br_type = br_type;
   }
   else if (xlate_point->targ_type == UCFALL_THRU) {
      prev_tip->left = 0;
   }
   else assert (xlate_point->targ_type == CDLFALL_THRU);/* Reach by hash tbl */

   /* Initial entry point to program has uninitialized (0) loophdr, cluster */
   if (prev_tip->loophdr) free_loophdr_mem (prev_tip->loophdr);
   if (prev_tip->cluster) free_cluster_mem (prev_tip->cluster);

   /* Now that we have fixed up "prev_tip", we can return if this point
      has already been translated.
   */
   if (xlated_addr != xlate_entry_raw_ptr  &&  !recompiling) return 0;

   /* Increase MAX_GROUPS_PER_PAGE if assertion fails */
   assert (curr_group < MAX_GROUPS_PER_PAGE);
   new_vliw->group = curr_group++;	/* Get unique group ID		    */
   new_vliw->time  = 0;			/* VLIW's in group relative to this */

   vliw_page[offset_in_page] = new_vliw;

   /* Make all non-architected registers available for renaming */
   init_reg_avail (curr_tip);

   /* Make all values and resources available immediately, identity mapping */
   assert (avail_mem_cnt < MAX_PATHS_PER_PAGE);
   curr_tip->avail       = avail_mem[avail_mem_cnt];
   curr_tip->gpr_writer  = gpr_wr_mem[avail_mem_cnt];
   curr_tip->gpr_rename  = map_mem[avail_mem_cnt];
   curr_tip->fpr_rename  = curr_tip->gpr_rename + NUM_PPC_GPRS_C;
   curr_tip->ccr_rename  = curr_tip->fpr_rename + NUM_PPC_FPRS; 
   curr_tip->stores      = stores_mem[avail_mem_cnt];
   curr_tip->loophdr     = get_loophdr_mem ();
   curr_tip->cluster     = get_cluster_mem ();
   curr_tip->last_store  = 0;
   curr_tip->stm_r1_time = -1;

   curr_tip->ca_rename.vliw_reg = CA;
   curr_tip->ca_rename.time     = 0;
   curr_tip->ca_rename.prev     = 0;

#ifdef ACCESS_RENAMED_OV_RESULT
   curr_tip->ov_rename.vliw_reg = OV;
   curr_tip->ov_rename.time     = 0;
   curr_tip->ov_rename.prev     = 0;
#endif

   mem_clear (curr_tip->avail,      NUM_MACHINE_REGS  *sizeof (curr_tip->avail[0]));
   mem_clear (curr_tip->gpr_rename, NUM_PPC_GPRS_C*sizeof (curr_tip->gpr_rename[0]));
   mem_clear (curr_tip->fpr_rename, NUM_PPC_FPRS  *sizeof (curr_tip->fpr_rename[0]));
   mem_clear (curr_tip->ccr_rename, NUM_PPC_CCRS  *sizeof (curr_tip->ccr_rename[0]));
   mem_clear (curr_tip->stores,     NUM_MA_HASHVALS *sizeof (curr_tip->stores[0]));

   avail_mem_cnt++;

   /* Have seen no LD/ST-UPDATE instructions in this group */
   mem_clear (new_vliw->mem_update_cnt, 
	      NUM_PPC_GPRS_C * sizeof (new_vliw->mem_update_cnt[0]));

   /* Set all GPR's to unwritten in the current group */
   for (i = 0; i < NUM_PPC_GPRS_C; i++) {
      curr_tip->gpr_writer[i] = (unsigned *) NO_GPR_WRITERS;
   }

   return new_vliw->entry;
}

/************************************************************************
 *									*
 *				add_tip_to_vliw				*
 *				---------------				*
 *									*
 * The code up to "xlate_point->source" (which is a branch) should	*
 * already have been translated.  We are now ready to translate either	*
 * the fall-through path of "xlate_point->source" or the branch target.	*
 * In either case, we do so by adding a new tip beginning at		*
 * "xlate_point->source", and adding the PPC code to the new tip.	*
 * (Commits go on or after the new tip -- the actual operation may be	*
 * percolated up.)							*
 *									*
 ************************************************************************/

TIP *add_tip_to_vliw (xlate_point)
XLATE_POINT *xlate_point;
{
   int *resrc_cnt;
   TIP *curr_tip;
   TIP *prev_tip;

   /* If original code had unconditional branch around, throw the
      branch away, and keep adding operations from the branch target
      at the tip on which the unconditional branch would have gone.
   */
   if (xlate_point->targ_type == UNCOND_TARG) return xlate_point->source;
   else {

      curr_tip = get_tip ();

      prev_tip			= xlate_point->source;
      curr_tip->vliw		= prev_tip->vliw;
      curr_tip->prev		= prev_tip;
      curr_tip->num_ops		= 0;		/* No ops in list yet */
      curr_tip->op		= 0;
      curr_tip->left		= 0;
      curr_tip->prob		= xlate_point->prob;

      if      (xlate_point->targ_type ==   COND_TARG) {
	 prev_tip->right = curr_tip;
	 tree_pos_right (&prev_tip->tp, &curr_tip->tp);
      }
      else if (xlate_point->targ_type == CDFALL_THRU) {
	 prev_tip->left  = curr_tip;
	 tree_pos_left  (&prev_tip->tp, &curr_tip->tp);
      }
      else assert (1 == 0);

      /* We set "avail" here, so make sure "set_tip_regs" not do any copying.
	 We set it here (actually in "contin.c" when we add the continuation
         to the list of continuations) because if we wait, the values can
	 change to reflect later activity on the other path of the
         conditional branch.
      */
      set_tip_regs (prev_tip, curr_tip, FALSE);
      curr_tip->avail		= xlate_point->avail;
      curr_tip->gpr_rename	= xlate_point->gpr_rename;
      curr_tip->fpr_rename	= xlate_point->fpr_rename;
      curr_tip->ccr_rename	= xlate_point->ccr_rename;
      curr_tip->stores		= xlate_point->stores;
      curr_tip->gpr_writer	= xlate_point->gpr_writer;
      curr_tip->last_store	= xlate_point->last_store;
      curr_tip->stm_r1_time	= xlate_point->stm_r1_time;
      curr_tip->loophdr         = xlate_point->loophdr;
      curr_tip->cluster         = xlate_point->cluster;
      curr_tip->ca_rename       = xlate_point->ca_rename;
      curr_tip->ov_rename	= xlate_point->ov_rename;

      /* Initially, we've seen as many LD/STORE-UPDATES on this path since
	 the VLIW start, as our predecessor tip.
      */
      mem_copy (curr_tip->mem_update_cnt, prev_tip->mem_update_cnt,
		NUM_PPC_GPRS_C * sizeof (prev_tip->mem_update_cnt[0]));

      resrc_cnt = prev_tip->vliw->resrc_cnt;
#ifdef LEAF_BRANCHES_USE_ISSUE_SLOT
      resrc_cnt[ISSUE]++;
#endif
/*    resrc_cnt[BRLEAF]++; */

      return curr_tip;
   }
}

/************************************************************************
 *									*
 *				xlate_illegal_p2v			*
 *				-----------------			*
 *									*
 * For ease in debugging, save the original address of the illegal	*
 * instruction in the "page" field of opcode.  This way we can access	*
 * it later (in "xlate_illegal_v2p" which dumps code to pass the	*
 * address to a runtime error handling routine.				*
 *									*
 ************************************************************************/

TIP *xlate_illegal_p2v (tip, opcode_in, ins_ptr, ins)
TIP	 *tip;
OPCODE1  *opcode_in;
unsigned *ins_ptr;
unsigned ins;
{
   OPCODE2 opcode;
   TIP     *rtn_tip;
   TIP	   *ins_tip;

   opcode.b	    = &opcode_in->b;
   opcode.op.num_rd = 0;
   opcode.op.num_wr = 0;
   opcode.b->page   = ins_ptr;

   rtn_tip = insert_op (&opcode, tip, tip->vliw->time, &ins_tip);

   rtn_tip->br_ins  = 0x48000000;		/* b OFFPAGE */
   rtn_tip->br_type = BR_OFFPAGE;
   rtn_tip->right   = rtn_tip;		/* If someone tries to use it, we'll */
   rtn_tip->left    = 0;		/* catch the infinite loop in the    */
					/* debugger.			     */
   return rtn_tip;
}

/************************************************************************
 *									*
 *				handle_illegal				*
 *				--------------				*
 *									*
 * Called when attempting to execute illegal opcode.			*
 *									*
 * NOTE:  The "start of original program" is generally 0x10000200,	*
 *	  i.e. the "_start" routine.					*
 *									*
 ************************************************************************/

void handle_illegal (offset, ins)
unsigned offset;
unsigned ins;
{
   fprintf (stderr, "Illegal opcode (0x%08x) at offset 0x%08x from start of original program\n",
	    ins, offset);
   exit (1);
}

/************************************************************************
 *									*
 *				hit_entry_pt_lim1			*
 *				-----------------			*
 *									*
 ************************************************************************/

hit_entry_pt_lim1 (cnt, addr)
unsigned cnt;
unsigned addr;
{
   fprintf (stderr, "\nExecuted %u dynamic crosspage and indirect jumps.\n", cnt);
   fprintf (stderr, "Next entry would have been at 0x%08x.\n", addr);

   /* Produce stats and exit with error code 1 */
   finish_daisy_compile (1);
}

/************************************************************************
 *									*
 *				hit_entry_pt_lim2			*
 *				-----------------			*
 *									*
 ************************************************************************/

hit_entry_pt_lim2 (cnt, addr)
unsigned cnt;
unsigned addr;
{
   unsigned	*rval;

   rval = dump_to_native ();
   exec_unxlated_code (addr, rval);
}

/************************************************************************
 *									*
 *				xlate_opcode_p2v			*
 *				----------------			*
 *									*
 * Translate PPC op to VLIW.  This means finding the appropriate	*
 * VLIW instruction in which to place the PPC operation and perhaps	*
 * its commit.								*
 *									*
 * Returns 0 if should keep translating along this straightline path,	*
 *	   1 bit set if "opcode" is condbranch(so stop xlating on path)	*
 *	   2 bit set if "opcode" is UC branch (so stop xlating on path)	*
 *	   4 bit set if "opcode" last on page (so stop xlating on path)	*
 *									*
 ************************************************************************/

int xlate_opcode_p2v (ptip, opcode, ins, ins_ptr, start, end)
TIP	 **ptip;
OPCODE2  *opcode;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int		type;
   int		rval = 0;
   int		offset;
   int		earliest;
   int	        old_reg;
   int	        old_avail;
   unsigned	uncond;
   unsigned	*target;
   unsigned	*xlated_addr;
   TIP		*tip = *ptip;
   TIP		*ins_tip;
   TIP		*ver_tip;
   OPERAND_OUT	*ver_op;
   BASE_OP      *prior_op;

   if (!is_branch (opcode)) {
      updated_orig_ops = FALSE;
      old_avail = get_dest_avail_time (tip, opcode, &old_reg, &prior_op);

      if      (combine_induc_var  (ptip, opcode, ins_ptr,	&ins_tip)) {}
      else if (sched_ld_before_st (ptip, opcode, ins_ptr, TRUE, &ins_tip,
				   &ver_tip, &ver_op))			   {}
      else if (sched_copy_op      (ptip, opcode, ins, -1,	&ins_tip)) {}
      else if (split_record	  (ptip, opcode, ins_ptr, start, end))     {}
      else if (sched_ppc_rz0_op   (ptip, opcode, -1,	        &ins_tip)) {}
      else if (sched_rlwimi       (ptip, opcode,	        &ins_tip)) {}
      else {
	 earliest = get_earliest_time (tip, opcode, FALSE);
	 *ptip = insert_op (opcode, tip, earliest, &ins_tip);
         ma_store (*ptip, opcode);	/* Save for alias analysis if STORE */
      }
      del_dead_tip_ops (tip, old_reg, old_avail, prior_op);
   }
   else {
      updated_orig_ops = TRUE;
      target = get_target (ins, ins_ptr - 1, opcode, &uncond);
      if (uncond) rval = 2;
      else	  rval = 1;  /* Non-TRAP branches: stop xlating on this path */

      if (branch_type == SVC) {
	 *ptip = handle_svc    (tip, opcode, ins_ptr - 1);
	 free_loophdr_mem ((*ptip)->loophdr);
	 free_cluster_mem ((*ptip)->cluster);
      }

      else if (branch_type == TRAP) {
	 *ptip = handle_trap1  (tip, opcode);
	 rval = 0;
      }

      else if (branch_type != NON_SUPERVISOR)
         *ptip = handle_rfpriv (tip, opcode, ins_ptr - 1, branch_type);
      else

      if (((unsigned) target) & INDIR_BRANCH) {
         if (ins & 0x400) type = BR_INDIR_CTR;
         else		  type = BR_INDIR_LR;

	 *ptip = handle_offind (type, tip, opcode, ins, ins_ptr - 1,
				end, target, uncond);
      }
      else if (target < start  ||  target >= end)
	 *ptip = handle_offind (BR_OFFPAGE, tip, opcode, ins, ins_ptr - 1,
				end, target, uncond);
      else *ptip = 
	 handle_onpage (tip, opcode, ins, ins_ptr - 1, end, target, uncond);
   }

   assert (ins_ptr <= end);

   /* Have we completed the last instruction on the page?  If so,
      stop translating along this path.
   */
   if (ins_ptr == end) return rval | 4;
   else return rval;
}

/************************************************************************
 *									*
 *				get_target				*
 *				-----------				*
 *									*
 ************************************************************************/

unsigned *get_target (ins, addr, opcode, uncond)
unsigned      ins;
unsigned char *addr;
OPCODE2	      *opcode;
unsigned      *uncond;		/* Unconditionally Executed ? */
{
   int		num;
   int		op_num;
   int		type;
   int		num_operands;
   unsigned	val;
   unsigned	*rval = (unsigned *) -1;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;

   branch_type = NON_SUPERVISOR;	/* Assume non-supervisor   */
   *uncond = TRUE;			/* Assume is unconditional */

   if (opcode->b->primary == 17) {
      branch_type = SVC;
      return get_svc_target (ins, opcode);
   }
   else if (opcode->b->primary == 19  &&  opcode->b->ext == 164) {
      branch_type = RFSVC;
      return INDIR_BRANCH;
   }
   else if (opcode->b->primary == 19  &&  opcode->b->ext == 100) {
      branch_type = RFI;
      return INDIR_BRANCH;
   }

   curr_format  = ppc_op_fmt[opcode->b->format];
   num_operands = curr_format->num_operands;

   for (num = 0; num < num_operands; num++) {
      op_num  = curr_format->ppc_asm_map[num];
      operand = &curr_format->list[op_num];
      type    = operand->type;

      /* Check for branch-always condition to determine if unconditional */
      if (type == BO)
	 if ((operand_val[BO & (~OPERAND_BIT)] & 0x14) != 0x14) *uncond=FALSE;

      if (type == LI  ||  type == BD  ||  type == LEV) {
	 val = extract_operand_val (ins, operand);
	 val = chk_immed_val (val, type);

	 assert (rval == -1);
	 if (is_rel_branch (opcode)) rval = (unsigned *) (addr + val);
	 else			     rval = val;
      }
      else if (type == TO) {		/* Trap Instruction */
	 *uncond     = FALSE;
	 branch_type = TRAP;
	 assert (rval == -1);
	 rval = 0x700;
      }
   }

   if      (rval != -1)    return rval;
   else if (opcode->b->sa) return 0x1FE0;
   else			   return INDIR_BRANCH;
}

/************************************************************************
 *									*
 *				get_svc_target				*
 *				--------------				*
 *									*
 * Assumes that MSR(IP) = 0, i.e. that system code is based at		*
 * address 0x00000000 (RAM), not 0xFFF00000 (ROM).  The ROM		*
 * offset (0xFFF00000) should be added to the returned values		*
 * if the code is in ROM.						*
 *									*
 ************************************************************************/

unsigned *get_svc_target (ins, opcode)
unsigned      ins;
OPCODE2	      *opcode;
{
/* if (opcode->b->sa)  return  0x1FE0;		More general alternative */
   if (ins & 0x02)     return  0x1FE0;
   else return 0x1000 | (ins & 0x0FE0);
}

/************************************************************************
 *									*
 *				handle_onpage				*
 *				-------------				*
 *									*
 ************************************************************************/

TIP *handle_onpage (curr_tip, opcode, ins, br_addr, end, target_addr, uncond)
TIP	 *curr_tip;
OPCODE2  *opcode;
unsigned ins;
unsigned *br_addr;
unsigned *end;
unsigned *target_addr;
unsigned uncond;
{
   int	    was_bct;
   int	    serialize;
   int	    earliest;
   int	    targ_type;
   int	    link = ins & LINK_BIT;		/* Boolean */
   int	    probs_ok;				/* Boolean */
   int	    fallthru_offpage;			/* Boolean */
   unsigned taken_cnt;
   unsigned fallthru_cnt;
   unsigned br_index;
   unsigned targ_index;
   double   taken;
   double   fallthru;
   double   prob;
   TIP	    *link_tip;
   TIP	    *ins_tip;
   TIP	    *off_tip;

   assert (branch_type == NON_SUPERVISOR);

   /* If this is a conditional branch whose target was never taken
      in the profiling run, then treat this branch as offpage, and
      don't waste time and space translating it, unless it actually
      occurs in this run.
   */
   if (!uncond  &&  (use_pdf_cnts  ||  use_pdf_prob)) {
      get_condbr_cnts (curr_tip, br_addr, &taken_cnt, &fallthru_cnt);
      if (taken_cnt == 0  &&  pdf_0cnt_offpage) {
	 return handle_offind (BR_OFFPAGE, curr_tip, opcode, ins, br_addr,
			       end, target_addr, uncond);
      }
   }

   br_index = (((unsigned) br_addr) >> 2) & rpagesize_mask;
   if (!is_bit_set (visited_br, br_index)) {

      targ_index = (((unsigned) target_addr) & rpagesize_mask) >> 2;

      if (join_cnt[targ_index] < 2) join_cnt[targ_index]++;
      if (targ_index != 0  &&  !is_uncond_br (target_addr - 1))
	 if (join_cnt[targ_index] < 2) join_cnt[targ_index]++;

      set_bit (visited_br, br_index);
   }

   /* Fill in our link register if need be */
   if (link) {
      curr_tip = branch_and_link (curr_tip, br_addr);
      serialize = TRUE;
      opcode->op.ins &= 0xFFFFFFFE;		/* Turn off link bit */
   }
   else serialize = FALSE;

   if (uncond) was_bct = FALSE;
   else {
      was_bct = handle_bcond (&curr_tip, opcode);
      if (was_bct) ins = opcode->op.ins;	/* Can change if was BCT */
   }

   /* The branch can go no earlier than the current VLIW, but may have
      to go later, if an input (e.g. cond register bit) is not yet available.
   */
   earliest = get_earliest_time (curr_tip, opcode, FALSE);
   if (earliest < curr_tip->vliw->time) earliest = curr_tip->vliw->time;

   curr_tip = insert_op (opcode, curr_tip, earliest, &ins_tip);
   curr_tip->br_type = BR_ONPAGE;
   curr_tip->br_addr = br_addr;
   curr_tip->br_link = link;
   if (was_bct) update_ccbit_for_bct (curr_tip);

   if (uncond) {
     curr_tip->br_ins  = opcode->op.ins;
     curr_tip->orig_ops++;

     if (use_pdf_cnts) prob = DBL_MAX/2;
     else	       prob = 1.00;

     add_to_xlate_list (curr_tip, br_addr, target_addr, prob, UNCOND_TARG,
			serialize);
   }
   else {
     curr_tip->br_condbit = latest_vliw_op->op.renameable[BI & (~OPERAND_BIT)];

     /* Handle conditional branch-and-links correctly */
     link_tip = curr_tip;

     fallthru_offpage = (br_addr + 1   == end) ||
			((fallthru_cnt == 0)   &&   pdf_0cnt_offpage &&
			 (use_pdf_cnts || use_pdf_prob));

     if (fallthru_offpage) {
        off_tip = add_leaf_br_tip (curr_tip, FALSE, BR_OFFPAGE);
        off_tip->right = (TIP *) (br_addr + 1);   /* Tell simul offpage addr */
        off_tip->left = 0;
        free_loophdr_mem (off_tip->loophdr);
        free_cluster_mem (off_tip->cluster);
     }

     if (!serialize) targ_type = COND_TARG;
     else {
	targ_type = UNCOND_TARG;
	curr_tip = add_leaf_br_tip (curr_tip, TRUE,  BR_ONPAGE);
     }

     link_tip->br_ins  = opcode->op.ins;
     link_tip->orig_ops++;

     if (use_pdf_cnts) {
        taken    = (double) taken_cnt;
        fallthru = (double) fallthru_cnt;
	probs_ok = TRUE;
     }
     else if (use_pdf_prob) {
	if (taken_cnt == 0  &&  fallthru_cnt == 0) probs_ok = FALSE;
	else {
	   taken    = ((double) taken_cnt) / (double) (taken_cnt+fallthru_cnt);
	   fallthru = 1.0 - taken;
	   probs_ok = TRUE;
	}
     }
     else probs_ok = FALSE;

     if (!probs_ok) {
	if (target_addr < br_addr) taken = loop_back_prob;

       /* Check if val < 0 often untaken error cond chk:  PPC specific code */
        else if (((ins >> 16) & 0x03) == 0) taken = negint_cmp_prob;

       /* Otherwise assume forward branch taken slightly less often than not */
        else taken = if_then_prob;

	fallthru = 1.0 - taken;
     }

     /* Put branch target in translate list */
     add_to_xlate_list (curr_tip, br_addr, target_addr, taken, targ_type,
			serialize);

     if (!fallthru_offpage) {

        /* Since cond branch, fall-thru always possible without serializing */
	add_to_xlate_list (link_tip, br_addr, br_addr+1, fallthru,
			   CDFALL_THRU, FALSE);
     }
   }

   return curr_tip;
}

/************************************************************************
 *									*
 *				handle_offind				*
 *				-------------				*
 *									*
 * NOTE:  We want offpage and indirect branches to always be		*
 *	  unconditional.  Other parts of the code assume that		*
 *	  conditional ops are SKIPS.  Hence conditional offpage		*
 *	  and indirect branches are converted to SKIPS followed		*
 *	  by an unconditional offpage/indirect branch.			*
 *									*
 ************************************************************************/

TIP *handle_offind (type, tip, opcode, ins, br_addr, end, targ_addr, uncond)
int	 type;			/* BR_OFFPAGE or BR_INDIR */
TIP	 *tip;
OPCODE2  *opcode;
unsigned ins;
unsigned *br_addr;
unsigned *end;
unsigned *targ_addr;
unsigned uncond;
{
   int	    serialize;
   int	    was_bct;
   int	    cr_bit;
   int	    earliest;
   int	    link = ins & LINK_BIT;		/* Boolean */
   int	    probs_ok;				/* Boolean */
   int	    fallthru_offpage;			/* Boolean */
   unsigned uncond_br;
   unsigned cond_br;
   unsigned taken_cnt;
   unsigned fallthru_cnt;
   double   taken;
   double   fallthru;
   double   prob;
   OPCODE1  *opcode1;
   TIP	    *save_tip;
   TIP	    *link_tip;
   TIP	    *off_tip;
   TIP	    *ins_tip;

   assert (type == BR_OFFPAGE  ||  (type & INDIR_BIT));
   assert (branch_type == NON_SUPERVISOR);

   /* Fill in our link register if need be */
   if (link) {
      if (type == BR_INDIR_LR) type = fixup_bcrl (&tip, opcode, &ins);
      tip = branch_and_link (tip, br_addr);
      serialize = TRUE;
      opcode->op.ins &= 0xFFFFFFFE;		/* Turn off link bit */
   }
   else serialize = FALSE;

   if (uncond) was_bct = FALSE;
   else {
      was_bct = handle_bcond (&tip, opcode);
      if (was_bct) ins = opcode->op.ins;	/* Can change if was BCT */
   }

   /* The branch can go no earlier than the current VLIW, but may have
      to go later, if an input (e.g. cond register bit) is not yet available.
   */
   earliest = get_earliest_time (tip, opcode, FALSE);
   if (earliest < tip->vliw->time) earliest = tip->vliw->time;

   if (uncond) {
     tip = insert_op (opcode, tip, earliest, &ins_tip);
     tip->br_ins  = opcode->op.ins;
     tip->br_type = type;
     tip->br_addr = br_addr;
     tip->br_link = link;
     tip->left    = 0;			/* Signals Leaf */
     tip->right   = (TIP *) targ_addr;	/* Tell simulator offpage address */
     tip->orig_ops++;

     free_loophdr_mem (tip->loophdr);
     free_cluster_mem (tip->cluster);
   }
   else {
     if (was_bct) cr_bit = opcode->op.renameable[BI & (~OPERAND_BIT)];

     /* Treat as simple conditional branch (bc), even if was BCT, BCR, or BCC.
	Then do uncond offpage/indir at end.
     */
     cond_br = 0x40000000 | (ins & 0x03FF0000);
     opcode1 = get_opcode (cond_br);
     opcode  = set_operands (opcode1, cond_br, opcode1->b.format);

     /* The bit is non-architected if was BCT. Fixup new opcode accordingly. */
     if (was_bct) replace_bc_crbit (opcode, cr_bit);

     tip = insert_op (opcode, tip, earliest, &ins_tip);
     tip->br_type    = BR_ONPAGE;
     tip->br_addr    = br_addr;
     tip->br_link    = link;
     tip->br_condbit = latest_vliw_op->op.renameable[BI & (~OPERAND_BIT)];
     if (was_bct) update_ccbit_for_bct (tip);

     save_tip = tip;

     link_tip = tip;

     if (use_pdf_cnts  ||  use_pdf_prob)
        get_condbr_cnts (tip, br_addr, &taken_cnt, &fallthru_cnt);

     fallthru_offpage = (br_addr + 1   == end) ||
			((fallthru_cnt == 0)   &&   pdf_0cnt_offpage &&
			 (use_pdf_cnts || use_pdf_prob));

     if (!fallthru_offpage) tip->left = 0;
     else {
        off_tip = add_leaf_br_tip (tip, FALSE, BR_OFFPAGE);
        off_tip->right = (TIP *) (br_addr + 1);   /* Tell simul offpage addr */
        off_tip->left = 0;
        free_loophdr_mem (off_tip->loophdr);
        free_cluster_mem (off_tip->cluster);
     }

     tip	    = add_leaf_br_tip (tip, TRUE, type);
     tip->right     = (TIP *) targ_addr;   /* Tell simulator offpage address */
     free_loophdr_mem (tip->loophdr);
     free_cluster_mem (tip->cluster);

     save_tip->br_ins = cond_br;
     save_tip->orig_ops++;

     if (!fallthru_offpage) {

       if (use_pdf_cnts) {
          taken    = (double) taken_cnt;
          fallthru = (double) fallthru_cnt;
	  probs_ok = TRUE;
       }

       else if (use_pdf_prob) {
	  if (taken_cnt == 0  &&  fallthru_cnt == 0) probs_ok = FALSE;
	  else {
	     taken    = ((double)taken_cnt) / (double)(taken_cnt+fallthru_cnt);
	     fallthru = 1.0 - taken;
	     probs_ok = TRUE;
	  }
       }
       else probs_ok = FALSE;

       if (!probs_ok) {
	  if (targ_addr < br_addr && (!(type & INDIR_BIT))) 
	     taken = loop_back_prob;

        /* Check if val < 0 often untaken error cond chk: PPC specific code */
          else if (((ins >> 16) & 0x03) == 0) taken = negint_cmp_prob;

        /* Otherwise assume forward branch taken slightly less often than not */
          else taken = if_then_prob;

	  fallthru = 1.0 - taken;
       }

       /* Since cond branch, fall-thru always possible without serializing */
       add_to_xlate_list (link_tip, br_addr, br_addr + 1, fallthru,
			  CDFALL_THRU, FALSE);
     }
   }

   return tip;
}

/************************************************************************
 *									*
 *				add_leaf_br_tip				*
 *				---------------				*
 *									*
 * Create a new tip, "br_tip", and place an unconditional branch on it.	*
 * Add "br_tip" to "in_tip" as either the right or left entry.  Since	*
 * room is always allowed for the leaf branch when skips are added, we	*
 * need not check resource usage, although we must increment it when	*
 * done.								*
 *									*
 * NOTE:  A random probability is assigned, because in all current	*
 *	  invocations, the new tip goes either to a serializing point	*
 *	  or to an OFFPAGE or INDIRECT location.			*
 *									*
 ************************************************************************/

TIP *add_leaf_br_tip (in_tip, right, type)
TIP    *in_tip;
int    right;		/* Boolean:  TRUE if add tip to right of "in_tip" */
int    type;		/* Branch type (e.g. BR_OFFPAGE) */
{
   int *resrc_cnt;
   TIP *br_tip;

   br_tip = get_tip ();
   if (right) {
      in_tip->right = br_tip;
      tree_pos_right (&in_tip->tp, &br_tip->tp);
   }
   else {
      in_tip->left = br_tip;
      tree_pos_left  (&in_tip->tp, &br_tip->tp);
   }

   br_tip->prev    = in_tip;
   br_tip->vliw    = in_tip->vliw;
   br_tip->br_type = type;
   br_tip->br_addr = curr_ins_addr;

   /* Make uncond ins */
   if       (type == BR_INDIR_CTR) br_tip->br_ins = 0x4E800420;
   else if  (type == BR_INDIR_LR)  br_tip->br_ins = 0x4E800020;
   else if  (type == BR_OFFPAGE)   br_tip->br_ins = 0x48000004;
   else if  (type == BR_ONPAGE)    br_tip->br_ins = 0x48000004;
   else assert (1 == 0);

   br_tip->num_ops = 0;
   br_tip->prob    = 0.12345;   /* SERIALIZING/OFFP/INDIR => prob not matter */

   br_tip->left    = 0;

   set_tip_regs (in_tip, br_tip, right);

   /* Initially, we've seen as many LD/STORE-UPDATES on this path since
      the VLIW start, as our predecessor tip.
   */
   mem_copy (br_tip->mem_update_cnt, in_tip->mem_update_cnt,
	     NUM_PPC_GPRS_C * sizeof (in_tip->mem_update_cnt[0]));

   resrc_cnt = in_tip->vliw->resrc_cnt;
#ifdef LEAF_BRANCHES_USE_ISSUE_SLOT
   resrc_cnt[ISSUE]++;
#endif
/* resrc_cnt[BRLEAF]++; */

   return br_tip;
}

/************************************************************************
 *									*
 *				replace_bc_crbit			*
 *				----------------			*
 *									*
 * Replace the condition register bit currently read in "bc_op" with	*
 * "cr_bit".  The opcode must be a conditional branch (including BCR	*
 * and BCC type instructions).						*
 *									*
 ************************************************************************/

replace_bc_crbit (bc_op, cr_bit)
OPCODE2 *bc_op;
int	cr_bit;
{
   unsigned	  i;
   unsigned	  num_rd = bc_op->op.num_rd;
   unsigned short *rd    = bc_op->op.rd;

   bc_op->op.renameable[BI & (~OPERAND_BIT)] = cr_bit;

   for (i = 0; i < num_rd; i++) {
      if (rd[i] >= CR0  &&  rd[i] < CR0 + num_ccbits) {
	 rd[i] = cr_bit;
	 return;
      }
   }

   assert (1 == 0);
}

/************************************************************************
 *									*
 *				add_page_hash_entries			*
 *				---------------------			*
 *									*
 * Should be called to translate VLIW code back into PPC code for	*
 * simulation.  (As each VLIW is translated, its translation address	*
 * field should be filled.)  Adds to the hash table, all the		*
 * serialization/entry points encountered during this translation of	*
 * this page.								*
 *									*
 ************************************************************************/

add_page_hash_entries (kernel_svc_entry)
int kernel_svc_entry;				  /* Boolean */
{
   int	    i;
   unsigned *xlate_addr;
   unsigned *ppc_addr;
   VLIW	    *vliw;

   for (i = 0; i < hash_table_page_cnt; i++) {
      ppc_addr = hash_table_page_list[i].ppc;
      vliw     = hash_table_page_list[i].vliw;

      if (ppc_addr == svc_instr_addr  &&  kernel_svc_entry)
           xlate_addr = kernel_svc_xlate;
      else xlate_addr = vliw->xlate;

      add_hash_entry (ppc_addr, xlate_addr);
   }
}

/************************************************************************
 *									*
 *				vliw_dump				*
 *				---------				*
 *									*
 * Dump the VLIW's produced on the most recent invocation of		*
 * "xlate_entry".							*
 *									*
 ************************************************************************/

vliw_dump ()
{
   int  i;
   int  fd_vliw_dump;
   VLIW *vliw;

   fd_vliw_dump = reopen_dump_file ();

   for (i = 0; i < hash_table_page_cnt; i++) {
      vliw = hash_table_page_list[i].vliw;
      vliw_dump_group (fd_vliw_dump, vliw, vliw->group);
   }

   close (fd_vliw_dump);
}

/************************************************************************
 *									*
 *			     vliw_to_ppc_simcode			*
 *			     -------------------			*
 *									*
 * Translate the VLIW instructions into PPC simulation code.  Call	*
 * the main translation routine once for each VLIW group.		*
 *									*
 ************************************************************************/

vliw_to_ppc_simcode ()
{
   int  i;
   VLIW *vliw;

   for (i = 0; i < hash_table_page_cnt; i++) {
      vliw = hash_table_page_list[i].vliw;
      vliw_to_ppc_group (vliw, vliw->group);
   }

   patch_vliw_xlate_pairs ();
}

/************************************************************************
 *									*
 *				setup_regs				*
 *				----------				*
 *									*
 * Make call to assembly language routine which initializes r13 to	*
 * point to VLIW registers.						*
 *									*
 ************************************************************************/

setup_regs (argc, argv, env)
int  argc;
char *argv[];
char *env;
{
   PTRGL *xe_raw     = (PTRGL *) xlate_entry_raw;
   PTRGL *lvfail_raw = (PTRGL *) loadver_fail_raw;

   setup_r13 (r13_area);

#ifdef PARM_REGS_NOT_REAL
   /* Setup the arguments in the standard registers */
   *((unsigned *) &r13_area[R3_OFFSET]) = argc;
   *((unsigned *) &r13_area[R4_OFFSET]) = argv;
   *((unsigned *) &r13_area[R5_OFFSET]) = env;
#endif

   *((unsigned *) &r13_area[PROG_START_OFFSET]) = prog_start;
   *((unsigned *) &r13_area[PROG_END_OFFSET])   = prog_end;
   *((unsigned *) &r13_area[LSX_AREA])		= lsx_area;
   *((unsigned *) &r13_area[HASH_TABLE])	= hash_table;
   *((unsigned *) &r13_area[XLATE_RAW])		= xe_raw->func;
   *((unsigned *) &r13_area[LDVER_FCN])		= lvfail_raw->func;
   *((unsigned *) &r13_area[VLIW_PATH_CNTS])	= perf_cnts;
   *((unsigned *) &r13_area[LD_VER_FAIL_CNT])	= 0;
   *((unsigned *) &r13_area[DYNPAGE_CNT])	= 0;
   *((unsigned *) &r13_area[DYNPAGE_LIM])	= max_dyn_page_cnt;

   /* Setup stack and allow xlate_entry to place values in what
      it thinks is the caller's stack area.
   */
   *((unsigned *) &r13_area[XLATE_ENTRY_STACK]) = 
	&xlate_entry_stack[XLATE_ENTRY_STK_SIZE-0x100];
}

/************************************************************************
 *									*
 *				init_dump_file				*
 *				--------------				*
 *									*
 * Call once before begin translation.  Wipes out any old dump file and	*
 * makes sure can open new one.						*
 *									*
 ************************************************************************/

static init_dump_file ()
{
   FILE *fd_vliw_dump;

   /* Get fully qualified path name, so can reopen even if application
      changes directory.
   */
   strcpy (daisy_vliw_file, curr_wd);
   strcat (daisy_vliw_file, "/daisy.vliw");

   /* Destroy any existing file */
   fd_vliw_dump = open (daisy_vliw_file, O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (fd_vliw_dump == -1) {
      fprintf (stderr, "Could not open \"daisy.vliw\" to dump VLIW translation\n");
      exit (1);
   }
   close (fd_vliw_dump);
}

/************************************************************************
 *									*
 *				reopen_dump_file			*
 *				----------------			*
 *									*
 * Call repeatedly as translate each entry point.  Reopens "daisy.vliw"	*
 * file for append writing.  We want to open and close it each time	*
 * so that when we stop, we will always have the full translation	*
 * dumped to disk.							*
 *									*
 ************************************************************************/

static int reopen_dump_file ()
{
   int fd_vliw_dump;

   fd_vliw_dump = open (daisy_vliw_file, O_WRONLY);/* Append to existing file */
   if (fd_vliw_dump == -1) {
      fprintf (stderr, "Could not open \"daisy.vliw\" to append VLIW translation\n");
      exit (1);
   }
   else lseek (fd_vliw_dump, 0, SEEK_END);	/* Go to end to append */

   return fd_vliw_dump;
}

/************************************************************************
 *									*
 *				main_reexec_info			*
 *				----------------			*
 *									*
 ************************************************************************/

int main_reexec_info (fd, fcn)
int fd;
int (*fcn) ();			/* read or write */
{
   unsigned *curr_prog_start = prog_start;
   assert (fcn (fd, &prog_start, sizeof (prog_start)) == sizeof (prog_start));

   prog_start_diff = curr_prog_start - prog_start;
   if (prog_start != curr_prog_start) {
      fprintf (stderr, "WARNING: Program not loaded in same place as before.\n");
/*    exit (1); */
   }

   assert (fcn (fd, &xlate_entry_cnt, sizeof (xlate_entry_cnt)) ==
	   sizeof (xlate_entry_cnt));

   assert (fcn (fd, &total_ppc_ops, sizeof (total_ppc_ops)) ==
	   sizeof (total_ppc_ops));

   assert (fcn (fd, &hash_table_page_cnt, sizeof (hash_table_page_cnt)) ==
	   sizeof (hash_table_page_cnt));

   prog_end += prog_start_diff;
   return prog_start_diff * sizeof (*prog_start);
}

/************************************************************************
 *									*
 *				init_svc				*
 *				--------				*
 *									*
 * Set the address of "svc_instr" in /unix by the version of AIX being	*
 * run.									*
 *									*
 ************************************************************************/

static init_svc ()
{
   if (aixver < 430) svc_instr_addr = 0x3600;
   else              svc_instr_addr = 0x37D0;
}
