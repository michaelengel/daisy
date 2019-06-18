/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_resrc_map
#define H_resrc_map

#define UNMAPPED			(-1)
#define INTREG				0
#define FLTREG				1
#define CONDREG				2
#define LINKREG				3
#define SHAD_INTREG			4
#define SHAD_FLTREG			5
#define SHAD_CONDREG			6

#define LINK_BIT			1 /* Bit for set LR in branch ins   */

/* Use register r13 as pointer to (non-real/mapped) VLIW registers */
#define RESRC_PTR			13

/* Use an illegal value -- there is no GPR=-1 nor FPR=-1 */
#define NO_MAP				(-1)
#define MAP_UNINIT			(-2)

typedef struct {
   unsigned       offset;		/* Offset of original rel to r13 */
   unsigned short orig;			/* Resource   used by orig ins */
	    char  type;			/* Resrc type used by orig ins */
} RESRC_MAP;

#endif
