/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_max_resrcs
#define H_max_resrcs

/* These define the maximum number of resources that the DAISY
   translator can handle.  If more are needed, these should be increased
   and the DAISY translator, recompiled.  We define SMALL_MACHINE_MODEL
   which reduces the amount of memory needed by the DAISY translator
   and helps to be able to compile large programs such as gcc.  However,
   SMALL_MACHINE_MODEL is probably representative of what can be built
   in the near future.
*/
#ifdef SMALL_MACHINE_MODEL

#define MAX_VLIW_INT_REGS		64
#define MAX_VLIW_FLT_REGS		64
#define MAX_VLIW_CC_REGS		64	/* Actually CC bits */
#define MAX_VLIW_SEG_REGS		16

#else

/* Using more than 64 registers is somewhat flakey currently */

#define MAX_VLIW_INT_REGS		256
#define MAX_VLIW_FLT_REGS		256
#define MAX_VLIW_CC_REGS		256	/* Actually CC bits */
#define MAX_VLIW_SEG_REGS		16

#endif

#define MAX_SKIPS		        7	/* Per VLIW */

#endif
