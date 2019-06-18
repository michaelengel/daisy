/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>

#include "debug.h"

int  dump_bkpt_on;
char *info_dir;
int  do_daisy_comp;
int  trace_xlated_pgm;
int  trace_all_br;		/* All types: Br-link & cond/uncond  */
int  trace_link_br;		/* Trace only branch-and-links       */
int  trace_cond_br;		/* Trace only conditional branches   */
int  trace_lib;			/* Trace lib fcns or only user code  */
int  unroll_while_ilp_better;
int  do_cache_simul;
int  do_cache_trace;
int  use_cont_cnt;
int  use_join_cnt;
int  use_ld_latency_for_ver;
int  do_split_record;
int  save_curr_xlation;
int  use_prev_xlation;
int  make_lhz_lbz_safe;		/* See "sched_ld_before_st" in ld_motion.c */
int  xlated_pgm_has_segv_handler;
int  max_dyn_pages_action;
int  dump_stderr_summary;
int  use_powerpc;
int  aixver = 414;
int  use_pdf_cnts;
int  use_pdf_prob;
int  pdf_0cnt_offpage;
unsigned max_dyn_page_cnt;

int  num_invoc_till_native;	/* If invoke xlator this many times, */
				/* clear previous xlations to force  */
				/* re-xlate ==> Find exact entry pt  */
				/* causing problem (with extra_inv.. */

int  extra_invoc_till_native;	/* If invoke xlator		     */
				/* (num+extra)_invoc_till_native,    */
				/* stop xlating and run native code  */
				/* from here forward.		     */

int  do_vliw_dump;		/* Create daisy.vliw with VLIW code   */
int  no_aux_files;		/* foo.vliw_perf_ins_info	     */
				/* foo.vliw_spec_ins_info	     */


/************************************************************************
 *									*
 *				init_flags				*
 *				----------				*
 *									*
 ************************************************************************/

init_flags ()
{
   dump_bkpt_on			= FALSE;
   info_dir			= "./";
   do_daisy_comp		= TRUE;
   trace_xlated_pgm		= FALSE;
   trace_all_br			= FALSE;
   trace_cond_br		= FALSE;
   trace_link_br		= FALSE;
   trace_lib			= FALSE;
   unroll_while_ilp_better	= FALSE;
   do_cache_simul		= FALSE;
   do_cache_trace		= FALSE;
   no_aux_files			= FALSE;
   do_vliw_dump			= FALSE;
   xlated_pgm_has_segv_handler	= FALSE;
   dump_stderr_summary		= FALSE;
   use_cont_cnt			= FALSE;
   use_join_cnt			= FALSE;
   use_ld_latency_for_ver	= FALSE;
   do_split_record		= TRUE;
   save_curr_xlation		= FALSE;
   use_prev_xlation		= FALSE;
   use_powerpc			= TRUE;
   use_pdf_cnts		        = FALSE;
   use_pdf_prob		        = FALSE;
   pdf_0cnt_offpage		= FALSE;
   make_lhz_lbz_safe		= FALSE;
   max_dyn_pages_action		= 0;			/* Just keep going */
   num_invoc_till_native	= -1;
   extra_invoc_till_native	= 0;
   aixver			= 414;
   max_dyn_page_cnt		= NO_DYN_PAGE_CNT;
}

/************************************************************************
 *									*
 *				handle_flags				*
 *				------------				*
 *									*
 ************************************************************************/

