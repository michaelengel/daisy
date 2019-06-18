/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "resrc_list.h"
#include "dis.h"
#include "dis_tbl.h"
#include "vliw.h"

extern int num_gprs;
extern int num_ccbits;

/*#######################################################################
  # These routines should probably be made macros for speed.		#
  #######################################################################*/

/************************************************************************
 *									*
 *				is_branch				*
 *				---------				*
 *									*
 ************************************************************************/

int is_branch (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_B:
      case OP_BA:
      case OP_BL:
      case OP_BLA:
      case OP_BC:
      case OP_BCA:
      case OP_BCL:
      case OP_BCLA:
      case OP_BCR:
      case OP_BCRL:
      case OP_BCR2:
      case OP_BCR2L:
      case OP_BCC:
      case OP_BCCL:

      case OP_SVC:
      case OP_SVCA:
      case OP_SVCL:
      case OP_SVCLA:
      case OP_RFSVC:
      case OP_RFI:

      case OP_TI:
      case OP_T:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_nontrap_branch			*
 *				-----------------			*
 *									*
 ************************************************************************/

int is_nontrap_branch (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_B:
      case OP_BA:
      case OP_BL:
      case OP_BLA:
      case OP_BC:
      case OP_BCA:
      case OP_BCL:
      case OP_BCLA:
      case OP_BCR:
      case OP_BCRL:
      case OP_BCR2:
      case OP_BCR2L:
      case OP_BCC:
      case OP_BCCL:

      case OP_SVC:
      case OP_SVCA:
      case OP_SVCL:
      case OP_SVCLA:
      case OP_RFSVC:
      case OP_RFI:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_rel_branch				*
 *				-------------				*
 *									*
 ************************************************************************/

int is_rel_branch (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_B:
      case OP_BL:
      case OP_BC:
      case OP_BCL:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_cr_bit_op				*
 *				------------				*
 *									*
 ************************************************************************/

int is_cr_bit_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_CREQV:
      case OP_CRAND:
      case OP_CRXOR:
      case OP_CROR:
      case OP_CRANDC:
      case OP_CRNAND:
      case OP_CRORC:
      case OP_CRNOR:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_record_op				*
 *				------------				*
 *									*
 ************************************************************************/

is_record_op (opcode)
OPCODE2 *opcode;
{
   if (!(opcode->op.ins & 1)) return FALSE;
   else switch (opcode->b->primary) {
      case 13:		/* ai.		     */
      case 20:		/* rlimi.	     */
      case 21:		/* rlinm.	     */
      case 22:		/* rlmi.	     */
      case 23:		/* rlnm.	     */
      case 31:		/* GPR Non-immediate */
      case 63:		/* FPR Op	     */
         return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_simple_record_op			*
 *				-------------------			*
 *									*
 * Only basic add and subtract operations are "simple".			*
 *									*
 ************************************************************************/

is_simple_record_op (opcode)
OPCODE2 *opcode;
{
   if (!is_record_op (opcode))		return FALSE;
   else if (opcode->b->primary == 13)	return TRUE;		/* ai.  */
   else switch (opcode->b->ext) {
      case   17:						/* sf.   */
      case   21:						/* a.    */
      case  533:						/* cax.  */
      case 1041:						/* sfo.  */
      case 1045:						/* ao.   */
      case 1557:						/* caxo. */
					return TRUE;
      default:				return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_complex_record_op			*
 *				--------------------			*
 *									*
 * Only basic add and subtract operations are "simple".			*
 *									*
 ************************************************************************/

is_complex_record_op (opcode)
OPCODE2 *opcode;
{
   if (!is_record_op (opcode))		return FALSE;
   else if (opcode->b->primary == 13)	return FALSE;		/* ai.  */
   else switch (opcode->b->ext) {
      case   17:						/* sf.   */
      case   21:						/* a.    */
      case  533:						/* cax.  */
      case 1041:						/* sfo.  */
      case 1045:						/* ao.   */
      case 1557:						/* caxo. */
					return FALSE;
      default:				return TRUE;
   }
}

/************************************************************************
 *									*
 *				is_condreg_field			*
 *				----------------			*
 *									*
 * Return TRUE  if vector of write operations is a bit field		*
 *	  FALSE otherwise.						*
 *									*
 ************************************************************************/

int is_condreg_field (vec, vec_size)
RESULT *vec;
int    vec_size;
{
   int reg0;
   int reg1;
   int reg2;
   int reg3;

   if (vec_size != 4) return FALSE;

   reg0 = vec[0].reg;
   reg1 = vec[1].reg;
   reg2 = vec[2].reg;
   reg3 = vec[3].reg;

   if (reg0  < CR0  ||  reg0 >= CR0 + num_ccbits) return FALSE;
   if (reg3  < CR0  ||  reg3 >= CR0 + num_ccbits) return FALSE;
   if ((reg0 & 3) != 0)   return FALSE;
   if (reg1  != reg0 + 1) return FALSE;
   if (reg2  != reg0 + 2) return FALSE;
   if (reg3  != reg0 + 3) return FALSE;

   return TRUE;
}

/************************************************************************
 *									*
 *				mult_cr_bits				*
 *				------------				*
 *									*
 * Return: TRUE if multiple crbits are referenced in the wr vector.	*
 *	   FALSE o.w.							*
 *									*
 ************************************************************************/

int mult_cr_bits (vec, size)
int    size;
RESULT *vec;
{
   int i;
   int cnt = 0;

   for (i = 0; i < size; i++)
      if (vec[i].reg >= CR0  &&  vec[i].reg < CR0 + num_ccbits) cnt++;

   if (cnt > 1) return TRUE;
   else		return FALSE;
}

/************************************************************************
 *									*
 *				is_simple_load_op			*
 *				-----------------			*
 *									*
 * Return TRUE if is "load" and the result goes to only one register.	*
 *									*
 ************************************************************************/

int is_simple_load_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LBZ:
      case OP_LBZX:
      case OP_LHZ:
      case OP_LHZX:
      case OP_LHA:
      case OP_LHAX:
      case OP_L:
      case OP_LX:
      case OP_LHBRX:
      case OP_LBRX:
      case OP_LM:

      case OP_LFS:
      case OP_LFSX:
      case OP_LFD:
      case OP_LFDX:
 	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_load_op				*
 *				----------				*
 *									*
 ************************************************************************/

int is_load_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LBZ:
      case OP_LBZX:
      case OP_LHZ:
      case OP_LHZX:
      case OP_LHA:
      case OP_LHAX:
      case OP_L:
      case OP_LX:
      case OP_LHBRX:
      case OP_LBRX:
      case OP_LM:

      case OP_LBZU:
      case OP_LBZUX:
      case OP_LHZU:
      case OP_LHZUX:
      case OP_LHAU:
      case OP_LHAUX:
      case OP_LU:
      case OP_LUX:
 
      case OP_LSX:
      case OP_LSI:
      case OP_LSCBX:
      case OP_LSCBX_:

      case OP_LFS:
      case OP_LFSX:
      case OP_LFD:
      case OP_LFDX:
      case OP_LFSU:
      case OP_LFSUX:
      case OP_LFDU:
      case OP_LFDUX:
 	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_store_op				*
 *				-----------				*
 *									*
 ************************************************************************/

int is_store_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_STB:
      case OP_STBX:
      case OP_STH:
      case OP_STHX:
      case OP_ST:
      case OP_STX:
      case OP_STHBRX:
      case OP_STBRX:
      case OP_STM:

      case OP_STBU:
      case OP_STBUX:
      case OP_STHU:
      case OP_STHUX:
      case OP_STU:
      case OP_STUX:

      case OP_STSX:
      case OP_STSI:

      case OP_STFS:
      case OP_STFSX:
      case OP_STFD:
      case OP_STFDX:
      case OP_STFSU:
      case OP_STFSUX:
      case OP_STFDU:
      case OP_STFDUX:

      /* Although not technically STORES, these modify memory */
      case OP_DCLZ:
      case OP_CLF:
      case OP_CLI:
      case OP_DCLST:
      case OP_DCS:
      case OP_ICS:
      case OP_TLBI:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_load_flt				*
 *				-----------				*
 *									*
 ************************************************************************/

int is_load_flt (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LFS:
      case OP_LFSX:
      case OP_LFD:
      case OP_LFDX:
      case OP_LFSU:
      case OP_LFSUX:
      case OP_LFDU:
      case OP_LFDUX:
 	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_store_flt				*
 *				------------				*
 *									*
 ************************************************************************/

int is_store_flt (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_STFS:
      case OP_STFSX:
      case OP_STFD:
      case OP_STFDX:
      case OP_STFSU:
      case OP_STFSUX:
      case OP_STFDU:
      case OP_STFDUX:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_memop				*
 *				--------				*
 *									*
 ************************************************************************/

int is_memop (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LBZ:
      case OP_LBZX:
      case OP_LHZ:
      case OP_LHZX:
      case OP_LHA:
      case OP_LHAX:
      case OP_L:
      case OP_LX:
      case OP_LHBRX:
      case OP_LBRX:
      case OP_LM:

      case OP_STB:
      case OP_STBX:
      case OP_STH:
      case OP_STHX:
      case OP_ST:
      case OP_STX:
      case OP_STHBRX:
      case OP_STBRX:
      case OP_STM:

      case OP_LBZU:
      case OP_LBZUX:
      case OP_LHZU:
      case OP_LHZUX:
      case OP_LHAU:
      case OP_LHAUX:
      case OP_LU:
      case OP_LUX:

      case OP_STBU:
      case OP_STBUX:
      case OP_STHU:
      case OP_STHUX:
      case OP_STU:
      case OP_STUX:

      case OP_LSX:
      case OP_LSI:
      case OP_LSCBX:
      case OP_LSCBX_:
      case OP_STSX:
      case OP_STSI:

      case OP_LFS:
      case OP_LFSX:
      case OP_LFD:
      case OP_LFDX:
      case OP_LFSU:
      case OP_LFSUX:
      case OP_LFDU:
      case OP_LFDUX:

      case OP_STFS:
      case OP_STFSX:
      case OP_STFD:
      case OP_STFDX:
      case OP_STFSU:
      case OP_STFSUX:
      case OP_STFDU:
      case OP_STFDUX:

      case OP_DCLZ:
      case OP_CLF:
      case OP_CLI:
      case OP_DCLST:
      case OP_DCS:
      case OP_ICS:
      case OP_TLBI:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_trap					*
 *				-------					*
 *									*
 ************************************************************************/

int is_trap (opcode)
OPCODE2 *opcode;
{
   if (opcode->b->op_num == OP_T || opcode->b->op_num == OP_TI) return TRUE;
   else								return FALSE;
}

/************************************************************************
 *									*
 *				is_idiv_op				*
 *				----------				*
 *									*
 * Return TRUE if is integer divide op.					*
 *									*
 ************************************************************************/

int is_idiv_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_DIV:
      case OP_DIV_:
      case OP_DIVS:
      case OP_DIVS_:

      case OP_DIVO:
      case OP_DIVO_:
      case OP_DIVSO:
      case OP_DIVSO_:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_mftb_op				*
 *				----------				*
 *									*
 * Return TRUE if is "mftb" or "mftbu".  These are PowerPC only.	*
 *									*
 ************************************************************************/

int is_mftb_op (opcode)
OPCODE2 *opcode;
{
   if (opcode->b->op_num == OP_MFTB) return TRUE;
   else				     return FALSE;
}

/************************************************************************
 *									*
 *				sets_ppc_ca_bit				*
 *				---------------				*
 *									*
 * Return TRUE if PPC version of "op" sets CA bit in XER register.	*
 *									*
 ************************************************************************/

int sets_ppc_ca_bit (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_AI:
      case OP_AI_:
      case OP_SFI:
      case OP_A:
      case OP_A_:
      case OP_AO:
      case OP_AO_:
      case OP_AE:
      case OP_AE_:
      case OP_AEO:
      case OP_AEO_:
      case OP_SF:
      case OP_SF_:
      case OP_SFO:
      case OP_SFO_:
      case OP_SFE:
      case OP_SFE_:
      case OP_SFEO:
      case OP_SFEO_:
      case OP_AME:
      case OP_AME_:
      case OP_AMEO:
      case OP_AMEO_:
      case OP_AZE:
      case OP_AZE_:
      case OP_AZEO:
      case OP_AZEO_:
      case OP_SFME:
      case OP_SFMEO:
      case OP_SFMEO_:
      case OP_SFZE:
      case OP_SFZE_:
      case OP_SFZEO:
      case OP_SFZEO_:
      case OP_SRAI:
      case OP_SRAI_:
      case OP_SRA:
      case OP_SRA_:
      case OP_SRAIQ:
      case OP_SRAIQ_:
      case OP_SRAQ:
      case OP_SRAQ_:
      case OP_SREA:
      case OP_SREA_:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				sets_ppc_ov_bit				*
 *				---------------				*
 *									*
 * Return TRUE if PPC version of "op" sets OV, SO in XER register.	*
 *									*
 ************************************************************************/

int sets_ppc_ov_bit (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_CAXO:
      case OP_CAXO_:
      case OP_AO:
      case OP_AO_:
      case OP_AEO:
      case OP_AEO_:
      case OP_SFO:
      case OP_SFO_:
      case OP_SFEO:
      case OP_SFEO_:
      case OP_AMEO:
      case OP_AMEO_:
      case OP_AZEO:
      case OP_AZEO_:
      case OP_SFMEO:
      case OP_SFMEO_:
      case OP_SFZEO:
      case OP_SFZEO_:
      case OP_DOZO:
      case OP_DOZO_:
      case OP_ABSO:
      case OP_ABSO_:
      case OP_NEGO:
      case OP_NEGO_:
      case OP_NABSO:
      case OP_NABSO_:
      case OP_MULO:
      case OP_MULO_:
      case OP_MULSO:
      case OP_MULSO_:
      case OP_DIVO:
      case OP_DIVO_:
      case OP_DIVSO:
      case OP_DIVSO_:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				sets_gpr_ca_bit				*
 *				---------------				*
 *									*
 * Return DESTGPR if VLIW version of "op" sets CA bit in its dest GPR	*
 *	          (as opposed to directly in the XER).  An op sets CA	*
 *	          in the dest GPR, if the dest GPR is not a PPC		*
 *	          architected register, i.e. if it is greater than r31.	*
 *									*
 *	   -1      Otherwise						*
 *									*
 ************************************************************************/

int sets_gpr_ca_bit (opcode)
OPCODE2 *opcode;
{
   int	  i;
   int	  num_wr;
   RESULT *wr;

   if (! sets_ppc_ca_bit (opcode)) return -1;

   wr	  = opcode->op.wr;
   num_wr = opcode->op.num_wr;
   for (i = 0; i < num_wr; i++) {
      if (wr[i].reg >= R0+NUM_PPC_GPRS  &&  wr[i].reg < R0+num_gprs) 
	 return wr[i].reg;
   }

   return -1;
}

/************************************************************************
 *									*
 *				sets_gpr_ov_bit				*
 *				---------------				*
 *									*
 * Return: DESTGPR if VLIW version of "op" sets OV bit in its dest GPR	*
 *	           (as opposed to directly in the XER).  An op sets OV	*
 *	           in the dest GPR, if the dest GPR is not a PPC	*
 *	           architected register, i.e. if it is greater than r31.*
 *									*
 *	   -1      Otherwise						*
 *									*
 ************************************************************************/

int sets_gpr_ov_bit (opcode)
OPCODE2 *opcode;
{
   int    i;
   int    num_wr;
   RESULT *wr;

   if (! sets_ppc_ov_bit (opcode)) return -1;

   wr	  = opcode->op.wr;
   num_wr = opcode->op.num_wr;
   for (i = 0; i < num_wr; i++) {
      if (wr[i].reg >= R0+NUM_PPC_GPRS  &&  wr[i].reg < R0+num_gprs)
	 return wr[i].reg;
   }

   return -1;
}

/************************************************************************
 *									*
 *				is_multiple				*
 *				-----------				*
 *									*
 ************************************************************************/

int is_multiple (opcode)
OPCODE1 *opcode;
{
   return opcode->b.mult;
}

/************************************************************************
 *									*
 *				is_ld_st_update				*
 *				---------------				*
 *									*
 ************************************************************************/

int is_ld_st_update (opcode)
OPCODE1 *opcode;
{
   switch (opcode->b.op_num) {
      case OP_LBZU:
      case OP_LBZUX:
      case OP_LHZU:
      case OP_LHZUX:
      case OP_LHAU:
      case OP_LHAUX:
      case OP_LU:
      case OP_LUX:

      case OP_STBU:
      case OP_STBUX:
      case OP_STHU:
      case OP_STHUX:
      case OP_STU:
      case OP_STUX:

      case OP_LFSU:
      case OP_LFSUX:
      case OP_LFDU:
      case OP_LFDUX:

      case OP_STFSU:
      case OP_STFSUX:
      case OP_STFDU:
      case OP_STFDUX:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_ld_update				*
 *				------------				*
 *									*
 ************************************************************************/

int is_ld_update (opcode)
OPCODE1 *opcode;
{
   switch (opcode->b.op_num) {
      case OP_LBZU:
      case OP_LBZUX:
      case OP_LHZU:
      case OP_LHZUX:
      case OP_LHAU:
      case OP_LHAUX:
      case OP_LU:
      case OP_LUX:

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_st_update				*
 *				------------				*
 *									*
 ************************************************************************/

int is_st_update (opcode)
OPCODE1 *opcode;
{
   switch (opcode->b.op_num) {
      case OP_STBU:
      case OP_STBUX:
      case OP_STHU:
      case OP_STHUX:
      case OP_STU:
      case OP_STUX:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_string_indir				*
 *				---------------				*
 *									*
 ************************************************************************/

int is_string_indir (opcode)
OPCODE2 *opcode;
{
   if      (opcode->b->op_num == OP_LSCBX)  return TRUE;
   if      (opcode->b->op_num == OP_LSCBX_) return TRUE;
   else if (opcode->b->op_num == OP_LSX)    return TRUE;
   else if (opcode->b->op_num == OP_STSX)   return TRUE;
   else					    return FALSE;
}

/************************************************************************
 *									*
 *				is_bcond				*
 *				--------				*
 *									*
 * Is instruction a conditional branch.					*
 * WARNING:  This routine is very PPC specific.				*
 *									*
 ************************************************************************/

is_bcond (ins)
unsigned ins;
{
   if ((ins >> 26) == 16) return TRUE;
   else return FALSE;
}

/************************************************************************
 *									*
 *				is_ppc_rz_op				*
 *				------------				*
 *									*
 * Operations in which the RA field is immediate 0 if 0, instead of r0.	*
 * We call such fields RZ not RA.)					*
 *									*
 ************************************************************************/

is_ppc_rz_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LBZ:
      case OP_LBZX:
      case OP_LHZ:
      case OP_LHZX:
      case OP_LHA:
      case OP_LHAX:
      case OP_L:
      case OP_LX:
      case OP_LHBRX:
      case OP_LBRX:
      case OP_LM:
      case OP_STB:
      case OP_STBX:
      case OP_STH:
      case OP_STHX:
      case OP_ST:
      case OP_STX:
      case OP_STHBRX:
      case OP_STBRX:
      case OP_STM:
      case OP_LBZU:
      case OP_LHZUX:
      case OP_LHAU:
      case OP_LHAUX:
      case OP_LU:
      case OP_LUX:
      case OP_STBU:
      case OP_STBUX:
      case OP_STHU:
      case OP_STHUX:
      case OP_STU:
      case OP_STUX:
      case OP_LSX:
      case OP_LSI:
      case OP_LSCBX:
      case OP_LSCBX_:
      case OP_STSX:
      case OP_STSI:
      case OP_CAL:
      case OP_CAU:
      case OP_LFS:
      case OP_LFSX:
      case OP_LFD:
      case OP_LFDX:
      case OP_LFSU:
      case OP_LFSUX:
      case OP_LFDU:
      case OP_LFDUX:
      case OP_STFS:
      case OP_STFSX:
      case OP_STFD:
      case OP_STFDX:
      case OP_STFSU:
      case OP_STFSUX:
      case OP_STFDU:
      case OP_STFDUX:
      case OP_RAC:
      case OP_RAC_:
      case OP_TLBI:
      case OP_MTSRI:
      case OP_MFSRI:
      case OP_DCLZ:
      case OP_CLF:
      case OP_CLI:
      case OP_DCLST:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_daisy_rz_op				*
 *				--------------				*
 *									*
 * Operations in which the RA field is immediate 0 if 0, instead of r0.	*
 * We call such fields RZ not RA.)					*
 *									*
 ************************************************************************/

is_daisy_rz_op (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LSX:
      case OP_LSI:
      case OP_LSCBX:
      case OP_LSCBX_:
      case OP_STSX:
      case OP_STSI:

      case OP_CAL:
      case OP_CAU:

      case OP_RAC:
      case OP_RAC_:
      case OP_TLBI:
      case OP_MTSRI:
      case OP_MFSRI:
      case OP_DCLZ:
      case OP_CLF:
      case OP_CLI:
      case OP_DCLST:
	 return TRUE;

      default:
	 return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_carry_ext_op				*
 *				---------------				*
 *									*
 * Returns TRUE if CA bit as read as an input AND used as a carry	*
 *	   extended input.  (Thus moving the XER to a GPR via MFXER	*
 *	   does not count.						*
 *									*
 ************************************************************************/

int is_carry_ext_op (op)
OPCODE2 *op;
{
   switch (op->b->op_num) {
      case OP_AE:
      case OP_AE_:
      case OP_AEO:
      case OP_AEO_:

      case OP_SFE:
      case OP_SFE_:
      case OP_SFEO:
      case OP_SFEO_:

      case OP_AME:
      case OP_AME_:
      case OP_AMEO:
      case OP_AMEO_:

      case OP_AZE:
      case OP_AZE_:
      case OP_AZEO:
      case OP_AZEO_:

      case OP_SFME:
      case OP_SFME_:
      case OP_SFMEO:
      case OP_SFMEO_:

      case OP_SFZE:
      case OP_SFZE_:
      case OP_SFZEO:
      case OP_SFZEO_:
         return TRUE;

      default:
         return FALSE;
   }
}

/************************************************************************
 *									*
 *				get_ca_ext_opcode			*
 *				-----------------			*
 *									*
 * Returns the extended opcode for the op using CA as an extended	*
 *	   input.  (If the input opcode is not of the correct type,	*
 *	   an assert failure occurs.)					*
 *									*
 ************************************************************************/

int get_ca_ext_opcode (op_num)
int op_num;
{
   switch (op_num) {
      case OP_AE:	return OP_EAE;
      case OP_AE_:	return OP_EAE_;
      case OP_AEO:	return OP_EAEO;
      case OP_AEO_:	return OP_EAEO_;

      case OP_SFE:	return OP_ESFE;
      case OP_SFE_:	return OP_ESFE_;
      case OP_SFEO:	return OP_ESFEO;
      case OP_SFEO_:	return OP_ESFEO_;

      case OP_AME:	return OP_EAME;
      case OP_AME_:	return OP_EAME_;
      case OP_AMEO:	return OP_EAMEO;
      case OP_AMEO_:	return OP_EAMEO_;

      case OP_AZE:	return OP_EAZE;
      case OP_AZE_:	return OP_EAZE_;
      case OP_AZEO:	return OP_EAZEO;
      case OP_AZEO_:	return OP_EAZEO_;

      case OP_SFME:	return OP_ESFME;
      case OP_SFME_:	return OP_ESFME_;
      case OP_SFMEO:	return OP_ESFMEO;
      case OP_SFMEO_:	return OP_ESFMEO_;

      case OP_SFZE:	return OP_ESFZE;
      case OP_SFZE_:	return OP_ESFZE_;
      case OP_SFZEO:	return OP_ESFZEO;
      case OP_SFZEO_:	return OP_ESFZEO_;

      default:		assert (1 == 0);  break;
   }
}

/************************************************************************
 *									*
 *				get_ca_ppc_opcode			*
 *				-----------------			*
 *									*
 * Returns the PowerPC opcode for the op using CA as an extended	*
 *	   input.  This has the effect of collapsing the new DAISY CA	*
 *	   opcodes back to their PowerPC values, thus allowing us	*
 *	   to stay within the 256 values allowed in the 8 bits passed	*
 *	   to "dvstats".						*
 *									*
 ************************************************************************/

int get_ca_ppc_opcode (op_num)
int op_num;
{
   switch (op_num) {
      case OP_EAE:	return OP_AE;
      case OP_EAE_:	return OP_AE_;
      case OP_EAEO:	return OP_AEO;
      case OP_EAEO_:	return OP_AEO_;

      case OP_ESFE:	return OP_SFE;
      case OP_ESFE_:	return OP_SFE_;
      case OP_ESFEO:	return OP_SFEO;
      case OP_ESFEO_:	return OP_SFEO_;

      case OP_EAME:	return OP_AME;
      case OP_EAME_:	return OP_AME_;
      case OP_EAMEO:	return OP_AMEO;
      case OP_EAMEO_:	return OP_AMEO_;

      case OP_EAZE:	return OP_AZE;
      case OP_EAZE_:	return OP_AZE_;
      case OP_EAZEO:	return OP_AZEO;
      case OP_EAZEO_:	return OP_AZEO_;

      case OP_ESFME:	return OP_SFME;
      case OP_ESFME_:	return OP_SFME_;
      case OP_ESFMEO:	return OP_SFMEO;
      case OP_ESFMEO_:	return OP_SFMEO_;

      case OP_ESFZE:	return OP_SFZE;
      case OP_ESFZE_:	return OP_SFZE_;
      case OP_ESFZEO:	return OP_SFZEO;
      case OP_ESFZEO_:	return OP_SFZEO_;

      default:		return op_num;
   }
}

/************************************************************************
 *									*
 *				get_load_size				*
 *				-------------				*
 *									*
 * Returns the number of bytes LOADED by "opcode".  An assert-fail	*
 * will result if "opcode" is not a LOAD.  LOADS with variable		*
 * numbers of bytes (e.g. LM, LSX, LSCBX, and LSI) return 0.		*
 *									*
 ************************************************************************/

int get_load_size (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_LBZ:
      case OP_LBZX:
      case OP_LBZU:
      case OP_LBZUX:
 	 return 1;

      case OP_LHZ:
      case OP_LHZX:
      case OP_LHA:
      case OP_LHAX:
      case OP_LHBRX:
      case OP_LHZU:
      case OP_LHZUX:
      case OP_LHAU:
      case OP_LHAUX:
 	 return 2;

      case OP_L:
      case OP_LX:
      case OP_LBRX:
      case OP_LU:
      case OP_LUX:
      case OP_LFS:
      case OP_LFSX:
      case OP_LFSU:
      case OP_LFSUX:
 	 return 4;

      case OP_LFD:
      case OP_LFDX:
      case OP_LFDU:
      case OP_LFDUX:
 	 return 8;

      case OP_LM:
      case OP_LSX:
      case OP_LSI:
      case OP_LSCBX:
      case OP_LSCBX_:
	 return 0;

      default:
	 assert (1 == 0);
   }
}

/************************************************************************
 *									*
 *				get_store_size				*
 *				--------------				*
 *									*
 * Returns the number of bytes STORED by "opcode".  An assert-fail	*
 * will result if "opcode" is not a STORE.  Stores with variable	*
 * numbers of bytes (e.g. STM, STSX, and STSI) return 0, as do		*
 * operations such as DCLZ which are not technically STORES.		*
 *									*
 ************************************************************************/

int get_store_size (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_STB:
      case OP_STBX:
      case OP_STBU:
      case OP_STBUX:
	 return 1;

      case OP_STH:
      case OP_STHX:
      case OP_STHBRX:
      case OP_STHU:
      case OP_STHUX:
	 return 2;

      case OP_ST:
      case OP_STX:
      case OP_STBRX:
      case OP_STU:
      case OP_STUX:
      case OP_STFS:
      case OP_STFSX:
      case OP_STFSU:
      case OP_STFSUX:
	 return 4;

      case OP_STFD:
      case OP_STFDX:
      case OP_STFDU:
      case OP_STFDUX:
	 return 8;

      case OP_STM:
      case OP_STSX:
      case OP_STSI:
	 return 0;

      /* Although not technically STORES, these modify memory */
      case OP_DCLZ:
      case OP_CLF:
      case OP_CLI:
      case OP_DCLST:
      case OP_DCS:
      case OP_ICS:
      case OP_TLBI:
	 return 0;

      default:
	 assert (1 == 0);
	 break;
   }
}

/************************************************************************
 *									*
 *				get_fmr_commit_type			*
 *				-------------------			*
 *									*
 * Returns:  Type of commit appropriate for floating point instruction.	*
 *	     No checking is done that "opcode" is indeed floating point.*
 *									*
 ************************************************************************/

int get_fmr_commit_type (opcode)
OPCODE2 *opcode;
{
   switch (opcode->b->op_num) {
      case OP_FA:
      case OP_FA_:
      case OP_FS:
      case OP_FS_:	return OP_FMR1;
	
      case OP_FM:
      case OP_FM_:	return OP_FMR2;
	
      case OP_FMA:
      case OP_FMA_:
      case OP_FMS:
      case OP_FMS_:
      case OP_FNMA:
      case OP_FNMA_:
      case OP_FNMS:
      case OP_FNMS_:	return OP_FMR3;
	
      case OP_FD:
      case OP_FD_:	return OP_FMR4;
	
      case OP_FRSP:
      case OP_FRSP_:	return OP_FMR5;

      default:		return OP_FMR;
   }
}

/************************************************************************
 *									*
 *			renameable_fp_arith_op				*
 *			----------------------				*
 *									*
 * Returns:  TRUE if this a floating point arithmetic op whose result	*
 *	     can be renamed.  For example, fa, fma, fd, etc.		*
 *									*
 ************************************************************************/

int renameable_fp_arith_op (op)
OPCODE2 *op;
{
   switch (op->b->op_num) {
      case OP_FA:
      case OP_FS:	return TRUE;
	
      case OP_FM:	return TRUE;
	
      case OP_FMA:
      case OP_FMS:
      case OP_FNMA:
      case OP_FNMS:	return TRUE;
	
      case OP_FD:	return TRUE;
      case OP_FRSP:	return TRUE;

#ifdef CAN_RENAME_FLT_COMPARES
      case OP_FCMPU:	return TRUE;
      case OP_FCMPO:	return TRUE;
#endif

      default:		return FALSE;
   }
}

/************************************************************************
 *									*
 *				is_uncond_br				*
 *				------------				*
 *									*
 * Returns:  TRUE if the instruction at "addr" is an unconditional	*
 *	     branch and is not a branch-and-link instruction.		*
 *									*
 ************************************************************************/

is_uncond_br (addr)
unsigned *addr;
{
   unsigned ins = *addr;
   unsigned primary;
   unsigned ext;

   /* Anything that is branch-and-link is not unconditional in the
      sense that the fall-thru instruction can be reached from this
      path.
   */
   if (ins & 1) return FALSE;

   primary = ins >> 26;
   switch (primary) {
      case 16:  if ((ins & 0x02800000) == 0x02800000) return TRUE;
		else				      return FALSE;

      case 18:					      return TRUE;

      /* Check for bcr and bcc */
      case 19:  ext = (ins >> 1) & 0x3FF;	
		if (ext != 16  &&  ext != 528)	      return FALSE; 
	        if ((ins & 0x02800000) == 0x02800000) return TRUE;
		else				      return FALSE;

      default:					      return FALSE;
   }

   assert (1 == 0);
}
