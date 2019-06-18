/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "stats.h"
#include "hash.h"

/* 2^(32-12-8) = 2^17 = 0x20000:
   2^32 bytes of memory
   2^12 bytes per page
   2^3  bits  per byte
*/
#define MAX_BYTES_OF_PAGES		(2<<(32-MIN_RPAGESIZE_LOG-3))
#define MAX_WORDS_OF_PAGES		(2<<(32-MIN_RPAGESIZE_LOG-5))

/*     HASH_ENTRY    *hash_table[HASH_TABLE_SIZE];  */
       HASH_ENTRY    **hash_table;
static HASH_ENTRY    *hash_mem_ptr;
/* static HASH_ENTRY  hash_mem[HASH_TABLE_ENTRIES]; */
static HASH_ENTRY    *hash_mem;
static HASH_ENTRY    *hash_valid;
/* static unsigned    page_ref[MAX_WORDS_OF_PAGES]; */
static unsigned	     *page_ref;
static unsigned	     num_bytes_of_pages;
static unsigned	     num_words_of_pages;

       unsigned	     num_duplic_groups;

extern unsigned      rpagesize_log;
extern unsigned      xlate_entry_raw_ptr;
extern unsigned      *prog_start;
extern unsigned      *prog_end;
extern int	     recompiling;

unsigned gen_mask (unsigned, unsigned);

/************************************************************************
 *									*
 *				init_hash				*
 *				---------				*
 *									*
 * Setup address of actual "xlate_entry_raw" routine (not descriptor).	*
 * for easy access by "get_xlated_addr".				*
 *									*
 ************************************************************************/

init_hash ()
{
   hash_table = get_obj_mem (HASH_TABLE_SIZE    * sizeof (hash_table[0]));
   hash_mem   = get_obj_mem (HASH_TABLE_ENTRIES * sizeof (hash_mem[0]));
   page_ref   = get_obj_mem (MAX_WORDS_OF_PAGES * sizeof (page_ref[0]));

   num_duplic_groups = 0;
   hash_mem_ptr = hash_mem;
   hash_valid   = hash_mem;
   num_bytes_of_pages = (2<<(32-rpagesize_log-3));
   num_words_of_pages = (2<<(32-rpagesize_log-5));
}

/************************************************************************
 *									*
 *				clr_all_hash_entries			*
 *				--------------------			*
 *									*
 * This routine is useful if we want to discard all previously		*
 * translated code and begin afresh.					*
 *									*
 ************************************************************************/

clr_all_hash_entries ()
{
   int i;

   hash_mem_ptr = hash_mem;
   for (i = 0; i < HASH_TABLE_SIZE; i++)
      hash_table[i] = 0;
}

/************************************************************************
 *									*
 *				get_num_hash_entries			*
 *				--------------------			*
 *									*
 ************************************************************************/

unsigned get_num_hash_entries ()
{
   return hash_mem_ptr - hash_mem;
}

/************************************************************************
 *									*
 *				get_hashval				*
 *				-----------				*
 *									*
 * Shift the address right by 2 bits since the two LSB's are zero.	*
 *									*
 ************************************************************************/

static unsigned get_hashval (addr)
unsigned addr;
{
   return (addr >> 2) & (HASH_TABLE_SIZE-1);
}

/************************************************************************
 *									*
 *				add_hash_entry				*
 *				--------------				*
 *									*
 * Put addr in the hash table so that code can know where to jump.	*
 *									*
 ************************************************************************/

void add_hash_entry (addr, xlated)
unsigned addr;
unsigned xlated;
{
   unsigned   hashval;
   HASH_ENTRY *entry;
   HASH_ENTRY *prev_entry;

   hashval = (addr >> 2) & (HASH_TABLE_SIZE-1);
   entry = hash_table[hashval];

   if (!entry) {
      assert (hash_mem_ptr < &hash_mem[HASH_TABLE_ENTRIES]);
      entry = hash_mem_ptr++;
      hash_table[hashval] = entry;
   }
   else {
      do {
	 if (entry->addr == addr) {
            /* Should have some way to reclaim the old "xlated" memory */
	    if (recompiling) { entry->xlated = xlated;    return; }
	    else {
	    /* This can happen legitimately at a serial join point that is
	       reached from multiple invocations of "xlate_entry".  For
	       example if "foo1" and "foo2" both call "foobar", but
	       "foo2" is not directly reachable via an onpage path from
	       "foo1", then this warning will be issued for the second
		instance of "foobar".  Just throw away the useless code
		in this case by returning without doing anything.
	    */
#ifdef WANT_WARNINGS_DISPLAYED
	       fprintf (stderr, "WARNING:  Attempt to add duplicate entry (0x%08x) to hash table\n",
			addr);
#endif
	       num_duplic_groups++;
	       return;
	    }
	 }
	 prev_entry = entry;
      } while (entry = entry->next);

      assert (hash_mem_ptr < &hash_mem[HASH_TABLE_ENTRIES]);
      prev_entry->next = hash_mem_ptr;
      entry = hash_mem_ptr++;
   }

   entry->addr   = addr;
   entry->xlated = xlated;
/* entry->next   = 0;		/* Redundant since initialized this way */
}

