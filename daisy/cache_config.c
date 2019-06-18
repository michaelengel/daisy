/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "cache.h"
#include "cacheconst.h"

#define MAX_CACHE_CONFIG_FILE_SIZE		8192

int		  main_mem_latency;
CACHE_INFO cache_info[NUM_CACHES];
CACHE_INFO tlb_info[NUM_CACHES];      /* Allow 3 levels even though not need */

static unsigned get_mult (char *);

/************************************************************************
 *									*
 *				init_cache_simul			*
 *				----------------			*
 *									*
 *									*
 ************************************************************************/

init_cache_simul (prog_name)
char *prog_name;
{
   get_config (prog_name, cache_info, "cache.config");
   get_config (prog_name,   tlb_info,   "tlb.config");

   init_caches ();
}

/************************************************************************
 *									*
 *				get_config				*
 *				----------				*
 *									*
 ************************************************************************/

get_config (prog_name, cache, config_fname)
char	   *prog_name;
CACHE_INFO *cache;
char	   *config_fname;
{
   unsigned   mult;
   char	      ctype;
   int	      clevel;
   int	      cnum;
   int	      cnt;
   FILE	      *fp;
   CACHE_INFO *c;
   char	      buff[128];
   char       mult_str[128];

   int    fd;
   int    size;
   char   *posn;
   char	  identifier_name[1024];
   char	  config_file[MAX_CACHE_CONFIG_FILE_SIZE];

   fd = open_cache_config_file (prog_name, &size, config_fname);
   if (fd != (-1)) {
      assert (size <= MAX_CACHE_CONFIG_FILE_SIZE);
      assert (read (fd, config_file, size) == size);
      close (fd);
   }

   init_fdgets (config_file, size);
   main_mem_latency = -1;

   while (fdgets (buff, 126)) {
      if (sscanf (buff, "Cache: %c%d", &ctype, &clevel) != 2) {
	 sprintf (buff, "Could not read Cache Type or Level from \"%s\"",
		  config_fname);
         print_usage (prog_name, buff);
      }

      switch (ctype) {
	 case 'd':
	 case 'D':	ctype = DATA;	 break;

	 case 'i':
	 case 'I':	ctype = INSTR;	 break;

	 case 'j':
	 case 'J':	ctype = JOINT;	 break;

	 case 'm':
	 case 'M':	ctype = MAINMEM; break;

	 default:
	    fprintf (stderr, "Unknown Cache Type:  %c\n", ctype);
	    exit (1);
      }

      if (ctype == MAINMEM) {
         fdgets (buff, 126);
         cnt = sscanf (buff, "Latency: %d", &main_mem_latency);

         if (cnt == 1) break;   /* Main mem latency should be the last item */
         else {
	    sprintf (buff, "Could not read Main Memory Latency from \"%s\"",
		     config_fname);
	    print_usage (prog_name, buff);
	 }
      }

      if (clevel < 0  || clevel >= NUM_CACHE_LEVELS) {
         fprintf (stderr, "Illegal Cache Level (%d).  Range is [0,%d]\n",
		  clevel, NUM_CACHE_LEVELS - 1);
	 exit (1);
      }

      cnum = clevel * NUM_CACHE_TYPES + ctype;
      c = &cache[cnum];

      fdgets (buff, 126);
      cnt = sscanf (buff, "Cache Size: %d%s", &c->cachesz, mult_str);
      if (cnt != 1  &&  cnt != 2) {
	 sprintf (buff, "Could not read Cache Size from \"%s\"", config_fname);
         print_usage (prog_name, buff);
      }

      if (cnt == 2) {
         mult = get_mult (mult_str);
         c->cachesz *= mult;
      }

      fdgets (buff, 126);
      cnt = sscanf (buff, "Associativity: %d%s", &c->assoc, mult_str);
      if (cnt != 1  &&  cnt != 2) {
	 sprintf (buff, "Could not read Cache Associativity from \"%s\"",
		  config_fname);
         print_usage (prog_name, buff);
      }

      if (cnt == 2) {
         mult = get_mult (mult_str);
         c->assoc *= mult;
      }

      fdgets (buff, 126);
      cnt = sscanf (buff, "Line Size: %d%s", &c->linesz, mult_str);
      if (cnt != 1  &&  cnt != 2) {
	 sprintf ("Could not read Cache Line Size from \"%s\"", config_fname);
         print_usage (prog_name, buff);
      }

      if (cnt == 2) {
         mult = get_mult (mult_str);
         c->linesz *= mult;
      }

      fdgets (buff, 126);
      cnt = sscanf (buff, "Latency: %d", &c->latency);
      if (cnt != 1) {
	 sprintf (buff, "Could not read Cache Latency from \"%s\"",
		  config_fname);
         print_usage (prog_name, buff);
      }

      /* Blank line should separate entries */
      fdgets (buff, 126);
   }

   finish_fdgets ();
   chk_cache_config_consis (config_fname, cache);
}

/************************************************************************
 *									*
 *			open_cache_config_file				*
 *			----------------------				*
 *									*
 * Open the configuration file "cache.config".  First look in the	*
 * current directory.  If "cache.config" does not exist, look in the	*
 * directory specified by the environment variable CACHE_CONFIG.  If	*
 * "cache.config" does not exist there either, warn the user and use	*
 * default values.  Also return the size of the "cache.config" file.	*
 *									*
 ************************************************************************/

