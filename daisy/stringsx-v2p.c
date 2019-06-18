/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*======================================================================*
 *									*
 * For a fuller explanation, see the .s files (lsx, lscbx, and stsx)	*
 * in the sx subdirectory.						*
 *									*
 * Tell "xlate_opcode" about the lsx/lscbx/stsx -- they should do	*
 * nothing.  We will dump out all the translated code.  Since lsx,... 	*
 * can modify any and all GPR's, we need to directly deal with it.	*
 * Unlike the other instructions (including lm, stm, and lsi),		*
 * we do not have a translation which could be inserted in the original	*
 * PPC code and function correctly.					*
 *									*
 *======================================================================*/

#include <stdio.h>
#include <assert.h>

#include "resrc_map.h"
#include "resrc_offset.h"
#include "dis.h"
#include "resrc_list.h"

extern RESRC_MAP *resrc_map[NUM_MACHINE_REGS];
extern RESRC_MAP real_resrc;
extern unsigned	 consec_ins_needed;

unsigned lsx_area[32];		/* Used by translations of lsx, lscbx, stsx
				   as intermediate storage for registers.
				*/

/************************************************************************
 *									*
 *				xlate_lsx_v2p				*
 *				-------------				*
 *									*
 ************************************************************************/

xlate_lsx_v2p (opcode)
OPCODE2  *opcode;
{
   int rt = opcode->op.renameable[RT & (~OPERAND_BIT)];
   int rz = opcode->op.renameable[RZ & (~OPERAND_BIT)];
   int rb = opcode->op.renameable[RB & (~OPERAND_BIT)];

   assert (rt < 32);

   lsx_lscbx_common1 (opcode->op.ins & 1, rz, rb, 0x15C, FALSE);
   lsx_loop ();
   lsx_lscbx_common2 (opcode->op.ins & 1, rt, rz, rb);

   return 0;			/* Keep translating along straightline path */
}

/************************************************************************
 *									*
 *				xlate_lscbx_v2p				*
 *				---------------				*
 *									*
 ************************************************************************/

xlate_lscbx_v2p (opcode)
OPCODE2  *opcode;
{
   int rt = opcode->op.renameable[RT & (~OPERAND_BIT)];
   int rz = opcode->op.renameable[RZ & (~OPERAND_BIT)];
   int rb = opcode->op.renameable[RB & (~OPERAND_BIT)];

   assert (rt < 32);

   lsx_lscbx_common1 (opcode->op.ins & 1, rz, rb, 0x170, TRUE);
   lscbx_loop ();
   lsx_lscbx_common2 (opcode->op.ins & 1, rt, rz, rb);

   return 0;			/* Keep translating along straightline path */
}

/************************************************************************
 *									*
 *				lsx_lscbx_common1			*
 *				-----------------			*
 *									*
 * When the number of bytes to be loaded in lscbx. is 0, i.e. when	*
 * XER25-31 is 0, the record bit is on, the value placed in CR0		*
 * is undefined (See p.39).  However, the memccpy routine assumes	*
 * that the equal bit is turned off in this case (i.e. no match		*
 * occurs if 0 bytes are loaded).  To accomodate this, we set the	*
 * equal bit (2) of CR0 accordingly.					*
 *									*
 ************************************************************************/

static lsx_lscbx_common1 (record_on, rz, rb, done_offset, inv_0chk)
unsigned record_on;
int	 rz;
int	 rb;
int	 done_offset;
int	 inv_0chk;
{
   int bump = load_rz_rb (&rz, &rb);

   if (!record_on) {			/* Record bit off => Save old val*/
      consec_ins_needed = 96;		/* A few more than we need      */
      dump (0x7fe00026);		/*   mfcr   r31			*/
      consec_ins_needed = 1;
      dump (0x93ed0000|CC_SAVE);	/*     st   r31,CC_SAVE(r13)	*/
      dump (0x7fc102a6);		/*  mfxer   r30			*/
   }
   else {
      dump (0x7fc102a6);		/*  mfxer   r30			*/
      consec_ins_needed = 1;
   }

   dump (0x57dd067e);			/*  rlinm   r29,r30,0,25,31	*/
   dump (0x2c1d0000);			/*   cmpi   cr0,r29,0		*/
   if (!record_on  ||  !inv_0chk) {
      consec_ins_needed = 1 + done_offset/4;
      dump (0x41820000|done_offset);	/*    beq   done		*/
      consec_ins_needed = 1;
   }
   else {
      dump (0x4c4211c2);		/* crnand   2,2,2		*/
      consec_ins_needed = 1 + done_offset/4;
      dump (0x40820000|done_offset);	/*    bne   done		*/
      consec_ins_needed = 1;
   }

   dump (0x7f8902a6);			/*  mfctr   r28			*/
   dump (0x938d0000|CTR_SAVE);		/*     st   r28,CTR_SAVE(r13)	*/

   dump (0x7fa903a6);			/*  mtctr   r29			*/

   if (rz == 0)
        dump (0x3BE00000|(rb<<16));	/*    cal   r31,0(r25)		*/
   else dump (0x7FE00214|(rz<<16)|(rb<<11));/*cax   r31,r25,r3		*/

   dump (0x838d0000|LSX_AREA);		/*      l   r28,LSX_AREA(r13)	*/
}

