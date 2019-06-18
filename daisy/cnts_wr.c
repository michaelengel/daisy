/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <stdlib.h>
#include <fcntl.h>

#include "cnts.h"

extern int	   use_prev_xlation;

extern int	   no_aux_files;
extern char	   curr_wd[];
extern VLIW_CNTS   __vliw_perf_ins_cnts[1];
extern unsigned    __vliw_perf_ins_num_files;
extern INS_OFFSET *__ins_offsets_in_file;

static INS_OFFSET stub_ins_ptr;

static unsigned   num_paths;
static unsigned   num_cnts_in;

static char       *simul_basename;
static char       ins_cnts_filename[256];
static unsigned   num_times_exec;

/* This suffix must match that in "read_vliwcnts.c" */
static const char *cntfile_suffix = ".vliw_perf_ins_cnts";

static void read_vliw_cnts_from_disk (void);
static void init_first_ins_in_file_offsets (void);
void __init_timer (char *);
void __finish_vliw_simul (void);
void __finish_timer (void);
static void write_vliw_cnts_to_disk (void);
static void mismatch_with_old_cnts (int);
static void write_lib_counts (int);
static void read_failure (void);
static void write_failure (void);
char *strip_dir_prefix (char *);

/************************************************************************
 *									*
 *			init_cnts_and_timer				*
 *			-------------------				*
 *									*
 ************************************************************************/

void init_cnts_and_timer (progname, num_paths, perf_cnts)
char	 *progname;
unsigned *num_paths;
unsigned *perf_cnts;
{
   simul_basename = strip_dir_prefix (progname);

   __vliw_perf_ins_cnts[0].name     = simul_basename;
   __vliw_perf_ins_cnts[0].num_cnts = num_paths;
   __vliw_perf_ins_cnts[0].cnts     = perf_cnts;

   read_vliw_cnts_from_disk ();

   __ins_offsets_in_file = &stub_ins_ptr;
   init_first_ins_in_file_offsets ();
   __init_timer (simul_basename);
}

/************************************************************************
 *									*
 *			finish_vliw_simul				*
 *			-----------------				*
 *									*
 * This routine should be called after exiting the VLIW code.		*
 *									*
 ************************************************************************/

void finish_vliw_simul ()
{
   static int already_called = FALSE;

   /* With "atexit" and hardwired calls at return, we may get called
      multiple times.  
   */
   if (already_called) return;
   else already_called = TRUE;

   __finish_timer ();
   write_vliw_cnts_to_disk ();
}

/************************************************************************
 *									*
 *			read_vliw_cnts_from_disk			*
 *			------------------------			*
 *									*
 ************************************************************************/

static void read_vliw_cnts_from_disk ()
{
   unsigned  i;
   unsigned  magic_num;
   unsigned  old_num_files;
   unsigned  name_len;
   int	     fd;
   VLIW_CNTS *vc;

   strcpy (ins_cnts_filename, curr_wd);
   strcat (ins_cnts_filename, "/");
   strcat (ins_cnts_filename, simul_basename);
   strcat (ins_cnts_filename, cntfile_suffix);

   fd = open (ins_cnts_filename, O_RDONLY, 0644);
   if (fd == -1) return;  /* There is no old cnts file to read in and update */

   if (read (fd, &magic_num, sizeof (magic_num)) != sizeof (magic_num))
      read_failure ();

   if (magic_num != CURR_CNTS_MAGIC_NUM) {
      mismatch_with_old_cnts (fd);
      return;
   }

   if (read (fd, &num_times_exec, sizeof (num_times_exec)) !=
				  sizeof (num_times_exec)) read_failure ();

   if (read (fd, &old_num_files, sizeof (old_num_files))   !=
				 sizeof (old_num_files))   read_failure ();

   if (old_num_files != 1) {
      mismatch_with_old_cnts (fd);
      return;
   }
   else {
      for (i = 0; i < old_num_files; i++) {
	 vc = &__vliw_perf_ins_cnts[i];

	 if (!name_len_ok (fd, vc, &name_len)) break;
	 if (!name_ok (fd, vc, name_len)) break;
	 if (!num_cnts_ok (fd, vc)) break;

	 assert (1 == 0);	/* Should not get here till can sync */

	 if (read (fd, vc->cnts, 4 * (*vc->num_cnts)) != 4 * (*vc->num_cnts))
	    read_failure ();
      }

      if (i != old_num_files) {
	 mismatch_with_old_cnts (fd);
	 return;
      }

      if (!read_lib_counts_ok (fd)) {
	 mismatch_with_old_cnts (fd);
	 return;
      }
   }

   close (fd);
}

