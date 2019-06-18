/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <signal.h>

#include "vliwcnts.h"
#include "brcorr.h"
#include "translate.h"
#include "parse.h"
#include "instrum.h"
#include "op_mapping.h"
#include "cnts_wr.h"

#define INS_ALIGNMENT			16
#define PAD_WORD			0
#define MAX_PATH_CONDS			1024  /* # Cond-Br, Group Start->End */

double   total_vliw_user_ins;
double   total_vliw_ins;
double   total_vliw_spec_ops;
double   total_vliw_perf_ops;
double   total_orig_user_ops;
double   total_orig_ops;
double   total_perf_ops;
double   total_spec_ops;

int	 make_cnts_file;
int	 skip_order;
int	 make_pdf_file;
int	 group_exits_by_cycles;
int	 group_exits_by_entry;
unsigned br_corr_order;

unsigned num_times_exec;

char	 curr_fname[128];

extern unsigned   curr_file_offset;

static void (*process_perf_fcn) ();
static void (*process_spec_fcn) ();
static void (*process_lib_fcn)  ();
static void (*init_vliwinp_fcn) ();
static void (*finish_vliwinp_fcn) ();

static unsigned vliw_ins_num_cnts;
static unsigned num_files;
static unsigned offset_in_file;

/* This suffix must match that in "cnts_wr.c" */
const char *cntfile_suffix = ".vliw_perf_ins_cnts";

/* These suffixes must match those in instrum.c" */
const char *vliw_perf_fname_suffix = ".vliw_perf_ins_info";
const char *vliw_spec_fname_suffix = ".vliw_spec_ins_info";

static char cnts_name[128];
static char perf_name[1024];
static char spec_name[1024];

static FILE *fp_cnts;
static FILE *fp_perf;
static FILE *fp_spec;

static char *vstats_name;

extern FILE *fp_cs;

char *get_info_dir_name (char *, char *);
char *strip_dir_prefix (char *);
unsigned read_cnts_and_info (unsigned);
unsigned update_offset_in_file (unsigned);
void null_routine (void);
int handle_args ();

/************************************************************************
 *									*
 *				main					*
 *				----					*
 *									*
 *									*
 ************************************************************************/

main (argc, argv)
int  argc;
char *argv[];
{
   int  num_flags;
   char *info_dir;
   char *exec_name;

   num_flags = handle_args (argc, argv);

   if (argc - num_flags == 2) info_dir = 0;
   else if (argc - num_flags == 3) info_dir = argv[2+num_flags];
   else print_usage (argv[0]);

   total_vliw_user_ins = 0.0;
   total_vliw_ins = 0.0;
   total_vliw_spec_ops = 0.0;
   total_vliw_perf_ops = 0.0;
   total_orig_user_ops = 0.0;
   total_orig_ops = 0.0;
   total_spec_ops = 0.0;
   total_perf_ops = 0.0;

/* strip_suffix (argv[1+num_flags]); */	/* Why did we ever call this ?? */
   strcpy (cnts_name, argv[1+num_flags]);
   strcat (cnts_name, cntfile_suffix);

   init_for_ops_sort   ();
   set_sort_op_mapping ();
   init_condbranch     ();
   offset_init	       ();

   /* 1. Calculate the total number of instructions executed and histograms */
   process_perf_fcn  = count_perf_ins;
   process_spec_fcn  = count_spec_ins;
   process_lib_fcn   = ignore_lib_calls;
   if (make_pdf_file) {  init_vliwinp_fcn = init_vliwinp_cnts;
   		       finish_vliwinp_fcn = finish_vliwinp_cnts; }
   else		      {  init_vliwinp_fcn = null_routine;
   		       finish_vliwinp_fcn = null_routine;        }

   open_cnts_file ();
   read_files (info_dir);
   fclose (fp_cnts);

   exec_name = strip_dir_prefix (argv[1+num_flags]);
   dump_all_histo_to_file (exec_name);

   /* 2. Compute & dump the statistics for the former ".cnts" file */
   process_perf_fcn   = cs_perf_ins;
   process_spec_fcn   = cs_spec_ins;
   process_lib_fcn    = cs_lib_calls;
   init_vliwinp_fcn   = null_routine;
   finish_vliwinp_fcn = null_routine;

   open_cs_file (exec_name);
   open_cnts_file ();
   read_files (info_dir);
   fclose (fp_cnts);
   if (fp_cs) fclose (fp_cs);

/* display_counts (); */

   offset_dump_histo	      (exec_name);
   dump_nonzero_vliws_to_file (exec_name);
   dump_condbranch_summary    (exec_name);
   dump_group_exit_info       (exec_name);
   dump_sort_ops              (exec_name, 0, FALSE);

   fclose (fp_cnts);

   exit (0);
}

