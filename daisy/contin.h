/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#define UCFALL_THRU			0
#define CDFALL_THRU			1
#define CDLFALL_THRU			2
#define COND_TARG			3
#define UNCOND_TARG			4
#define OFFPAGE				0x80	/* Or-in if target offpage */

typedef struct {
   double	  prob;			/* Of executing "source" tip */
   TIP	          *source;		/* In VLIW code */
   unsigned	  *ppc_src;		/* In PowerPC code */
   unsigned	  *target;		/* In PowerPC code */
   int	          targ_type;		/* FALL_THRU, COND_TARG, UNCOND_TARG */
   int	          serialize;		/* Boolean      */
   unsigned short *avail;		/* Point to avail times for path   */
   unsigned char  *cluster;		/* Point to cluster of each reg	   */
   REG_RENAME     **gpr_rename;		/* Point to reg mappings for GPRS  */
   REG_RENAME     **fpr_rename;		/* Point to reg mappings for FPRS  */
   REG_RENAME     **ccr_rename;		/* Point to reg mappings for CCRS  */
   MEMACC	  **stores;		/* Point to STORES on this path    */
   int		  stm_r1_time;		/* Track last STM to r1            */
   int		  last_store;		/* Track last store on path	   */
   unsigned       **gpr_writer;		/* Who writes GPR's on this path,  */
					/* possibly none or multiple.      */
   unsigned char  *loophdr;		/* Page locns visited on this path */
   REG_RENAME	  ca_rename;	        /* GPR with CA result or CA        */
   REG_RENAME	  ov_rename;	        /* GPR with OV result or OV        */
} XLATE_POINT;
