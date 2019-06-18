/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliwcnts.h"
#include "dis.h"
#include "translate.h"
#include "parse.h"
#include "instrum.h"
#include "op_mapping.h"

#define DEFAULT_VLIWINP_LINES		65536
#define DEFAULT_NUM_GROUP_EXITS		 1024

typedef struct {
   unsigned	  group_entry_vliw;
   unsigned	  ppc_code_addr;
   unsigned	  vmm_invoc_cnt;
   unsigned	  tip_label;
   unsigned short cycles_from_start;
   unsigned char  same_group;			/* Boolean */
   unsigned	  times_exec;
   double	  num_group_cycles;
   double	  ilp;
} GROUP_EXIT;

typedef struct {
   int	  first;	/* Flag used to indicate if 1st time seeing "skip" */
   OPCODE *op;
} POPS;

FILE *fp_cs;

extern double     total_vliw_user_ins;
extern double     total_vliw_ins;
extern double	  total_orig_user_ops;
extern double	  total_orig_ops;
extern double	  total_perf_ops;
extern double	  total_spec_ops;
extern double	  *offset_perf_histo;
extern double     *offset_spec_histo;
extern char	  curr_fname[];
extern OP_MAPPING *op_map_table[];

extern OPCODE     *br_op;
extern OPCODE     *eiu_op;
extern OPCODE     *skip_op;
extern OPCODE     *xerx_op;

extern unsigned	  num_times_exec;

extern int	  group_exits_by_cycles;	/* Flag */
extern int	  group_exits_by_entry;		/* Flag */

extern int	  make_cnts_file;
extern int	  skip_order;
extern int	  make_pdf_file;

static double   *vliwinp_cnt;
static unsigned vliwinp_alloc;

static double	spec_ins_cnt;
static double	perf_ins_cnt;
static double	fall_thru_cnt;

static double   num_vliws_exec;
static double   num_static_vliws;

/* We can have from 0 to OPS_PER_VLIW operations per instruc */
static double perf_alu_histo[OPS_PER_VLIW+1];
static double perf_mem_histo[OPS_PER_VLIW+1];
static double perf_bra_histo[OPS_PER_VLIW+1];
static double perf_alu_mem_histo[OPS_PER_VLIW+1];
static double perf_alu_mem_bra_histo[OPS_PER_VLIW+1];

static double spec_alu_histo[OPS_PER_VLIW+1];
static double spec_mem_histo[OPS_PER_VLIW+1];
static double spec_bra_histo[OPS_PER_VLIW+1];
static double spec_alu_mem_histo[OPS_PER_VLIW+1];
static double spec_alu_mem_bra_histo[OPS_PER_VLIW+1];

static int	  group_exit_num;
static GROUP_EXIT *group_exit;

#ifndef MUST_MODIFY_FOR_DAISY
       OPCNT  perf_op_cnt[512];
       OPCNT  spec_op_cnt[512];
#else
       OPCNT  perf_op_cnt[NUM_OPCODES];
       OPCNT  spec_op_cnt[NUM_OPCODES];
#endif
       double skip_op_perf_cnt;
       double skip_op_spec_cnt;

int      cmp_exit_cycles  (GROUP_EXIT *, GROUP_EXIT *);
int      cmp_exit_entries (GROUP_EXIT *, GROUP_EXIT *);
double   get_exits_per_entry (int *, int*);
unsigned get_num_paths_covering (int);

/************************************************************************
 *									*
 *				open_cs_file				*
 *				------------				*
 *									*
 * Open file where put how many times each VLIW instruction was		*
 * executed and other information, plus how many times the supported	*
 * set of library routines were called.	 The name of this file was	*
 * formerly "_counts.script".						*
 *									*
 ************************************************************************/

open_cs_file (name)
char *name;
{
   char buff[128];

   if (!make_cnts_file) return;

   strcpy (buff, name);
   strcat (buff, ".cnts");

   fp_cs = fopen (buff, "w");
   if (!fp_cs) {
      printf ("Error:  Could not open \"%s\" for writing.\n", buff);
      exit (1);
   }

   fprintf (fp_cs, "(00000 %-25s %10.0f 100.0000 %10u %10u)\n",
	    "\"*TOTAL*\"", total_vliw_ins, 0, 0);
}

