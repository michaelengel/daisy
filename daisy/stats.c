/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include "dis.h"
#include "stats.h"
#include "resrc_offset.h"

CNT *cnt_array[NUM_STATS];

static CNT _cnt_array[] = {
{"Invocations of translator                  ", XLATOR_INVOC},
{"Number of pages visited                    ", PAGES_VISITED},
{"Number of user pages visited               ", USER_PAGES_VISITED},
{"Total number of entry points               ", TOTAL_ENTRY_PTS},
{"Average number of entry points per page    ", AVG_E_PTS_PER_PAGE},
{"# of groups which were created             ", GROUP_DEFINES},
{"# of groups which executed   > 10 times    ", GROUP_EXECS_10},
{"# of groups which executed 5 -  9 times    ", GROUP_EXECS_5},
{"# of groups which executed 2 -  4 times    ", GROUP_EXECS_2},
{"# of groups which executed      1 time     ", GROUP_EXECS_1},
{"# of direct offpage    branches executed   ", OFFPAGE_DIR_BR},
{"# of indirect CTR      branches executed   ", INDIR_CTR_BR},
{"# of indirect LINKREG  branches executed   ", INDIR_LR_BR},
{"# of indirect LINKREG2 branches executed   ", INDIR_LR2_BR},
{"# of TOTAL  crosspage  branches executed   ", TOTAL_CROSS_BR},
{"# of hash values with single entry point   ", NUM_SINGLE_HASH},
{"Max # of entry points for any hash value   ", MAX_MULTI_HASH},
{"Hash value with Max # of entry points      ", MAX_MULTI_INDEX},
{"Max # of open paths                        ", MAX_OPEN_PATHS},
{"Total PPC ins (some xlated mult times)     ", TOTAL_PPC_OPS},
{"Total VLIW instructions                    ", TOTAL_VLIWS},
{"Total size of VLIW          code           ", VLIW_OPS_SIZE},
{"Size of VLIW code from LOAD_VERIFY fails   ", LD_VER_FAIL_SIZE},
{"Size of VLIW code from LOAD_VERIFY recomps ", LD_VER_RECOMP_SIZE},
{"Number of              LOAD_VERIFY recomps ", LD_VER_RECOMP_CNT},
{"# Times LOAD-VERIFY instruc failed         ", LD_VER_FAIL},
{"# Sites LOAD-VERIFY instruc failed         ", LD_VER_SITE},
{"Total size of simulation    code           ", XLATED_CODE_SIZE},
{"Total size of original user code           ", ORIG_CODE_SIZE},
{"Average size of translated page of code    ", AVG_XLATE_PAGE_SIZE}
};

extern int	dump_stderr_summary;
extern char	curr_wd[];
extern int	lvfail_sites;
extern unsigned xlate_entry_cnt;
extern unsigned textins;
/* extern unsigned xlate_mem[]; */
extern unsigned *xlate_mem;
extern unsigned *xlate_mem_ptr;
extern unsigned *prog_start;
extern unsigned char r13_area[];
extern char	*daisy_progname;
extern unsigned vliw_cnt;
extern unsigned num_duplic_groups;
extern unsigned total_groups;
extern unsigned max_open_paths;
extern unsigned group_cnt[];
extern unsigned lv_vliw_ops;
extern unsigned lv_recomp_vliw_ops;
extern unsigned lv_recomp_cnt;
extern unsigned total_vliw_ops;
extern unsigned total_ppc_ops;		/* NOTE: May xlate single op mult 
						 times.  Each xlation counted
						 here so total may be inflated.
					 */
extern unsigned rpagesize;
extern unsigned rpagesize_mask;

static unsigned egroup1_histo;
static unsigned egroup2_histo;
static unsigned egroup5_histo;
static unsigned egroup10_histo;
static unsigned exec_groups;
static double   total_group_execs;


unsigned *xlate_entry_raw    (unsigned *);

/************************************************************************
 *									*
 *				do_stats				*
 *				--------				*
 *									*
 ************************************************************************/

do_stats ()
{
   int fd;
   char daisy_stats_file[256];

   strcpy (daisy_stats_file, curr_wd);
   strcat (daisy_stats_file, "/daisy.stats");

   fd = open (daisy_stats_file, O_CREAT | O_WRONLY, 0644);
   assert (fd != -1);
   lseek (fd, 0, SEEK_END);		/* Go to end to append */

   collect_stats   ();
   print_cnt_array (fd);

   if (dump_stderr_summary) print_cnt_array (stderr->_file);

   close (fd);
}

/************************************************************************
 *									*
 *				init_cnt_array				*
 *				--------------				*
 *									*
 * Set up the array of pointer to point to the right place in the	*
 * real array.  By using the array of pointers, the real array can	*
 * be listed in any order.  We assume that counts are all 0.  Otherwise	*
 * they could be so set here.						*
 *									*
 ************************************************************************/

init_cnt_array ()
{
   int i;

   for (i = 0; i < NUM_STATS; i++)
      cnt_array[_cnt_array[i].index] = &_cnt_array[i];
}

/************************************************************************
 *									*
 *				collect_stats				*
 *				-------------				*
 *									*
 ************************************************************************/

