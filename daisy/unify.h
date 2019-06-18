/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_unify
#define H_unify

typedef struct _unify {
   TIP		  *tip;
   OPCODE2	  *op;
   unsigned short set_time;
   unsigned short dest;
   struct _unify  *next;
} UNIFY;

void	   unify_init_page   (void);
void	   unify_add_op      (TIP     *, OPCODE2 *);
UNIFY	  *unify_find        (TIP     *, OPCODE2 *, int);

#endif
