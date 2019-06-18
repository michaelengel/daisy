/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_rename
#define H_rename

#define NUM_MA_HASHVALS			64

/* One of these for each renameable register on each TIP.
   (Make vliw_reg a signed value because it is set to -1 as a flag.)

   The map info must be kept with the final tip on each path.  This
   is because different paths may have different mappings.  For example
      VLIW1:    r7 = ...
                r6 = ...
                if (...) GOTO PATH1
                else     GOTO PATH2

       PATH1:   r3 = r7
                GOTO ...

       PATH2:   r3 = r6
                GOTO ...

   The mapping of r3 in VLIW1 is r7 on PATH1 and r6 on PATH2.
*/
typedef struct _reg_rename {
   short	      time;		/* Keep chain of rename info */
   short	      vliw_reg;
   struct _reg_rename *prev;
} REG_RENAME;

typedef struct _mem_acc {
   REG_RENAME	   *src_defn;		/* Of PPC Store Src GPR/FPR	  */
   REG_RENAME	   *orig_defn;		/* Of PPC Address   GPR		  */
   union {
      REG_RENAME   *defn2;		/* For STX types:  stx rx,ry,rz   */
      int	   offset;		/* For ST  types:  st  rx, 4(rz)  */
   }		   _2;
   short	   src_reg;
   short	   orig_reg;
   short	   orig_reg2;
   short	   time;		/* Of STORE			  */
   short	   store_num;		/* 1,2,3,... on this path	  */
   char		   indexed;		/* Boolean:  TRUE if STX type     */
   char		   flt;			/* Boolean:  TRUE if Flt Pt Store */
   char		   size;
   struct _mem_acc *next;
} MEMACC;

#endif
