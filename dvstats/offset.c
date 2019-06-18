/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "offset.h"

/* We have ranges:  [0,0], [1,1], [2,3], [4,7], [8,15], ..., [2G,4G-1] */
/*		    and their negative counterparts.		       */
#define NUM_HISTO_RANGES      65
#define FIRST_POS_RANGE	      0
#define LAST_POS_RANGE	      32
#define FIRST_NEG_RANGE	      33
#define LAST_NEG_RANGE	      64

double *offset_perf_histo;
double *offset_spec_histo;

static double perf_histo[NUM_OFFSET_TYPES][NUM_HISTO_RANGES];
static double spec_histo[NUM_OFFSET_TYPES][NUM_HISTO_RANGES];

static char *offset_type_name[NUM_OFFSET_TYPES] = {
   "No Immediates                                 ",
   "MB / ME Fields      (10 bits)                 ",
   "SH / MB / ME Fields (15 bits)                 ",
   "ALU    Op           (16 bits -   Signed)      ",
   "ALU    Op           (16 bits - Unsigned)      ",
   "ALU    Op           (16 bits - Unsigned Upper)",
   "Load   Op           (16 bits -   Signed)      ",
   "Store  Op           (16 bits -   Signed)      ",
   "Cond   Branch       (16 bits -   Signed)      ",
   "Uncond Branch       (26 bits - Unsigned)      "
};

static FILE *open_offset_dump_file (char *, char *);

/************************************************************************
 *									*
 *				offset_init				*
 *				-----------				*
 *									*
 ************************************************************************/

offset_init ()
{
   int i;
   int j;

   offset_perf_histo = (double *) perf_histo;
   offset_spec_histo = (double *) spec_histo;

   for (i = 0; i < NUM_OFFSET_TYPES; i++) {
      for (j = 0; j < NUM_HISTO_RANGES; j++) {
         perf_histo[i][j] = 0.0;
         spec_histo[i][j] = 0.0;
      }
   }
}

/************************************************************************
 *									*
 *				offset_dump_histo			*
 *				-----------------			*
 *									*
 ************************************************************************/

offset_dump_histo (name)
char *name;
{
   offset_dump_histo_type (name, ".poff", perf_histo);
   offset_dump_histo_type (name, ".soff", spec_histo);
}

/************************************************************************
 *									*
 *				offset_dump_histo_type			*
 *				----------------------			*
 *									*
 ************************************************************************/

static offset_dump_histo_type (name, app, histo)
char   *name;
char   *app;
double histo[NUM_OFFSET_TYPES][NUM_HISTO_RANGES];
{
   int	i;
   FILE *fp;

   fp = open_offset_dump_file (name, app);

   for (i = 0; i < NUM_OFFSET_TYPES; i++) {
      fprintf (fp, "\n\n%s\n", offset_type_name[i]);
      fprintf (fp, "----------------------------------------------\n");
      offset_dump_single_histo (fp, histo[i]);
   }

   fclose (fp);
}

/************************************************************************
 *									*
 *				open_offset_dump_file			*
 *				---------------------			*
 *									*
 ************************************************************************/

static FILE *open_offset_dump_file (name, app)
char *name;
char *app;
{
   FILE *fp_histo;
   char buff[128];

   strcpy (buff, name);
   strcat (buff, app);

   fp_histo = fopen (buff, "w");
   if (!fp_histo) {
      fprintf (stderr, "Error:  Could not open \"%s\" for writing.\n", buff);
      exit (1);
   }

   return fp_histo;
}

/************************************************************************
 *									*
 *				offset_dump_single_histo		*
 *				------------------------		*
 *									*
 ************************************************************************/

