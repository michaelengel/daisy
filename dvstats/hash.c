/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "brcorr.h"

#pragma alloca

#define HASH_MEM_SIZE		0x10000
#define HASH_TBL_SIZE		0x4000		/* get_hashval ==> pwr of 2 */
#define HASH_TBL_MASK		0x3FFF
#define LOG_HASH_TBL_SIZE	    14

typedef struct _hash_entry {
   unsigned	      address;
   unsigned	      num_paths_occurs;
   unsigned	      num_times_occurs;
   unsigned	      target_cnt;
   unsigned	      fallthru_cnt;
   int		      last_path_on;
   unsigned	      br_hist[MAX_BR_HIST_DEPTH];
   struct _hash_entry *next;
} HASH_ENTRY;

extern unsigned   br_corr_order;

static HASH_ENTRY **hash_tbl;
static HASH_ENTRY *hash_mem;

static unsigned   hash_mem_entries;
static int	  curr_path;

static HASH_ENTRY *get_hash_entry  (unsigned *, int, int);
static HASH_ENTRY *make_hash_entry (unsigned *, int, int);
static int cmp_condbranch (HASH_ENTRY *, HASH_ENTRY *);

/************************************************************************
 *									*
 *				init_condbranch				*
 *				---------------				*
 *									*
 ************************************************************************/

init_condbranch ()
{
   hash_mem = (HASH_ENTRY *)  malloc (HASH_MEM_SIZE * sizeof (hash_mem[0]));
   hash_tbl = (HASH_ENTRY **) malloc (HASH_TBL_SIZE * sizeof (hash_tbl[0]));

   assert (hash_mem);
   assert (hash_tbl);

   /* If this assert fails, increase MAX_BR_HIST_DEPTH to desired size */
   assert (br_corr_order < MAX_BR_HIST_DEPTH);

   hash_mem_entries = 0;
   curr_path = 0;
}

/************************************************************************
 *									*
 *				dump_condbranch_summary			*
 *				-----------------------			*
 *									*
 ************************************************************************/

dump_condbranch_summary (fname)
char *fname;
{
   int	      cnt1;
   int	      cnt2;
   int	      cnt3;
   int	      cnt4;
   unsigned   i, j;
   FILE	      *fp_br;
   HASH_ENTRY *h;
   char	      buff[512];

   strcpy (buff, fname);
   strcat (buff, ".condbr");

   fp_br = fopen (buff, "w");
   assert (fp_br);

   qsort (hash_mem, hash_mem_entries, sizeof (hash_mem[0]), cmp_condbranch);

#ifdef DEBUGGING
   dump_branch_stats (fp_br);

   fprintf (fp_br, "                |---- DYNAMIC ----| |--- STATIC --|\n");
   fprintf (fp_br, "Branch Addr     Goto Targ  Fallthru # Paths # Times\n");
   fprintf (fp_br, "                                    Occurs   Occurs\n");
   fprintf (fp_br, "-----------     ---------  -------- ------- -------\n");

   for (i = 0; i < hash_mem_entries; i++) {
      h = &hash_mem[i];
      assert (h->br_hist[0] == h->address);
      fprintf (fp_br, "0x%08x:  %8u  %8u     %4u  %4u\n",
	       h->address, h->target_cnt, h->fallthru_cnt,
	       h->num_paths_occurs, h->num_times_occurs);

      for (j = 1; j <= br_corr_order; j++)
	 fprintf (fp_br, "0x%08x\n", h->br_hist[j]);

      if (br_corr_order > 0) fprintf (fp_br, "\n");
   }
#else
   cnt1 = fwrite (&hash_mem_entries, sizeof (hash_mem_entries), 1, fp_br);
   assert (cnt1 == 1);

   cnt1 = fwrite (&br_corr_order, sizeof (br_corr_order), 1, fp_br);
   assert (cnt1 == 1);

   for (i = 0; i < hash_mem_entries; i++) {
      h = &hash_mem[i];
      assert (h->br_hist[0] == h->address);

      cnt2=fwrite (h->br_hist, sizeof(h->br_hist[0]), br_corr_order+1, fp_br);
      assert (cnt2 == br_corr_order + 1);

      cnt3 = fwrite (&h->target_cnt,   sizeof (h->target_cnt),   1, fp_br);
      cnt4 = fwrite (&h->fallthru_cnt, sizeof (h->fallthru_cnt), 1, fp_br);
      assert (cnt3 == 1);
      assert (cnt4 == 1);
   }
#endif

   fclose (fp_br);
}

