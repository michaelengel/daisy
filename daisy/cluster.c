/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*======================================================================*
 * This module is quite similar to loophdr.c in its memory management.	*
 * It would probably be good to merge them at some point, along with	*
 * the memory management for other values kept per path in the		*
 * XLATE_POINT struct.							*
 *======================================================================*/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "vliw.h"
#include "resrc_list.h"

#define MAX_OPEN_PATHS		512

typedef struct _clustmem {
   unsigned char    *mem;
   struct _clustmem *next;
} CLUSTMEM;

extern int	     is_daisy1;	       /* Boolean */
extern int	     num_clusters;     /* 0 ==> Not have clustered machine   */
extern int	     unusable_cluster; /* (-1) if all clusters usable	     */
extern int	     cross_cluster_penalty;
extern int	     cluster_resrc_cnt[][NUM_VLIW_RESRC_TYPES+1];
extern int	     cluster_resrc[];  /* Bool: Reesource assoc with	     */
				       /*       clusters/whole machine	     */
extern int	     extra_resrc[];

static unsigned	     max_paths_open;
static unsigned	     num_paths_open;
static CLUSTMEM	     *free_mem;
static CLUSTMEM	     clust_list_mem[MAX_OPEN_PATHS];
static unsigned char clust_mem[MAX_OPEN_PATHS][NUM_MACHINE_REGS];

static void set_clust_lim (TIP *, int, int *, int *);

/************************************************************************
 *									*
 *				init_cluster_mem			*
 *				----------------			*
 *									*
 ************************************************************************/

init_cluster_mem ()
{
   int		 i;
   unsigned char *p       = &clust_mem[0][0];
   CLUSTMEM	 *listmem = clust_list_mem;

   free_mem = clust_list_mem;

   for (i = 0; i < MAX_OPEN_PATHS; i++, listmem++) {
      listmem->mem  = clust_mem[i];
      listmem->next = listmem + 1;
   }

   /* The last element of the list has a NULL "next" */
   listmem--;
   listmem->next = 0;
}

/************************************************************************
 *									*
 *				page_verif_cluster_mem			*
 *				----------------------			*
 *									*
 * This routine is for debugging purposes only.				*
 *									*
 ************************************************************************/

int page_verif_cluster_mem ()
{
   int	    i;
   int	    cnt;
   CLUSTMEM *p;

   assert (num_paths_open == 0);

#ifdef DEBUGGING
   cnt		 = 0;
   for (p = free_mem; p; p = p->next, cnt++);

   if (cnt != MAX_OPEN_PATHS) {
      printf ("cnt = %d   MAX_OPEN_PATHS = %d\n", cnt, MAX_OPEN_PATHS);

      for (i = 0, p = free_mem; p; p = p->next, i++) {
         printf (" %3d", (p->mem - clust_mem));
	 if ((i & 7) == 7) printf ("\n");
      }

      exit (1);
   }
#endif
}

/************************************************************************
 *									*
 *				get_cluster_mem				*
 *				---------------				*
 *									*
 ************************************************************************/

unsigned char *get_cluster_mem ()
{
   unsigned char *rval;

   /* If assert fails, CLUSTER_MEMSIZE should be increased */
   assert (free_mem);

   rval     = free_mem->mem;
   free_mem = free_mem->next;

   if (++num_paths_open > max_paths_open) max_paths_open = num_paths_open;

   return rval;
}

/************************************************************************
 *									*
 *				free_cluster_mem			*
 *				----------------			*
 *									*
 * Make "mem" available for another path to use, and clear it so that	*
 * no location on the path is yet visited.  As each path closes, e.g.	*
 * via an OFFPAGE branch, this routine should be invoked.  This being	*
 * the case, no special cleanup is needed at the start of page		*
 * processing.								*
 *									*
 ************************************************************************/