/************************************************************************
 *									*
 *				handle_args				*
 *				-----------				*
 *									*
 *									*
 ************************************************************************/

int handle_args (argc, argv)
int  argc;
char *argv[];
{
   int i;
   int cnt;
   int num_flags = 0;

   make_cnts_file	 = TRUE;
   skip_order		 = FALSE;
   make_pdf_file	 = FALSE;
   group_exits_by_cycles = TRUE;
   group_exits_by_entry  = FALSE;
   br_corr_order	 = 0;

   for (i = 1; i < argc; i++) {
      if (argv[i][0] == '-') {
	 num_flags++;

	 switch (argv[i][1]) {

	    case 'b':   cnt = sscanf (&argv[i][2], "%u", &br_corr_order);
		        if (cnt == 1  &&  br_corr_order < MAX_BR_HIST_DEPTH)
			     break;
		        else print_usage (argv[0]);

	    case 'c':	make_cnts_file = FALSE;
			break;

	    case 's':	skip_order  = TRUE;
			break;

	    case 'p':	make_pdf_file = TRUE;
			break;

	    case 'e':   group_exits_by_cycles = FALSE;
			group_exits_by_entry  = TRUE;
			break;

	    default:	fprintf (stderr, "\nUnknown flag:  %s\n", argv[i]);
		        print_usage (argv[0]);
			break;
	 }
      }
      else break;
   }

   return num_flags;
}

/************************************************************************
 *									*
 *				print_usage				*
 *				-----------				*
 *									*
 *									*
 ************************************************************************/

print_usage (name)
char *name;
{
   char *info_dir;
   char buff[128];

   printf ("\nThe \"%s\" utility is used to obtain statistics about a PowerPC\n", name);
   printf ("program which has been translated using \"daisy\".  Its usage is\n");
   printf ("\n");
   printf ("         %s [flags] <cnts_file> [<info_dir>]\n", name);
   printf ("\n");
   printf ("where <cnts_file> must have a \".vliw_perf_ins_cnts\" suffix, and contain\n");
   printf ("counts of the number of times each VLIW instruction was executed.\n");
   printf ("(The \".vliw_perf_ins_cnts\" suffix need not be specified.)  The\n");
   printf ("The <cnts_file> should be automatically produced by running \"daisy\".\n");
   printf ("\n");
   printf ("The <info_dir> is where information about each instruction\n");
   printf ("is placed by \"daisy\" for use here. \n");
   printf ("\n");
   printf ("Valid flags are \"-b#\", \"-c\", ,\"-e\", \"-p\", and \"-s\".  Dynamic opcode\n");
   printf ("frequencies are always produced.  The \"-p\" flag causes a \".pdf\" file\n");
   printf ("to be created.  This \".pdf\" file contains profile-directed-feedback\n");
   printf ("information.  The \"-c\" flag turns off generation of the .cnts file\n");
   printf ("(which can be very large).  The \"-e\" flag controls how the \".exits\"\n");
   printf ("file is generated. (This file is generated for \"%s\" only).  By\n", name);
   printf ("default the \".exits\" file is sorted by frequency of group exit.  (Each\n");
   printf ("group is a tree of VLIW tree instructions.)  With the \"-e\" flag, the\n");
   printf ("\".exits\" file is sorted by the VLIW at the entry point of a group.  In\n");
   printf ("this way it can be determined how many of the paths through the group\n");
   printf ("tree are actually exercised.  The \"-b#\" flag is used to specify the\n");
   printf ("degree of branch correlation used in producing the \".condbr\" file.\n");
   printf ("The value must be non-negative and less than 16.\n");
   printf ("\n");
   printf ("Several output files are produced from an executable file \"foo\":\n");
   printf ("\n");
   printf ("foo.histo:      Histograms (both textual and graphical) of the\n");
   printf ("                number of operations executed per VLIW instruction.\n");
   printf ("                Useful for determining ILP achieved.\n");
   printf ("\n");
   printf ("foo.exits:      A list of the paths in each VLIW group, and the\n");
   printf ("                ILP attained on each.  Useful for finding areas\n");
   printf ("                where compiler is deficient.\n");
   printf ("\n");
   printf ("foo.cnts:       Lists the number of times each VLIW instruction\n");
   printf ("\n");
   printf ("foo.pops1:      A list of opcode frequencies in decreasing order\n");
   printf ("                of use.  These frequencies are those of operations\n");
   printf ("                on the taken path through a VLIW instruction.\n");
   printf ("\n");
   printf ("foo.sops1:      A list of opcode frequencies in decreasing order\n");
   printf ("                of use.  These frequencies are those of operations\n");
   printf ("                on all paths through a VLIW instruction.\n");
   printf ("\n");

   exit (1);
}

