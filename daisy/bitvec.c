/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

/************************************************************************
 *									*
 *				set_bit					*
 *				-------					*
 *									*
 ************************************************************************/

set_bit (v, posn)
unsigned *v;
unsigned posn;
{
   unsigned word = posn >> 5;
   unsigned mask = 1 << (posn & 0x1F);

   v[word] |= mask;
}

/************************************************************************
 *									*
 *				clr_bit					*
 *				-------					*
 *									*
 ************************************************************************/

clr_bit (v, posn)
unsigned *v;
unsigned posn;
{
   unsigned word = posn >> 5;
   unsigned mask = ~(1 << (posn & 0x1F));

   v[word] &= mask;
}

/************************************************************************
 *									*
 *				set_bit_range_word			*
 *				------------------			*
 *									*
 * Can only handle ranges contained within one 32-bit word of the bit	*
 * vector (for efficiency).  This can easily be arranged for our needs	*
 * (Machine Register Usage).						*
 *									*
 ************************************************************************/

set_bit_range_word (v, start, end)
unsigned *v;
unsigned start;
unsigned end;
{
   unsigned start_word = start >> 5;
   unsigned end_word   = end >> 5;
   unsigned mask1;
   unsigned mask2;
   unsigned shift;

   assert (start_word == end_word);

   shift = end - start + 1;
   if (shift == 32) v[start_word] = 0xFFFFFFFF;
   else {
      mask1 = ~(0xFFFFFFFF << shift);
      mask2 = mask1 << (start & 0x1F);
      v[start_word] |= mask2;
   }
}

/************************************************************************
 *									*
 *				clr_bit_range_word			*
 *				------------------			*
 *									*
 * Can only handle ranges contained within one 32-bit word of the bit	*
 * vector (for efficiency).  This can easily be arranged for our needs	*
 * (Machine Register Usage).						*
 *									*
 ************************************************************************/

clr_bit_range_word (v, start, end)
unsigned *v;
unsigned start;
unsigned end;
{
   unsigned start_word = start >> 5;
   unsigned end_word   = end >> 5;
   unsigned mask1;
   unsigned mask2;
   unsigned shift;

   assert (start_word == end_word);

   shift = end - start + 1;
   if (shift == 32) v[start_word] = 0;
   else {
      mask1 = ~(0xFFFFFFFF << shift);
      mask2 = ~(mask1 << (start & 0x1F));
      v[start_word] &= mask2;
   }
}

/************************************************************************
 *									*
 *				clr_bit_range				*
 *				-------------				*
 *									*
 ************************************************************************/

clr_bit_range (v, start, end)
unsigned *v;
unsigned start;
unsigned end;
{
   unsigned i;
   unsigned limit;
   unsigned start_word = start >> 5;
   unsigned end_word   = end >> 5;

   switch (end_word - start_word) {

      default:
         limit = end_word - 1;
         for (i = start_word + 1; i <= limit; i++)
	    v[i] = 0;

	 /* FALL THRU */

      case 1:
	 clr_bit_range_word (v, start, (start & 0xFFFFFFE0) + 31);
	 clr_bit_range_word (v, end & 0xFFFFFFE0, end);
	 break;

      case 0:
	 clr_bit_range_word (v, start, end);
	 break;
   }
}

/************************************************************************
 *									*
 *				set_bit_range				*
 *				-------------				*
 *									*
 ************************************************************************/

set_bit_range (v, start, end)
unsigned *v;
unsigned start;
unsigned end;
{
   unsigned i;
   unsigned limit;
   unsigned start_word = start >> 5;
   unsigned end_word   = end >> 5;

   switch (end_word - start_word) {

      default:
         limit = end_word - 1;
         for (i = start_word + 1; i <= limit; i++)
	    v[i] = 0xFFFFFFFF;

	 /* FALL THRU */

      case 1:
	 set_bit_range_word (v, start, (start & 0xFFFFFFE0) + 31);
	 set_bit_range_word (v, end & 0xFFFFFFE0, end);
	 break;

      case 0:
	 set_bit_range_word (v, start, end);
	 break;
   }
}

