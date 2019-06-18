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

	.rename	H.10.NO_SYMBOL{PR},""
	.rename	Hsetup_r13{TC},"setup_r13"
	.rename Hxlate_entry_raw{TC},"xlate_entry_raw"
	.rename Htrace_br_raw{TC},"trace_br_raw"
	.rename Hfind_data_raw{TC},"find_data_raw"
	.rename Hfind_instr_raw{TC},"find_instr_raw"
	.rename Hfinish_daisy_compile_raw{TC},"finish_daisy_compile_raw"
	.rename Hhandle_illegal_raw{TC},"handle_illegal_raw"
	.rename Hhit_entry_pt_lim1_raw{TC},"hit_entry_pt_lim1_raw"
	.rename Hhit_entry_pt_lim2_raw{TC},"hit_entry_pt_lim2_raw"
	.rename Hflush_cache_line{TC},"flush_cache_line"
	.rename Hsync_caches{TC},"sync_caches"
	.rename Hget_dcache_line_size{TC},"get_dcache_line_size"
	.rename Hget_icache_line_size{TC},"get_icache_line_size"

	.lglobl	H.10.NO_SYMBOL{PR}
	.lglobl	H.11.NO_SYMBOL{PR}
	.lglobl	H.12.NO_SYMBOL{PR}
	.lglobl	H.13.NO_SYMBOL{PR}
	.lglobl	H.14.NO_SYMBOL{PR}
	.globl	.main
	.globl	.setup_r13
	.globl	.xlate_entry_raw
	.globl	.exec_unxlated_code
	.globl	.trace_br_raw
	.globl	.find_data_raw
	.globl	.find_instr_raw
	.globl	.finish_daisy_compile_raw
	.globl	.handle_illegal_raw
	.globl	.hit_entry_pt_lim1_raw
	.globl	.hit_entry_pt_lim2_raw
	.globl	.sync_caches
	.globl	.flush_cache_line
	.globl	.icache_sync_line
	.globl	.sync_caches_pwr
	.globl	.get_dcache_line_size
	.globl	.get_icache_line_size
	.globl	.gen_mask_old
	.globl	setup_r13{DS}
	.globl	xlate_entry_raw{DS}
	.globl	exec_unxlated_code{DS}
	.globl	trace_br_raw{DS}
	.globl	find_data_raw{DS}
	.globl	find_instr_raw{DS}
	.globl	finish_daisy_compile_raw{DS}
	.globl	handle_illegal_raw{DS}
	.globl	hit_entry_pt_lim1_raw{DS}
	.globl	hit_entry_pt_lim2_raw{DS}
	.globl	flush_cache_line{DS}
	.globl	sync_caches{DS}
	.globl	get_dcache_line_size{DS}
	.globl	get_icache_line_size{DS}
	.extern	.xlate_entry{PR}
	.extern	.trace_br_xlated{PR}
	.extern	.find_data{PR}
	.extern	.find_instr{PR}
	.extern	.finish_daisy_compile{PR}
	.extern	.handle_illegal{PR}
	.extern	.hit_entry_pt_lim1{PR}
	.extern	.hit_entry_pt_lim2{PR}
	.extern	.main2{PR}

	.csect	H.10.NO_SYMBOL{PR}      

#########################################################################
# Put the stack value as a 4th parameter to the "real main", i.e "main2"#
#########################################################################
.main:
	cal	r6,0(SP)
	b	.main2{PR}

#########################################################################
# o Setup register r13 to point to simulated registers			#
# o Save TOC for later invocations of xlate_entry from xlate_entry_raw	#
#########################################################################

.setup_r13:
	.file	"r13.c"                 
	cal	r13,0(r3)
	st	RTOC,XLATE_ENTRY_TOC(r13) # Save TOC for xlate_raw_entry
	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Save and restore CC, XER, CTR, TOC when calling xlate_entry		#
#########################################################################

