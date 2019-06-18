/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "opcode_fmt.h"

#define LOOPHDR_MEMSIZE		65536

typedef struct _loopmem {
   unsigned char   *mem;
   struct _loopmem *next;
} LOOPMEM;

extern unsigned	     rpage_entries;

       int	     loophdr_bytes_per_path;
       unsigned	     max_open_paths;

static unsigned	     num_open_paths;
static LOOPMEM	     *free_mem;
static LOOPMEM	     loop_list_mem[8*(LOOPHDR_MEMSIZE/MIN_RPAGE_ENTRIES)];
static unsigned char loophdr_mem[LOOPHDR_MEMSIZE];

/************************************************************************
 *									*
 *				init_loophdr_mem			*
 *				----------------			*
 *									*
 ************************************************************************/

init_loophdr_mem ()
{
   int	   i;
   int	   num_mem_paths;
   char	   *p       = loophdr_mem;
   LOOPMEM *listmem = loop_list_mem;

   free_mem		  = loop_list_mem;
   loophdr_bytes_per_path = rpage_entries   / 8;
   num_mem_paths	  = LOOPHDR_MEMSIZE / loophdr_bytes_per_path;

   assert (rpage_entries   % 8			    == 0);
   assert (LOOPHDR_MEMSIZE % loophdr_bytes_per_path == 0);
   assert (LOOPHDR_MEMSIZE % MIN_RPAGESIZE	    == 0);

   for (i = 0; i < num_mem_paths; i++, listmem++, p+=loophdr_bytes_per_path) {
      listmem->mem  = p;
      listmem->next = listmem + 1;
   }

   /* The last element of the list has a NULL "next" */
   listmem--;
   listmem->next = 0;
}

/************************************************************************
 *									*
 *				page_verif_loophdr_mem			*
 *				----------------------			*
 *									*
 * This routine is for debugging purposes only.				*
 *									*
 ************************************************************************/

int page_verif_loophdr_mem ()
{
   int	   i;
   int	   cnt;
   int	   num_mem_paths;
   LOOPMEM *p;

   assert (num_open_paths == 0);

#ifdef DEBUGGING
   cnt		 = 0;
   for (p = free_mem; p; p = p->next, cnt++);

   num_mem_paths = LOOPHDR_MEMSIZE / loophdr_bytes_per_path;
   if (cnt != num_mem_paths) {
      printf ("cnt = %d   num_mem_paths = %d\n", cnt, num_mem_paths);

      for (i = 0, p = free_mem; p; p = p->next, i++) {
         printf (" %3d", (p->mem - loophdr_mem) / loophdr_bytes_per_path);
	 if ((i & 7) == 7) printf ("\n");
      }

      exit (1);
   }
#endif
}

/************************************************************************
 *									*
 *				get_loophdr_mem				*
 *				---------------				*
 *									*
 ************************************************************************/

unsigned char *get_loophdr_mem ()
{
   unsigned char *rval;

   /* If assert fails, LOOPHDR_MEMSIZE should be increased */
   assert (free_mem);

   rval     = free_mem->mem;
   free_mem = free_mem->next;

   if (++num_open_paths > max_open_paths) max_open_paths = num_open_paths;

   return rval;
}

/************************************************************************
 *									*
 *				free_loophdr_mem			*
 *				----------------			*
 *									*
 * Make "mem" available for another path to use, and clear it so that	*
 * no location on the path is yet visited.  As each path closes, e.g.	*
 * via an OFFPAGE branch, this routine should be invoked.  This being	*
 * the case, no special cleanup is needed at the start of page		*
 * processing.								*
 *									*
 ************************************************************************/

free_loophdr_mem (mem)
unsigned char *mem;
{
   int index = (mem - loophdr_mem) / loophdr_bytes_per_path;

   assert (loop_list_mem[index].mem == mem);

   mem_clear (mem, loophdr_bytes_per_path);

   loop_list_mem[index].next = free_mem;
   free_mem = &loop_list_mem[index];

   num_open_paths--;
}