/************************************************************************
 *									*
 *				set_all_bits				*
 *				------------				*
 *									*
 ************************************************************************/

set_all_bits (v, size)
unsigned *v;
unsigned size;
{
   unsigned i;
   unsigned num_words = (size + 31) >> 5;

   for (i = 0; i < num_words; i++)
      v[i] = 0xFFFFFFFF;
}

/************************************************************************
 *									*
 *				clr_all_bits				*
 *				------------				*
 *									*
 ************************************************************************/

clr_all_bits (v, size)
unsigned *v;
unsigned size;
{
   unsigned i;
   unsigned num_words = (size + 31) >> 5;

   for (i = 0; i < num_words; i++)
      v[i] = 0;
}

/************************************************************************
 *									*
 *				and_vectors				*
 *				-----------				*
 *									*
 ************************************************************************/

and_vectors (v1, v2, result, size)
unsigned *v1;
unsigned *v2;
unsigned *result;
unsigned size;
{
   exit (1);
}

/************************************************************************
 *									*
 *				or_vectors				*
 *				----------				*
 *									*
 ************************************************************************/

or_vectors (v1, v2, result, size)
unsigned *v1;
unsigned *v2;
unsigned *result;
unsigned size;
{
   exit (1);
}

/************************************************************************
 *									*
 *				is_bit_set				*
 *				----------				*
 *									*
 ************************************************************************/

int is_bit_set (v, posn)
unsigned *v;
unsigned posn;
{
   unsigned word = posn >> 5;
   unsigned mask = 1 << (posn & 0x1F);

   return v[word] & mask;
}

/************************************************************************
 *									*
 *				first_bit_set				*
 *				-------------				*
 *									*
 * Returns position of first 1-bit starting from end of vector and	*
 * moving towards start.  If there are no 1-bits, then -1 returned.	*
 *									*
 ************************************************************************/

int first_bit_set (v, size)
unsigned *v;
unsigned size;
{
   unsigned num_words = (size + 31) >> 5;
   int	    base = 32 * (num_words - 1);
   int	    first_one;
   int	    i;

   for (i = num_words - 1; i >= 0; i--, base -= 32) {
      first_one = get_1st_one (v[i]);
      if (first_one != 32) return base + (31 - first_one);
   }

   return -1;
}

/************************************************************************
 *									*
 *				is_range_set				*
 *				------------				*
 *									*
 ************************************************************************/

int is_range_set (v, start, end)
unsigned *v;
unsigned start;
unsigned end;
{
   unsigned start_word = start >> 5;
   unsigned end_word   = end >> 5;
   unsigned mask1;
   unsigned mask2;
   unsigned shift;

   assert (start_word == end_word);

   shift = end - start + 1;
   if (shift == 32) mask2 = 0xFFFFFFFF;
   else {
      mask1 = ~(0xFFFFFFFF << shift);
      mask2 = mask1 << (start & 0x1F);
   }

   return v[start_word] & mask2;
}

/************************************************************************
 *									*
 *				is_overlap				*
 *				----------				*
 *									*
 ************************************************************************/

int is_overlap (v1, v2, size)
unsigned *v1;
unsigned *v2;
unsigned size;
{
   unsigned i;
   unsigned num_words = (size + 31) >> 5;

   for (i = 0; i < num_words; i++)
      if (v1[i] & v2[i]) return TRUE;

   return FALSE;
}

/************************************************************************
 *									*
 *				bit_vector_iterate			*
 *				------------------			*
 *									*
 * Invokes func (bit_posn) at all set bit positions.			*
 *									*
 ************************************************************************/

bit_vector_iterate (v, size, func)
unsigned *v;
unsigned size;
void	 (*func) ();
{
   unsigned i;
   unsigned mask;
   unsigned posn;
   unsigned num_words = (size + 31) >> 5;

   posn = 0;
   for (i = 0; i < num_words; i++) {
      if (v[i] == 0) posn += 32;	/* Efficiency hack */

      else for (mask = 1; mask; mask <<= 1) {
	 if (v[i] & mask) func (posn);
	 posn++;
      }
   }
}
