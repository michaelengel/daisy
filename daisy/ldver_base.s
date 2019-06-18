.set r0,0; .set SP,1; .set RTOC,2; .set r3,3; .set r4,4
.set r5,5; .set r6,6; .set r7,7; .set r8,8; .set r9,9
.set r10,10; .set r11,11; .set r12,12; .set r13,13; .set r14,14
.set r15,15; .set r16,16; .set r17,17; .set r18,18; .set r19,19
.set r20,20; .set r21,21; .set r22,22; .set r23,23; .set r24,24
.set r25,25; .set r26,26; .set r27,27; .set r28,28; .set r29,29
.set r30,30; .set r31,31
.set fp0,0;  .set fp1,1;  .set fp2,2;   .set fp3,3; 
.set fp24,24; .set fp25,25; .set fp26,26; .set fp27,27;
.set fp28,28; .set fp29,29; .set fp30,30; .set fp31,31;
.set MQ,0; .set XER,1; .set LR,8; .set CTR,9; 
.set BO_ALWAYS,20; .set BO_ALWAYS_1,21; .set BO_ALWAYS_2,22
.set CR0_LT,0; .set CR0_GT,1; .set CR0_EQ,2; .set CR0_SO,3

	.rename Hloadver_fail_raw{TC},"loadver_fail_raw"

	.globl	.loadver_fail_raw
	.globl	loadver_fail_raw{DS}
	.extern	.loadver_fail{PR}

.loadver_fail_raw:
	mfcr	r31			# Save condition code register
	st	r31,CC_SAVE(r13)

.save_xlate_regs:
					# As indicated in rios_map_tbl.c
					# r0-r12 use the real RIOS regs
					# Hence we must save those regs here.
					# 
	st	r0,R0_OFFSET(r13)	# Save r0
	st	SP,R1_OFFSET(r13)	# Save r1/SP
	st	r3,R3_OFFSET(r13)	# Save r3
	st	r4,R4_OFFSET(r13)	# Save r4
	st	r5,R5_OFFSET(r13)	# Save r5
	st	r6,R6_OFFSET(r13)	# Save r6
	st	r7,R7_OFFSET(r13)	# Save r7
	st	r8,R8_OFFSET(r13)	# Save r8
	st	r9,R9_OFFSET(r13)	# Save r9
	st	r10,R10_OFFSET(r13)	# Save r10
	st	r11,R11_OFFSET(r13)	# Save r11
	st	r12,R12_OFFSET(r13)	# Save r12

					# As indicated in rios_map_tbl.c
					# fp0-fp23 use the real RIOS regs
					# We only use fp0-2 (thus far).
					# 
	stfd	fp0,FP0_OFFSET(r13)	# Save fp0
	stfd	fp1,FP1_OFFSET(r13)	# Save fp1
	stfd	fp2,FP2_OFFSET(r13)	# Save fp2
	stfd	fp3,FP3_OFFSET(r13)	# Save fp3

					# r27 must match simul.c
	cal	r3,0(r27)		# Setup parm for load_ver
					# in correct registers

	mfspr	r31,XER			# Save XER
	st	r31,XER_SAVE(r13)

	mfspr	r31,CTR			# Save CTR
	st	r31,CTR_SAVE(r13)

	mfspr	r31,MQ			# Save MQ
	st	r31,MQ_SAVE(r13)

					# Save xlated pgm's TOC
	st	RTOC,XLATED_PGM_TOC(r13)

					# Set load_ver's TOC
	l	RTOC,XLATE_ENTRY_TOC(r13)

					# Set load_ver's Stack
	l	SP,XLATE_ENTRY_STACK(r13)

	bl	.loadver_fail{PR}	# Returns in r3 address to branch to
	cror	15,15,15

					# Restore registers
.restore_xlate_regs:
	mtspr	LR,r3			# 1st set LR to xlated code going to

	l	r31,CC_SAVE(r13)	# Restore condition code register
	mtcrf	255,r31		

	l	r31,XER_SAVE(r13)	# Restore XER
	mtspr	XER,r31		

	l	r31,CTR_SAVE(r13)	# Restore CTR
	mtspr	CTR,r31			
					
	l	r31,MQ_SAVE(r13)	# Restore MQ
	mtspr	MQ,r31			
					
	l	RTOC,XLATED_PGM_TOC(r13)# Restore xlated pgm's TOC

	l	r0,R0_OFFSET(r13)	# Restore r0
	l	SP,R1_OFFSET(r13)	# Restore r1/SP
	l	r3,R3_OFFSET(r13)	# Restore r3
	l	r4,R4_OFFSET(r13)	# Restore r4
	l	r5,R5_OFFSET(r13)	# Restore r5
	l	r6,R6_OFFSET(r13)	# Restore r6
	l	r7,R7_OFFSET(r13)	# Restore r7
	l	r8,R8_OFFSET(r13)	# Restore r8
	l	r9,R9_OFFSET(r13)	# Restore r9
	l	r10,R10_OFFSET(r13)	# Restore r10
	l	r11,R11_OFFSET(r13)	# Restore r11
	l	r12,R12_OFFSET(r13)	# Restore r12

	lfd	fp0,FP0_OFFSET(r13)	# Restore fp0
	lfd	fp1,FP1_OFFSET(r13)	# Restore fp1
	lfd	fp2,FP2_OFFSET(r13)	# Restore fp2
	lfd	fp3,FP3_OFFSET(r13)	# Restore fp3

	bcr	BO_ALWAYS,CR0_LT


	.toc
Tloadver_fail_raw:
	.tc	Hloadver_fail_raw{TC},loadver_fail_raw{DS}


	.csect	loadver_fail_raw{DS}           
	.long	.loadver_fail_raw
	.long	TOC{TC0}
	.long	0x00000000

