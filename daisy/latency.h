/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_latency
#define H_latency

/* There is a difference between "Execution Time" and "Latency":

     o EXECUTION TIME is the amount of time until an independent
       operation can re-use the same function unit.  It gives
       a measure of how fully pipelined is the function unit,
       with 1 being fully pipelined and <Latency> being no
       pipelining.

     o LATENCY is the amount of time until a dependent operation
       may use the result of this one.

   Some operations such as "add." produce more than one value.  The
   time until the different values are available may differ.  Hence
   a different value is allowed for each output result -- see dis.h
   and dis_tbl.c.
*/

#define E1			1		/* BRANCH Exec Time */
#define L1			1		/* BRANCH Latency   */

#define E2			1		/* SVC    Exec Time */
#define L2			1		/* SVC    Latency   */

#define E3			1		/* RFS    Exec Time */
#define L3			1		/* RFS    Latency   */

#define E4			1		/* RFI    Exec Time */
#define L4			1		/* RFI    Latency   */

#define E5			1		/* MCRF   Exec Time */
#define L5			1		/* MCRF   Latency   */

#define E6			1		/* CR_OP  Exec Time */
#define L6			1		/* CR_OP  Latency   */

#define E7			1		/* Int LD  Exec Time */
#define L7			2		/* Int LD  Latency */
#define C7			3		/* Int LD  Recd Latency */
#define U7			1		/* Int LD  Update Latency */

#define E8			2		/* LM      Exec Time */
#define L8			2		/* LM      Latency   */

#define E9			1		/* Int ST  Exec Time */
#define L9			0		/* Int ST  Latency   */
#define C9			2		/* Int ST  Recd Latency */
#define U9			1		/* Int ST  Update Latency */

#define E10			2		/* STM     Exec Time */
#define L10			2		/* STM     Latency   */

#define E11			1		/* Int LSX Exec Time */
#define L11			2		/* Int LSX Latency   */
#define C11			4		/* Int LSX Recd Latency */
#define X11			1		/* Int LSX XER   Latency */

#define E12			1		/* Int LSI Exec Time */
#define L12			2		/* Int LSI Latency   */
#define C12			4		/* Int LSI Recd Latency */

#define E13			1		/* Int STSX Exec Time */
#define L13			0		/* Int STSX Latency   */
#define C13			2		/* Int STSX Recd Latency */
#define M13			1		/* Int STSX MQ    Latency */

#define E14			1		/* Int STSI Exec Time */
#define L14			0		/* Int STSI Latency   */
#define C14			2		/* Int STSI Recd Latency */
#define M14			1		/* Int STSI MQ    Latency */

#define E15			1		/* Add/Sub Exec Time */
#define L15			1		/* Add/Sub Latency   */
#define C15			1		/* Add/Sub Recd Latency */
#define O15			1		/* Add/Sub O,SO,CA Latency */

#define E16			1		/* Add/Sub-E Exec Time */
#define L16			1		/* Add/Sub-E Latency   */
#define C16			1		/* Add/Sub-E Recd Latency */
#define O16			1		/* Add/Sub-E O,SO,CA Latency */

#define E17			2		/* MUL-U Exec Time */
#define L17			4		/* MUL-U Latency   */
#define C17			5		/* MUL-U Recd Latency */
#define O17			5		/* MUL-U O,SO,CA Latency */
#define M17			5		/* MUL-U MQ      Latency */

#define E18			3		/* MULI  Exec Time */
#define L18			3		/* MULI  Latency   */
#define M18			5		/* MULI  MQ Latency */

#define E19			4		/* MUL-S Exec Time */
#define L19			4		/* MUL-S Latency   */
#define C19			5		/* MUL-S Recd Latency */
#define O19			5		/* MUL-S O,SO,CA Latency */
#define M19			5		/* MUL-S MQ      Latency */

#define E20			19		/* DIV Exec Time */
#define L20			20		/* DIV Latency   */
#define C20			21		/* DIV Recd Latency */
#define O20			21		/* DIV O,SO,CA Latency */
#define M20			21		/* DIV MQ      Latency */

#define E21			1		/* CMP Exec Time */
#define L21			1		/* CMP Latency */
#define C21			1		/* CMP Recd Latency */

#define E22			1		/* TRAP Exec Time */
#define L22			1		/* TRAP Latency   */
#define C22			1		/* TRAP Recd Latency */

#define E23			1		/* Logical Exec Time */
#define L23			1		/* Logical Latency   */
#define C23			2		/* Logical Recd Latency */
#define O23			1		/* Logical O,SO,CA Latency */

#define E24			1		/* CNTLZ Exec Time */
#define L24			1		/* CNTLZ Latency   */
#define C24			2		/* CNTLZ Recd Latency */

#define E25			1		/* RLIMI Type Exec Time */
#define L25			1		/* RLIMI Type Latency   */
#define C25			2		/* RLIMI Type Recd Latency */

