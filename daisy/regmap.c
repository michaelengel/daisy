/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "resrc_list.h"

#define MAX_NUM_MAPPINGS		65536

static REG_RENAME mapping_mem[MAX_NUM_MAPPINGS];
static int	  num_mappings;

REG_RENAME *set_mapping (TIP *, int, int, int, int);

/************************************************************************
 *									*
 *				init_page_regmap			*
 *				----------------			*
 *									*
 ************************************************************************/

init_page_regmap ()
{
   num_mappings = 0;
}

/************************************************************************
 *									*
 *				set_mapping				*
 *				-----------				*
 *									*
 * Set the mapping resulting from a COPY or commit operation and return	*
 * it.									*
 *									*
 ************************************************************************/

REG_RENAME *set_mapping (tip, src, dest, avail_time, orig_defn)
TIP *tip;
int src;			/* COPY dest <-- src			     */
int dest;
int avail_time;			/* Time at which result of copy/commit avail */
int orig_defn;			/* TRUE if 1st element in definition chain   */
{
   int	      num_ppc_regs;
   int	      basereg;
   REG_RENAME *mapping;
   REG_RENAME **rename;

   if	   (dest >= R0   &&  dest < R0  + NUM_PPC_GPRS_C) {
      rename  = tip->gpr_rename;
      basereg = R0;
      num_ppc_regs = NUM_PPC_GPRS;
   }
   else if (dest >= FP0  &&  dest < FP0 + NUM_PPC_FPRS) {
      rename  = tip->fpr_rename;
      basereg = FP0;
      num_ppc_regs = NUM_PPC_FPRS;
   }
   else if (dest >= CR0  &&  dest < CR0 + NUM_PPC_CCRS) {
      rename   = tip->ccr_rename;
      basereg  = CR0;
      num_ppc_regs = NUM_PPC_CCRS;
   }
   else assert (1 == 0);

   assert (num_mappings < MAX_NUM_MAPPINGS);
   mapping = &mapping_mem[num_mappings++];

   mapping->time	  = avail_time;
   mapping->vliw_reg	  = src;

   if (orig_defn) mapping->prev = 0;
   else		  mapping->prev = rename[src - basereg];

   rename[dest - basereg] = mapping;

   return mapping;
}

/************************************************************************
 *									*
 *				clr_mapping				*
 *				-----------				*
 *									*
 * Clear the mapping for a renameable register (as when defining the	*
 * register via a non-COPY in the last VLIW on the path.)		*
 *									*
 * CURRENTLY UNUSED.							*
 *									*
 ************************************************************************/

clr_mapping (tip, reg)
TIP *tip;
int reg;
{
   int	      basereg;
   REG_RENAME **rename;

   if	   (reg >= R0   &&  reg < R0  + NUM_PPC_GPRS_C) {
      rename  = tip->gpr_rename;
      basereg = R0;
   }
   else if (reg >= FP0  &&  reg < FP0 + NUM_PPC_FPRS) {
      rename  = tip->fpr_rename;
      basereg = FP0;
   }
   else if (reg >= CR0  &&  reg < CR0 + NUM_PPC_CCRS) {
      rename  = tip->ccr_rename;
      basereg = CR0;
   }
   else assert (1 == 0);

   rename[reg - basereg] = 0;

}

/************************************************************************
 *									*
 *				get_mapping				*
 *				-----------				*
 *									*
 * Returns:  The register to which "reg" maps at time "time" on		*
 *	     the path terminated by "tip".				*
 *									*
 ************************************************************************/

int get_mapping (tip, reg, time)
TIP *tip;
int reg;
int time;
{
   int	      map_time;
   int	      map = reg;
   REG_RENAME *mapping;

   if	   (reg >= R0   &&  reg < R0  + NUM_PPC_GPRS_C) 
      mapping = tip->gpr_rename[reg - R0];
   else if (reg >= FP0  &&  reg < FP0 + NUM_PPC_FPRS)
      mapping = tip->fpr_rename[reg - FP0];
   else if (reg >= CR0  &&  reg < CR0 + NUM_PPC_CCRS)
      mapping = tip->ccr_rename[reg - CR0];
   else if (reg == CA)
      mapping = &tip->ca_rename;
   else assert (1 == 0);

   /* If there is no mapping, the register maps to itself */
   if (!mapping) assert (tip->avail[reg] == 0);

   while (mapping) {
      map_time = mapping->time;
      if (time >= map_time) {
	 assert (tip->avail[reg] <= map_time);
	 reg = mapping->vliw_reg;
	 break;
      }

      mapping = mapping->prev;
   }

   return reg;
}
