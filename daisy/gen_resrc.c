/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "max_resrcs.h"
#include "vliw.h"
#include "gen_resrc.h"

#define NUM_XER_BITS			32
#define NUM_FPSCR_BITS			32
#define NUM_MSR_BITS			32

static int curr_resrc;
static int curr_offset;

static FILE *fp_list;
static FILE *fp_offset;
static FILE *fp_s;

/************************************************************************
 *									*
 *				main					*
 *				----					*
 *									*
 * Generates files:  resrc_list.h, resrc_offset.h, and r13_hdr.s.	*
 * Automatically generating these files:				*
 *									*
 *	(1) Lets us change the maximum number of VLIW registers easily	*
 *									*
 *	(2) Helps to maintain consistency between offsets from r13 in	*
 *	    C and assembly language files.				*
 *									*
 ************************************************************************/

main (argc, argv)
int  argc;
char *argv[];
{
   curr_resrc  = 0;
   curr_offset = 0;

   dump_resrc_list_hdr   ();
   dump_resrc_offset_hdr ();

   open_files  ();

   dump_ints	      (SIMGPR_SIZE);	/* Use 4 bytes per int	      */
   dump_flts	      (SIMFPR_SIZE);	/* Use 8 bytes per flt	      */
   dump_conds	      (SIMCRF_SIZE);	/* Use 4 bytes per cond field */

   dump_xer	      (1);		/* Use 1 byte  per XER   bit  */
   dump_fpscr	      (1);		/* Use 1 byte  per FPSCR bit  */
   dump_msr	      (1);		/* Use 1 byte  per FPSCR bit  */
   dump_segs	      (4);		/* Use 4 bytes per seg   reg  */
   dump_other	      ();
   dump_other_offsets ();

   dump_num_machine_regs ();

   /* Round up to multiple of 0x100 for ease of debugging */
   curr_offset = 0x100 * ((curr_offset + 0xFF) / 0x100);

   /* XER extender bits on GPR's are part of the GPR in the real machine */
   dump_int_xers      (SIMXER_SIZE);	/* Use 4 bytes per int XER ext   */

   /* FPSCR extender bits on FPR's are part of the FPR in the real machine */
   dump_flt_fpscrs    (SIMFPSCR_SIZE);	/* Use * bytes per int FPSCR ext   */

   /* Shadow registers are not real machine resources, but scratch
      memory used for simulation.
   */
   dump_shad_ints     (SIMGPR_SIZE);	/* Use 4 bytes per int	          */
   dump_shad_flts     (SIMFPR_SIZE);	/* Use 8 bytes per flt	          */
   dump_shad_conds    (SIMCRF_SIZE);	/* Use 4 bytes per cond field     */
   dump_shad_xers     (SIMXER_SIZE);	/* Use 4 bytes per xer extender   */
   dump_shad_fpscrs   (SIMFPSCR_SIZE);	/* Use 8 bytes per fpscr extender */

   dump_r13_area_size ();

   close_files ();

   dump_resrc_list_trailer   ();
   dump_resrc_offset_trailer ();

   fprintf (stderr, "Generated files \"resrc_list.h\", \"resrc_offset.h\", and \"r13_hdr.s\"\n\n");
   exit (0);
}

/************************************************************************
 *									*
 *				open_files				*
 *				----------				*
 *									*
 ************************************************************************/

open_files ()
{
   fp_list   = fopen ("resrc_list.h", "a");
   fp_offset = fopen ("resrc_offset.h", "a");
   fp_s      = fopen ("r13_hdr.s", "w");
}

/************************************************************************
 *									*
 *				close_files				*
 *				-----------				*
 *									*
 ************************************************************************/

close_files ()
{
   fclose (fp_list);
   fclose (fp_offset);
   fclose (fp_s);
}

/************************************************************************
 *									*
 *			dump_resrc_list_hdr				*
 *			-------------------				*
 *									*
 ************************************************************************/

dump_resrc_list_hdr ()
{
   system ("cp -f resrc_list.header resrc_list.h");
}