int open_cache_config_file (prog_name, p_size, config_fname)
char *prog_name;
int  *p_size;				/* Output */
char *config_fname;
{
   int		fd;
   int		len;
   int		stat_val;
   struct stat	file_stat;
   char		*path;
   char		buff[1024];

   fd = open (config_fname, O_RDONLY);

   if (fd != -1) path = config_fname;
   else {
      path = (char *) getenv ("CACHE_CONFIG");

      if (!path) path = config_fname;
      else {
	 len = strlen (path);
	 if (path[len-1] != '/') {
	    strcpy (buff, path);
	    strcat (buff, "/");
            path = buff;
	 }
         strcat (path, config_fname);
      }


      fd = open (path, O_RDONLY);
      if (fd == -1) {
	 sprintf (buff, "Could not open \"%s\"", config_fname);
	 print_usage (prog_name, buff);
      }
   }

   if (fd != -1) {
      stat_val = stat (path, &file_stat);
      assert (stat_val == 0);
      *p_size = file_stat.st_size;
   }

   return fd;
}

/************************************************************************
 *									*
 *				is_level0_data_cache			*
 *				--------------------			*
 *									*
 * Return TRUE if only 1 cache is specified and that is a level 0	*
 * data cache.								*
 *									*
 ************************************************************************/

is_level0_dcache (cache)
CACHE_INFO *cache;
{
   int i;
   int num_caches = 0;

   for (i = 0; i < NUM_CACHES; i++)
      if (cache[i].cachesz != 0) num_caches++;

   if (num_caches != 1) return FALSE;
   else if (cache[0 * NUM_CACHE_TYPES + DATA].cachesz == 0) return FALSE;
   else return TRUE;
}

/************************************************************************
 *									*
 *				chk_cache_config_consis			*
 *				-----------------------			*
 *									*
 ************************************************************************/

static chk_cache_config_consis (fname, cache)
char *fname;
CACHE_INFO *cache;
{
   int	      i;
   int	      base_num;
   int	      ins_valid = TRUE;
   int	      data_valid = TRUE;
   int	      ins_linesz = 0;
   int	      data_linesz = 0;
   CACHE_INFO *c;

   if (main_mem_latency == -1) {
      fprintf (stderr, "Main Memory Latency not specified in \"%s\"\n", fname);
      exit (1);
   }

   for (i = 0; i < NUM_CACHE_LEVELS; i++) {

      base_num = i * NUM_CACHE_TYPES;
      c = &cache[base_num];

      if (c[JOINT].cachesz) {
	 if (c[INSTR].cachesz)     too_many_caches (fname, "Instruction", i);
         else if (c[DATA].cachesz) too_many_caches (fname, "Data", i);
	 if (c[JOINT].linesz < data_linesz ||
	     c[JOINT].linesz < ins_linesz)
	       invalid_line_size (fname, "Joint", i);
	 data_linesz = ins_linesz = c[JOINT].linesz;
      }
      else {
	 if (c[INSTR].cachesz) {
	    if (!ins_valid) skipped_cache_level (fname, "Instruc", i);
	    if (c[INSTR].linesz < ins_linesz)
	       invalid_line_size (fname, "Instruc", i);
	    ins_linesz = c[INSTR].linesz;
	 }
	 else ins_valid = FALSE;
	 if (c[DATA].cachesz) {
	    if (!data_valid) skipped_cache_level (fname, "Data", i);
	    if (c[DATA].linesz < data_linesz)
	       invalid_line_size (fname, "Data", i);
	    data_linesz = c[DATA].linesz;
	 }
	 else data_valid = FALSE;
      }
   }
}

/************************************************************************
 *									*
 *				too_many_caches				*
 *				---------------				*
 *									*
 ************************************************************************/

static too_many_caches (fname, typename, level)
char *fname;
char *typename;
int  level;
{
   fprintf (stderr, "%s: Specified both Joint cache and %s cache\n", fname, typename);
   fprintf (stderr, "at level %d.\n\n", level);
   exit (1);
}

/************************************************************************
 *									*
 *				invalid_line_size			*
 *				-----------------			*
 *									*
 ************************************************************************/

static invalid_line_size (fname, typename, level)
char *fname;
char *typename;
int  level;
{
   fprintf (stderr, "%s: Level %d %s cache has a line size smaller than\n",
	    fname, level, typename);
   fprintf (stderr, "a cache containing that information at an earlier level.\n\n");
   exit (1);
}

/************************************************************************
 *									*
 *				skipped_cache_level			*
 *				-------------------			*
 *									*
 ************************************************************************/

static skipped_cache_level (fname, typename, level)
char *fname;
char *typename;
int  level;
{
   fprintf (stderr, "%s: Level %d %s cache defined, but no %s or Joint,\n",
	    fname, level, typename, typename);
   fprintf (stderr, "cache at an earlier level.\n\n");
   exit (1);
}

/************************************************************************
 *									*
 *				get_mult				*
 *				--------				*
 *									*
 * Get the multiplier for the current value.  Currently accept:		*
 *									*
 *	k	(kbytes)						*
 *	m	(mbytes)						*
 *	w	(VLIW Instruction Word (64 bytes)			*
 *									*
 ************************************************************************/

static unsigned get_mult (mult_str)
char *mult_str;
{
   char *p = mult_str;

   while (*p) {
      switch (*p) {
	 case 'k':
	 case 'K':	return 1024;	break;

	 case 'm':
	 case 'M':	return 1048576;	break;

	 case 'w':
	 case 'W':	return WORDSZ;	break;

	 default:	break;
      }
      p++;
   }

   return 1;
}
