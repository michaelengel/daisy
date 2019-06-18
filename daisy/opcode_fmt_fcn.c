/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/* TODO:
   1. Handle EVERYTHING
   2. Efficiently allocate memory for opcodes:
	Do we need the wr[] and rd[] fields in OPERAND_OUT?
*/

#include <stdio.h>
#include <assert.h>

#include "resrc_list.h"
#include "dis.h"
#include "dis_tbl.h"

/* Basically we only need one page (1024) worth of instructions.
   However, some instructions (e.g. lm, stm) expand to multiple
   instructions.  Thus we must allow room for the expansion.  This
   should be plenty.
*/
#define PPC_OPS_MEM_SIZE		131072

extern OPCODE1     *ppc_opcode[];
extern PPC_OP_FMT  ppc_op_fmt_base[];
extern int	   num_ppc_fmts;
extern int	   use_powerpc;
extern unsigned	   *curr_ins_addr;

       PPC_OP_FMT  *ppc_op_fmt[MAX_PPC_FORMATS];
static PPC_OP_FMT  *illegal_ppc_op_fmt;

       int  operand_val[NUM_PPC_OPERAND_TYPES];
       char operand_set[NUM_PPC_OPERAND_TYPES];

static OPCODE2	      ppc_ops_mem[PPC_OPS_MEM_SIZE];
static int	      ppc_ops_mem_index;
static RESULT	      wr_area[MAX_PAGE_MOD_RESRC];
static          char  wr_expl[MAX_PAGE_MOD_RESRC];
static unsigned short rd_area[MAX_PAGE_USE_RESRC];
static		char  rd_expl[MAX_PAGE_USE_RESRC];
static RESULT	      *ptr_wr = wr_area;
static unsigned short *ptr_rd = rd_area;
static FILE	      *curr_fp;

OPCODE2 *set_opcode_rd_wr  (OPCODE1 *, PPC_OP_FMT *);
OPCODE2 *set_operands (OPCODE1 *, unsigned, unsigned);
OPCODE2 *check_fxm_flm	   (OPCODE2 *);
OPCODE2 *check_mtctr_mfctr (OPCODE2 *);
unsigned extract_operand_val (unsigned, OPERAND *);

/************************************************************************
 *									*
 *				init_opcode_fmt				*
 *				---------------				*
 *									*
 ************************************************************************/

init_opcode_fmt ()
{
   int i;

   for (i = 0; i < MAX_PPC_FORMATS; i++)
      ppc_op_fmt[i] = illegal_ppc_op_fmt;

   for (i = 0; i < num_ppc_fmts; i++)
      ppc_op_fmt[ppc_op_fmt_base[i].template] = &ppc_op_fmt_base[i];
}

/************************************************************************
 *									*
 *				init_opcode_ptrs			*
 *				----------------			*
 *									*
 ************************************************************************/

init_opcode_ptrs ()
{
   ptr_wr = wr_area;
   ptr_rd = rd_area;
   ppc_ops_mem_index = 0;
}

/************************************************************************
 *									*
 *				set_operands				*
 *				------------				*
 *									*
 * Indicate all the machine resources that the current instruction	*
 * reads and writes.							*
 *									*
 ************************************************************************/

OPCODE2 *set_operands (opcode, ins, format)
OPCODE1	      *opcode;
unsigned      ins;
unsigned      format;
{
   int		i;
   int		num_operands;
   int		op_num;
   int		type;
   unsigned	val;
   char		*asm_map;
   OPERAND	*list;
   OPERAND	*operand;
   PPC_OP_FMT	*curr_format;
   OPCODE2      *opcode_out;
   int		type_list[MAX_PPC_OPERANDS];

   assert (format < MAX_PPC_FORMATS);

   curr_format = ppc_op_fmt[format];
   num_operands = curr_format->num_operands;

   asm_map = curr_format->ppc_asm_map;
   list    = curr_format->list;
   for (i = 0; i < num_operands; i++) {
/* Expand set_val inline for speed   */
/*    set_val (ins, curr_format, i); */

      op_num  = asm_map[i];
      operand = &list[op_num];
      val     = extract_operand_val (ins, operand);
      type    = operand->type;
      val     = chk_immed_val (val, type);

      type	       &= ~OPERAND_BIT;
      type_list[i]      = type;
      operand_val[type] = val;
      operand_set[type] = TRUE;
   }

   opcode_out = set_opcode_rd_wr (opcode, curr_format);
   opcode_out->op.ins = ins;			/* Keep orig instruc */

   for (i = 0; i < num_operands; i++) {
/* Expand reset_val inline for speed   */
/*    reset_val (curr_format, i); */
      type = type_list[i];
      operand_set[type] = FALSE;
   }

   opcode_out = check_mtctr_mfctr (opcode_out);
   opcode_out = check_fxm_flm     (opcode_out);
   opcode_out->op.orig_addr = curr_ins_addr;
   return opcode_out;
}

/************************************************************************
 *									*
 *				set_opcode_rd_wr			*
 *				----------------			*
 *									*
 ************************************************************************/