/************************************************************************
 *									*
 *				cs_perf_ins				*
 *				-----------				*
 *									*
 ************************************************************************/

void cs_perf_ins (times_exec, index, num_ops, orig_ops, num_branches, 
		  vliwinp_line, opcode, path_num, offset_in_file, user_code,
		  group_entry_vliw, ppc_code_addr, vmm_invoc_cnt,
		  tip_label, cycles_from_start, 
		  num_conds, branch_dir, branch_addr, same_group,
		  path_orig_ops)
unsigned       times_exec;
unsigned       index;
unsigned       num_ops;
unsigned       orig_ops;
unsigned       num_branches;
unsigned       *vliwinp_line;
unsigned char  *opcode;
unsigned       path_num;
unsigned       offset_in_file;
unsigned       user_code;
unsigned       group_entry_vliw;
unsigned       ppc_code_addr;
unsigned       vmm_invoc_cnt;
unsigned       tip_label;
unsigned short cycles_from_start;
int	       num_conds;
unsigned char  *branch_dir;
unsigned       *branch_addr;
unsigned       same_group;
unsigned       path_orig_ops;

{
   double dtimes_exec = (double) times_exec;

   spec_ins_cnt += dtimes_exec;
   perf_ins_cnt += (num_ops + num_branches) * dtimes_exec;

   if (path_num > 0) fall_thru_cnt += dtimes_exec;
}

/************************************************************************
 *									*
 *				cs_spec_ins				*
 *				-----------				*
 *									*
 * For each specified Instruction, INS, print to former			*
 * "_counts.script":							*
 *									*
 * 1 Index[INS]								*
 * 2 Instruction Label[INS]						*
 * 3 Times Specified to Execute[INS]					*
 * 4 Percent of Total Instructions[INS]					*
 * 5 Number of Times Take non-Default Path[INS] (i.e. pathno[INS] > 0)	*
 * 6 Wastage:  17 x (Times Specified to Execute[INS]) -			*
 *	       (Ops Performed[INS])					*
 *									*
 ************************************************************************/

void cs_spec_ins (name, index, num_branches, num_ops, opcode, offset,
		  offset_in_file, num_bytes)
char	       *name;
unsigned       index;
int	       num_branches;
int	       num_ops;
unsigned char  *opcode;
unsigned       *offset;
unsigned       offset_in_file;
unsigned       num_bytes;
{
   char buff[MAX_TOKEN_SIZE];

   if (make_cnts_file) {
      buff[0] = '\"';
      strcpy (&buff[1], name);
      strcat (buff, "\"");

      fprintf (fp_cs, "(%05d %-25s %10.0f %8.4f %10.0f %10.0f)\n",
	       index, buff, spec_ins_cnt,
	       (spec_ins_cnt * 100.0) / total_vliw_ins,
	       fall_thru_cnt, 17.0 * spec_ins_cnt - perf_ins_cnt);
   }

   if (spec_ins_cnt > 0.0) num_vliws_exec++;
   num_static_vliws++;

   spec_ins_cnt = 0.0;
   perf_ins_cnt = 0.0;
   fall_thru_cnt = 0.0;
}

/************************************************************************
 *									*
 *				cs_lib_calls				*
 *				------------				*
 *									*
 * For each specified Library CALL, print to former "_counts.script" as	*
 * in "cs_spec_ins" above.						*
 *									*
 ************************************************************************/

void cs_lib_calls (name, index, cnt)
char     *name;
unsigned index;
unsigned cnt;
{
   char buff[MAX_TOKEN_SIZE];

   if (!make_cnts_file) return;

   buff[0] = '\"';
   strcpy (&buff[1], name);
   strcat (buff, "\"");

   fprintf (fp_cs, "(%05d %-25s %10u %8.4f %10u %10u)\n",
	    index, buff, cnt, (((double) cnt) * 100.0) / total_vliw_ins, 0, 0);
}

/************************************************************************
 *									*
 *				ignore_lib_calls			*
 *				----------------			*
 *									*
 * Do nothing--just stub to match "cs_lib_calls".			*
 *									*
 ************************************************************************/

void ignore_lib_calls (name, index, cnt)
char     *name;
unsigned index;
unsigned cnt;
{
}

