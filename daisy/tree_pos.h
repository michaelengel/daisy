/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_tree_pos
#define H_tree_pos

typedef struct {
   unsigned tree_pos;	        /* Root=1, LKid=2*par,RKid=2*par+1*/
   unsigned tree_pos_log;
   unsigned *tree_pos_ext;
} TREE_POS;

#endif
