/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "resrc_map.h"
#include "dis.h"
#include "resrc_list.h"
#include "vliw.h"

int doing_load_mult;	/* Flag used in ld_motion.c to prevent motion */

/************************************************************************
 *									*
 *				xlate_mult_p2v				*
 *				--------------				*
 *									*
 * Translate PPC op with vector (multiple) destination or source	*
 * registers to VLIW.  This means finding the appropriate VLIW		*
 * instruction(s) in which to place the PPC operation and perhaps	*
 * their commits.							*
 *									*
 ************************************************************************/

int xlate_mult_p2v (ptip, opcode, ins, ins_ptr, start, end)
TIP	 **ptip;
OPCODE1  *opcode;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   if      (opcode->b.primary == 46) 
      return xlate_lm  (ptip, ins, ins_ptr, start, end);
   else if (opcode->b.primary == 47) 
      return xlate_stm (ptip, ins, ins_ptr, start, end);

   else if (opcode->b.primary != 31) {
      fprintf (stderr, "Unexpected ins (0x%08x) in xlate_mult\n", ins);
      exit (1);
   }
   else {
      switch (opcode->b.ext) {
	 case 0x4AA:
	 case 0x4AB:
	    return xlate_lsi       (ptip, ins, ins_ptr, start, end);
	    break;

	 case 0x5AA:
	 case 0x5AB:
	    return xlate_stsi      (ptip, ins, ins_ptr, start, end);
	    break;

	 case 0x42A:
	 case 0x42B:  
	    return xlate_lsx_p2v   (ptip, ins, ins_ptr, end, opcode);
	    break;

	 case 0x22A:
	 case 0x22B:
	    return xlate_lscbx_p2v (ptip, ins, ins_ptr, end, opcode);
	    break;

	 case 0x52A:
	 case 0x52B:
	    return xlate_stsx_p2v  (ptip, ins, ins_ptr, end, opcode);
	    break;

	 default:     
	    fprintf (stderr, "Unexpected ins (0x%08x) in xlate_mult\n", ins);
	    exit (1);
      }
   }
}

/************************************************************************
 *									*
 *				xlate_lm				*
 *				--------				*
 *									*
 ************************************************************************/

int xlate_lm (ptip, ins, ins_ptr, start, end)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    rval;
   int	    bump;
   int	    rt       = (ins >> 21) & 0x1F;
   int	    ra       = (ins >> 16) & 0x1F;
   int	    offset   =  (int) (((short) (ins & 0xFFFF)));
   unsigned new_ins  = 0x80000000 | (ins & 0x3FFFFFF);     /*   l RT,D(RA)   */
   OPCODE1  *opcode;
   OPCODE2  *opcode_out;

   doing_load_mult = TRUE;

   bump = lm_stm_bump_up (ptip, rt, ra, offset, ins_ptr, start, end);

   opcode = get_opcode (new_ins);

   if (ra == 0) ra = 32;		/* If RA=0, always want RT written */
   for (; rt < 32; offset += 4) {
      if (rt != ra) {
	 opcode_out = set_operands (opcode, new_ins, opcode->b.format);
	 rval = xlate_opcode_p2v (ptip, opcode_out, new_ins, ins_ptr,
				  start, end);
      }
      new_ins += 4;
      new_ins &= 0xFC1FFFFF;
      new_ins |= (++rt) << 21;
   }

   if (bump) rval = lm_stm_bump_down (ptip, offset, ra, ins_ptr, start, end);

   return rval;
}

/************************************************************************
 *									*
 *				xlate_stm				*
 *				---------				*
 *									*
 ************************************************************************/

int xlate_stm (ptip, ins, ins_ptr, start, end)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    rval;
   int	    bump;
   int	    rs      = (ins >> 21) & 0x1F;
   int	    ra      = (ins >> 16) & 0x1F;
   int	    offset  =  (int) (((short) (ins & 0xFFFF)));
   unsigned new_ins = 0x90000000 | (ins & 0x3FFFFFF);      /* st RT,D(RA) */
   OPCODE1  *opcode;
   OPCODE2  *opcode_out;

   bump = lm_stm_bump_up (ptip, rs, ra, offset, ins_ptr, start, end);

   opcode = get_opcode (new_ins);

   for (; rs < 32; offset += 4) {

      opcode_out = set_operands (opcode, new_ins, opcode->b.format);
      rval = xlate_opcode_p2v (ptip, opcode_out, new_ins, ins_ptr,
			       start, end);

      new_ins += 4;
      new_ins &= 0xFC1FFFFF;
      new_ins |= (++rs) << 21;
   }

   if (bump) rval = lm_stm_bump_down (ptip, offset, ra, ins_ptr, start, end);

   /* Heuristic:  Most STM instructions on Power involve R1, the stack */
   if (ra == 1) (*ptip)->stm_r1_time = (*ptip)->vliw->time;

   return rval;
}