/************************************************************************
 *									*
 *				invalidate_hash_castouts		*
 *				------------------------		*
 *									*
 * If we are reusing simulation memory, invalidate any hash table	*
 * entries that point to that memory.  Then bump the "hash_valid"	*
 * ptr to the next element of the "hash_mem" array.			*
 *									*
 ************************************************************************/

invalidate_hash_castouts (xlate_mem_ptr)
unsigned *xlate_mem_ptr;
{
   if (xlate_mem_ptr == hash_valid->xlated) {
      hash_valid->addr |= 1;		/* No instruc address has LSB set */
      hash_valid++;
   }
}

/************************************************************************
 *									*
 *				get_xlated_addr				*
 *				---------------				*
 *									*
 * This routine is actually inlined in the code when jumping to an	*
 * offpage location, but is used by the "in_atexit" routine and		*
 * possibly elsewhere.							*
 *									*
 * RETURNS:  The address of the translated code for "addr" (in the	*
 *	     original code, or if the code has not yet been translated,	*
 *	     returns "xlate_entry_raw_ptr".				*
 *									*
 ************************************************************************/

unsigned get_xlated_addr (addr)
unsigned addr;
{
   unsigned   hashval;
   HASH_ENTRY *entry;

   hashval = (addr >> 2) & (HASH_TABLE_SIZE-1);
   entry = hash_table[hashval];

   if (!entry) return xlate_entry_raw_ptr;

   do 
      if (entry->addr == addr) break;
   while (entry = entry->next);

   if (entry) return entry->xlated;
   else return xlate_entry_raw_ptr;
}

/************************************************************************
 *									*
 *				mark_visited_pages			*
 *				------------------			*
 *									*
 * Mark a bit vector with a 1 at each page accessed.			*
 *									*
 ************************************************************************/

mark_visited_pages ()
{
   unsigned	 i;
   unsigned	 bit;
   unsigned      e_points = hash_mem_ptr - hash_mem;

   for (i = 0; i < e_points; i++) {
      bit = hash_mem[i].addr >> rpagesize_log;
      set_bit (page_ref, bit);
   }
}

/************************************************************************
 *									*
 *				count_num_pages				*
 *				---------------				*
 *									*
 * RETURNS:  The number of pages accessed in the hash table.		*
 *									*
 ************************************************************************/

unsigned count_num_pages ()
{
   return count_ones_vector (page_ref, num_words_of_pages);
}

/************************************************************************
 *									*
 *				count_num_user_pages			*
 *				--------------------			*
 *									*
 * RETURNS:  The number of pages accessed in the hash table.		*
 *									*
 ************************************************************************/

unsigned count_num_user_pages ()
{
   unsigned mask;
   unsigned start_mod;
   unsigned end_mod;
   unsigned start_cnt;
   unsigned mid_cnt;
   unsigned end_cnt;
   unsigned start_page	    = ((unsigned) prog_start) >> rpagesize_log;
   unsigned end_page	    = ((unsigned) prog_end)   >> rpagesize_log;
   unsigned start_page_word = start_page >> 5;
   unsigned end_page_word   = end_page   >> 5;

   switch (end_page_word - start_page_word) {
      case 0:
	 start_mod = start_page & 0x1F;
	 end_mod   = end_page   & 0x1F;
	 assert (end_mod >= start_mod);
	 mask      = gen_mask (31 - end_mod, 31 - start_mod);
	 return count_ones_word (page_ref[start_page_word] & mask);

      case 1:
	 start_mod = start_page & 0x1F;
	 mask      = gen_mask (0, 31 - start_mod);
	 start_cnt = count_ones_word (page_ref[start_page_word] & mask);
	 end_mod   = end_page & 0x1F;
	 mask      = gen_mask (31 - end_mod, 31);
	 end_cnt   = count_ones_word (page_ref[end_page_word]   & mask);
         return start_cnt + end_cnt;

      default:
	 start_mod = start_page & 0x1F;
	 mask      = gen_mask (0, 31 - start_mod);
	 start_cnt = count_ones_word (page_ref[start_page_word] & mask);
	 end_mod   = end_page & 0x1F;
	 mask      = gen_mask (31 - end_mod, 31);
	 end_cnt   = count_ones_word (page_ref[end_page_word]   & mask);
	 mid_cnt   = count_ones_vector (&page_ref[start_page_word+1],
					end_page_word - start_page_word - 1);
	 return start_cnt + mid_cnt + end_cnt;
   }
}

