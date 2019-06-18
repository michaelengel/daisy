/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#define MAX_ADDR			4095
#define NUM_CACHE_FILES			   8

#define CACHE_SLEEP_TIME	      100000		/* Microseconds */

#define NUM_PTRS_TO_FREE		32   /* Be safe, only 19 currently */
					     /* 19 = 2 * NUM_CACHES + 1	   */

#define MAX_LOAD_STORE			     8	/* In one VLIW */
#define LOAD_STORE_COMBIN	(((MAX_LOAD_STORE+1)*(MAX_LOAD_STORE+2))/2)

typedef struct {
   unsigned addr;
   unsigned type;
} ADDR_ENTRY;

typedef struct {
   unsigned   num_addr;
   int	      pad;
   ADDR_ENTRY list[MAX_ADDR];
} FILEHDR;

typedef struct dirent {
   int      vaddr;
   unsigned cnt;
   short    lru;
   short    dirty;
} DIRENT;

typedef struct {
   DIRENT   **directory;
   unsigned latency;
   unsigned cachesz;
   unsigned linesz;
   unsigned assoc;
   unsigned log2_linesz;
   unsigned set_mask;
   unsigned num_sets;
   unsigned log2_num_sets;
   unsigned r_hits;
   unsigned w_hits;
   unsigned t_hits;
   unsigned r_misses;
   unsigned w_misses;
   unsigned t_misses;
   unsigned castouts;
   unsigned ins_castouts;
   unsigned ins_r_misses;
   unsigned ins_w_misses;
   unsigned ins_t_misses;
   int 	    cacheflag;
   unsigned cachestattab[MAX_LOAD_STORE+1][MAX_LOAD_STORE+1];
} CACHE_INFO;

unsigned *find_data  (unsigned *, unsigned, int);
unsigned *find_instr (unsigned *, unsigned);