/************************************************************************
 *									*
 *				lm_stm_bump_up				*
 *				--------------				*
 *									*
 * Returns TRUE if the RA register was bumped by 128, FALSE otherwise.	*
 *									*
 * NOTE:  128 must match "lm_stm_bump_down".				*
 *									*
 ************************************************************************/

int lm_stm_bump_up (ptip, rt, ra, offset, ins_ptr, start, end)
TIP	 **ptip;
int	 rt;
int	 ra;
int	 offset;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    earliest;
   int	    bump = FALSE;
   OPCODE1  *opcode;
   OPCODE2  *opcode_out;
   TIP	    *ins_tip;
   unsigned add_ins = 0x38000000|(ra<<21)|(ra<<16)|128;  /* cal RA,128(RA) */

   if (offset > 0x8000 - 4 * (32 - rt)) {
      if (ra == 0) {
	 fprintf (stderr, "INTERNAL LIMIT Offset %d in \"lm\" instr exceeded allowed 32764\n",
		  offset);
	 fprintf (stderr, "Translator must be fixed to handle this program\n");
	 exit (1);
      }
      else {
         opcode = get_opcode (add_ins);
         opcode_out = set_operands (opcode, add_ins, opcode->b.format);
         earliest = get_earliest_time (*ptip, opcode_out, FALSE);
         *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

         bump = TRUE;
      }
   }

   return bump;
}

/************************************************************************
 *									*
 *				lm_stm_bump_down			*
 *				----------------			*
 *									*
 *									*
 * Restores the RA register to the value it had prior to bump up, i.e.	*
 * 128 is subtracted to the contents of RA.  (This must match		*
 * "lm_stm_bump_up".							*
 *									*
 * Returns "rval" from decrement operation.				*
 *									*
 * NOTE:  Do not call xlate_opcode_p2v for CAL -- it will incorrectly	*
 *	  try and and combine with the CAL for bump_up.			*
 *									*
 ************************************************************************/

int lm_stm_bump_down (ptip, offset, ra, ins_ptr, start, end)
TIP	 **ptip;
int	 offset;
int	 ra;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    earliest;
   int	    rval;
   OPCODE1  *opcode;
   OPCODE2  *opcode_out;
   unsigned sub_ins;
   TIP	    *ins_tip;

   sub_ins = 0x38000000|(ra<<21)|(ra<<16)|(-128&0xFFFF); /* cal RA,-128(RA) */
   opcode = get_opcode (sub_ins);
   opcode_out = set_operands (opcode, sub_ins, opcode->b.format);

   earliest = get_earliest_time (*ptip, opcode_out, FALSE);
   *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

   if (ins_ptr == end) return 4;
   else		       return 0;
}

/************************************************************************
 *									*
 *				xlate_lsi				*
 *				---------				*
 *									*
 ************************************************************************/

int xlate_lsi (ptip, ins, ins_ptr, start, end)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    i;
   int	    rt      = (ins >> 21) & 0x1F;
   int	    ra      = (ins >> 16) & 0x1F;
   int	    nb      = (ins >> 11) & 0x1F;
   int	    nr;
   int	    old_rt;
   unsigned new_ins = 0x80000000 | (ins & 0x3FF0000);	     /* l RT,D(RA) */
   OPCODE1  *opcode;
   OPCODE2  *opcode_out;

   doing_load_mult = TRUE;

   opcode = get_opcode (new_ins);

   if (nb == 0) nb = 32;
   nr = (nb + 3) >> 2;
   if (nb & 3) nr--;			/* Do last 1-3 bytes special */

   if (ra == 0) ra = 32;		/* If RA=0, always want RT written */
   for (i = 0; i < nr; i++) {
      if (rt != ra) {
         opcode_out = set_operands (opcode, new_ins, opcode->b.format);
         xlate_opcode_p2v (ptip, opcode_out, new_ins, ins_ptr, start, end);
      }

      new_ins += 4;
      new_ins &= 0xFC1FFFFF;
      if (++rt == 32) rt = 0;
      new_ins |= rt << 21;
   }

   if (nb & 3) do_lsi_rlinm (ptip, rt, ra, nb, nb & 3, new_ins & 0xFFFF,
			     ins_ptr, start, end);

   if (ins_ptr == end) return 4;
   else		       return 0;
}

/************************************************************************
 *									*
 *				do_lsi_rlinm_old			*
 *				----------------			*
 *									*
 * Zero any bytes not loaded in the last word.				*
 *									*
 ************************************************************************/

