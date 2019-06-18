/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "dis.h"
#include "parse.h"
#include "op_mapping.h"

extern int    for_ops_elem;
extern OPCODE for_ops[];
extern OPCODE *for_ops_sort[];
extern double total_vliw_spec_ops;
extern double total_vliw_perf_ops;
extern double total_perf_ops;
extern double total_spec_ops;

extern OPCNT  perf_op_cnt[];
extern OPCNT  spec_op_cnt[];
extern double skip_op_perf_cnt;
extern double skip_op_spec_cnt;

extern OP_MAPPING  *op_map_table[];

int cnt_array_cmp (OPCNT *, OPCNT *);
FILE *open_op_freq_file (char *, char *);

/************************************************************************
 *									*
 *				init_for_ops_sort			*
 *				-----------------			*
 *									*
 ************************************************************************/

init_for_ops_sort ()
{
   int i;

    skip_op_perf_cnt = 0.0;
    skip_op_spec_cnt = 0.0;

   for (i = 0; i < NUM_OPCODES; i++) {
      perf_op_cnt[i].cnt = 0.0;
      spec_op_cnt[i].cnt = 0.0;

      perf_op_cnt[i].opcode = i;
      spec_op_cnt[i].opcode = i;
   }

   for (i = 0; i < for_ops_elem; i++) {
      for_ops_sort[i] = &for_ops[i];
   }
}

/************************************************************************
 *									*
 *				bump_spec_op_cnt			*
 *				----------------			*
 *									*
 *									*
 ************************************************************************/

bump_spec_op_cnt (op_code, spec_ins_cnt)
OPCODE *op_code;
double spec_ins_cnt;
{
   if (op_code) {
      op_code->spec_cnt   += spec_ins_cnt;
      total_vliw_spec_ops += spec_ins_cnt;
   }
}

/************************************************************************
 *									*
 *				bump_perf_op_cnt			*
 *				----------------			*
 *									*
 *									*
 ************************************************************************/

bump_perf_op_cnt (op_code, perf_ins_cnt)
OPCODE *op_code;
double perf_ins_cnt;
{
   assert (op_code);

   op_code->perf_cnt   += perf_ins_cnt;
   total_vliw_perf_ops += perf_ins_cnt;
}

/************************************************************************
 *									*
 *				sort_ops_by_freq			*
 *				----------------			*
 *									*
 * Used for internal daisy frequencies.					*
 *									*
 ************************************************************************/

sort_ops_by_freq (cnt_array)
OPCNT *cnt_array;
{
   qsort (cnt_array, NUM_OPCODES, sizeof (cnt_array[0]), cnt_array_cmp);
}

/************************************************************************
 *									*
 *				cnt_array_cmp				*
 *				-------------				*
 *									*
 ************************************************************************/

int cnt_array_cmp (a, b)
OPCNT *a;
OPCNT *b;
{
   if	   (a->cnt > b->cnt) return -1;
   else if (a->cnt < b->cnt) return  1;
   else			     return  0;
}

/************************************************************************
 *									*
 *				dump_sort_ops				*
 *				-------------				*
 *									*
 * Dump opcode frequencies obtained using internal files produced by	*
 * daisy.								*
 *									*
 ************************************************************************/

dump_sort_ops (name, num_print, print_zeros)
char *name;
int  num_print;		/* If > 0, print only first "num_print" entries */
int  print_zeros;	/* If TRUE, print entries with 0-counts */
{
   FILE *fp_ops;

   sort_ops_by_freq (spec_op_cnt);
   fp_ops = open_op_freq_file (name, ".sops1");
   print_sort_ops (fp_ops, num_print, print_zeros, TRUE);
   fclose (fp_ops);

   sort_ops_by_freq (perf_op_cnt);
   fp_ops = open_op_freq_file (name, ".pops1");
   print_sort_ops (fp_ops, num_print, print_zeros, FALSE);
   fclose (fp_ops);
}

/************************************************************************
 *									*
 *				open_op_freq_file			*
 *				-----------------			*
 *									*
 ************************************************************************/

FILE *open_op_freq_file (name, app)
char *name;
char *app;
{
   FILE *fp_ops;
   char buff[128];

   strcpy (buff, name);
   strcat (buff, app);

   fp_ops = fopen (buff, "w");
   if (!fp_ops) {
      fprintf (stderr, "Error:  Could not open \"%s\" for writing.\n", buff);
      exit (1);
   }

   return fp_ops;
}

/************************************************************************
 *									*
 *				print_sort_ops				*
 *				--------------				*
 *									*
 * Print opcode frequencies obtained using internal files produced by	*
 * daisy.								*
 *									*
 * Expects "spec_op_cnt" and "perf_op_cnt" arrays to be in descending	*
 * order by opcode frequency.						*
 *									*
 ************************************************************************/

print_sort_ops (fp, num_print, print_zeros, spec)
FILE *fp;
int  num_print;		/* If > 0, print only first "num_print" entries */
int  print_zeros;	/* If TRUE, print entries with 0-counts */
int  spec;		/* Boolean */
{
   int    i;
   int    opcode;
   int    limit;
   double cnt; 
   double skip_cnt; 
   double total_vliw_ops;
   OPCNT  *cnt_array;

   if (num_print > 0  &&  num_print <= NUM_OPCODES) limit = num_print;
   else limit = NUM_OPCODES;

   if (spec) {
      cnt_array = spec_op_cnt;
      skip_cnt  = skip_op_spec_cnt;
      total_vliw_ops = total_spec_ops;
   }
   else {
      cnt_array = perf_op_cnt;
      skip_cnt  = skip_op_perf_cnt;
      total_vliw_ops = total_perf_ops;
   }

   fprintf (fp, "%10s    %8.0f  (%5.1f%%)\n",
	    "TOTAL", total_vliw_ops, 100.0);

   for (i = 0; i < limit; i++) {

      cnt    = cnt_array[i].cnt;
      opcode = cnt_array[i].opcode;

      if (cnt < skip_cnt) {
         fprintf (fp, "%10s    %8.0f  (%5.1f%%)\n",
		  "SKIP", skip_cnt, (100.0 * skip_cnt) / total_vliw_ops);
	 skip_cnt = -1.0;
      }

      if (!print_zeros)
	 if (cnt == 0.0) break;

      fprintf (fp, "%10s    %8.0f  (%5.1f%%)\n",
	       op_map_table[opcode]->vliw_name,
	       cnt, (100.0 * cnt) / total_vliw_ops);
   }
}
