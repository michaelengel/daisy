/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/************************************************************************
  init_caches			Initialization

  find_data			Actual Simulation
  find_instr
  stats_per_instr

  prnt_cachestat		Results

  flush_caches			Not currently done

  clear_cache			Internal Routines
  searchcache
  prnt_cache_hit_miss

************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cache.h"
#include "cacheconst.h"

/* The actual memory size where keep track of lines accessed */
#define VMEMSIZE		0x30000000	/* Get PPC text and data */
#define DIRMEM_SIZE	        0x200000	/* 2 Mbytes */

static char dir_mem[DIRMEM_SIZE];
static char *pdir_mem = dir_mem;

/* # of 4-byte words so that there is 1 bit for each of the */
/* 1M pages in a 4G page space				    */
#define WORDS_WITH_4G_PAGESPACE	32768

extern int        do_cache_simul;
extern int        do_cache_trace;
extern int	  main_mem_latency;

extern CACHE_INFO cache_info[];
extern CACHE_INFO tlb_info[];
static CACHE_INFO *cache;
static CACHE_INFO *tlb;

static int   dcsize0;
static int   workset[WORDS_WITH_4G_PAGESPACE];

static int   addressing_error = 0;

static int   clear = FLUSHCYCLE;
static char  ch[3] = {'d','j','i'};

static FILE  *simout;

static void addrerr (unsigned, int);
static void clear_cache (CACHE_INFO *, int);
static double prnt_cache_hit_miss (CACHE_INFO *, int, int, int);
static void prnt_cachestat_internal (CACHE_INFO *, char *);
static unsigned get_main_mem_rds (CACHE_INFO *);
static void *dir_alloc (int);
unsigned *stats_per_instr (CACHE_INFO *, unsigned *);

/************************************************************************
 *									*
 *				init_caches				*
 *				-----------				*
 *									*
 ************************************************************************/

init_caches ()
{
   cache = cache_info;		/* Set up our static private values */
   tlb	 = tlb_info;

   init_caches_internal (cache);
   init_caches_internal (tlb);
}

/************************************************************************
 *									*
 *				init_caches_internal			*
 *				--------------------			*
 *									*
 ************************************************************************/

static init_caches_internal (cb)
CACHE_INFO *cb;
{
   int cnum;
   int ctype;
   int clevel;
   int base_num;

   /* Set cache flags */
   for (cnum = 0; cnum < NUM_CACHES; cnum++)
      if (cb[cnum].cachesz) cb[cnum].cacheflag = FOUND;

   cnum = 0;
   for (clevel = 0; clevel < NUM_CACHE_LEVELS; clevel++) {
      for (ctype = 0; ctype < NUM_CACHE_TYPES; ctype++) {
	 if (cb[cnum].cacheflag) init_cache_areas (cb, cnum, clevel, ctype);
	 cnum++;
      }
   }
}

/************************************************************************
 *									*
 *				init_cache_areas			*
 *				----------------			*
 *									*
 ************************************************************************/

static init_cache_areas (cb, cnum, clevel, ctype)
CACHE_INFO *cb;
int	   cnum;
int	   clevel;
int	   ctype;
{
   int	         i;
   int	         dirspace_size;
   int	         dir_size;
   struct dirent *dirspace;	 /* Ptr to the space of directory entries */
   CACHE_INFO    *c = &cb[cnum];

#ifdef DB_CACHESTAT
   printf ("%ccachesz%d=%d bytes %clinesz%d=%d bytes %cassoc%d=%d way\n",
	   ch[ctype], clevel, c->cachesz,
	   ch[ctype], clevel, c->linesz,
	   ch[ctype], clevel, c->assoc);
#endif

   if (c->assoc * c->linesz > c->cachesz) {

      fprintf (stderr, "%cassoc%d * %clinesz%d > %ccachesz%d?\n",
	       ch[ctype], clevel, ch[ctype], clevel, ch[ctype], clevel);
      exit(1);
   }

   c->log2_linesz = whichpow2 (c->linesz);
   c->num_sets = (c->cachesz / c->assoc) >> c->log2_linesz;
   c->log2_num_sets = whichpow2 (c->num_sets);
   c->set_mask = c->num_sets - 1;

   dirspace_size = c->num_sets * c->assoc * sizeof(dirspace[0]);
   dir_size      = c->num_sets *            sizeof(c->directory[0]);

   dirspace     = (struct dirent *)  dir_alloc (dirspace_size);
   c->directory = (struct dirent **) dir_alloc (dir_size);

   /* Make each entry of directory point to a different DIRENT array,
      where the number of entries in each array is the associativity */
   for (i = 0; i < c->num_sets; i++)
      c->directory[i] = &dirspace[i * c->assoc];

   /* Initialize cache directory to all invalid entries */
   clear_cache (cb, cnum);
}

