/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*======================================================================*
 *									*
 * This module contains routines for tracing branch addresses in	*
 * translated DAISY code.						*
 *									*
 *======================================================================*/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/vmuser.h>
#include <errno.h>

#include "dis.h"
#include "dis_tbl.h"
#include "branch.h"
#include "stats.h"
#include "vliw.h"

#define MAX_NUM_BRKS			0x80000		/*  512K */
#define MAX_NUM_INS			0x200000	/* 2048K */

typedef struct {
   unsigned real_ins;
   unsigned record : 1;
} BRK;

static BRK	   brk_list[MAX_NUM_BRKS];
static unsigned    brk_num;
static unsigned    copy_gap;
static int	   fd_trace;

extern unsigned    *prog_start;
extern unsigned    *prog_end;

extern int	   trace_xlated_pgm;
extern int	   trace_all_br;	/* All types: Br-link & cond/uncond */
extern int	   trace_link_br;	/* Trace only branch-and-links      */
extern int	   trace_cond_br;	/* Trace only conditional branches  */
extern int	   trace_lib;		/* Trace lib fcns or only user code */
extern int	   operand_val[NUM_PPC_OPERAND_TYPES];
extern PPC_OP_FMT  *ppc_op_fmt[MAX_PPC_FORMATS];

unsigned *trace_br_xlated (unsigned *, unsigned *);

/************************************************************************
 *									*
 *				init_trace_br_xlated			*
 *				--------------------			*
 *									*
 ************************************************************************/

init_trace_br_xlated ()
{
   fd_trace = open ("xlated.trace", O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (fd_trace == -1) {
      fprintf (stderr, "Could not open \"xlated.trace\" to dump trace.\n");
      exit (1);
   }
}

/************************************************************************
 *									*
 *				trace_br_xlated				*
 *				---------------				*
 *									*
 * Translate addresses in translated DAISY code.			*
 *									*
 ************************************************************************/

unsigned *trace_br_xlated (rval, br_addr)
unsigned *rval;
unsigned *br_addr;
{
   int write_ok;

   /* If the application closes all file descriptors, as does the
      AIX sort utility, we may have to reopen and go to the end.
   */
   if (write (fd_trace, &br_addr, sizeof (br_addr)) ==
       sizeof (br_addr))				      write_ok = TRUE;
   else if (errno == EBADF) {
      fd_trace = open ("xlated.trace", O_RDWR, 0644);
      if (fd_trace == -1)				      write_ok = FALSE;
      else {
	 lseek (fd_trace, 0, SEEK_END);
	 if (write (fd_trace, &br_addr, sizeof (br_addr)) == 
	     sizeof (br_addr))				      write_ok = TRUE;
         else						      write_ok = FALSE;
      }
   }

   if (!write_ok) {
      fprintf (stderr, "Failure writing to \"xlated.trace\"\n");
      exit (1);
   }

   return rval;
}

/************************************************************************
 *									*
 *			    dump_simul_trace_code			*
 *			    ---------------------			*
 *									*
 * This routine is called when doing tracing from DAISY			*
 * compiled and simulated code.						*
 *									*
 ************************************************************************/

dump_simul_trace_code (tip, trace_br_upper, trace_br_lower)
TIP	 *tip;
unsigned trace_br_upper;
unsigned trace_br_lower;
{
   int	    tr_type_ok;
   int	    tr_locn_ok;
   unsigned addr_upper;
   unsigned addr_lower;
   unsigned *br_addr;

   if (trace_xlated_pgm) {

      br_addr = tip->br_addr;

      /* Is this the type of branch we want:  any/conditional/brlink ? */
      if (trace_all_br)	     tr_type_ok = TRUE;
      else if (trace_cond_br  &&  tip->br_type == BR_ONPAGE  &&  tip->left)
			     tr_type_ok = TRUE;
      else if (trace_link_br  &&  tip->br_link)
			     tr_type_ok = TRUE;
      else		     tr_type_ok = FALSE;

      /* Is the branch in a part of the program we are tracing ?
         (We could eventually let users specify specific address regions.)
      */
      if      (trace_lib) tr_locn_ok = TRUE;		 /* Doing everywhere */
      else if (br_addr >= prog_start && br_addr < prog_end) tr_locn_ok = TRUE;
      else		  tr_locn_ok = FALSE;

      /* If branch, type, and location are ok, do a trace */
      if (br_addr  &&  tr_type_ok  &&  tr_locn_ok) {
          addr_upper = ((unsigned) tip->br_addr) >> 16;
          addr_lower = ((unsigned) tip->br_addr)  & 0xFFFF;
	  dump (0x3FC00000 | addr_upper);	/* liu   r30,    addr_upper  */
	  dump (0x63DE0000 | addr_lower);	/* oril  r30,r30,addr_lower  */
	  dump (0x3FE00000 | trace_br_upper);	/* liu   r31, trace_br_UPPER */
	  dump (0x63FF0000 | trace_br_lower);	/* oril  r31,r31,     _LOWER */
	  dump (0x7FE803A6);			/* mtlr  r31		     */
	  dump (0x4E800021);			/* bcrl			     */
       }
   }
}
