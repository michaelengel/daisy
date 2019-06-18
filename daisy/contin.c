/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/* Handle continuations:
	(1) Adding them
	(2) Returning the highest priority one
*/

#include <stdio.h>
#include <assert.h>
#include <float.h>

#include "vliw.h"
#include "contin.h"

#define XLATE_LIST_SIZE		(32*MAX_RPAGE_ENTRIES)

extern REG_RENAME     *map_mem[][NUM_PPC_GPRS_C+NUM_PPC_FPRS+NUM_PPC_CCRS];
extern MEMACC	      *stores_mem[][NUM_MA_HASHVALS];
extern unsigned	      *gpr_wr_mem[][NUM_PPC_GPRS_C];
extern unsigned short avail_mem[][NUM_MACHINE_REGS];
extern int	      avail_mem_cnt;
extern int	      use_pdf_cnts;
extern int	      use_pdf_prob;
extern int	      loophdr_bytes_per_path;

static unsigned	   xlate_list_size;
static XLATE_POINT *curr_xlate_ptr;
static XLATE_POINT xlate_list[XLATE_LIST_SIZE];
static XLATE_POINT *xlate_heap[XLATE_LIST_SIZE];
static TIP	   dummy_tip;
static VLIW	   dummy_vliw;
static XLATE_POINT dummy_xl_pt;

TIP *init_continuation_page (unsigned *);
int  xlate_list_cmp    (XLATE_POINT *, XLATE_POINT *);
void xlate_list_setmax (XLATE_POINT *);

/************************************************************************
 *									*
 *				init_continuation			*
 *				-----------------			*
 *									*
 ************************************************************************/

init_continuation ()
{
   dummy_tip.vliw = &dummy_vliw;
   dummy_tip.left = 0;		    /* This is a leaf tip */
   dummy_vliw.group = INT_MIN;	    /* Make sure not sched with other groups */
   dummy_xl_pt.source = &dummy_tip;
}

/************************************************************************
 *									*
 *				init_continuation_page			*
 *				----------------------			*
 *									*
 ************************************************************************/

TIP *init_continuation_page (entry)
unsigned *entry;
{
   xlate_list_size	   = 1;
   xlate_list[0].ppc_src  = entry - 1;
   xlate_list[0].source    = &dummy_tip;
   xlate_list[0].target    = entry;
   xlate_list[0].targ_type = UCFALL_THRU;
   xlate_list[0].serialize = TRUE;		/* Init entry starts VLIW */
   curr_xlate_ptr	   = xlate_list;

   if (use_pdf_cnts) xlate_list[0].prob = DBL_MAX / 2;
   else		     xlate_list[0].prob = 1.00;

   heap_insert (&xlate_list[0], xlate_heap, 0,
		&dummy_xl_pt, xlate_list_cmp, xlate_list_setmax);

   return &dummy_tip;
}

/************************************************************************
 *									*
 *				get_highest_prio_cont			*
 *				---------------------			*
 *									*
 * RETURNS:  The highest priority continuation or NULL if there are no	*
 *	     more continuations on this page.				*
 *									*
 * NOTE:     Until we implement the heap, we cheat and just return the	*
 *	     first continuation in the list.				*
 *									*
 ************************************************************************/

XLATE_POINT *get_highest_prio_cont ()
{
   if (xlate_list_size == 0) return 0;
   else {
      xlate_list_size--;
      curr_xlate_ptr++;
      return heap_remove (xlate_heap, xlate_list_size+1, xlate_list_cmp);
/*    return curr_xlate_ptr++;*/
   }
}

/************************************************************************
 *									*
 *				add_to_xlate_list			*
 *				-----------------			*
 *									*
 * Make this into a heap data structure so can efficiently add entries	*
 * and extract those with the highest priority/probability		*
 *									*
 ************************************************************************/

