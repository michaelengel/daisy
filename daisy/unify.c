/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "unify.h"

#define MAX_UNIFY_OPS_PER_PAGE		  16384

extern int      num_gprs;
extern int      num_fprs;
extern int      num_ccbits;

extern unsigned rpage_entries;
extern unsigned rpagesize_mask;

static int   unify_op_cnt;
static UNIFY unify_mem[MAX_UNIFY_OPS_PER_PAGE];
static UNIFY *unify_page[MAX_RPAGE_ENTRIES];

static int unify_inputs_same (UNIFY   *, OPCODE2 *, TIP   *);
static int unify_ok_to_add   (OPCODE2 *, int     *, int   *);

/************************************************************************
 *									*
 *				unify_init_page				*
 *				---------------				*
 *									*
 ************************************************************************/

void unify_init_page ()
{
   int i;

   unify_op_cnt = 0;
   for (i = 0; i < rpage_entries; i++)
      unify_page[i] = 0;
}

/************************************************************************
 *									*
 *				unify_add_op				*
 *				------------				*
 *									*
 ************************************************************************/

void unify_add_op (tip, op)
TIP	*tip;
OPCODE2 *op;
{
   int	    latency;
   int	    dest;
   unsigned offset;
   UNIFY    *old_unify;

   if (!unify_ok_to_add (op, &latency, &dest)) return;

   offset    = (op->op.orig_addr & rpagesize_mask) >> 2;
   old_unify = unify_page[offset];

   assert (unify_op_cnt < MAX_UNIFY_OPS_PER_PAGE);
   unify_page[offset]           = &unify_mem[unify_op_cnt++];
   unify_page[offset]->next     = old_unify;

   unify_page[offset]->tip      = tip;
   unify_page[offset]->op       = op;
   unify_page[offset]->set_time = tip->vliw->time;
   unify_page[offset]->dest     = dest;
}

/************************************************************************
 *									*
 *				unify_ok_to_add_op			*
 *				------------------			*
 *									*
 ************************************************************************/

static int unify_ok_to_add (op, p_latency, p_dest)
OPCODE2 *op;
int     *p_latency;
int     *p_dest;
{
   int    i;
   int    reg;
   int    ok_reg;
   int	  ok_lat;
   int    ok_cnt    = 0;
   int    ca_ov_cnt = 0;
   int    bad_cnt   = 0;
   int    num_wr;
   RESULT *wr;

   wr     = op->op.wr;
   num_wr = op->op.num_wr;
   for (i = 0; i < num_wr; i++) {
      reg = wr[i].reg;

      if      (reg >=  R0 + NUM_PPC_GPRS_L && reg <  R0 + num_gprs) {
	 ok_reg = reg;
	 ok_lat = wr[i].latency;
	 ok_cnt++;
      }
      else if (reg >= FP0 + NUM_PPC_FPRS   && reg < FP0 + num_fprs) {
	 ok_reg = reg;
	 ok_lat = wr[i].latency;
	 ok_cnt++;
      }
      else if (reg >= CR0 + NUM_PPC_CCRS   && reg < CR0 + num_ccbits) {
	 ok_reg = reg;
	 ok_lat = wr[i].latency;
	 ok_cnt++;
      }
      else if (reg == CA) ca_ov_cnt++;
      else if (reg == OV) ca_ov_cnt++;
      else return FALSE;
   }

   if	   (ok_cnt == 0) return FALSE;
   else if (ok_cnt == 1) {
      *p_latency = ok_lat;
      *p_dest    = ok_reg;
      return TRUE;
   }
   else if (ok_cnt == 4) {
      /* Make sure we have a register field */
      assert ((wr[0].reg & 3) == 0);
      assert  (wr[1].reg == wr[0].reg + 1);
      assert  (wr[2].reg == wr[0].reg + 2);
      assert  (wr[3].reg == wr[0].reg + 3);

      *p_latency = ok_lat;
      *p_dest    = wr[0].reg;	       /* Want start of field */
      return TRUE;
   }
   else assert (1 == 0);
}

/************************************************************************
 *									*
 *				unify_find				*
 *				----------				*
 *									*
 * Find the first operation in the list which produces the same result	*
 * as this one.								*
 *									*
 * Seven conditions must be met in order to use the result, PREV, of	*
 * this same operation, CURR, encountered on a different path:		*
 *									*
 *    o PREV and CURR come from the same PowerPC address,		*
 *									*
 *    o PREV and CURR are same ins (Some PPC ins -> Mult VLIW ops)	*
 *									*
 *    o PREV must occur at or after the earliest time for CURR.		*
 *      (This prevents LOAD-VERIFY's from moving too early.)		*
 *									*
 *    o PREV and CURR are in the same group,				*
 *									*
 *    o PREV is a reachable ancestor of CURR in the tree of tree VLIW's,*
 *									*
 *    o The inputs for PREV must be mapped the same as those for CURR	*
 *      in the VLIW in which PREV occurs				*
 *									*
 *    o The non-arch reg set by PREV must not be killed before CURR,	*
 *      (i.e. set_time[PREV] >= avail[CURR_dest])			*
 *									*
 ************************************************************************/

