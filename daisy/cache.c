/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

/* extern unsigned  xlate_mem[]; */
extern unsigned *xlate_mem;
extern unsigned *xlate_mem_end;

static unsigned dcache_line_size;
static unsigned icache_line_size;

/************************************************************************
 *									*
 *				init_cache				*
 *				----------				*
 *									*
 * OBSOLETE:  No longer needs to be called.				*
 * Must call assembly language routines for this.			*
 *									*
 ************************************************************************/

init_cache ()
{
   dcache_line_size = get_dcache_line_size ();
   dcache_line_size = 64;     /* Frodo has problems with 128 (val returned) */
   icache_line_size = get_icache_line_size ();

   /* Make sure within architecturally specified limits -- see p.129 */
   /* However PowerPC returns 0 byte line sizes to this command,     */
   /* so default to 64.						     */
   if (!(dcache_line_size >= 64  &&  dcache_line_size <= 4096))
      dcache_line_size = 64;

   if (!(icache_line_size >= 64  &&  icache_line_size <= 4096))
      icache_line_size = 64;
}

/************************************************************************
 *									*
 *				flush_cache_old				*
 *				---------------				*
 *									*
 * Make sure that we our newly generated code is placed in icache by	*
 * flushing it out of the dcache.					*
 *									*
 ************************************************************************/

flush_cache_old (start, end)
char *start;
char *end;
{
   char *line;
   char *start_line;
   char *end_line;

   if (start < end) {
      start_line = (char *) (((unsigned) start) & (~(dcache_line_size-1)));
      end_line   = (char *) (((unsigned) end)   & (~(dcache_line_size-1)));
      for (line = start_line; line <= end_line; line += dcache_line_size)
         flush_cache_line (line);
   }
   else {
      start_line = (char *) (((unsigned) start) & (~(dcache_line_size-1)));
      end_line   = (char *) (((unsigned) (xlate_mem_end - 1)) & 
			     (~(dcache_line_size-1)));

      for (line = start_line; line <= end_line; line += dcache_line_size)
         flush_cache_line (line);

      start_line = (char *) (((unsigned) &xlate_mem[0]) &
			     (~(dcache_line_size-1)));
      end_line   = (char *) (((unsigned) end)   & (~(dcache_line_size-1)));

      for (line = start_line; line <= end_line; line += dcache_line_size)
         flush_cache_line (line);
   }

   sync_caches ();
}

/************************************************************************
 *									*
 *				flush_cache				*
 *				-----------				*
 *									*
 ************************************************************************/

flush_cache (start, end)
char *start;
char *end;
{
   char *line;
   char *start_line;
   char *end_line;
   unsigned icache_line_size = 32;   /* Assume cache lines are at least 32 */
				     /* bytes:  Trouble occurs if less     */

   start_line = (char *) (((unsigned) start) & (~(icache_line_size-1)));
   end_line   = (char *) (((unsigned) end)   & (~(icache_line_size-1)));
   for (line = start_line; line <= end_line; line += icache_line_size)
      icache_sync_line (line);

   sync_caches ();
}

