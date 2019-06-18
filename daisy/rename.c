/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#pragma alloca

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "vliw-macros.h"

int curr_rename_reg;		/* Used in brlink.c, rz0.c */
int curr_rename_reg_valid;

extern int      num_gprs;
extern int      num_fprs;
extern int      num_ccbits;

extern unsigned renameable[];

static int      max_regs;
static int      max_reg_words;

/************************************************************************
 *									*
 *				init_rename				*
 *				-----------				*
 *									*
 ************************************************************************/

init_rename ()
{
   if	   (num_fprs   > num_gprs) max_regs = num_fprs;
   else if (num_ccbits > num_gprs) max_regs = num_ccbits;
   else				   max_regs = num_gprs;

   max_reg_words = (max_regs + WORDBITS - 1) / WORDBITS;
}

/************************************************************************
 *									*
 *				set_reg_for_op				*
 *				--------------				*
 *									*
 * Returns TRUE  if the op has no renameable destination register, or	*
 *		 if it has and a rename register is available.		*
 * Returns FALSE otherwise						*
 *									*
 * This routine also puts the rename register in "curr_rename_reg"	*
 * and sets "curr_rename_reg_valid" if a rename register is needed	*
 * and is available.  This allows us to avoid calling this routine	*
 * twice, once to check if "op" will fit in the current tip, another	*
 * to actually place it there with the appropriate rename register.	*
 *									*
 * NOTE:  It is assumed that "op" has at most one renameable dest reg.	*
 *	  (If there is more than one, the op should be scheduled at	*
 *	  the end without renaming, and without calling this routine.)	*
 *									*
 ************************************************************************/

int set_reg_for_op (op, first_tip, last_tip)
OPCODE2 *op;
TIP	*first_tip;
TIP	*last_tip;
{

   if (num_gprs <= 64  &&  num_fprs <= 64  &&  num_ccbits <= 64)
        return set_reg_for_op_small (op, first_tip, last_tip);
   else return set_reg_for_op_big   (op, first_tip, last_tip);
}

/************************************************************************
 *									*
 *				set_reg_for_op_small			*
 *				--------------------			*
 *									*
 * Handle usual architecture where have 64 or fewer GPRs, FPRs, CCBITS.	*
 *									*
 ************************************************************************/

