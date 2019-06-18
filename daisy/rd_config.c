/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "max_resrcs.h"
#include "rd_config.h"
#include "opcode_fmt.h"
#include "gen_resrc.h"
#include "vliw.h"

#define MAX_CONFIG_FILE_SIZE		8192

int store_latency = 1;		/* If 0, need N*(N-1)/2 compares with LOADS */

int num_gprs     = 64;		/* Defaults if no configuration file */
int num_fprs     = 64;
int num_ccbits   = 64;
int num_ccfields;

int num_resrc[NUM_VLIW_RESRC_TYPES+1];
int bytes_per_vliw = 96;

int lv_fail_recomp_cnt  = 10;/* LOAD-VER fails this much ==> recomp page */

int max_till_ser_loop   = 2; /* If see loop entry 2 times, serialize it      */
int max_till_ser_other  = 2; /* If see diamond ----------------------->      */
int max_unroll_loop     = 4; /* If see loop entry 4 more times, goto ser. pt */
int max_duplic_other    = 4; /* If see diamond ----------------------------> */

int max_ins_per_group   = 0; /* 0 or neg ==> No limit		             */

int alu_loss_w_skip	= 0; /* Some archs go 15->13 or 7->6 ALU's if any skips
			         ==> Loss of   2    or  1		     */
			     /* Arch where go 15->13 ALU's if skips allows   */
int issue_gain[MAX_SKIPS];   /* o 16 ops with 0 skips (15 ALU, 1 br)  0 gain */
			     /* o 16 ops with 1 skips (13 ALU, 2 br)  0 gain */
			     /* o 18 ops with 2 skips (13 ALU, 3 br)  2 gain */
			     /* o 20 ops with 3 skips (13 ALU, 4 br)  4 gain */

int is_daisy1		  = 0;
int num_clusters          = 0;	       /* 0 ==> Not have clustered machine   */
int void_cluster_w_skips  = 7;         /* No ops in this cluster if skips    */
int cross_cluster_penalty = 1;
int cluster_resrc[NUM_VLIW_RESRC_TYPES+1] =  /* Bool: Reesource assoc with   */
	{1, 1, 0, 0, 0, 1, 1, 1};	     /*       clusters/whole machine */
int cluster_resrc_cnt[MAX_CLUSTERS][NUM_VLIW_RESRC_TYPES+1] =
       {{2, 1, 0, 0, 0, 6, 2, 2},	     /* 8 Cluster machine with	     */
        {2, 1, 0, 0, 0, 6, 2, 2},	     /* 2 ALU's and 1 Mem Unit	     */
        {2, 1, 0, 0, 0, 6, 2, 2},	     /* and at most 2 ops per	     */
        {2, 1, 0, 0, 0, 6, 2, 2},	     /* cluster.  6 reg read	     */
        {2, 1, 0, 0, 0, 6, 2, 2},	     /* ports, 2 reg write ports     */
        {2, 1, 0, 0, 0, 6, 2, 2},	     /* per cluster.		     */
        {2, 1, 0, 0, 0, 6, 2, 2},
        {2, 1, 0, 0, 0, 6, 2, 2}};

int reorder_cc_ops  = TRUE;
int do_load_motion  = TRUE;
int do_combining    = TRUE;
int do_copyprop     = TRUE;
int inline_ucond_br = TRUE;

unsigned rpagesize	= 4096;		/* 4096 byte pages */
unsigned rpagesize_mask = 4095;
unsigned rpage_entries  = 1024;
unsigned rpagesize_log  = 12;

#ifndef NOT_USING_PDF
double loop_back_prob  = 0.90;  /* Loop back edges are normally taken      */
double negint_cmp_prob = 0.20;  /* If val neg, this is likely an error chk */
double if_then_prob    = 0.40;	/* Fwd branch taken slightly less than not */
#else
double loop_back_prob  = 0.95;
double negint_cmp_prob = 0.20;
double if_then_prob    = 0.20;
#endif