/************************************************************************
 *									*
 *				count_perf_ins				*
 *				--------------				*
 *									*
 * Update the histograms for performed operations.  Also bump the count	*
 * of total VLIW instructions executed.					*
 *									*
 ************************************************************************/

void count_perf_ins (times_exec, index, num_ops, orig_ops, bra_ops, 
		     vliwinp_line, opcode, path_num, offset_in_file, user_code,
		     group_entry_vliw, ppc_code_addr, vmm_invoc_cnt,
		     tip_label, cycles_from_start,
		     num_conds, branch_dir, branch_addr, same_group,
		     path_orig_ops)
unsigned       times_exec;
unsigned       index;
unsigned       num_ops;
unsigned       orig_ops;
unsigned       bra_ops;
unsigned       *vliwinp_line;
unsigned char  *opcode;
unsigned       path_num;
unsigned       offset_in_file;
unsigned       user_code;
unsigned       group_entry_vliw;
unsigned       ppc_code_addr;
unsigned       vmm_invoc_cnt;
unsigned       tip_label;
unsigned short cycles_from_start;
int	       num_conds;
unsigned char  *branch_dir;
unsigned       *branch_addr;
unsigned       same_group;
unsigned       path_orig_ops;
{
   int    i;
   int    optype;
   int    alu_ops = 0;
   int    mem_ops = 0;
   int	  match_cnt;
   double dtimes_exec = (double) times_exec;
   POPS   *op_list;			/* Elem 0 = size of list */

   /* Don't waste time counting opcodes in path that never executes */
   if (times_exec == 0) return;

   total_vliw_ins += dtimes_exec;
   spec_ins_cnt   += dtimes_exec;
   total_orig_ops += orig_ops * dtimes_exec;

   if (user_code) {
      total_vliw_user_ins += dtimes_exec;
      total_orig_user_ops += orig_ops * dtimes_exec;
   }

   if (cycles_from_start > 0  &&
      (group_exits_by_cycles  ||  group_exits_by_entry)) {

      save_condbranch_info (num_conds, branch_dir, branch_addr, times_exec);

      check_group_exit_mem ();
      group_exit[group_exit_num].group_entry_vliw  = group_entry_vliw;
      group_exit[group_exit_num].ppc_code_addr     = ppc_code_addr;
      group_exit[group_exit_num].vmm_invoc_cnt     = vmm_invoc_cnt;
      group_exit[group_exit_num].same_group	   = (same_group) ? 1 : 0;
      group_exit[group_exit_num].tip_label         = tip_label;
      group_exit[group_exit_num].cycles_from_start = cycles_from_start;
      group_exit[group_exit_num].times_exec        = times_exec;
      group_exit[group_exit_num].num_group_cycles  = dtimes_exec *
						     cycles_from_start;
      group_exit[group_exit_num].ilp		   = 
		 ((double) path_orig_ops) / (double) cycles_from_start;

      group_exit_num++;
   }

   total_perf_ops += (num_ops + bra_ops) * dtimes_exec;
   skip_op_perf_cnt += bra_ops * dtimes_exec;

   for (i = 0; i < num_ops; i++) {
      offset_histo_add (vliwinp_line[i], offset_perf_histo, dtimes_exec);

      perf_op_cnt[opcode[i]].cnt += dtimes_exec;
      optype = op_map_table[opcode[i]]->optype;
      if (optype & ALU_OP) alu_ops++;
      else if (optype & MEM_OP) mem_ops++;
      else {
	 printf ("Unexpected optype (0x%04x) neither ALU nor MEM\n", optype);
	 exit (1);
      }
   }

   perf_alu_histo[alu_ops] += dtimes_exec;
   perf_mem_histo[mem_ops] += dtimes_exec;
   perf_bra_histo[bra_ops] += dtimes_exec;
   perf_alu_mem_histo[alu_ops+mem_ops] += dtimes_exec;
   perf_alu_mem_bra_histo[alu_ops+mem_ops+bra_ops] += dtimes_exec;

   if (make_pdf_file) update_vliwinp_cnts (num_ops, vliwinp_line, dtimes_exec);
}

/************************************************************************
 *									*
 *				count_spec_ins				*
 *				--------------				*
 *									*
 * Update the histograms for specified operations.			*
 *									*
 ************************************************************************/

