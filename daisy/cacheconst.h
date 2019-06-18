/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#define READ_OP				0x80

#define CSTORE				(0)
#define CLOAD				(1|READ_OP)
#define FINAL_CACHE_ADDR		(2)
#define INSFETCH			(3|READ_OP)
#define CTOUCH				(4)

#define DATA				0
#define JOINT				1
#define INSTR				2
#define NUM_CACHE_TYPES			3
#define MAINMEM				3
 
#define NUM_CACHE_LEVELS		4
#define NUM_CACHES			(NUM_CACHE_LEVELS*NUM_CACHE_TYPES)

#define NOTFOUND 0
#define FOUND 1
 
#define CLEAN 0
#define DIRTY 1
 
#define INVALID -1
 
#define DONTFLUSH 0
 
#define WORDSZ		  64	/* Default size of one VLIW word in bytes */
 
#define WKSETLSZ	4096	/* Default line size of the work set: */
#define LOG2_WKSETLSZ	  12	/* 1 page, was 64 bytes		      */
 
#define FLUSHCYCLE DONTFLUSH  /* default # of cycles per cache flush */
 
#define DATASEGM_START 0x20000000

#define DONE_BYTE			('D')
#define RUNNING_BYTE			('R')

/* Need the size of minimal stack frame for initializing stack
   used when calling cache routines.
*/
#define MIN_STACK_FRAME_WORDS		13

/* Generally useful macro and constant definitions */
#define RETRIEVE_BIT(x,y)  (((x)[(y)>>5] >> (31-((y)&31))) & 1)

#define SET_BIT(x,y) { ((x)[(y)>>5]) |= (1 << (31-((y)&31))) ; }