/************************************************************************
 *									*
 *				lscbx_loop				*
 *				----------				*
 *									*
 ************************************************************************/

static lscbx_loop ()
{
   dump (0x57ddc63e);			/*  rlinm   r29,r30,24,24,31	*/
   dump (0x7f5ad278);			/*    xor   r26,r26,r26		*/

/* main_loop:								*/
   dump (0x8b7f0000);			/*    lbz   r27,0(r31)		*/
   dump (0x3bff0001);			/*    cal   r31,1(r31)		*/
   dump (0x7c1be840);			/*   cmpl   cr0,r27,r29		*/
   dump (0x9b7c0000);			/*    stb   r27,0(r28)		*/
   dump (0x3b9c0001);			/*    cal   r28,1(r28)		*/
   dump (0x3b5a0001);			/*    cal   r26,1(r26)		*/
   dump (0x4002ffe8);			/*  bdnne   main_loop		*/

   dump (0x535e067e);			/*  rlimi   r30,r26,0,25,31	*/

   dump (0x3bfa0003);			/*    cal   r31,3(r26)	*/
}

/************************************************************************
 *									*
 *				lsx_loop				*
 *				--------				*
 *									*
 ************************************************************************/

static lsx_loop ()
{
/* main_loop:								*/
   dump (0x8b7f0000);			/*    lbz   r27,0(r31)		*/
   dump (0x3bff0001);			/*    cal   r31,1(r31)		*/
   dump (0x9b7c0000);			/*    stb   r27,0(r28)		*/
   dump (0x3b9c0001);			/*    cal   r28,1(r28)		*/
   dump (0x4202fff0);			/*     bd   main_loop		*/

   dump (0x3bfd0003);			/*    cal   r31,3(r29)		*/
}

/************************************************************************
 *									*
 *				lsx_lscbx_common2			*
 *				-----------------			*
 *									*
 ************************************************************************/

static lsx_lscbx_common2 (record_on, rt, rz, rb)
unsigned record_on;
int	 rt;
int	 rz;
int	 rb;
{
   int	      i;
   int	      dest;
   int	      offset;
   RESRC_MAP  *resrc;

   dump (0x57ff0838);			/*  rlinm   r31,r31,1,0,28	*/
   dump (0x23ff011c);			/*    sfi   r31,r31,284		*/

   /* XER CA bit can be corrupted by "sfi" so restore with init value	*/
   /* (updated for lscbx with number of bytes in string).		*/
   dump (0x7fc103a6);			/*  mtxer   r30			*/

   dump (0x48000005);			/*     bl   next_ins		*/
/* next_ins:								*/
   dump (0x7fc802a6);			/*   mflr   r30			*/
   dump (0x7ffff214);			/*    cax   r31,r31,r30		*/
   dump (0x7fe803a6);			/*   mtlr   r31			*/

   dump (0x83ed0000|CTR_SAVE);		/*      l   r31,CTR_SAVE(r13)	*/
   dump (0x7fe903a6);			/*  mtctr   r31			*/

   dump (0x83ed0000|LSX_AREA);		/*      l   r31,LSX_AREA(r13)	*/

   dump (0x4e800020);			/*     br			*/

   if (rt == 0) dest = 31;
   else dest = rt - 1;

   if (rz == 0) rz = 32;		/* Ok to write to rz if rz = 0	*/
   offset = 124;			/* From end to start		*/

   for (i = 0; i < 32; i++, offset -= 4) {

      resrc = resrc_map[R0 + dest];

      if (dest == rz  ||  dest == rb) {
	 dump (0x7fbdeb78);		/*     mr   r29,r29	(NOP)	*/
	 dump (0x7fbdeb78);		/*     mr   r29,r29	(NOP)	*/
      }
      else if (resrc == &real_resrc) {
	 dump (0x801f0000|(dest<<21)|offset);/* l   dest,offset(r31)	*/
	 dump (0x7fbdeb78);		/*     mr   r29,r29	(NOP)	*/
      }
      else {
	 dump (0x83df0000|offset);	/*      l   r30,offset(r31)	*/
	 dump (0x93cd0000|resrc->offset);/*    st   r30,offset(r13)	*/
      }

      if (dest == 0) dest = 31;
      else dest--;
   }

/* done:								*/
   if (!record_on) {			/* Record off => Restore old val*/
      dump (0x83ed0000|CC_SAVE);	/*      l   r31,CC_SAVE(r13)	*/
      dump (0x7feff120);		/*   mtcr   r31			*/
   }
   else {
      dump (0x7FE00026);		/*   mfcr   r31			*/
      dump (0x93E00000 | (RESRC_PTR<<16)/*     st   r31,CRF0(r13)	*/
		       | CRF0_OFFSET);
   }
}