/************************************************************************
 *									*
 *				read_config				*
 *				-----------				*
 *									*
 * Eventually should read the number of registers and function units	*
 * from a file.  For now just use defaults and make sure we don't	*
 * inadvertantly exceed maximum allowed.				*
 *									*
 ************************************************************************/

read_config ()
{
   int    fd;
   int    size;
   double identifier_value;
   char   *posn;
   char	  identifier_name[1024];
   char	  config_file[MAX_CONFIG_FILE_SIZE];

   set_default_resources ();

   fd = open_config_file (&size);

   if (fd != (-1)) {
      assert (size <= MAX_CONFIG_FILE_SIZE);
      assert (read (fd, config_file, size) == size);
      close (fd);

      for (posn = config_file; posn < config_file + size; ) {

	 if (sscanf (posn, "%s %lf", identifier_name, &identifier_value) == 2)
	    set_identifier_val (identifier_name, identifier_value);

	 while (*posn != '\n') posn++;
	 posn++;
      }

      if (rpagesize > MAX_RPAGESIZE) {
	 fprintf (stderr, "\nMaximum PowerPC page size is %u.\n", MAX_RPAGESIZE);
	 fprintf (stderr, "Increase MAX_RPAGESIZE and recompile if this is insufficient\n");
	 exit (1);
      }
      else {
	 rpagesize_mask = rpagesize -  1;
	 rpage_entries  = rpagesize >> 2 ;
	 rpagesize_log  = log2 (rpagesize);
      }

   }

   chk_num_resrcs   ();
   chk_cluster_data ();
   num_ccfields = num_ccbits / SIMCRF_SIZE;	/* 4 bits per CR field */
}

/************************************************************************
 *									*
 *				chk_cluster_data			*
 *				----------------			*
 *									*
 * Make sure all the cluster data is consistent and fits within		*
 * available limits.  Initialize the "cluster_resrc" array to FALSE	*
 * if "num_clusters" is 0.						*
 *									*
 ************************************************************************/

int chk_cluster_data ()
{
   int i, j;

   if (cross_cluster_penalty < 0) {
      fprintf (stderr, "\nCross cluster penalty = %d.  Must be non-negative\n",
	       cross_cluster_penalty);
      exit (1);
   }

   if (num_clusters < 0  ||  num_clusters > MAX_CLUSTERS) {
      fprintf (stderr, "\nBad number of clusters (%d) specified.\n",
	       num_clusters);
      fprintf (stderr, "Should be [0,%d].  0 means do not use clustering\n\n",
	       MAX_CLUSTERS);
      exit (1);
   }
   else if (num_clusters == 0) {
      for (i = 0; i <= NUM_VLIW_RESRC_TYPES; i++)
         cluster_resrc[i] = FALSE;
   }
   else {
      cluster_resrc[SKIP]   = FALSE;
      cluster_resrc[BRLEAF] = FALSE;
      cluster_resrc[VTRAP]  = FALSE;
      for (i = 0; i < num_clusters; i++) {
	 cluster_resrc_cnt[i][SKIP]   = 0;
	 cluster_resrc_cnt[i][BRLEAF] = 0;
	 cluster_resrc_cnt[i][VTRAP]  = 0;

	 for (j = 0; j <= NUM_VLIW_RESRC_TYPES; j++) {
	    if (cluster_resrc_cnt[i][j] < 0) {
	       fprintf (stderr, "Cluster counts must be non-negative\n");
	       exit (1);
	    }
	 }

	 if (cluster_resrc_cnt[i][ALU]   > cluster_resrc_cnt[i][ISSUE])
	    fprintf (stderr, "WARNING:  Cluster %d allows more ALU ops than ISSUE slots\n", i);

	 if (cluster_resrc_cnt[i][MEMOP] > cluster_resrc_cnt[i][ISSUE])
	    fprintf (stderr, "WARNING:  Cluster %d allows more MEM ops than ISSUE slots\n", i);
      }
   }
}

/************************************************************************
 *									*
 *				set_identifier_val			*
 *				------------------			*
 *									*
 ************************************************************************/

