/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

unsigned count_ones_nibble (unsigned);

/************************************************************************
 *									*
 *				mem_copy				*
 *				--------				*
 *									*
 * Copy block of memory from "src" to "dest" one word at a time.	*
 * Invoke this if do not want to use library functions.			*
 *									*
 * ASSUMES src and dest are non-overlapping.				*
 *									*
 ************************************************************************/

mem_copy (dest, src, num_bytes)
char *dest;
char *src;
int  num_bytes;
{
   int	    i;
   int	    num_words;
   unsigned *wsrc;
   unsigned *wdest;

   switch (num_bytes & 3) {
      case 0:
	 num_words = num_bytes >> 2;
	 break;

      case 1:
	 *dest++ = *src++;
	 if (num_bytes == 1) return;
         else num_words = num_bytes >> 2;
	 break;

      case 2:
	 *dest++ = *src++;
	 *dest++ = *src++;
	 if (num_bytes == 2) return;
         else num_words = num_bytes >> 2;
	 break;

      case 3:
	 *dest++ = *src++;
	 *dest++ = *src++;
	 *dest++ = *src++;
	 if (num_bytes == 3) return;
         else num_words = num_bytes >> 2;
	 break;

      default:
	 assert (1 == 0);
	 break;
   }

   wdest = (unsigned *) dest;
   wsrc  = (unsigned *) src;
   for (i = 0; i < num_words; i++)
      *wdest++ = *wsrc++;
}

/************************************************************************
 *									*
 *				mem_clear				*
 *				---------				*
 *									*
 * Clear block of memory at "dest" one word at a time.			*
 * Invoke this if do not want to use library functions.			*
 *									*
 ************************************************************************/

mem_clear (dest, num_bytes)
char *dest;
int  num_bytes;
{
   int	    i;
   int	    num_words;
   unsigned *wdest;

   switch (num_bytes & 3) {
      case 0:
	 num_words = num_bytes >> 2;
	 break;

      case 1:
	 *dest++ = 0;
	 if (num_bytes == 1) return;
         else num_words = num_bytes >> 2;
	 break;

      case 2:
	 *dest++ = 0;
	 *dest++ = 0;
	 if (num_bytes == 2) return;
         else num_words = num_bytes >> 2;
	 break;

      case 3:
	 *dest++ = 0;
	 *dest++ = 0;
	 *dest++ = 0;
	 if (num_bytes == 3) return;
         else num_words = num_bytes >> 2;
	 break;

      default:
	 assert (1 == 0);
	 break;
   }

   wdest = (unsigned *) dest;
   for (i = 0; i < num_words; i++)
      *wdest++ = 0;
}

/************************************************************************
 *									*
 *				count_ones_vector			*
 *				-----------------			*
 *									*
 ************************************************************************/

unsigned count_ones_vector (v, size)
unsigned *v;
unsigned size;
{
   unsigned i;
   unsigned word_val;
   unsigned cnt = 0;

   for (i = 0; i < size; i++) {
      word_val = v[i];
      cnt += count_ones_nibble  (word_val & 0x0000000F) + 
	     count_ones_nibble ((word_val & 0x000000F0) >>  4) + 
	     count_ones_nibble ((word_val & 0x00000F00) >>  8) + 
	     count_ones_nibble ((word_val & 0x0000F000) >> 12) + 
	     count_ones_nibble ((word_val & 0x000F0000) >> 16) + 
	     count_ones_nibble ((word_val & 0x00F00000) >> 20) + 
	     count_ones_nibble ((word_val & 0x0F000000) >> 24) + 
	     count_ones_nibble ((word_val & 0xF0000000) >> 28);
   }

   return cnt;
}

/************************************************************************
 *									*
 *				count_ones_word				*
 *				---------------				*
 *									*
 ************************************************************************/

