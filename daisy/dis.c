/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "dis_tbl.h"
#include "latency.h"

#define NUM_PRIMARY_PPC_OPCODES		64

extern unsigned    *image;
extern unsigned    image_size;
extern OPCODE1	   *ppc_opcode[];
extern OPCODE1     for_ops[];
extern int	   for_ops_elem;

       OPCODE1  **ext_tbl[NUM_PRIMARY_PPC_OPCODES];
       OPCODE1  *no_ext_tbl[NUM_PRIMARY_PPC_OPCODES];

OPCODE1 illegal_opcode = {0, E15,  0,	0,	0, OP_ILLEGAL, "ILLEGAL",
			  0, 0, 0, 0, 1};

static void *malloc_tbl_mem (int);

/************************************************************************
 *									*
 *				init_everything				*
 *				---------------				*
 *									*
 * Initialize (almost) everything.					*
 *									*
 ************************************************************************/

init_everything ()
{
   int i;

   sort_to_ascending ();
   check_ascending   ();
   sort_ppc_opcodes  ();
   sort_resources    ();
   setup_regs ();
   vliw_init  ();
   init_get_obj_mem     ();
   init_simul		();
   init_continuation    ();
   init_fixup_bcrl	();
   init_branch_and_link ();
   init_schedule_commit ();
   init_cache ();
   init_hash  ();
   init_cnt_array ();
   init_bkpt ();
   init_bct  ();
   init_combining ();
   init_cluster_mem ();
   init_loophdr_mem ();
   init_resrc_usage ();
   init_rename      ();

#ifdef DEBUGGING3
   dump_for_ops_table (stdout);
#endif

   set_ext_list ();

   init_opcode_fmt ();
}

#ifdef DEBUGGING3
/************************************************************************
 *									*
 *				dump_for_ops_table			*
 *				------------------			*
 *									*
 ************************************************************************/

dump_for_ops_table (fp)
FILE *fp;
{
   int i;

   for (i = 0; i < for_ops_elem; i++) {
      if (for_ops[i].b.extsize > 0)
         fprintf (fp, "%3d:  %2d -%4d  %-10s  (p. %d)\n", 
		  i, for_ops[i].b.primary, for_ops[i].b.ext, 
		  for_ops[i].b.name, for_ops[i].b.page);

      else fprintf (fp, "%3d:  %2d        %-10s  (p. %d)\n", 
		    i, for_ops[i].b.primary, for_ops[i].b.name, 
		    for_ops[i].b.page);
   }
}
#endif

#ifdef DEBUGGING1
/************************************************************************
 *									*
 *				print_ppc_opcode_info			*
 *				---------------------			*
 *									*
 ************************************************************************/

print_ppc_opcode_info (fp, ins, opcode)
FILE	 *fp;
unsigned ins;
OPCODE1  *opcode;
{
   if (opcode == 0) fprintf (fp, "\nOpcode 0x%08x not found.\n\n", ins);
   else if (opcode == &illegal_opcode)
      fprintf (fp, "0x%08x:  %s\n", ins, opcode->b.name);
   else {
      fprintf (fp, "0x%08x:  %-8s", ins, opcode->b.name);
      dump_operands (fp, ins, opcode->b.format);
      fprintf (fp, "  Primary = %d   Ext = %d   Form = %d   Page = %d\n",
	       opcode->b.primary, opcode->b.ext, opcode->b.format,
	       opcode->b.page);
   }
}
#endif

/************************************************************************
 *									*
 *				set_ext_list				*
 *				------------				*
 *									*
 * ASSUMES that "for_ops" is ordered in increasing order of (1)	primary	*
 * opcodes and (2) extended opcodes.   See "opcode_cmp" function below.	*
 *									*
 ************************************************************************/

set_ext_list ()
{
   int	  i;
   int	  num_ext;
   int	  max_extsize;
   int	  start_index;
   int    primary;
   int	  prev_primary;

   prev_primary = for_ops[0].b.primary;
   start_index = 0;
   num_ext = 0;
   max_extsize = 0;

   for (i = 0; i < NUM_PRIMARY_PPC_OPCODES; i++) {
      no_ext_tbl[i] = &illegal_opcode;
      ext_tbl[i] = &no_ext_tbl[i];
   }

   init_new_primary_opcode (0, prev_primary,
			    &start_index, &num_ext, &max_extsize);

   for (i = 0; i < for_ops_elem; i++) {
      primary = for_ops[i].b.primary;

      if (primary != prev_primary) {
	 create_ext_tbl_entry (i, start_index, num_ext, prev_primary,
			       1 << max_extsize);

         init_new_primary_opcode (i, primary,
				  &start_index, &num_ext, &max_extsize);
	 prev_primary = primary;
      }
      else {
	 num_ext++;
	 if (for_ops[i].b.extsize > max_extsize)
	    max_extsize = for_ops[i].b.extsize;
      }
   }

   /* If we ended with a series of extended opcodes, make sure they
      are in the table.
   */
   create_ext_tbl_entry (i, start_index, num_ext, prev_primary,
			 1 << max_extsize);
}