/************************************************************************
 *									*
 *				open_cnts_file				*
 *				--------------				*
 *									*
 * This is very similar (but inverse) to the glue routine in cnts_wr.c	*
 *									*
 ************************************************************************/

open_cnts_file ()
{
   unsigned magic_num;

   fp_cnts = fopen (cnts_name, "r");
   if (!fp_cnts) {
      printf ("Could not open \"%s\" for reading\n", cnts_name);
      exit (1);
   }

   if (fread (&magic_num, sizeof (magic_num), 1, fp_cnts) != 1)
      read_failure ();

   if (magic_num != CURR_CNTS_MAGIC_NUM) {
      printf ("Counts file magic number read (%08x) does not match expected (%08x)\n",
	      magic_num, CURR_CNTS_MAGIC_NUM);
      exit (1);
   }

   if (fread (&num_times_exec, sizeof (num_times_exec), 1, fp_cnts) != 1) {
      printf ("Failure reading \"%s\"\n", cnts_name);
      exit (1);
   }

   if (fread (&num_files, sizeof (num_files), 1, fp_cnts) != 1){
      printf ("Failure reading \"%s\"\n", cnts_name);
      exit (1);
   }
}

/************************************************************************
 *									*
 *				read_files				*
 *				----------				*
 *									*
 * This is very similar (but inverse) to the glue routine in cnts_wr.c	*
 * and the routine instr_save_path_instr in "instrum.c".		*
 *									*
 ************************************************************************/