void count_spec_ins (name, index, bra_ops, num_ops, opcode, offset,
		     offset_in_file, num_bytes)
char	       *name;
unsigned       index;
int	       bra_ops;
int	       num_ops;
unsigned char  *opcode;
unsigned       *offset;
unsigned       offset_in_file;
unsigned       num_bytes;
{
   int	    i;
   int	    optype;
   int	    alu_ops = 0;
   int	    mem_ops = 0;
   int	    match_cnt;
   unsigned *op;
   OPCODE   *op_code;

   /* Don't waste time counting opcodes in path that never executes */
   if (spec_ins_cnt == 0.0) return;

   total_spec_ops += (num_ops + bra_ops) * spec_ins_cnt;
   skip_op_spec_cnt += bra_ops * spec_ins_cnt;

   for (i = 0; i < num_ops; i++) {
      offset_histo_add (offset[i], offset_spec_histo, spec_ins_cnt);

      spec_op_cnt[opcode[i]].cnt += spec_ins_cnt;

      optype = op_map_table[opcode[i]]->optype;
      if (optype & ALU_OP) alu_ops++;
      else if (optype & MEM_OP) mem_ops++;
      else {
	 printf ("Unexpected optype (0x%04x) neither ALU nor MEM\n", optype);
	 exit (1);
      }
   }

   spec_alu_histo[alu_ops] += spec_ins_cnt;
   spec_mem_histo[mem_ops] += spec_ins_cnt;
   spec_bra_histo[bra_ops] += spec_ins_cnt;
   spec_alu_mem_histo[alu_ops+mem_ops] += spec_ins_cnt;
   spec_alu_mem_bra_histo[alu_ops+mem_ops+bra_ops] += spec_ins_cnt;

   spec_ins_cnt = 0;
}

/************************************************************************
 *									*
 *				init_vliwinp_cnts			*
 *				-----------------			*
 *									*
 * Should be called every time processing of a new source file		*
 * ("curr_fname") is begun.						*
 *									*
 ************************************************************************/

void init_vliwinp_cnts ()
{
   /* Start with default # of lines for first file.  Others use previous max */
   if (!vliwinp_cnt) vliwinp_alloc = DEFAULT_VLIWINP_LINES;

   /* Any vliwinp_cnt for a previous source file should have been
      free'd by "finish_vliwinp_cnts".
   */
   vliwinp_cnt = (double *) calloc (vliwinp_alloc, sizeof (vliwinp_cnt[0]));
   assert (vliwinp_cnt);
}

/************************************************************************
 *									*
 *				finish_vliwinp_cnts			*
 *				-------------------			*
 *									*
 * Should be called every time processing of a new source file		*
 * is finished.  It produces "fname.pdf" which has the following	*
 * format for every .vliwinp line which is executed at least once	*
 *									*
 *	<Line Number>:  <Number of Times Executed>			*
 *									*
 ************************************************************************/

void finish_vliwinp_cnts (fname)
char *fname;
{
   int  i;
   FILE *fp_pdf;
   char buff[512];

   strcpy (buff, fname);
   strcat (buff, ".pdf");

   fp_pdf = fopen (buff, "w");
   assert (fp_pdf);

   for (i = 0; i < vliwinp_alloc; i++) {
      if (vliwinp_cnt[i] > 0)
	 fprintf (fp_pdf, " %4d:  %10.0f\n", i, vliwinp_cnt[i]);
   }

   fclose (fp_pdf);
   free (vliwinp_cnt);
}

/************************************************************************
 *									*
 *				update_vliwinp_cnts			*
 *				-------------------			*
 *									*
 ************************************************************************/

update_vliwinp_cnts (num_ops, vliwinp_line, dtimes_exec)
int    num_ops;
int    *vliwinp_line;
double dtimes_exec;
{
   int i, j;
   int line_num;
   int old_vliwinp_alloc;

   for (i = 0; i < num_ops; i++) {
      line_num = vliwinp_line[i];

      if (line_num >= vliwinp_alloc) {
         old_vliwinp_alloc = vliwinp_alloc;
         vliwinp_alloc = 2 * line_num;
	 vliwinp_cnt = (double *) realloc (vliwinp_cnt, vliwinp_alloc * sizeof (vliwinp_cnt[0]));
         assert (vliwinp_cnt);

         for (j = old_vliwinp_alloc; j < vliwinp_alloc; j++) {
	    vliwinp_cnt[j] = 0;
	 }
      }

      vliwinp_cnt[line_num] += dtimes_exec;
   }
}

