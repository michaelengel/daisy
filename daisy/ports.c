/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "dis.h"

/************************************************************************
 *									*
 *				cnt_explicit_ports			*
 *				------------------			*
 *									*
 ************************************************************************/

int cnt_explicit_ports (expl_list, list_size, pgports, pfports, pcports)
unsigned char *expl_list;
int	      list_size;
int	      *pgports;				/* Outputs */
int	      *pfports;
int	      *pcports;
{
   int i;
   int gpr_ports = 0;
   int fpr_ports = 0;
   int ccr_ports = 0;

   for (i = 0; i < list_size; i++) {
      switch (expl_list[i] | OPERAND_BIT) {
	 case RT:
	 case RS:
	 case RA:
	 case RB:
	 case RZ:
	    gpr_ports++;
	    break;

	 case FRT:
	 case FRS:
	 case FRA:
	 case FRB:
	 case FRC:
	    fpr_ports++;
	    break;

	 case BT:
	 case BA:
	 case BB:
	 case BI:
	 case BF:
	 case BFA:
	 case BF2:
	    ccr_ports++;
	    break;

	 default:
	    break;
      }
   }

   *pgports = gpr_ports;
   *pfports = fpr_ports;
   *pcports = ccr_ports;

   return gpr_ports + fpr_ports + ccr_ports;
}
