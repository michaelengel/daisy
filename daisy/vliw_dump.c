/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>

#include "vliw.h"

extern int	   do_vliw_dump;	/* Create daisy.vliw with VLIW code   */
extern int	   num_gprs;
extern OPCODE1	   illegal_opcode;
extern PPC_OP_FMT  *ppc_op_fmt[];
extern unsigned	   xlate_entry_cnt;
       unsigned	   total_vliw_ops_dumped;
       unsigned    vliw_cnt = 1;

static unsigned    curr_path_num;

/************************************************************************
 *									*
 *				vliw_dump_group				*
 *				---------------				*
 *									*
 * Dump out an assembly listing of all the VLIW's reachable from"vliw"	*
 * and in the same group.						*
 *									*
 ************************************************************************/

vliw_dump_group (fd, vliw, group)
int  fd;
VLIW *vliw;
int  group;
{
   int  i;
   int  num_vliws;
   VLIW *vliw_list[MAX_VLIWS_PER_PAGE];

   num_vliws = make_vliw_list (vliw, vliw_list, MAX_VLIWS_PER_PAGE, group);

   if (!do_vliw_dump) return;

   for (i = 0; i < num_vliws; i++) {
/*    fdprintf (fd, "\nV%u_%08x:\n", xlate_entry_cnt, vliw_list[i]); */
      fdprintf (fd, "\nV%u:\n", vliw_cnt++);
      tip_dump (fd, vliw_list[i]->entry);
   }
}

/************************************************************************
 *									*
 *				tip_dump				*
 *				--------				*
 *									*
 * Dump out an assembly listing of "tip" and (recursively) all the tips	*
 * below it in the tree.						*
 *									*
 ************************************************************************/

tip_dump (fd, tip)
int fd;
TIP *tip;
{
   int     i;
   int     index;
   BASE_OP *bop;
   OPCODE1 *op1;
   OPCODE2 *op;
   OPCODE2 *op_list[MAX_OPS_PER_VLIW];
   char    label[32];

   if (tip->br_addr)
	fdprintf (fd, "T%u_%08x:\t\t\t\t\t# 0x%08x, Cnts Offset: %x\n",
		  xlate_entry_cnt, tip, tip->br_addr, 4 * curr_path_num);
   else fdprintf (fd, "T%u_%08x:\t\t\t\t\t#             Cnts Offset: %x\n",
		  xlate_entry_cnt, tip, 4 * curr_path_num);

   sprintf (label, "T%u_%08x", xlate_entry_cnt, tip->right);

   total_vliw_ops_dumped += tip->num_ops + 1; /* The 1 is for the tip branch */
   bop = tip->op;
   for (i = 0; i < tip->num_ops; i++, bop = bop->next)
      op_list[i] = bop->op;

   /* The ops in the list were in reverse order.  Output code in fwd order */
   for (i = 0, index = tip->num_ops - 1; i < tip->num_ops; i++) {
      op = op_list[index--];
      if (op->b->illegal) fdprintf (fd, "\t\tILLEGAL");
      else {
	 fdprintf (fd, "\t\t%s%s\t", op->b->name, op->op.verif ? ".v" : "");
	 dump_vliw_operands (fd, tip, op, label);
      }
      if (op->op.verif) fdprintf (fd, ",0x%08x", op->op.orig_addr);
      fdprintf (fd, "\n");
   }

   op1 = get_opcode (tip->br_ins);
   op  = set_operands (op1, tip->br_ins, op1->b.format);
   fdprintf (fd, "\t\t%s\t", op->b->name);
   if (tip->br_type != BR_ONPAGE) dump_vliw_operands (fd, tip, op, "OFFPAGE");
   else {
      dump_vliw_operands (fd, tip, op, label); 
      if (tip->left != 0) fdprintf (fd, "\t#\t      SKIP");
   }
   fdprintf (fd, "\n");
      
   if (tip->left  &&  tip->br_type == BR_ONPAGE) {
      tip_dump (fd, tip->left);
      tip_dump (fd, tip->right);
   }
   else curr_path_num++;		/* Match numbering in "simul.c" */
}

/************************************************************************
 *									*
 *			dump_vliw_operands				*
 *			------------------				*
 *									*
 ************************************************************************/

dump_vliw_operands (fd, tip, op, label)
int     fd;
TIP	*tip;
OPCODE2 *op;
char	*label;
{
   int		i;
   int		num_operands;
   unsigned     format;
   PPC_OP_FMT	*curr_format;

   format       = op->b->format;
   assert (format < MAX_PPC_FORMATS);

   curr_format  = ppc_op_fmt[format];
   num_operands = curr_format->num_operands;

   dump_vliw_operand_val (fd, tip, op, curr_format, 0, "  ", label);

   for (i = 1; i < num_operands; i++) {
      dump_vliw_operand_val (fd, tip, op, curr_format, i, ",", label);
   }
}

/************************************************************************
 *									*
 *			dump_vliw_operand_val				*
 *			---------------------				*
 *									*
 ************************************************************************/

dump_vliw_operand_val (fd, tip, op, curr_format, num, prefix, label)
int	    fd;
TIP	    *tip;
OPCODE2     *op;
PPC_OP_FMT  *curr_format;
int	    num;
char	    *prefix;
char	    *label;
{
   int		op_num;
   int		type;
   static int	prev_type = SI;		/* Anything but D */
   unsigned	val;
   OPERAND	*operand;

   op_num  = curr_format->ppc_asm_map[num];
   operand = &curr_format->list[op_num];
   type    = operand->type;

   if ((type & (~OPERAND_BIT)) < NUM_RENAMEABLE_OPERANDS) {
      if (type == BI) val = tip->br_condbit;
      else {
        val = op->op.renameable[type & (~OPERAND_BIT)];
	if (val >= SHDR0  &&  val < SHDR0 + num_gprs)
	   val -= SHDR0 - R0;
      }
   }
   else val = extract_operand_val (op->op.ins, operand);

   if      (dump_is_int_reg_zero (type, val))
      if (prev_type == D)
           fdprintf (fd, "(r%d)",	    val - R0);
      else fdprintf (fd, "%sr%d",    prefix, val - R0);
   else if (dump_is_flt_reg (type))
           fdprintf (fd, "%sfp%d",   prefix, val - FP0);
   else if (dump_is_cc_bit (type))
           fdprintf (fd, "%s%d",   prefix, val - CR0);
   else if (dump_is_cc_field (type))
           fdprintf (fd, "%s%d",   prefix, (val - CR0) >> 2);
   else if (type == BD  ||  type == LI)
	   fdprintf (fd, "%s%s",  prefix, label);
   else {
	   val = chk_immed_val (val, type);
	   fdprintf (fd, "%s%d",     prefix, val);
   }

   if (type == D) prev_type = D;
   else prev_type = SI;			/* Anything but D */
}

/************************************************************************
 *									*
 *				vliw_dump_reexec_info			*
 *				---------------------			*
 *									*
 ************************************************************************/

vliw_dump_reexec_info (fd, fcn)
int fd;
int (*fcn) ();			/* read or write */
{
   assert (fcn (fd, &vliw_cnt, sizeof (vliw_cnt)) == sizeof (vliw_cnt));
	   
   assert (fcn (fd, &total_vliw_ops_dumped, sizeof (total_vliw_ops_dumped)) ==
	   sizeof (total_vliw_ops_dumped));
	   
   assert (fcn (fd, &curr_path_num, sizeof (curr_path_num)) ==
	   sizeof (curr_path_num));
}