/************************************************************************
 *									*
 *				init_new_primary_opcode			*
 *				-----------------------			*
 *									*
 ************************************************************************/

init_new_primary_opcode (i, primary, start_index, num_ext, max_extsize)
int i;
int primary;
int *start_index;
int *num_ext;
int *max_extsize;
{
   if (for_ops[i].b.extsize == 0) {
      no_ext_tbl[primary] = &for_ops[i];
      ext_tbl[primary] = &no_ext_tbl[primary];
   }
   else {
      *start_index = i;
      *num_ext = 1;
      *max_extsize = for_ops[i].b.extsize;
   }
}

/************************************************************************
 *									*
 *				create_ext_tbl_entry			*
 *				--------------------			*
 *									*
 ************************************************************************/

create_ext_tbl_entry (i, index, num_ext, prev_primary, malloc_cnt)
int i;
int index;
int num_ext;
int prev_primary;
int malloc_cnt;
{
   int	   j;
   int	   offset;
   OPCODE1 **tbl;

   if (for_ops[i-1].b.extsize > 0) {

      tbl = malloc_tbl_mem (malloc_cnt * sizeof (tbl[0]));
      assert (tbl);
      ext_tbl[prev_primary] = tbl;

      /* Initialize unused extensions to be illegal */
      for (j = 0; j < malloc_cnt; j++)
	 tbl[j] = &illegal_opcode;

      for (j = 0; j < num_ext; j++) {
	 offset = for_ops[index].b.ext;
	 tbl[offset] = &for_ops[index++];
      }
   }
}

/************************************************************************
 *									*
 *				malloc_tbl_mem				*
 *				--------------				*
 *									*
 * If we call malloc/calloc/etc, it can alter slightly the		*
 * execution of the translated program.  The library routine		*
 * "getenv" is invoked during the first execution of malloc.		*
 * In addition the relative pieces of allocated memory may		*
 * change.  								*
 *									*
 * We know from running an instrumented version of this, that		*
 * for the PPC opcodes, 24656 bytes are needed in total for		*
 * the table.  Just to be safe, we use 32768.				*
 *									*
 ************************************************************************/

static void *malloc_tbl_mem (size)
int size;
{
   static unsigned char tbl_mem[32768];
   static unsigned char *ptbl_end = &tbl_mem[32768];
   static unsigned char *ptbl_mem = tbl_mem;
	  unsigned char *rval     = ptbl_mem;

   ptbl_mem += size;
   assert (ptbl_mem <= ptbl_end);
   return rval;
}

/************************************************************************
 *									*
 *				sort_to_ascending			*
 *				-----------------			*
 *									*
 ************************************************************************/

sort_to_ascending ()
{
   qsort (for_ops, for_ops_elem, sizeof (for_ops[0]), opcode_cmp);
}

/************************************************************************
 *									*
 *				check_ascending				*
 *				---------------				*
 *									*
 ************************************************************************/

void check_ascending ()
{
   int i;
   int primary;
   int ext;
   int prev_primary = -1;
   int prev_ext = -1;

   for (i = 0; i < for_ops_elem; i++) {
      primary = for_ops[i].b.primary;
      if (primary < prev_primary) assert (1 == 0);
      else if (primary == prev_primary) {
         ext = for_ops[i].b.ext;
	 if (ext < prev_ext) assert (1 == 0);
	 else prev_ext = ext;
      }
      else prev_ext = -1;

      prev_primary = primary;
   }
}

/************************************************************************
 *									*
 *				get_opcode				*
 *				----------				*
 *									* 
 * RETURNS:  Pointer to opcode entry in "for_ops" if match found,	*
 *	     Otherwise returns NULL.					*
 *									*
 * ASSUMES:								*
 *									*
 * (1) That there is no ambiguity in opcodes, in particular that each	*
 *     primary opcode has at most one size matching extended opcode. An	*
 *     example where this is not the case would be if primary opcode 5	*
 *     had extended opcodes of size 3-bits and 4-bits.  If one of the	*
 *     3-bit extended opcodes is 5 and one of the 4-bit extended	*
 *     opcodes is 13, then it is ambigous which is the correct		*
 *     disassembly.  Power / Power-PC have no such ambiguities.		*
 *     No check is made for this.					*
 *									*
 ************************************************************************/

