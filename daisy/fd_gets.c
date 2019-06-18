/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

/*======================================================================*
 * Make a primitive version of fgets which works on a buffer containing	*
 * the entire file.  This fdgets may only be used on one file at a time.*
 *======================================================================*/

#include <stdio.h>
#include <assert.h>

static char *file_end;
static char *posn;
static int  doing_fdgets;

void   init_fdgets (char *, int);
char       *fdgets (char *, int);
void finish_fdgets (void);

/************************************************************************
 *									*
 *				init_fdgets				*
 *				-----------				*
 *									*
 ************************************************************************/

void init_fdgets (file_start, file_size)
char *file_start;
int  file_size;
{
   assert (!doing_fdgets);

   file_end = file_start + file_size;
   posn     = file_start;

   doing_fdgets = TRUE;
}

/************************************************************************
 *									*
 *				finish_fdgets				*
 *				-------------				*
 *									*
 ************************************************************************/

void finish_fdgets ()
{
   doing_fdgets = FALSE;
}

/************************************************************************
 *									*
 *				fdgets					*
 *				------					*
 *									*
 ************************************************************************/

char *fdgets (buff, max_size)
char *buff;
int  max_size;
{
   int size = 0;

   assert (doing_fdgets);

   max_size--;
   while (*posn != '\n') {
      if (posn < file_end)
	 if (size < max_size) buff[size++] = *posn++;
	 else { buff[size] = 0;  return buff;  }
      else if (size == 0)	 return 0;
      else    { buff[size] = 0;  return buff;  }
   }
   posn++;	buff[size] = 0;  return buff;		/* Bump past CR */
}