#define E26			1		/* Shift Exec Time */
#define L26			1		/* Shift Latency   */
#define C26			2		/* Shift Recd Latency */
#define O26			2		/* Shift O,SO,CA Latency */
#define M26			2		/* Shift MQ      Latency */

#define E27			1		/* MTSPR Exec Time */
#define L27			1		/* MTSPR Type Latency   */

/* Have to take worst case:  MFXER */
#define E28			3		/* MFSPR Exec Time */
#define L28			3		/* MFSPR Type Latency   */

#define E29			1		/* MTCRF Exec Time */
#define L29			1		/* MTCRF Type Latency   */
#define C29			2		/* MTCRF Recd Latency */

#define E30			1		/* MFCR Exec Time */
#define L30			3		/* MFCR Type Latency   */
#define C30			4		/* MFCR Recd Latency */

#define E31			1		/* MCRXR Exec Time */
#define L31			3		/* MCRXR Type Latency   */
#define C31			4		/* MCRXR Recd Latency */
#define X31			3		/* MCRXR XER   Latency */

#define E32			1		/* MTMSR Exec Time */
#define L32			3		/* MTMSR Type Latency   */
#define C32			3		/* MTMSR Recd Latency */

#define E33			1		/* MFMSR Exec Time */
#define L33			3		/* MFMSR Type Latency   */
#define C33			3		/* MFMSR Recd Latency */

#define E34			1		/* Flt LD  Exec Time */
#define L34			3		/* Flt LD  Latency  */
#define C34			4		/* Flt LD  Recd Latency */
#define U34			1		/* Flt LD  Update Latency */

#define E35			1		/* Flt ST  Exec Time */
#define L35			0		/* Flt ST  Latency   */
#define C35			2		/* Flt ST  Recd Latency */
#define U35			1		/* Flt ST  Update Latency */

#define E36			1		/* FMR Type Exec Time */
#define L36			1		/* FMR Type Latency   */
#define C36			4		/* FMR Type Recd Latency */
#define F36			1		/* FMR FP Status Reg */

#define E37			1		/* FA, FS Exec Time */
#define L37			3		/* FA, FS Latency   */
#define C37			4		/* FA, FS Recd Latency */
#define F37			4		/* FA, FS FP Status Reg */

#define E38			1		/* FM, FMA Type Exec Time */
#define L38			3		/* FM, FMA Type Latency   */
#define C38			4		/* FM, FMA Type Recd Latency */
#define F38			4		/* FM, FMA Type Status Reg */

#define E39			32		/* FD Exec Time */
#define L39			32		/* FD Latency   */
#define C39			33		/* FD Recd Latency */
#define F39			33		/* FD Status Reg */

#define E40			1		/* FRSP Exec Time */
#define L40			3		/* FRSP Latency   */
#define C40			4		/* FRSP Recd Latency */
#define F40			4		/* FRSP Status Reg */

#define E41			1		/* FCMP Exec Time */
#define L41			3		/* FCMP Latency   */
#define C41			4		/* FCMP Recd Latency */
#define F41			4		/* FCMP Status Reg */

#define E42			1		/* MFFS Exec Time */
#define L42			3		/* MFFS Latency   */
#define C42			4		/* MFFS Recd Latency */

#define E43			3		/* MCRFS Exec Time */
#define L43			3		/* MCRFS Latency   */
#define C43			4		/* MCRFS Recd Latency */

#define E44			3		/* MTFSF Exec Time */
#define L44			3		/* MTFSF Latency   */
#define C44			4		/* MTFSF Recd Latency */

#define E45			1		/* RAC Exec Time */
#define L45			1		/* RAC Latency   */
#define C45			2		/* RAC Recd Latency */

#define E46			1		/* TLBI Exec Time */
#define L46			2		/* TLBI Latency   */
#define C46			3		/* TLBI Recd Latency */

#define E47			1		/* MTSR Exec Time */
#define L47			1		/* MTSR Latency   */
#define C47			2		/* MTSR Recd Latency */

#define E48			1		/* MFSR Exec Time */
#define L48			3		/* MFSR Latency   */
#define C48			3		/* MFSR Recd Latency */

#define E49			1		/* Cache Line Type Exec Time */
#define L49			2		/* Cache Line Type Latency   */
#define C49			3		/* Cache Line Type Recd Lat  */

#define E50			2		/* Cache Sync Exec Time     */
#define L50			2		/* Cache Sync Latency       */
#define C50			3		/* Cache Sync Recd Latency  */

#define E51			2		/* Cache Line Size Exec Time */
#define L51			2		/* Cache Line Size Latency   */
#define C51			3		/* Cache Line Size Recd Lat  */

#define E52			1		/* FSQRT Exec Time */
#define L52			3		/* FSQRT Latency   */
#define C52			4		/* FSQRT Recd Latency */
#define F52			4		/* FSQRT Status Reg */

#define E53			1		/* FSEL Exec Time */
#define L53			3		/* FSEL Latency   */
#define C53			4		/* FSEL Recd Latency */
#define F53			4		/* FSEL Status Reg */

#endif
