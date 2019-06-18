/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

#include "vliw.h"
#include "brcorr.h"

#define MAX_COND_BR		0x10000		/* 64K */

typedef struct {
   unsigned	      target_cnt;
   unsigned	      fallthru_cnt;
   unsigned	      len;
   unsigned	      br_hist[MAX_BR_HIST_DEPTH];
} COND_BR;

static unsigned num_cond_br;
static unsigned br_corr_order;
static int	cmp_depth;
static COND_BR  cond_br[MAX_COND_BR];
static COND_BR  *lcond_br[MAX_COND_BR];		/* Sorted by length */
static COND_BR  *pcond_br[MAX_COND_BR];		/* Sorted by prefix */

static int  cmp_addr  (COND_BR *,  COND_BR *);
static int lcmp_addr  (COND_BR **, COND_BR **);
static int pcmp_addr  (COND_BR **, COND_BR **);
static int pcmp_exact (COND_BR **, COND_BR **);

/************************************************************************
 *									*
 *				read_pdf				*
 *				--------				*
 *									*
 * Reads the pdf file produced by "dvstats".				*
 * (See vstats/hash.c for details on how the info is collected.		*
 *									*
 * NOTE: The "get_condbr_cnts" routine below assumes that the		*
 *	 conditional branches are sorted in ascending order by		*
 *	 current branch address, then by previous branch address,	*
 *	 then by 2nd previous branch address, etc.			*
 *									*
 ************************************************************************/

read_pdf (fname)
char *fname;
{
   int	   i, j;
   int     cnt;
   int     fd;
   char    *base_fname;
   COND_BR *cb;
   char    buff[512];

   base_fname = strip_dir_prefix (fname);
   strcpy (buff, base_fname);
   strcat (buff, ".condbr");

   fd = open (buff, O_RDONLY);
   assert (fd >= 0);

   cnt = read (fd, &num_cond_br, sizeof (num_cond_br));
   assert (cnt == sizeof (num_cond_br));
   assert (num_cond_br < MAX_COND_BR);

   cnt = read (fd, &br_corr_order, sizeof (br_corr_order));
   assert (cnt == sizeof (br_corr_order));
   assert (br_corr_order < MAX_BR_HIST_DEPTH);

   for (i = 0; i < num_cond_br; i++) {
      cb = &cond_br[i];

      cnt = read (fd, cb->br_hist, (br_corr_order+1)*sizeof (cb->br_hist[0]));
      assert (cnt == (br_corr_order + 1) * sizeof (cb->br_hist[0]));
      
      cnt = read (fd, &cb->target_cnt, sizeof (cb->target_cnt));
      assert (cnt == sizeof (cb->target_cnt));
      
      cnt = read (fd, &cb->fallthru_cnt, sizeof (cb->fallthru_cnt));
      assert (cnt == sizeof (cb->fallthru_cnt));
   }

   for (i = 0; i < num_cond_br; i++) {
      cb = &cond_br[i];
      lcond_br[i] = &cond_br[i];
      pcond_br[i] = &cond_br[i];

      for (j = 0; j <= br_corr_order; j++)
	 if (cb->br_hist[j] == NO_CORR_BR_AVAIL) break;

      assert (j != 0);
      cb->len = j - 1;

/*    reverse_order (cb->br_hist, j); */
   }

   cmp_depth = br_corr_order;
   qsort (lcond_br, num_cond_br, sizeof (lcond_br[0]), lcmp_addr);
   qsort (pcond_br, num_cond_br, sizeof (pcond_br[0]), pcmp_addr);

   close (fd);
}

/************************************************************************
 *									*
 *				reverse_order				*
 *				-------------				*
 *									*
 * Reverse the order of the elements in the branch history.  "dvstats"	*
 * lists them with the current branch last in the order.  It would	*
 * probably be aesthetically better for "dvstats" to list them in the	*
 * "proper" order, but it is simpler to just reverse the order here.	*
 *									*
 ************************************************************************/

static reverse_order (br_hist, num_elem)
unsigned *br_hist;
int	 num_elem;
{
   int	    b, e;
   unsigned tmp;

   for (b = 0, e = num_elem - 1; b < e; b++, e--) {
      tmp        = br_hist[e];
      br_hist[e] = br_hist[b];
      br_hist[b] = tmp;
   }
}