OPCODE2 *set_opcode_rd_wr (opcode_in, curr_format)
OPCODE1    *opcode_in;
PPC_OP_FMT *curr_format;
{
   int	    i, j;
   int      num_resrc;
   int      srcdest_com_type;
   int	    num_wr = opcode_in->op.num_wr;
   int	    num_rd = opcode_in->op.num_rd;
   char	    *expl;
   unsigned restype;
   unsigned *resrc;
   OPCODE2  *opcode_out;

   assert (ppc_ops_mem_index < PPC_OPS_MEM_SIZE);
   opcode_out = &ppc_ops_mem[ppc_ops_mem_index++];

   opcode_out->b = &opcode_in->b;
   opcode_out->op.verif = FALSE;

   /* Resources WRITTEN TO / MODIFIED */
   resrc                      = opcode_in->op.wr;
   opcode_out->op.wr	      = ptr_wr;
   opcode_out->op.wr_explicit = expl = wr_expl + (ptr_wr - wr_area);

   for (i = 0; i < num_wr; i++, resrc++, expl++) {
      restype = *resrc;
      if (!(restype & OPERAND_BIT)) {
         *expl           = -1;		/* Indicates not an explicit operand */
         ptr_wr->reg     = restype;
         ptr_wr->latency = opcode_in->op.wr_lat[i];
         ptr_wr++;
      }
      else {
	 num_resrc = set_resrc_usage (ptr_wr, &expl, restype, &opcode_out->op,
				      TRUE, opcode_in->op.wr_lat[i]);
/*
	 if      (num_resrc == 1) expl++;
	 else if (num_resrc  > 1) {
	    for (j = 0; j < num_resrc; j++) *expl++ = -1;
	 }
*/
         ptr_wr += num_resrc;
      }
   }
   opcode_out->op.num_wr = ptr_wr - opcode_out->op.wr;
   assert (ptr_wr < &wr_area[MAX_PAGE_MOD_RESRC]);
   /* If this assertion fails, MAX_PAGE_MOD_RESRC should be increased */

   /* Resources READ FROM / USED */
   resrc      = opcode_in->op.rd;
   opcode_out->op.rd = ptr_rd;
   opcode_out->op.rd_explicit = expl = rd_expl + (ptr_rd - rd_area);

   if (curr_format->srcdest_common < 0) srcdest_com_type = -1;
   else srcdest_com_type = curr_format->list[curr_format->srcdest_common].type;
      
   /* -1 signifies that the instruction does not require any sources
      and destinations use a common register.  (PPC defines only
      4 such instructions:  RLIMI, RLMI, RRIB, and MASKIR.)
   */
   opcode_out->op.srcdest_common = -1;

   for (i = 0; i < num_rd; i++, resrc++, expl++) {
      restype = *resrc;
      if (!((restype) & OPERAND_BIT)) {
         *expl     = -1;		/* Indicates not an explicit operand */
         *ptr_rd++ = restype;
      }
      else {
         num_resrc = set_resrc_usage (ptr_rd, &expl, restype, &opcode_out->op,
				      FALSE, 0);
	 if (restype == srcdest_com_type) {
	    /* We only support one common src/dest register */
	    assert (opcode_out->op.srcdest_common == -1);
	    opcode_out->op.srcdest_common = *ptr_rd;
	 }

	 if (num_resrc  > 1)
	    for (j = 1; j < num_resrc; j++) *(++expl) = -1;

	 ptr_rd += num_resrc;
      }
   }
   opcode_out->op.num_rd = ptr_rd - opcode_out->op.rd;
   assert (ptr_rd < &rd_area[MAX_PAGE_USE_RESRC]);
   /* If this assertion fails, MAX_PAGE_USE_RESRC should be increased */

   return opcode_out;
}

/************************************************************************
 *									*
 *				set_resrc_usage				*
 *				---------------				*
 *									*
 * Indicate in "resrc_list" that "resrc"				*
 * is used by the current operation.  "resrc" may specify (1) an actual	*
 * machine resource (e.g. MEM), (2) an instruction operand type		*
 * (e.g. RT), or (3) an instruction operand type which uses multiple	*
 * resources (e.g. FXM).						*
 *									*
 * RETURNS:  The number of resources used/modified by "resrc".		*
 *									*
 ************************************************************************/

int set_resrc_usage (resrc_list, expl, resrc, operand, is_wlist, latency)
void	       *resrc_list;
char	       **expl;
unsigned       resrc;
OPERAND_OUT    *operand;
int	       is_wlist;
int	       latency;
{
   int	    val;
   unsigned type;
   unsigned mod_resrc;
   unsigned short *resrc_list1 = resrc_list;
   RESULT	  *resrc_list2 = resrc_list;

   /* If the input resource specifies an
      operand type like RT, then put the real resource like R3 in
      the list.  If an operand type like BF specifies multiple real
      resources like CR4-CR7, then place all the real resources
      immediately following the operand type.
   */

   assert (resrc & OPERAND_BIT);

   type = resrc & (~OPERAND_BIT);
   assert (operand_set[type] == TRUE);
   val  = operand_val[type];
   **expl = type;			/* Keep mapping to expl operand type */

   /* Optimize common cases of "get_mod_resrc" by inlining them. */
   switch (resrc) {
      case RA:
      case RB:
      case RS:
      case RT:
	 val += R0;
	 if (!is_wlist) *resrc_list1 = val;
         else {
	    resrc_list2->reg	 = val;
	    resrc_list2->latency = latency;
	 }
	 operand->renameable[type] = val;
	 return 1;
	 break;

      case RZ:     
	 /* Make sure no one gets confused if checking if RZ==0 */
	 operand->renameable[type] = val;
	 if (val == 0) { 
	    (*expl)--;   
	    return 0;
	 }
	 else {
	    val += R0;

	    if (!is_wlist) *resrc_list1 = val;
            else {
	       resrc_list2->reg	 = val;
	       resrc_list2->latency = latency;
	    }

	    return 1;
	 }
	 break;

      default:
	 break;
   }

   mod_resrc = get_mod_resrc (resrc, val);

   if (mod_resrc == MULTIPLE) {
      return set_mod_mult_resrc (resrc_list, resrc, val, operand,
				 is_wlist, latency);
   }
   else if (mod_resrc ==  XER) {
      return set_mod_mult_resrc (resrc_list, XER,   val, operand,
				 is_wlist, latency);
   }
   else if (mod_resrc != NOTHING) {

      if (!is_wlist) *resrc_list1 = mod_resrc;
      else {
	 resrc_list2->reg     = mod_resrc;
	 resrc_list2->latency = latency;
      }

      if (type < NUM_RENAMEABLE_OPERANDS) operand->renameable[type]=mod_resrc;
      return 1;
   }
   else {
      (*expl)--;	/* Keep in sync with all else.  Useful with RZ=0 */
      return 0;
   }
}