int set_reg_for_op_small (op, first_tip, last_tip)
OPCODE2 *op;
TIP	*first_tip;
TIP	*last_tip;
{
   int	    i;
   int	    reg;
   int	    rename_reg;
   int	    renameable_cnt;
   int	    cr_bit_op = is_cr_bit_op (op);
   unsigned reg_use_word;
   TIP	    *tip;

   assert (curr_rename_reg_valid == FALSE);

   curr_rename_reg = MAXINT;
   renameable_cnt = 0;

   /* Optimize most common cases:  64 GPRS and 64 FPR's */
   assert (num_gprs <= 64  &&  num_fprs <= 64  &&  num_ccbits <= 64);

   for (i = 0; i < op->op.num_wr; i++) {
      reg = op->op.wr[i].reg;

      if (is_bit_set (renameable, reg)) {

	 /* CA renaming happens automatically with GPR */
	 if (reg == CA) continue;

	 renameable_cnt++;

	 reg_use_word = 0xFFFFFFFF;
	 if      (is_gpr   (reg)) {
	    for (tip = last_tip; tip; tip = tip->prev) {
	       reg_use_word &= tip->gpr_vliw[1];
	       if (tip == first_tip) break;
	    }

	    if (reg_use_word != 0) {
	       rename_reg = 31 - get_1st_one (reg_use_word);
	       curr_rename_reg = rename_reg + R0 + 32;
	       curr_rename_reg_valid = TRUE;
	    }
	    else {
	       curr_rename_reg_valid = FALSE;
	       return FALSE;
	    }
	 }
	 else if (is_fpr   (reg)) {
	    for (tip = last_tip; tip; tip = tip->prev) {
	       reg_use_word &= tip->fpr_vliw[1];
	       if (tip == first_tip) break;
	    }

	    if (reg_use_word != 0) {
	       rename_reg = 31 - get_1st_one (reg_use_word);
	       curr_rename_reg = rename_reg + FP0 + 32;
	       curr_rename_reg_valid = TRUE;
	    }
	    else {
	       curr_rename_reg_valid = FALSE;
	       return FALSE;
	    }
	 }
	 else if (is_ccbit (reg)) {
	    for (tip = last_tip; tip; tip = tip->prev) {
	       reg_use_word &= tip->ccr_vliw[1];
	       if (tip == first_tip) break;
	    }

	    if (reg_use_word != 0) {

	       /* If this is a register field op, we must get 4 bits
	          starting on 4-bit boundary.
	       */
	       if (!cr_bit_op) {
		  rename_reg = get_1st_crfield (reg_use_word);
		  if (rename_reg >= 0) {
		     curr_rename_reg       = CRF8 + rename_reg;
		     curr_rename_reg_valid = TRUE;
		  }
		  else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }
	       }
	       else {
		  rename_reg	        = 31 - get_1st_one (reg_use_word);
		  curr_rename_reg       = rename_reg + CR0 + 32;
		  curr_rename_reg_valid = TRUE;
	       }
	    }
	    else {
	       curr_rename_reg_valid = FALSE;
	       return FALSE;
	    }
	 }
	 else assert (1 == 0);
      }
   }

   assert (renameable_cnt <= 1  ||  
	   (op->op.wr[0].reg >= CR0  &&  op->op.wr[0].reg < CR0 + num_ccbits
	    &&  op->op.num_wr == 4));
	       
   return TRUE;
}

/************************************************************************
 *									*
 *				set_reg_for_op_big			*
 *				------------------			*
 *									*
 * Handle unusual arch where have more than 64 GPRs, FPRs, CCBITS.	*
 *									*
 ************************************************************************/