read_files (info_dir)
char *info_dir;
{
   unsigned	i;
   int		name_len;
   unsigned	code_size;
   unsigned	perf_magic_num;
   unsigned	spec_magic_num;
   unsigned	base_spec_index = 1;	/* Allow for *TOTALS* as index 0 */
   char		buff[1024];

   for (i = 0; i < num_files; i++) {

      offset_in_file = 0;

      if (fread (&name_len, sizeof (name_len), 1, fp_cnts) != 1)
	 read_failure (cnts_name);

      if (fread (curr_fname, name_len, 1, fp_cnts) != 1)
	 read_failure (cnts_name);

      if (fread (&vliw_ins_num_cnts, sizeof (vliw_ins_num_cnts), 1, fp_cnts) != 1)
	 read_failure (cnts_name);

      strip_suffix (curr_fname);
      info_dir = get_info_dir_name (info_dir, buff);

      init_vliwinp_fcn ();

      /* OPEN THE FILE WITH THE PERFORMED OPS IN THE INSTRUCTIONS */

      sprintf (perf_name, "%s%s%s",
	       info_dir, curr_fname, vliw_perf_fname_suffix);

      fp_perf = fopen (perf_name, "r");
      if (!fp_perf) {
	 printf ("Could not open \"%s\" for reading\n", perf_name);
	 exit (1);
      }

      /* Check that magic number is correct */
      if (fread (&perf_magic_num, sizeof (perf_magic_num), 1, fp_perf) != 1) {
         printf ("Failure reading from \"%s\"\n", perf_name);
         exit (1);
      }

      if (perf_magic_num != CURRENT_PERF_MAGIC_NUM) {
	 fprintf (stderr, "Incorrect magic number (0x%08x) in %s.  Expecting 0x%08x\n",
		  perf_magic_num, perf_name, CURRENT_PERF_MAGIC_NUM);
	 exit (1);
      }

      /* OPEN THE FILE WITH THE SPECIFIED OPS IN THE INSTRUCTIONS */

      sprintf (spec_name, "%s%s%s",
	       info_dir, curr_fname, vliw_spec_fname_suffix);

      fp_spec = fopen (spec_name, "r");
      if (!fp_spec) {
	 printf ("Could not open \"%s\" for reading\n", spec_name);
	 exit (1);
      }

      /* Check that magic number is correct */
      if (fread (&spec_magic_num, sizeof (spec_magic_num), 1, fp_spec) != 1) {
         printf ("Failure reading from \"%s\"\n", spec_name);
         exit (1);
      }

      if (spec_magic_num != CURRENT_SPEC_MAGIC_NUM) {
	 fprintf (stderr, "Incorrect magic number (0x%08x) in %s.  Expecting 0x%08x\n",
		  spec_magic_num, spec_name, CURRENT_SPEC_MAGIC_NUM);
	 exit (1);
      }

      else if (fread (&code_size, sizeof (code_size), 1, fp_spec) != 1) {
         printf ("Failure reading from \"%s\"\n", spec_name);
         exit (1);
      }

      base_spec_index += read_cnts_and_info (base_spec_index);

      fclose (fp_perf);
      fclose (fp_spec);
      finish_vliwinp_fcn (curr_fname);
   }

   read_lib_cnts (base_spec_index);
}

/************************************************************************
 *									*
 *				read_cnts_and_info			*
 *				------------------			*
 *									*
 * This is very similar (but inverse) to the glue routine in cnts_wr.c	*
 * and the routine instr_save_path_instr in "instrum.c".		*
 *									*
 ************************************************************************/