unsigned count_ones_word (word_val)
unsigned word_val;
{
   return count_ones_nibble  (word_val & 0x0000000F) + 
	  count_ones_nibble ((word_val & 0x000000F0) >>  4) + 
	  count_ones_nibble ((word_val & 0x00000F00) >>  8) + 
	  count_ones_nibble ((word_val & 0x0000F000) >> 12) + 
	  count_ones_nibble ((word_val & 0x000F0000) >> 16) + 
	  count_ones_nibble ((word_val & 0x00F00000) >> 20) + 
	  count_ones_nibble ((word_val & 0x0F000000) >> 24) + 
	  count_ones_nibble ((word_val & 0xF0000000) >> 28);
}

/************************************************************************
 *									*
 *				count_ones_byte				*
 *				---------------				*
 *									*
 ************************************************************************/

unsigned count_ones_byte (byte_val)
unsigned char byte_val;
{
   assert (byte_val < 256);

   return count_ones_nibble  (byte_val & 0x0F) + 
	  count_ones_nibble ((byte_val & 0xF0) >> 4);
}

/************************************************************************
 *									*
 *				count_ones_nibble			*
 *				-----------------			*
 *									*
 ************************************************************************/

unsigned count_ones_nibble (nibble_val)
unsigned nibble_val;
{
   assert (nibble_val < 16);

   switch (nibble_val) {
      case 0:	return 0;
      case 1:	return 1;
      case 2:	return 1;
      case 3:	return 2;
      case 4:	return 1;
      case 5:	return 2;
      case 6:	return 2;
      case 7:	return 3;
      case 8:	return 1;
      case 9:	return 2;
      case 10:	return 2;
      case 11:	return 3;
      case 12:	return 2;
      case 13:	return 3;
      case 14:	return 3;
      case 15:	return 4;
   }
}

/************************************************************************
 *									*
 *				log2					*
 *				----					*
 *									*
 * If "pow_of_2" is an integer power of 2, log2 (pow_of_2) is returned.	*
 * Otherwise an error is dumped and the program exit fails.		*
 *									*
 ************************************************************************/

unsigned log2 (pow_of_2)
unsigned pow_of_2;
{
   switch (pow_of_2) {
      case 0x00000001:	return 0;
      case 0x00000002:	return 1;
      case 0x00000004:	return 2;
      case 0x00000008:	return 3;
      case 0x00000010:	return 4;
      case 0x00000020:	return 5;
      case 0x00000040:	return 6;
      case 0x00000080:	return 7;
      case 0x00000100:	return 8;
      case 0x00000200:	return 9;
      case 0x00000400:	return 10;
      case 0x00000800:	return 11;
      case 0x00001000:	return 12;
      case 0x00002000:	return 13;
      case 0x00004000:	return 14;
      case 0x00008000:	return 15;
      case 0x00010000:	return 16;
      case 0x00020000:	return 17;
      case 0x00040000:	return 18;
      case 0x00080000:	return 19;
      case 0x00100000:	return 20;
      case 0x00200000:	return 21;
      case 0x00400000:	return 22;
      case 0x00800000:	return 23;
      case 0x01000000:	return 24;
      case 0x02000000:	return 25;
      case 0x04000000:	return 26;
      case 0x08000000:	return 27;
      case 0x10000000:	return 28;
      case 0x20000000:	return 29;
      case 0x40000000:	return 30;
      case 0x80000000:	return 31;

      default:
	 fprintf (stderr, "Non power of 2 (%u) passed to \"log2\"\n",pow_of_2);
	 exit (1);
   }
}

/************************************************************************
 *									*
 *				fdprintf				*
 *				--------				*
 *									*
 * A very simplified version of fprintf, but that takes a file		*
 * descriptor instead of a file stream pointer.  There can be		*
 * at most 3 arguments, and they must reside in GPR's.			*
 *									*
 ************************************************************************/

int fdprintf (fd, fmt_string, arg1, arg2, arg3)
int  fd;
char *fmt_string;
int  arg1;
int  arg2;
int  arg3;
{
   int  len;
   char buff[1024];

   sprintf (buff, fmt_string, arg1, arg2, arg3);
   len = strlen (buff);
   if (write (fd, buff, len) != len) assert (1 == 0);
   else return len;
}