/************************************************************************
 *									*
 *				read_lib_counts_ok			*
 *				------------------			*
 *									*
 * Reads counts of the number of times each library function was	*
 * called.  If all the fields correspond to the current counts, return	*
 * TRUE, otherwise return FALSE.					*
 *									*
 ************************************************************************/

static int read_lib_counts_ok (fd)
int fd;
{
   unsigned num_lib_fcn;

   if (read (fd, &num_lib_fcn, sizeof (num_lib_fcn)) != sizeof (num_lib_fcn))
      read_failure ();

   if (num_lib_fcn !=  0) return FALSE;

   return TRUE;
}

/************************************************************************
 *									*
 *			mismatch_with_old_cnts				*
 *			----------------------				*
 *									*
 * An earlier counts file exists for this executable, however its	*
 * fields do not match the current fields (meaning probably that the	*
 * executable has been changed and/or recompiled with a new compiler.	*
 *									*
 * If this occurs, reset all counts to 0 (in case we already read some)	*
 * and warn the user.							*
 *									*
 ************************************************************************/

static void mismatch_with_old_cnts (fd)
int fd;
{
   int	     i;
   VLIW_CNTS *vc;

   return;	/* Nothing to do until we can compare counts */

   printf ("WARNING: Incompatible counts file exists for this executable.\n");
   printf ("         All counts reset to 0.\n\n");

   num_times_exec = 0;

   for (i = 0; i < __vliw_perf_ins_num_files; i++) {
      vc = &__vliw_perf_ins_cnts[i];
      memset (vc->cnts, 0, *vc->num_cnts * 4);
   }

   close (fd);
}

/************************************************************************
 *									*
 *				name_len_ok				*
 *				-----------				*
 *									*
 ************************************************************************/

