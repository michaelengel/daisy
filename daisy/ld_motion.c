/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "dis.h"
#include "dis_tbl.h"
#include "resrc_list.h"
#include "unify.h"

extern int        num_gprs;
extern int        num_fprs;

extern int	  do_load_motion;	   /* Boolean */
extern int	  make_lhz_lbz_safe;	   /* See note in sched_ld_before_st */
extern int	  use_ld_latency_for_ver;
extern int	  store_latency;

extern int	  doing_load_mult;	/* If set, not do load motion */
extern int	  recompiling;		/* If set, had LOAD-VERIFY failure */
extern unsigned	  *curr_ins_addr;
extern UNIFY      *insert_unify;        /* Non-0 if unify instead of insert */

extern int        cluster_resrc[];     /* Bool: Reesource assoc with	     */
				       /*       clusters/whole machine	     */
static OPCODE2	  *copy_op;

TIP *must_alias_load (TIP *, TIP *, REG_RENAME *, int, int, int, int, int,
		      int, int, int, OPCODE2 *, OPCODE2 *, int,
		      int *, TIP **);

/************************************************************************
 *									*
 *				sched_ld_before_st			*
 *				------------------			*
 *									*
 * If we have a simple load, i.e. to only one register, speculatively	*
 * move it past any stores:						*
 *									*
 *	ORIGINAL			TRANSLATED			*
 *					l    r3',0(r5)			*
 *	st ?				st   ?				*
 *	l  r3,0(r5)			l.v  r3',0(r5),ppc_addr		*
 *					copy r3,r3'			*
 *									*
 * The "l.v" instruction reloads the value at 0(r5) and compares it	*
 * to the value in r3'.  If it is the same, execution continues with	*
 * the "copy" commit operation for r3.  Otherwise the usual hash table	*
 * offpage jump is done to the translation of "ppc_addr" (or to the	*
 * DAISY translator in the more common case, when ppc_addr is not	*
 * already an entry point.)						*
 *									*
 * We first insert a LOAD instruction, which will turn into a		*
 * LOAD-VERIFY (assuming no aliasing).  This LOAD/LOAD-VERIFY includes	*
 * memory dependences in setting its earliest time.  Next we insert	*
 * another LOAD with an earliest time that ignores memory dependences.	*
 * Generally this will result in the second LOAD being scheduled well	*
 * ahead of the LOAD-VERIFY, with a COPY COMMIT operation in the same	*
 * VLIW as the LOAD-VERIFY.  If the second LOAD cannot be placed early	*
 * enough, then it and any copy are deleted.				*
 *									*
 * We have a tradeoff in making 8 and 16-bit LOADS "safe".  We can do	*
 * so by doing an RLINM of the STORE source register to the LOAD	*
 * destination register.  However, in that case copy propagation	*
 * information begins after the RLINM.  In other words, subsequent	*
 * operations that depend on the LOAD cannot be moved earlier than the	*
 * RLINM.  If 8 and 16-bit LOADS are not "safe", then a 32-bit COPY can	*
 * be performed instead of an RLINM and the copy propagation from the	*
 * STORE source register is extended.  Hence operations that depend on	*
 * the LOAD may be able to be moved earlier than the RLINM.		*
 *									*
 * Returns:  0 if not moved past stores and caller should   schedule op	*
 *	     1 if     moved past stores and caller need not schedule op	*
 *	     ALSO update ptip to indicate current tip			*
 *									*
 ************************************************************************/

int sched_ld_before_st (ptip, op, ins_ptr, do_must_alias, p_ins_tip,
			p_ver_tip, p_ver_op)
