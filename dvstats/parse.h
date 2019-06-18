/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_parse
#define H_parse

#ifndef H_opcodes
#include "opcodes.h"
#endif

#define MAX_TOKEN_SIZE		4096	/* Max chars in any token (word) */

/* These must have different values than EOF */
#define OPEN_PAREN_TOKEN	(EOF+1)
#define CLOSE_PAREN_TOKEN	(EOF+2)
#define OTHER_TOKEN		(EOF+3)
#define ANNOTATE_TOKEN		(EOF+4)

#define VARIABLE_NUM_OPERANDS		(-1)
#define DEFAULT_NUM_OPERANDS		16

#define MAX_PPC_OPERANDS                14
#define MAX_VLIW_OPERANDS		14

/*========================================================================*
 * NOTE:  OPERATOR and VR_OPERATOR have the first 3 fields in common,	  *
 *	  however, nothing currently depends on this.			  *
 *========================================================================*/

typedef struct operator_struct          OPERATOR;
typedef struct vr_operator_struct       VR_OPERATOR;
typedef struct vliw_opcode_struct       VLIW_OPCODE;

typedef struct operator_struct {
   int  vliw_op;		/* Just for consistency checking */
   char *vliw_name;
   int  num_vliw_operands;
};

typedef struct vr_operator_struct {
   int      vliw_op;            /* Code for VLIW operation               */
   char     special_flag;       /* If translation needs special handling */
   char     *ppc_name;          /* Text String of PowerPC operation "add"*/
   int      num_vliw_operands;  /*                                       */
                                /* operand_map[i] = # of corresp VLIW    */
   char     operand_map[MAX_PPC_OPERANDS];
                                /*                  operand (0,1,...)    */
   int      num_ppc_operands;   /*                                       */
                                /* Type of each operand (eg. INTREG_SRC) */
   unsigned operand_type[MAX_PPC_OPERANDS];
};

#define SPECIAL 1

typedef struct vliw_opcode_struct {
   int	       num_entries;
   int	       num_alloc;
   VR_OPERATOR **list;
};

#define VARIABLE_OPERANDS_FLAG	0x01

typedef struct {
   short     op_type0;
   short     pos;
   short     op_type;
   short     tokens_seen;
   int	     line_num;
   unsigned  flags;
   int	     operands_alloc;	/* Used only if tokens_remaining is variable */
   char	     **operand_list;
} EXPR;

/* Legal VLIW Operators (Not Including Non-Branch Machine Instructions) */
#define DORG			 (0+NUM_OPCODES)
#define DEFCON			 (1+NUM_OPCODES)
#define DEFSYM			 (2+NUM_OPCODES)
#define DEFEQU			 (3+NUM_OPCODES)
#define RELOC			 (4+NUM_OPCODES)
#define RELOC_TOC		 (5+NUM_OPCODES)
#define RELOC_BR		 (6+NUM_OPCODES)
#define RELOC_LAB		 (7+NUM_OPCODES)
#define EXTERN			 (8+NUM_OPCODES)
#define SYMTAB_ENTRY		 (9+NUM_OPCODES)
#define PLUS			(10+NUM_OPCODES)
#define MINUS			(11+NUM_OPCODES)

#define PROC			(12+NUM_OPCODES)
#define PEND			(13+NUM_OPCODES)

#define GOTO			(14+NUM_OPCODES)
#define MGOTO			(15+NUM_OPCODES)
#define CALL			(16+NUM_OPCODES)
#define CALLF			(17+NUM_OPCODES)

#define E_CC			(18+NUM_OPCODES)
#define NE_CC			(19+NUM_OPCODES)
#define L_CC			(20+NUM_OPCODES)
#define NL_CC			(21+NUM_OPCODES)
#define H_CC			(22+NUM_OPCODES)
#define NH_CC			(23+NUM_OPCODES)
#define O_CC			(24+NUM_OPCODES)
#define NO_CC			(25+NUM_OPCODES)

#define IF_CLAUSE		(26+NUM_OPCODES)
#define ELSE_CLAUSE		(27+NUM_OPCODES)
#define SKIP			(28+NUM_OPCODES)
#define SKIP_TARGET		(29+NUM_OPCODES)

					/* Don't count irregular operators:
					   LABEL, DEST_REG, DOUBLE_OP, UNKNOWN
					*/
#define NUM_VLIW_OPERATORS	(30+NUM_OPCODES)

#define LABEL			(31+NUM_OPCODES)
#define DEST_REG		(32+NUM_OPCODES)
#define UNKNOWN			(33+NUM_OPCODES)
/*
#define DOUBLE_OP		(34+NUM_OPCODES)
*/

#endif
