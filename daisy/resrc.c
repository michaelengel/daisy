/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "vliw.h"

extern int is_daisy1;	       /* Boolean */
extern int curr_rename_reg_valid;
extern int fits_cluster;
extern int num_clusters;
extern int extra_resrc[];
extern int cluster_resrc[];
extern int num_resrc[];

static int resrc_limit[NUM_VLIW_RESRC_TYPES+1];

void get_needed_resrcs (OPCODE2 *, int *);

/************************************************************************
 *									*
 *				init_resrc_usage			*
 *				----------------			*
 *									*
 ************************************************************************/

init_resrc_usage ()
{
   int i;

   for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++)
      resrc_limit[i] = num_resrc[i];
}

/************************************************************************
 *									*
 *				opcode_fits				*
 *				-----------				*
 *									*
 * Returns TRUE if "op" fits in "tip", FALSE otherwise.			*
 *									*
 ************************************************************************/

int opcode_fits (op, first_tip, last_tip, do_renaming, same_clust_req,
		 resrc_needed)
OPCODE2 *op;
TIP	*first_tip;
TIP	*last_tip;
int	do_renaming;
int     same_clust_req;	   /* Boolean:  TRUE when "op" must go in same   */
			   /*	        cluster as inputs to meet timing */
int     *resrc_needed;
{
   int	    i;
   int	    save_valid;
   int	    legal_cluster;
   int	    *resrc_used;

   /* If this is not a commit, make sure we have a free, 
      non-PPC-architected register for the result.  This would have
      to be generalized if we made condition code registers and GPR's
      renamable in the same instruction (e.g. renaming both the GPR
      and CCR for ai.)
   */
   if (do_renaming)
      if (!set_reg_for_op (op, first_tip, last_tip)) return FALSE;

   resrc_used = first_tip->vliw->resrc_cnt;

   /* Temporarily mark rename invalid in case we fail other resrc reqs */
   save_valid = curr_rename_reg_valid;
   curr_rename_reg_valid = FALSE;

   adjust_resrc_limit (resrc_used);

   for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++) {
      if (resrc_used[i] + resrc_needed[i] + extra_resrc[i] > 
	  resrc_limit[i]) return FALSE;
   }

   if (num_clusters < 2) legal_cluster = 1;
   else if (!(legal_cluster =
	      any_cluster_avail (first_tip, op, same_clust_req, resrc_needed)))
      return FALSE;

   fits_cluster	         = legal_cluster;
   curr_rename_reg_valid = save_valid;
   return TRUE;
}

/************************************************************************
 *									*
 *				incr_resrc_usage			*
 *				----------------			*
 *									*
 * Bump the resource usage for all (function-unit type) resources used	*
 * by op.  The "resrc_used" array is typically from a VLIW.		*
 *									*
 ************************************************************************/

incr_resrc_usage (tip, resrc_used, resrc_needed, clust_num)
TIP	*tip;
int     *resrc_used;
int     *resrc_needed;
int     clust_num;
{
   int   i;
   short *clust_used = tip->vliw->cluster[clust_num-1];

   if (num_clusters < 2) {
        for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++)
           resrc_used[i] += resrc_needed[i];
   }
   else for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++) {
           resrc_used[i] += resrc_needed[i];
	   if (cluster_resrc[i]) clust_used[i] += resrc_needed[i];
   }
}

/************************************************************************
 *									*
 *				decr_resrc_usage			*
 *				----------------			*
 *									*
 * Reduce the resource usage for all (function-unit type) resources	*
 * used by op.  The "resrc_used" array is typically from a VLIW.	*
 *									*
 ************************************************************************/

decr_resrc_usage (tip, resrc_used, resrc_needed, clust_num)
TIP	*tip;
int     *resrc_used;
int     *resrc_needed;
int     clust_num;
{
   int   i;
   short *clust_used = tip->vliw->cluster[clust_num-1];

   if (num_clusters < 2) {
        for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++)
           resrc_used[i] -= resrc_needed[i];
   }
   else for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++) {
           resrc_used[i] -= resrc_needed[i];
	   if (cluster_resrc[i]) clust_used[i] -= resrc_needed[i];
   }
}

/************************************************************************
 *									*
 *				get_needed_resrcs			*
 *				-----------------			*
 *									*
 * Determine the number of function unit and RD/WR port resources are	*
 * required by "op" and return them in the "resrc_needed" vector passed	*
 * by the caller.							*
 *									*
 * NOTE:  Unlike Foresta, DAISY branch ops do not consume ISSUE slots.	*
 *									*
 ************************************************************************/

void get_needed_resrcs (op, resrc_needed)
OPCODE2 *op;
int     *resrc_needed;
{
   int i;

   for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++)
      resrc_needed[i] = 0;

   cnt_explicit_ports (op->op.rd_explicit, op->op.num_rd,
		       &resrc_needed[GPR_RD_PORTS],
		       &resrc_needed[FPR_RD_PORTS],
		       &resrc_needed[CCR_RD_PORTS]);

   cnt_explicit_ports (op->op.wr_explicit, op->op.num_wr,
		       &resrc_needed[GPR_WR_PORTS],
		       &resrc_needed[FPR_WR_PORTS],
		       &resrc_needed[CCR_WR_PORTS]);

   if      (is_branch (op)) {
      /* In DAISY, branch ops do not consume ISSUE slots */
      if (op->b->primary == BCOND_PRIMARY) {
	 resrc_needed[SKIP]   = 1;
	 resrc_needed[BRLEAF] = 1;	/* Allocate room for leaf with SKIP */
      }
   }
   else if (is_memop  (op)) {
      resrc_needed[MEMOP] = 1;
      resrc_needed[ISSUE] = 1;
   }
   else {
      resrc_needed[ALU]   = 1;
      resrc_needed[ISSUE] = 1;
   }
}

/************************************************************************
 *									*
 *				adjust_resrc_limit			*
 *				------------------			*
 *									*
 * This routine is very architecture specific.				*
 *									*
 ************************************************************************/

adjust_resrc_limit (resrc_used)
int *resrc_used;
{
   int i;

   if (!is_daisy1) return;
   else {
      assert (num_resrc[ISSUE] == 8);
      if (resrc_used[SKIP] > 0) resrc_limit[ISSUE] = 6;
      else			resrc_limit[ISSUE] = 8;
   }
}

