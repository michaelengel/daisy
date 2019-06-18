/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/* Primary optypes--each Ins should have at most one */
#define ALU_OP			0x0001
#define MEM_OP			0x0002
#define BRA_OP			0x0004

/* Secondary optypes--combinations of these may be OR'ed w/ primary optypes */
#define INT_OP			0x0008
#define FLT_OP			0x0010
#define LOAD_OP			0x0020
#define LOAD_TOC_OP		0x0040
#define STORE_OP		0x0080
#define COPY_OP			0x0100
#define CR_OP			0x0200
#define SP_OP			0x0400
#define TRAP_OP			0x0800
#define SIMPLE_OP		0x1000
#define ACB_OP			0x2000

typedef struct {
   char		   *vliw_name;
   short  	    opcode;
   short            num_src;
   unsigned         optype;
} OP_MAPPING;