/************************************************************************
 *									*
 *				dump_branch_stats			*
 *				-----------------			*
 *									*
 ************************************************************************/

dump_branch_stats (fp)
FILE *fp;
{
   int        i;
   unsigned   val;
   double     good_predict     = 0.0;   
   double     bad_predict      = 0.0;
   double     one_direc_dyn    = 0.0;
   double     one_direc_static = 0.0;
   HASH_ENTRY *h;
   char	      suffix[2];

   for (i = 0; i < hash_mem_entries; i++) {
      h = &hash_mem[i];
      if (h->target_cnt > h->fallthru_cnt) {
	 good_predict += h->target_cnt;
	 bad_predict  += h->fallthru_cnt;
      }
      else {
	 good_predict += h->fallthru_cnt;
	 bad_predict  += h->target_cnt;
      }

      if (h->target_cnt < 1.0) {
         if (h->fallthru_cnt >= 1.0) {
	    one_direc_dyn    += h->fallthru_cnt;
	    one_direc_static += 1.0;
	 }
      }
      else if (h->fallthru_cnt < 1.0) {
	    one_direc_dyn    += h->target_cnt;
	    one_direc_static += 1.0;
      }
   }

   fprintf (fp, "%4.1f%% prediction accuracy if assume probable direction always taken\n",
	    (100.0 * good_predict) / (good_predict + bad_predict));

   make_readable (good_predict + bad_predict, &val, suffix);
   fprintf (fp, "%4u%s conditional branches executed\n",
	    val, suffix);

   make_readable ((double) hash_mem_entries, &val, suffix);
   fprintf (fp, "%4u%s conditional branch paths executed at least once\n",
	    val, suffix);

   fprintf (fp, "%4.1f%% of executed conditional branch paths go in only 1 direction\n",
	    (100.0 * one_direc_dyn) / (good_predict + bad_predict));

   fprintf (fp, "%4.1f%% of static   conditional branch paths go in only 1 direction\n\n",
	    (100.0 * one_direc_static) / hash_mem_entries);
}

/************************************************************************
 *									*
 *				make_readable				*
 *				-------------				*
 *									*
 ************************************************************************/

make_readable (in_val, out_val, suffix)
double   in_val;
unsigned *out_val;
char     *suffix;
{
   if      (in_val < 10000.0) {
      *out_val  = (unsigned) in_val;
      suffix[0] = ' ';
   }
   else if (in_val < 10000000.0) {
      *out_val  = (unsigned) (in_val / 1000.0);
      suffix[0] = 'K';
   }
   else if (in_val < 10000000000.0) {
      *out_val  = (unsigned) (in_val / 1000000.0);
      suffix[0] = 'M';
   }
   else {
      *out_val  = (unsigned) (in_val / 1000000000.0);
      suffix[0] = 'G';
   }

   suffix[1] = 0;
}

/************************************************************************
 *									*
 *				save_condbranch_info			*
 *				--------------------			*
 *									*
 ************************************************************************/

save_condbranch_info (num_conds, branch_dir, branch_addr, times_exec)
int	       num_conds;
unsigned char  *branch_dir;
unsigned       *branch_addr;
unsigned       times_exec;
{
   int	      i;
   int	      offset;
   int	      hashval;
   HASH_ENTRY *entry;

   offset = num_conds - 1;
   for (i = 0; i < num_conds; i++, offset--) {
      entry             = get_hash_entry  (branch_addr, i, offset);
      if (!entry) entry = make_hash_entry (branch_addr, i, offset);

      entry->num_times_occurs++;

      if (entry->last_path_on != curr_path) {
	 entry->num_paths_occurs++;
	 entry->last_path_on = curr_path;
      }

      if (branch_dir[offset]) entry->target_cnt   += times_exec;
      else		      entry->fallthru_cnt += times_exec;
   }

   curr_path++;
}

