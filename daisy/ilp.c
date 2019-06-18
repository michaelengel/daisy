/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"
#include "opcode_fmt.h"

#define MAX_PAGE_TIPS			8192

typedef struct _page_tip_list {
   TIP			 *tip;
   struct _page_tip_list *next;
} PAGE_TIP_LIST;

/* Ptr to list of entry point tips for each address on current page */
PAGE_TIP_LIST  *page_tips[MAX_RPAGE_ENTRIES];
PAGE_TIP_LIST   page_tip_list[MAX_PAGE_TIPS];
int	        num_page_tips;

extern unsigned rpagesize_mask;

int ilp_increasing (TIP *, unsigned);
TIP *get_prev_tip_for_addr (TIP *, PAGE_TIP_LIST *);
int get_ilp (TIP *);

/************************************************************************
 *									*
 *				ilp_increasing				*
 *				--------------				*
 *									*
 * RETURNS:  TRUE  if ILP has increased on this path since the previous	*
 *	           time we saw "addr" on the current path of "tip",	*
 *	     TRUE  if we have never seen "addr" on the current path,	*
 *	     FALSE otherwise.						*
 *									*
 ************************************************************************/

int ilp_increasing (tip, addr)
TIP      *tip;
unsigned addr;
{
   int		 prev_ilp;
   int		 curr_ilp;
   int		 rval;
   unsigned	 index = (addr & rpagesize_mask) >> 2;
   TIP		 *pred_tip;
   PAGE_TIP_LIST *pt_list;
   PAGE_TIP_LIST *next;

   if (page_tips[index] == 0) {
      next = 0;
      rval = TRUE;
   }
   else {
      pt_list = page_tips[index];
      pred_tip = get_prev_tip_for_addr (tip, pt_list);
      if (pred_tip == 0) rval = TRUE;
      else {
	 prev_ilp = get_ilp (pred_tip);
	 curr_ilp = get_ilp (tip);
	 if (curr_ilp > prev_ilp) rval = TRUE;
	 else			  rval = FALSE;
      }
      next = pt_list;
   }

   assert (num_page_tips < MAX_PAGE_TIPS);
   page_tips[index] = &page_tip_list[num_page_tips];
   page_tip_list[num_page_tips].tip  = tip;
   page_tip_list[num_page_tips].next = next;
   num_page_tips++;

   return rval;
}

/************************************************************************
 *									*
 *				get_prev_tip_for_addr			*
 *				---------------------			*
 *									*
 * RETURNS:  On current path, nearest predecessor tip which starts from	*
 *	     same address as "tip".  If no predecessor tips begin from	*
 *	     the same address as "tip", return 0.			*
 *									*
 ************************************************************************/

static TIP *get_prev_tip_for_addr (tip, pt_list_in)
TIP	      *tip;
PAGE_TIP_LIST *pt_list_in;
{
   int		 start_group = tip->vliw->group;
   PAGE_TIP_LIST *pt_list;

   /* Traverse up chain of our predecessors */
   do {
      assert (tip->vliw->group == start_group);

      /* Look at list of tips starting from this PPC address */
      pt_list = pt_list_in;
      do {
	 if (tip == pt_list->tip) return tip;  /* Pred has same addr as us */
	 pt_list = pt_list->next;
      } while (pt_list);

      tip = tip->prev;
   } while (tip);

   return tip;
}

/************************************************************************
 *									*
 *				get_ilp					*
 *				-------					*
 *									*
 * RETURNS:  1000 times the ILP from this tip up to the start of the	*
 *	     group.							*
 *									*
 ************************************************************************/

int get_ilp (tip)
TIP *tip;
{
   int  num_vliws   = 1;
   int  orig_ops    = 0;
   int  start_group = tip->vliw->group;
   VLIW *prev_vliw  = tip->vliw;

   do {
      assert (tip->vliw->group == start_group);

      orig_ops += tip->orig_ops;
      if (tip->vliw != prev_vliw) {
	 num_vliws++;
	 prev_vliw = tip->vliw;
      }
      tip = tip->prev;
   } while (tip);

   return (1000 * orig_ops) / num_vliws;
}