int handle_flags (argc, argv)
int  argc;
char *argv[];
{
   int i;
   int cnt;
   int cnt_type;
   int num_flags = 0;
   int pdf_opt;

   init_flags ();

   for (i = 1; i < argc; i++) {
      if (argv[i][0] != '-') break;
      else {
	 num_flags++;
	 switch (argv[i][1]) {
	    case 'A':  dump_bkpt_on   = TRUE;		break;
	    case 'B':  info_dir       = &argv[i][2];	break;
	    case 'C':  do_daisy_comp  = FALSE;		break;

	    case 'E':  trace_xlated_pgm = TRUE;

		       if      (argv[i][2] == '1') {
			    trace_all_br  = TRUE;
		       }
		       else if (argv[i][2] == '2') {
			    trace_link_br = TRUE;
		       }
		       else if (argv[i][2] == '3') {
			    trace_cond_br = TRUE;
		       }
		       else if (argv[i][2] == '4') {
			    trace_all_br  = TRUE;
			    trace_lib     = TRUE;
		       }
		       else if (argv[i][2] == '5') {
			    trace_link_br = TRUE;
			    trace_lib     = TRUE;
		       }
		       else if (argv[i][2] == '6') {
			    trace_cond_br = TRUE;
			    trace_lib     = TRUE;
		       }
							break;

	    case 'F':  unroll_while_ilp_better = TRUE;	break;
	    case 'G':  do_cache_simul	       = TRUE;	break;
	    case 'Z':  do_cache_trace	       = TRUE;	break;

	    case 'H':  cnt = sscanf (&argv[i][2], "%d",
				     &num_invoc_till_native);
		       if (cnt == 1)			break;
		       else print_usage (argv[0],
			 "Incorrect val with -H");

	    case 'J':  cnt = sscanf (&argv[i][2], "%d",
				     &extra_invoc_till_native);
		       if (cnt == 1)			break;
		       else print_usage (argv[0],
			 "Incorrect val with -J");

	    case 'I':  no_aux_files	          =TRUE;  break;
	    case 'N':  do_vliw_dump		  =TRUE;  break;
	    case 'K':  xlated_pgm_has_segv_handler=TRUE;  break;

	    case 'L':  cnt = sscanf (&argv[i][2], "%u", &max_dyn_page_cnt);
		       if (cnt == 1)			break;
		       else print_usage (argv[0],
			 "Incorrect val with -L");

	    case 'M':  cnt = sscanf (&argv[i][2], "%d", &max_dyn_pages_action);
		       if (cnt == 1)			break;
		       else print_usage (argv[0],
			 "Incorrect val with -M");

	    case 'V':  cnt = sscanf (&argv[i][2], "%d", &aixver);
		       if (cnt == 1)			break;
		       else print_usage (argv[0],
			 "Incorrect val with -V");

	    case 'Q':  cnt = sscanf (&argv[i][2], "%d", &cnt_type);
		       if (cnt == 1) {
			  if (cnt_type == 0) use_join_cnt = TRUE;
			  else		     use_cont_cnt = TRUE;
							break;
		       }
		       else print_usage (argv[0],
			 "Incorrect val with -V");

	    case 'P':  use_powerpc	      = FALSE;	break;
	    case 'R':  use_ld_latency_for_ver = TRUE;	break;
	    case 'S':  do_split_record	      = FALSE;	break;
	    case 'T':  save_curr_xlation      = TRUE;	break;
	    case 'U':  use_prev_xlation	      = TRUE;	break;
	    case 'W':  dump_stderr_summary    = TRUE;	break;
	    case 'Y':  make_lhz_lbz_safe      = TRUE;	break;

	    case 'X':  cnt = sscanf (&argv[i][2], "%d", &pdf_opt);
		       if (cnt == 1) {
			 switch (pdf_opt) {
			   case 0:  pdf_0cnt_offpage = TRUE;
			   case 1:  use_pdf_cnts     = TRUE;
				    break;

			   case 2:  pdf_0cnt_offpage = TRUE;
			   default: use_pdf_prob     = TRUE;
				    break;
			 }				break;
		       }
		       else print_usage (argv[0],
			 "Incorrect val with -X");	break;
	    default:					break;
	 }
      }
   }

   /* Make sure that only one of these is set */
   if (use_pdf_cnts) use_pdf_prob = FALSE;

   /* Make sure that only one of these is set */
   if (use_join_cnt) use_cont_cnt = FALSE;

   return num_flags;
}

/************************************************************************
 *									*
 *				print_usage				*
 *				-----------				*
 *									*
 ************************************************************************/