/************************************************************************
 *									*
 *				get_mod_resrc				*
 *				-------------				*
 *									*
 * Return the machine resource that is used/modified by the operand.	*
 * For example, "operand" might be RT and "val" might be 5, in which	*
 * case this routine returns R5.					*
 *									*
 ************************************************************************/

int get_mod_resrc (operand, val)
unsigned operand;
int	 val;
{
   switch (operand) {
      case RA:
      case RB:
      case RS:
      case RT:
		   return R0 + val;
	 
      case RZ:     if (val == 0) return NOTHING;
		   else return R0 + val;

      case FRT:
      case FRS:
      case FRA:
      case FRB:
      case FRC:
		   return FP0 + val;

      case SPR_F:  return spr_f (val);
      case SPR_T:  return spr_t (val);
      case SR:	   return SR0 + val;

      case D:
      case SI:
      case UI:
      case SH:
      case NB:
      case MB:
      case ME:
      case TO:
      case BD:
      case LI:
      case I:
      case FL1:
      case FL2:
      case LEV:
      case SV:
		   return NOTHING;

      case BA:	   return    CR0 + val;
      case BB:	   return    CR0 + val;
      case BI:	   return    CR0 + val;
      case BT:	   return    CR0 + val;
      case BTF:    return FPSCR0 + val;
      case BO:	   return bo (val);

      case GCA:	   return CA;
      case GOV:	   return OV;

      case BF:	   return MULTIPLE;
      case BFF:	   return MULTIPLE;
      case BFA:	   return MULTIPLE;
      case BFB:	   return MULTIPLE;

      case FXM:
      case FLM:
		   return MULTIPLE;

      case RTM:
      case RSM:
      case RTS:
      case RSS:
		   return MULTIPLE;

      default:	   fprintf (stderr, "%d  Unknown Operand Type\n", operand);
		   exit (1);
   }
}


/************************************************************************
 *									*
 *				set_mod_mult_resrc			*
 *				------------------			*
 *									*
 * Set the machine resources that are used/modified by the operand.	*
 * RETURNS:  The number of resources used/modified by the operand.	*
 *									*
 ************************************************************************/

int set_mod_mult_resrc (list, type, val, operand, is_wlist, latency)
unsigned short *list;
unsigned       type;
int	       val;
OPERAND_OUT    *operand;
int	       is_wlist;
int	       latency;
{
   switch (type) {
      case BF:
      case BFA:	   return bfc (list, val, operand, type, is_wlist, latency);
      case BFF:
      case BFB:	   return bff (list, val, is_wlist, latency);

      case FXM:	   return fxm (list, val, is_wlist, latency);
      case FLM:	   return flm (list, val, is_wlist, latency);

      case RTM:
      case RSM:    return rtm (list, val, is_wlist, latency);
      case RTS:
      case RSS:    return rts (list, val, is_wlist, latency);

      case XER:    return xer (list,      is_wlist, latency);

      default:	   fprintf (stderr, "%d  Unknown Operand Type\n", type);
		   exit (1);
   }
}

/************************************************************************
 *									*
 *				spr_t					*
 *				-----					*
 *									*
 * Values returned by the SPR field in the mtspr instruction.  See p.66	*
 *									*
 ************************************************************************/

int spr_t (val)
int val;
{
   switch (val) {
      case 0:	return MQ;			/* Only in Power */
      case 1:	return XER;
      case 8:	return LR;
      case 9:	return CTR;
      case 10:	return LR2;
      case 17:	return TID;			/* Only in Power */
      case 18:	return DSISR;
      case 19:	return DAR;
      case 20:	return RTCU;			/* Only in Power */
      case 21:	return RTCL;			/* Only in Power */
      case 22:	return DEC;			/* Only in Power */
      case 24:	return SDR0;			/* Only in Power */
      case 25:	return SDR1;
      case 26:	return SRR0;
      case 27:	return SRR1;

      case 272: return SPRG0;			/* Only in PowerPC */
      case 273: return SPRG1;			/* Only in PowerPC */
      case 274: return SPRG2;			/* Only in PowerPC */
      case 275: return SPRG3;			/* Only in PowerPC */
      case 280: return ASR;			/* Only in PowerPC */
      case 282: return EAR;			/* Only in PowerPC */
      case 284: return TBL;			/* Only in PowerPC */
      case 285: return TBU;			/* Only in PowerPC */
      case 528: return IBAT0U;			/* Only in PowerPC */
      case 529: return IBAT0L;			/* Only in PowerPC */
      case 530: return IBAT1U;			/* Only in PowerPC */
      case 531: return IBAT1L;			/* Only in PowerPC */
      case 532: return IBAT2U;			/* Only in PowerPC */
      case 533: return IBAT2L;			/* Only in PowerPC */
      case 534: return IBAT3U;			/* Only in PowerPC */
      case 535: return IBAT3L;			/* Only in PowerPC */
      case 536: return DBAT0U;			/* Only in PowerPC */
      case 537: return DBAT0L;			/* Only in PowerPC */
      case 538: return DBAT1U;			/* Only in PowerPC */
      case 539: return DBAT1L;			/* Only in PowerPC */
      case 540: return DBAT2U;			/* Only in PowerPC */
      case 541: return DBAT2L;			/* Only in PowerPC */
      case 542: return DBAT3U;			/* Only in PowerPC */
      case 543: return DBAT3L;			/* Only in PowerPC */

      default:  return NOTHING;
   }
}