free_cluster_mem (mem)
unsigned char *mem;
{
   int index;

   index = (mem-&clust_mem[0][0]) / (NUM_MACHINE_REGS*sizeof(clust_mem[0][0]));
   assert (clust_list_mem[index].mem == mem);

   mem_clear (mem, NUM_MACHINE_REGS * sizeof (clust_mem[0][0]));

   clust_list_mem[index].next = free_mem;
   free_mem = &clust_list_mem[index];

   num_paths_open--;
}

/************************************************************************
 *									*
 *				any_cluster_avail			*
 *				-----------------			*
 *									*
 * Returns positive cluster number if a cluster is available, 0 o.w.	*
 *									*
 ************************************************************************/

int any_cluster_avail (tip, op, same_clust_req, resrc_needed)
TIP	 *tip;
OPCODE2  *op;
int      same_clust_req;   /* Boolean:  TRUE when "op" must go in same   */
			   /*	        cluster as inputs to meet timing */
int     *resrc_needed;
{
   int	      i;
   int	      input_cluster;
   int	      is_avail;
   int	      old_clust_num;
   static int clust_num = 1;

   input_cluster = latest_inputs_same_cluster (tip, op);
   if (input_cluster > 0) {
      if (cluster_avail (tip, input_cluster, resrc_needed))
			       return input_cluster;
      else if (same_clust_req) return 0; /* Can't go in same cluster as inps */
   }

   /* Use round-robin to assign clusters when can't get input_cluster */
   for (i = 1; i <= num_clusters; i++) {
      is_avail = cluster_avail (tip, clust_num, resrc_needed);

      old_clust_num = clust_num;
      if (clust_num < num_clusters) clust_num++;
      else			    clust_num = 1;

      if (is_avail) return old_clust_num;
   }

   return 0;
}

/************************************************************************
 *									*
 *				cluster_avail				*
 *				-------------				*
 *									*
 * Returns TRUE if "op" fits in "cluster", FALSE otherwise.		*
 *									*
 ************************************************************************/

int cluster_avail (tip, cluster, resrc_needed)
TIP *tip;
int cluster;
int *resrc_needed;
{
   int      i;
   int	    clust_lim[NUM_VLIW_RESRC_TYPES+1];
   short    *clust_used;

   if (cluster == unusable_cluster) return FALSE;

   clust_used = tip->vliw->cluster[cluster-1];
   set_clust_lim (tip, cluster, cluster_resrc_cnt[cluster-1], clust_lim);

   for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++) {
      if (!cluster_resrc[i]) continue;
      else if (clust_used[i] + resrc_needed[i] + extra_resrc[i] > 
	       clust_lim[i]) return FALSE;
   }

   return TRUE;
}

/************************************************************************
 *									*
 *				inputs_same_cluster			*
 *				-------------------			*
 *									*
 * Returns -1 if inputs on different clusters.				*
 *          0 if no inputs have been assigned in this group.		*
 *	    <Positive Cluster Number> if all inputs in same cluster.	*
 *									*
 ************************************************************************/

int inputs_same_cluster (tip, op)
TIP     *tip;
OPCODE2 *op;
{
   int		  i;
   int		  base_cluster;
   int		  num_rd = op->op.num_rd;
   unsigned short *rd    = op->op.rd;

   for (i = 0; i < num_rd; i++)
      if (tip->cluster[rd[i]]) {base_cluster = tip->cluster[rd[i]];   break;}

   if (i == num_rd) return 0;

   for (; i < num_rd; i++) {
      if (tip->cluster[rd[i]]  &&  tip->cluster[rd[i]] != base_cluster)
	 return -1;
   }

   return base_cluster;
}

/************************************************************************
 *									*
 *				latest_inputs_same_cluster		*
 *				--------------------------		*
 *									*
 * Returns -1 if inputs on different clusters.				*
 *          0 if no inputs have been assigned in this group.		*
 *	    <Positive Cluster Number> if				*
 *              (1) All inputs finishing within				*
 *                  (cross_cluster_penalty-1) of the latest finisher	*
 *		    are in the same cluster, and			*
 *	        (2) opcode fits in that cluster				*
 *									*
 *     ** Note that all inputs in the same cluster is a subcase of (1).	*
 *									*
 ************************************************************************/