/************************************************************************
 *									*
 *				get_hash_entry				*
 *				--------------				*
 *									*
 * Returns pointer to hash entry if it exists, 0 otherwise.		*
 *									*
 ************************************************************************/

static HASH_ENTRY *get_hash_entry (br_hist, depth, offset)
unsigned *br_hist;
int	 depth;
int	 offset;
{
   int	      i;
   int	      hashval;
   unsigned   branch_addr;
   HASH_ENTRY *h;

   branch_addr = br_hist[offset];
   hashval = get_hashval (branch_addr);
   h = hash_tbl[hashval];

   /* Assign values beyond our current depth to match NO_CORR_BR_AVAIL */
   for (i = depth+1; i <= br_corr_order; i++)
      br_hist[i + offset] = NO_CORR_BR_AVAIL;

   while (h) {
      if (h->address == branch_addr) {
	 for (i = 0; i <= br_corr_order; i++)
	    if (h->br_hist[i] != br_hist[i + offset]) break;

	 if (i > br_corr_order) return h;
	 else h = h->next;
      }
      else h = h->next;
   }

   return 0;		/* Did not find a hash entry for this address */
}

/************************************************************************
 *									*
 *				make_hash_entry				*
 *				---------------				*
 *									*
 * Make hash entry.  Set "address" to "branch_addr" and initialize	*
 * counts to 0.								*
 *									*
 ************************************************************************/

static HASH_ENTRY *make_hash_entry (br_hist, depth, offset)
unsigned *br_hist;
int	 depth;
int	 offset;
{
   int	      i;
   int	      limit;
   int	      hashval;
   unsigned   branch_addr;
   HASH_ENTRY *h;
   HASH_ENTRY *new_entry;
   HASH_ENTRY *last_entry;

   assert (hash_mem_entries < HASH_MEM_SIZE);
   new_entry = &hash_mem[hash_mem_entries++];

   branch_addr = br_hist[offset];

   new_entry->address = branch_addr;
   new_entry->last_path_on = -1;
   new_entry->num_paths_occurs = 0;
   new_entry->num_times_occurs = 0;
   new_entry->target_cnt = 0;
   new_entry->fallthru_cnt = 0;
   new_entry->next = 0;

   if (depth < MAX_BR_HIST_DEPTH - 1) limit = depth;
   else				      limit = MAX_BR_HIST_DEPTH - 1;
   for (i = 0; i <= limit; i++)
      new_entry->br_hist[i] = br_hist[i + offset];

   for (; i <= br_corr_order; i++) 
      new_entry->br_hist[i] = NO_CORR_BR_AVAIL;

   hashval = get_hashval (branch_addr);
   h = hash_tbl[hashval];

   /* Are we the first entry at this point in the hash table? */
   if (!h) hash_tbl[hashval] = new_entry;
   else {

      last_entry = h;
      while (h) {
	 /* get_hash_entry should be called first and precludes this */
	 if (h->address == branch_addr  &&  br_corr_order == 0) assert (1==0);

	 last_entry = h;
	 h = h->next;
      }

      last_entry->next = new_entry;
   }

   return new_entry;
}

/************************************************************************
 *									*
 *				get_hashval				*
 *				-----------				*
 *									*
 ************************************************************************/

static get_hashval (branch_addr)
unsigned branch_addr;
{
   return (branch_addr >> 2) & HASH_TBL_MASK;
}

/************************************************************************
 *									*
 *				cmp_condbranch				*
 *				---------------				*
 *									*
 ************************************************************************/

static int cmp_condbranch (a, b)
HASH_ENTRY *a;
HASH_ENTRY *b;
{
   int i;

   if	   (a->address > b->address) return  1;
   else if (a->address < b->address) return -1;
   else for (i = 0; i < br_corr_order; i++) {
      if      (a->br_hist[i] > b->br_hist[i]) return  1;
      else if (a->br_hist[i] < b->br_hist[i]) return -1;
   }

   return 0;
}
