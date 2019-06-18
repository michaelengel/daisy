/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <values.h>

/************************************************************************
 *									*
 *				heap_insert				*
 *				-----------				*
 *									*
 * Insert a new item "v" into heap structure "a" with "a_size"		*
 * elements.  (Note that a[0] is reserved, hence a_size is one		*
 * greater than the "real" number of elements.)  The "a_cmp"		*
 * function returns 1 if its 1st arg is greater than its 2nd,		*
 * returns -1, if its 1st arg is less, and returns 0 if it is		*
 * equal.  The caller must update "a_size" itself, and insure		*
 * that space exists for an additional element.				*
 *									*
 ************************************************************************/

void heap_insert (v, a, a_size, dummy, a_cmp, a_setmax)
void	 *v;
void	 **a;
void	 *dummy;		/* Ptr to Struct putting in heap */
unsigned a_size;
int	 (*a_cmp) ();		/* Function to compare two elements in a */
int	 (*a_setmax) ();	/* Function to set elem of a to largest val */
{
   a_size++;
   a[a_size] = v;
   a[0]      = dummy;
   upheap (a, a_size, a_cmp, a_setmax);
}

/************************************************************************
 *									*
 *				heap_remove				*
 *				-----------				*
 *									*
 * Remove and return largest item from heap structure "a" with		*
 * "a_size" elements.  The caller must decrement "a_size" itself.	*
 *									*
 ************************************************************************/

void *heap_remove (a, a_size, a_cmp)
void	 **a;
unsigned a_size;
int	 (*a_cmp) ();		/* Function to compare two elements in a */
{
   void *remove = a[1];

   a[1] = a[a_size--];
   downheap (a, a_size, 1, a_cmp);

   return remove;
}

/************************************************************************
 *									*
 *				heap_replace				*
 *				------------				*
 *									*
 * Replace the largest item in heap "a" with a new item "v", unless	*
 * "v" is larger than the largest item in "a".  There are "a_size"	*
 * items in "a".							*
 *									*
 ************************************************************************/

void *heap_replace (v, a, a_size, a_cmp)
void	 *v;
void	 **a;
unsigned a_size;
int	 (*a_cmp) ();		/* Function to compare two elements in a */
{
   a[0] = v;
   downheap (a, a_size, 0, a_cmp);

   return a[0];
}

/************************************************************************
 *									*
 *				upheap					*
 *				------					*
 *									*
 * Subroutine usually called by "heap_insert".				*
 *									*
 ************************************************************************/

static upheap (a, k, a_cmp, a_setmax)
void     **a;
unsigned k;
int	 (*a_cmp) ();		/* Function to compare two elements in a */
int	 (*a_setmax) ();	/* Function to set elem of a to largest val */
{
   void     *v;
   unsigned k2;

   v = a[k];
   
   a_setmax (a[0]);

   k2 = k >> 1;
   while (a_cmp (a[k2], v) <= 0) {
      a[k] = a[k2];
      k    = k2;
      k2 >>= 1;
   }
   a[k] = v;
}

/************************************************************************
 *									*
 *				downheap				*
 *				--------				*
 *									*
 * Subroutine usually called by "heap_remove" and "heap_replace".	*
 *									*
 ************************************************************************/

static downheap (a, a_size, k, a_cmp)
void	 **a;
unsigned a_size;
unsigned k;
int	 (*a_cmp) ();		/* Function to compare two elements in a */
{
   int      i, j;
   unsigned a_size2 = a_size >> 1;
   void	    *v      = a[k];

   while (k <= a_size2) {
      j = k + k;				/* Find 1st of two children */
      if (j < a_size)
	 if (a_cmp (a[j], a[j+1]) < 0) j++;	/* Find larger child */

      if (a_cmp (v, a[j]) >= 0) break;		/* Child smaller than v */
      else {
	 a[k] = a[j];
	   k  =   j;
      }
   }

   a[k] = v;
}

#ifdef TESTING_HEAP_ROUTINES

typedef struct {
   int x;
   int prio;
} HTEST;

#define HTEST_SIZE	20

HTEST data[HTEST_SIZE];
HTEST *htest[HTEST_SIZE+1];

main ()
{
   int   i;
   int	 size;
   HTEST *v;
   HTEST dummy;
   void  ht_setmax (HTEST *);
   int   ht_cmp    (HTEST *, HTEST *);

   for (i = 0; i < HTEST_SIZE; i++)
      data[i].prio = 75 - 3 * (i-HTEST_SIZE/2) * (i-HTEST_SIZE/2);

   for (i = 0; i < HTEST_SIZE; i++)
      printf ("data[%d].prio = %d\n", i, data[i].prio);

   size = 0;
   for (i = 0; i < HTEST_SIZE; i++)
      heap_insert (&data[i], htest, size++, &dummy, ht_cmp, ht_setmax);

   for (i = 1; i <= HTEST_SIZE; i++)
      printf ("htest[%d].prio = %d\n", i, htest[i]->prio);

   dummy.prio = -1000;
   heap_replace (&dummy, htest, size, ht_cmp);

   printf ("\nAFTER REPLACING LARGEST WITH -1000:\n");
   for (i = 1; i <= HTEST_SIZE; i++)
      printf ("htest[%d].prio = %d\n", i, htest[i]->prio);

   for (i = 0; i < HTEST_SIZE; i++) {
      v = heap_remove (htest, size--, ht_cmp);
      printf ("Removed prio = %d\n", v->prio);
   }
}

int ht_cmp (a, b)
HTEST *a;
HTEST *b;
{
   if      (a->prio > b->prio) return  1;
   else if (a->prio < b->prio) return -1;
   else			       return  0;
}

void ht_setmax (a)
HTEST *a;
{
   a->prio = MAXINT;
}

#endif