/************************************************************************
 *									*
 *				dir_alloc				*
 *				---------				*
 *									*
 ************************************************************************/

static void *dir_alloc (size)
int size;
{
   void *rval;

   /* If assert fails, make DIRMEM_SIZE bigger, recompile, and run again */
   if (&dir_mem[DIRMEM_SIZE] - pdir_mem < size) assert (1 == 0);
   else {
      rval     = pdir_mem;
      pdir_mem += size;

      return rval;
   }
}

/************************************************************************
 *									*
 *				clear_cache				*
 *				-----------				*
 *									*
 * Initializes cache directory to all invalid entries.			*
 *									*
 ************************************************************************/

static void clear_cache (cb, cnum)
CACHE_INFO *cb;
int	   cnum;
{
   int		 i,j;
   struct dirent *set;
   CACHE_INFO	 *c = &cb[cnum];

   for (i = 0; i < c->num_sets; i++) {
      set = c->directory[i];

      for (j = 0; j < c->assoc; j++) {
	 set[j].vaddr = INVALID;
	 set[j].dirty = FALSE;
	 set[j].lru = 0;
      }
   }
}

/************************************************************************
 *									*
 *				flush_caches				*
 *				------------				*
 *									*
 * Clears all caches.							*
 *									*
 ************************************************************************/

void flush_caches ()
{
   int		 cnum;

   for (cnum = 0; cnum < NUM_CACHES; cnum++)
      if (cache[cnum].cacheflag) clear_cache (cache, cnum);
}

/************************************************************************
 *									*
 *				flush_tlbs				*
 *				----------				*
 *									*
 * Clears all tlbs.							*
 *									*
 ************************************************************************/

void flush_tlbs ()
{
   int		 cnum;

   for (cnum = 0; cnum < NUM_CACHES; cnum++)
      if (tlb[cnum].cacheflag) clear_cache (tlb, cnum);
}

/************************************************************************
 *									*
 *				find_data				*
 *				---------				*
 *									*
 * This finds data by calling either data or joint caches		*
 * and updates the workset - called for every LOAD/STORE.		*
 *									*
 ************************************************************************/

unsigned *find_data (vliw_rtn_pt, addr, accesstyp)
unsigned *vliw_rtn_pt;
unsigned addr;
int	 accesstyp;
{

#ifdef PTRACE
   addr -= DATASEGM_START;
#endif

   if (do_cache_trace) {
      trace_dump (addr, accesstyp);
      if (!do_cache_simul) return vliw_rtn_pt;
   }

   if (addr >= VMEMSIZE) addrerr (addr,accesstyp);

   assert (LOG2_WKSETLSZ == 12);	/* 1 page for full 4G addr space */
   SET_BIT (workset, addr >> LOG2_WKSETLSZ);

   find_data_internal (cache, addr, accesstyp);
   find_data_internal (tlb,   addr, CLOAD);

   return vliw_rtn_pt;
}

/************************************************************************
 *									*
 *				find_data_internal			*
 *				------------------			*
 *									*
 ************************************************************************/

static find_data_internal (cb_in, addr, accesstyp)
CACHE_INFO *cb_in;
unsigned   addr;
int	   accesstyp;
{
   int	      clevel = 0;
   int	      found  = FALSE;
   CACHE_INFO *cb = cb_in;

   do {
      if (cb[DATA].cacheflag)
	 found = searchcache (cb_in, addr, accesstyp, clevel, DATA);

      else if (cb[JOINT].cacheflag)
	 found = searchcache (cb_in, addr, accesstyp, clevel, JOINT);

      cb += NUM_CACHE_TYPES;
      clevel++;
   } while (!found && clevel < NUM_CACHE_LEVELS);
}