/************************************************************************
 *									*
 *				xlate_stsx_v2p				*
 *				--------------				*
 *									*
 ************************************************************************/

xlate_stsx_v2p (opcode)
OPCODE2  *opcode;
{
   int bump;
   int rs = opcode->op.renameable[RS & (~OPERAND_BIT)];
   int rz = opcode->op.renameable[RZ & (~OPERAND_BIT)];
   int rb = opcode->op.renameable[RB & (~OPERAND_BIT)];

   assert (rs < 32);

   bump = load_rz_rb (&rz, &rb);
   stsx1 (opcode->op.ins & 1, 0x15C);
   stsx2 (rs);
   stsx3 (opcode->op.ins & 1, rz, rb);

   return 0;			/* Keep translating along straightline path */
}

/************************************************************************
 *									*
 *				stsx1					*
 *				-----					*
 *									*
 ************************************************************************/

static stsx1 (record_on, done_offset)
unsigned record_on;
int	 done_offset;
{
   if (!record_on) {			/* Record bit off => Save old val*/
      dump (0x7fe00026);		/*   mfcr   r31			*/
      dump (0x93ed0000|CC_SAVE);	/*     st   r31,CC_SAVE(r13)	*/
      dump (0x7fc102a6);		/*  mfxer   r30			*/
   }
   else dump (0x7fc102a6);		/*  mfxer   r30			*/

   dump (0x57dd067e);			/*  rlinm   r29,r30,0,25,31	*/
   dump (0x2c1d0000);			/*   cmpi   cr0,r29,0		*/
   consec_ins_needed = done_offset / 4;
   dump (0x41820000|done_offset);	/*    beq   done		*/
   consec_ins_needed = 1;

   dump (0x3bfd0003);			/*    cal   r31,3(r29)		*/
   dump (0x57ff0838);			/*  rlinm   r31,r31,1,0,28	*/
   dump (0x23ff0114);			/*    sfi   r31,r31,276		*/

   /* XER CA bit can be corrupted by "sfi" so restore with init value	*/
   /* (updated for lscbx with number of bytes in string).		*/
   dump (0x7fc103a6);			/*     mtxer   r30		*/

   dump (0x48000005);			/*     bl   next_ins		*/
/* next_ins:								*/
   dump (0x7fc802a6);			/*   mflr   r30			*/
   dump (0x7ffff214);			/*    cax   r31,r31,r30		*/
   dump (0x7fe803a6);			/*   mtlr   r31			*/

   dump (0x83ed0000|LSX_AREA);		/*      l   r31,LSX_AREA(r13)	*/

   dump (0x4e800020);			/*     br			*/
}

/************************************************************************
 *									*
 *				stsx2					*
 *				-----					*
 *									*
 ************************************************************************/