/************************************************************************
 *									*
 *				dump_perf_ins				*
 *				-------------				*
 *									*
 ************************************************************************/

void dump_perf_ins (times_exec, index, num_ops, orig_ops, num_branches,
		    vliwinp_line, opcode, path_num, offset_in_file)
unsigned       times_exec;
unsigned       index;
unsigned       num_ops;
unsigned       orig_ops;
unsigned       num_branches;
unsigned       *vliwinp_line;
unsigned char  *opcode;
unsigned       path_num;
unsigned       offset_in_file;
{
   int i;

   total_vliw_ins += (double) times_exec;

   printf ("%s %4u:  %6u)  %u BR  ",
	   curr_fname, index, times_exec, num_branches);

   for (i = 0; i < num_ops; i++) {
      printf ("%u ", opcode[i]);
   }

   printf ("\n");
}

/************************************************************************
 *									*
 *				dump_spec_ins				*
 *				-------------				*
 *									*
 ************************************************************************/

void dump_spec_ins (name, index, num_branches, num_ops, opcode, offset,
		    offset_in_file, num_bytes)
char	       *name;
unsigned       index;
int	       num_branches;
int	       num_ops;
unsigned char  *opcode;
unsigned       *offset;
unsigned       offset_in_file;
unsigned       num_bytes;
{
   int i;

   printf ("\n%s:  %u BR  ", name, num_branches);

   for (i = 0; i < num_ops; i++) {
      printf ("%u ", opcode[i]);
   }

   printf ("\n");
}

/************************************************************************
 *									*
 *				dump_nonzero_vliws_to_file		*
 *				--------------------------		*
 *									*
 ************************************************************************/

dump_nonzero_vliws_to_file (name)
char *name;
{
   FILE *fp;
   char buff[128];

   strcpy (buff, name);
   strcat (buff, ".non0");

   fp = fopen (buff, "w");
   if (!fp) {
      printf ("Could not open file %s for writing\n", buff);
      exit (1);
   }

   fprintf (fp, "%7.0f    VLIW's executed at least once\n", num_vliws_exec);
   fprintf (fp, "%7.0f    VLIW's total (static count)\n",   num_static_vliws);
   fprintf (fp, "%9.1f%% Ratio\n", (100.0*num_vliws_exec) / num_static_vliws);

   fclose (fp);
}

/************************************************************************
 *									*
 *				dump_all_histo_to_file			*
 *				----------------------			*
 *									*
 ************************************************************************/

dump_all_histo_to_file (name)
char *name;
{
   FILE *fp;
   char buff[128];

   strcpy (buff, name);
   strcat (buff, ".histo");

   fp = fopen (buff, "w");
   if (!fp) {
      printf ("Could not open file %s for writing\n", buff);
      exit (1);
   }

   fprintf (fp, "%7.0f VLIW      Instructions Executed over %u run%c\n", 
	    total_vliw_ins, num_times_exec, (num_times_exec == 1) ? ' ' : 's');

   fprintf (fp, "%7.0f VLIW User Instructions Executed over %u run%c\n\n", 
	    total_vliw_user_ins, num_times_exec, (num_times_exec == 1) ? ' ' : 's');

   fprintf (fp, "%7.0f Dynamic PPC Ops in Original Program with shared libs\n\n",
	    total_orig_ops);
   fprintf (fp, "       PPC Path Length\n%4.1f = ----------------\n",
	    total_orig_ops / total_vliw_ins);
   fprintf (fp, "       VLIW Path Length\n\n");

   fprintf (fp, "%7.0f Dynamic PPC Ops in Original Program -- User only\n\n",
	    total_orig_user_ops);
   fprintf (fp, "       PPC Path Length\n%4.1f = ----------------\n",
	    total_orig_user_ops / total_vliw_user_ins);
   fprintf (fp, "       VLIW Path Length\n\n");

   fprintf (fp,   "Specified ALU Operations Histogram:            ");
   dump_histo (fp, spec_alu_histo);

   fprintf (fp, "\nSpecified MEM Operations Histogram:            ");
   dump_histo (fp, spec_mem_histo);

   fprintf (fp, "\nSpecified BRA Operations Histogram:            ");
   dump_histo (fp, spec_bra_histo);

   fprintf (fp, "\nSpecified ALU+MEM Operations Histogram:        ");
   dump_histo (fp, spec_alu_mem_histo);

   fprintf (fp, "\nSpecified ALU+MEM+BRANCH Operations Histogram: ");
   dump_histo (fp, spec_alu_mem_bra_histo);

   fprintf (fp, "\n-----------------------------------------------------------------------------\n");

   fprintf (fp, "\nPerformed ALU Operations Histogram:            ");
   dump_histo (fp, perf_alu_histo);

   fprintf (fp, "\nPerformed MEM Operations Histogram:            ");
   dump_histo (fp, perf_mem_histo);

   fprintf (fp, "\nPerformed BRA Operations Histogram:            ");
   dump_histo (fp, perf_bra_histo);

   fprintf (fp, "\nPerformed ALU+MEM Operations Histogram:        ");
   dump_histo (fp, perf_alu_mem_histo);

   fprintf (fp, "\nPerformed ALU+MEM+BRANCH Operations Histogram: ");
   dump_histo (fp, perf_alu_mem_bra_histo);

   plot_all_hist (fp);

   fclose (fp);
}

