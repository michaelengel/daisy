/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_vliw
#define H_vliw

#include "max_resrcs.h"
#include "resrc_list.h"
#include "dis.h"
#include "rename.h"
#include "tree_pos.h"

#define MAX_VLIWS_PER_PAGE		65536		/* Was  65536 */
#define MAX_TIPS_PER_PAGE	        98304		/* Was 131072 */
#define MAX_BASE_OPS_PER_PAGE	       196608		/* Was 262144 */
#define MAX_PATHS_PER_PAGE		 6144		/* Was   8192 */

#define GPR_BITS		(MAX_VLIW_INT_REGS/WORDBITS)
#define FPR_BITS		(MAX_VLIW_FLT_REGS/WORDBITS)
#define CCR_BITS		(MAX_VLIW_CC_REGS/WORDBITS)

#define NUM_PPC_GPRS		32
#define NUM_PPC_GPRS_C		33	/* Including CTR as R32 */
#define NUM_PPC_GPRS_L		34	/* Including LR  as R33 */
#define NUM_PPC_FPRS		32
#define NUM_PPC_CCRS		32

#define NO_GPR_WRITERS		(-1)	/* On curr path in curr group */
#define MULT_GPR_WRITERS	(-3)	/* On curr path in curr group */

#define INDIR_BIT			 0x40
#define BR_ONPAGE			 0
#define BR_OFFPAGE			 1
#define BR_SVC				 2
#define BR_TRAP				 3
#define BR_INDIR_LR			(4|INDIR_BIT)
#define BR_INDIR_LR2			(5|INDIR_BIT)
#define BR_INDIR_CTR			(6|INDIR_BIT)
#define BR_INDIR_RFSVC			(7|INDIR_BIT)
#define BR_INDIR_RFI			(8|INDIR_BIT)

typedef struct _base_op {
   OPCODE2	   *op;
   struct _base_op *next;
   short	   commit;		/* Boolean */
   short	   cluster;
} BASE_OP;

typedef struct {
   short ccbit;				/* Which condition register bit  */
   short sense;				/* Boolean:  if ccbit TRUE/FALSE */
} COND;

/* Total number of ops in VLIW must be obeyed.  Not want to confuse
   ISSUE with real function unit types.
*/
#define	ALU				0
#define	MEMOP				1
#define	SKIP				2
#define	BRLEAF				3
#define	VTRAP				4
#define GPR_RD_PORTS			5
#define FPR_RD_PORTS			6
#define CCR_RD_PORTS			7
#define GPR_WR_PORTS			8
#define FPR_WR_PORTS			9
#define CCR_WR_PORTS			10
#define	NUM_VLIW_RESRC_TYPES		11
#define	ISSUE				NUM_VLIW_RESRC_TYPES

#define GPR				0
#define FPR				1
#define CCR				2

#define MAX_OPS_PER_VLIW		24
#define MAX_CLUSTERS			 8

typedef struct _vliw {
   int		 time;			  /* Offset from entry VLIW */
   int		 group;			  /* Group for each serialization pt */
   struct _tip	 *entry;
   unsigned	 *xlate;		  /* Addr of PPC Simcode for VLIW  */
   unsigned	 visited : 1;		  /* Used for iterating		   */
   int		 resrc_cnt[NUM_VLIW_RESRC_TYPES+1]; /* +1 Allow ISSUE ops  */
   short	 cluster[MAX_CLUSTERS][NUM_VLIW_RESRC_TYPES+1];
   short	 mem_update_cnt[NUM_PPC_GPRS_C]; /* Amts to Loop unroll cnt */
					  /* # of CAL instruc from LD/ST-UPD */
} VLIW;

typedef struct _tip {
   VLIW		  *vliw;		/* Info about the overall VLIW	  */
   BASE_OP	  *op;			/* Linked list:  In reverse order */
   struct _tip	  *prev;		/* Prev tip in VLIW. 1st VLIW: 0  */
   struct _tip	  *left;		/* Fall-thru tip in VLIW.  Null	  */
					/* if leaf.			  */
   struct _tip	  *right;		/* Branch target tip in VLIW.	  */
					/* Pts to succ VLIW if onpage leaf*/
					/* Pts to offpage addr if offpage */
   unsigned	  br_ins;		/* Original PPC branch instruc    */
   unsigned	  br_addr;		/* Original PPC branch addr       */
   TREE_POS	  tp;		        /* Root=1, LKid=2*par,RKid=2*par+1*/
   short	  num_ops;		/* # of elements in op linked list*/
   char		  br_type;		/* If leaf:  On/Offpage or indir  */
   char		  br_link;		/* Boolean:  Is branch-and-link?  */

					/* 32-byte boundary: Ease Debug   */

   unsigned	  gpr_vliw[GPR_BITS];	/* Track VLIW non-architected     */
   unsigned	  fpr_vliw[FPR_BITS];	/* register usage		  */
   unsigned	  ccr_vliw[CCR_BITS];

   REG_RENAME	  **gpr_rename;		/* Track register mappings	  */
   REG_RENAME	  **fpr_rename;		/* and commits.			  */
   REG_RENAME	  **ccr_rename;

   REG_RENAME	  ca_rename;
   REG_RENAME	  ov_rename;

/* MEMACC	  *stores[NUM_MA_HASHVALS]*//* Track STORES on curr path  */
   MEMACC	  **stores;

					    /* Next 2 used for ld motion: */
   int		  stm_r1_time;		    /* Track last STM to r1       */
   int		  last_store;		    /* Track last store on path	  */

/* unsigned short avail[NUM_MACHINE_REGS];*//* Track when each machine    */
   unsigned short *avail;		    /* register available.	  */
   unsigned char  *cluster;		    /* Cluster for each mach reg  */
   unsigned char  *loophdr;		    /* Track ins visited on path  */

						  /* Used for combining on   */
/* unsigned	  *gpr_writer[NUM_PPC_GPRS_C];*/ /* LD/STORE-UPDATE instruc */
   unsigned	  **gpr_writer;			  /* -1=nothing, -3=mult wr  */
   unsigned char  mem_update_cnt[NUM_PPC_GPRS_C];/* On this path thru VLIW  */

   double	  prob;			/* Est probability tip executes   */
   unsigned	  orig_ops;		/* Real PPC instrucs executed     */
   unsigned	  br_condbit;		/* Meaningless unless cond branch */
} TIP;

#endif