static stsx2 (rs)
int	 rs;
{
   int	      i;
   int	      src;
   int	      offset;
   RESRC_MAP  *resrc;

   if (rs == 0) src = 31;
   else src = rs - 1;

   offset = 124;			/* From end to start		*/

   for (i = 0; i < 32; i++, offset -= 4) {

      resrc = resrc_map[R0 + src];

      if (resrc == &real_resrc) {
	 dump (0x901f0000|(src<<21)|offset);/* st   src,offset(r31)	*/
	 dump (0x7fbdeb78);		    /* mr   r29,r29    (NOP)	*/
      }
      else {
	 dump (0x83cd0000|resrc->offset);/*     l   r30,offset(r13)	*/
	 dump (0x93df0000|offset);	/*     st   r30,offset(r31)	*/
      }

      if (src == 0) src = 31;
      else src--;
   }
}

/************************************************************************
 *									*
 *				stsx3					*
 *				-----					*
 *									*
 ************************************************************************/

static stsx3 (record_on, rz, rb)
unsigned record_on;
int	 rz;
int	 rb;
{
   dump (0x7f8902a6);			/*  mfctr   r28			*/
   dump (0x938d0000|CTR_SAVE);		/*     st   r28,CTR_SAVE(r13)	*/

   dump (0x7fa903a6);			/*  mtctr   r29			*/

   if (rz == 0)
        dump (0x3BE00000|(rb<<16));	/*    cal   r31,0(r25)		*/
   else dump (0x7FE00214|(rz<<16)|(rb<<11));/*cax   r31,r25,r3		*/

   dump (0x838d0000|LSX_AREA);		/*      l   r28,LSX_AREA(r13)	*/

/* main_loop:								*/
   dump (0x8b7c0000);			/*    lbz   r27,0(r28)		*/
   dump (0x3b9c0001);			/*    cal   r28,1(r28)		*/
   dump (0x9b7f0000);			/*    stb   r27,0(r31)		*/
   dump (0x3bff0001);			/*    cal   r31,1(r31)		*/
   dump (0x4202fff0);			/*     bd   main_loop		*/

   dump (0x83ed0000|CTR_SAVE);		/*      l   r31,CTR_SAVE(r13)	*/
   dump (0x7fe903a6);			/*  mtctr   r31			*/

/* done:								*/
   if (!record_on) {			/* Record off => Restore old val*/
      dump (0x83ed0000|CC_SAVE);	/*      l   r31,CC_SAVE(r13)	*/
      dump (0x7feff120);		/*   mtcr   r31			*/
   }
   else {
      dump (0x7FE00026);		/*   mfcr   r31			*/
      dump (0x93E00000 | (RESRC_PTR<<16)/*     st   r31,CRF0(r13)	*/
		       | CRF0_OFFSET);
   }
}

/************************************************************************
 *									*
 *				load_rz_rb				*
 *				----------				*
 *									*
 * The address registers, RA/RZ and RB may be real PPC registers or	*
 * be memory mapped.  If memory mapped, they must be loaded into	*
 * real registers (except RA when RA=0).  This routine does so,		*
 * putting RA in r27 and RB in r26.  (Registers r26 and r27 are		*
 * assumed to be memory mapped and hence safe.)  If RA and/or RB	*
 * must be loaded an additional instruction is needed and the		*
 * distance from this point to the end of the lsx/lscbx/stsx code	*
 * is correspondingly increased.					*
 *									*
 * RETURNS:  The increase in code size from loading RZ/RB.		*
 *	     At "p_rz", 27 if RZ me mapped and val in r27, ow unchanged	*
 *	     At "p_rb", 26 if RB me mapped and val in r26, ow unchanged	*
 *									*
 ************************************************************************/

static int load_rz_rb (p_rz, p_rb)
int *p_rz;
int *p_rb;
{
   int	     bump;
   int	     rz = *p_rz;
   int	     rb = *p_rb;
   RESRC_MAP *resrc_z;
   RESRC_MAP *resrc_b;

   /* Insure where we put RB and RZ are memory mapped. */
   assert (resrc_map[R0 + 26] != &real_resrc);		/* RB */
   assert (resrc_map[R0 + 27] != &real_resrc);		/* RZ */

   resrc_b = resrc_map[R0 + rb];
   if (resrc_b == &real_resrc) bump = 0;
   else {
      bump  = 4;
      *p_rb = 26;
      dump (0x834d0000|resrc_b->offset);/*      l   r26,RB_OFFSET(r13)	*/
   }

   if (rz != 0) {
      resrc_z = resrc_map[R0 + rz];
      if (resrc_z != &real_resrc) {
	 bump +=  4;
	 *p_rz = 27;
         dump (0x836d0000|resrc_z->offset); /*  l   r27,RZ_OFFSET(r13)	*/
      }
   }

   return bump;
}
