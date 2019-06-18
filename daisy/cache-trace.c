/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include "cacheconst.h"

#define TRACE_BUFF_SIZE		       8192

typedef unsigned TRACE_ENTRY;

static int	   fd;
static int	   buff_cnt;
static TRACE_ENTRY trace_buff[TRACE_BUFF_SIZE];

void trace_init (char *);
void trace_done (void);
void trace_dump (unsigned, int);

/************************************************************************
 *									*
 *				trace_init				*
 *				----------				*
 *									*
 ************************************************************************/

void trace_init (name)
char *name;
{
   char buff[1024];

   buff_cnt = 0;

   strcpy (buff, name);
   strcat (buff, ".trace");

   fd = open (buff, O_CREAT | O_TRUNC | O_RDWR, 0644);
   if (fd < 0) {
      printf ("Could not open \"%s\" for writing\n", buff);
      exit (1);
   }
}

/************************************************************************
 *									*
 *				trace_done				*
 *				----------				*
 *									*
 ************************************************************************/

void trace_done ()
{
   int cnt;

   cnt = write (fd, trace_buff, buff_cnt * sizeof (trace_buff[0]));
   assert (cnt == buff_cnt * sizeof (trace_buff[0]));

   close (fd);
}

/************************************************************************
 *									*
 *				trace_dump				*
 *				----------				*
 *									*
 ************************************************************************/

void trace_dump (addr, accesstype)
unsigned addr;
int	 accesstype;
{
   int	    cnt;
   unsigned tt_addr;		 /* For Mark Charney's csim */
   unsigned tt_type;

   if (buff_cnt == TRACE_BUFF_SIZE) {
      cnt = write (fd, trace_buff, sizeof (trace_buff));
      assert (cnt == sizeof (trace_buff));
      buff_cnt = 0;
   }

   /* We support only LOADS, STORES, and FETCHES.  If the "assert" fails,
      add support for the appropriate new type.
   */
   switch (accesstype) {
      case CSTORE:	tt_type = 3;	 break;
      case CLOAD:	tt_type = 2;	 break;
      case INSFETCH:    tt_type = 1;	 break;
      default:	        assert (1 == 0); break;
   }

   tt_addr = (addr & 0xFFFFFFFC) | tt_type;     /* Strip 2 LSB used by type */
   trace_buff[buff_cnt] = tt_addr;

   buff_cnt++;
}