/************************************************************************
 *									*
 *				spr_f					*
 *				-----					*
 *									*
 * Values returned by the SPR field in the mfspr instruction.  See p.66	*
 *									*
 ************************************************************************/

int spr_f (val)
int val;
{
   switch (val) {
      case 0:	return MQ;			/* Only in Power */
      case 1:	return XER;
      case 4:	return RTCU;			/* Only in Power */
      case 5:	return RTCL;			/* Only in Power */
      case 6:	return DEC;			/* Only in Power */
      case 8:	return LR;
      case 9:	return CTR;
      case 10:	return LR2;
      case 17:	return TID;			/* Only in Power */
      case 18:	return DSISR;
      case 19:	return DAR;
      case 24:	return SDR0;			/* Only in Power */
      case 25:	return SDR1;
      case 26:	return SRR0;
      case 27:	return SRR1;

      case 268: return TBL;			/* Only in PowerPC */
      case 269: return TBU;			/* Only in PowerPC */
      case 272: return SPRG0;			/* Only in PowerPC */
      case 273: return SPRG1;			/* Only in PowerPC */
      case 274: return SPRG2;			/* Only in PowerPC */
      case 275: return SPRG3;			/* Only in PowerPC */
      case 280: return ASR;			/* Only in PowerPC */
      case 282: return EAR;			/* Only in PowerPC */
      case 287: return PVR;			/* Only in PowerPC */
      case 528: return IBAT0U;			/* Only in PowerPC */
      case 529: return IBAT0L;			/* Only in PowerPC */
      case 530: return IBAT1U;			/* Only in PowerPC */
      case 531: return IBAT1L;			/* Only in PowerPC */
      case 532: return IBAT2U;			/* Only in PowerPC */
      case 533: return IBAT2L;			/* Only in PowerPC */
      case 534: return IBAT3U;			/* Only in PowerPC */
      case 535: return IBAT3L;			/* Only in PowerPC */
      case 536: return DBAT0U;			/* Only in PowerPC */
      case 537: return DBAT0L;			/* Only in PowerPC */
      case 538: return DBAT1U;			/* Only in PowerPC */
      case 539: return DBAT1L;			/* Only in PowerPC */
      case 540: return DBAT2U;			/* Only in PowerPC */
      case 541: return DBAT2L;			/* Only in PowerPC */
      case 542: return DBAT3U;			/* Only in PowerPC */
      case 543: return DBAT3L;			/* Only in PowerPC */

      default:  return NOTHING;
   }
}

/************************************************************************
 *									*
 *				bo					*
 *				--					*
 *									*
 * Values returned by the BO fields in the bc, bca, bcl, bcla, bcr,	*
 * bcrl, bcc and bccl instructions.  See p.12 and p.13.			*
 *									*
 ************************************************************************/

int bo (val)
int val;
{
   if ((val & 0x1C) ==  4) return NOTHING;
   if ((val & 0x1C) == 12) return NOTHING;
   if ((val & 0x14) == 20) return NOTHING;
   return CTR;
}

/************************************************************************
 *									*
 *				bfc					*
 *				---					*
 *									*
 * Set a 4-bit field in the condition code register, i.e. CR0, CR1, etc.*
 *									*
 ************************************************************************/

int bfc (list, field, operand, type, is_wlist, latency)
void	       *list;
int	       field;
OPERAND_OUT    *operand;
unsigned       type;
int	       is_wlist;
int	       latency;
{
   int start		 = CR0 + 4 * field;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   operand->renameable[type & (~OPERAND_BIT)] = start;

   if (!is_wlist) {
      list1[0] = start;
      list1[1] = start + 1;
      list1[2] = start + 2;
      list1[3] = start + 3;
   }
   else {
      list2[0].reg     = start;
      list2[1].reg     = start + 1;
      list2[2].reg     = start + 2;
      list2[3].reg     = start + 3;

      list2[0].latency = latency;
      list2[1].latency = latency;
      list2[2].latency = latency;
      list2[3].latency = latency;
   }

   return 4;
}

/************************************************************************
 *									*
 *				bff					*
 *				---					*
 *									*
 * Set a 4-bit field in the floating point status and control register	*
 * (FPSCR): FPSCR0-3, FPSCR4-7, etc.					*
 *									*
 ************************************************************************/

int bff (list, field, is_wlist, latency)
void	       *list;
int	       field;
int	       is_wlist;
int	       latency;
{
   int i;
   int start		 = FPSCR0 + 4 * field;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   if (!is_wlist) {
      list1[0] = start;
      list1[1] = start + 1;
      list1[2] = start + 2;
      list1[3] = start + 3;
   }
   else {
      list2[0].reg     = start;
      list2[1].reg     = start + 1;
      list2[2].reg     = start + 2;
      list2[3].reg     = start + 3;

      list2[0].latency = latency;
      list2[1].latency = latency;
      list2[2].latency = latency;
      list2[3].latency = latency;
   }

   return 4;
}

