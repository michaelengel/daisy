/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#define XLATOR_INVOC			0
#define PAGES_VISITED			1
#define USER_PAGES_VISITED		2
#define TOTAL_ENTRY_PTS			3
#define AVG_E_PTS_PER_PAGE		4
#define GROUP_DEFINES			5
#define GROUP_EXECS_10			6
#define GROUP_EXECS_5			7
#define GROUP_EXECS_2			8
#define GROUP_EXECS_1			9
#define OFFPAGE_DIR_BR			10
#define INDIR_CTR_BR			11
#define INDIR_LR_BR			12
#define INDIR_LR2_BR			13
#define TOTAL_CROSS_BR			14
#define NUM_SINGLE_HASH			15
#define MAX_MULTI_HASH			16
#define MAX_MULTI_INDEX			17
#define MAX_OPEN_PATHS			18
#define TOTAL_PPC_OPS			19
#define TOTAL_VLIWS			20
#define VLIW_OPS_SIZE			21
#define LD_VER_FAIL_SIZE		22
#define LD_VER_RECOMP_SIZE		23
#define LD_VER_RECOMP_CNT		24
#define LD_VER_FAIL			25
#define LD_VER_SITE			26
#define XLATED_CODE_SIZE		27
#define ORIG_CODE_SIZE			28
#define AVG_XLATE_PAGE_SIZE		29

#define NUM_STATS			30

typedef struct {
   char     *name;
   int	    index;
   unsigned cnt;
} CNT;

typedef struct {
   void	    *func;
   unsigned toc;
   unsigned other;
} PTRGL;