static int name_len_ok (fd, vc, old_name_len)
int	  fd;
VLIW_CNTS *vc;
int	  *old_name_len;
{
   int	     name_len;

   if (read (fd, old_name_len, sizeof (*old_name_len)) !=
			       sizeof (*old_name_len)) read_failure ();

   name_len = strlen (vc->name) + 1;	  /* Include null terminator  */
   name_len = 4 * ((name_len + 3) / 4);	  /* Round to 4 byte boundary */

   if (name_len == (*old_name_len)) return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *				name_ok					*
 *				-------					*
 *									*
 ************************************************************************/

static int name_ok (fd, vc, name_len)
int	  fd;
VLIW_CNTS *vc;
int	  name_len;
{
   char	  old_name[128];

   if (read (fd, old_name, name_len) != name_len) read_failure ();

   if (!strcmp (old_name, vc->name)) return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *				name_cnts_ok				*
 *				------------				*
 *									*
 * We don't know at the start of execution how many counts there will	*
 * be.  When we get around to saving the translation to disk and re-	*
 * reading it, we can bring this function back to life.  Until then, we	*
 * always report back that number of counts is NOT ok.			*
 *									*
 ************************************************************************/

static int num_cnts_ok (fd, vc)
int	  fd;
VLIW_CNTS *vc;
{
   unsigned num_cnts;

   if (read (fd, &num_cnts, sizeof (num_cnts)) != sizeof (num_cnts))
      read_failure ();

   return FALSE;

   if (num_cnts == (*vc->num_cnts)) return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *			write_vliw_cnts_to_disk				*
 *			-----------------------				*
 *									*
 * This routine should be called after exiting the VLIW code.  Note	*
 * that it requires "read_vliw_cnts_from_disk" to be called previously	*
 * to initialize "ins_cnts_filename".					*
 *									*
 ************************************************************************/

static void write_vliw_cnts_to_disk ()
{
   unsigned  i;
   unsigned  magic_num = CURR_CNTS_MAGIC_NUM;
   int	     name_len;
   int	     fd;
   VLIW_CNTS *vc;

   if (no_aux_files) return;

   fd = open (ins_cnts_filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (fd == -1) {
      printf ("Could not open \"%s\" for writing\n", ins_cnts_filename);
      exit (1);
   }

   if (write (fd, &magic_num, sizeof (magic_num)) != sizeof (magic_num))
      write_failure ();

   num_times_exec++;
   if (write(fd, &num_times_exec, sizeof (num_times_exec)) !=
				  sizeof (num_times_exec)) write_failure ();

   if (write (fd, &__vliw_perf_ins_num_files, sizeof (__vliw_perf_ins_num_files))
					  !=  sizeof (__vliw_perf_ins_num_files))
      write_failure ();

   assert (__vliw_perf_ins_num_files == 1);

   for (i = 0; i < __vliw_perf_ins_num_files; i++) {
      vc = &__vliw_perf_ins_cnts[i];

      name_len = strlen (vc->name) + 1;		/* Include null terminator  */
      name_len = 4 * ((name_len + 3) / 4);	/* Round to 4 byte boundary */

      if (write (fd, &name_len, sizeof (name_len)) != sizeof (name_len))
	 write_failure ();

      if (write (fd, vc->name, name_len) != name_len) write_failure ();

      if (write (fd, vc->num_cnts, sizeof (*vc->num_cnts)) !=
			           sizeof (*vc->num_cnts)) write_failure ();

      if (write (fd, vc->cnts, 4 * (*vc->num_cnts)) != 4 * (*vc->num_cnts))
	 write_failure ();
   }

   write_lib_counts (fd);

   close (fd);
}

/************************************************************************
 *									*
 *				write_lib_counts			*
 *				----------------			*
 *									*
 * Writes counts of the number of times each library function was	*
 * called.								*
 *									*
 ************************************************************************/

static void write_lib_counts (fd)
int fd;
{
   unsigned num_lib_fcn;

   num_lib_fcn = 0;

   if (write (fd, &num_lib_fcn, sizeof (num_lib_fcn)) != sizeof (num_lib_fcn))
      write_failure ();
}

/************************************************************************
 *									*
 *			init_first_ins_in_file_offsets			*
 *			------------------------------			*
 *									*
 *									*
 ************************************************************************/

static void init_first_ins_in_file_offsets ()
{
   unsigned   i;
   unsigned   curr_offset;
   INS_OFFSET **p = (INS_OFFSET **) &__ins_offsets_in_file;
   int	      dump_addresses;

   dump_addresses = (getenv ("VLIW_ADDRESS_DUMP") != NULL);

   /* Fill in the offsets of the first VLIW instruction in each file.
      The first user code file is assigned base address VLIW_INST_BASE_ADDR +
      VLIW_INST_ALIGN because the startup code '__start' is assumed to be at
      address VLIW_INST_BASE_ADDR.
   */
   curr_offset = VLIW_INST_BASE_ADDR + VLIW_INST_ALIGN;
   for (i = 0; i < __vliw_perf_ins_num_files; i++, p++) {
      (*p)->offset = curr_offset;

      if (dump_addresses)
	 fprintf (stderr, "VLIW code base address for file \"%s\" = 0x%x\n",
		  &((*p)->basename), curr_offset);

      curr_offset += ((*p)->code_size);
      curr_offset  = (curr_offset + (VLIW_INST_ALIGN - 1)) &
		     (~(VLIW_INST_ALIGN - 1));
   }
}

/************************************************************************
 *									*
 *				write_failure				*
 *				-------------				*
 *									*
 ************************************************************************/

static void write_failure ()
{
   printf ("Failure writing to \"%s\"\n", ins_cnts_filename);
   exit (1);
}

/************************************************************************
 *									*
 *				read_failure				*
 *				------------				*
 *									*
 ************************************************************************/

static void read_failure ()
{
   printf ("Failure reading from \"%s\"\n", ins_cnts_filename);
   exit (1);
}

/************************************************************************
 *                                                                      *
 *                              strip_dir_prefix			*
 *                              ----------------                        *
 *                                                                      *
 * Removes any path name from passed "name".  Returns pointer to start	*
 * of actual base name in "name".					*
 *                                                                      *
 ************************************************************************/

char *strip_dir_prefix (name)
char *name;
{
   char *p;
   char *pslash = 0;

   for (p = name; *p; p++)
      if (*p == '/') pslash = p;

   if (*pslash == '/') return pslash+1;
   else return name;
}