static offset_dump_single_histo (fp, histo)
FILE   *fp;
double histo[NUM_HISTO_RANGES];
{
   int	  i;
   int	  base;
   int	  last_pos_nonzero;
   int	  first_neg_nonzero;
   double sum;
   double total = 0.0;

   last_pos_nonzero = -1;
   for (i = FIRST_POS_RANGE; i <= LAST_POS_RANGE; i++) {
      if (histo[i] != 0.0) {
	 last_pos_nonzero = i;
	 total += histo[i];
      }
   }

   first_neg_nonzero = -1;
   for (i = FIRST_NEG_RANGE; i <= LAST_NEG_RANGE; i++) {
      if (histo[i] != 0.0) {
	 total += histo[i];
	 if (first_neg_nonzero < 0) first_neg_nonzero = i;
      }
   }

   sum = 0.0;
   for (i = FIRST_POS_RANGE, base = 0; i <= last_pos_nonzero; i++) {
      sum += histo[i];
      fprintf (fp, "<= %12d:  %9.0f (%5.0f%%)\n",
	       base, histo[i], (100.0 * sum) / total);
      base = 2 * base + 1;
   }

   if (first_neg_nonzero < 0) return;
   else fprintf (fp, "------------------------------------\n");

   for (i = FIRST_NEG_RANGE, base = -2147483648; i < first_neg_nonzero; i++)
      base /= 2;

   assert (i == first_neg_nonzero);
   for (; i <= LAST_NEG_RANGE; i++) {
      sum += histo[i];
      fprintf (fp, ">= %12d:  %9.0f (%5.0f%%)\n",
	       base, histo[i], (100.0 * sum) / total);
      base /= 2;
   }
}

/************************************************************************
 *									*
 *				offset_histo_add			*
 *				----------------			*
 *									*
 ************************************************************************/

offset_histo_add (offset, histo, cnt)
unsigned offset;
double   histo[NUM_OFFSET_TYPES][NUM_HISTO_RANGES];   /* perf or spec */
double   cnt;
{
   int index;
   int type;
   int offset_val;

   type = offset >> 28;

   switch (type) {
      case NO_IMMEDS:
         return;

      case MB_ME:
         offset_val = (int) (offset & 0x03FF);
         break;

      case SH_MB_ME:
         offset_val = (int) (offset & 0x7FFF);
         break;

      case ALU_SIGNED16:
      case LD_SIGNED16:
      case ST_SIGNED16:
         offset_val = (int) ((short) (offset & 0xFFFF));
         break;

      case ALU_UNSIGNED16:
      case UP_UNSIGNED16:
         offset_val = (int) (offset & 0xFFFF);
         break;

      case BC_SIGNED14_16:
         offset_val = (int) ((short) (offset & 0xFFFC));
         break;

      case B_SIGNED24_26:
         if (offset & 0x02000000) {
	    offset_val = (int) (-(offset & 0x01FFFFFC));
	    if (offset_val == 0) offset_val = -33554432;
	 }
	 else offset_val = (int) (  offset & 0x01FFFFFC );
         break;

      default:
         assert (1 == 0);
	 break;
   }

   index = offset_histo_index (offset_val);

   histo[type][index] += cnt;
}

/************************************************************************
 *									*
 *				offset_histo_index			*
 *				------------------			*
 *									*
 ************************************************************************/

static int offset_histo_index (offset_val)
int offset_val;
{
   int rval;
   int cmp_val;

   if (offset_val < 0) return offset_histo_index_neg (offset_val);
   else for (rval = FIRST_POS_RANGE, cmp_val = 0;
	     cmp_val != 2147483647;
	     rval++) {
      if (offset_val <= cmp_val) return rval;
      else cmp_val = 2 * cmp_val + 1;
   }

   assert (1 == 0);
   return rval;
}

/************************************************************************
 *									*
 *				offset_histo_index_neg			*
 *				----------------------			*
 *									*
 ************************************************************************/

static int offset_histo_index_neg (offset_val)
int offset_val;
{
   int rval;
   int cmp_val;

   assert (offset_val < 0);

   for (rval = FIRST_NEG_RANGE, cmp_val = -1073741824;
	cmp_val <= 0;
	rval++) {
      if (offset_val < cmp_val) return rval;
      else cmp_val /= 2;
   }

   assert (1 == 0);
   return rval;
}

#ifdef DEBUGGING

main ()
{
   unsigned val;

   offset_init ();

   val = (ALU_SIGNED16 << 28) + 0;
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) + 1;
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) + 2;
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) + 3;
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) + 4;
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) | (0xFFFF);		 /* -1 */
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) | (0xFFFE);		 /* -2 */
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) | (0xFFFD);		 /* -3 */
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) | (0xFFFC);		 /* -4 */
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) | (0xFFFB);		 /* -5 */
   offset_histo_add (val, offset_perf_histo, 1.0);

   val = (ALU_SIGNED16 << 28) | (0xF000);		 /* -4096 */
   offset_histo_add (val, offset_perf_histo, 1.0);

   offset_dump_single_histo (stdout, perf_histo[ALU_SIGNED16]);
}

#endif
