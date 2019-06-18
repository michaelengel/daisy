/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_simul
#define H_simul

#ifdef NORMAL_COMPILATION
#define XLATE_MEM_SIZE			0x1000000	/*   16 Mbytes */
#else
#define XLATE_MEM_SIZE			0xC000000	/*  196 Mbytes */
#endif

#define XLATE_MEM_WORDS			(XLATE_MEM_SIZE/4)
#define XLATE_MEM_SEGMENT		0x30000000

#endif
