/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

/************************************************************************
 *									*
 *				save_reexec_info			*
 *				----------------			*
 *									*
 ************************************************************************/

save_reexec_info ()
{
   int fd;

   fd = creat ("save.daisy", 0644);
   assert (fd >= 0);

   main_reexec_info	 (fd, write);
   simul_reexec_info	 (fd, write);
   hash_reexec_info	 (fd, write);
   vliw_dump_reexec_info (fd, write);
}

/************************************************************************
 *									*
 *				read_reexec_info			*
 *				----------------			*
 *									*
 ************************************************************************/

read_reexec_info ()
{
   int  fd;
   int  diff;
   char *err;

   fd = open ("save.daisy", O_RDONLY, 0644);
   if (fd < 0) {
      fprintf (stderr, "WARNING: Could not read info from previous run(s) in \"save.daisy\"\n");
      exit (1);
   }

   diff = main_reexec_info (fd, read);
   simul_reexec_info	   (fd, read);
   hash_reexec_info	   (fd, read);
   vliw_dump_reexec_info   (fd, read);

   /* This does not work because the original code has hardwired
      addresses from the original location for cross-page branches.
   */
/* hash_reexec_fix_addr (diff); */

   /* This is a piece of hardwired code used upon return from a kernel
      call that must be set up and is in a hardwired position.
   */
   setup_kernel_rtn ();
}
