/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>

/* This directory must match that in read_vliwcnts.c. */

const char *vliw_info_dir	   = "/tmp/vliw_info/";

char *get_info_dir_name (char *, char *);

/************************************************************************
 *									*
 *				get_info_dir_name			*
 *				-----------------			*
 *									*
 ************************************************************************/

char *get_info_dir_name (special_info_dir, buff)
char *special_info_dir;
char *buff;
{
   int  len;
   char *info_dir;
   char *user_id;
   char user_name[L_cuserid];

   if (!special_info_dir) {
      strcpy (buff, vliw_info_dir);
      user_id = cuserid (user_name);
      if (!user_id) {
	 printf ("Could not obtain userid for use in directory name\n");
	 exit (1);
      }
      strcat (buff, user_id);
      strcat (buff, "/");
      info_dir = buff;
   }
   else {
      /* Make sure the name ends in a '/' */
      len = strlen (special_info_dir);

      if (special_info_dir[len-1] == '/') info_dir = special_info_dir;
      else {
	 strcpy (buff, special_info_dir);
	 buff[len]   = '/';
	 buff[len+1] = 0;
	 info_dir = buff;
      }
   }

   return info_dir;
}
