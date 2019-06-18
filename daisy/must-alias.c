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

#define MAX_STORES_PER_PAGE		8192

static int    num_stores;
static MEMACC store_mem[MAX_STORES_PER_PAGE];

/************************************************************************
 *									*
 *				ma_init_page				*
 *				------------				*
 *									*
 ************************************************************************/

ma_init_page ()
{
   num_stores = 0;
}

/************************************************************************
 *									*
 *				ma_store				*
 *				--------				*
 *									*
 * Keep a record of the characteristics of this STORE needed to		*
 * determine if a later LOAD must-alias with it.			*
 *									*
 ************************************************************************/

ma_store (tip, st_op)
TIP	*tip;
OPCODE2 *st_op;
{
   int	  rz;
   int	  rs;
   int	  frs;
   int	  offset;
   int	  index_reg;
   int	  indexed;
   int	  hashval;
   MEMACC *curr_store;

   if (!is_store_op (st_op)) return;

   hashval = ma_get_hashval (st_op, &rz, &index_reg, &offset, &indexed);

   assert (num_stores < MAX_STORES_PER_PAGE);
   curr_store = &store_mem[num_stores++];

   curr_store->orig_defn = get_orig_defn (tip, rz);
   curr_store->indexed   = indexed;
   curr_store->size      = get_store_size (st_op);
   curr_store->orig_reg  = rz;
   curr_store->orig_reg2 = index_reg;
   curr_store->time	 = tip->vliw->time;
   curr_store->store_num = ++tip->last_store;
   assert (curr_store->size > 0);

   if (indexed) curr_store->_2.defn2  = get_orig_defn (tip, index_reg);
   else		curr_store->_2.offset = offset;

   if (is_store_flt (st_op)) {
      curr_store->flt = TRUE;
      frs = st_op->op.renameable[FRS & (~OPERAND_BIT)];
      curr_store->src_defn = tip->fpr_rename[frs - FP0];
      curr_store->src_reg  = frs;
   }
   else {
      curr_store->flt = FALSE;
      rs = st_op->op.renameable[RS & (~OPERAND_BIT)];
      curr_store->src_defn = tip->gpr_rename[rs - R0];
      curr_store->src_reg  = rs;
   }

   curr_store->next     = tip->stores[hashval];
   tip->stores[hashval] = curr_store;
}

/************************************************************************
 *									*
 *				ma_get_alias				*
 *				------------				*
 *									*
 * If "ld_op" does not have a must-alias with any STORE on the current	*
 * path, return (-1).  Otherwise return the rename entry for the most	*
 * recent STORE on this path which must alias with "ld_op".  For	*
 * example, if  "st  r3,96(r1)" aliases to the address in "ld_op", then	*
 * the rename entry for "r3" is returned.				*
 *									*
 * NOTE:  The rename entry may be NULL, for example if r3 was not	*
 *	  defined on the current path prior to the STORE.  Hence if	*
 *	  NULL is returned, the caller should assume the identity map	*
 *	  for the value returned at "*p_src_reg".			*
 *									*
 * NOTE:  There may be STORES after the must-alias STORE which alias	*
 *	  with "ld_op".  A LOAD-VERIFY must always be used to ensure	*
 *	  that this is not the case.					*
 *									*
 ************************************************************************/

REG_RENAME *ma_get_alias (tip, ld_op, p_src_reg, p_store_time, p_store_num,
			   p_store_size)
TIP	*tip;
OPCODE2 *ld_op;
int	*p_src_reg;			/* Output */
int	*p_store_time;			/* Output */
int	*p_store_num;			/* Output */
int	*p_store_size;			/* Output */
{
   int		hashval;
   int		rz;
   int		ld_index_reg;
   int		ld_offset;
   int		ld_indexed;
   int		ld_size;
   int		ld_flt;
   REG_RENAME	*ld_orig_defn;
   REG_RENAME	*ld_defn2;
   MEMACC	*curr_store;

   hashval=ma_get_hashval (ld_op, &rz, &ld_index_reg, &ld_offset, &ld_indexed);

   ld_size = get_load_size (ld_op);
   ld_flt  = is_load_flt (ld_op);
   ld_orig_defn = get_orig_defn (tip, rz);
   if (ld_indexed) ld_defn2 = get_orig_defn (tip, ld_index_reg);

   for (curr_store = tip->stores[hashval];
	curr_store;
	curr_store = curr_store->next) {

      if (curr_store->size	!= ld_size)	      continue;
      if (curr_store->flt	!= ld_flt)	      continue;
      if (curr_store->orig_defn != ld_orig_defn)      continue;

      if (ld_orig_defn) {
	 if (curr_store->orig_defn != ld_orig_defn)   continue;
      }
      else if (curr_store->orig_defn  || 
	       curr_store->orig_reg != rz)	      continue;

      if (curr_store->indexed	!= ld_indexed)	      continue;
      if (ld_indexed) {

	 if (ld_defn2) {
	    if (curr_store->_2.defn2 != ld_defn2)     continue;
	 }
	 else if (curr_store->_2.defn2  || 
	       curr_store->orig_reg2 != ld_index_reg) continue;
      }
      else if (curr_store->_2.offset != ld_offset)    continue;

      *p_src_reg    = curr_store->src_reg;
      *p_store_time = curr_store->time;
      *p_store_num  = curr_store->store_num;
      *p_store_size = curr_store->size;
      return curr_store->src_defn;
   }

   return -1;
}

/************************************************************************
 *									*
 *				ma_get_hashval				*
 *				--------------				*
 *									*
 * Return the hashval for the memory location referenced by a LOAD or	*
 * STORE op.  Since several fields are computed which are later used	*
 * in other routines, return those values as well.			*
 *									*
 ************************************************************************/

int ma_get_hashval (op, p_rz, p_index_reg, p_offset, p_indexed)
OPCODE2 *op;
int	*p_rz;			/* Output */
int	*p_index_reg;		/* Output */
int	*p_offset;		/* Output */
int	*p_indexed;		/* Output */
{
   int	    hashval;
   unsigned rz;
   unsigned index_reg;
   unsigned offset;

   *p_rz = rz = op->op.renameable[RZ & (~OPERAND_BIT)];

   if (op->b->primary == 31) {
      index_reg    = op->op.renameable[RB & (~OPERAND_BIT)];
      hashval      = (rz + index_reg) / 1025;
      *p_indexed   = TRUE;
      *p_index_reg = index_reg;
   }
   else {
      *p_offset  = chk_immed_val (op->op.ins & 0xFFFF, D);
      offset     = (*p_offset + 32832);
      hashval    = (rz + offset) / 1025;
      *p_indexed = FALSE;
   }

   assert (NUM_MA_HASHVALS == 64);
   assert (hashval >= 0  &&  hashval <= NUM_MA_HASHVALS);

   return hashval;
}

/************************************************************************
 *									*
 *				get_orig_defn				*
 *				-------------				*
 *									*
 ************************************************************************/

get_orig_defn (tip, reg)
TIP *tip;
int reg;
{
   REG_RENAME *mapping;
   REG_RENAME *oldest_mapping;

   if	   (reg >= R0   &&  reg < R0  + NUM_PPC_GPRS_C) 
      mapping = tip->gpr_rename[reg - R0];
   else if (reg >= FP0  &&  reg < FP0 + NUM_PPC_FPRS) 
      mapping = tip->fpr_rename[reg - FP0];

   oldest_mapping = mapping;
   for (; mapping; mapping = mapping->prev)
      oldest_mapping = mapping;

   return oldest_mapping;
}
