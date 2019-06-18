/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*#######################################################################
 #									#
 # These routines dump out information in a format suitable for dvstats.#
 #									#
 #######################################################################*/

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/mode.h>

#include "dis.h"
#include "vliw.h"
#include "resrc_offset.h"
#include "dis_tbl.h"

#define CURRENT_SPEC_MAGIC_NUM		0xFeedBacd
#define CURRENT_PERF_MAGIC_NUM		0xFeedBacc

/* These suffixes must match those in read_vliwcnts.c" */
const char *vliw_perf_fname_suffix = ".vliw_perf_ins_info";
const char *vliw_spec_fname_suffix = ".vliw_spec_ins_info";

extern int	use_prev_xlation;
extern unsigned xlate_entry_cnt;

extern unsigned *prog_start;
extern unsigned *prog_end;
extern unsigned *curr_entry_addr;

extern unsigned curr_spec_addr;
extern unsigned curr_spec_size;

extern int	no_aux_files;

static char vliw_perf_fname[128];
static char vliw_spec_fname[128];

static int  fd_perf;
static int  fd_spec;

static unsigned spec_magic_num;
static unsigned perf_magic_num;

/************************************************************************
 *									*
 *				instr_init				*
 *				----------				*
 *									*
 ************************************************************************/

instr_init (name, info_dir)
char *name;
char *info_dir;
{
   if (no_aux_files) return;

   make_info_dir (info_dir);
   /* Let everybody access this directory if we made it */
   chmod (info_dir, S_IRWXU|S_IRWXG|S_IRWXO);

   name = strip_dir_prefix (name);
   sprintf (vliw_perf_fname, "%s%s%s", info_dir, name, vliw_perf_fname_suffix);
   if (use_prev_xlation) {
      fd_perf = open (vliw_perf_fname, O_RDWR, 0644);
      lseek (fd_perf, 0, SEEK_END);
   }
   else fd_perf = open (vliw_perf_fname, O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (fd_perf == -1) {
      printf ("Could not open \"%s\" for writing\n", vliw_perf_fname);
      exit (1);
   }

   if (!use_prev_xlation) {
      perf_magic_num = CURRENT_PERF_MAGIC_NUM;
      if (write (fd_perf, &perf_magic_num, sizeof (perf_magic_num)) != 
					   sizeof (perf_magic_num)) {
	 printf ("Failure writing to \"%s\"\n", vliw_perf_fname);
         exit (1);
      }
   }

   sprintf (vliw_spec_fname, "%s%s%s", info_dir, name, vliw_spec_fname_suffix);
   if (use_prev_xlation) {
      fd_spec = open (vliw_spec_fname, O_RDWR, 0644);
      lseek (fd_spec, 0, SEEK_END);
   }
   else fd_spec = open (vliw_spec_fname, O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (fd_spec == -1) {
      printf ("Could not open \"%s\" for writing\n", vliw_spec_fname);
      exit (1);
   }

   if (!use_prev_xlation) {
      spec_magic_num = CURRENT_SPEC_MAGIC_NUM;
      if (write (fd_spec, &spec_magic_num, sizeof (spec_magic_num)) != 
					   sizeof (spec_magic_num)) {
	 printf ("Failure writing to \"%s\"\n", vliw_spec_fname);
	 exit (1);
      }

      /* Leave space for code size of the file, which we will insert at end */
      if (write (fd_spec, &curr_spec_addr, sizeof (curr_spec_addr)) !=
					   sizeof (curr_spec_addr)) {
	 printf ("Failure writing to \"%s\"\n", vliw_spec_fname);
	 exit (1);
      }
   }
}

/************************************************************************
 *									*
 *				make_info_dir				*
 *				-------------				*
 *									*
 ************************************************************************/

make_info_dir (info_dir)
char *info_dir;
{
   int  mkdir_status;
   char *p;

   /* We may have to make multiple directories.  If results are to
      go in /tmp/vliw_info/foo, but /tmp/vliw_info does not exist,
      both it and /tmp/vliw_info/foo must be created.
   */

   if (info_dir[0] == '/') p = &info_dir[1];
   else p = info_dir;

   for (; *p; p++) {
      if (*p == '/') {
         *p = 0;

	 mkdir_status = mkdir (info_dir, 0755);
	 if (mkdir_status == -1  &&  errno != EEXIST) {
	    printf ("Unable to create directory \"%s\" for info files\n",
		    info_dir);
	    exit (1);
	 }
	 
         *p = '/';
      }
   }
}

/************************************************************************
 *									*
 *				instr_wrapup				*
 *				------------				*
 *									*
 * Indicate the total size of VLIW's translated (curr_spec_addr) in	*
 * the spec info file.							*
 *									*
 ************************************************************************/

instr_wrapup ()
{
   if (no_aux_files) return;

   /* Insert the code size in the spec file */
   lseek (fd_spec, sizeof (spec_magic_num), SEEK_SET);
   if (write (fd_spec, &curr_spec_addr, sizeof (curr_spec_addr)) !=
					sizeof (curr_spec_addr)) {
      printf ("Failure writing to \"%s\"\n", vliw_spec_fname);
      exit (1);
   }

   close (fd_perf);
   close (fd_spec);

   fd_perf = 0;			/* Indicate that these are closed */
   fd_spec = 0;
}

/************************************************************************
 *									*
 *				instr_bump_leaf_cnt			*
 *				-------------------			*
 *									*
 * Uses r29 thru r31 for scratch.					*
 *									*
 ************************************************************************/

instr_bump_leaf_cnt (slot)
unsigned slot;
{
   unsigned disp = 4 * slot;
   char	    buff[128];

   if (no_aux_files) return;

   dump (0x83ED0000 | VLIW_PATH_CNTS);	/* l r31,VLIW_PATH_CNTS(r13) */

   if (disp <= (32768 - 4)) {		/* Can we use     l   r31,disp(r31) */
      dump (0x83DF0000 | disp);		/* l    r30,disp(r31)        */
      dump (0x3BDE0001);		/* cal  r30,1(r30)           */
      dump (0x93DF0000 | disp);		/* st   r30,disp(r31)        */
   }
   else {				/* Have to use    lx   r29,r30,r31  */
      dump (0x3FC00000 | (disp>>16)  );	/* cau  r30,r0,disp_UPPER    */
      dump (0x63DE0000 | (disp&0xFFFF));/* oril r30,r30,disp_LOWER   */
      dump (0x7FBEF82E);		/* lx   r29,r30,r31	     */
      dump (0x3BBD0001);		/* cal  r29,1(r29)	     */
      dump (0x7FBEF92E);		/* stx  r29,r30,r31	     */
   }
}

/************************************************************************
 *									*
 *				instr_save_path_ins			*
 *				-------------------			*
 *									*
 * Returns TRUE if actually saved path info,  FALSE otherwise.		*
 * (The info is not saved at the end (in "exit") after the info		*
 * files have already been closed.)					*
 *									*
 ************************************************************************/

int instr_save_path_ins (index, path_ins, path_ins_cnt, orig_ops, spec_index,
			 path_num, branch_depth, entry_vliw, entry_vliw_num,
			 ppc_code_addr, tip, num_cycles)
unsigned       index;
OPCODE2	       *path_ins[];
int	       path_ins_cnt;
int	       orig_ops;
unsigned       spec_index;
int	       path_num;
int	       branch_depth;
VLIW	       *entry_vliw;
unsigned       entry_vliw_num;
unsigned       ppc_code_addr;
TIP	       *tip;
int	       num_cycles;	/* From Group Start to exit, 0 ==> Not exit */
{
   int		 i;
   int		 write_ok;
   unsigned char *p;
   unsigned char buff[5*MAX_OPS_PER_VLIW+512];

   if (no_aux_files) return;

   /* At end of translation, i.e. in exit (), may have already closed file */
   if (!fd_perf) return FALSE;

   p = buff;
   *((unsigned *) p) = index;
   p += sizeof (index);

   *((unsigned *) p) = spec_index;
   p += sizeof (spec_index);

   *p++ = branch_depth;
   *p++ = path_num;
   *p++ = path_ins_cnt;
   *p++ = orig_ops;

   /* Put a 1 in next byte if this tip is from a user page */
   if (curr_entry_addr >= prog_start  &&  curr_entry_addr < prog_end) *p++ = 1;
   else								      *p++ = 0;
   
   /* Put out info about group exits so we can see what and how many
      paths account for the bulk of the programs execution time.
      We can also do profile directed feedback.
   */
   p = save_group_exit_info (p, entry_vliw, entry_vliw_num, ppc_code_addr, 
			     tip, num_cycles);

   for (i = 0; i < path_ins_cnt; i++)
      *p++ = get_ca_ppc_opcode (path_ins[i]->b->op_num & (~RECORD_ON));

   /* We use this for recording offsets. */
   for (i = 0; i < path_ins_cnt; i++) {
      *((unsigned *) p) = get_offset (path_ins[i]);
      p += sizeof (unsigned);
   }

   /* Make sure we don't overrun our buffer */
   assert (p < &buff[5*MAX_OPS_PER_VLIW+512]);

   /* If the application closes all file descriptors, as does the
      AIX sort utility, we may have to reopen and go to the end.
   */
   if (write (fd_perf, &buff[0], p - buff) == p - buff)       write_ok = TRUE;
   else if (errno == EBADF) {
      fd_perf = open (vliw_perf_fname, O_RDWR, 0644);
      if (fd_perf == -1)				      write_ok = FALSE;
      else {
         lseek (fd_perf, 0, SEEK_END);
         if (write (fd_perf, &buff[0], p - buff) == p - buff) write_ok = TRUE;
         else						      write_ok = FALSE;
      }
   }

   if (!write_ok) {
      printf ("Failure writing to \"%s\"\n", vliw_perf_fname);
      exit (1);
   }

   return TRUE;
}

/************************************************************************
 *									*
 *				save_specified_ins			*
 *				------------------			*
 *									*
 * REMEMBER:  Insert spec_index into output of instr_save_path_ins.	*
 *									*
 * REMEMBER:  Count and insert specified branch ops			*
 *		(incr in "handle_cond", reset in "is_vliw_label"	*
 *									*
 ************************************************************************/

save_specified_ins (name, index, specified_op, num_ops, branch_spec_cnt)
char	 *name;
unsigned index;
OPCODE2	 *specified_op[];
int	 num_ops;
int	 branch_spec_cnt;
{
   int		 i;
   int		 write_ok;
   unsigned char name_len;
   unsigned char buff[MAX_OPS_PER_VLIW+512];
   unsigned char *p = buff;

   if (no_aux_files) return;

   /* At end of translation, i.e. in exit (), may have already closed file */
   if (!fd_spec) return;	

   *((unsigned *) p) = index;
   p += sizeof (index);

   *((unsigned *) p) = curr_spec_size;
   p += sizeof (curr_spec_size);

   name_len = strlen (name) + 1;
   *p++ = name_len;

   strcpy (p, name);
   p += name_len;

   *p++ = branch_spec_cnt;

   /* This was true for the architecture during the summer of 1994,
      but is no longer.
   */

   *p++ = num_ops;
   
   for (i = 0; i < num_ops; i++)
      *p++ = get_ca_ppc_opcode (specified_op[i]->b->op_num);

   for (i = 0; i < num_ops; i++) {
      *((unsigned *) p) = get_offset (specified_op[i]);
      p += sizeof (unsigned);
   }

   /* If the application closes all file descriptors, as does the
      AIX sort utility, we may have to reopen and go to the end.
   */
   if (write (fd_spec, &buff[0], p - buff) == p - buff)       write_ok = TRUE;
   else if (errno == EBADF) {
      fd_spec = open (vliw_spec_fname, O_RDWR, 0644);
      if (fd_spec == -1)				      write_ok = FALSE;
      else {
         lseek (fd_spec, 0, SEEK_END);
         if (write (fd_spec, &buff[0], p - buff) == p - buff) write_ok = TRUE;
         else						      write_ok = FALSE;
      }
   }

   if (!write_ok) {
      printf ("Failure writing to \"%s\"\n", vliw_spec_fname);
      exit (1);
   }
}