.xlate_entry_raw:
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
	cal	r3,0(r27)		# Setup parm for xlate_entry
					# in correct registers

	mfspr	r31,XER			# Save XER
	st	r31,XER_SAVE(r13)

	mfspr	r31,CTR			# Save CTR
	st	r31,CTR_SAVE(r13)

	mfspr	r31,MQ			# Save MQ
	st	r31,MQ_SAVE(r13)

					# Save xlated pgm's TOC
	st	RTOC,XLATED_PGM_TOC(r13)

					# Set xlate_entry's TOC
	l	RTOC,XLATE_ENTRY_TOC(r13)

					# Set xlate_entry's Stack
	l	SP,XLATE_ENTRY_STACK(r13)

	bl	.xlate_entry{PR}	# Returns in r3 address to branch to
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

########################################################################
# Instead of returning to simulation code, return to untranslated code #
########################################################################

.exec_unxlated_code:
	mtspr	LR,r4			# 1st set LR to orig code

	l	r31,XER_SAVE(r13)	# Restore XER
	mtspr	XER,r31		

	l	r31,CTR_OFFSET(r13)	# Copy CTR from VLIW
	mtspr	CTR,r31			
					
	l	r31,MQ_SAVE(r13)	# Restore MQ
	mtspr	MQ,r31			
					
	lfd	fp0,FP0_OFFSET(r13)	# Restore fp0
	lfd	fp1,FP1_OFFSET(r13)	# Restore fp1
	lfd	fp2,FP2_OFFSET(r13)	# Restore fp2
	lfd	fp24,FP24_OFFSET(r13)	# Copy    fp24 from VLIW
	lfd	fp25,FP25_OFFSET(r13)	# Copy    fp25 from VLIW
	lfd	fp26,FP26_OFFSET(r13)	# Copy    fp26 from VLIW
	lfd	fp27,FP27_OFFSET(r13)	# Copy    fp27 from VLIW
	lfd	fp28,FP28_OFFSET(r13)	# Copy    fp28 from VLIW
	lfd	fp29,FP29_OFFSET(r13)	# Copy    fp29 from VLIW
	lfd	fp30,FP30_OFFSET(r13)	# Copy    fp30 from VLIW
	lfd	fp31,FP31_OFFSET(r13)	# Copy    fp31 from VLIW

        l	r31,CRF0_OFFSET(r13)	# Copy    cr0 field from VLIW
	rlimi	r30,r31,0,0,3
        l	r31,CRF1_OFFSET(r13)	# Copy    cr1 field from VLIW
	rlimi	r30,r31,28,4,7
        l	r31,CRF2_OFFSET(r13)	# Copy    cr2 field from VLIW
	rlimi	r30,r31,24,8,11
        l	r31,CRF3_OFFSET(r13)	# Copy    cr3 field from VLIW
	rlimi	r30,r31,20,12,15
        l	r31,CRF4_OFFSET(r13)	# Copy    cr4 field from VLIW
	rlimi	r30,r31,16,16,19
        l	r31,CRF5_OFFSET(r13)	# Copy    cr5 field from VLIW
	rlimi	r30,r31,12,20,23
        l	r31,CRF6_OFFSET(r13)	# Copy    cr6 field from VLIW
	rlimi	r30,r31,8,24,27
        l	r31,CRF7_OFFSET(r13)	# Copy    cr7 field from VLIW
	rlimi	r30,r31,4,28,31
	mtcrf	255,r30			# Copy r30 to 32-bit RIOS cond reg
	

	l	RTOC,XLATED_PGM_TOC(r13)# Restore xlated pgm's TOC

	l	r0,R0_OFFSET(r13)	# Restore r0
	l	SP,R1_OFFSET(r13)	# Restore r1/SP
					# Restore r3 done in stub
	l	r4,R4_OFFSET(r13)	# Restore r4
	l	r5,R5_OFFSET(r13)	# Restore r5
	l	r6,R6_OFFSET(r13)	# Restore r6
	l	r7,R7_OFFSET(r13)	# Restore r7
	l	r8,R8_OFFSET(r13)	# Restore r8
	l	r9,R9_OFFSET(r13)	# Restore r9
	l	r10,R10_OFFSET(r13)	# Restore r10
	l	r11,R11_OFFSET(r13)	# Restore r11
	l	r12,R12_OFFSET(r13)	# Restore r12

	l	r31,R31_OFFSET(r13)	# Copy    r31 from VLIW
	l	r30,R30_OFFSET(r13)	# Copy    r30 from VLIW
	l	r29,R29_OFFSET(r13)	# Copy    r29 from VLIW
	l	r28,R28_OFFSET(r13)	# Copy    r28 from VLIW
	l	r27,R27_OFFSET(r13)	# Copy    r27 from VLIW
	l	r26,R26_OFFSET(r13)	# Copy    r26 from VLIW
	l	r25,R25_OFFSET(r13)	# Copy    r25 from VLIW
	l	r24,R24_OFFSET(r13)	# Copy    r24 from VLIW
	l	r23,R23_OFFSET(r13)	# Copy    r23 from VLIW
	l	r22,R22_OFFSET(r13)	# Copy    r22 from VLIW
	l	r21,R21_OFFSET(r13)	# Copy    r21 from VLIW
	l	r20,R20_OFFSET(r13)	# Copy    r20 from VLIW
	l	r19,R19_OFFSET(r13)	# Copy    r19 from VLIW
	l	r18,R18_OFFSET(r13)	# Copy    r18 from VLIW
	l	r17,R17_OFFSET(r13)	# Copy    r17 from VLIW
	l	r16,R16_OFFSET(r13)	# Copy    r16 from VLIW
	l	r15,R15_OFFSET(r13)	# Copy    r15 from VLIW
	l	r14,R14_OFFSET(r13)	# Copy    r14 from VLIW
					# Copy    r13 from VLIW done in stub

	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Setup to branch to trace_br_xlated					#