do_lsi_rlinm_old (rt, ra, nb, nb_mod4, offset, ins_ptr, start, end)
int rt;
int ra;
int nb;
int nb_mod4;		/* Must be in range [0,3] */
int offset;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   unsigned	new_ins;
   OPCODE1	*opcode;
   OPCODE2	*opcode_out;

   if (rt != ra) switch (nb_mod4) {
      case 0:
	 break;

      case 1:	
      case 2:	
      case 3:
	 /* rlimn  RT, RT, 0, 8 * (nb_mod4), 31 */
	 new_ins = 0x5400003E | (rt<<21) | (rt<<16) | (nb_mod4<<9);

	 opcode = get_opcode (new_ins);
         opcode_out = set_operands (opcode, new_ins, opcode->b.format);
         xlate_opcode_p2v (opcode_out, new_ins, ins_ptr, start, end);

	 break;

      default:
	 fprintf (stderr, "Reached illegal condition in \"xlate_lsi\"\n");
	 exit (1);
   }
}

/************************************************************************
 *									*
 *				do_lsi_rlinm				*
 *				------------				*
 *									*
 * If the number of bytes loaded is not a multiple of 4, handle the	*
 * loads needed for the last "nb_mod4" bytes of the string.		*
 *									*
 * NOTE:  We do this without the need of additional registers and	*
 *	  without modifying the value in any registers (but at the	*
 *	  cost of one extra rlinm instruction).				*
 *									*
 * NOTE:  Do not call xlate_opcode_p2v for RLINM -- it will incorrectly	*
 *	  try and and combine the LOAD and the RLINM.			*
 *									*
 ************************************************************************/

do_lsi_rlinm (ptip, rt, ra, nb, nb_mod4, offset, ins_ptr, start, end)
TIP **ptip;
int rt;
int ra;
int nb;
int nb_mod4;		/* Must be in range [0,3] */
int offset;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int		earliest;
   unsigned	new_ins;
   OPCODE1	*opcode;
   OPCODE2	*opcode_out;
   TIP		*ins_tip;

   switch (nb_mod4) {
      case 0:
	fprintf (stderr, "Did not expect in nb_mod4=0 in \"xlate_lsi\"\n");
	exit (1);
	break;

      case 1:	
	/* lbz    RT,offset(RA) */
	new_ins = 0x88000000 | (rt<<21) | (ra<<16) | offset;
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* rlimn  RT, RT, 24, 0, 7 */
	new_ins = 0x5400C00E | (rt<<21) | (rt<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);

        earliest = get_earliest_time (*ptip, opcode_out, FALSE);
        *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);
	break;

      case 2:	
	/* lhz    RT,offset(RA) */
	new_ins = 0xa0000000 | (rt<<21) | (ra<<16) | offset;
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* rlimn  RT, RT, 16, 0, 15 */
	new_ins = 0x5400801E | (rt<<21) | (rt<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);

        earliest = get_earliest_time (*ptip, opcode_out, FALSE);
        *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

	break;

      case 3:
	if (nb > 3) {
	   /* l    RT,offset-1(RA) */
	   /* NOTE: This Load-word is guaranteed safe */
	   new_ins = 0x80000000 | (rt<<21) | (ra<<16) | (offset-1);
	   opcode = get_opcode (new_ins);
           opcode_out = set_operands (opcode, new_ins, opcode->b.format);
           xlate_opcode_p2v (ptip, opcode_out, new_ins, ins_ptr, start, end);

	   /* rlimn  RT, RT, 8, 0, 23 */
	   new_ins = 0x5400402E | (rt<<21) | (rt<<16);
	   opcode = get_opcode (new_ins);
           opcode_out = set_operands (opcode, new_ins, opcode->b.format);

	   earliest = get_earliest_time (*ptip, opcode_out, FALSE);
	   *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);
	}
        else {
	   /* l     RT,offset(RA)					*/
	   /* NOTE: This Load-word is NOT guaranteed safe.		*/
	   /*       DANGER if 3-byte string flush against segment end	*/
	   /*       ==> Segmentation violation				*/
	   new_ins = 0x80000000 | (rt<<21) | (ra<<16) | offset;
	   opcode = get_opcode (new_ins);
           opcode_out = set_operands (opcode, new_ins, opcode->b.format);
           xlate_opcode_p2v (ptip, opcode_out, new_ins, ins_ptr, start, end);

	   /* rlimn  RT, RT, 0, 0, 23 */
	   new_ins = 0x5400002E | (rt<<21) | (rt<<16);
	   opcode = get_opcode (new_ins);
           opcode_out = set_operands (opcode, new_ins, opcode->b.format);

	   earliest = get_earliest_time (*ptip, opcode_out, FALSE);
	   *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);
	}

	break;

      default:
	 fprintf (stderr, "Reached illegal condition in \"xlate_lsi\"\n");
	 exit (1);
   }
}

/************************************************************************
 *									*
 *				xlate_stsi				*
 *				----------				*
 *									*
 ************************************************************************/

