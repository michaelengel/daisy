/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <fcntl.h>

extern unsigned    *prog_start;
extern unsigned    *prog_end;
extern int	   dump_bkpt_on;

static int	   bkpt_fd;
static unsigned	   native_diff;

/************************************************************************
 *									*
 *				init_bkpt				*
 *				---------				*
 *									*
 ************************************************************************/

init_bkpt ()
{
   unsigned *native_start;

   if (!dump_bkpt_on) return;

   /* This is a good guess at any rate */
   native_start = (unsigned *) (((unsigned) 0x10000000) + 
				 (unsigned) (((unsigned) prog_start) & 0xFFF));
   native_diff  = prog_start - native_start;
   bkpt_fd      = open ("daisy.bk", O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (bkpt_fd == -1) {
      fprintf (stderr, "Could not open \"./daisy.bk\" file for writing breakpoints\n");
      exit (1);
   }
}

/************************************************************************
 *									*
 *				finish_bkpt				*
 *				-----------				*
 *									*
 ************************************************************************/

finish_bkpt ()
{
   dump_bkpt_on = FALSE;

   if (bkpt_fd  &&  (bkpt_fd != stdout->_file) && (bkpt_fd != stderr->_file)) 
      close (bkpt_fd);
}

/************************************************************************
 *									*
 *				dump_bkpt				*
 *				---------				*
 *									*
 ************************************************************************/

dump_bkpt (entry)
unsigned *entry;
{
   if (entry >= prog_start && entry < prog_end)
      fdprintf (bkpt_fd, "stopi at 0x%08x\n", entry - native_diff);

   /* Probably a library in segment 13, but we also get the kernel
      invocation at 0x3600 ("svc_instr" in /unix).  However, putting
      a breakpoint there upsets "dbx", so skip it.
   */
   else if (((unsigned) entry) >= 0x10000000)
      fdprintf (bkpt_fd, "stopi at 0x%08x\n", entry);

/*  fflush (bkpt_fp);*/
}