set_identifier_val (name, value)
char   *name;
double value;
{
   if	   (!strcmp (name, "num_gprs"))          num_gprs	    = value;
   else if (!strcmp (name, "num_fprs"))          num_fprs	    = value;
   else if (!strcmp (name, "num_ccbits"))        num_ccbits	    = value;

   else if (!strcmp (name, "num_gpr_rd_ports"))  num_resrc[GPR_RD_PORTS] = value;
   else if (!strcmp (name, "num_gpr_wr_ports"))  num_resrc[GPR_WR_PORTS] = value;
   else if (!strcmp (name, "num_fpr_rd_ports"))  num_resrc[FPR_RD_PORTS] = value;
   else if (!strcmp (name, "num_fpr_wr_ports"))  num_resrc[FPR_WR_PORTS] = value;
   else if (!strcmp (name, "num_ccr_rd_ports"))  num_resrc[CCR_RD_PORTS] = value;
   else if (!strcmp (name, "num_ccr_wr_ports"))  num_resrc[CCR_WR_PORTS] = value;

   else if (!strcmp (name, "num_skips"))         num_resrc[SKIP]    = value;
   else if (!strcmp (name, "num_mem"))           num_resrc[MEMOP]   = value;
   else if (!strcmp (name, "num_alus"))          num_resrc[ALU]	    = value;
   else if (!strcmp (name, "ops_per_vliw"))      num_resrc[ISSUE]   = value;
   else if (!strcmp (name, "bytes_per_vliw"))    bytes_per_vliw	    = value;
   else if (!strcmp (name, "pagesize"))          rpagesize	    = value;
   else if (!strcmp (name, "lv_fail_recomp_cnt"))lv_fail_recomp_cnt = value;
   else if (!strcmp (name, "max_till_ser_loop")) max_till_ser_loop  = value;
   else if (!strcmp (name, "max_till_ser_other"))max_till_ser_other = value;
   else if (!strcmp (name, "max_unroll_loop"))   max_unroll_loop    = value;
   else if (!strcmp (name, "max_duplic_other"))  max_duplic_other   = value;
   else if (!strcmp (name, "max_ins_per_group")) max_ins_per_group  = value;
   else if (!strcmp (name, "loop_back_prob"))    loop_back_prob	    = value;
   else if (!strcmp (name, "negint_cmp_prob"))   negint_cmp_prob    = value;
   else if (!strcmp (name, "if_then_prob"))      if_then_prob	    = value;
   else if (!strcmp (name, "reorder_cc_ops"))    reorder_cc_ops	    = value;
   else if (!strcmp (name, "do_load_motion"))    do_load_motion	    = value;
   else if (!strcmp (name, "do_combining"))      do_combining	    = value;
   else if (!strcmp (name, "do_copyprop"))       do_copyprop	    = value;
   else if (!strcmp (name, "inline_ucond_br"))   inline_ucond_br    = value;
   else if (!strcmp (name, "store_latency"))     store_latency      = value;
   else if (!strcmp (name, "alu_loss_w_skip"))	 alu_loss_w_skip    = value;
   else if (!strcmp (name, "is_daisy1"))	 is_daisy1	    = value;

   else if (!strcmp (name, "issue_gain_w_0sk"))  issue_gain[0]      = value;
   else if (!strcmp (name, "issue_gain_w_1sk"))  issue_gain[1]      = value;
   else if (!strcmp (name, "issue_gain_w_2sk"))  issue_gain[2]      = value;
   else if (!strcmp (name, "issue_gain_w_3sk"))  issue_gain[3]      = value;
   else if (!strcmp (name, "issue_gain_w_4sk"))  issue_gain[4]      = value;
   else if (!strcmp (name, "issue_gain_w_5sk"))  issue_gain[5]      = value;
   else if (!strcmp (name, "issue_gain_w_6sk"))  issue_gain[6]      = value;
   else if (!strcmp (name, "issue_gain_w_7sk"))  issue_gain[7]      = value;

   else if (!strcmp (name, "num_clusters"))      num_clusters	    = value;
   else if (!strcmp (name, "void_cluster_w_skips")) 
					       void_cluster_w_skips = value;

   else if (!strcmp (name, "cross_cluster_penalty"))
					     cross_cluster_penalty  = value;
   else if (!strcmp (name, "clustALU"))	     cluster_resrc[ALU]     = value;
   else if (!strcmp (name, "clustMEM"))	     cluster_resrc[MEMOP]   = value;
   else if (!strcmp (name, "clustISSUE"))    cluster_resrc[ISSUE]   = value;

   else if (!strcmp (name, "clustGPR_RDPORT"))
      cluster_resrc[GPR_RD_PORTS] = value;
   else if (!strcmp (name, "clustGPR_WRPORT"))
      cluster_resrc[GPR_WR_PORTS] = value;
   else if (!strcmp (name, "clustFPR_RDPORT"))
      cluster_resrc[FPR_RD_PORTS] = value;
   else if (!strcmp (name, "clustFPR_WRPORT"))
      cluster_resrc[FPR_WR_PORTS] = value;
   else if (!strcmp (name, "clustCCR_RDPORT"))
      cluster_resrc[CCR_RD_PORTS] = value;
   else if (!strcmp (name, "clustCCR_WRPORT"))
      cluster_resrc[CCR_WR_PORTS] = value;


   else if (!strcmp (name, "c1_numALU"))  cluster_resrc_cnt[0][ALU] = value;
   else if (!strcmp (name, "c2_numALU"))  cluster_resrc_cnt[1][ALU] = value;
   else if (!strcmp (name, "c3_numALU"))  cluster_resrc_cnt[2][ALU] = value;
   else if (!strcmp (name, "c4_numALU"))  cluster_resrc_cnt[3][ALU] = value;
   else if (!strcmp (name, "c5_numALU"))  cluster_resrc_cnt[4][ALU] = value;
   else if (!strcmp (name, "c6_numALU"))  cluster_resrc_cnt[5][ALU] = value;
   else if (!strcmp (name, "c7_numALU"))  cluster_resrc_cnt[6][ALU] = value;
   else if (!strcmp (name, "c8_numALU"))  cluster_resrc_cnt[7][ALU] = value;

   else if (!strcmp (name, "c1_numMEM"))  cluster_resrc_cnt[0][MEMOP] = value;
   else if (!strcmp (name, "c2_numMEM"))  cluster_resrc_cnt[1][MEMOP] = value;
   else if (!strcmp (name, "c3_numMEM"))  cluster_resrc_cnt[2][MEMOP] = value;
   else if (!strcmp (name, "c4_numMEM"))  cluster_resrc_cnt[3][MEMOP] = value;
   else if (!strcmp (name, "c5_numMEM"))  cluster_resrc_cnt[4][MEMOP] = value;
   else if (!strcmp (name, "c6_numMEM"))  cluster_resrc_cnt[5][MEMOP] = value;
   else if (!strcmp (name, "c7_numMEM"))  cluster_resrc_cnt[6][MEMOP] = value;
   else if (!strcmp (name, "c8_numMEM"))  cluster_resrc_cnt[7][MEMOP] = value;

   else if (!strcmp (name, "c1_numISSUE")) cluster_resrc_cnt[0][ISSUE] = value;
   else if (!strcmp (name, "c2_numISSUE")) cluster_resrc_cnt[1][ISSUE] = value;
   else if (!strcmp (name, "c3_numISSUE")) cluster_resrc_cnt[2][ISSUE] = value;
   else if (!strcmp (name, "c4_numISSUE")) cluster_resrc_cnt[3][ISSUE] = value;
   else if (!strcmp (name, "c5_numISSUE")) cluster_resrc_cnt[4][ISSUE] = value;
   else if (!strcmp (name, "c6_numISSUE")) cluster_resrc_cnt[5][ISSUE] = value;
   else if (!strcmp (name, "c7_numISSUE")) cluster_resrc_cnt[6][ISSUE] = value;
   else if (!strcmp (name, "c8_numISSUE")) cluster_resrc_cnt[7][ISSUE] = value;

   else if (!strcmp (name, "c1_numGPR_RDPORT"))
      cluster_resrc_cnt[0][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c2_numGPR_RDPORT"))
      cluster_resrc_cnt[1][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c3_numGPR_RDPORT"))
      cluster_resrc_cnt[2][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c4_numGPR_RDPORT"))
      cluster_resrc_cnt[3][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c5_numGPR_RDPORT"))
      cluster_resrc_cnt[4][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c6_numGPR_RDPORT"))
      cluster_resrc_cnt[5][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c7_numGPR_RDPORT"))
      cluster_resrc_cnt[6][GPR_RD_PORTS] = value;
   else if (!strcmp (name, "c8_numGPR_RDPORT"))
      cluster_resrc_cnt[7][GPR_RD_PORTS] = value;

   else if (!strcmp (name, "c1_numGPR_WRPORT"))
      cluster_resrc_cnt[0][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c2_numGPR_WRPORT"))
      cluster_resrc_cnt[1][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c3_numGPR_WRPORT"))
      cluster_resrc_cnt[2][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c4_numGPR_WRPORT"))
      cluster_resrc_cnt[3][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c5_numGPR_WRPORT"))
      cluster_resrc_cnt[4][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c6_numGPR_WRPORT"))
      cluster_resrc_cnt[5][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c7_numGPR_WRPORT"))
      cluster_resrc_cnt[6][GPR_WR_PORTS] = value;
   else if (!strcmp (name, "c8_numGPR_WRPORT"))
      cluster_resrc_cnt[7][GPR_WR_PORTS] = value;

   else if (!strcmp (name, "c1_numFPR_RDPORT"))
      cluster_resrc_cnt[0][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c2_numFPR_RDPORT"))
      cluster_resrc_cnt[1][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c3_numFPR_RDPORT"))
      cluster_resrc_cnt[2][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c4_numFPR_RDPORT"))
      cluster_resrc_cnt[3][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c5_numFPR_RDPORT"))
      cluster_resrc_cnt[4][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c6_numFPR_RDPORT"))
      cluster_resrc_cnt[5][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c7_numFPR_RDPORT"))
      cluster_resrc_cnt[6][FPR_RD_PORTS] = value;
   else if (!strcmp (name, "c8_numFPR_RDPORT"))
      cluster_resrc_cnt[7][FPR_RD_PORTS] = value;

   else if (!strcmp (name, "c1_numFPR_WRPORT"))
      cluster_resrc_cnt[0][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c2_numFPR_WRPORT"))
      cluster_resrc_cnt[1][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c3_numFPR_WRPORT"))
      cluster_resrc_cnt[2][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c4_numFPR_WRPORT"))
      cluster_resrc_cnt[3][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c5_numFPR_WRPORT"))
      cluster_resrc_cnt[4][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c6_numFPR_WRPORT"))
      cluster_resrc_cnt[5][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c7_numFPR_WRPORT"))
      cluster_resrc_cnt[6][FPR_WR_PORTS] = value;
   else if (!strcmp (name, "c8_numFPR_WRPORT"))
      cluster_resrc_cnt[7][FPR_WR_PORTS] = value;

   else if (!strcmp (name, "c1_numCCR_RDPORT"))
      cluster_resrc_cnt[0][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c2_numCCR_RDPORT"))
      cluster_resrc_cnt[1][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c3_numCCR_RDPORT"))
      cluster_resrc_cnt[2][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c4_numCCR_RDPORT"))
      cluster_resrc_cnt[3][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c5_numCCR_RDPORT"))
      cluster_resrc_cnt[4][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c6_numCCR_RDPORT"))
      cluster_resrc_cnt[5][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c7_numCCR_RDPORT"))
      cluster_resrc_cnt[6][CCR_RD_PORTS] = value;
   else if (!strcmp (name, "c8_numCCR_RDPORT"))
      cluster_resrc_cnt[7][CCR_RD_PORTS] = value;

   else if (!strcmp (name, "c1_numCCR_WRPORT"))
      cluster_resrc_cnt[0][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c2_numCCR_WRPORT"))
      cluster_resrc_cnt[1][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c3_numCCR_WRPORT"))
      cluster_resrc_cnt[2][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c4_numCCR_WRPORT"))
      cluster_resrc_cnt[3][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c5_numCCR_WRPORT"))
      cluster_resrc_cnt[4][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c6_numCCR_WRPORT"))
      cluster_resrc_cnt[5][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c7_numCCR_WRPORT"))
      cluster_resrc_cnt[6][CCR_WR_PORTS] = value;
   else if (!strcmp (name, "c8_numCCR_WRPORT"))
      cluster_resrc_cnt[7][CCR_WR_PORTS] = value;

   else {
      fprintf (stderr, "Unknown parameter:  %s  in \"daisy.config\" file\n",
	       name);
      exit (1);
   }
}

/************************************************************************
 *									*
 *				open_config_file			*
 *				----------------			*
 *									*
 * Open the configuration file "daisy.config".  First look in the	*
 * current directory.  If "daisy.config" does not exist, look in the	*
 * directory specified by the environment variable DAISY_CONFIG.  If	*
 * "daisy.config" does not exist there either, warn the user and use	*
 * default values.  Also return the size of the "daisy.config" file.	*
 *									*
 ************************************************************************/

int open_config_file (p_size)
int *p_size;				/* Output */
{
   int		fd;
   int		len;
   int		stat_val;
   struct stat	file_stat;
   char		*path;
   char		buff[1024];

   fd = open ("daisy.config", O_RDONLY);

   if (fd != -1) path = "daisy.config";
   else {
      path = getenv ("DAISY_CONFIG");

      if (!path) path = "daisy.config";
      else {
	 len = strlen (path);
	 if (path[len-1] != '/') {
	    strcpy (buff, path);
	    strcat (buff, "/");
            path = buff;
	 }
         strcat (path, "daisy.config");
      }


      fd = open (path, O_RDONLY);
      if (fd == -1) 
         fdprintf (stderr->_file, "WARNING:  Could not open \"daisy.config\".  Using default values\n");
   }

   if (fd != -1) {
      stat_val = stat (path, &file_stat);
      assert (stat_val == 0);
      *p_size = file_stat.st_size;
   }

   return fd;
}

/************************************************************************
 *									*
 *				chk_num_resrcs				*
 *				--------------				*
 *									*
 * Make sure that we are not specifying more resources than the .h	*
 * files made by "gen_resrc" allow.					*
 *									*
 ************************************************************************/

chk_num_resrcs ()
{
   if (num_gprs   > MAX_VLIW_INT_REGS ||
       num_fprs   > MAX_VLIW_FLT_REGS ||
       num_ccbits > MAX_VLIW_CC_REGS) {

      fprintf (stderr, "Exceeded maximum resource limits of DAISY translator.\n");
      fprintf (stderr, "%d GPR's   (%d allowed)\n", num_gprs,  MAX_VLIW_INT_REGS);
      fprintf (stderr, "%d FPR's   (%d allowed)\n", num_fprs,  MAX_VLIW_FLT_REGS);
      fprintf (stderr, "%d CCBIT's (%d allowed)\n", num_ccbits, MAX_VLIW_CC_REGS);
      fprintf (stderr, "\nPlease increase the values in \"max_resrcs.h\" and recompile\n");
      fprintf (stderr, "the DAISY translator\n\n");

      exit (1);
   }

   /* Assign number of LEAF BRANCHES based on SKIPS and the number of
      TRAPS based on ALU's.
   */
   num_resrc[BRLEAF] = num_resrc[SKIP] + 1;
   num_resrc[VTRAP]  = num_resrc[ALU];
}

static set_default_resources ()
{
   num_resrc[GPR_RD_PORTS] = 72;
   num_resrc[GPR_WR_PORTS] = 24;
   num_resrc[FPR_RD_PORTS] = 72;
   num_resrc[FPR_WR_PORTS] = 24;
   num_resrc[CCR_RD_PORTS] = 72;
   num_resrc[CCR_WR_PORTS] = 24;

   num_resrc[SKIP]    =  7;
   num_resrc[MEMOP]   =  8;
   num_resrc[ALU]     = 16;
   num_resrc[ISSUE]   = 24;

   bytes_per_vliw = 96;
}
