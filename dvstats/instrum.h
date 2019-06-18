/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_instrum
#define H_instrum

/* Initially, we do not know what ops are duplicates, so give max possible
   if all operations are duplicated in each of the maximum 8 branches.
*/
/*#define MAX_VLIW_OPS			64*/	/* 8 branches * 8 ops */
/*#define MAX_VLIW_BRANCHES		 8*/

/*#define OPS_PER_VLIW			16*/


/* The three values above are for a real machine.  We need to be able
   to handle hypothetical machines with more function units.  Hence
   we'll octuple those values.  There are asserts in "vliw_operands.c"
   and "to_vliw.c" to insure that arrays of these sizes do not overflow.
*/

#define MAX_VLIW_OPS			512
#define MAX_VLIW_BRANCHES		64

#define OPS_PER_VLIW			128

#define CURRENT_SPEC_MAGIC_NUM		0xFeedBacd
#define CURRENT_PERF_MAGIC_NUM		0xFeedBacc
#endif