/************************************************************************
 *									*
 *				find_instr				*
 *				----------				*
 *									*
 * This finds an instruction by calling either instruction or		*
 * joint caches - called once per instruction call.			*
 *									*
 ************************************************************************/

unsigned *find_instr (vliw_rtn_pt, addr)
unsigned *vliw_rtn_pt;
unsigned addr;
{
   if (do_cache_trace) {
      trace_dump (addr, INSFETCH);
      if (!do_cache_simul) return vliw_rtn_pt;
   }

   find_instr_internal (cache, addr);
   find_instr_internal (tlb,   addr);

   return vliw_rtn_pt;
}

/************************************************************************
 *									*
 *				find_instr_internal			*
 *				-------------------			*
 *									*
 ************************************************************************/

static find_instr_internal (cb_in, addr)
CACHE_INFO *cb_in;
unsigned   addr;
{
   int	      clevel = 0;
   int	      found = FALSE;
   CACHE_INFO *cb = cb_in;

   stats_per_instr (cb_in, 0);

   do {
      if (cb[INSTR].cacheflag)
	 found = searchcache (cb_in, addr, INSFETCH, clevel, INSTR);

      else if (cb[JOINT].cacheflag)
	 found = searchcache (cb_in, addr, INSFETCH, clevel, JOINT);

      cb += NUM_CACHE_TYPES;
      clevel++;
   } while (!found && clevel < NUM_CACHE_LEVELS);
}

/************************************************************************
 *									*
 *				searchcache				*
 *				-----------				*
 *									*
 * This routine searches a cache for the required information,		*
 * returning TRUE if the value is found, FALSE otherwise, and updating	*
 * cache statistics along the way.  Called once per memory operation.	*
 *									*
 ************************************************************************/

static int searchcache (cb, addr, accesstyp, clevel, ctype)
CACHE_INFO *cb;
unsigned   addr;
int	   accesstyp;
int	   clevel;
int	   ctype;
{
    int		  set_num;
    struct dirent *set;
    int		  tag;
    int		  i, j;
    int		  lruent;
    int		  lruval;
    int		  cnum;
    int		  l_assoc;
    CACHE_INFO	  *c;

    cnum = clevel * NUM_CACHE_TYPES + ctype;
    c = &cb[cnum];
    l_assoc = c->assoc;

#ifdef DB_CACHESTAT
    printf ("searchcache input: ctype=%c cache#=%d addr=%08x accesstype=%d\n",
	    ch[ctype],clevel,addr,accesstyp);
#endif

   addr >>= c->log2_linesz;
   set_num = addr & c->set_mask;
   tag = addr >> c->log2_num_sets;

   set = c->directory[set_num];

#ifdef DB_CACHESTAT
   for (i = 0; i < l_assoc; i++)
      printf ("  %cdirectory%d[%d][%d].vaddr=%08x lru=%08x dirty=%d\n",
	      ch[ctype], clevel, set_num, i,
	      set[i].vaddr, set[i].lru, set[i].dirty);
#endif

   lruval = 0x7fffffff; /* to find min value of lru's */

   for (i = 0; i < l_assoc; i++)  {

      if (set[i].vaddr == tag)  {
	 /* Update number of hits */
	 if (accesstyp & READ_OP) c->r_hits++;
	 else if (accesstyp == CSTORE) {
	    c->w_hits++;
	    set[i].dirty = TRUE;	    /* If STORE, mark line dirty */
	 }
	 else if (accesstyp == CTOUCH) c->t_hits++;

	 if (set[i].lru != (1 << l_assoc)) {
	    /* Hit entry is not Most Recently Used */
	    /* Shift lru information to the right */
	    for (j=0; j < l_assoc; j++) {
	       set[j].lru = ((unsigned) set[j].lru) >> 1;
	    }
	    /* Mark hit entry as Most Recently Used */
	    set[i].lru = (1 << l_assoc);
	 }

#ifdef DB_CACHESTAT
	 printf ("  %ccache%d hit occurred:\n", ch[ctype], clevel);

	 for (j = 0; j < l_assoc; j++)
	    printf ("  %cdirectory%d[%d][%d].vaddr=%08x lru=%08x dirty=%d\n",
		    ch[ctype], clevel, set_num, j,
		    set[j].vaddr, set[j].lru, set[j].dirty);

	 printf ("  r_hits=%u w_hits=%u t_hits=%u\n",
		 c->r_hits, c->w_hits, c->t_hits);
#endif

	 /* Return control to find_data or find_instr */
	 return TRUE;
      }

      /* Find Least Recently Used entry (in case there is a miss) */
      if (lruval > set[i].lru) {
	 lruval = set[i].lru;
	 lruent = i;
      }
   }

   /* Update castouts, miss count */
   if (set[lruent].dirty)   { c->castouts++;     c->ins_castouts++;}

   if      (accesstyp & READ_OP) { c->r_misses++;     c->ins_r_misses++;}
   else if (accesstyp == CSTORE) { c->w_misses++;     c->ins_w_misses++;}
   else				 { c->t_misses++;     c->ins_t_misses++;}

   /* Shift lru information to the right */
   for (j=0; j < l_assoc; j++) {
      set[j].lru = ((unsigned) set[j].lru) >> 1;
   }

   /* Update least recently used member of this set */
   set[lruent].vaddr = tag;
   set[lruent].lru = (1 << l_assoc);
   set[lruent].dirty = (accesstyp == CSTORE);

#ifdef DB_CACHESTAT
   printf ("  %ccache%d miss occurred, after replacement:\n",
	   ch[ctype], clevel);

   printf("  lruent=%d\n", lruent);

   for (i = 0; i < l_assoc; i++)
      printf ("  %cdirectory%d[%d][%d].vaddr=%08x lru=%08x dirty=%d\n",
	      ch[ctype], clevel, set_num, i,
	      set[i].vaddr, set[i].lru, set[i].dirty);

   printf ("  r_misses=%u w_misses=%u t_misses=%u castouts=%u\n",
	   c->r_misses, c->w_misses, c->t_misses, c->castouts);
#endif

   /* Return control to find_data or find_instr */
   return FALSE;
}