TIP	    **ptip;
OPCODE2     *op;
unsigned    *ins_ptr;
int	    do_must_alias;			/* Boolean flag */
TIP	    **p_ins_tip;			/* Outputs ...  */
TIP	    **p_ver_tip;
OPERAND_OUT **p_ver_op;
{
   int		  i;
   int		  time;
   int		  is_flt;
   int		  st_time;
   int		  store_num;
   int		  verif_time;
   int		  earliest_mem;
   int		  earliest_nomem;
   int		  orig_dest;
   int		  renamed_dest;
   int		  stored_reg;
   int		  store_size;
   int	          clust_num;
   int		  do_ld_verif;
   int		  copy_prop_safe;
   int		  safe_must_alias; /* LD-ST must alias,no other ST may alias */
   int		  min_ver_commit_time;
   int	          resrc_needed[NUM_VLIW_RESRC_TYPES+1];
   TIP		  *tip = *ptip;
   TIP		  *ins_tip;
   TIP		  *verify_tip;
   VLIW		  *new_vliw;
   OPERAND_OUT	  *verop;
   BASE_OP	  *save_op;
   OPCODE2	  *v_unify_op;
   REG_RENAME     *rt_save_rename;
   REG_RENAME     *st_alias;
   unsigned short rt_save_avail;
   unsigned short rt_save_clust;
   unsigned short rt;

   /* Assume no LOAD-VERIFY needed initially */
   op->op.verif = FALSE;

   if (!do_load_motion) return 0;

   if (doing_load_mult) return 0;

   if (!is_simple_load_op (op)) return 0;

   *p_ver_tip = 0;	   /* Assume no LOAD-VERIFY until sure we have one */
   earliest_nomem = get_earliest_time (tip, op, TRUE);
   earliest_mem   = get_earliest_time (tip, op, FALSE);

   /* If reg availability, not mem is limiting factor, just sched as normal */
   if (earliest_mem == earliest_nomem) return 0;

   /* If this load has caused a LOAD-VERIFY failure,  just sched as normal */
   if (recompiling)
      if (had_loadver_fail (ins_ptr - 1))
	 return 0;

   orig_dest = get_gpr_fpr_dest (op);

   /* The first insert_op may destroy the rename and avail info if the real
      PPC destination is the same as one of the sources.  However the 2nd
      insert_op needs the rename and avail info too, so save and restore it.
   */
   if (orig_dest >=  R0  &&  orig_dest <  R0 + num_gprs) {
      rt = op->op.renameable[RT & (~OPERAND_BIT)];
      rt_save_rename = tip->gpr_rename[rt-R0];
      is_flt = FALSE;
   }
   else {
      rt = op->op.renameable[FRT & (~OPERAND_BIT)];
      rt_save_rename = tip->fpr_rename[rt-FP0];
      is_flt = TRUE;
   }
   rt_save_avail = tip->avail[rt];
   rt_save_clust = tip->cluster[rt];

   /* "do_must_alias is currently FALSE for load-updates, since we don't
      know the offset from the base register until we know the VLIW
      in which the operation is scheduled.  To determine aliasing
      in this case requires more analysis than we perform.
   */
   copy_prop_safe = TRUE;
   if (!do_must_alias) {
      st_alias	      = -1;
      safe_must_alias = FALSE;
   }
   else {
      st_alias = ma_get_alias (tip, op, &stored_reg, &st_time, &store_num,
			       &store_size);
      safe_must_alias = FALSE;
      if (st_alias != (-1)) {
	 if (store_num == tip->last_store) {
	    if (is_flt) {
	         if    (store_size == 8) safe_must_alias = TRUE;
	    }
	    else if    (store_size == 4) safe_must_alias = TRUE;
	    else if (make_lhz_lbz_safe) {
	       if      (store_size == 1) {
		     safe_must_alias = TRUE;
		     copy_prop_safe  = FALSE;
	       }
	       else if (store_size == 2) {
	          if (op->b->op_num == OP_LHZ  ||  op->b->op_num == OP_LHZX) {
		     safe_must_alias = TRUE;
		     copy_prop_safe  = FALSE;
		  }
	       }
	    }
	 }
      }
   }

   /* Insert the LOAD-VERIFY in the last VLIW.  (We could do it
      in any VLIW after the latest STORE, but that's more bookkeeping
      so for now, we'll do it this way.)
   */
   if (earliest_mem > tip->vliw->time) verif_time = earliest_mem;
   else				       verif_time = tip->vliw->time;

   if (!safe_must_alias) {
      if (!sched_ppc_rz0_op (&tip, op, verif_time, &verify_tip))
	 tip = insert_op (op, tip, verif_time, &verify_tip);

      if (insert_unify) v_unify_op = insert_unify->op;
      else		v_unify_op = 0;
   }

   /* Restore fields that may have been corrupted by insert_op */
   tip->avail[rt]   = rt_save_avail;
   tip->cluster[rt] = rt_save_clust;
   if (orig_dest >=  R0  &&  orig_dest <  R0 + num_gprs)
        tip->gpr_rename[rt-R0]  = rt_save_rename;
   else tip->fpr_rename[rt-FP0] = rt_save_rename;

   /* IF this LOAD is definitely aliased with an earlier STORE, 
        THEN Replace LOAD with a COPY of the register being STORE'd.
        ELSE Insert the speculative load in the earliest possible VLIW
   */
   if (st_alias != (-1)) {
     tip = must_alias_load (tip, verify_tip, st_alias, copy_prop_safe,
			    safe_must_alias, st_time, store_size,
			    op->op.renameable[RZ & (~OPERAND_BIT)]-R0,
			    stored_reg, rt, earliest_nomem, op,
			    v_unify_op, is_flt,
			    &do_ld_verif, &ins_tip);

     if (!do_ld_verif) {
        if (!sched_ppc_rz0_op (&tip, op, earliest_mem, &ins_tip))
	   tip = insert_op (op, tip, earliest_mem, &ins_tip);
     }
   }
   else if ((op->op.renameable[RZ & (~OPERAND_BIT)] == R1)  &&
	    (tip->stm_r1_time >= earliest_nomem)) {
      /* Heuristic: Don't move stack refs beyond store-mult at start of proc */
      tip = insert_op (op, tip, earliest_mem, &ins_tip);
      do_ld_verif = FALSE;
   }
   else {
      /* Make sure the commit allows time for the LOAD-VERIFY result
	 to be received.  This may mean we have to insert additional
         VLIW instructions to wait for the result.
      */
      if (use_ld_latency_for_ver) {
         min_ver_commit_time = verify_tip->vliw->time +
			       get_result_latency (op, orig_dest);
	 for (time = tip->vliw->time+1; time <= min_ver_commit_time; time++) {
	    new_vliw = append_new_vliw (tip, time);
	    tip = new_vliw->entry;
	 }
      }

      if (!sched_ppc_rz0_op (&tip, op, earliest_nomem, &ins_tip))
         tip = insert_op (op, tip, earliest_nomem, &ins_tip);

      do_ld_verif = TRUE;
   }

   /* Even though we could move the LOAD above STORES, did we? */
   if (ins_tip->vliw->time >= earliest_mem  &&  st_alias == (-1)) {
      /* NO:  Delete the LOAD-VERIFY */
      if (verify_tip == tip  ||  verify_tip == ins_tip) {

         save_op = verify_tip->op;       /* 1st is ld or commit of the load */

	 if (save_op->next->op != v_unify_op) {
	    get_needed_resrcs (save_op->next->op, resrc_needed);

	    clust_num = save_op->next->cluster;
	    decr_resrc_usage (verify_tip, verify_tip->vliw->resrc_cnt,
			      resrc_needed, clust_num);

	    verify_tip->num_ops--;
	    assert (save_op->next);               /* 2nd is LOAD-VERIFY */
	    save_op->next = save_op->next->next;  /* Delete LOAD-VERIFY */
	 }
      }
      else {
	 if (verify_tip->op->op != v_unify_op) {

	    get_needed_resrcs (verify_tip->op->op, resrc_needed);

	    clust_num = verify_tip->op->cluster;
	    decr_resrc_usage (verify_tip, verify_tip->vliw->resrc_cnt,
			      resrc_needed, clust_num);

	    verify_tip->num_ops--;
	    verify_tip->op = verify_tip->op->next;/* LOAD-VERIFY 1st, delete */
	 }
      }
   }

   /* YES:  Patch the LOAD-VERIFY instruction to use renamed LOAD-dest */
   else if (do_ld_verif  &&  !safe_must_alias) {

      if (insert_unify) renamed_dest = insert_unify->dest;
      else		renamed_dest = get_gpr_fpr_dest (ins_tip->op->op);

      if (verify_tip != tip) verop = &verify_tip->op->op->op;
      else		     verop = &verify_tip->op->next->op->op;
      verop->verif = TRUE;
      for (i = 0; i < verop->num_wr; i++) {
	 if (verop->wr[i].reg == orig_dest) {
	    verop->wr[i].reg = renamed_dest;

	    if      (orig_dest >= R0   &&  orig_dest < R0  + num_gprs)
	       verop->renameable[ RT & (~OPERAND_BIT)] = renamed_dest;
	    else if (orig_dest >= FP0  &&  orig_dest < FP0 + num_fprs)
	       verop->renameable[FRT & (~OPERAND_BIT)] = renamed_dest;
	    else assert (1 == 0);

	    break;
	 }
      }
      *p_ver_tip = verify_tip;
      *p_ver_op  = verop;
   }

   *ptip      = tip;
   *p_ins_tip = ins_tip;
   return 1;
}

