/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "resrc_offset.h"

#define MAX_LVFAIL_SITES		4096

typedef struct {
   unsigned *addr;
   unsigned cnt;
} LVFAIL;

int		     recompiling;
int		     lvfail_sites;
int		     num_ins_in_loadver_fail_simgen = 3;
unsigned	     lv_vliw_ops;
unsigned	     lv_recomp_vliw_ops;
unsigned	     lv_recomp_cnt;

extern int	     lv_fail_recomp_cnt;
extern char	     *daisy_progname;
extern unsigned      xlate_entry_raw_ptr;
extern unsigned	     total_vliw_ops;
extern char	     curr_wd[];
extern unsigned char r13_area[];

static LVFAIL	     lvfail[MAX_LVFAIL_SITES];

static double	     num_loadver_fails;

unsigned *loadver_fail (unsigned *);
static LVFAIL *seen_loadver_fail (unsigned *);
static LVFAIL *new_loadver_fail  (unsigned *);

/************************************************************************
 *									*
 *				loadver_fail_simgen			*
 *				-------------------			*
 *									*
 * Dump code to branch indirectly offpage.  The pointer we jump to	*
 * points to the assembly language entry pt for a LOAD-VERIFY failure.	*
 *									*
 * ASSUMES: r27 has already been loaded with address to branch to	*
 *	    in original code.						*
 *									*
 * WARNING:  If the number of instrucs in this code is modified		*
 *	     "load_verify_v2r" in simul.c should also be changed	*
 *	     so that its branch-around offset will be correct.		*
 *									*
 ************************************************************************/

loadver_fail_simgen ()
{
   assert (num_ins_in_loadver_fail_simgen == 3);

   /* LR = r29 = address to jump to */
   dump (0x83AD0000 | LDVER_FCN);	/* l	 r29,LDVER_FCN(r13) */
   dump (0x7FA803A6);			/* mtspr LR,r29		    */
   dump (0x4E800020);			/* bcr   (UNCOND)	    */
}

/************************************************************************
 *									*
 *				loadver_fail				*
 *				------------				*
 *									*
 * Invoked whenever a LOAD-VERIFY fails.				*
 *									*
 ************************************************************************/

unsigned *loadver_fail (fail_addr)
unsigned *fail_addr;
{
   LVFAIL   *fail;
   unsigned *rval;
   unsigned *page_entry;
   unsigned total_ops_in;

   num_loadver_fails++;

   if  (fail = seen_loadver_fail (fail_addr)) fail->cnt++;
   else fail =  new_loadver_fail (fail_addr);

   /* If a LOAD-VERIFY from a particular site fails enough,
      recompile the original page, this time without moving
      the problem LOAD-VERIFY.
   */
   if (fail->cnt >= lv_fail_recomp_cnt) {
      page_entry   = *((unsigned **) &r13_area[PAGE_ENTRY]);
      total_ops_in = total_vliw_ops;

      recompiling = TRUE;
      xlate_entry (page_entry);
      recompiling = FALSE; 

      lv_recomp_cnt++;
      lv_recomp_vliw_ops += total_vliw_ops - total_ops_in;
   }

   rval = get_xlated_addr (fail_addr);
   if (rval == xlate_entry_raw_ptr) {
      total_ops_in = total_vliw_ops;
      rval = xlate_entry (fail_addr);
      lv_vliw_ops += total_vliw_ops - total_ops_in;
   }

   return rval;
}

/************************************************************************
 *									*
 *				had_loadver_fail			*
 *				----------------			*
 *									*
 * Returns TRUE if the LOAD associated with "addr" has had a		*
 *	   LOAD-VERIFY failure.						*
 *									*
 * This routine just does a brute force search through the array. Since	*
 * early indications are that the array has few entries, this should	*
 * not cause too many problems.  This should be re-evaluated at some	*
 * point.								*
 *									*
 ************************************************************************/

int had_loadver_fail (addr)
unsigned *addr;
{
   int i;

   for (i = 0; i < lvfail_sites; i++)
      if (addr == lvfail[i].addr) return TRUE;

   return FALSE;
}

/************************************************************************
 *									*
 *				dump_loadver_summary			*
 *				--------------------			*
 *									*
 ************************************************************************/

dump_loadver_summary ()
{
   int    i;
   char   *simul_basename;
   double total;
   FILE   *fp;
   static char lvf_fname[256];
   int    cmp_lvfail (LVFAIL *, LVFAIL *);

   simul_basename = strip_dir_prefix (daisy_progname);
   strcpy (lvf_fname, curr_wd);
   strcat (lvf_fname, "/");
   strcat (lvf_fname, simul_basename);
   strcat (lvf_fname, ".lvfail");

   fp = fopen (lvf_fname, "w");
   assert (fp);

   qsort (lvfail, lvfail_sites, sizeof lvfail[0], cmp_lvfail);
   total = 0.0;
   for (i = 0; i < lvfail_sites; i++) {
      fprintf (fp, "0x%08x:  %11u\n", lvfail[i].addr, lvfail[i].cnt);
      total += lvfail[i].cnt;
   }

   fprintf (fp, "TOTAL:       %11.0f\n", total);
   assert (total == num_loadver_fails);

   fclose (fp);
}

/************************************************************************
 *									*
 *				cmp_lvfail				*
 *				----------				*
 *									*
 * Sort in descending order.						*
 *									*
 ************************************************************************/

static int cmp_lvfail (a, b)
LVFAIL *a;
LVFAIL *b;
{
   if      (a->cnt  > b->cnt)  return -1;
   else if (a->cnt  < b->cnt)  return  1;
   else if (a->addr > b->addr) return  1;
   else if (a->addr < b->addr) return -1;
   else			       return  0;
}

/************************************************************************
 *									*
 *				seen_loadver_fail			*
 *				-----------------			*
 *									*
 ************************************************************************/

static LVFAIL *seen_loadver_fail (addr)
unsigned *addr;
{
   int i;

   for (i = 0; i < lvfail_sites; i++)
      if (lvfail[i].addr == addr) return &lvfail[i];

   /* Have not seen this failure before */
   return 0;
}

/************************************************************************
 *									*
 *				new_loadver_fail			*
 *				----------------			*
 *									*
 ************************************************************************/

static LVFAIL *new_loadver_fail (addr)
unsigned *addr;
{
   LVFAIL *fail;

   /* If assert fails, increase MAX_LVFAIL_SITES */
   assert (lvfail_sites < MAX_LVFAIL_SITES);

   fail = &lvfail[lvfail_sites++];

   fail->cnt  = 1;
   fail->addr = addr;

   return fail;
}