/************************************************************************
 *									*
 *				get_condbr_cnts				*
 *				---------------				*
 *									*
 * RETURNS: The number of times the branch at the specified address	*
 *	    was taken and fell through in a previous run of the		*
 *	    program.  We return the counts instead of the probability	*
 *	    because it is probably more important to schedule a branch	*
 *	    target which is taken 1000 times, but with probability 30%,	*
 *	    than a branch which is taken 1 time with probability 100%.	*
 *									*
 * NOTE:    Assumes that the conditional branches are sorted in		*
 *	    ascending order by branch address.				*
 *									*
 ************************************************************************/

get_condbr_cnts (tip, addr, taken, fallthru)
TIP	 *tip;
unsigned addr;
unsigned *taken;
unsigned *fallthru;
{
   int	   daisy_depth;
   COND_BR key;
   COND_BR *pkey = &key;
   COND_BR **match;

   daisy_depth = set_br_hist (tip, addr, key.br_hist);

   /* DAISY branches form prefix to pdf branches.  See comments in	*/
   /* "chk_mult_matches".						*/
   if (daisy_depth < br_corr_order) {
      cmp_depth = daisy_depth;
      key.len   = daisy_depth;
      match = bsearch (&pkey, pcond_br, num_cond_br, sizeof (pcond_br[0]),
		       pcmp_addr);

      if (match)
	 if (pcmp_exact (&pkey, match)) {
	    *taken    = (*match)->target_cnt;
            *fallthru = (*match)->fallthru_cnt;
	    return;
	 }
         else if (!pcmp_prefix_exact (&pkey, match))
	    cmp_depth = daisy_depth - 1;
	 else return chk_mult_matches (pkey, match, taken, fallthru);
      else cmp_depth = daisy_depth - 1;
   }
   else cmp_depth = br_corr_order;

   /* We have seen at least as many conditional branches on the current	*/
   /* group path as the degree of branch correlation.  Thus the pdf	*/
   /* data has at most one match for our path.				*/
   assert (daisy_depth >= cmp_depth);

   while (cmp_depth >= 0) {
      key.len = cmp_depth;
      match = bsearch (&pkey, lcond_br, num_cond_br, sizeof (lcond_br[0]),
		       lcmp_addr);

      if (match) break;		
      else cmp_depth--;
   }

   if (match) {
      *taken    = (*match)->target_cnt;
      *fallthru = (*match)->fallthru_cnt;
   }
   else {
      *taken    = 0;
      *fallthru = 0;
   }
}

/************************************************************************
 *									*
 *				check_mult_matches			*
 *				------------------			*
 *									*
 * Called if the number of conditional branches we have seen thus far	*
 * along this path of the current group is less than degree of branch	*
 * correlation, e.g. we have seen 2 conditional branches and we are	*
 * using 5th order branch prediction.  In that case, multiple sequences	*
 * from the previous run may match our 2 conditional branches.  In such	*
 * cases, we base our statistics on all sequences whose most recent	*
 * two conditional branches match our 2 conditional branches.		*
 *									*
 ************************************************************************/

chk_mult_matches (pkey, anchor, taken, fallthru)
COND_BR  *pkey;
COND_BR  **anchor;
unsigned *taken;			/* Output */
unsigned *fallthru;			/* Output */
{
   unsigned total_taken    = (*anchor)->target_cnt;
   unsigned total_fallthru = (*anchor)->fallthru_cnt;
   COND_BR  **entry;

   /* Look backward from the current match for other matches */
   for (entry = anchor - 1; entry >= pcond_br; entry--) {
      if     (!pcmp_prefix_exact (&pkey, entry)) break;
      else if (pcmp_exact        (&pkey, entry)) {
	 *taken    = (*entry)->target_cnt;
	 *fallthru = (*entry)->fallthru_cnt;
	 return;
      }
      else {
	 total_taken    += (*entry)->target_cnt;
	 total_fallthru += (*entry)->fallthru_cnt;
      }
   }

   /* Look forward from the current match for other matches */
   for (entry = anchor + 1; entry < &pcond_br[num_cond_br]; entry++) {
      if     (!pcmp_prefix_exact (&pkey, entry)) break;
      else if (pcmp_exact        (&pkey, entry)) {
	 *taken    = (*entry)->target_cnt;
	 *fallthru = (*entry)->fallthru_cnt;
	 return;
      }
      else {
	 total_taken    += (*entry)->target_cnt;
	 total_fallthru += (*entry)->fallthru_cnt;
      }
   }

   *taken    = total_taken;
   *fallthru = total_fallthru;
}