/************************************************************************
 *									*
 *			dump_resrc_list_trailer				*
 *			-----------------------				*
 *									*
 ************************************************************************/

dump_resrc_list_trailer ()
{
   system ("cat resrc_list.trailer >> resrc_list.h");
}

/************************************************************************
 *									*
 *			dump_resrc_offset_hdr				*
 *			---------------------				*
 *									*
 ************************************************************************/

dump_resrc_offset_hdr ()
{
   system ("cp -f resrc_offset.header resrc_offset.h");
}

/************************************************************************
 *									*
 *			dump_resrc_offset_trailer			*
 *			-------------------------			*
 *									*
 ************************************************************************/

dump_resrc_offset_trailer ()
{
   system ("cat resrc_offset.trailer >> resrc_offset.h");
}

/************************************************************************
 *									*
 *				dump_ints				*
 *				---------				*
 *									*
 ************************************************************************/

dump_ints (size)
int size;
{
   int i;

   fprintf (fp_s,      "\n# Integer Registers\n");

   for (i = 0; i < MAX_VLIW_INT_REGS; i++) {
      fprintf (fp_list,   "#define R%d\t\t\t\t%d\n", i, curr_resrc);
      fprintf (fp_offset, "#define R%d_OFFSET\t\t\t\t%3d\n", i, curr_offset);
      fprintf (fp_s,      ".set R%d_OFFSET,%d;", i, curr_offset);

      if (i & 1) fprintf (fp_s, "\n");
      else	 fprintf (fp_s, "\t");

      curr_resrc++;
      curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_shad_ints				*
 *				--------------				*
 *									*
 ************************************************************************/

dump_shad_ints (size)
int size;
{
   int i;

   fprintf (fp_list,   "\n/* Integer Shadow Registers (Simul Use Only) */\n");
   fprintf (fp_offset, "\n/* Integer Shadow Registers (Simul Use Only) */\n");
   fprintf (fp_s,      "\n# Integer Shadow Registers (Simul Use Only)\n");

   for (i = 0; i < MAX_VLIW_INT_REGS; i++) {
     fprintf (fp_list,   "#define SHDR%d\t\t\t\t%d\n", i, curr_resrc);
     fprintf (fp_offset, "#define SHDR%d_OFFSET\t\t\t\t%3d\n", i, curr_offset);
     fprintf (fp_s,      ".set SHDR%d_OFFSET,%d;", i, curr_offset);

     if (i & 1) fprintf (fp_s, "\n");
     else	fprintf (fp_s, "\t");

     curr_resrc++;
     curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_int_xers				*
 *				-------------				*
 *									*
 ************************************************************************/

dump_int_xers (size)
int size;
{
   int i;

   fprintf (fp_offset, "\n/* Integer XER Register Extender Bits */\n");
   fprintf (fp_s,      "\n# Integer XER Register Extender Bits\n");

   for (i = 0; i < MAX_VLIW_INT_REGS; i++) {
     fprintf (fp_offset, "#define R%d_XER_OFFSET\t\t\t\t%3d\n", i,curr_offset);
     fprintf (fp_s,      ".set R%d_XER_OFFSET,%d;", i, curr_offset);

     if (i & 1) fprintf (fp_s, "\n");
     else	fprintf (fp_s, "\t");

     curr_resrc++;
     curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_shad_xers				*
 *				--------------				*
 *									*
 ************************************************************************/

dump_shad_xers (size)
int size;
{
   int i;

   fprintf (fp_offset, "\n/* Integer XER Shadow Register Extender Bits */\n");
   fprintf (fp_s,      "\n# Integer XER Shadow Register Extender Bits\n");

   for (i = 0; i < MAX_VLIW_INT_REGS; i++) {
     fprintf (fp_offset, "#define SHDR%d_XER_OFFSET\t\t\t\t%3d\n", i,curr_offset);
     fprintf (fp_s,      ".set SHDR%d_XER_OFFSET,%d;", i, curr_offset);

     if (i & 1) fprintf (fp_s, "\n");
     else	fprintf (fp_s, "\t");

     curr_resrc++;
     curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_flts				*
 *				---------				*
 *									*
 ************************************************************************/

dump_flts (size)
int size;
{
   int i;

   fprintf (fp_list,   "\n/* Floating Point Registers (Simul Use Only) */\n");
   fprintf (fp_offset, "\n/* Floating Point Registers (Simul Use Only) */\n");
   fprintf (fp_s,      "\n# Floating Point Registers (Simul Use Only)\n");

   for (i = 0; i < MAX_VLIW_FLT_REGS; i++) {
      fprintf (fp_list,   "#define FP%d\t\t\t\t%d\n", i, curr_resrc);
      fprintf (fp_offset, "#define FP%d_OFFSET\t\t\t\t%3d\n", i, curr_offset);
      fprintf (fp_s,      ".set FP%d_OFFSET,%d;", i, curr_offset);

      if (i & 1) fprintf (fp_s, "\n");
      else	 fprintf (fp_s, "\t");

      curr_resrc++;
      curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_shad_flts				*
 *				--------------				*
 *									*
 ************************************************************************/

dump_shad_flts (size)
int size;
{
   int i;

   fprintf (fp_list,   "\n/* Floating Point Shadow Registers */\n");
   fprintf (fp_offset, "\n/* Floating Point Shadow Registers */\n");
   fprintf (fp_s,      "\n# Floating Point Shadow Registers\n");

   for (i = 0; i < MAX_VLIW_FLT_REGS; i++) {
      fprintf (fp_list,   "#define SHDFP%d\t\t\t\t%d\n", i, curr_resrc);
      fprintf (fp_offset, "#define SHDFP%d_OFFSET\t\t\t\t%3d\n", i, curr_offset);
      fprintf (fp_s,      ".set SHDFP%d_OFFSET,%d;", i, curr_offset);

      if (i & 1) fprintf (fp_s, "\n");
      else	 fprintf (fp_s, "\t");

      curr_resrc++;
      curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_flt_fpscrs				*
 *				---------------				*
 *									*
 ************************************************************************/

dump_flt_fpscrs (size)
int size;
{
   int i;

   fprintf (fp_offset, "\n/* Floating Point FPSCR Register Extender Bits */\n");
   fprintf (fp_s,      "\n# Floating Point FPSCR Register Extender Bits\n");

   for (i = 0; i < MAX_VLIW_FLT_REGS; i++) {
     fprintf (fp_offset, "#define FP%d_FPSCR_OFFSET\t\t\t%3d\n", i,curr_offset);
     fprintf (fp_s,      ".set FP%d_FPSCR_OFFSET,%d;", i, curr_offset);

     if (i & 1) fprintf (fp_s, "\n");
     else	fprintf (fp_s, "\t");

     curr_resrc++;
     curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_shad_fpscrs			*
 *				----------------			*
 *									*
 ************************************************************************/

dump_shad_fpscrs (size)
int size;
{
   int i;

   fprintf (fp_offset, "\n/* Floating Point FPSCR Shadow Register Extender Bits */\n");
   fprintf (fp_s,      "\n# Floating Point FPSCR Shadow Register Extender Bits\n");

   for (i = 0; i < MAX_VLIW_FLT_REGS; i++) {
     fprintf (fp_offset, "#define SHDFP%d_FPSCR_OFFSET\t\t\t\t%3d\n", i,curr_offset);
     fprintf (fp_s,      ".set SHDFP%d_FPSCR_OFFSET,%d;", i, curr_offset);

     if (i & 1) fprintf (fp_s, "\n");
     else	fprintf (fp_s, "\t");

     curr_resrc++;
     curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_conds				*
 *				----------				*
 *									*
 ************************************************************************/

dump_conds (size)
int size;
{
   int i;

   fprintf (fp_list,   "\n/* Condition Code Registers */\n");
   fprintf (fp_offset, "\n/* Condition Code Registers */\n");
   fprintf (fp_s,      "\n# Condition Code Registers\n");

   for (i = 0; i < MAX_VLIW_CC_REGS; i++) {
      fprintf (fp_list,   "#define CR%d\t\t\t\t%d\n", i, curr_resrc);
      curr_resrc++;
   }

   /* We keep the CCR stored as 4-bit fields */
   for (i = 0; i < MAX_VLIW_CC_REGS / 4; i++) {
      fprintf (fp_offset, "#define CRF%d_OFFSET\t\t\t\t%3d\n", i, curr_offset);
      fprintf (fp_s,      ".set CRF%d_OFFSET,%d;", i, curr_offset);

      if (i & 1) fprintf (fp_s, "\n");
      else	 fprintf (fp_s, "\t");

      curr_offset += size;
   }
}

/************************************************************************
 *									*
 *				dump_shad_conds				*
 *				---------------				*
 *									*
 ************************************************************************/

dump_shad_conds (size)
int size;
{
   int i;

   fprintf (fp_list,   "\n/* Condition Code Shadow Registers (Simul Use Only) */\n");
   fprintf (fp_offset, "\n/* Condition Code Shadow Registers (Simul Use Only) */\n");
   fprintf (fp_s,      "\n# Condition Code Shadow Registers (Simul Use Only)\n");

   for (i = 0; i < MAX_VLIW_CC_REGS; i++) {
      fprintf (fp_list,   "#define SHDCR%d\t\t\t\t%d\n", i, curr_resrc);
      curr_resrc++;
   }

   /* We keep the CCR stored as 4-bit fields */
   for (i = 0; i < MAX_VLIW_CC_REGS / 4; i++) {
      fprintf (fp_offset, "#define SHDCRF%d_OFFSET\t\t\t\t%3d\n", i, curr_offset);
      fprintf (fp_s,      ".set SHDCRF%d_OFFSET,%d;", i, curr_offset);

      if (i & 1) fprintf (fp_s, "\n");
      else	 fprintf (fp_s, "\t");

      curr_offset += size;
   }

   fprintf (fp_offset, "\n");
}

/************************************************************************
 *									*
 *				dump_xer				*
 *				--------				*
 *									*
 * Only put out resource list for now.  Assume that XER uses PPC XER.	*
 *									*
 ************************************************************************/

dump_xer (size)
int size;
{
   int i;

   fprintf (fp_list, "\n/* XER */\n");

   for (i = 0; i < NUM_XER_BITS; i++) {
      fprintf (fp_list,   "#define XER%d\t\t\t\t%d\n", i, curr_resrc);

      curr_resrc++;
   }
}

/************************************************************************
 *									*
 *				dump_fpscr				*
 *				----------				*
 *									*
 * Only put out resource list for now.  Assume FPSCR uses PPC FPSCR.	*
 *									*
 ************************************************************************/

dump_fpscr (size)
int size;
{
   int i;

   fprintf (fp_list, "\n/* Floating Point Status and Control Register */\n");

   for (i = 0; i < NUM_FPSCR_BITS; i++) {
      fprintf (fp_list,   "#define FPSCR%d\t\t\t\t%d\n", i, curr_resrc);

      curr_resrc++;
   }
}

/************************************************************************
 *									*
 *				dump_msr				*
 *				--------				*
 *									*
 * Only put out resource list for now.  Assume that MSR uses PPC MSR.	*
 *									*
 ************************************************************************/

dump_msr (size)
int size;
{
   int i;

   fprintf (fp_list, "\n/* MSR = Machine State Register */\n");

   for (i = 0; i < NUM_MSR_BITS; i++) {
      fprintf (fp_list,   "#define MSR%d\t\t\t\t%d\n", i, curr_resrc);

      curr_resrc++;
   }
}

/************************************************************************
 *									*
 *				dump_segs				*
 *				---------				*
 *									*
 * Only put out resource list for now.  Assume use PPC	segment regs.	*
 *									*
 ************************************************************************/

dump_segs (size)
int size;
{
   int i;

   fprintf (fp_list, "\n/* Segment Registers */\n");

   for (i = 0; i < MAX_VLIW_SEG_REGS; i++) {
      fprintf (fp_list,   "#define SR%d\t\t\t\t%d\n", i, curr_resrc);

      curr_resrc++;
   }
}

/************************************************************************
 *									*
 *				dump_other				*
 *				----------				*
 *									*
 ************************************************************************/

dump_other ()
{
   fprintf (fp_list, "\n/* Other registers */\n");

   fprintf (fp_list, "#define IAR\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define LR\t\t\t\t%d\n",     curr_resrc++);
   fprintf (fp_list, "#define LR2\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define RTCU\t\t\t\t%d\n",   curr_resrc++);
   fprintf (fp_list, "#define RTCL\t\t\t\t%d\n",   curr_resrc++);
   fprintf (fp_list, "#define DEC\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define TID\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define DSISR\t\t\t\t%d\n",  curr_resrc++);
   fprintf (fp_list, "#define DAR\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define SDR0\t\t\t\t%d\n",   curr_resrc++);
   fprintf (fp_list, "#define SDR1\t\t\t\t%d\n",   curr_resrc++);
   fprintf (fp_list, "#define SRR0\t\t\t\t%d\n",   curr_resrc++);
   fprintf (fp_list, "#define SRR1\t\t\t\t%d\n",   curr_resrc++);
   fprintf (fp_list, "#define EIM\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define EIS\t\t\t\t%d\n",    curr_resrc++);

   fprintf (fp_list, "#define SPRG0\t\t\t\t%d\n",  curr_resrc++);
   fprintf (fp_list, "#define SPRG1\t\t\t\t%d\n",  curr_resrc++);
   fprintf (fp_list, "#define SPRG2\t\t\t\t%d\n",  curr_resrc++);
   fprintf (fp_list, "#define SPRG3\t\t\t\t%d\n",  curr_resrc++);
   fprintf (fp_list, "#define ASR\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define EAR\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define TBL\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define TBU\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define PVR\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define IBAT0U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT0L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT1U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT1L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT2U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT2L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT3U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define IBAT3L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT0U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT0L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT1U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT1L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT2U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT2L\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT3U\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DBAT3L\t\t\t\t%d\n", curr_resrc++);

#ifdef CTR_IS_NOT_R32
   fprintf (fp_list, "#define CTR\t\t\t\t%d\n",    curr_resrc++);
#endif

#ifdef MQ_IS_NOT_R33
   fprintf (fp_list, "#define MQ\t\t\t\t%d\n",     curr_resrc++);
#endif

   fprintf (fp_list, "\n/* New for VLIW */\n");
   fprintf (fp_list, "#define GPR_VERIFY\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define FPR_VERIFY\t\t\t%d\n", curr_resrc++);

   fprintf (fp_list, "\n/* Current instruction address */\n");
   fprintf (fp_list, "#define INSADR\t\t\t\t%d\n", curr_resrc++);

   fprintf (fp_list, "\n/* Memory */\n");
   fprintf (fp_list, "#define MEM\t\t\t\t%d\t/* ==> TLB, PFT Modified */\n",
					           curr_resrc++);
   fprintf (fp_list, "#define ICACHE\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define DCACHE\t\t\t\t%d\n", curr_resrc++);
   fprintf (fp_list, "#define TLB\t\t\t\t%d\n",    curr_resrc++);
   fprintf (fp_list, "#define PFT\t\t\t\t%d\t/* Page Frame Table */\n",
					           curr_resrc++);
   fprintf (fp_list, "#define ATOMIC_RES\t\t\t%d\t/* For LWARX, STWCX */\n",
						   curr_resrc++);
}

/************************************************************************
 *									*
 *				dump_other_offsets			*
 *				------------------			*
 *									*
 * Many of these values are used to save registers when xlate_entry	*
 * is invoked for a new page while running the program.  These values	*
 * are used in the "xlate_entry_raw" routine of "r13.s"			*
 *									*
 ************************************************************************/

dump_other_offsets ()
{
   fprintf (fp_offset, "/* Linkreg offsets -- many used in r13.s */\n");
   fprintf (fp_s,      "\n# Linkreg offsets -- many used in resrc_list.h\n");

   fprintf (fp_offset, "#define LR_OFFSET\t\t\t\t%d\n",	        curr_offset);
   fprintf (fp_s,      ".set LR_OFFSET,%d;\t\t",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define LR2_OFFSET\t\t\t\t%d\n\n",      curr_offset);
   fprintf (fp_s,      ".set LR2_OFFSET,%d;\n",			curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "\n/* New for VLIW */\n");
   fprintf (fp_s,      "\n# New for VLIW\n");

   fprintf (fp_offset, "#define FPR_V_OFFSET\t\t\t\t%d\n",      curr_offset);
   fprintf (fp_s,      ".set FPR_V_OFFSET,%d;\n",		curr_offset);
   curr_offset += 8;
   fprintf (fp_offset, "#define GPR_V_OFFSET\t\t\t\t%d\n\n",    curr_offset);
   fprintf (fp_s,      ".set GPR_V_OFFSET,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define XER_SCR_OFFSET\t\t\t\t%d\n",    curr_offset);
   fprintf (fp_s,      ".set XER_SCR_OFFSET,%d;\n",		curr_offset);
   curr_offset += SIMXER_SIZE;

   fprintf (fp_offset, "#define FPSCR_SCR_OFFSET\t\t\t\t%d\n\n",curr_offset);
   fprintf (fp_s,      ".set FPSCR_SCR_OFFSET,%d;\n",		curr_offset);
   curr_offset += SIMFPSCR_SIZE;

   fprintf (fp_offset, "/* Other offsets -- many used in r13.s */\n");
   fprintf (fp_s,      "\n# Other offsets -- many used in resrc_list.h\n");

   fprintf (fp_offset, "#define PROG_START_OFFSET\t\t\t%d\n",   curr_offset);
   fprintf (fp_s,      ".set PROG_START_OFFSET,%d;\t",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define PROG_END_OFFSET\t\t\t\t%d\n",   curr_offset);
   fprintf (fp_s,      ".set PROG_END_OFFSET,%d;\n",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define TEXTCOPY_OFFSET\t\t\t\t%d\n\n", curr_offset);
   fprintf (fp_s,      ".set TEXTCOPY_OFFSET,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define MSR_OFFSET\t\t\t\t%d\n",        curr_offset);
   fprintf (fp_s,      ".set MSR_OFFSET,%d;\t\t",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define LR_PRIV\t\t\t\t\t%d\n\n",       curr_offset);
   fprintf (fp_s,      ".set LR_PRIV,%d;\n",			curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define CC_SAVE\t\t\t\t\t%d\n",	        curr_offset);
   fprintf (fp_s,      ".set CC_SAVE,%d;\t\t",			curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define XER_SAVE\t\t\t\t%d\n",	        curr_offset);
   fprintf (fp_s,      ".set XER_SAVE,%d;\n",			curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define CTR_SAVE\t\t\t\t%d\n",	        curr_offset);
   fprintf (fp_s,      ".set CTR_SAVE,%d;\t\t",			curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define MQ_SAVE\t\t\t\t\t%d\n",	        curr_offset);
   fprintf (fp_s,      ".set MQ_SAVE,%d;\n",			curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define XLATE_ENTRY_TOC\t\t\t\t%d\n",   curr_offset);
   fprintf (fp_s,      ".set XLATE_ENTRY_TOC,%d;\n",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define XLATED_PGM_TOC\t\t\t\t%d\n",    curr_offset);
   fprintf (fp_s,      ".set XLATED_PGM_TOC,%d;\t",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define XLATE_ENTRY_STACK\t\t\t%d\n\n", curr_offset);
   fprintf (fp_s,      ".set XLATE_ENTRY_STACK,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define LSX_AREA\t\t\t\t%d\n\n",        curr_offset);
   fprintf (fp_s,      ".set LSX_AREA,%d;\n",			curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define HASH_TABLE\t\t\t\t%d\n",        curr_offset);
   fprintf (fp_s,      ".set HASH_TABLE,%d;\t\t",		curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define XLATE_RAW\t\t\t\t%d\n",		curr_offset);
   fprintf (fp_s,      ".set XLATE_RAW,%d;\n",			curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define LDVER_FCN\t\t\t\t%d\n",		curr_offset);
   fprintf (fp_s,      ".set LDVER_FCN,%d;\n",			curr_offset);
   curr_offset += 4;
   fprintf (fp_offset, "#define PAGE_ENTRY\t\t\t\t%d\n\n",	curr_offset);
   fprintf (fp_s,      ".set PAGE_ENTRY,%d;\n",			curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define VLIW_PATH_CNTS\t\t\t\t%d\n",    curr_offset);
   fprintf (fp_s,      ".set VLIW_PATH_CNTS,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define LD_VER_FAIL_CNT\t\t\t\t%d\n",   curr_offset);
   fprintf (fp_s,      ".set LD_VER_FAIL_CNT,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define OFFPAGE_CNT\t\t\t\t%d\n",	curr_offset);
   fprintf (fp_s,      ".set OFFPAGE_CNT,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define INDIR_CTR_CNT\t\t\t\t%d\n",	curr_offset);
   fprintf (fp_s,      ".set INDIR_CTR_CNT,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define INDIR_LR_CNT\t\t\t\t%d\n",	curr_offset);
   fprintf (fp_s,      ".set INDIR_LR_CNT,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define INDIR_LR2_CNT\t\t\t\t%d\n",	curr_offset);
   fprintf (fp_s,      ".set INDIR_LR2_CNT,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define DYNPAGE_CNT\t\t\t\t%d\n",	curr_offset);
   fprintf (fp_s,      ".set DYNPAGE_CNT,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_offset, "#define DYNPAGE_LIM\t\t\t\t%d\n",	curr_offset);
   fprintf (fp_s,      ".set DYNPAGE_LIM,%d;\n",		curr_offset);
   curr_offset += 4;

   fprintf (fp_s,      ".set CTR_OFFSET,R32_OFFSET;\n");
   fprintf (fp_s,      ".set MQ_OFFSET,R33_OFFSET;\n");
}

/************************************************************************
 *									*
 *				dump_num_machine_regs			*
 *				---------------------			*
 *									*
 * NOTE:  We have simulation registers such as the shadow integer,	*
 *	  float, and condition code registers that are numbered in	*
 *	  conjunction with real registers, but which do not count	*
 *	  as part of "NUM_MACHINE_REGS".  They are accessed via r13	*
 *	  however, and must be included in the calculation of size	*
 *	  below in "dump_r13_area_size".				*
 *									*
 ************************************************************************/

dump_num_machine_regs ()
{
   fprintf (fp_list, "\n/* Everything -- use with BRANCH-AND-LINK for example */\n");
   fprintf (fp_list, "#define EVERYTHING\t\t\t%d\n\n", curr_resrc);

   fprintf (fp_list, "/* Total number of machine resources that instruction can read/write */\n");
   fprintf (fp_list, "#define NUM_MACHINE_REGS\t\t%d\n", curr_resrc++);
}

/************************************************************************
 *									*
 *				dump_r13_area_size			*
 *				------------------			*
 *									*
 * See note above in "dump_num_machine_regs".				*
 *									*
 ************************************************************************/

dump_r13_area_size ()
{
   fprintf (fp_offset, "#define R13_AREA_SIZE\t\t\t\t%d\n",     curr_offset);
   fprintf (fp_s,      ".set R13_AREA_SIZE,%d;\n",		curr_offset);

   /* We generate a lot of simulation code with <offset>(r13).
      If the <offset> is too big, this fails.
   */
   assert (curr_offset < 32768);
}

/************************************************************************
 *									*
 *				blank_line				*
 *				----------				*
 *									*
 ************************************************************************/

blank_line ()
{
   fprintf (fp_list,   "\n");
   fprintf (fp_offset, "\n");
   fprintf (fp_s,      "\n");
}
