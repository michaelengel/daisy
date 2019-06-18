/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*######################################################################*
 * NOTE:  This module is invoked since we do not translate kernel	*
 *	  code (and hence entry points to the kernel must be properly	*
 *	  massaged.							*
 *######################################################################*/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "resrc_map.h"
#include "resrc_offset.h"
#include "branch.h"
#include "vliw.h"

TIP *handle_svc    (TIP *, OPCODE2 *, unsigned *);
TIP *handle_trap1  (TIP *, OPCODE2 *);
TIP *handle_rfpriv (TIP *, OPCODE2 *, unsigned *, unsigned);

/************************************************************************
 *									*
 *				handle_svc				*
 *				----------				*
 *									*
 * SVC without link bit set means either it never returns or		*
 * returns to the place that called us:					*
 *									*
 *	1) Move into the real PPC link register the address of		*
 *	   a piece of code ("svc_rtn_code") dumped by "daisy"		*
 *	   that does a hash table lookup of LR(r13) and jumps		*
 *	   to the previously translated code or the translator		*
 *	   as appropriate.  This piece of code is essentially		*
 *	   that produced by "xlate_indir_v2p".				*
 *									*
 *	2) Dump out the svc instruction (which does not have		*
 *	   the link bit set).						*
 *									*
 * NOTE:  SVC Opcodes put in VLIW will never have the link bit set.	*
 *	  The translation of an svc instruction from a VLIW should	*
 *									*
 ************************************************************************/

TIP *handle_svc (tip, opcode, br_addr)
TIP	 *tip;
OPCODE2	 *opcode;
unsigned *br_addr;
{
   TIP *ins_tip;
   TIP *rtn_tip;

   /* Fill in our link register if need be */
   if (opcode->op.ins & LINK_BIT) {
      branch_and_link (tip, br_addr);
      opcode->op.ins &= ~LINK_BIT;	/* Insert without link bit set */
   }

   rtn_tip = insert_op (opcode, tip, tip->vliw->time, &ins_tip);

   rtn_tip->br_type = BR_SVC;
   rtn_tip->br_ins  = opcode->op.ins & 0xFFFFFFFE;	/* Link bit off */
   rtn_tip->orig_ops++;
   rtn_tip->left    = 0;		/* To avoid confusing simul codegen */
   rtn_tip->right   = 0;

   return rtn_tip;
}

/************************************************************************
 *									*
 *				handle_trap1				*
 *				------------				*
 *									*
 * Make sure the comparison registers are in real (not memory mapped)	*
 * PPC registers and do the trap.					*
 *									*
 ************************************************************************/

TIP *handle_trap1 (tip, opcode)
TIP	 *tip;
OPCODE2  *opcode;
{
   int earliest;
   TIP *ins_tip;
   TIP *rtn_tip;

   earliest = get_earliest_time (tip, opcode, FALSE);
   if (earliest < tip->vliw->time) earliest = tip->vliw->time;

   rtn_tip = insert_op (opcode, tip, earliest, &ins_tip);

/* NOT DO THIS BECAUSE NOT TREAT TRAP AS BRANCH:
   rtn_tip->br_type = BR_TRAP;
   rtn_tip->br_ins  = opcode->op.ins;
*/
   rtn_tip->orig_ops++;

   return rtn_tip;
}

/************************************************************************
 *									*
 *				handle_rfpriv				*
 *				-------------				*
 *									*
 * For correct user code, this should never be called.			*
 *									*
 ************************************************************************/

TIP *handle_rfpriv (tip, opcode, br_addr, branch_type)
TIP	 *tip;
OPCODE2	 *opcode;
unsigned *br_addr;
unsigned branch_type;
{
   char *br_name;
   TIP  *ins_tip;
   TIP  *rtn_tip;

   rtn_tip = insert_op (opcode, tip, tip->vliw->time, &ins_tip);

   if (branch_type == RFI) {
      br_name = "rfi";
      rtn_tip->br_type = BR_INDIR_RFI;
   }
   else {
      br_name = "rfsvc";
      rtn_tip->br_type = BR_INDIR_RFSVC;
   }
   rtn_tip->br_ins  = opcode->op.ins;
   rtn_tip->orig_ops++;
   rtn_tip->left    = 0;		/* To avoid confusing simul codegen */
   rtn_tip->right   = 0;

   fprintf (stderr, "WARNING:  \"%s\" found at location 0x%08x in orig pgm\n",
	    br_name, br_addr);

   return rtn_tip;
}
