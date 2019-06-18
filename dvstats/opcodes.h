/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_opcodes
#define H_opcodes

#ifndef PSOpcodes_h
#include "PSOpcodes.h"

#define NUM_OPCODES			DISPATCH_SIZE

#endif

/* If a VLIW operand (such as "_M") does not map to any PPC operand	 */
#define NO_MAPPING		(-1)
#define IMMED_ZERO		(-2)
#define NULL_OPERAND0		(-3)
#define NULL_OPERAND1		(-4)
#define NULL_OPERAND2		(-5)
#define NULL_OPERAND3		(-6)
#define NULL_OPERANDLAST	(-6)

typedef struct ppc_operand_struct    PPC_OPERAND;
typedef struct ppc_op_struct         PPC_OP;

typedef struct ppc_operand_struct {
   unsigned type;
   unsigned old_type;
   short    op0_or_op1;
   short    operand_num;
   char     ppc_regnum;
   char	    dead;
   int      num;
   int      old_num;
   double   flt_num;
   char     *label;
};

typedef struct ppc_op_struct {
   struct vr_operator_struct  *template0;
   struct vr_operator_struct  *template;
   short	opcode0;
   short	opcode;
   short	pos;
   short	num_src;
   short	num_dest;
   int		line_num;
   int		vliwinp_line;
   PPC_OPERAND  *src;
   PPC_OPERAND  *dest;
};

#endif