/************************************************************************
 *									*
 *				set_br_hist				*
 *				-----------				*
 *									*
 ************************************************************************/

static set_br_hist (tip, addr, br_hist)
TIP	 *tip;
unsigned addr;
unsigned *br_hist;
{
   int cnt = 1;
   TIP *prev;

   br_hist[0] = addr;
   while (tip  &&  cnt <= br_corr_order) {
      if (prev = tip->prev) {			   /* Reached group start ? */
	 if (prev->right  &&  prev->left) {	   /* No, at cond branch  ? */
	    br_hist[cnt++] = prev->br_addr;
	 }
      }
      tip = tip->prev;
   }

   return cnt - 1;
}

/************************************************************************
 *									*
 *				cmp_addr				*
 *				--------				*
 *									*
 ************************************************************************/

static int cmp_addr (a, b)
COND_BR *a;
COND_BR *b;
{
   int i;

   for (i = 0; i <= cmp_depth; i++) {
      if      (a->br_hist[i] > b->br_hist[i]) return  1;
      else if (a->br_hist[i] < b->br_hist[i]) return -1;
   }

   return 0;
}

/************************************************************************
 *									*
 *				lcmp_addr				*
 *				---------				*
 *									*
 ************************************************************************/

static int lcmp_addr (pa, pb)
COND_BR **pa;
COND_BR **pb;
{
   int	   i;
   COND_BR *a = *pa;
   COND_BR *b = *pb;

   if      (a->len > b->len) return  1;
   else if (a->len < b->len) return -1;

   for (i = 0; i <= cmp_depth; i++) {
      if      (a->br_hist[i] > b->br_hist[i]) return  1;
      else if (a->br_hist[i] < b->br_hist[i]) return -1;
   }

   return 0;
}

/************************************************************************
 *									*
 *				pcmp_addr				*
 *				---------				*
 *									*
 ************************************************************************/

static int pcmp_addr (pa, pb)
COND_BR **pa;
COND_BR **pb;
{
   int	   i;
   int	   len;
   COND_BR *a = *pa;
   COND_BR *b = *pb;

   if (a->len < b->len) {
        if (a->len < cmp_depth) len = a->len;
	else                    len = cmp_depth;
   }
   else if (b->len < cmp_depth) len = b->len;
   else				len = cmp_depth;

   for (i = 0; i <= len; i++) {
      if      (a->br_hist[i] > b->br_hist[i]) return  1;
      else if (a->br_hist[i] < b->br_hist[i]) return -1;
   }

   if (len == cmp_depth)     return  0;	/* a, b are eq up to cmp depth ==> = */
   else if (a->len > b->len) return  1; /* a and/or b < cmp_depth ==> longer */
   else if (a->len < b->len) return -1; /*                         is bigger */
   else			     return  0;
}

/************************************************************************
 *									*
 *				pcmp_exact				*
 *				----------				*
 *									*
 ************************************************************************/

static int pcmp_exact (pa, pb)
COND_BR **pa;
COND_BR **pb;
{
   int	   i;
   int	   len;
   COND_BR *a = *pa;
   COND_BR *b = *pb;

   if (a->len != b->len) return FALSE;
   else len = a->len;

   for (i = 0; i <= len; i++)
      if (a->br_hist[i] != b->br_hist[i]) return FALSE;

   return TRUE;
}

/************************************************************************
 *									*
 *				pcmp_prefix_exact			*
 *				-----------------			*
 *									*
 ************************************************************************/

static int pcmp_prefix_exact (pa, pb)
COND_BR **pa;
COND_BR **pb;
{
   int	   i;
   COND_BR *a = *pa;
   COND_BR *b = *pb;

   if (a->len < b->len) {
        if (a->len < cmp_depth) return FALSE;
   }
   else if (b->len < cmp_depth) return FALSE;

   for (i = 0; i <= cmp_depth; i++)
      if (a->br_hist[i] != b->br_hist[i]) return FALSE;

   return TRUE;
}