#########################################################################

.trace_br_raw:
	mfcr	r31			# Save condition code register
	st	r31,CC_SAVE(r13)

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

					# r30 must match simul.c
	cal	r4,0(r30)		# Setup parm for trace_br
					# in correct register
	mfspr	r3,LR			# Pass our rtn address

	mfspr	r31,XER			# Save XER
	st	r31,XER_SAVE(r13)

	mfspr	r31,CTR			# Save CTR
	st	r31,CTR_SAVE(r13)

	mfspr	r31,MQ			# Save MQ
	st	r31,MQ_SAVE(r13)

					# Save xlated pgm's TOC
	st	RTOC,XLATED_PGM_TOC(r13)

					# Set xlate_entry's TOC
	l	RTOC,XLATE_ENTRY_TOC(r13)

					# Set xlate_entry's Stack
	l	SP,XLATE_ENTRY_STACK(r13)

	bl	.trace_br_xlated{PR}	#
	cror	15,15,15

	mtspr	LR,r3			# Retrieve our rtn address

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

	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Setup to branch to find_data (in cache for LOAD or STORE reference    #
#########################################################################

.find_data_raw:
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

					# r30 must match rios_map.c
	cal	r4,0(r31)		# Put address and
	cal	r5,0(r30)		#     access-type for "find_data"
					# in correct register
	mfspr	r3,LR			# Pass our rtn address

	mfcr	r31			# Save condition code register
	st	r31,CC_SAVE(r13)

	mfspr	r31,XER			# Save XER
	st	r31,XER_SAVE(r13)

	mfspr	r31,CTR			# Save CTR
	st	r31,CTR_SAVE(r13)

	mfspr	r31,MQ			# Save MQ
	st	r31,MQ_SAVE(r13)

					# Save xlated pgm's TOC
	st	RTOC,XLATED_PGM_TOC(r13)

					# Set xlate_entry's TOC
	l	RTOC,XLATE_ENTRY_TOC(r13)

					# Set xlate_entry's Stack
	l	SP,XLATE_ENTRY_STACK(r13)

	bl	.find_data{PR}		#
	cror	15,15,15

	mtspr	LR,r3			# Retrieve our rtn address

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

	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Setup to branch to find_instr (in cache for Instruction Fetch)
#########################################################################

.find_instr_raw:
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

					# r30 must match rios_map.c
	cal	r4,0(r31)		# Put address and for "find_instr"
					# in correct register
	mfspr	r3,LR			# Pass our rtn address

	mfcr	r31			# Save condition code register
	st	r31,CC_SAVE(r13)

	mfspr	r31,XER			# Save XER
	st	r31,XER_SAVE(r13)

	mfspr	r31,CTR			# Save CTR
	st	r31,CTR_SAVE(r13)

	mfspr	r31,MQ			# Save MQ
	st	r31,MQ_SAVE(r13)

					# Save xlated pgm's TOC
	st	RTOC,XLATED_PGM_TOC(r13)

					# Set xlate_entry's TOC
	l	RTOC,XLATE_ENTRY_TOC(r13)

					# Set xlate_entry's Stack
	l	SP,XLATE_ENTRY_STACK(r13)

	bl	.find_instr{PR}		#
	cror	15,15,15

	mtspr	LR,r3			# Retrieve our rtn address

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

	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Setup to branch to finish_daisy_compile				#
#########################################################################

.finish_daisy_compile_raw:
	l	r31,LR2_OFFSET(r13)		# Setup return address
	mtspr	LR,r31
	l	RTOC,XLATE_ENTRY_TOC(r13)	# Make sure TOC ok
	b	.finish_daisy_compile{PR}

#########################################################################
# Set up TOC for handle_illegal routine in C.				#
#########################################################################

.handle_illegal_raw:
					
	l	RTOC,XLATE_ENTRY_TOC(r13) # Set handle_illegal's TOC

	bl	.handle_illegal{PR}	# Do not expect handle_illegal
	cror	15,15,15		# to return

#########################################################################
# Set up TOC, r3, and r4 for hit_entry_pt_lim1 routine in C.		#
#########################################################################

.hit_entry_pt_lim1_raw:
					# Copy parms to where they're expected
					# -- Must match xlate_offpage_indir ()
					#    routine in simul.c 
	or	r3,r31,r31		# COUNTS
	or	r4,r27,r27		# ADDRESS OF NEXT PAGE

	l	RTOC,XLATE_ENTRY_TOC(r13) # Set hit_entry_pt_lim's TOC

	bl	.hit_entry_pt_lim1{PR}	# Do not expect hit_entry_pt_lim1
	cror	15,15,15		# to return

#########################################################################
# Set up STK, TOC, r3, and r4 for hit_entry_pt_lim2 routine in C.	#
#########################################################################

.hit_entry_pt_lim2_raw:
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

					# Copy parms to where they're expected
					# -- Must match xlate_offpage_indir ()
					#    routine in simul.c 
	or	r3,r31,r31		# COUNTS
	or	r4,r27,r27		# ADDRESS OF NEXT PAGE

	mfspr	r31,XER			# Save XER
	st	r31,XER_SAVE(r13)

	mfspr	r31,CTR			# Save CTR
	st	r31,CTR_SAVE(r13)

	mfspr	r31,MQ			# Save MQ
	st	r31,MQ_SAVE(r13)

	st	RTOC,XLATED_PGM_TOC(r13)  # Save xlated pgm's TOC
	l	RTOC,XLATE_ENTRY_TOC(r13) # Set hit_entry_pt_lim's TOC
	l	SP,XLATE_ENTRY_STACK(r13) # Set hit_entry_pt_lim's STACK

	bl	.hit_entry_pt_lim2{PR}	# Do not expect hit_entry_pt_lim
	cror	15,15,15		# to return

#########################################################################
# Flush specified cache line to main memory.				#
#########################################################################

.icache_sync_line:
        dcbst   0,r3                    # Flush out of data cache
        icbi    0,r3                    # Make sure not in ins Cache
        blr

#########################################################################
# Make sure flushed lines are available to ICache (as outlined on page  #
# 142 of the "Power Processor Architecture Manual", Version 1.52, Feb'90#
#########################################################################

.sync_caches:
.sync_caches_ppc:
	sync
	isync
	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Flush specified cache line to main memory.				#
#########################################################################

.flush_cache_line:
#	clf	r0,r3
	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Make sure flushed lines are available to ICache (as outlined on page  #
# 142 of the "Power Processor Architecture Manual", Version 1.52, Feb'90#
#########################################################################

.sync_caches_pwr:
	dcs
	ics
	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Get L1 data cache line size.						#
#########################################################################

.get_dcache_line_size:
#	clcs	r3,r13
	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Get L1 instruction cache line size.					#
#########################################################################

.get_icache_line_size:
#	clcs	r3,r12
	bcr	BO_ALWAYS,CR0_LT

#########################################################################
# Get L1 instruction cache line size.					#
#########################################################################

.gen_mask_old:
#	maskg	r3,r3,r4
#	bcr	BO_ALWAYS,CR0_LT

	.toc
Tsetup_r13:
	.tc	Hsetup_r13{TC},setup_r13{DS}

Txlate_entry_raw:
	.tc	Hxlate_entry_raw{TC},xlate_entry_raw{DS}

Texec_unxlated_code:
	.tc	Hexec_unxlated_code{TC},exec_unxlated_code{DS}

Ttrace_br_raw:
	.tc	Htrace_br_raw{TC},trace_br_raw{DS}

Tfind_data_raw:
	.tc	Hfind_data_raw{TC},find_data_raw{DS}

Tfind_instr_raw:
	.tc	Hfind_instr_raw{TC},find_instr_raw{DS}

Tfinish_daisy_compile_raw:
	.tc	Hfinish_daisy_compile_raw{TC},finish_daisy_compile_raw{DS}

Thandle_illegal_raw:
	.tc	Hhandle_illegal_raw{TC},handle_illegal_raw{DS}

Thit_entry_pt_lim1_raw:
	.tc	Hhit_entry_pt_lim1_raw{TC},hit_entry_pt_lim1_raw{DS}

Thit_entry_pt_lim2_raw:
	.tc	Hhit_entry_pt_lim2_raw{TC},hit_entry_pt_lim2_raw{DS}

Tflush_cache_line:
	.tc	Hflush_cache_line{TC},flush_cache_line{DS}

Tsync_caches:
	.tc	Hsync_caches{TC},sync_caches{DS}

Tget_dcache_line_size:
	.tc	Hget_dcache_line_size{TC},get_dcache_line_size{DS}

Tget_icache_line_size:
	.tc	Hget_icache_line_size{TC},get_icache_line_size{DS}

	.csect	setup_r13{DS}           
	.long	.setup_r13
	.long	TOC{TC0}
	.long	0x00000000

	.csect	xlate_entry_raw{DS}           
	.long	.xlate_entry_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	exec_unxlated_code{DS}
	.long	.exec_unxlated_code
	.long	TOC{TC0}
	.long	0x00000000

	.csect	trace_br_raw{DS}           
	.long	.trace_br_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	find_data_raw{DS}           
	.long	.find_data_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	find_instr_raw{DS}           
	.long	.find_instr_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	finish_daisy_compile_raw{DS}           
	.long	.finish_daisy_compile_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	handle_illegal_raw{DS}           
	.long	.handle_illegal_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	hit_entry_pt_lim1_raw{DS}           
	.long	.hit_entry_pt_lim1_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	hit_entry_pt_lim2_raw{DS}           
	.long	.hit_entry_pt_lim2_raw
	.long	TOC{TC0}
	.long	0x00000000

	.csect	flush_cache_line{DS}           
	.long	.flush_cache_line
	.long	TOC{TC0}
	.long	0x00000000

	.csect	sync_caches{DS}           
	.long	.sync_caches
	.long	TOC{TC0}
	.long	0x00000000

	.csect	get_dcache_line_size{DS}           
	.long	.get_dcache_line_size
	.long	TOC{TC0}
	.long	0x00000000

	.csect	get_icache_line_size{DS}           
	.long	.get_icache_line_size
	.long	TOC{TC0}
	.long	0x00000000
