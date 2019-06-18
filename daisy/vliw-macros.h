/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_vliw_macros
#define H_vliw_macros

#ifndef USE_REG_CLASSIFY_FUNCS

#define is_gpr(reg)      ((reg>=R0)&&(reg<R0+num_gprs))
#define is_fpr(reg)      ((reg>=FP0)&&(reg<FP0+num_fprs))
#define is_ccbit(reg)    ((reg>=CR0)&&(reg<CR0+num_ccbits))

#endif

#endif