/************************************************************************
 *									*
 *				stats_per_instr				*
 *				---------------				*
 *									*
 * This routine will enter the number of read misses, write misses	*
 * and castouts that occurred during this instruction in a table and	*
 * reset the per-instruction cache miss counters to 0.			*
 *									*
 * Called after each instruction.					*
 *									*
 ************************************************************************/

unsigned *stats_per_instr (cb_in, vliw_rtn_pt)
CACHE_INFO *cb_in;
unsigned   *vliw_rtn_pt;
{
   stats_per_instr_internal (cb_in);

   return vliw_rtn_pt;
}

/************************************************************************
 *									*
 *				stats_per_instr_internal		*
 *				------------------------		*
 *									*
 ************************************************************************/

static stats_per_instr_internal (cb_in)
CACHE_INFO *cb_in;
{
   int	      i;
   int	      cnum;
   CACHE_INFO *c;

   for (cnum = 0; cnum < NUM_CACHES; cnum++) {
      c = &cb_in[cnum];

      if (c->cacheflag) {

	 c->cachestattab[c->ins_r_misses][c->ins_w_misses]++;

	 c->ins_castouts = 0;
	 c->ins_r_misses = 0;
	 c->ins_w_misses = 0;
	 c->ins_t_misses = 0;
      }
   }
}

/************************************************************************
 *									*
 *				prnt_cachestat				*
 *				--------------				*
 *									*
 * This subroutine will print cache statistics to file simout.		*
 *									*
 ************************************************************************/

void prnt_cachestat ()
{
   prnt_cachestat_internal (cache, "cache.out");
   prnt_cachestat_internal (tlb,     "tlb.out");
}


/************************************************************************
 *									*
 *				prnt_cachestat_internal			*
 *				-----------------------			*
 *									*
 ************************************************************************/

