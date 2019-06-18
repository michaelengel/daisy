/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"

static VLIW **vliw_list;
static int  index;
static int  max_list_size;

/************************************************************************
 *									*
 *				clr_vliw_visit				*
 *				--------------				*
 *									*
 * Used mutually recursively with "clr_vliw_visit_tip" to clear the	*
 * "visited" bit from VLIW's after iterating through them.		*
 *									*
 ************************************************************************/

clr_vliw_visit (vliw, group)
VLIW *vliw;
int  group;
{
   if (vliw->visited  &&  vliw->group == group) {
      vliw->visited = FALSE;
      clr_vliw_visit_tip (vliw->entry, group);
   }
}

/************************************************************************
 *									*
 *				clr_vliw_visit_tip			*
 *				------------------			*
 *									*
 * Used mutually recursively with "clr_vliw_visit" to clear the		*
 * "visited" bit from VLIW's after iterating through them.		*
 *									*
 ************************************************************************/

static clr_vliw_visit_tip (tip, group)
TIP *tip;
int group;
{
   if (tip->br_type == BR_ONPAGE) {
      if (tip->left == 0) {
	 clr_vliw_visit (tip->right->vliw, group);
      }
      else {
	 clr_vliw_visit_tip (tip->left, group);
	 clr_vliw_visit_tip (tip->right, group);
      }
   }
}

/************************************************************************
 *									*
 *				make_vliw_list				*
 *				--------------				*
 *									*
 * Put in vliw_list, one pointer to each VLIW on the page.  Return the	*
 * number of VLIW's placed in vliw_list.				*
 *									*
 ************************************************************************/

int make_vliw_list (vliw, _vliw_list, _max_list_size, group)
VLIW *vliw;
VLIW **_vliw_list;
int  _max_list_size;
int  group;
{
   vliw_list     = _vliw_list;
   max_list_size = _max_list_size;
   index         = 0;

   make_vliw_list_recur (vliw, group);
   clr_vliw_visit (vliw, group);

   return index;
}

/************************************************************************
 *									*
 *				make_vliw_list_recur			*
 *				--------------------			*
 *									*
 * Used mutually recursively with "make_vliw_list_tip" to make a list	*
 * of VLIW's reachable (and on the same page) as the "vliw" used to	*
 * start the recursion.							*
 *									*
 ************************************************************************/

static make_vliw_list_recur (vliw, group)
VLIW *vliw;
int  group;
{
   if (!vliw->visited  &&  vliw->group == group) {

      assert (index < max_list_size);
      vliw_list[index++] = vliw;

      vliw->visited = TRUE;

      make_vliw_list_tip (vliw->entry, group);
   }
}

/************************************************************************
 *									*
 *				make_vliw_list_tip			*
 *				------------------			*
 *									*
 * Used mutually recursively with "make_vliw_list_recur" to make a list	*
 * of VLIW's reachable (and on the same page) as the "vliw" used to	*
 * start the recursion.							*
 *									*
 ************************************************************************/

static make_vliw_list_tip (tip, group)
TIP  *tip;
int  group;
{
   if (tip->br_type == BR_ONPAGE) {
      if (tip->left == 0) {
	 make_vliw_list_recur (tip->right->vliw, group);
      }
      else { 
	 make_vliw_list_tip (tip->left, group);
	 make_vliw_list_tip (tip->right, group);
      }
   }
}
