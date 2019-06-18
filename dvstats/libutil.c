/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>

/************************************************************************
 *									*
 *				is_string_number			*
 *				----------------			*
 *									*
 ************************************************************************/

is_string_number (s)
char *s;
{
   int  period_cnt = 0;
   char *p;

   for (p = s; *p; p++) {
      if ((!isdigit (*p))  &&  *p != '-'  &&  !isspace(*p)) {
         if (*p != '.') return FALSE;
         else if (period_cnt++ != 0) return FALSE;
      }
   }

   return TRUE;
}

/************************************************************************
 *									*
 *				has_minus				*
 *				---------				*
 *									*
 ************************************************************************/

has_minus (s)
char *s;
{
   char *p;

   for (p = s; *p; p++)
      if (*p == '-') return TRUE;

   return FALSE;
}

/************************************************************************
 *									*
 *				has_period				*
 *				----------				*
 *									*
 ************************************************************************/

has_period (s)
char *s;
{
   char *p;

   for (p = s; *p; p++)
      if (*p == '.') return TRUE;

   return FALSE;
}

/************************************************************************
 *									*
 *				strip_suffix				*
 *				------------				*
 *									*
 * Removes the final suffix from name -- for example "foo.vliwasm0"	*
 * becomes just "foo".  This is done in place in the passed "name".	*
 *									*
 ************************************************************************/

strip_suffix (name)
char *name;
{
   char *p;
   char *pdot = 0;

   for (p = name; *p; p++)
      if (*p == '.') pdot = p;

   if (pdot) {
      if (!strcmp (pdot+1, "vliwasm0")  ||
	  !strcmp (pdot+1, "vliwinp")	||
	  !strcmp (pdot+1, "s")		||
	  !strcmp (pdot+1, "o")		||
	  !strcmp (pdot+1, "c")		||
	  !strcmp (pdot+1, "f"))
         *pdot = 0;
   }
}

/************************************************************************
 *									*
 *				normalize_token				*
 *				---------------				*
 *									*
 * This is similar to "check_var_basename".				*
 *									*
 ************************************************************************/

normalize_token (target, source)
char *target;
char *source;
{
   while (*source) {
      if (*source == '$') *target = 'Z';
      else if (*source == '@') *target = 'A';
      else if (*source == '&') *target = 'X';
      else if (*source == '-') *target = '_';
      else *target = *source;

      source++;
      target++;
   }
   *target = 0;
}