static void prnt_cachestat_internal (cb, name)
CACHE_INFO *cb;
char	   *name;
{
   int      i;
   int      cnum;
   int      ctype;
   int      clevel;
   int      dataflag;
   int	    instrflag;
   unsigned num_main_mem_rds;
   double   rd_latency = 0.0;

   dataflag = NOTFOUND;
   instrflag = NOTFOUND;

   simout = fopen (name, "w");

   if (!simout) {
      fprintf (stderr, "Could not open cache statistics file:  %s\n", name);
      exit (1);
   }

   fprintf (simout,
	    "\n----------------------------------------------------\n\n");

   /* Print out cache hierarchy schematic */
   fprintf (simout, "Cache schematic:\n\n");

   for (clevel = NUM_CACHE_LEVELS-1; clevel>=0; clevel--) {
       fprintf (simout, "Level #%d:     ",clevel);

       for (ctype = 0; ctype < NUM_CACHE_TYPES; ctype++) {
	  if (cb[(clevel*NUM_CACHE_TYPES)+ctype].cacheflag)
	       fprintf (simout,"* ");
	  else fprintf (simout,"0 ");
       }

      fprintf (simout, "\n");

      if (clevel != 0)
	 if (cb[(clevel*NUM_CACHE_TYPES)+JOINT].cacheflag)
	    if (cb[((clevel-1)*NUM_CACHE_TYPES)+JOINT].cacheflag)
		 fprintf (simout, "                |\n");
	    else fprintf (simout, "               / \\\n");

	 else if (cb[((clevel-1)*NUM_CACHE_TYPES)+JOINT].cacheflag)
	      fprintf (simout,"               \\ /\n");
	 else fprintf (simout,"              |   |\n");

   }

   fprintf (simout, "\nCache type:   D J I\n\n\n");

   if (clear != DONTFLUSH)
	fprintf (simout, "Caches were flushed every %d cycles.\n\n",clear);
   else fprintf (simout, "Caches were not flushed.\n\n");

   for (ctype = 0; ctype < NUM_CACHE_TYPES; ctype++) {
      switch (ctype) {

	 case DATA:  fprintf (simout, "Data caches:\n");
		     fprintf (simout, "------------\n\n");
		     break;

	 case JOINT: fprintf (simout, "\n\nJoint caches:\n");
		     fprintf (simout, "-------------\n\n");
		     break;

	 case INSTR: fprintf (simout, "\n\nInstruction caches:\n");
		     fprintf (simout, "-------------------\n\n");
		     break;
      }

      for (clevel = 0; clevel < NUM_CACHE_LEVELS; clevel++) {

	 if (ctype == DATA && !dataflag) {
	    fprintf (simout, "%d references beyond x%08x bytes in data memory\n",
		     addressing_error, VMEMSIZE);

	    fprintf (simout, "A total of %d %d-byte distinct lines referenced\n\n",
		     count_ones (workset, WORDS_WITH_4G_PAGESPACE), WKSETLSZ);

	    dataflag = FOUND;
	 }

	 else if (ctype == INSTR && !instrflag) {
	    fprintf (simout,"For instr caches, one word = %d bytes\n\n",WORDSZ);
	    instrflag = FOUND;
	 }

	 cnum = (clevel*NUM_CACHE_TYPES) + ctype;
	 if (! cb[cnum].cacheflag)
	    fprintf (simout,"%ccache #%d was inactive\n",ch[ctype],clevel);

	 else rd_latency += prnt_cache_hit_miss (cb, cnum, ctype, clevel);
      }
   }

   num_main_mem_rds = get_main_mem_rds (cb);
   rd_latency += ((double) num_main_mem_rds) * (double) main_mem_latency;

   fprintf (simout, "Total Read Latency = %2.0f\n", rd_latency);

   fclose (simout);
}

/************************************************************************
 *									*
 *				prnt_cache_hit_miss			*
 *				-------------------			*
 *									*
 *									*
 ************************************************************************/