/************************************************************************
 *									*
 *				fxm					*
 *				---					*
 *									*
 * Set all 4-bit condition register fields specified by "mask".  There	*
 * can be at most 8 (CR0 to CR7).					*
 *									*
 ************************************************************************/

int fxm (list, mask, is_wlist, latency)
void	       *list;
int	       mask;
int	       is_wlist;
int	       latency;
{
   int		  i;
   int		  start;
   int		  cnt = 0;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   for (i = 7; i >= 0; i--, mask >>= 1) {
      if (mask & 0x01) {
	 start = CR0 + 4 * i;

	 if (!is_wlist) {
	    list1[cnt]   = start;
	    list1[cnt+1] = start + 1;
	    list1[cnt+2] = start + 2;
	    list1[cnt+3] = start + 3;
	 }
	 else {
	    list2[cnt].reg   = start;
	    list2[cnt+1].reg = start + 1;
	    list2[cnt+2].reg = start + 2;
	    list2[cnt+3].reg = start + 3;

	    list2[cnt].latency   = latency;
	    list2[cnt+1].latency = latency;
	    list2[cnt+2].latency = latency;
	    list2[cnt+3].latency = latency;
	 }

	 cnt += 4;
      }
   }

   return cnt;
}

/************************************************************************
 *									*
 *				flm					*
 *				---					*
 *									*
 * Set all 4-bit FPSCR fields specified by "mask".  There can be at	*
 * most 8 (FPSCR0-3 to FPSCR28-31).					*
 *									*
 ************************************************************************/

int flm (list, mask, is_wlist, latency)
void	       *list;
int	       mask;
int	       is_wlist;
int	       latency;
{
   int		  i;
   int		  cnt = 0;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   if (!is_wlist) for (i = 7; i >= 0; i--, mask >>= 1) {
      if (mask & 0x01) {
	 bff (&list1[cnt], i, is_wlist, latency);
	 cnt += 4;
      }
   }
   else for (i = 7; i >= 0; i--, mask >>= 1) {
      if (mask & 0x01) {
	 bff (&list2[cnt], i, is_wlist, latency);
	 cnt += 4;
      }
   }


   return cnt;
}

/************************************************************************
 *									*
 *				rtm					*
 *				---					*
 *									*
 * Set all registers from "startreg" to R31 as being used/modified.	*
 * If this range includes RA on an "lm" instruction, RA is not actually	*
 * modified, however we (pessimistically) indicate that it is.		*
 *									*
 ************************************************************************/

int rtm (list, startreg, is_wlist, latency)
void *list;
int  startreg;
int  is_wlist;
int  latency;
{
   int		  reg;
   int		  cnt = 0;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   startreg += R0;

   if (!is_wlist) for (reg = startreg; reg <= R31; reg++)
      list1[cnt++] = reg;
   else for (reg = startreg; reg <= R31; reg++, cnt++) {
      list2[cnt].reg	 = reg;
      list2[cnt].latency = latency;
   }

   return R31 - startreg + 1;
}

/************************************************************************
 *									*
 *				rts					*
 *				---					*
 *									*
 * Set all registers from "startreg" to the register NB number of bytes	*
 * past "startreg" (with wraparound from R31 to R0) as being		*
 * used/modified.  If this range includes RA on an "lsi" instruction,	*
 * RA is not actually modified, however we (pessimistically) indicate	*
 * that it is.								*
 *									*
 * NOTE:  Assumes that R0-R31 are numbered consecutively.		*
 *									*
 ************************************************************************/

int rts (list, startreg, is_wlist, latency)
void *list;
int  startreg;
int  is_wlist;
int  latency;
{
   int		  reg;
   int		  lastreg;
   int		  cnt = 0;
   int		  num_bytes = operand_val[NB & (~OPERAND_BIT)];
   int		  num_regs;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   if (num_bytes == 0) num_regs = 32;
   else num_regs  = (num_bytes + 3) >> 2;

   startreg += R0;
   if (startreg + num_regs - 1 > R31) {
      lastreg = startreg + num_regs - 2 - R31;

      if (!is_wlist) {
	 for (reg = startreg; reg <= R31; reg++)
            list1[cnt++] = reg;

	 for (reg = 0; reg <= lastreg; reg++)
            list1[cnt++] = reg;
      }
      else {
	 for (reg = startreg; reg <= R31; reg++, cnt++) {
            list2[cnt].reg     = reg;
            list2[cnt].latency = latency;
	 }

	 for (reg = 0; reg <= lastreg; reg++, cnt++) {
            list2[cnt].reg     = reg;
            list2[cnt].latency = latency;
	 }
      }
   }
   else {
      lastreg = startreg + num_regs - 1;

      if (!is_wlist) {
	 for (reg = startreg; reg <= lastreg; reg++)
            list1[cnt++] = reg;
      }
      else {
	 for (reg = startreg; reg <= lastreg; reg++, cnt++) {
            list2[cnt].reg     = reg;
            list2[cnt].latency = latency;
	 }
      }
   }

   return num_regs;
}

/************************************************************************
 *									*
 *				xer					*
 *				---					*
 *									*
 * Set all bits of the XER register as used/modified.			*
 *									*
 ************************************************************************/