unsigned read_cnts_and_info (base_spec_index)
unsigned base_spec_index;
{
   unsigned	  i, j;
   unsigned	  times_exec;
   unsigned	  index;
   unsigned	  spec_index;
   unsigned	  prev_spec_index;
   unsigned char  num_branches;
   unsigned char  path_num;
   unsigned char  num_ops;
   unsigned char  orig_ops;
   unsigned char  user_code;
   unsigned	  group_entry_vliw;
   unsigned	  ppc_code_addr;
   unsigned	  vmm_invoc_cnt;
   unsigned	  same_group;
   unsigned	  tip_label;
   unsigned short num_cycles;
   unsigned short num_conds;
   unsigned short path_ops;
   unsigned char  opcodes[OPS_PER_VLIW+1];
   unsigned	  vliwinp_line[OPS_PER_VLIW+1];
   unsigned char  branch_dir[MAX_PATH_CONDS];
   unsigned	  branch_addr[MAX_PATH_CONDS];

   prev_spec_index = 0;			/* Match 1st spec_index */
   for (i = 0; i < vliw_ins_num_cnts; i++) {

      if (fread (&times_exec, 4, 1, fp_cnts) != 1) read_failure (cnts_name);

      if (fread (&index, sizeof (index), 1, fp_perf) != 1)
	 read_failure (perf_name);
      if (fread (&spec_index, sizeof (spec_index), 1, fp_perf) != 1)
	 read_failure (perf_name);
      if (fread (&num_branches, sizeof (num_branches), 1, fp_perf) != 1)
	 read_failure (perf_name);
      if (fread (&path_num, sizeof (path_num), 1, fp_perf) != 1)
	 read_failure (perf_name);
      if (fread (&num_ops, sizeof (num_ops), 1, fp_perf) != 1)
	 read_failure (perf_name);

      if (fread (&orig_ops, sizeof (orig_ops), 1, fp_perf) != 1)
         read_failure (perf_name);
      if (fread (&user_code, sizeof (user_code), 1, fp_perf) != 1)
	 read_failure (perf_name);

      if (fread (&num_cycles, sizeof (num_cycles), 1, fp_perf) != 1)
	 read_failure (perf_name);

      if (num_cycles != 0) {

	 if (fread (&group_entry_vliw, sizeof (group_entry_vliw), 1, fp_perf) != 1)
	    read_failure (perf_name);
	 if (fread (&ppc_code_addr, sizeof (ppc_code_addr), 1, fp_perf) != 1)
	    read_failure (perf_name);
	 if (fread (&vmm_invoc_cnt, sizeof (vmm_invoc_cnt), 1, fp_perf) != 1)
	    read_failure (perf_name);

	 /* The high order bit of "vmm_invoc_cnt" indicates whether
	    the current exit branches to the start of its group or
	    to another group.
	 */
	 same_group = vmm_invoc_cnt &  0x80000000;
         vmm_invoc_cnt              &= 0x7FFFFFFF;

	 if (fread (&tip_label, sizeof (tip_label), 1, fp_perf) != 1)
	    read_failure (perf_name);

	 if (fread (&path_ops, sizeof (path_ops), 1, fp_perf) != 1)
	    read_failure (perf_name);

	 if (fread (&num_conds, sizeof (num_conds), 1, fp_perf) != 1)
	    read_failure (perf_name);

	 assert (num_conds < MAX_PATH_CONDS);

	 for (j = 0; j < num_conds; j++) {
	    if (fread (&branch_dir[j], sizeof(branch_dir[0]), 1, fp_perf) != 1)
	       read_failure (perf_name);

	    if (fread (&branch_addr[j], sizeof(branch_addr[0]), 1, fp_perf) != 1)
	       read_failure (perf_name);
	 }
      }

      if (fread (opcodes, sizeof (opcodes[0]), num_ops, fp_perf) != num_ops)
	 read_failure (perf_name);
      if (fread (vliwinp_line, sizeof (vliwinp_line[0]), num_ops, fp_perf)
	  != num_ops)
	 read_failure (perf_name);

      if (index != i) {
	 printf ("Synchronization Error Between %s (%u) and %s (%u)\n",
		 cnts_name, i, perf_name, index);
	 exit (1);
      }

      if (prev_spec_index != spec_index) {
	 read_spec_info (base_spec_index, prev_spec_index);
         prev_spec_index = spec_index;
      }

      process_perf_fcn (times_exec, index, num_ops, orig_ops, num_branches,
			vliwinp_line, opcodes, path_num, offset_in_file,
			user_code, group_entry_vliw, ppc_code_addr,
			vmm_invoc_cnt, tip_label, (int) num_cycles,
			(int) num_conds, branch_dir, branch_addr, same_group,
			path_ops);
   }

   /* Get the last instruction */
   if (vliw_ins_num_cnts == 0) return 0;
   else {
      read_spec_info (base_spec_index, prev_spec_index);
      return prev_spec_index + 1;
   }
}

/************************************************************************
 *									*
 *				read_spec_info				*
 *				--------------				*
 *									*
 ************************************************************************/