print_usage (progname, err)
char *progname;
char *err;
{
   fprintf (stdout, "\n%s:\n", err);
   fprintf (stdout, "Correct usage is\n\n\t%s [<flags>] <Executable> [<Executable Args>]\n\n",
	    progname);
   fprintf (stdout, "where <Executable> is a fully linked and executable XCOFF file, and\n");
   fprintf (stdout, "<Executable Args> are the normal arguments, if any, to <Executable>.\n"); 
   fprintf (stdout, "The <flags> are for \"%s\".  Currently there are:\n\n",
	    progname);
   fprintf (stdout, "\t-A:  Dump breakpoints for dbx in file \"daisy.bk\"\n\n");
   fprintf (stdout, "\t-B<info_dir>:  Directory to put info files for \"dvstats\"\n");
   fprintf (stdout, "\t               (Give full path if <Executable> changes curr directory)\n\n");
   fprintf (stdout, "\t-C:  Run without DAISY translation.\n\n");
   fprintf (stdout, "\t-E1: Trace all branches     in xlated   pgm.\n");
   fprintf (stdout, "\t-E2: Trace branch-and-links in xlated   pgm.\n");
   fprintf (stdout, "\t-E3: Trace cond branches    in xlated   pgm.\n");
   fprintf (stdout, "\t-E4: Trace all branches     in xlated   pgm and libs.\n");
   fprintf (stdout, "\t-E5: Trace branch-and-links in xlated   pgm and libs.\n");
   fprintf (stdout, "\t-E6: Trace cond branches    in xlated   pgm and libs.\n\n");
   fprintf (stdout, "\t-F:  Unroll loops as long as ILP improves.\n\n");
   fprintf (stdout, "\t-G:  Perform cache simulation.\n");
   fprintf (stdout, "\t-Z:  Save cache trace (as <Executable>.trace in <Executable> directory).\n\n");
   fprintf (stdout, "\t-H<num1>: If DAISY compiler invoked <num1> times, clr xlation table.\n");
   fprintf (stdout, "\t-J<num2>: If DAISY compiler invoked <num1> + <num2>\n");
   fprintf (stdout, "\t          times, finish execution in native mode.\n\n");
   fprintf (stdout, "\t-I:  Do NOT dump files needed by \"dvstats\"\n");
   fprintf (stdout, "\t-N:  Dump VLIW code in \"daisy.vliw\"\n\n");
   fprintf (stdout, "\t-K:  Override xlated program's segv handler\n\n");
   fprintf (stdout, "\t-L<num>: Do -M action if hit <num> dyn crosspage entry pts\n");
   fprintf (stdout, "\t-M<num>: If <num> = 1 stop, if 2 exec native code, else cont\n\n");
   fprintf (stdout, "\t-P:  Support Power architecture instead of PowerPC\n\n");
   fprintf (stdout, "\t-Q<num>:  If <num>==0, use real join pts, o.w. cnt only when pt starts\n");
   fprintf (stdout, "\t          a continuation.  W/no -Q, cnt whenever instruc encountered.\n\n");
   fprintf (stdout, "\t-R:  Use LOAD latency with LOAD-VERIFY, o.w. available immediately\n\n");
   fprintf (stdout, "\t-S:  Do NOT split complex record ops: op. into op,cmpi\n\n");
   fprintf (stdout, "\t-T:  Save xlation to disk (in ./save.daisy) for possible reuse)\n");
   fprintf (stdout, "\t-U:  Retrieve prev xlation from disk (in ./save.daisy) for reuse)\n\n");
   fprintf (stdout, "\t-V<num>: Running AIX version <num> (Default=%d)\n\n", aixver);
   fprintf (stdout, "\t-W:  Dump to stdout, a copy of stats appended to \"daisy.stats\"\n\n");
   fprintf (stdout, "\t-X<num>:  Must have done \"dvstats\" on previous run\n");
   fprintf (stdout, "\t  <num> = 0,1:  Use prof-dir-feedback w/cnts.\n");
   fprintf (stdout, "\t  <num> = 2,3:  Use prof-dir-feedback w/prob.\n");
   fprintf (stdout, "\t  <num> = 0,2:  Generate code for 0-count targets only if executed.\n");
   fprintf (stdout, "\t  <num> = 1,3:  Always generate code for 0-count targets.\n\n");
   fprintf (stdout, "\t-Y: STB r3,X ==> STB   r3,x          | STH r3,X ==> STH   r3,x\n");
   fprintf (stdout, "\t    LBZ r4,X ==> RLINM r4,r3,0,24,31 | LHZ r4,X ==> RLINM r4,r3,0,16,31\n");
   fprintf (stdout, "\t    No LD-VER needed, but copy-prop info for r3 not go to r4\n\n");

   fprintf (stdout, "Summary statistics are displayed to \"stdout\" and are\n");
   fprintf (stdout, "appended to the file \"daisy.stats\".  Cache simulation\n");
   fprintf (stdout, "results are placed in the file \"cache.out\", while the\n");
   fprintf (stdout, "cache configuration is read from the file \"cache.config\"\n");
   fprintf (stdout, "in the current directory or from the directory specified by\n");
   fprintf (stdout, "the CACHE_CONFIG environment variable.\n\n");

   fprintf (stdout, "Additional configuration parameters are read from the\n");
   fprintf (stdout, "file \"daisy.config\", if it exists.  For example \"num_alus\"\n");
   fprintf (stdout, "specifies the number of ALUS.  The format of \"daisy.config\"\n");
   fprintf (stdout, "is one quantity (e.g. \"num_alus\") per line followed by\n");
   fprintf (stdout, "whitespace and the numerical value, e.g. 16.  Blank lines\n");
   fprintf (stdout, "are ignored and # is a comment prefix for the line.  If\n");
   fprintf (stdout, "\"daisy.config\" exists in the current directory, it is used,\n");
   fprintf (stdout, "otherwise if \"daisy.config\" exists in the directory specified\n");
   fprintf (stdout, "by the environment variable DAISY_CONFIG, it is used.  Defaults\n");
   fprintf (stdout, "are used and a warning issued if neither is present.\n\n");

   fprintf (stdout, "The -L and -M can be very useful as debugging aids.\n");
   fprintf (stdout, "\"%s\" can be run several times with the target program\n", progname);
   fprintf (stdout, "using binary search to assign values to -L<num>.  For example use\n");
   fprintf (stdout, "-L1000 -M2 to see if the DAISY translated program crashes in the first\n");
   fprintf (stdout, "1000 page crossings.  If it does, then try -L500 -M2 in binary search\n");
   fprintf (stdout, "fashion.  Continue this until the precise page causing the crash is\n");
   fprintf (stdout, "located.  Assume the last value for which the program runs correctly\n");
   fprintf (stdout, "is -L319.  Then do an additional run with -L319 -M1 to determine the\n");
   fprintf (stdout, "PowerPC address of the page entry causing the problem.\n");
   fprintf (stdout, "\n");
   fprintf (stdout, "NOTE:  The -H and -J flags used to be used for similar purposes, but\n");
   fprintf (stdout, "       use of -L and -M is much preferred.\n");

   exit (1);
}