int xer (list, is_wlist, latency)
void *list;
int  is_wlist;
int  latency;
{
   int		  reg;
   int		  cnt = 0;
   unsigned short *list1 = list;
   RESULT	  *list2 = list;

   if (!is_wlist) for (reg = XER0; reg <= XER31; reg++)
      list1[cnt++] = reg;
   else for (reg = XER0; reg <= XER31; reg++, cnt++) {
      list2[cnt].reg	= reg;
      list2[cnt].latency = latency;
   }

   return 32;
}

/************************************************************************
 *									*
 *				check_fxm_flm				*
 *				-------------				*
 *									*
 * If FXM or FLM specify only one 4-bit field, then map them to a new	*
 * instruction, which specifies only one field.  This helps with	*
 * renaming.  See also the comments below in "fxm_to_bf2."  Example:	*
 *									*
 *	MTCRF  0x10,r4   ==>  MTCRF2  3,3,r4				*
 *									*
 * It may be possible to move the MTCRF2 up and rename its output	*
 * field.  Perhaps CR3 is renamed CR14:					*
 *									*
 *	MTCRF2  14,3,r4							*
 *									*
 * The second "3" is the BFEX field and indicates that we want bits	*
 * 12-15 of r4 to go to CR14.						*
 *									*
 ************************************************************************/

OPCODE2 *check_fxm_flm (op)
OPCODE2 *op;
{
   int	    bf2;
   int	    op_num2;
   int	    op_num   = op->b->op_num;
   unsigned orig_ins = op->op.ins;
   unsigned rs;

   /* Can only change to renameable form if dest one CR field (4 bits) */
   if (op->op.num_wr != 4) return op;

   switch (op_num) {
      case OP_MTCRF:
	 op_num2 = OP_MTCRF2;
	 bf2 = fxm_to_bf2 ((orig_ins >> 12) & 0xFF);
	 rs  = (orig_ins >> 21) & 0x1F;
	 op->op.renameable      [BF2 & (~OPERAND_BIT)] = CR0 + 4 * bf2;
	 op->op.renameable      [RB  & (~OPERAND_BIT)] = rs;
	 op->op.wr_explicit[0] = BF2 & (~OPERAND_BIT);
	 op->op.rd_explicit[0] = RB  & (~OPERAND_BIT);
	 op->op.ins  = 0x7C000122;
         op->op.ins |= rs  << 11;			/* Move RS to RB */
         op->op.ins |= bf2 << 23;			/* Set BF2  field */
         op->op.ins |= bf2 << 18;			/* Set BFEX field */
	 break;

#ifdef FPSCR_IS_RENAMEABLE
      case OP_MTFSF:
	 op_num2 = OP_MTFSF2;
	 bf2 = fxm_to_bf2 ((orig_ins >> 17) & 0xFF);
	 op->op.renameable      [BFF2 & (~OPERAND_BIT)] = FPSCR0 + 4 * bf2;
	 op->op.wr_explicit[0] = BFF2 & (~OPERAND_BIT);
	 op->op.ins  = 0xFC000590;
         op->op.ins |= (orig_ins & 0xF800);		/* Move FRB to FRB */
         op->op.ins |= bf2 << 23;			/* Set BFF2 field */
         op->op.ins |= bf2 << 18;			/* Set BFEX field */
	 break;

      case OP_MTFSF_:
	 op_num2 = OP_MTFSF2_;
	 bf2 = fxm_to_bf2 ((orig_ins >> 17) & 0xFF);
	 op->op.renameable      [BFF2 & (~OPERAND_BIT)] = FPSCR0 + 4 * bf2;
	 op->op.wr_explicit[0] = BFF2 & (~OPERAND_BIT);
	 op->op.ins  = 0xFC000591;
         op->op.ins |= (orig_ins & 0xF800);		/* Move FRB to FRB */
         op->op.ins |= bf2 << 23;			/* Set BFF2 field */
         op->op.ins |= bf2 << 18;			/* Set BFEX field */
	 break;
#endif

      default:
	 return op;
   }

   op->b         = &ppc_opcode[op_num2]->b;

   return op;
}

/************************************************************************
 *									*
 *				fxm_to_bf2				*
 *				----------				*
 *									*
 * Map the values for FXM or FLM to the new (VLIW) BF2 and BFF2		*
 * operand forms.  Unlike FXM and FLM, BF2 and BFF2 can only specify	*
 * one 4-bit field.  This field is filled from the corresponding bits	*
 * in a GPR or FPR.  For example, if FXM were 0x10, then we want CR3	*
 * and bits 12-15 of the GPR would be put in CR3.  Use of the BF2 and	*
 * BFF2 fields allows us to also specify non-architected, VLIW only	*
 * condition register fields.						*
 *									*
 ************************************************************************/

int fxm_to_bf2 (fxm)
int fxm;
{
   switch (fxm) {
      case 0x01:  return 7;
      case 0x02:  return 6;
      case 0x04:  return 5;
      case 0x08:  return 4;
      case 0x10:  return 3;
      case 0x20:  return 2;
      case 0x40:  return 1;
      case 0x80:  return 0;

      default:
	 fprintf (stderr, "Unexpected value (0x%2x) for FXM/FLM field\n", fxm);
	 exit (1);
   }
}

/************************************************************************
 *									*
 *				check_mtctr_mfctr			*
 *				-----------------			*
 *									*
 * Since CTR==R32,							*
 *   Convert      mtspr CTR,rx   ==>   oril CTR,rx,0			*
 *   Convert      mfspr rx,CTR   ==>   oril rx,CTR,0			*
 *									*
 * NOTE:  op->op.rd and op->op.wr should not need changes.		*
 *									*
 ************************************************************************/

