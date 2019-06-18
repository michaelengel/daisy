/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#pragma alloca

#include <stdio.h>
#include <assert.h>

#include "tree_pos.h"

#define TREE_POS_ELEM		 1024

static unsigned tree_mem[TREE_POS_ELEM];
static unsigned *tree_pos;

void tree_pos_init_page		     (void);
void tree_pos_group_start	     (TREE_POS *);
void tree_pos_right		     (TREE_POS *, TREE_POS *);
void tree_pos_left		     (TREE_POS *, TREE_POS *);
int  tree_pos_is_ancestor	     (TREE_POS *, TREE_POS *);
static int  tree_pos_is_ancestor_big (TREE_POS *, TREE_POS *, int);
static void handle_big_pos	     (TREE_POS *, TREE_POS *);
static void times2		     (int, unsigned *, unsigned *);

/************************************************************************
 *									*
 *				tree_pos_init_page			*
 *				------------------			*
 *									*
 ************************************************************************/

void tree_pos_init_page (void)
{
   tree_pos = tree_mem;
}

/************************************************************************
 *									*
 *				tree_pos_group_start			*
 *				--------------------			*
 *									*
 ************************************************************************/

void tree_pos_group_start (tree_pos)
TREE_POS *tree_pos;
{
   tree_pos->tree_pos     = 1;
   tree_pos->tree_pos_log = 0;
}

/************************************************************************
 *									*
 *				tree_pos_right				*
 *				--------------				*
 *									*
 * Child = 2 * Parent + 1						*
 *									*
 ************************************************************************/

void tree_pos_right (parent, child)
TREE_POS *parent;
TREE_POS *child;
{
   child->tree_pos_log = parent->tree_pos_log + 1;	 /* Common case */
   if (child->tree_pos_log < 32) child->tree_pos = 2 * parent->tree_pos + 1;
   else {
      /* Double the parent into the child (in multiple words). */
      handle_big_pos (parent, child);

      assert ((child->tree_pos_ext[0] & 1) == 0);
      child->tree_pos_ext[0]++;
   }
}

/************************************************************************
 *									*
 *				tree_pos_left				*
 *				-------------				*
 *									*
 * Child = 2 * Parent							*
 *									*
 ************************************************************************/

void tree_pos_left (parent, child)
TREE_POS *parent;
TREE_POS *child;
{
   child->tree_pos_log = parent->tree_pos_log + 1;	 /* Common case */
   if (child->tree_pos_log < 32) child->tree_pos = 2 * parent->tree_pos;
   else handle_big_pos (parent, child);
}

/************************************************************************
 *									*
 *				tree_pos_copy				*
 *				-------------				*
 *									*
 * Child = Parent							*
 *									*
 ************************************************************************/

void tree_pos_copy (parent, child)
TREE_POS *parent;
TREE_POS *child;
{
   child->tree_pos_log = parent->tree_pos_log;	      /* Common case */
   if (child->tree_pos_log < 32) child->tree_pos = parent->tree_pos;
   else child->tree_pos_ext = parent->tree_pos_ext;
}

/************************************************************************
 *									*
 *				tree_pos_is_ancestor			*
 *				--------------------			*
 *									*
 ************************************************************************/