OPCODE1 *get_opcode (ins)
unsigned ins;
{
   int	    ext;
   int	    extsize;
   unsigned primary;
   unsigned mask;
   OPCODE1  **tbl;

   primary = ins >> 26;

   switch (primary) {
      case 16:
      case 17:
      case 18:  extsize =  2;	break;

      case 19:  extsize = 11;	break;

      case 20:
      case 21:
      case 22:
      case 23:  extsize =  1;   break;

      case 31:  extsize = 11;	break;
		
      case 59:
      case 63:  if (ins & 0x20) extsize = 6;  else extsize = 11;  break;

      default:  extsize = 0;	break;
   }

   tbl = ext_tbl[primary];
/* Ambiguous for primary = 63
   extsize = tbl[0]->b.extsize;
*/

   if (extsize == 0) return tbl[0];
   else {
      mask = ~(0xFFFFFFFF << extsize);
      ext = ins & mask;
      return tbl[ext];
   }
}

/************************************************************************
 *									*
 *				opcode_cmp				*
 *				----------				*
 *									*
 ************************************************************************/

int opcode_cmp (a, b)
OPCODE1 *a;
OPCODE1 *b;
{
   if      (a->b.primary > b->b.primary) return  1;
   else if (a->b.primary < b->b.primary) return -1;

   else if (a->b.extsize > 0) {
      if      (a->b.ext > b->b.ext     ) return  1;
      else if (a->b.ext < b->b.ext     ) return -1;
      else assert (1 == 0);
   }

   return 0;
}

/************************************************************************
 *									*
 *				sort_ppc_opcodes			*
 *				-----------------			*
 *									*
 * Make "ppc_opcode" be a table of pointers to the "for_ops" table	*
 * defined in "dis_tbl.c".  Unlike "for_ops", "ppc_opcode" has only	*
 * 1 entry per opcode name.  The extra entry in "for_ops" always	*
 * corresponds to an opcode which has some undefined effect, such	*
 * as "stsx" with the condition code bit set leaves the condition	*
 * code register in an undefined state.  Since "ppc_opcode" is used	*
 * to generate (not translate) instructions, we have no need for	*
 * such arguably illegal instructions.					*
 *									*
 ************************************************************************/

sort_ppc_opcodes ()
{
   int i;
   int op_num;

   for (i = 0; i < for_ops_elem; i++) {
      op_num = for_ops[i].b.op_num;

      if (ppc_opcode[op_num] == 0) ppc_opcode[op_num] = &for_ops[i];
      else if (for_ops[i].b.ext < ppc_opcode[op_num]->b.ext)
	 ppc_opcode[op_num] = &for_ops[i];
   }
}

#ifdef DEBUGGING1
/************************************************************************
 *									*
 *				main					*
 *				----					*
 *									*
 ************************************************************************/

main (argc, argv)
int  argc;
char *argv[];
{
   int	    cnt = 1;
   unsigned ins;
   OPCODE1  *opcode;
   char     buff[1024];

   init_everything ();

   do {
      printf ("Enter opcode:  ");
      gets (buff);
      if (sscanf (buff, "%x", &ins) != 1) break;

      opcode = get_opcode (ins);
      print_ppc_opcode_info (stdout, ins, opcode);

      if (cnt % 3 == 0) init_page_xlate ();
      set_operands (opcode, ins, opcode->b.format, 0);
      dump_vector_resrc_usage (stdout, 0);
      dump_list_resrc_usage (stdout, 0);
   } while (TRUE); 

   exit (0);
}
#endif

#ifdef DEBUGGING2
/************************************************************************
 *									*
 *				main					*
 *				----					*
 *									*
 ************************************************************************/

main (argc, argv)
int  argc;
char *argv[];
{
   unsigned i;
   unsigned ins;
   OPCODE1  *opcode;

   init_everything ();

   init_image ();
   read_vliw_image ("hello", 0x80);

   for (i = 0; i < image_size / sizeof (unsigned); i++) {
      opcode = get_opcode (image[i]);
      print_ppc_opcode_info (stdout, image[i], opcode);
   }

   exit (0);
}
#endif