read_spec_info (base_spec_index, index)
unsigned base_spec_index;
unsigned index;
{
   unsigned	  spec_index;
   unsigned	  num_bytes;
   unsigned char  name_len;
   unsigned char  num_branches;
   unsigned char  num_ops;
   unsigned char  name[128];
   unsigned char  opcodes[OPS_PER_VLIW+1];
   unsigned       offsets[OPS_PER_VLIW+1];

   if (fread (&spec_index, sizeof (spec_index), 1, fp_spec) != 1)
      read_failure (spec_name);
   if (fread (&num_bytes, sizeof (num_bytes), 1, fp_spec) != 1)
      read_failure (spec_name);
   if (fread (&name_len, sizeof (name_len), 1, fp_spec) != 1)
      read_failure (spec_name);
   if (fread (name, name_len, 1, fp_spec) != 1)
      read_failure (spec_name);
   if (fread (&num_branches, sizeof (num_branches), 1, fp_spec) != 1)
      read_failure (spec_name);
   if (fread (&num_ops, sizeof (num_ops), 1, fp_spec) != 1)
      read_failure (spec_name);
   if (fread (opcodes, sizeof (opcodes[0]), num_ops, fp_spec) != num_ops)
      read_failure (spec_name);
   if (fread (offsets, sizeof (offsets[0]), num_ops, fp_spec) != num_ops)
      read_failure (spec_name);

   if (index != spec_index) {
      printf ("Synchronization Error Between %s (%u) and %s (%u)\n",
	      perf_name, index, spec_name, spec_index);
      exit (1);
   }

   fix_name (name);

   process_spec_fcn (name, base_spec_index + index, num_branches, num_ops,
		     opcodes, offsets, offset_in_file, num_bytes);
}

/************************************************************************
 *									*
 *				read_lib_cnts				*
 *				-------------				*
 *									*
 ************************************************************************/

read_lib_cnts (base_spec_index)
unsigned base_spec_index;
{
   int	    i;
   int	    name_len;
   unsigned lib_cnt;
   unsigned num_lib_fcn;
   unsigned num_libs_called;
   char	    buff[128];
 
   if (fread (&num_lib_fcn, sizeof (num_lib_fcn), 1, fp_cnts) != 1)
      read_failure (cnts_name);

   num_libs_called = 0;
   for (i = 0; i < num_lib_fcn; i++) {

      if (fread (&lib_cnt, sizeof (lib_cnt), 1, fp_cnts) != 1)
         read_failure (cnts_name);

      if (fread (&name_len, sizeof (name_len), 1, fp_cnts) != 1)
	 read_failure (cnts_name);

      if (fread (buff, name_len, 1, fp_cnts) != 1) read_failure (cnts_name);

      if (lib_cnt > 0) {
         process_lib_fcn (buff, base_spec_index + num_libs_called, lib_cnt);
	 num_libs_called++;
      }
   }
}

/************************************************************************
 *									*
 *				fix_name				*
 *				--------				*
 *									*
 * Change the most obvious artifact of making names AIX compatible,	*
 * i.e. change "_ZZ" in names back to "_$$".  We ignore the chance that	*
 * the original symbol contained "_ZZ".					*
 *									*
 ************************************************************************/

fix_name (name)
char *name;
{
   char *p;

   for (p = name; *p; p++) {

      if (*(p+0) == '_')
	 if (*(p+1) == 'Z')
	    if (*(p+2) == 'Z')
	       break;
   }

   if (*p) {
      *(p+1) = '$';
      *(p+2) = '$';
   }
}

/************************************************************************
 *									*
 *				no_memory				*
 *				---------				*
 *									*
 ************************************************************************/

no_memory (s)
char *s;
{
   printf ("No memory for %s\n", s);
   exit (1);
}

/************************************************************************
 *                                                                      *
 *                              strip_dir_prefix			*
 *                              ----------------                        *
 *                                                                      *
 * Removes any path name from passed "name".  Returns pointer to start	*
 * of actual base name in "name".					*
 *                                                                      *
 ************************************************************************/

char *strip_dir_prefix (name)
char *name;
{
   char *p;
   char *pslash = 0;

   for (p = name; *p; p++)
      if (*p == '/') pslash = p;

   if (pslash && *pslash == '/') return pslash+1;
   else return name;
}

/************************************************************************
 *									*
 *				read_failure				*
 *				------------				*
 *									*
 ************************************************************************/

read_failure (name)
char *name;
{
   printf ("Failure reading \"%s\"\n", name);
   exit (1);
}

/************************************************************************
 *									*
 *				do_nothing				*
 *				----------				*
 *									*
 * Do nothing.								*
 *									*
 ************************************************************************/

void null_routine ()
{
}