int tree_pos_is_ancestor (parent, child)
TREE_POS *parent;
TREE_POS *child;
{
   int	    diff;
   unsigned base;
   unsigned num_desc;

   diff = child->tree_pos_log - parent->tree_pos_log;
   if (diff < 0) return FALSE;

   if (child->tree_pos_log >= 31)
      return tree_pos_is_ancestor_big (parent, child, diff);

   num_desc = (((unsigned) 1) << diff);
   base     = parent->tree_pos * num_desc;

   if (child->tree_pos >= base  &&  child->tree_pos < base + num_desc)
      return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *				tree_pos_is_ancestor_big		*
 *				------------------------		*
 *									*
 * If bits [ 0,    |_ log2 (Parent) _| ]  match bits			*
 *	   [ diff, |_ log2 (Child)  _| ]				*
 *									*
 * where "diff" is the difference in logs of parent and child values,	*
 * then then parent is indeed an ancestor of child.  (Note that 0 is	*
 * assumed to be the LSB above.						*
 *									*
 ************************************************************************/

static int tree_pos_is_ancestor_big (parent, child, diff)
TREE_POS *parent;
TREE_POS *child;
int diff;
{
   unsigned i;
   unsigned par_bit;
   unsigned child_bit;
   unsigned par_bit_mask;
   unsigned child_bit_mask;
   unsigned child_array_size;
   unsigned par_array_size;
   unsigned *par_word;
   unsigned *child_word;

   assert (diff >= 0);
   child_bit_mask = ((unsigned) 1)  <<  (diff % 32);
   par_bit_mask   = 1;
 
   if (diff == 0) {
      if (child->tree_pos_log >= 32) {
	 child_word = child->tree_pos_ext;
         par_word   = parent->tree_pos_ext;
      }
      else {
	 child_word = &child->tree_pos;
         par_word   = &parent->tree_pos;
      }
   }
   else {			 /* Must copy parent array to "par_word" */
      if (child->tree_pos_log >= 32)
	   child_word = &child->tree_pos_ext[diff / 32];
      else child_word = &child->tree_pos;

      child_array_size = 1 + (child->tree_pos_log / 32);
      par_word = alloca (child_array_size * sizeof (par_word[0]));

      if (parent->tree_pos_log < 32) {	     /* Parent in "tree_pos"     */
	 par_word[0] = parent->tree_pos;
	 i = 1;
      }
      else {				     /* Parent in "tree_pos_ext" */
         par_array_size = 1 + (parent->tree_pos_log / 32);
	 for (i = 0; i < par_array_size; i++)
	    par_word[i] = parent->tree_pos_ext[i];
      }

      /* Upper portions of parent position filled with zeroes */
      for (; i < child_array_size; i++)
         par_word[i] = 0;
   }

   for (i = 0; i <= parent->tree_pos_log; i++) {

      child_bit = (*child_word) & child_bit_mask;
      par_bit   =   (*par_word) &   par_bit_mask;

      if      ( child_bit  &&  !par_bit) return FALSE;
      else if (!child_bit  &&   par_bit) return FALSE;

      if (  par_bit_mask == 0x80000000) {   par_word++;   par_bit_mask   = 1; }
      else						  par_bit_mask <<= 1;
      if (child_bit_mask == 0x80000000) { child_word++; child_bit_mask   = 1; }
      else						child_bit_mask <<= 1;
   }

   return TRUE;
}

/************************************************************************
 *									*
 *				handle_big_pos				*
 *				--------------				*
 *									*
 * For cases in which the child requires more than 32 bits, the		*
 * contents of the parent are doubled and placed in the child.  It	*
 * is up to the caller to add 1, if need be.				*
 *									*
 ************************************************************************/

static void handle_big_pos (parent, child)
TREE_POS *parent;
TREE_POS *child;
{
   int      i;
   int      array_size;
   unsigned *dummy;

   assert (child->tree_pos_log >= 32);

   array_size = 1 + (child->tree_pos_log / 32);
   child->tree_pos_ext = tree_pos;
   tree_pos += array_size;
   assert (tree_pos < &tree_mem[TREE_POS_ELEM]);

   dummy = alloca (array_size * sizeof (child->tree_pos_ext[0]));

   if (parent->tree_pos_log != 31) {
      if ((parent->tree_pos_log & 0x1F) != 31)
	 times2 (array_size, parent->tree_pos_ext, child->tree_pos_ext);
      else {
	 /* When child has more words than parent, must add 0 word to parent */
	 dummy[array_size - 1] = 0;
	 for (i = array_size - 2; i >= 0; i--)
	    dummy[i] = parent->tree_pos_ext[i];

	 times2 (array_size, dummy,		   child->tree_pos_ext);
      }
   }
   else {
      dummy[0] = parent->tree_pos;
      dummy[1] = 0;
      times2    (array_size, dummy,		   child->tree_pos_ext);
      assert (child->tree_pos_ext[1] == 1);
   }
}

/************************************************************************
 *									*
 *				times2					*
 *				------					*
 *									*
 * Multiply by 2, arbitrary precision value in "parent" and place in	*
 * child.								*
 *									*
 ************************************************************************/

static void times2 (array_size, parent, child)
int	 array_size;
unsigned *parent;
unsigned *child;
{
   int i;
   int shift_out;

   shift_out = 0;
   for (i = 0; i < array_size; i++) {
      child[i] = (parent[i] << 1) + shift_out;
      shift_out = parent[i] >> 31;
   }
}

#ifdef DEBUG_TREE_POS

/************************************************************************
 *									*
 *				main					*
 *				----					*
 *									*
 * For unit testing of routines.					*
 *									*
 ************************************************************************/

main ()
{
   int	  i, j;
   int	  num_words;
   int	  ancest;
   static TREE_POS tree_pos[256];

   tree_pos_init_page   ();
   tree_pos_group_start (&tree_pos[0]);

   for (i = 2; i < 150; i+=2) {
      tree_pos_right (&tree_pos[i-2], &tree_pos[i-1]);
      tree_pos_left  (&tree_pos[i-2], &tree_pos[i]);
   }
/*
   for (i = 1; i < 75; i+=1) {
      tree_pos_left  (&tree_pos[i-1], &tree_pos[i]);
   }
*/
   for (i = 1; i < 150; i++) {
      ancest = tree_pos_is_ancestor (&tree_pos[128], &tree_pos[i]);
      printf ("%d: %d is ancestor of %d\n", ancest, 128, i);
   }

   printf ("\n");

/*
   for (i = 0; i < 75; i++) {
      printf ("[%2d]  log = %2d,  tree_pos = 0x%08x,  tree_pos_ext = 0x%08x\n",
	      i, tree_pos[i].tree_pos_log, tree_pos[i].tree_pos,
	         tree_pos[i].tree_pos_ext);
   }
*/

   for (i = 0; i < 150; i++) {

      if (tree_pos[i].tree_pos_log < 32) {
         printf ("[%2d]  log = %2d,  tree_pos = 0x%08x\n",
		 i, tree_pos[i].tree_pos_log, tree_pos[i].tree_pos);
      }
      else {
	 num_words = tree_pos[i].tree_pos_log / 32;
	 printf ("[%2d]  log = %2d,  tree_pos = 0x",
		 i, tree_pos[i].tree_pos_log);

	 for (j = num_words; j >= 0; j--)
	    printf ("%08x", tree_pos[i].tree_pos_ext[j]);

	 printf ("\n");
      }
   }

}
#endif