static double prnt_cache_hit_miss (cb, cnum, ctype, clevel)
CACHE_INFO *cb;
int	   cnum;
int	   ctype;
int	   clevel;
{
   int	      i;
   int	      index;
   int	      cast;
   int	      sum;
   int	      rd_miss;
   int	      wr_miss;
   double     rd_latency;
   CACHE_INFO *c = &cb[cnum];

   fprintf (simout, "\n%ccache #%d\n", ch[ctype], clevel);
   fprintf (simout, "---------\n\n");
   fprintf (simout, "Cache size = %d bytes\n", c->cachesz);

   fprintf (simout, "Cache line size = %d bytes\n", c->linesz);
   fprintf (simout, "Cache associativity = %d\n", c->assoc);
   fprintf (simout, "Total read hits = %u\n", c->r_hits);
   fprintf (simout, "Total read misses = %u\n", c->r_misses);
   fprintf (simout, "Read miss ratio = %.4f\n",
	   (double) c->r_misses / ((double) c->r_misses + (double) c->r_hits));

   /* Latency of 1 in L0 cache means avail in next cycle, i.e. no delay */
   if (clevel == 0)
        rd_latency = ((double) (c->latency - 1) * (double) c->r_hits);
   else rd_latency = ((double)  c->latency      * (double) c->r_hits);

   fprintf (simout, "Read miss delay cycles = %2.0f\n", rd_latency);

   if (ctype != INSTR) {

      fprintf (simout, "Total write hits = %u\n", c->w_hits);
      fprintf (simout, "Total write misses = %u\n", c->w_misses);
      fprintf (simout, "Write miss ratio = %.4f\n",
	       (double)  c->w_misses /
	       ((double) c->w_misses + (double) c->w_hits));

      fprintf (simout, "Total touch hits = %u\n", c->t_hits);
      fprintf (simout, "Total touch misses = %u\n", c->t_misses);
      fprintf (simout, "Touch miss ratio = %.4f\n",
	       (double)  c->t_misses /
	       ((double) c->t_misses + (double) c->t_hits));

      fprintf (simout, "\n");

      fprintf (simout, "Total hits = %u\n", c->r_hits + c->w_hits + c->t_hits);
      fprintf (simout, "Total misses = %u\n",
			  c->r_misses + c->w_misses + c->t_misses);
      fprintf (simout, "Total castouts = %u\n", c->castouts);
      fprintf (simout, "Miss ratio = %.4f\n",
	       ( (double) c->r_misses +
		 (double) c->w_misses +
		 (double) c->t_misses) /
	       ( (double) c->r_misses + (double) c->r_hits +
		 (double) c->w_misses + (double) c->w_hits +
		 (double) c->t_misses + (double) c->t_hits ) );

      fprintf (simout, "\n");

      fprintf (simout, "Count of instructions with cache misses:\n");
      fprintf (simout, "%8s   %8s   %8s   :  %8s\n",
	       "#rd miss","#wr miss","","count");

      for (rd_miss = 0; rd_miss <= MAX_LOAD_STORE; rd_miss++) {
	 for (wr_miss = 0; wr_miss <= MAX_LOAD_STORE; wr_miss++) {

	    if (c->cachestattab[rd_miss][wr_miss] != 0)
	       fprintf (simout, "%8u   %8u              :  %8u\n",
			rd_miss, wr_miss, c->cachestattab[rd_miss][wr_miss]);
	 }
      }
   } /* End of if */

   fprintf (simout,"\n");

   return rd_latency;
}

/************************************************************************
 *									*
 *				ispow2					*
 *				------					*
 *									*
 * Return 1 iff x is a  power of 2 between 2**m and 2**n		*
 *									*
 ************************************************************************/

static int ispow2 (x, m, n)
{
   int i,k;
   k=1;
   for (i=0; i <  m; i++) k=k*2;
   for (i=m; i <= n; i++) {
      if (k == x) return 1;
      k *= 2;
   }
   return 0;
}

/************************************************************************
 *									*
 *				whichpow2				*
 *				---------				*
 *									*
 * Assuming x=2**i, i >= 0, Returns i (log2(x))				*
 *									*
 ************************************************************************/