/************************************************************************
 *									*
 *				must_alias_load				*
 *				---------------				*
 *									*
 ************************************************************************/

static TIP *must_alias_load (tip, ver_tip, st_alias, copy_prop_safe, 
			     safe_must_alias, st_time, st_size, addr_reg, 
			     st_srcreg, load_dest, earliest_nomem, op,
			     v_unify_op, is_flt,
			     do_ld_verif, p_ins_tip)
TIP	    *tip;
TIP	    *ver_tip;
REG_RENAME  *st_alias;
int	    copy_prop_safe;
int	    safe_must_alias;
int	    st_time;
int	    st_size;
int	    addr_reg;
int	    st_srcreg;
int	    load_dest;
int	    earliest_nomem;
OPCODE2	    *op;
OPCODE2	    *v_unify_op;
int	    is_flt;			/* Boolean */
int	    *do_ld_verif;		/* Output  */
TIP	    **p_ins_tip;		/* Output  */
{
   int	       earliest;
   int	       latest;
   int	       stored_reg_avail;
   int	       stored_reg_clust;
   int	       load_dest_avail;
   int	       load_dest_clust;
   int	       clust_num;
   int	       avail_time;
   int	       st_src_avail;		/* Boolean */
   int	       resrc_needed[NUM_VLIW_RESRC_TYPES+1];
   TIP	       *ins_tip;
   OPCODE2     *ins_op;
   OPCODE2     *i_unify_op;
   REG_RENAME  *stored_reg_rename;
   REG_RENAME  *load_dest_rename;
   REG_RENAME  *load_map;
   REG_RENAME  *last_load_map;
   REG_RENAME  *map;
   REG_RENAME  *save_map;
   REG_RENAME  ident_map;

#ifdef DEBUGGING
   if (st_alias == 0)
        printf ("[%08x]  Found ST-LD must-alias:  ST Srcreg = %d,  Basereg = %d\n",
		curr_ins_addr, st_srcreg, addr_reg);
   else printf ("[%08x]  Found ST-LD must-alias:  ST Srcreg = %d,  Basereg = %d\n",
		curr_ins_addr, st_alias->vliw_reg, addr_reg);
#endif

   stored_reg_avail = tip->avail[st_srcreg];
   stored_reg_clust = tip->cluster[st_srcreg];
   if (st_alias == 0) earliest = 0;
   else {
      st_srcreg = st_alias->vliw_reg;
      map = st_alias;
      for (earliest = map->time; map; map = map->prev) earliest = map->time;
   }

   /* Make sure "insert_op" does not get confused and think that the
      STORE src is not available.  Save the current values if we
      need to change them to fool "insert_op".
   */
   if (is_flt) {
     if      (tip->fpr_rename[st_srcreg-FP0]       == 0)       st_src_avail=TRUE;
     else if (tip->fpr_rename[st_srcreg-FP0]->time <= st_time) st_src_avail=TRUE;
     else						       st_src_avail=FALSE;
   }
   else {
     if      (tip->gpr_rename[st_srcreg-R0]       == 0)	     st_src_avail=TRUE;
     else if (tip->gpr_rename[st_srcreg-R0]->time <= st_time)st_src_avail=TRUE;
     else						     st_src_avail=FALSE;
   }

   if (st_src_avail) {
      if (safe_must_alias)
	   latest = tip->vliw->time;   /* COPY to ld dest reg anytime<=curr  */

/* If not a safe_must_alias, e.g. LHA or LFS, then the STORE	 */
/* source register cannot be copied directly in the current VLIW */
#ifdef WHATS_GOING_ON
      else if (tip->vliw->time == earliest)
           latest = tip->vliw->time;   /* ST in currtip=>srcreg avail direct */
#endif

      else latest = tip->vliw->time-1; /* COPY to non-arch reg anytime<curr  */

      if (is_flt) stored_reg_rename = tip->fpr_rename[st_srcreg-FP0];
      else	  stored_reg_rename = tip->gpr_rename[st_srcreg-R0];
   }
   else {
      latest = st_time;

      /* If the alias is not safe, e.g. LHA or LFS, then the STORE
	 source register cannot be copied directly in the current VLIW
      */
      if (!safe_must_alias  &&  latest >= tip->vliw->time)
	 latest = tip->vliw->time - 1;

      tip->avail[st_srcreg] = earliest;
      ident_map.time	    = earliest;
      ident_map.vliw_reg    = st_srcreg;
      ident_map.prev	    = 0;
      if (is_flt) {
	 stored_reg_rename = tip->fpr_rename[st_srcreg-FP0];
	 tip->fpr_rename[st_srcreg-FP0] = &ident_map;
      }
      else {
	 stored_reg_rename = tip->gpr_rename[st_srcreg-R0];
	 tip->gpr_rename[st_srcreg-R0] = &ident_map;
      }
   }

   /* In cases where the LOAD overwrites its address register, e.g. l r4,0(r4)
      we must be able to restore the old rename info for r4 if we are unable
      to insert the LOAD-VERIFY and COPY operations at the appropriate
      places.
   */
   load_dest_avail = tip->avail[load_dest];
   load_dest_clust = tip->cluster[load_dest];
   if (is_flt) load_dest_rename = tip->fpr_rename[load_dest-FP0];
   else	       load_dest_rename = tip->gpr_rename[load_dest-R0];

   setup_copy_op (load_dest, st_srcreg, is_flt, st_size, copy_prop_safe);
   tip = insert_op (copy_op, tip, earliest, &ins_tip);

   if (insert_unify) i_unify_op = insert_unify->op;
   else		     i_unify_op = 0;

/*
   assert (sched_copy_op (&tip, copy_op, copy_op->op.ins, earliest, &ins_tip));
*/
   *p_ins_tip = ins_tip;

   /* If we were unable to COPY the STORE src register to a
      non-arch register in time, delete COPY and do in order LOAD
    */
   if (ins_tip->vliw->time > latest) {

      if (insert_unify) i_unify_op = insert_unify->op;
      else		i_unify_op = 0;

      /* Delete the COPY and its COMMIT */
      get_needed_resrcs (tip->op->op, resrc_needed);
      clust_num = tip->op->cluster;
      decr_resrc_usage (tip, tip->vliw->resrc_cnt, resrc_needed, clust_num);

      tip->num_ops--;				/* COPY 1st, delete */
      tip->op = tip->op->next;

      if (ins_tip != tip) {			/* If eq, COPY, COMMIT same */

	 /* Don't delete if used by somebody else */
	 if (ins_tip->op != i_unify_op) {
	    get_needed_resrcs (ins_tip->op->op, resrc_needed);

	    clust_num = ins_tip->op->cluster;
	    decr_resrc_usage (ins_tip, ins_tip->vliw->resrc_cnt,
			      resrc_needed, clust_num);

	    ins_tip->num_ops--;
	    ins_tip->op = ins_tip->op->next;
	 }
      }

      /* Delete the LOAD-VERIFY */
      if (!safe_must_alias) {

	 /* Don't delete if used by somebody else */
	 if (ver_tip->op != v_unify_op) {
	    get_needed_resrcs (ver_tip->op->op, resrc_needed);

	    clust_num = ver_tip->op->cluster;
	    decr_resrc_usage (ver_tip, ver_tip->vliw->resrc_cnt,
			      resrc_needed, clust_num);

	    ver_tip->num_ops--;
	    ver_tip->op = ver_tip->op->next;
	 }
      }

      tip->avail[load_dest]   = load_dest_avail;
      tip->cluster[load_dest] = load_dest_clust;
      if (is_flt) tip->fpr_rename[load_dest-FP0] = load_dest_rename;
      else	  tip->gpr_rename[load_dest-R0]  = load_dest_rename;

      *do_ld_verif = FALSE;
   }
   else {
      /* Store should be at least 1 cycle ahead of LOAD-VERIFY */
      if (!safe_must_alias  &&  store_latency > 0) assert (ins_tip != tip);

      /* Fixup the STORE src according to the VLIW in which we are
	 getting it, i.e. account for copy propagation.
      */
      for (map = st_alias; map; map = map->prev)
	 if (map->time <= ins_tip->vliw->time) break;

      if (st_alias) assert (map);

      if (!map) {
	 /* If the source has no definition in this group, i.e. we are using
	    the value at entry to the group, make the source map to itself
	    with availability at time 0.
	 */
	 if (!st_alias) {
	    if (load_dest == st_srcreg) {
	       if (is_flt) save_map = tip->fpr_rename[load_dest - FP0];
	       else	   save_map = tip->gpr_rename[load_dest - R0];
	    }

	    map = set_mapping (tip, st_srcreg, st_srcreg, 0, TRUE);

	    if (load_dest == st_srcreg) {
	       if (is_flt) tip->fpr_rename[load_dest - FP0]  = save_map;
	       else	   tip->gpr_rename[load_dest - R0] = save_map;
	    }
	 }
      }
      else {

	 if (i_unify_op) ins_op = i_unify_op;
	 else		 ins_op = ins_tip->op->op;

	 if (is_flt) {
	    assert (ins_op->b->op_num == OP_FMR);
	    ins_op->op.renameable[FRB & (~OPERAND_BIT)]=map->vliw_reg;
	    ins_op->op.rd[0] = map->vliw_reg;
	 }
	 else if (ins_op->b->op_num == OP_OR) {
	    ins_op->op.renameable[RS & (~OPERAND_BIT)]=map->vliw_reg;
	    ins_op->op.renameable[RB & (~OPERAND_BIT)]=map->vliw_reg;
	    ins_op->op.rd[0] = map->vliw_reg;
	    ins_op->op.rd[1] = map->vliw_reg;
	 }
	 else if (ins_op->b->op_num == OP_RLINM) {
	    ins_op->op.renameable[RS & (~OPERAND_BIT)]=map->vliw_reg;
	    ins_op->op.rd[0] = map->vliw_reg;
	 }
	 else assert (1 == 0);
      }

      /* Keep copy propagation information up-to-date */
      if (is_flt) last_load_map = tip->fpr_rename[load_dest - FP0];
      else	  last_load_map = tip->gpr_rename[load_dest - R0];

      if (copy_prop_safe) {
	 for (load_map = last_load_map; load_map; 
	      load_map = load_map->prev) last_load_map = load_map;

	 /* Hook into the copy-propagation chain for STORE source reg */
	 last_load_map->prev = map;

	 /* Availability time for "load_dest" is when STORE srcreg avail */
	 for (; map; map = map->prev) avail_time = map->time;
	 tip->avail[load_dest] = avail_time;
      }
      *do_ld_verif = TRUE;
   }

   /* If we changed this info to fool "insert_op", change it back unless
      we overwrote it with the result of the LOAD or we were unable to
      COPY the STORE source register in time.
   */
   if ((!st_src_avail && st_srcreg != load_dest) || !(*do_ld_verif)) {
      tip->avail[st_srcreg]   = stored_reg_avail;
      tip->cluster[st_srcreg] = stored_reg_clust;
      if (is_flt) tip->fpr_rename[st_srcreg-FP0] = stored_reg_rename;
      else	  tip->gpr_rename[st_srcreg-R0]  = stored_reg_rename;
   }

   return tip;
}