/************************************************************************
 *									*
 *				max_multi_hash				*
 *				--------------				*
 *									*
 * RETURNS:  The maximum number entry points mapped to any one hash	*
 *	     value.							*
 *									*
 ************************************************************************/

unsigned max_multi_hash (pmax_index, pnum_single)
unsigned *pmax_index;
unsigned *pnum_single;
{
   unsigned   i;
   unsigned   curr_multi;
   unsigned   num_single = 0;
   unsigned   max_multi  = 0;
   unsigned   max_multi_index;
   HASH_ENTRY *entry;

   for (i = 0; i < HASH_TABLE_SIZE; i++) {
      
      entry = hash_table[i];

      if (!entry) continue;

      curr_multi = 0;
      do curr_multi++;
      while (entry = entry->next);

      if (curr_multi == 1) num_single++;

      if (curr_multi > max_multi) {
	 max_multi	 = curr_multi;
	 max_multi_index = i;
      }
   }

   *pmax_index  = max_multi_index;
   *pnum_single = num_single;

   return max_multi;
}

/************************************************************************
 *									*
 *				hash_reexec_info			*
 *				----------------			*
 *									*
 * Should we include page_ref?						*
 *									*
 ************************************************************************/

hash_reexec_info (fd, fcn)
int fd;
int (*fcn) ();			/* read or write */
{
   int mem_size;
   int table_size = HASH_TABLE_SIZE    * sizeof (hash_table[0]);

   assert (fcn (fd, hash_table, table_size) == table_size);
   assert (fcn (fd, &hash_mem_ptr, sizeof (hash_mem_ptr)) == 
				   sizeof (hash_mem_ptr));
   mem_size   = (hash_mem_ptr - hash_mem) * sizeof (hash_mem[0]);
   assert (fcn (fd, hash_mem,     mem_size) ==   mem_size);
}

/************************************************************************
 *									*
 *				hash_reexec_fix_addr			*
 *				--------------------			*
 *									*
 * OBSOLETE:  This does not work because the original code has		*
 *	      hardwired addresses from the original location for	*
 *	      cross-page branches.					*
 *									*
 ************************************************************************/

hash_reexec_fix_addr (diff)
int diff;			/* Curr prog_start - Earlier Run prog_start */
{
   int	      i;
   int	      index;
   int	      table_diff;
   HASH_ENTRY *h;
   HASH_ENTRY *hash_table_copy[HASH_TABLE_SIZE];

   return;

   /* Page offsets should always be the same */
   assert ((diff & 0xFFF) == 0);

   for (h = hash_mem; h < hash_mem_ptr; h++)
      h->addr += diff;

   for (i = 0; i < HASH_TABLE_SIZE; i++)
      hash_table_copy[i] = hash_table[i];

   if (diff >= 0) {
      table_diff = get_hashval (diff);

      for (i = 0; i < HASH_TABLE_SIZE; i++) {
         index = (i + table_diff) & (HASH_TABLE_SIZE-1);
         hash_table[index] = hash_table_copy[i];
      }
   }
   else {
      table_diff = get_hashval (-diff);

      for (i = 0; i < HASH_TABLE_SIZE; i++) {
         index = (i + table_diff) & (HASH_TABLE_SIZE-1);
         hash_table[i] = hash_table_copy[index];
      }
   }
}

/************************************************************************
 *									*
 *				gen_mask				*
 *				--------				*
 *									*
 ************************************************************************/

unsigned gen_mask (begin, end)
unsigned begin;
unsigned end;
{
   unsigned diff;
   unsigned x;

   assert (begin <= end);

   diff   = end - begin + 1;
   x      = ~0;
   x    <<= diff;
   x      = ~x;
   x    <<= 31 - end;

   return x;
}
