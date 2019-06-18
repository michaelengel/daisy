/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_vliwcnts
#define H_vliwcnts

/* These 3 functions must all take the same arguments, as each
   can be pointed to by the function pointer, "process_perf_fcn"
*/
void cs_perf_ins    (unsigned, unsigned, unsigned, unsigned, unsigned, unsigned *, unsigned char *, unsigned, unsigned, unsigned,
		     unsigned, unsigned, unsigned, unsigned, int,
		     int, unsigned char *, unsigned *, unsigned, unsigned);
void count_perf_ins (unsigned, unsigned, unsigned, unsigned, unsigned, unsigned *, unsigned char *, unsigned, unsigned, unsigned,
		     unsigned, unsigned, unsigned, unsigned, int, 
		     int, unsigned char *, unsigned *, unsigned, unsigned);
void dump_perf_ins  (unsigned, unsigned, unsigned, unsigned, unsigned, unsigned *, unsigned char *, unsigned, unsigned);

/* These 3 functions also must all take the same arguments, as each
   can be pointed to by the function pointer, "process_spec_fcn"
*/
void cs_spec_ins     (char *, unsigned, int, int, unsigned char *, unsigned *, unsigned, unsigned);
void count_spec_ins  (char *, unsigned, int, int, unsigned char *, unsigned *, unsigned, unsigned);
void dump_spec_ins   (char *, unsigned, int, int, unsigned char *, unsigned *, unsigned, unsigned);

/* These 2 functions must all take the same arguments, as each
   can be pointed to by the function pointer, "process_lib_fcn"
*/

void cs_lib_calls     (char *, unsigned, unsigned);
void ignore_lib_calls (char *, unsigned, unsigned);

/* These 2 functions are used with pdf (profile-directed-feedback) */
void init_vliwinp_cnts (void);
void finish_vliwinp_cnts (char *);

#endif
