/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#include "simul.h"

static char *next_free_mem;

extern int  errno;
extern char *errno_text[];

/************************************************************************
 *									*
 *				init_get_obj_mem			*
 *				----------------			*
 *									*
 ************************************************************************/

init_get_obj_mem ()
{
   next_free_mem = (char *) XLATE_MEM_SEGMENT;
}

/************************************************************************
 *									*
 *				get_obj_mem				*
 *				-----------				*
 *									*
 * Used for allocating memory in segments 3 and above.  This memory	*
 * should be used for large static arrays, and not like malloc, since	*
 * there is no way to free it.						*
 *									*
 ************************************************************************/

void *get_obj_mem (size)
int size;				/* In bytes */
{
   char *rval = next_free_mem;

   next_free_mem += size;

   return rval;
}