UNIFY *unify_find (tip, op, earliest)
TIP	*tip;
OPCODE2 *op;
int     earliest;
{
   int	    group  = tip->vliw->group;
   TREE_POS *tp    = &tip->tp;
   unsigned offset = (op->op.orig_addr & rpagesize_mask) >> 2;
   UNIFY    *unify;

   for (unify = unify_page[offset]; unify; unify = unify->next) {
      if (unify->op->op.orig_addr != op->op.orig_addr) continue;
      if (unify->op->op.ins	  != op->op.ins)       continue;
      if (unify->tip->vliw->time  <  earliest)	       continue;
      if (unify->tip->vliw->group != group)	       continue;
      if (!tree_pos_is_ancestor (&unify->tip->tp, tp)) continue;
      if (!unify_inputs_same (unify, op, tip))	       continue;
      if (unify->set_time < tip->avail[unify->dest])   continue;
      if (unify_dest_killed (tip, unify->dest))	       continue;

      return unify;
   }

   return 0;
}

/************************************************************************
 *									*
 *				unify_inputs_same			*
 *				-----------------			*
 *									*
 * Returns TRUE if inputs to "curr_op" and "prev_op" from "unify"	*
 *	   are mapped to the same registers, FALSE otherwise.		*
 *									*
 ************************************************************************/

static int unify_inputs_same (unify, curr_op, curr_tip)
UNIFY   *unify;
OPCODE2 *curr_op;
TIP	*curr_tip;
{
   int		  i;
   int		  reg;
   int		  set_time;
   OPCODE2	  *prev_op = unify->op;
   int	          num_rd   = curr_op->op.num_rd;
   unsigned short *curr_rd = curr_op->op.rd;
   unsigned short *prev_rd = prev_op->op.rd;

   assert (num_rd == prev_op->op.num_rd);

   set_time = unify->set_time;
   for (i = 0; i < num_rd; i++) {
      reg = curr_rd[i];

      if      (reg >=  R0  &&  reg <  R0 + NUM_PPC_GPRS_C)
         reg = get_mapping (curr_tip, reg, set_time);

      else if (reg >= FP0  &&  reg < FP0 + NUM_PPC_FPRS)
         reg = get_mapping (curr_tip, reg, set_time);

      else if (reg >= CR0  &&  reg < CR0 + NUM_PPC_CCRS)
         reg = get_mapping (curr_tip, reg, set_time);

      else if (reg == CA)
         reg = get_mapping (curr_tip, reg, set_time);

      if (reg != prev_rd[i]) return FALSE;
   }

   return TRUE;
}

/************************************************************************
 *									*
 *				unify_dest_killed			*
 *				-----------------			*
 *									*
 * The "dest" (a non PowerPC architected register) should not be used	*
 * between here and when we list it as available.  This is normally the *
 * case, but sometimes "dest" is reused on another path, unbeknownst to	*
 * us, so we have to check.  (That is the other path reuses "dest"	*
 * prior to the point where its control flow splits off from our path.	*
 * Our "avail" array does not have information on such moves.)		*
 *									*
 ************************************************************************/

static int unify_dest_killed (tip, dest)
TIP *tip;
int dest;
{
   int	    avail_time = tip->avail[dest];
   int	    offset;
   unsigned mask;

   if	   (dest >=  R0 + NUM_PPC_GPRS_L  &&  dest <  R0 + num_gprs) {
      offset =       (dest - R0) >> 5;
      mask   = 1 << ((dest - R0) & 0x1F);
      while (tip) {
	 if      (tip->vliw->time < avail_time)    return FALSE;
	 else if (!(tip->gpr_vliw[offset] & mask)) return TRUE;
         else tip = tip->prev;
      }
   }
   else if (dest >= FP0 + NUM_PPC_FPRS    &&  dest < FP0 + num_fprs) {
      offset =       (dest - FP0) >> 5;
      mask   = 1 << ((dest - FP0) & 0x1F);
      while (tip) {
	 if      (tip->vliw->time < avail_time)    return FALSE;
	 else if (!(tip->fpr_vliw[offset] & mask)) return TRUE;
         else tip = tip->prev;
      }
   }
   else if (dest >= CR0 + NUM_PPC_CCRS    &&  dest < CR0 + num_ccbits) {
      offset =       (dest - CR0) >> 5;
      mask   = 1 << ((dest - CR0) & 0x1F);
      while (tip) {
	 if      (tip->vliw->time < avail_time)    return FALSE;
	 else if (!(tip->ccr_vliw[offset] & mask)) return TRUE;
         else tip = tip->prev;
      }
   }
   else assert (1 == 0);

   return FALSE;
}