int latest_inputs_same_cluster (tip, op)
TIP     *tip;
OPCODE2 *op;
{
   int		  i;
   int	          latest;
   int	          latest_cluster;
   int		  num_rd = op->op.num_rd;
   unsigned short *rd    = op->op.rd;

   latest = 0;
   for (i = 0; i < num_rd; i++) {
      if (tip->avail[rd[i]] > latest) {
	 latest = tip->avail[rd[i]];
	 latest_cluster = tip->cluster[rd[i]];
      }
   }

   if (latest == 0) return 0;

   for (; i < num_rd; i++) {
      if (tip->cluster[rd[i]]		         &&
	  tip->cluster[rd[i]] != latest_cluster  &&
	  tip->avail[rd[i]] + cross_cluster_penalty - 1 >= latest)
	 return -1;
   }

   return latest_cluster;
}

/************************************************************************
 *									*
 *			do_reg_cluster_assignments			*
 *			--------------------------			*
 *									*
 * Assign all destinations of op to "clust_num".  We even set memory	*
 * since no one checks it and it would be slower to do the check to	*
 * avoid setting memory.						*
 *									*
 ************************************************************************/

do_reg_cluster_assignments (tip, op, clust_num)
TIP     *tip;
OPCODE2 *op;
int     clust_num;
{
   int    i;
   int    reg;
   int    num_wr = op->op.num_wr;
   RESULT *wr = op->op.wr;

   for (i = 0; i < num_wr; i++) {
      reg = wr[i].reg;
      tip->cluster[reg] = clust_num;
   }
}

/************************************************************************
 *									*
 *				is_cluster_empty			*
 *				----------------			*
 *									*
 * Returns TRUE if there are no operations currently in the "clust_num".*
 *	   FALSE otherwise.						*
 *									*
 ************************************************************************/

int is_cluster_empty (vliw, clust_num)
VLIW *vliw;
int  clust_num;
{
   int i;

   /* If not using clustering, always empty.  Likewise if "dummy" cluster */
   if (num_clusters == 0)  return TRUE;
   else if (clust_num < 1) return TRUE;	     

   for (i = 0; i < NUM_VLIW_RESRC_TYPES+1; i++)
      if (vliw->cluster[clust_num][i] !=
	  cluster_resrc_cnt[clust_num][i]) return FALSE;

   return TRUE;
}

/************************************************************************
 *									*
 *				is_mem_locn				*
 *				-----------				*
 *									*
 * Returns TRUE if "val" references memory (as opposed to some		*
 *              processor register or status bit).			*
 *	   FALSE otherwise.						*
 *									*
 ************************************************************************/

int is_mem_locn (val)
int val;
{
   switch (val) {
      case MEM:
      case ICACHE:
      case DCACHE:
      case TLB:
      case PFT:
      case ATOMIC_RES:		return TRUE;
      default:			return FALSE;
   }
}

/************************************************************************
 *									*
 *				set_clust_lim				*
 *				-------------				*
 *									*
 * This routine is very architecture specific.				*
 *									*
 ************************************************************************/

static void set_clust_lim (tip, clust_num, hard_clust_lim, soft_clust_lim)
TIP *tip;
int clust_num;
int *hard_clust_lim;	      /* Input  */
int *soft_clust_lim;	      /* Output */
{
   int i;

   for (i = 0; i < NUM_VLIW_RESRC_TYPES + 1; i++)
      soft_clust_lim[i] = hard_clust_lim[i];

   if (!is_daisy1) return;
   else if (clust_num == 2  &&  tip->vliw->resrc_cnt[SKIP] > 0) {
      assert (hard_clust_lim[ISSUE] == 4);
      soft_clust_lim[ISSUE] = 2;
   }
}