/************************************************************************
 *									*
 *				plot_all_histo				*
 *				--------------				*
 *									*
 ************************************************************************/

plot_all_hist (fp)
FILE *fp;
{
   init_plot ();

   plot_histo (fp, spec_alu_histo, "Specified ALU Operations");
   plot_histo (fp, spec_mem_histo, "Specified MEM Operations");
   plot_histo (fp, spec_bra_histo, "Specified BRA Operations");
   plot_histo (fp, spec_alu_mem_histo, "Specified ALU+MEM Operations");
   plot_histo (fp, spec_alu_mem_bra_histo,"Specified ALU+MEM+BRANCH Operations");

   /*-------------------------------------------------------------------*/

   plot_histo (fp, perf_alu_histo, "Performed ALU Operations");
   plot_histo (fp, perf_mem_histo, "Performed MEM Operations");
   plot_histo (fp, perf_bra_histo, "Performed BRA Operations");
   plot_histo (fp, perf_alu_mem_histo, "Performed ALU+MEM Operations");
   plot_histo (fp, perf_alu_mem_bra_histo, "Performed ALU+MEM+BRANCH Operations");
}

/************************************************************************
 *									*
 *				dump_histo				*
 *				----------				*
 *									*
 ************************************************************************/

dump_histo (fp, histo)
FILE     *fp;
double   *histo;
{
   int	    i;
   double   total_ops = 0.0;
   double   percent;
   double   cum = 0.0;

   for (i = 1; i < OPS_PER_VLIW + 1; i++)
      total_ops += histo[i] * (double) i;

   fprintf (fp, "%9.0f ops (%4.1f ops / ins)\n",
	    total_ops, total_ops / total_vliw_ins);

   for (i = 0; i < OPS_PER_VLIW + 1; i++) {
      if (histo[i] != 0.0) {

	 percent = (histo[i] * 100.0) / total_vliw_ins;
	 cum += percent;

	 fprintf (fp, "%2d: %10.0f (%6.2f%%  %6.2f%% cumulative)\n",
		  i, histo[i], percent, cum);
      }
   }
}

/************************************************************************
 *									*
 *				display_counts				*
 *				--------------				*
 *									*
 ************************************************************************/

display_counts ()
{
   printf ("\n%%1.0f VLIW Instructions Executed Total\n\n", total_vliw_ins);
}

/************************************************************************
 *									*
 *				dump_group_exit_info			*
 *				--------------------			*
 *									*
 * We sometimes do 3 qsorts, but only need 2 if we were more efficient.	*
 *									*
 ************************************************************************/