int set_reg_for_op_big (op, first_tip, last_tip)
OPCODE2 *op;
TIP	*first_tip;
TIP	*last_tip;
{
   int	    i, j;
   int	    reg;
   int	    rename_reg;
   int	    renameable_cnt;
   int	    cr_bit_op;
   int	    base_field;
   unsigned *reg_use_word;
   TIP	    *tip;

   reg_use_word = alloca (max_regs * sizeof (reg_use_word[0]));

   assert (curr_rename_reg_valid == FALSE);

   curr_rename_reg = MAXINT;
   renameable_cnt = 0;

   cr_bit_op = is_cr_bit_op (op);
   for (i = 0; i < op->op.num_wr; i++) {
      reg = op->op.wr[i].reg;

      if (is_bit_set (renameable, reg)) {

	 /* CA renaming happens automatically with GPR */
	 if (reg == CA) continue;

	 renameable_cnt++;

	 /* We start with 1 because 0 is GPRs 0-31, the architected regs */
	 for (j = 0; j < max_reg_words; j++)
	    reg_use_word[j] = 0xFFFFFFFF;

	 if      (is_gpr   (reg)) {
	    for (tip = last_tip; tip; tip = tip->prev) {
	       for (j = 1; j < max_reg_words; j++)
	          reg_use_word[j] &= tip->gpr_vliw[j];
	       if (tip == first_tip) break;
	    }

	    rename_reg = first_bit_set (&reg_use_word[1],
					num_gprs - NUM_PPC_GPRS);
	    if (rename_reg != -1) {
	       curr_rename_reg = rename_reg + R0;
	       curr_rename_reg_valid = TRUE;
	    }
	    else {
	       curr_rename_reg_valid = FALSE;
	       return FALSE;
	    }
	 }

	 else if (is_fpr   (reg)) {
	    for (tip = last_tip; tip; tip = tip->prev) {
	       for (j = 1; j < max_reg_words; j++)
	          reg_use_word[j] &= tip->fpr_vliw[j];
	       if (tip == first_tip) break;
	    }

	    rename_reg = first_bit_set (&reg_use_word[1],
					num_fprs - NUM_PPC_FPRS);
	    if (rename_reg != -1) {
	       curr_rename_reg = rename_reg + FP0;
	       curr_rename_reg_valid = TRUE;
	    }
	    else {
	       curr_rename_reg_valid = FALSE;
	       return FALSE;
	    }
	 }

	 else if (is_ccbit (reg)) {
	    for (tip = last_tip; tip; tip = tip->prev) {
	       for (j = 1; j < max_reg_words; j++)
	          reg_use_word[j] &= tip->ccr_vliw[j];
	       if (tip == first_tip) break;
	    }

	    /* If this is a register field op, we must get 4 bits
	       starting on 4-bit boundary.
	    */
	    if (!cr_bit_op) {

	       base_field = CRF8;
	       for (j = 1; j < max_reg_words; j++) {

		  rename_reg = get_1st_crfield (reg_use_word[j]);
		  if (rename_reg >= 0) {
		     curr_rename_reg	   = base_field + rename_reg;
		     curr_rename_reg_valid = TRUE;
		     break;
		  }

		  base_field += (CRF8 - CRF0);
	       }

	       if (rename_reg < 0) {
		  curr_rename_reg_valid = FALSE;
		  return FALSE;
	       }
	    }
	    else {
	       rename_reg = first_bit_set (&reg_use_word[1],
					   num_ccbits - NUM_PPC_CCRS);
	       if (rename_reg != -1) {
		  curr_rename_reg = rename_reg + CR32;
		  curr_rename_reg_valid = TRUE;
	       }
	       else {
		  curr_rename_reg_valid = FALSE;
		  return FALSE;
	       }
	    }
	 }
	 else assert (1 == 0);
      }
   }

   assert (renameable_cnt <= 1  ||  
	   (op->op.wr[0].reg >= CR0  &&  op->op.wr[0].reg < CR0 + num_ccbits
	    &&  op->op.num_wr == 4));
	       
   return TRUE;
}

/************************************************************************
 *									*
 *				set_reg_for_op_big1			*
 *				-------------------			*
 *									*
 * Handle unusual arch where have more than 64 GPRs, FPRs, CCBITS.	*
 *									*
 * OLD: Looks for MINIMUM register which has bit set in "gpr_vliw"	*
 *      from "first_tip" to "last_tip".  This is not good, as it	*
 *	requires that whenever a register is used, that all higher	*
 *	numbered registers in its range also be marked used.		*
 *	Otherwise the MINIMUM register with its bit set on one tip	*
 *	may not have that register available on another tip.  Thus it	*
 *      makes more sense to AND the "gpr_vliw" fields for all of the	*
 *	tips, and any location with a 1 in the resulting bit vector is	*
 *	an available register.						*
 *									*
 ************************************************************************/