OPCODE2 *check_mtctr_mfctr (op)
OPCODE2 *op;
{
   int	    op_num   = op->b->op_num;
   unsigned orig_ins = op->op.ins;
   unsigned gpr      = (orig_ins >> 21) & 0x1F;

   switch (op_num) {
      case OP_MTSPR:
         if (spr_t ((orig_ins >> 16) & 0x1F) != CTR) return op;
	 op->b         = &ppc_opcode[OP_ORIL]->b;
	 op->op.renameable      [RA & (~OPERAND_BIT)] = CTR;
	 op->op.renameable      [RS & (~OPERAND_BIT)] = gpr;

	 /* Unlikely that RC bit is set. */
         assert (op->op.num_wr == 1);
	 op->op.wr_explicit[0] = RA & (~OPERAND_BIT);
	 op->op.rd_explicit[0] = RS & (~OPERAND_BIT);

	 op->op.ins = 0x60000000 | (gpr << 21);
	 return op;

      case OP_MFSPR:
         if (spr_f ((orig_ins >> 16) & 0x1F) != CTR) return op;
	 op->b         = &ppc_opcode[OP_ORIL]->b;
	 op->op.renameable      [RA & (~OPERAND_BIT)] = gpr;
	 op->op.renameable      [RS & (~OPERAND_BIT)] = CTR;

	 /* Unlikely that RC bit is set. */
         assert (op->op.num_wr == 1);
	 op->op.wr_explicit[0] = RA & (~OPERAND_BIT);
	 op->op.rd_explicit[0] = RS & (~OPERAND_BIT);

	 op->op.ins = 0x60000000 | (gpr << 16);
	 return op;

      default:
	 return op;
   }
}

/************************************************************************
 *									*
 *				set_val					*
 *				-------					*
 *									*
 ************************************************************************/

set_val (ins, curr_format, num)
unsigned   ins;
PPC_OP_FMT *curr_format;
int	   num;
{
   int		op_num;
   int		type;
   unsigned	val;
   OPERAND	*operand;

   op_num  = curr_format->ppc_asm_map[num];
   operand = &curr_format->list[op_num];
   val	   = extract_operand_val (ins, operand);
   type    = operand->type;
   val     = chk_immed_val (val, type);

   type		    &= ~OPERAND_BIT;
   operand_val[type] = val;
   operand_set[type] = TRUE;
}

/************************************************************************
 *									*
 *				reset_val				*
 *				---------				*
 *									*
 ************************************************************************/

reset_val (curr_format, num)
PPC_OP_FMT *curr_format;
int	   num;
{
   int     op_num    = curr_format->ppc_asm_map[num];
   OPERAND *operand  = &curr_format->list[op_num];
   int	   type      = operand->type;

   type		    &= ~OPERAND_BIT;
   operand_set[type] = FALSE;
}

/************************************************************************
 *									*
 *				get_result_latency			*
 *				------------------			*
 *									*
 * Return the maximum latency of all the results of operation "op".	*
 *									*
 * NOTE 1:  This expects an operand of type OPCODE2.			*
 * NOTE 2:  Routine assert fails if "result" is not among results	*
 *	    produced by op.						*
 *									*
 ************************************************************************/

unsigned get_result_latency (op, result)
OPCODE2 *op;
int	result;
{
   unsigned i;
   RESULT   *wr_list;
   unsigned list_size;

   wr_list   = op->op.wr;
   list_size = op->op.num_wr;

   for (i = 0; i < list_size; i++)
      if (wr_list[i].reg == result) return wr_list[i].latency;

   assert (1 == 0);
}

/************************************************************************
 *									*
 *				get_max_latency1			*
 *				----------------			*
 *									*
 * Return the maximum latency of all the results of operation "op".	*
 * NOTE:  This expects an operand of type OPCODE1.			*
 *									*
 ************************************************************************/

unsigned get_max_latency1 (op)
OPCODE1 *op;
{
   unsigned i;
   unsigned max_latency = 0;
   unsigned *latency_list;
   unsigned list_size;

   latency_list = op->op.wr_lat;
   list_size    = op->op.num_wr;

   for (i = 0; i < list_size; i++)
      if (latency_list[i] > max_latency) max_latency = latency_list[i];

   return max_latency;
}

#ifdef DEBUGGING1
/************************************************************************
 *									*
 *				dump_operands				*
 *				-------------				*
 *									*
 ************************************************************************/

dump_operands (fp, ins, format)
FILE	      *fp;
unsigned      ins;
unsigned      format;
{
   int		i;
   int		num_operands;
   PPC_OP_FMT	*curr_format;

   assert (format < MAX_PPC_FORMATS);

   curr_format = ppc_op_fmt[format];
   num_operands = curr_format->num_operands;

   dump_val (fp, ins, curr_format, 0, "  ");

   for (i = 1; i < num_operands; i++) {
      dump_val (fp, ins, curr_format, i, ",");
   }
}

/************************************************************************
 *									*
 *				dump_val				*
 *				--------				*
 *									*
 ************************************************************************/