int xlate_stsi (ptip, ins, ins_ptr, start, end)
TIP	 **ptip;
unsigned ins;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int	    i;
   int	    rs      = (ins >> 21) & 0x1F;
   int	    ra      = (ins >> 16) & 0x1F;
   int	    nb      = (ins >> 11) & 0x1F;
   int	    nr;
   int	    nb_mod4;
   unsigned new_ins = 0x90000000 | (ins & 0x3FF0000);	   /* st RT,D(RA) */
   OPCODE1  *opcode;
   OPCODE2  *opcode_out;

   if (nb == 0) nb = 32;
   nr = (nb + 3) >> 2;
   nb_mod4 = nb & 3;

   if (nb_mod4 != 0) nr--;

   opcode = get_opcode (new_ins);

   for (i = 0; i < nr; i++) {

      opcode_out = set_operands (opcode, new_ins, opcode->b.format);
      xlate_opcode_p2v (ptip, opcode_out, new_ins, ins_ptr, start, end);

      new_ins += 4;
      new_ins &= 0xFC1FFFFF;
      if (++rs == 32) rs = 0;
      new_ins |= rs << 21;
   }

   if (nb_mod4 != 0) do_stsi_rlinm (ptip, rs, ra, nb_mod4,
				    new_ins & 0xFFFF, ins_ptr, start, end);
   if (ins_ptr == end) return 4;
   else		       return 0;
}

/************************************************************************
 *									*
 *				do_stsi_rlinm				*
 *				-------------				*
 *									*
 * If the number of bytes stored is not a multiple of 4, handle the	*
 * stores needed for the last "nb_mod4" bytes of the string.		*
 *									*
 * NOTE:  We do this without the need of additional registers and	*
 *	  without modifying the value in any registers (but at the	*
 *	  cost of one extra rlinm instruction).				*
 *									*
 * NOTE:  Do not call xlate_opcode_p2v for 2nd or 3rd RLINM -- that	*
 *	  will incorrectly try and and combine the LOAD and the RLINM.	*
 *									*
 ************************************************************************/

do_stsi_rlinm (ptip, rs, ra, nb_mod4, offset, ins_ptr, start, end)
TIP **ptip;
int rs;
int ra;
int nb_mod4;		/* Must be in range [0,3] */
int offset;
unsigned *ins_ptr;
unsigned *start;
unsigned *end;
{
   int		earliest;
   unsigned	new_ins;
   OPCODE1	*opcode;
   OPCODE2	*opcode_out;
   TIP		*ins_tip;

   switch (nb_mod4) {
      case 0:
	 fprintf (stderr, "Did not expect in nb_mod4=0 in \"xlate_stsi\"\n");
	 exit (1);
	 break;

      case 1:	
	/* rlimn  RS, RS, 8, 0, 31 */
	new_ins = 0x5400403E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* stb    RS,offset(RA) */
	new_ins = 0x98000000 | (rs<<21) | (ra<<16) | offset;
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* rlimn  RS, RS, 24, 0, 31:  RESTORE RS to original value */
	new_ins = 0x5400C03E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);

        earliest = get_earliest_time (*ptip, opcode_out, FALSE);
        *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

	break;

      case 2:	
	/* rlimn  RS, RS, 16, 0, 31 */
	new_ins = 0x5400803E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* sth    RS,offset(RA) */
	new_ins = 0xb0000000 | (rs<<21) | (ra<<16) | offset;
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);


	/* rlimn  RS, RS, 16, 0, 31:  RESTORE RS to original value */
	new_ins = 0x5400803E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);

        earliest = get_earliest_time (*ptip, opcode_out, FALSE);
        *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

	break;

      case 3:
	/* rlimn  RS, RS, 16, 0, 31 */
	new_ins = 0x5400803E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* sth    RS,offset(RA) */
	new_ins = 0xb0000000 | (rs<<21) | (ra<<16) | offset;
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* rlimn  RS, RS, 8, 0, 31 */
	new_ins = 0x5400403E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        earliest = get_earliest_time (*ptip, opcode_out, FALSE);
        *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

	/* stb    RS,offset+2(RA) */
	new_ins = 0x98000000 | (rs<<21) | (ra<<16) | offset+2;
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        xlate_opcode_p2v(ptip, opcode_out, new_ins, ins_ptr, start, end);

	/* rlimn  RS, RS, 8, 0, 31:  RESTORE RS to original value */
	new_ins = 0x5400403E | (rs<<21) | (rs<<16);
	opcode = get_opcode (new_ins);
        opcode_out = set_operands (opcode, new_ins, opcode->b.format);
        earliest = get_earliest_time (*ptip, opcode_out, FALSE);
        *ptip = insert_op (opcode_out, *ptip, earliest, &ins_tip);

	break;

      default:
	 fprintf (stderr, "Reached illegal condition in \"xlate_stsi\"\n");
	 exit (1);
   }
}
