/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/* 
   Routines to prevent segmentation violations.  The "zeromem"
   routine should be invoked as the first thing done in "main".
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>

#include "resrc_map.h"
#include "resrc_list.h"
#include "resrc_offset.h"

#ifndef DBG
#define DBG 0
#endif

#ifndef XTNDSEG2
#define XTNDSEG2 0
#endif

extern unsigned char r13_area[];	/* Acc mem-mapped VLIW regs */
extern RESRC_MAP     *resrc_map[];
extern RESRC_MAP     real_resrc;
extern char	     *errno_text[];

/* Global variables used by the exception handler */
static unsigned long segv_count;
static unsigned long segv_iar;
static unsigned long segv_instr;
static unsigned long segv_basereg;

void zeromem_cleanup();

/************************************************************************
 *									*
 *				segv_handler				*
 *				------------				*
 *									*
 * This routine ignores a segmentation violation and continues with the	*
 * next instruction.							*
 *									*
 ************************************************************************/

static void segv_handler (signal, code, scp)
int		  signal;
int		  code;
struct sigcontext *scp;
{
  unsigned int reg;
  unsigned segv_addr;

  /* Remember the instruction and operand address that caused the
     last exception, to print at the end
  */

  segv_iar	= scp->sc_jmpbuf.jmp_context.iar;
  segv_instr	= *(long *) segv_iar;
  segv_addr	= scp->sc_jmpbuf.jmp_context.o_vaddr;
  reg		= (segv_instr >> 16) & 0x1f;
  segv_basereg  = scp->sc_jmpbuf.jmp_context.gpr[reg];

  if (DBG) {
    fprintf (stderr, "segv_handler: signal=%d code=%d\n", signal,code);
    fprintf (stderr, "segv_handler: o_vaddr=0x%08x\n", scp->sc_jmpbuf.jmp_context.o_vaddr);
    fprintf (stderr, "segv_handler: IAR=%08x *IAR=%08x\n",
	     segv_iar, segv_instr);
  }

  /* Make sure the instruction causing the segv was a load */
  switch (segv_instr >> 26) {
    case 32:
    case 33:
    case 34:
    case 35:
    case 40:
    case 41:
    case 42:
    case 43:
    case 46:
    case 48:
    case 49:
    case 50:
    case 51:
    case 58:
      break;

    case 31:
      switch ((segv_instr >> 1) & 0x3ff) {
	case 20:
	case 21:
	case 23:
	case 53:
	case 55:
	case 84:
	case 87:
	case 119:
	case 279:
	case 311:
	case 341:
	case 343:
	case 373:
	case 375:
	case 533:
	case 534:
	case 535:
	case 567:
	case 597:
	case 599:
	case 631:
	case 790:
	  break;

	default:
	  store_segv (scp, segv_iar, segv_instr, reg, segv_basereg);
	  break;
      }

      break;

    default:
      store_segv (scp, segv_iar, segv_instr, reg, segv_basereg);
      break;
  }

  /* Increment iar to point to the next instruction */
  scp->sc_jmpbuf.jmp_context.iar += 4;

  /* Make sure the system does not think we want this (4096-byte) page */
  disclaim (segv_addr & 0xFFFFF000, 4096, ZERO_MEM);	

  /* Count segmentation violations, to print at the end */
  segv_count++;

#ifdef WANT_LIST_OF_SEG_VIOLATIONS_FROM_SPECULATIVE_LOADS
  if ((segv_count & 0x0000ffff) == 0)
    fprintf (stderr, "zeromem: %u segmentation violations\n", segv_count);
#endif
}

/************************************************************************
 *									*
 *				store_segv				*
 *				----------				*
 *									*
 ************************************************************************/