int set_reg_for_op_big1 (op, first_tip, last_tip)
OPCODE2 *op;
TIP	*first_tip;
TIP	*last_tip;
{
   int	    i;
   int	    reg;
   int	    rename_reg;
   int	    renameable_cnt;
   int	    cr_bit_op = is_cr_bit_op (op);
   unsigned reg_use_word;
   TIP	    *tip;

   assert (curr_rename_reg_valid == FALSE);

   curr_rename_reg = MAXINT;
   renameable_cnt = 0;

   /* Have more than 64 GPR's and/or more than 64 FPR's:  go slower */
   for (i = 0; i < op->op.num_wr; i++) {
      reg = op->op.wr[i].reg;

      if (is_bit_set (renameable, reg)) {

	 /* CA renaming happens automatically with GPR */
	 if (reg == CA) continue;

	 renameable_cnt++;

	 for (tip = last_tip; tip; tip = tip->prev) {

	    if      (is_gpr   (reg)) {
	       if (num_gprs > 64) {
		  rename_reg = first_bit_set (tip->gpr_vliw, num_gprs);
	          if (rename_reg != -1) rename_reg += R0;
	          else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }
	       }
	       else {
		  rename_reg = 31 - get_1st_one (tip->gpr_vliw[1]);
		  if (rename_reg != -1) rename_reg += R0 + 32;
		  else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }
	       }
	    }
	    else if (is_fpr   (reg)) {
	       if (num_fprs > 64) {
		  rename_reg = first_bit_set (tip->fpr_vliw, num_fprs);
	          if (rename_reg != -1) rename_reg += FP0;
	          else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }
	       }
	       else {
		  rename_reg = 31 - get_1st_one (tip->fpr_vliw[1]);
		  if (rename_reg != -1) rename_reg += FP0 + 32;
		  else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }
	       }
	    }
	    else if (is_ccbit (reg)) {
	       if (num_ccbits > 64) {
		  rename_reg = first_bit_set (tip->ccr_vliw, num_ccbits);
	          if (rename_reg != -1) rename_reg += CR0;
	          else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }

		  /* Make sure on 4-bit boundary for CR field */
		  if (!cr_bit_op) {
		     if ((rename_reg & 3) != 3) rename_reg -= 4;
		     rename_reg &= 0xFFFFFFFC;
		     if (rename_reg < NUM_PPC_CCRS) {
		        curr_rename_reg_valid = FALSE;
			return FALSE;
		     }
		  }
	       }
	       else {
		  rename_reg = 31 - get_1st_one (tip->fpr_vliw[1]);
		  if (rename_reg != -1) rename_reg += CR0 + 32;
		  else {
		     curr_rename_reg_valid = FALSE;
		     return FALSE;
		  }

		  /* Make sure on 4-bit boundary for CR field */
		  if (!cr_bit_op) {
		     if ((rename_reg & 3) != 3) rename_reg -= 4;
		     rename_reg &= 0xFFFFFFFC;
		     if (rename_reg < NUM_PPC_CCRS) {
		        curr_rename_reg_valid = FALSE;
			return FALSE;
		     }
		  }
	       }
	    }
	    else assert (1 == 0);

	    if (rename_reg < curr_rename_reg) {
	       curr_rename_reg       = rename_reg;
	       curr_rename_reg_valid = TRUE;
	    }

	    if (tip == first_tip) break;
	 }
      }
   }

   assert (renameable_cnt <= 1);
   return TRUE;
}

/************************************************************************
 *									*
 *				get_1st_crfield				*
 *				---------------				*
 *									*
 * Return the first 0xF, 4-bit nibble aligned on a 4 bit boundary,	*
 *	  corresponding to the first available field for a renamed	*
 *	  CR field.							*
 *									*
 ************************************************************************/

static int get_1st_crfield (reg_use_word)
unsigned reg_use_word;
{
   if	   (((reg_use_word >> 28)      ) == 0xF) return CRF7 - CRF0;
   else if (((reg_use_word >> 24) & 0xF) == 0xF) return CRF6 - CRF0;
   else if (((reg_use_word >> 20) & 0xF) == 0xF) return CRF5 - CRF0;
   else if (((reg_use_word >> 16) & 0xF) == 0xF) return CRF4 - CRF0;
   else if (((reg_use_word >> 12) & 0xF) == 0xF) return CRF3 - CRF0;
   else if (((reg_use_word >>  8) & 0xF) == 0xF) return CRF2 - CRF0;
   else if (((reg_use_word >>  4) & 0xF) == 0xF) return CRF1 - CRF0;
   else if (((reg_use_word      ) & 0xF) == 0xF) return CRF0 - CRF0;
   else						 return -1;
}