static int whichpow2 (x)
unsigned x;
{
   switch (x) {
      case 0x00000001:		return 0;
      case 0x00000002:		return 1;
      case 0x00000004:		return 2;
      case 0x00000008:		return 3;

      case 0x00000010:		return 4;
      case 0x00000020:		return 5;
      case 0x00000040:		return 6;
      case 0x00000080:		return 7;

      case 0x00000100:		return 8;
      case 0x00000200:		return 9;
      case 0x00000400:		return 10;
      case 0x00000800:		return 11;

      case 0x00001000:		return 12;
      case 0x00002000:		return 13;
      case 0x00004000:		return 14;
      case 0x00008000:		return 15;

      case 0x00010000:		return 16;
      case 0x00020000:		return 17;
      case 0x00040000:		return 18;
      case 0x00080000:		return 19;

      case 0x00100000:		return 20;
      case 0x00200000:		return 21;
      case 0x00400000:		return 22;
      case 0x00800000:		return 23;

      case 0x01000000:		return 24;
      case 0x02000000:		return 25;
      case 0x04000000:		return 26;
      case 0x08000000:		return 27;

      case 0x10000000:		return 28;
      case 0x20000000:		return 29;
      case 0x40000000:		return 30;
      case 0x80000000:		return 31;

      default:
	 fprintf (stderr, "Illegal argument to \"whichpow2\"\n");
	 exit (1);
   }
}

/************************************************************************
 *									*
 *				count_ones				*
 *				----------				*
 *									*
 * Returns the number of ones in the bit vector array of n words.	*
 *									*
 ************************************************************************/

static int count_ones (array, n)
int array[];
int n;
{
   int i;
   int j;
   int count = 0;

   for (i=0; i < n; i++) {
      if (array[i]==0) continue;

      for (j = 0; j < 32; j++)
	 if (RETRIEVE_BIT (array, (i<<5)+j)) count++;
   }

   return count;
}

/************************************************************************
 *									*
 *				addrerr					*
 *				-------					*
 *									*
 * Prints a message about data memory address being out of range.	*
 * This is also called when an invalid IO request has been made.	*
 *									*
 ************************************************************************/

static void addrerr (addr, accesstyp)
unsigned addr;
int	 accesstyp;
{
   char *str;

   if	   (accesstyp == CLOAD)    str = "LOAD";
   else if (accesstyp == CSTORE)   str = "STORE";
   else if (accesstyp == CTOUCH)   str = "TOUCH";
   else if (accesstyp == INSFETCH) str = "FETCH";
   addressing_error++;

#ifdef PTRACE
   if (addressing_error <= 100)
      fprintf (simout, "data mem addr(%s) 0x%08x out of range in ins %s\n",
	       str, addr);

   if (addressing_error == 100)
      fprintf (simout, "No more addr out of range messages will be printed\n");
#endif
}

/************************************************************************
 *									*
 *				get_main_mem_rds			*
 *				----------------			*
 *									*
 * Return the number of read references to main memory.  This routine	*
 * will not return the correct value if there are no caches on the	*
 * instruction path from the processor or if  there are no caches on	*
 * the data path from the processor.					*
 *									*
 ************************************************************************/

static unsigned get_main_mem_rds (cb)
CACHE_INFO *cb;
{
   int	    clevel;
   int	    cnum;
   unsigned ic_main_refs;
   unsigned dc_main_refs;

   /*********************************************************************/
   /* Look for JCache furthest from processor in */
   for (clevel = NUM_CACHE_LEVELS - 1; clevel >= 0; clevel--) {
      cnum = (clevel*NUM_CACHE_TYPES) + JOINT;
      if (cb[cnum].cacheflag) break;
   }

   /* If hierarchy has JCache, main mem reads = # of last JCache Misses */
   if (clevel >= 0) return cb[cnum].r_misses;
   /*********************************************************************/

   /*********************************************************************/
   /* Look for ICache furthest from processor in */
   for (clevel = NUM_CACHE_LEVELS - 1; clevel >= 0; clevel--) {
      cnum = (clevel*NUM_CACHE_TYPES) + INSTR;
      if (cb[cnum].cacheflag) break;
   }

   /* If hierarchy has ICache, main mem reads = # of last ICache Misses */
   if (clevel >= 0) ic_main_refs = cb[cnum].r_misses;
   /*********************************************************************/

   /*********************************************************************/
   /* Look for DCache furthest from processor in */
   for (clevel = NUM_CACHE_LEVELS - 1; clevel >= 0; clevel--) {
      cnum = (clevel*NUM_CACHE_TYPES) + DATA;
      if (cb[cnum].cacheflag) break;
   }

   /* If hierarchy has DCache, main mem reads = # of last DCache Misses */
   if (clevel >= 0) dc_main_refs = cb[cnum].r_misses;
   /*********************************************************************/

   return ic_main_refs + dc_main_refs;
}