store_segv (scp, segv_iar, segv_instr, reg, segv_basereg)
struct sigcontext *scp;
unsigned	  segv_iar;
unsigned	  segv_instr;
unsigned	  reg;
unsigned	  segv_basereg;
{
  int     i, j;
  int     regnum;
  ulong_t *gpr;
  char    buff[16];

#ifndef WANT_LIST_OF_SEG_VIOLATIONS_FROM_SPECULATIVE_LOADS
  return;
#endif

  gpr = scp->sc_jmpbuf.jmp_context.gpr;

  fprintf (stderr, "\nSeg violation from non-LOAD:  IAR=0x%08x  *IAR=0x%08x\n",
	   segv_iar, segv_instr);

  fprintf (stderr, "RA base reg = r%d,  [r%d] = 0x%08x\n\n",
	   reg, reg, segv_basereg);

  fprintf (stderr, "PPC  Registers:\n---------------\n");

  regnum  = 0;
  buff[0] = 'r';
  buff[3] = 0;
  for (i = 0; i < 8; i++) {
     for (j = 0; j < 4; j++) {
        sprintf (&buff[1], "%d", regnum);
        if (regnum < 10) buff[2] = ' ';
	fprintf (stderr, "  %s = 0x%08x", buff, gpr[regnum++]);
     }
     fprintf (stderr, "\n");
  }

  fprintf (stderr, "\nVLIW Registers:\n---------------\n");

  regnum  = 0;
  buff[0] = 'r';
  buff[3] = 0;
  for (i = 0; i < 16; i++) {
     for (j = 0; j < 4; j++) {
        sprintf (&buff[1], "%d", regnum);
        if (regnum < 10) buff[2] = ' ';

	if (resrc_map[R0 + regnum] == &real_resrc)
             fprintf (stderr, "  %s = 0x%08x", buff, gpr[regnum++]);
        else fprintf (stderr, "  %s = 0x%08x",
		      buff,
		      *((unsigned *) (r13_area + R0_OFFSET + 4 * regnum++)));
     }
     fprintf (stderr, "\n");
  }

  fprintf (stderr, "\n\n");

  exit (1);
}

/************************************************************************
 *									*
 *				zeromem					*
 *				-------					*
 *									*
 * Assign all zero read-only values to memory locations between		*
 * 0x30000000 to 0xcfffffff.  Also call malloc to make as much of	*
 * segment 0x20000000 accessible as possible.				*
 *									*
 ************************************************************************/

void zeromem (void)
{
  caddr_t	   rc;
  int		   size;
  char		   *x;

  /* Make data segments readable so we don't get seg violations from
     speculative loads.
  */
/*rc = mmap ((caddr_t) 0x30000000, 0xA0000000, PROT_READ|PROT_WRITE|PROT_EXEC,*/
  rc = mmap ((caddr_t) 0x30000000, 0x40000000, PROT_READ|PROT_WRITE|PROT_EXEC,
	    MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);

  if (rc != (caddr_t) 0x30000000) {
    fprintf (stderr, "mmap 1 failed with errno=%d (%s)\n",
	     errno, errno_text[errno]);
    exit(1);
  }

  /* Try to make as much of seg 2 accessible as possible */
  if (XTNDSEG2) {

    size = 0x0f000000;
    while ((x = (char *) malloc (size)) == NULL) size -= 65536;

    fprintf (stderr, "zeromem: allocated %08x bytes from malloc\n", size);
    free    (x);
  }

   setup_segv_handler ();
}

/************************************************************************
 *									*
 *				setup_segv_handler			*
 *				------------------			*
 *									*
 * Establish signal handler for segmentation violations.		*
 *									*
 ************************************************************************/

setup_segv_handler ()
{
  struct sigaction mysigaction;

  mysigaction.sa_handler = (void (*)(int)) segv_handler;
  SIGINITSET (mysigaction.sa_mask);
  SIGADDSET  (mysigaction.sa_mask, SIGSEGV);
  sigaction  (SIGSEGV, &mysigaction, NULL);
}

/************************************************************************
 *									*
 *				zeromem_cleanup				*
 *				---------------				*
 *									*
 * Establish exit routine to print stats at end.  Should be invoked by	*
 * "atexit" or routine called by "atexit".				*
 *									*
 ************************************************************************/

void zeromem_cleanup (void)
{
#ifdef WANT_LIST_OF_SEG_VIOLATIONS_FROM_SPECULATIVE_LOADS
  if (segv_count > 0) {
    fprintf (stderr, "zeromem: %u segmentation violations\n", segv_count);
    fprintf (stderr, "zeromem: IAR=0x%08x    Instr=0x%08x    Basereg=%d\n",
	     segv_iar, segv_instr, segv_basereg);
  }
#endif
}
