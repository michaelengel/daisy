/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#define NUM_PRIMARY_OPCODES		16
#define MAX_DIFF_EXT_SIZES		2

typedef struct {
   int num;
   int list[MAX_DIFF_EXT_SIZES];
} EXTSIZE;

EXTSIZE extsize_list[NUM_PRIMARY_OPCODES];

typedef struct {
   double cnt;
   int    opcode;
} OPCNT;

typedef struct {
   unsigned char  primary;
   unsigned short ext;
   unsigned char  exttype;
   unsigned char  extsize;
   char		  *name;
   int		  page;
   double	  spec_cnt;
   double	  perf_cnt;
} OPCODE;

void set_extsize_list (void);
void check_ascending (void);
OPCODE *get_opcode (unsigned);
int opcode_cmp (OPCODE *, OPCODE *);

