/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_bitvec_macros
#define H_bitvec_macros

#define is_bit_set(v,posn)	(v[posn>>5]&(1<<(posn&0x1F)))

#endif