dump_val (fp, ins, curr_format, num, prefix)
FILE	   *fp;
unsigned   ins;
PPC_OP_FMT *curr_format;
int	   num;
char	   *prefix;
{
   int		op_num;
   int		type;
   static int	prev_type = SI;		/* Anything but D */
   unsigned	val;
   OPERAND	*operand;

   op_num  = curr_format->ppc_asm_map[num];
   operand = &curr_format->list[op_num];
   val	   = extract_operand_val (ins, operand);
   type    = operand->type;

   if      (dump_is_int_reg_zero (type, val))
      if (prev_type == D)
           fprintf (fp, "(r%d)",	    val);
      else fprintf (fp, "%sr%d",    prefix, val);
   else if (dump_is_flt_reg (type))
           fprintf (fp, "%sfp%d",   prefix, val);
   else {
	   val = chk_immed_val (val, type);
	   fprintf (fp, "%s%d",     prefix, val);
   }

   if (type == D) prev_type = D;
   else prev_type = SI;			/* Anything but D */
}

/************************************************************************
 *									*
 *				dump_list_resrc_usage			*
 *				---------------------			*
 *									*
 ************************************************************************/

dump_list_resrc_usage (fp, index)
FILE	    *fp;
unsigned    index;
{
   int		  i;
   int		  num_resrc;
   RESULT	  *wr_resrc;
   unsigned short *rd_resrc;
   OPCODE2	  *opcode_out = &ppc_ops_mem[index];

   /* Resources WRITTEN TO / MODIFIED */
   wr_resrc  = opcode_out->op.wr;
   num_resrc = opcode_out->op.num_wr;

   fprintf (fp, "\n\nWRITTEN (LIST):    ");
   for (i = 0; i < num_resrc; i++) {
      fprintf (fp, "%d ", wr_resrc[i].reg);
   }

   /* Resources READ FROM / USED */
   rd_resrc  = opcode_out->op.rd;
   num_resrc = opcode_out->op.num_rd;

   fprintf (fp, "\nREAD    (LIST):    ");
   for (i = 0; i < num_resrc; i++) {
      fprintf (fp, "%d ", rd_resrc[i]);
   }

   fprintf (fp, "\n\n");
}

#endif

/************************************************************************
 *									*
 *				extract_spr_val				*
 *				---------------				*
 *									*
 * Return the value of the SPR register for an "mtspr" or "mfspr"	*
 * based on whether we are running for a Power or PowerPC.		*
 *									*
 ************************************************************************/

unsigned extract_operand_val (ins, operand)
unsigned ins;
OPERAND  *operand;
{
   int	    type;
   unsigned val;
   unsigned low;
   unsigned high;

   if (!use_powerpc)
      return (unsigned) ((ins >> operand->shift)       & operand->mask);
   else if (type != SPR_F  &&  type != SPR_T)
      return (unsigned) ((ins >> operand->shift)       & operand->mask);
   else {
      low  = (unsigned) ((ins >>  operand->shift)      & operand->mask);
      high = (unsigned) ((ins >> (operand->shift - 5)) & operand->mask);
      return (high << 5) + low;
   }
}

/************************************************************************
 *									*
 *				chk_immed_val				*
 *				-------------				*
 *									*
 ************************************************************************/

int chk_immed_val (val, type)
int val;
int type;
{
   switch (type) {
      case SI:
      case D:
		return (int) ((short) val);
      case BD:
		val <<= 2;
		return (int) ((short) val);
      case LI:
		if (val & 0x800000) val |= 0xFF000000;
		return val << 2;

      case LEV:
		return (val << 5) + 0x1000;

      default:  return val;
   }
}

/************************************************************************
 *									*
 *				is_int_reg				*
 *				----------				*
 *									*
 ************************************************************************/

static int is_int_reg (type, val)
int type;
int val;
{
   switch (type) {
      case RT:
      case RTM:
      case RTS:
      case RS:
      case RSM:
      case RSS:
      case RA:
      case RB:
	        return TRUE;
      case RZ:
  if (val != 0) return TRUE;
  else		return FALSE;
	     
      default:  return FALSE;
   }
}

/************************************************************************
 *									*
 *				dump_is_int_reg_zero			*
 *				--------------------			*
 *									*
 * For some addressing modes, if RA (or RZ in our terminology) is zero	*
 * then it refers to immediate_0, not register_0 (r0).  This function	*
 * returns TRUE regardless, while "is_int_reg" returns TRUE for the RZ	*
 * type only if the RZ field non-zero.					*
 *									*
 ************************************************************************/

int dump_is_int_reg_zero (type, val)
int type;
int val;
{
   switch (type) {
      case RT:
      case RTM:
      case RTS:
      case RS:
      case RSM:
      case RSS:
      case RA:
      case RB:
      case RZ:
	        return TRUE;
	     
      default:  return FALSE;
   }
}

/************************************************************************
 *									*
 *				dump_is_flt_reg				*
 *				---------------				*
 *									*
 ************************************************************************/

int dump_is_flt_reg (type)
int type;
{
   switch (type) {
      case FRT:
      case FRS:
      case FRA:
      case FRB:
      case FRC:
	        return TRUE;
      default:  return FALSE;
   }
}

/************************************************************************
 *									*
 *				dump_is_cc_bit				*
 *				--------------				*
 *									*
 ************************************************************************/

int dump_is_cc_bit (type)
int type;
{
   switch (type) {
      case BT:
      case BA:
      case BB:
      case BI:  return TRUE;
      default:  return FALSE;
   }
}

/************************************************************************
 *									*
 *				dump_is_cc_field			*
 *				----------------			*
 *									*
 ************************************************************************/

int dump_is_cc_field (type)
int type;
{
   switch (type) {
      case BF:
      case BF2:
      case BFA: return TRUE;
      default:  return FALSE;
   }
}
