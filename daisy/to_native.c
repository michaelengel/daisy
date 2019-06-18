/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "resrc_offset.h"

extern unsigned char r13_area[];
extern unsigned *xlate_mem_ptr;

unsigned *dump_to_native (void);

/************************************************************************
 *									*
 *				dump_to_native				*
 *				--------------				*
 *									*
 * We dump out the following code, so that in conjunction with the -H	*
 * flag, we can switch to executing native code instead of translated	*
 * VLIW code.  However there is a fundamental problem.  We must use	*
 * either the PPC LINK REGISTER or the PPC CTR REGISTER to branch to	*
 * the native code.  However, both of these registers may be live in	*
 * the native program.  For example, if we use the LINK REGISTER and	*
 * the native program tries to return from a function call via the LINK	*
 * REGISTER, it could end up going to the same place we transferred	*
 * control.  Completely eliminating this problem seems unavoidable.	*
 * However the kluge below should work in all cases except when the	*
 * native program tries to return via the value in the link register	*
 * via a "bcrl" instruction, i.e. when the program tries to return and	*
 * set the link register at the same time, something that occurs quite	*
 * infrequently.  The idea is that when the native program branches to	*
 * the value in the LINK REGISTER, it will go to "link_jmp" below,	*
 * which in turn will branch to the value that the native program	*
 * expects to reside in the LINK REGISTER at this point.  After this,	*
 * the native program should run freely by itself.			*
 *									*
 * stub_rtn:	mtlr	r3						*
 *		l	r3,R3_OFFSET(r13)				*
 *		l	r13,R13_OFFSET(r13)				*
 *		bcrl	ALWAYS		     # Native pgm goes to next	*
 *					     # ins when goes to Linkreg	*
 *									*
 * link_jmp:	cal	r1,-256(r1)	     # Buy some room on stack	*
 *		st	r3,0(r1)					*
 *		bcrl	next_ins					*
 * next_ins:	mflr	r3		     # Addr of this ins in r3	*
 *		l	r3,24(r3)	     # Native linkreg val	*
 *		mtlr	r3						*
 *		l	r3,0(r1)	     # Restore r3		*
 *		cal	r1,256(r1)	     # Restore stack pointer	*
 *		bcr	ALWAYS						*
 * .long	<Native Linkreg Val>	     # Used by l r3,24(r3)	*
 *									*
 ************************************************************************/

unsigned *dump_to_native ()
{
   unsigned lr_val = *((unsigned *) &r13_area[LR_OFFSET]);
   unsigned *start = xlate_mem_ptr;

   dump (0x7C6803A6);				/* mtlr	 r3		*/
   dump (0x806D0000 | R3_OFFSET);      		/* l	 r3,0(r13)	*/
   dump (0x81AD0000 | R13_OFFSET);		/* l	 r13,0(r13)	*/
   dump (0x4E800021);				/* bcrl	 20,0		*/

   dump (0x3821FF00);				/* cal	 r1,-256(r1)	*/
   dump (0x90610000);				/* st	 r3,0(r1)	*/
   dump (0x48000005);				/* bl	 next_ins	*/
   dump (0x7C6802A6);				/* mflr	 r3		*/
   dump (0x80630018);				/* l	 r3,24(r3)	*/
   dump (0x7C6803A6);				/* mtlr	 r3		*/
   dump (0x80610000);				/* l	 r3,0(r1)	*/
   dump (0x38210100);				/* cal	 r1,256(r1)	*/
   dump (0x4E800020);				/* bcr	 20,0		*/

   dump (lr_val);				/* .long <LINKREG_VAL>	*/

   flush_cache (start, xlate_mem_ptr);

   return start;
}
