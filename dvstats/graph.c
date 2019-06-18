/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include <stdio.h>

#include "instrum.h"

extern double   total_vliw_ins;

#define NUM_COLS		75
#define NUM_ROWS		31

#define X_AXIS_ROW		3
#define Y_AXIS_COL		2

#define LEGEND_ROW		(X_AXIS_ROW-1)
#define TITLE_ROW		(X_AXIS_ROW-3)

#define X_AXIS_TITLE_COL	(NUM_COLS-10)
#define X_AXIS_TITLE_ROW	(X_AXIS_ROW-2)
#define Y_AXIS_TITLE_ROW	(NUM_ROWS-1)

#define FIRST_IMAGE_COL		(Y_AXIS_COL+1)
#define FIRST_IMAGE_ROW		4
#define LAST_IMAGE_ROW		(NUM_ROWS-2)

/* Leave 1 row to put the Percent Value on Top of the Bar */
#define NUM_IMAGE_ROWS		(LAST_IMAGE_ROW-FIRST_IMAGE_ROW)

char plot_image[NUM_ROWS][NUM_COLS];

static char *num_name[] = {
    " 0", " 1", " 2", " 3", " 4", " 5", " 6", " 7", " 8", " 9",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
    "20", "21", "22", "23", "24"};

/************************************************************************
 *									*
 *				init_plot				*
 *				---------				*
 *									*
 ************************************************************************/

init_plot ()
{
   int i, j;

   memset (plot_image, ' ', NUM_COLS * NUM_ROWS);

   for (i = Y_AXIS_COL+1; i < NUM_COLS; i++)
      plot_image[X_AXIS_ROW][i] = '-';

   for (i = X_AXIS_ROW; i < NUM_ROWS; i++)
      plot_image[i][Y_AXIS_COL] = '|';

   for (i = 0; i < NUM_ROWS; i++)
      plot_image[i][NUM_COLS-1] = 0;

   strcpy (&plot_image[Y_AXIS_TITLE_ROW][Y_AXIS_COL-1], "% of Ins");
   strcpy (&plot_image[X_AXIS_TITLE_ROW][X_AXIS_TITLE_COL], "Ops / Ins");
}

/************************************************************************
 *									*
 *				plot_histo				*
 *				----------				*
 *									*
 ************************************************************************/

plot_histo (fp, histo, title)
FILE	 *fp;
double   *histo;
char	 *title;
{
   int	    i, j;
   int	    height;
   int	    max_non_zero_bin;
   int	    last_non_space_col;
   double   max_val = 0.0;
   double   scale_factor;
   char	    buff[NUM_COLS];

   put_plot_title (title);

   for (i = 0; i < OPS_PER_VLIW + 1; i++)
      if (histo[i] > max_val) max_val = histo[i];

   erase_plot_area ();
   max_non_zero_bin = put_plot_x_legend (histo);

   scale_factor = ((double) NUM_IMAGE_ROWS) / max_val;

   for (i = 0; i <= max_non_zero_bin; i++) {
      height = (int) (histo[i] * scale_factor);
      plot_column (i, height, (100.0 * histo[i]) / total_vliw_ins);
   }

   fprintf (fp, "\n");

   for (i = NUM_ROWS - 1; i >= 0; i--) {
      memcpy (buff, plot_image[i], NUM_COLS);

      /* Eliminate spaces at line end--they slow paging in the editor */
      last_non_space_col = 0;
      for (j = 1; j < NUM_COLS - 1; j++)
	 if (!isspace(buff[j])) last_non_space_col = j;
      buff[last_non_space_col+1] = 0;

      fprintf (fp, "%s\n", buff);
   }

   fprintf (fp, "\n");
}

/************************************************************************
 *									*
 *				put_plot_title				*
 *				--------------				*
 *									*
 ************************************************************************/

put_plot_title (title)
char *title;
{
   int len = strlen (title);
   int col = (NUM_COLS - Y_AXIS_COL - len) / 2 + Y_AXIS_COL;

   memset (plot_image[TITLE_ROW], ' ', NUM_COLS-1);
   strcpy (&plot_image[TITLE_ROW][col], title);
}

/************************************************************************
 *									*
 *				put_plot_x_legend			*
 *				-----------------			*
 *									*
 ************************************************************************/

int put_plot_x_legend (histo)
double *histo;
{
   int i;
   int col;
   int max_non_zero_bin = 0;

   for (i = 0; i < OPS_PER_VLIW + 1; i++)
      if (histo[i] > 0.0) max_non_zero_bin = i;

   col = Y_AXIS_COL + 2;
   for (i = 0; i <= max_non_zero_bin && col+1 < NUM_COLS ; i++) {
      plot_image[LEGEND_ROW][col]   = num_name[i][0];
      plot_image[LEGEND_ROW][col+1] = num_name[i][1];
      col += 4;
   }

   for (; i < OPS_PER_VLIW + 1 && col+1 < NUM_COLS; i++) {
      plot_image[LEGEND_ROW][col]   = ' ';
      plot_image[LEGEND_ROW][col+1] = ' ';
      col += 4;
   }

   return max_non_zero_bin;
}

/************************************************************************
 *									*
 *				plot_column				*
 *				-----------				*
 *									*
 ************************************************************************/

plot_column (bin_num, height, dpercent)
int    bin_num;
int    height;
double dpercent;
{
   int i;
   int col = Y_AXIS_COL + 2 + bin_num * 4;
   int percent = (int) (dpercent + 0.5);

   if( col + 2 >= NUM_COLS ) {
     return;
   }

   /* Might be off by 1 because of rounding */
   if (height > NUM_IMAGE_ROWS)
      height = NUM_IMAGE_ROWS;

   height += FIRST_IMAGE_ROW;
   for (i = FIRST_IMAGE_ROW; i < height; i++) {
      plot_image[i][col]   = '#';
      plot_image[i][col+1] = '#';
   }

   if (dpercent > 99.999) {
      plot_image[i][col-1] = '1';
      plot_image[i][col-0] = '0';
      plot_image[i][col+1] = '0';
      plot_image[i][col+2] = '%';
   }
   else if (percent >= 10) {
      plot_image[i][col+2] = '%';
      plot_image[i][col+1] = '0' + percent % 10;
      percent /= 10;
      plot_image[i][col]   = '0' + percent % 10;
   }
   else {
      plot_image[i][col+1] = '%';
      plot_image[i][col+0] = '0' + percent;
   }
}

/************************************************************************
 *									*
 *				erase_plot_area				*
 *				---------------				*
 *									*
 ************************************************************************/

erase_plot_area ()
{
   int i, j;

   for (i = FIRST_IMAGE_ROW; i <= LAST_IMAGE_ROW; i++)
      memset (&plot_image[i][FIRST_IMAGE_COL], ' ',
	      NUM_COLS - FIRST_IMAGE_COL);

   for (i = FIRST_IMAGE_ROW; i <= LAST_IMAGE_ROW; i++)
      plot_image[i][NUM_COLS-1] = 0;
}

