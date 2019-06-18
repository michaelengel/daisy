/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_opcode_fmt
#define H_opcode_fmt

#include <values.h>
#include "resrc_list.h"

#define MAX_PPC_OPERANDS	5		/* M format instructions */
#define MAX_PPC_FORMATS		256		/* Fits in unsigned char */

#define MAX_RPAGESIZE		16384		/* 16384 byte pages */
#define MAX_RPAGESIZE_MASK	(MAX_RPAGESIZE-1)
#define MAX_RPAGE_ENTRIES	(MAX_RPAGESIZE>>2)
#define MAX_RPAGESIZE_LOG	14

#define MIN_RPAGESIZE		256		/* 256 byte pages */
#define MIN_RPAGESIZE_MASK	(MIN_RPAGESIZE-1)
#define MIN_RPAGE_ENTRIES	(MIN_RPAGESIZE>>2)
#define MIN_RPAGESIZE_LOG	8

/* Types of PPC operands: */

/* Make sure differ from real resources (i.e. less than 16384 real resources)*/
#define OPERAND_BIT			0x4000

#define RT				(OPERAND_BIT|0)
#define RS				(OPERAND_BIT|1)
#define RA				(OPERAND_BIT|2)
#define RB				(OPERAND_BIT|3)
#define RZ				(OPERAND_BIT|4)

#define FRT				(OPERAND_BIT|5)
#define FRS				(OPERAND_BIT|6)
#define FRA				(OPERAND_BIT|7)
#define FRB				(OPERAND_BIT|8)
#define FRC				(OPERAND_BIT|9)

#define BT				(OPERAND_BIT|10)	/* CC Bits   */
#define BA				(OPERAND_BIT|11)
#define BB				(OPERAND_BIT|12)
#define BI				(OPERAND_BIT|13)
#define BF				(OPERAND_BIT|14)	/* CC Fields */
#define BFA				(OPERAND_BIT|15)
#define BF2				(OPERAND_BIT|16)

#define GCA				(OPERAND_BIT|17)
#define GOV				(OPERAND_BIT|18)

#define NUM_RENAMEABLE_OPERANDS		19

#define BFB				(OPERAND_BIT|19)    /* FPSCR Fields */
#define BFF				(OPERAND_BIT|20)    
#define BFF2				(OPERAND_BIT|21)    
#define BTF				(OPERAND_BIT|22)    /* FPSCR Bits   */

#define BO				(OPERAND_BIT|23)

#define RTM				(OPERAND_BIT|24) /* For lm  instruc */
#define RTS				(OPERAND_BIT|25) /* For lsi instruc */
#define RSM				(OPERAND_BIT|26) /* For stm  instruc */
#define RSS				(OPERAND_BIT|27) /* For stsi instruc */

#define FXM				(OPERAND_BIT|28)
#define FLM				(OPERAND_BIT|29)

#define SPR_F				(OPERAND_BIT|30)
#define SPR_T				(OPERAND_BIT|31)
#define SR				(OPERAND_BIT|32)

#define D				(OPERAND_BIT|33)
#define SI				(OPERAND_BIT|34)
#define UI				(OPERAND_BIT|35)
#define SH				(OPERAND_BIT|36)
#define NB				(OPERAND_BIT|37)
#define MB				(OPERAND_BIT|38)
#define ME				(OPERAND_BIT|39)
#define TO				(OPERAND_BIT|40)
#define BD				(OPERAND_BIT|41)
#define LI				(OPERAND_BIT|42)
#define I				(OPERAND_BIT|43)
#define BFEX				(OPERAND_BIT|44)

#define FL1				(OPERAND_BIT|45)
#define FL2				(OPERAND_BIT|46)
#define LEV				(OPERAND_BIT|47)
#define SV				(OPERAND_BIT|48)
#define NUM_PPC_OPERAND_TYPES		49

typedef struct {
   short     type;
   char      shift;
   unsigned  dest   : 1;
   unsigned  src    : 1;
   unsigned  mask;
} OPERAND;

typedef struct {
   char    template;   
   char    srcdest_common;		/* Neg if none, o.w. index to list */
   char    num_operands;
   char    ppc_asm_map[MAX_PPC_OPERANDS];
   OPERAND list[MAX_PPC_OPERANDS];
} PPC_OP_FMT;

#endif
