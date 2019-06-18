/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"

extern unsigned xlate_entry_cnt;

char *save_group_exit_info (char *, VLIW *, unsigned, unsigned, TIP *, int);
char *save_cond_branch_info (TIP *, char *);

/************************************************************************
 *									*
 *				save_group_exit_info			*
 *				--------------------			*
 *									*
 ************************************************************************/

char *save_group_exit_info (p, entry_vliw, entry_vliw_num, ppc_code_addr,
			    tip, num_cycles)
char	 *p;
VLIW	 *entry_vliw;
unsigned entry_vliw_num;
unsigned ppc_code_addr;
TIP	 *tip;
int	 num_cycles;	/* From Group Start to exit, 0 ==> Not exit */
{
   unsigned	  same_group;
   unsigned	  vmm_invoc_cnt;
   unsigned short path_ops;

   *((unsigned short *) p) = (unsigned short) num_cycles;
   assert (num_cycles <= 0xFFFF);		p += sizeof (unsigned short);

   /* Don't waste space putting out info for non-exits */
   if (num_cycles == 0) return p;

   if (tip->right->vliw == entry_vliw) same_group = 0x80000000;
   else				       same_group = 0;

   /* Use the high order bit to flag whether this exit is a loop back
      to the start of the group.
   */
   vmm_invoc_cnt = xlate_entry_cnt | same_group;

   /* Put out info about group exits so we can see what and how many
      paths account for the bulk of the programs execution time.
   */
   *((unsigned *) p)	   = entry_vliw_num;	p += sizeof (entry_vliw_num);
   *((unsigned *) p)	   = ppc_code_addr;	p += sizeof (ppc_code_addr);
   *((unsigned *) p)	   = vmm_invoc_cnt;     p += sizeof (vmm_invoc_cnt);
   *((unsigned *) p)	   = tip;		p += sizeof (tip);

   path_ops = (unsigned short) num_orig_ops_on_path (tip);
   assert (path_ops <= 0xFFFF);		
   *((unsigned short *) p) = path_ops;	        p += sizeof (unsigned short);

   return save_cond_branch_info (tip, p);
}

/************************************************************************
 *									*
 *				save_cond_branch_info			*
 *				---------------------			*
 *									*
 ************************************************************************/

char *save_cond_branch_info (tip, p)
TIP  *tip;
char *p;
{
   char		  dir;					/* Boolean */
   unsigned short num_conds   = 0;
   unsigned short *pnum_conds;
   TIP		  *prev;

   pnum_conds = (unsigned short *) p;		   /* Keep marker to where  */
   p += sizeof (num_conds);			   /* we fill # of cond-brs */

   while (tip) {
      if (prev = tip->prev) {			   /* Reached group start ? */
	 if (prev->right  &&  prev->left) {	   /* No, at cond branch  ? */
	    if      (prev->left  == tip) dir = 0;  /* Are we fall-thru    ? */
	    else if (prev->right == tip) dir = 1;  /* Are we target	  ? */
	    else assert (1 == 0);

	    assert (prev->br_addr != 0);

	    *p++ = dir;
	    *((unsigned *) p) = (unsigned) prev->br_addr;
	    p += sizeof (prev->br_addr);
	    num_conds++;
	 }
      }

      tip = tip->prev;
   }

   *pnum_conds = num_conds;

   return p;
}

/************************************************************************
 *									*
 *				num_orig_ops_on_path			*
 *				--------------------			*
 *									*
 ************************************************************************/

int num_orig_ops_on_path (tip)
TIP *tip;
{
   int num_ops = 0;
   TIP *t;

   for (t = tip; t; t = t->prev)
      num_ops += t->orig_ops;

   return num_ops;
}