/************************************************************************
 *									*
 *				setup_copy_op				*
 *				-------------				*
 *									*
 ************************************************************************/

static setup_copy_op (dest, src, is_flt, size, copy_prop_safe)
int dest;
int src;
int is_flt;
int size;
int copy_prop_safe;
{
   unsigned ins;
   OPCODE1  *opcode;

   if (is_flt) {
      ins = 0xFC000090;					/* FMR */
      opcode  = get_opcode (ins);
      copy_op = set_operands (opcode, ins, opcode->b.format);

      copy_op->op.renameable[FRT & (~OPERAND_BIT)] = dest;
      copy_op->op.renameable[FRB & (~OPERAND_BIT)] = src;
      copy_op->op.wr[0].reg = dest;
      copy_op->op.rd[0]     = src;
   }
   else if (copy_prop_safe) {
      ins = 0x7C000378;					/* OR  */
      opcode  = get_opcode (ins);
      copy_op = set_operands (opcode, ins, opcode->b.format);

      copy_op->op.renameable[RA & (~OPERAND_BIT)] = dest;
      copy_op->op.renameable[RS & (~OPERAND_BIT)] = src;
      copy_op->op.renameable[RB & (~OPERAND_BIT)] = src;
      copy_op->op.wr[0].reg = dest;
      copy_op->op.rd[0]     = src;
      copy_op->op.rd[1]     = src;
   }
   else {
      if      (size == 2) ins = 0x5400043E;		/* RLINM  */
      else if (size == 1) ins = 0x5400063E;		/* RLINM  */
      else assert (1 == 0);

      opcode  = get_opcode (ins);
      copy_op = set_operands (opcode, ins, opcode->b.format);

      copy_op->op.renameable[RA & (~OPERAND_BIT)] = dest;
      copy_op->op.renameable[RS & (~OPERAND_BIT)] = src;
      copy_op->op.wr[0].reg = dest;
      copy_op->op.rd[0]     = src;
   }
}

/************************************************************************
 *									*
 *				is_mem_operand				*
 *				--------------				*
 *									*
 ************************************************************************/

int is_mem_operand (operand)
int operand;
{
   switch (operand) {
      case MEM:
      case ICACHE:
      case DCACHE:
      case TLB:
      case PFT:      return TRUE;

      default:	     return FALSE;
   }
}

/************************************************************************
 *									*
 *				get_gpr_fpr_dest			*
 *				----------------			*
 *									*
 * Return the single GPR or FPR destination of an instruction.		*
 *									*
 ************************************************************************/

int get_gpr_fpr_dest (op)
OPCODE2 *op;
{
   int	  i;
   int	  reg;
   int	  rval;
   int	  cnt    = 0;
   int	  num_wr = op->op.num_wr;
   RESULT *wr    = op->op.wr;

   for (i = 0; i < num_wr; i++) {
      reg = wr[i].reg;
      if      (reg >=  R0  &&  reg <  R0 + num_gprs) { rval = reg;  cnt++; }
      else if (reg >= FP0  &&  reg < FP0 + num_fprs) { rval = reg;  cnt++; }
   }

   assert (cnt == 1);

   return rval;
}