add_to_xlate_list (tip, ppc_src, ppc_targ, prob, targ_type, serialize)
TIP	 *tip;			/* Tip from which invoked		     */
unsigned *ppc_src;
unsigned *ppc_targ;
double	 prob;			/* How likely this path:  percent from 0-100 */
int	 targ_type;		/* UC/CDFALL_THRU, COND_TARG, UNCOND_TARG    */
int	 serialize;		/* Boolean:  TRUE if start new VLIW here     */
{
   unsigned short *avail;
   unsigned char  *cluster;
   unsigned char  *loophdr;
   unsigned       **gpr_writer;
   REG_RENAME	  **gpr_rename;
   REG_RENAME	  **fpr_rename;
   REG_RENAME	  **ccr_rename;
   MEMACC	  **path_stores;

   /* If assert fails, XLATE_LIST_SIZE should be increased */
   assert (&curr_xlate_ptr[xlate_list_size] < &xlate_list[XLATE_LIST_SIZE]);

   curr_xlate_ptr[xlate_list_size].source      = tip;
   curr_xlate_ptr[xlate_list_size].ppc_src     = ppc_src;
   curr_xlate_ptr[xlate_list_size].target      = ppc_targ;
   curr_xlate_ptr[xlate_list_size].targ_type   = targ_type;
   curr_xlate_ptr[xlate_list_size].serialize   = serialize;
   curr_xlate_ptr[xlate_list_size].stm_r1_time = tip->stm_r1_time;
   curr_xlate_ptr[xlate_list_size].last_store  = tip->last_store;
   curr_xlate_ptr[xlate_list_size].ca_rename   = tip->ca_rename;
#ifdef ACCESS_RENAMED_OV_RESULT
   curr_xlate_ptr[xlate_list_size].ov_rename   = tip->ov_rename;
#endif

   if (use_pdf_cnts) 
        curr_xlate_ptr[xlate_list_size].prob   = prob;
   else curr_xlate_ptr[xlate_list_size].prob   = prob * tip->prob;


   if (targ_type != COND_TARG) {
      avail       = tip->avail;
      gpr_writer  = tip->gpr_writer;
      gpr_rename  = tip->gpr_rename;
      fpr_rename  = tip->fpr_rename;
      ccr_rename  = tip->ccr_rename;
      path_stores = tip->stores;
      loophdr     = tip->loophdr;
      cluster     = tip->cluster;
   }
   else {
      assert (avail_mem_cnt < MAX_PATHS_PER_PAGE);

      avail	  = avail_mem[avail_mem_cnt];
      gpr_writer  = gpr_wr_mem[avail_mem_cnt];
      gpr_rename  = map_mem[avail_mem_cnt];
      fpr_rename  = gpr_rename + NUM_PPC_GPRS_C;
      ccr_rename  = fpr_rename + NUM_PPC_FPRS;
      path_stores = stores_mem[avail_mem_cnt];
      loophdr     = get_loophdr_mem ();
      cluster     = get_cluster_mem ();

      avail_mem_cnt++;

      mem_copy (avail, tip->avail, NUM_MACHINE_REGS * sizeof (tip->avail[0]));
      mem_copy (gpr_writer, tip->gpr_writer,
		NUM_PPC_GPRS_C * sizeof (gpr_writer[0]));

      mem_copy (gpr_rename, tip->gpr_rename, NUM_PPC_GPRS_C * sizeof(gpr_rename[0]));
      mem_copy (fpr_rename, tip->fpr_rename, NUM_PPC_FPRS   * sizeof(fpr_rename[0]));
      mem_copy (ccr_rename, tip->ccr_rename, NUM_PPC_CCRS   * sizeof(ccr_rename[0]));

      mem_copy (loophdr, tip->loophdr, loophdr_bytes_per_path);
      mem_copy (cluster, tip->cluster, NUM_MACHINE_REGS * sizeof (cluster[0]));
      mem_copy (path_stores, tip->stores,
		NUM_MA_HASHVALS * sizeof (path_stores[0]));

   }

   curr_xlate_ptr[xlate_list_size].avail      = avail;
   curr_xlate_ptr[xlate_list_size].gpr_writer = gpr_writer;
   curr_xlate_ptr[xlate_list_size].gpr_rename = gpr_rename;
   curr_xlate_ptr[xlate_list_size].fpr_rename = fpr_rename;
   curr_xlate_ptr[xlate_list_size].ccr_rename = ccr_rename;
   curr_xlate_ptr[xlate_list_size].stores     = path_stores;
   curr_xlate_ptr[xlate_list_size].loophdr    = loophdr;
   curr_xlate_ptr[xlate_list_size].cluster    = cluster;

   heap_insert (&curr_xlate_ptr[xlate_list_size], xlate_heap, xlate_list_size,
		&dummy_xl_pt, xlate_list_cmp, xlate_list_setmax);

   xlate_list_size++;
}

/************************************************************************
 *									*
 *				xlate_list_cmp				*
 *				--------------				*
 *									*
 * Return 1 if "a" has higher probability than "b", -1 if "a" has lower	*
 * probability, and 0 if "a" and "b" have equal probability.		*
 *									*
 ************************************************************************/

int xlate_list_cmp (a, b)
XLATE_POINT *a;
XLATE_POINT *b;
{
   /* Give preference to low group numbers.  Since group numbers
      are assigned in ascending order, (See handle_serialize in main.c),
      this will cause only one group to have multiple paths open at
      one time.
   */

   if      (a->source->vliw->group < b->source->vliw->group) return  1;
   else if (a->source->vliw->group > b->source->vliw->group) return -1;

   else if (a->prob > b->prob)				     return  1;
   else if (a->prob < b->prob)				     return -1;

   else			      				     return  0;
}

/************************************************************************
 *									*
 *				xlate_list_setmax			*
 *				-----------------			*
 *									*
 * Set the probability of "a" to something larger than any real legal	*
 * value.								*
 *									*
 ************************************************************************/

void xlate_list_setmax (a)
XLATE_POINT *a;
{
   /* This will be higher than any real probability */
   if (use_pdf_cnts) a->prob = DBL_MAX;
   else		     a->prob = 1.01;	

   a->source->vliw->group = INT_MIN;
}