dump_group_exit_info (fname)
char *fname;
{
   unsigned i;
   unsigned vic;
   unsigned cover_paths;
   unsigned te;
   unsigned nc;
   char     te_suffix[2];
   char     nc_suffix[2];
   int	    max_exits;
   int	    max_exits_group;
   FILE     *fp_exits;
   char	    *filler;
   char     buff[512];
   int	    (*cmp_fcn) ();
   double   mean_exits_per_entry;
   double   cum_group_cycles = 0.0;
   double   group_transitions;
   double   loop_transitions;

   if      (group_exits_by_cycles) cmp_fcn = cmp_exit_cycles;
   else if (group_exits_by_entry)  cmp_fcn = cmp_exit_entries;
   else				   return;

   strcpy (buff, fname);
   strcat (buff, ".exits");

   fp_exits = fopen (buff, "w");
   assert (fp_exits);

   qsort (group_exit, group_exit_num, sizeof (group_exit[0]), cmp_exit_entries);
   mean_exits_per_entry = get_exits_per_entry (&max_exits,&max_exits_group);

   fprintf (fp_exits, "%4.1f Group Exits Executed Per Group Entry (Mean)\n",
	    mean_exits_per_entry);
   fprintf (fp_exits, "%4d Group Exits Executed Per Group Entry (Max, e.g. V%d)\n\n",
	    max_exits, max_exits_group);

   qsort (group_exit, group_exit_num, sizeof (group_exit[0]), cmp_exit_cycles);
   cover_paths = get_num_paths_covering (90);
   fprintf (fp_exits, "90%% of cycles covered by %u group paths\n\n",
	    cover_paths);

   group_transitions = 0.0;
   loop_transitions  = 0.0;
   for (i = 0; i < group_exit_num; i++) {
      group_transitions   += group_exit[i].times_exec;
      if (group_exit[i].same_group)
         loop_transitions += group_exit[i].times_exec;
   }

   fprintf (fp_exits,"%11.0f Groups Executed Dynamically\n", group_transitions);
   fprintf (fp_exits,"%11.0f (%2.0f%%) looped to themselves\n", 
	    loop_transitions, (100.0 * loop_transitions) / group_transitions);
   fprintf (fp_exits,"%11.1f ILP limit from group serializations\n\n",
	    total_orig_ops / group_transitions);

   dump_branch_stats (fp_exits);

   if (!group_exits_by_cycles)
      qsort (group_exit, group_exit_num, sizeof (group_exit[0]), cmp_fcn);

   fprintf (fp_exits, "                                           Num      Num\n");
   fprintf (fp_exits, "                                           Cycles:  Times\n");
   fprintf (fp_exits, " Group   Address in                        Group    Exit    Total    %% | Cum %%\n");
   fprintf (fp_exits, " Start    Original         Group           Start    TIP    Cycles  of Program\n");
   fprintf (fp_exits, " VLIW #     Code          Exit TIP     ILP to End   Execs  on Path    Cycles\n");
   fprintf (fp_exits, "-------  ----------     ------------   --- ------   -----  ------- ----  ----\n");

   for (i = 0; i < group_exit_num; i++) {

      vic = group_exit[i].vmm_invoc_cnt;
      if      (vic < 10)     filler = "     ";
      else if (vic < 100)    filler = "    ";
      else if (vic < 1000)   filler = "   ";
      else if (vic < 10000)  filler = "  ";
      else if (vic < 100000) filler = " ";
      else		     filler = "";

      make_readable ((double) group_exit[i].times_exec,       &te, te_suffix);
      make_readable ((double) group_exit[i].num_group_cycles, &nc, nc_suffix);

      cum_group_cycles += group_exit[i].num_group_cycles;
      fprintf (fp_exits, "%6u   0x%08x %sT%u_%08x%s  %3.1f %4u     %4u%s    %4u%s   %2.0f   %3.0f\n",
	       group_exit[i].group_entry_vliw,
	       group_exit[i].ppc_code_addr,		filler,
	       vic,
	       group_exit[i].tip_label,
	      (group_exit[i].same_group) ? "*" : " ",
	       group_exit[i].ilp,
	       group_exit[i].cycles_from_start,
	       te, te_suffix,
	       nc, nc_suffix,

	       (100.0 * group_exit[i].num_group_cycles) / total_vliw_ins,
	       (100.0 * cum_group_cycles)		/ total_vliw_ins);
   }

   fclose (fp_exits);
}