collect_stats ()
{
   int	    i;
   int	    page_visited;
   int	    e_points_on_page;
   unsigned boundary;
   PTRGL    *ptrgl   = (PTRGL *) xlate_entry_raw;
   unsigned raw	     = ptrgl->func;
   unsigned a, b, c, d;

   assert (num_duplic_groups == 0);

   cnt_array[XLATOR_INVOC]->cnt     = xlate_entry_cnt;
   cnt_array[XLATED_CODE_SIZE]->cnt = get_xlated_code_size ();
   cnt_array[ORIG_CODE_SIZE]->cnt   = textins * sizeof (unsigned);

   handle_groups ();
   cnt_array[GROUP_DEFINES]->cnt  = total_groups;
   cnt_array[GROUP_EXECS_10]->cnt = egroup10_histo;
   cnt_array[GROUP_EXECS_5]->cnt  = egroup5_histo;
   cnt_array[GROUP_EXECS_2]->cnt  = egroup2_histo;
   cnt_array[GROUP_EXECS_1]->cnt  = egroup1_histo;

   boundary = (rpagesize - (((unsigned) prog_start) & rpagesize_mask)) >> 2;
   page_visited     = FALSE;
   e_points_on_page = 0;

   mark_visited_pages ();
   cnt_array[PAGES_VISITED]->cnt        = count_num_pages ();
   cnt_array[USER_PAGES_VISITED]->cnt   = count_num_user_pages ();
   cnt_array[MAX_MULTI_HASH]->cnt	= max_multi_hash (
				          &cnt_array[MAX_MULTI_INDEX]->cnt,
				          &cnt_array[NUM_SINGLE_HASH]->cnt);

   cnt_array[MAX_OPEN_PATHS]->cnt	= max_open_paths;
   cnt_array[TOTAL_PPC_OPS]->cnt	= total_ppc_ops;
   cnt_array[TOTAL_VLIWS]->cnt          = vliw_cnt;
   cnt_array[VLIW_OPS_SIZE]->cnt        = total_vliw_ops * 4;
   cnt_array[LD_VER_FAIL_SIZE]->cnt     = lv_vliw_ops * 4;
   cnt_array[LD_VER_RECOMP_SIZE]->cnt   = lv_recomp_vliw_ops * 4;
   cnt_array[LD_VER_RECOMP_CNT]->cnt    = lv_recomp_cnt;
   cnt_array[AVG_XLATE_PAGE_SIZE]->cnt  = cnt_array[VLIW_OPS_SIZE]->cnt /
					  cnt_array[PAGES_VISITED]->cnt;

   a=cnt_array[OFFPAGE_DIR_BR]->cnt = *((unsigned *) &r13_area[OFFPAGE_CNT]);
   b=cnt_array[INDIR_CTR_BR]->cnt   = *((unsigned *) &r13_area[INDIR_CTR_CNT]);
   c=cnt_array[INDIR_LR_BR]->cnt    = *((unsigned *) &r13_area[INDIR_LR_CNT]);
   d=cnt_array[INDIR_LR2_BR]->cnt   = *((unsigned *) &r13_area[INDIR_LR2_CNT]);
     cnt_array[TOTAL_CROSS_BR]->cnt = a + b + c + d;

   cnt_array[LD_VER_FAIL]->cnt    = *((unsigned *) &r13_area[LD_VER_FAIL_CNT]);
   cnt_array[LD_VER_SITE]->cnt    = lvfail_sites;

   /* Put these last, since the translator translates us and we want
      the closest approximation to the number of entries.  In addition
      putting this before the "max_multi_hash" call yields the seemingly
      anomalous result that there are more hash values with a single
      entry point than total entry points.
   */
   cnt_array[TOTAL_ENTRY_PTS]->cnt	= get_num_hash_entries ();

   cnt_array[AVG_E_PTS_PER_PAGE]->cnt   = cnt_array[TOTAL_ENTRY_PTS]->cnt /
					  cnt_array[PAGES_VISITED]->cnt;
}

/************************************************************************
 *									*
 *				print_cnt_array				*
 *				---------------				*
 *									*
 ************************************************************************/

print_cnt_array (fd)
int fd;
{
   int i;

   fdprintf (fd, "\n\n%s\n", daisy_progname);
   fdprintf (fd, "--------------------------------------------------\n");

   for (i = 0; i < NUM_STATS; i++) {
      fdprintf (fd, "%s   %d\n", cnt_array[i]->name, cnt_array[i]->cnt);
   }

   fdprintf (fd, "\n");
}

/************************************************************************
 *									*
 *				handle_groups				*
 *				-------------				*
 *									*
 ************************************************************************/

static handle_groups ()
{
   unsigned i;
   int	    group_cmp (int *, int *);

   qsort (group_cnt, total_groups, sizeof (group_cnt[0]), group_cmp);

   exec_groups	     = 0;
   total_group_execs = 0.0;
   for (i = 0; i < total_groups; i++) {
      if (group_cnt[i] > 0) {

         if      (group_cnt[i] >=  10) egroup10_histo++;
         else if (group_cnt[i] >=   5) egroup5_histo++;
         else if (group_cnt[i] >=   2) egroup2_histo++;
         else			       egroup1_histo++;

	 exec_groups++;
	 total_group_execs += (double) group_cnt[i];
      }
      else break;
   }
}

/************************************************************************
 *									*
 *				group_cmp				*
 *				---------				*
 *									*
 * Make qsort create list in descending order.				*
 *									*
 ************************************************************************/

static int group_cmp (a, b)
int *a;
int *b;
{
   if	   (*a < *b) return  1;
   else if (*a > *b) return -1;
   else		     return  0;
}