/************************************************************************
 *									*
 *				cmp_exit_cycles				*
 *				---------------				*
 *									*
 * For qsort to sort group exits by the number of cycles consumed on	*
 * this path between group start and group exit.			*
 *									*
 ************************************************************************/

int cmp_exit_cycles (a, b)
GROUP_EXIT *a;
GROUP_EXIT *b;
{
   if      (a->num_group_cycles < b->num_group_cycles) return  1;
   else if (a->num_group_cycles > b->num_group_cycles) return -1;
   else						       return  0;
}

/************************************************************************
 *									*
 *				cmp_exit_entries			*
 *				----------------			*
 *									*
 * For qsort to sort group exits by the (1) entry VLIW of the group,	*
 * and (2) by the number of cycles consumed on this path between group	*
 * start and group exit.						*
 *									*
 * This measure yields a list which tells whether execution is spread	*
 * among the different paths of groups or if it is concentrated in one	*
 * or a few paths.							*
 *									*
 ************************************************************************/

int cmp_exit_entries (a, b)
GROUP_EXIT *a;
GROUP_EXIT *b;
{
   if      (a->group_entry_vliw > b->group_entry_vliw) return  1;
   else if (a->group_entry_vliw < b->group_entry_vliw) return -1;
   else if (a->num_group_cycles < b->num_group_cycles) return  1;
   else if (a->num_group_cycles > b->num_group_cycles) return -1;
   else						       return  0;
}

/************************************************************************
 *									*
 *				get_exits_per_entry			*
 *				-------------------			*
 *									*
 * Return the mean number of exits per entry encountered.  Also return	*
 * the maximum number of exits encountered for any group, and the VLIW	*
 * number starting this group.  If multiple groups have the maximum	*
 * number of group exits executed then the VLIW beginning the first	*
 * group will be returned as "max_exits_group".				*
 *									*
 * NOTE:  This routine assumes that the "group_exit" list has been	*
 *	  sorted by group entry point, i.e. the -e flag has been used.	*
 *									*
 ************************************************************************/

double get_exits_per_entry (max_exits, max_exits_group)
int *max_exits;
int *max_exits_group;
{
   int i;
   int prev = -1;
   int max_group;
   int max_group_exits = 0;
   int num_group_starts = 0;
   int num_group_exits = 0;

   for (i = 0; i < group_exit_num; i++) {
      if (group_exit[i].group_entry_vliw != prev) {
	 if (num_group_exits > max_group_exits) {
	    max_group_exits = num_group_exits;
	    max_group = prev;
	 }
         num_group_exits = 1;
	 num_group_starts++;
	 prev = group_exit[i].group_entry_vliw;

      }
      else num_group_exits++;
   }

   /* In case the last group was the one with maximum number of exits */
   if (num_group_exits > max_group_exits) {
      max_group_exits = num_group_exits;
      max_group = prev;
   }

   *max_exits	    = max_group_exits;
   *max_exits_group = max_group;

   return ((double) group_exit_num) / ((double) num_group_starts);
}

/************************************************************************
 *									*
 *				get_num_paths_covering			*
 *				----------------------			*
 *									*
 ************************************************************************/

unsigned get_num_paths_covering (cover_percent)
int cover_percent;
{
   unsigned i;
   double   num_cycles = 0.0;
   double   cover_cycles = (cover_percent * total_vliw_ins) / 100.0;

   for (i = 0; i < group_exit_num; i++) {
      num_cycles += group_exit[i].num_group_cycles;
      if (num_cycles >= cover_cycles) return i + 1;
   }
}

/************************************************************************
 *									*
 *				check_group_exit_mem			*
 *				--------------------			*
 *									*
 ************************************************************************/

check_group_exit_mem ()
{
   static int group_exits_alloc;

   if (group_exit_num == group_exits_alloc) {
      if (group_exit_num == 0) {
	 group_exits_alloc = DEFAULT_NUM_GROUP_EXITS;
	 group_exit = (GROUP_EXIT *) malloc (group_exits_alloc * 
					     sizeof (group_exit[0]));
	 assert (group_exit);
      }
      else {
	 group_exits_alloc *= 2;
	 group_exit = (GROUP_EXIT *) realloc (group_exit, group_exits_alloc *
					      sizeof (group_exit[0]));
	 assert (group_exit);
      }
   }
}
