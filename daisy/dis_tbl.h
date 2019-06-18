/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef H_dis_tbl
#define H_dis_tbl

/* Integer numbering of PPC opcodes.  Note that there are 368
   entries in dis_tbl.c.  Some opcodes are used twice, for example
   with the condition bit on and off, but with the same opcode name.
   In these cases, the case with the condition code on generally
   produces undefined results and hence generally should not be
   used.

   The base set of opcodes, i.e. NUM_BASIC_PPC_OPCODES must be
   256 or less because the format of the .vliw_perf_ins_info
   allows only 1 byte per opcode.  See the function 
   "instr_save_path_ins" in "instrum.c" for details.

   We have a problem, because including PowerPC instructions, there
   are more than 256.  Hence we have moved the branch instructions
   to last in the list because they do not appear as ops on the VLIW
   tree edges.  In this way, no op saved to the .vliw_perf_ins_info
   file should have a base opcode higher than 255.
*/
#define RECORD_ON			0x200

#define OP_FSQRT			 0		/* PowerPC Only */
#define OP_FSQRT_		        (0|RECORD_ON)	/* PowerPC Only */
#define OP_FSQRTS			 1		/* PowerPC Only */
#define OP_FSQRTS_		        (1|RECORD_ON)	/* PowerPC Only */
#define OP_FSQRTE			 2		/* PowerPC Only */
#define OP_FSQRTE_		        (2|RECORD_ON)	/* PowerPC Only */

#define OP_FCTIW			 3		/* PowerPC Only */
#define OP_FCTIW_		        (3|RECORD_ON)	/* PowerPC Only */
#define OP_FCTIWZ			 4		/* PowerPC Only */
#define OP_FCTIWZ_		        (4|RECORD_ON)	/* PowerPC Only */

#define OP_ICBI				 5		/* PowerPC Only */
#define OP_LWARX			 6		/* PowerPC Only */
#define OP_STWCX_			 7		/* PowerPC Only */
#define OP_STFIWX			 8		/* PowerPC Only */

#define OP_TLBIA			 9		/* PowerPC Only */
#define OP_TLBSYNC			10		/* PowerPC Only */
#define OP_MFSRIN			11		/* PowerPC Only */

#define OP_SVC				12
#define OP_SVCL				13
#define OP_SVCA				14
#define OP_SVCLA			15
#define OP_RFSVC			16
#define OP_RFI				17
#define OP_MCRF				18
#define OP_CREQV			19
#define OP_CRAND			20
#define OP_CRXOR			21
#define OP_CROR				22
#define OP_CRANDC			23
#define OP_CRNAND			24
#define OP_CRORC			25
#define OP_CRNOR			26
#define OP_LBZ				27
#define OP_LBZX				28
#define OP_LHZ				29
#define OP_LHZX				30
#define OP_LHA				31
#define OP_LHAX				32
#define OP_L				33
#define OP_LX				34
#define OP_LHBRX			35
#define OP_LBRX				36
#define OP_LM				37
#define OP_STB				38
#define OP_STBX				39
#define OP_STH				40
#define OP_STHX				41
#define OP_ST				42
#define OP_STX				43
#define OP_STHBRX			44
#define OP_STBRX			45
#define OP_STM				46
#define OP_LBZU				47
#define OP_LBZUX			48
#define OP_LHZU				49
#define OP_LHZUX			50
#define OP_LHAU				51
#define OP_LHAUX			52
#define OP_LU				53
#define OP_LUX				54
#define OP_STBU				55
#define OP_STBUX			56
#define OP_STHU				57
#define OP_STHUX			58
#define OP_STU				59
#define OP_STUX				60
#define OP_LSX				61
#define OP_LSI				62
#define OP_LSCBX			63
#define OP_LSCBX_			(63|RECORD_ON)
#define OP_STSX				64
#define OP_STSI				65
#define OP_CAL				66
#define OP_CAU				67
#define OP_CAX				68
#define OP_CAX_				(68|RECORD_ON)
#define OP_CAXO				69
#define OP_CAXO_			(69|RECORD_ON)
#define OP_AI				70
#define OP_AI_				(70|RECORD_ON)
#define OP_SFI				71
#define OP_A				72
#define OP_A_				(72|RECORD_ON)
#define OP_AO				73
#define OP_AO_				(73|RECORD_ON)
#define OP_AE				74
#define OP_AE_				(74|RECORD_ON)
#define OP_AEO				75
#define OP_AEO_				(75|RECORD_ON)
#define OP_SF				76
#define OP_SF_				(76|RECORD_ON)
#define OP_SFO				77
#define OP_SFO_				(77|RECORD_ON)
#define OP_SFE				78
#define OP_SFE_				(78|RECORD_ON)
#define OP_SFEO				79
#define OP_SFEO_			(79|RECORD_ON)
#define OP_AME				80
#define OP_AME_				(80|RECORD_ON)
#define OP_AMEO				81
#define OP_AMEO_			(81|RECORD_ON)
#define OP_AZE				82
#define OP_AZE_				(82|RECORD_ON)
#define OP_AZEO				83
#define OP_AZEO_			(83|RECORD_ON)
#define OP_SFME				84
#define OP_SFME_			(84|RECORD_ON)
#define OP_SFMEO			85
#define OP_SFMEO_			(85|RECORD_ON)
#define OP_SFZE				86
#define OP_SFZE_			(86|RECORD_ON)
#define OP_SFZEO			87
#define OP_SFZEO_			(87|RECORD_ON)
#define OP_DOZI				88
#define OP_DOZ				89
#define OP_DOZ_				(89|RECORD_ON)
#define OP_DOZO				90
#define OP_DOZO_			(90|RECORD_ON)
#define OP_ABS				91
#define OP_ABS_				(91|RECORD_ON)
#define OP_ABSO				92
#define OP_ABSO_			(92|RECORD_ON)
#define OP_NEG				93
#define OP_NEG_				(93|RECORD_ON)
#define OP_NEGO				94
#define OP_NEGO_			(94|RECORD_ON)
#define OP_NABS				95
#define OP_NABS_			(95|RECORD_ON)
#define OP_NABSO			96
#define OP_NABSO_			(96|RECORD_ON)
#define OP_MUL				97
#define OP_MUL_				(97|RECORD_ON)
#define OP_MULO				98
#define OP_MULO_			(98|RECORD_ON)
#define OP_MULI				99
#define OP_MULS				100
#define OP_MULS_			(100|RECORD_ON)
#define OP_MULSO			101
#define OP_MULSO_			(101|RECORD_ON)
#define OP_DIV				102
#define OP_DIV_				(102|RECORD_ON)
#define OP_DIVO				103
#define OP_DIVO_			(103|RECORD_ON)
#define OP_DIVS				104
#define OP_DIVS_			(104|RECORD_ON)
#define OP_DIVSO			105
#define OP_DIVSO_			(105|RECORD_ON)
#define OP_CMPI				106
#define OP_CMPLI			107
#define OP_CMP				108
#define OP_CMPL				109
#define OP_TI				110
#define OP_T				111
#define OP_ANDIL			112
#define OP_ANDIU			113
#define OP_AND				114
#define OP_AND_				(114|RECORD_ON)
#define OP_ORIL				115
#define OP_ORIU				116
#define OP_OR				117
#define OP_OR_				(117|RECORD_ON)
#define OP_XORIL			118
#define OP_XORIU			119
#define OP_XOR				120
#define OP_XOR_				(120|RECORD_ON)
#define OP_ANDC				121
#define OP_ANDC_			(121|RECORD_ON)
#define OP_EQV				122
#define OP_EQV_				(122|RECORD_ON)
#define OP_ORC				123
#define OP_ORC_				(123|RECORD_ON)
#define OP_NOR				124
#define OP_NOR_				(124|RECORD_ON)
#define OP_EXTS				125
#define OP_EXTS_			(125|RECORD_ON)
#define OP_NAND				126
#define OP_NAND_			(126|RECORD_ON)
#define OP_CNTLZ			127
#define OP_CNTLZ_			(127|RECORD_ON)
#define OP_RLIMI			128
#define OP_RLIMI_			(128|RECORD_ON)
#define OP_RLINM			129
#define OP_RLINM_			(129|RECORD_ON)
#define OP_RLMI				130
#define OP_RLMI_			(130|RECORD_ON)
#define OP_RLNM				131
#define OP_RLNM_			(131|RECORD_ON)
#define OP_RRIB				132
#define OP_RRIB_			(132|RECORD_ON)
#define OP_MASKG			133
#define OP_MASKG_			(133|RECORD_ON)
#define OP_MASKIR			134
#define OP_MASKIR_			(134|RECORD_ON)
#define OP_SL				135
#define OP_SL_				(135|RECORD_ON)
#define OP_SR				136
#define OP_SR_				(136|RECORD_ON)
#define OP_SLQ				137
#define OP_SLQ_				(137|RECORD_ON)
#define OP_SRQ				138
#define OP_SRQ_				(138|RECORD_ON)
#define OP_SLIQ				139
#define OP_SLIQ_			(139|RECORD_ON)
#define OP_SLLIQ			140
#define OP_SLLIQ_			(140|RECORD_ON)
#define OP_SRIQ				141
#define OP_SRIQ_			(141|RECORD_ON)
#define OP_SRLIQ			142
#define OP_SRLIQ_			(142|RECORD_ON)
#define OP_SLLQ				143
#define OP_SLLQ_			(143|RECORD_ON)
#define OP_SRLQ				144
#define OP_SRLQ_			(144|RECORD_ON)
#define OP_SLE				145
#define OP_SLE_				(145|RECORD_ON)
#define OP_SLEQ				146
#define OP_SLEQ_			(146|RECORD_ON)
#define OP_SRE				147
#define OP_SRE_				(147|RECORD_ON)
#define OP_SREQ				148
#define OP_SREQ_			(148|RECORD_ON)
#define OP_SRAI				149
#define OP_SRAI_			(149|RECORD_ON)
#define OP_SRA				150
#define OP_SRA_				(150|RECORD_ON)
#define OP_SRAIQ			151
#define OP_SRAIQ_			(151|RECORD_ON)
#define OP_SRAQ				152
#define OP_SRAQ_			(152|RECORD_ON)
#define OP_SREA				153
#define OP_SREA_			(153|RECORD_ON)
#define OP_MTSPR			154
#define OP_MFSPR			155
#define OP_MTCRF			156
#define OP_MFCR				157
#define OP_MCRXR			158
#define OP_MTMSR			159
#define OP_MFMSR			160
#define OP_LFS				161
#define OP_LFSX				162
#define OP_LFD				163
#define OP_LFDX				164
#define OP_LFSU				165
#define OP_LFSUX			166
#define OP_LFDU				167
#define OP_LFDUX			168
#define OP_STFS				169
#define OP_STFSX			170
#define OP_STFD				171
#define OP_STFDX			172
#define OP_STFSU			173
#define OP_STFSUX			174
#define OP_STFDU			175
#define OP_STFDUX			176
#define OP_FMR				177
#define OP_FMR_				(177|RECORD_ON)
#define OP_FABS				178
#define OP_FABS_			(178|RECORD_ON)
#define OP_FNEG				179
#define OP_FNEG_			(179|RECORD_ON)
#define OP_FNABS			180
#define OP_FNABS_			(180|RECORD_ON)
#define OP_FA				181
#define OP_FA_				(181|RECORD_ON)
#define OP_FS				182
#define OP_FS_				(182|RECORD_ON)
#define OP_FM				183
#define OP_FM_				(183|RECORD_ON)
#define OP_FD				184
#define OP_FD_				(184|RECORD_ON)
#define OP_FRSP				185
#define OP_FRSP_			(185|RECORD_ON)
#define OP_FMA				186
#define OP_FMA_				(186|RECORD_ON)
#define OP_FMS				187
#define OP_FMS_				(187|RECORD_ON)
#define OP_FNMA				188
#define OP_FNMA_			(188|RECORD_ON)
#define OP_FNMS				189
#define OP_FNMS_			(189|RECORD_ON)
#define OP_FCMPU			190
#define OP_FCMPO			191
#define OP_MFFS				192
#define OP_MFFS_			(192|RECORD_ON)
#define OP_MCRFS			193
#define OP_MTFSF			194
#define OP_MTFSF_			(194|RECORD_ON)
#define OP_MTFSFI			195
#define OP_MTFSFI_			(195|RECORD_ON)
#define OP_MTFSB1			196
#define OP_MTFSB1_			(196|RECORD_ON)
#define OP_MTFSB0			197
#define OP_MTFSB0_			(197|RECORD_ON)
#define OP_RAC				198
#define OP_RAC_				(198|RECORD_ON)
#define OP_TLBI				199
#define OP_MTSR				200
#define OP_MTSRI			201
#define OP_MFSR				202
#define OP_MFSRI			203
#define OP_DCLZ				204
#define OP_CLF				205
#define OP_CLI				206
#define OP_DCLST			207
#define OP_DCS				208
#define OP_ICS				209
#define OP_CLCS				210

#define OP_MFTB				211		/* PowerPC Only */

#define OP_MULHW			212		/* PowerPC Only */
#define OP_MULHW_		       (212|RECORD_ON)	/* PowerPC Only */
#define OP_MULHWU			213		/* PowerPC Only */
#define OP_MULHWU_		       (213|RECORD_ON)	/* PowerPC Only */
#define OP_MULLD			214		/* PowerPC Only */
#define OP_MULLD_		       (214|RECORD_ON)	/* PowerPC Only */
#define OP_MULLDO			215		/* PowerPC Only */
#define OP_MULLDO_		       (215|RECORD_ON)	/* PowerPC Only */
#define OP_MULHD			216		/* PowerPC Only */
#define OP_MULHD_		       (216|RECORD_ON)	/* PowerPC Only */
#define OP_MULHDU			217		/* PowerPC Only */
#define OP_MULHDU_		       (217|RECORD_ON)	/* PowerPC Only */

#define OP_DIVD				218		/* PowerPC Only */
#define OP_DIVD_		       (218|RECORD_ON)	/* PowerPC Only */
#define OP_DIVDO			219		/* PowerPC Only */
#define OP_DIVDO_		       (219|RECORD_ON)	/* PowerPC Only */
#define OP_DIVW				220		/* PowerPC Only */
#define OP_DIVW_		       (220|RECORD_ON)	/* PowerPC Only */
#define OP_DIVWO			221		/* PowerPC Only */
#define OP_DIVWO_		       (221|RECORD_ON)	/* PowerPC Only */
#define OP_DIVDU			222		/* PowerPC Only */
#define OP_DIVDU_		       (222|RECORD_ON)	/* PowerPC Only */
#define OP_DIVDUO			223		/* PowerPC Only */
#define OP_DIVDUO_		       (223|RECORD_ON)	/* PowerPC Only */
#define OP_DIVWU			224		/* PowerPC Only */
#define OP_DIVWU_		       (224|RECORD_ON)	/* PowerPC Only */
#define OP_DIVWUO			225		/* PowerPC Only */
#define OP_DIVWUO_		       (225|RECORD_ON)	/* PowerPC Only */

#define OP_SUBF				226		/* PowerPC Only */
#define OP_SUBF_		       (226|RECORD_ON)	/* PowerPC Only */
#define OP_SUBFO			227		/* PowerPC Only */
#define OP_SUBFO_		       (227|RECORD_ON)	/* PowerPC Only */

#define OP_DCBF				228		/* PowerPC Only */
#define OP_DCBI				229		/* PowerPC Only */
#define OP_DCBST			230		/* PowerPC Only */
#define OP_DCBT				231		/* PowerPC Only */
#define OP_DCBTST			232		/* PowerPC Only */

#define OP_EIEIO			233		/* PowerPC Only */

#define OP_EXTSB			234		/* PowerPC Only */
#define OP_EXTSB_		       (234|RECORD_ON)	/* PowerPC Only */

#define OP_FADDS			235		/* PowerPC Only */
#define OP_FADDS_		       (235|RECORD_ON)	/* PowerPC Only */
#define OP_FSUBS			236		/* PowerPC Only */
#define OP_FSUBS_		       (236|RECORD_ON)	/* PowerPC Only */
#define OP_FMULS			237		/* PowerPC Only */
#define OP_FMULS_		       (237|RECORD_ON)	/* PowerPC Only */
#define OP_FDIVS			238		/* PowerPC Only */
#define OP_FDIVS_		       (238|RECORD_ON)	/* PowerPC Only */
#define OP_FMADDS			239		/* PowerPC Only */
#define OP_FMADDS_		       (239|RECORD_ON)	/* PowerPC Only */
#define OP_FMSUBS			240		/* PowerPC Only */
#define OP_FMSUBS_		       (240|RECORD_ON)	/* PowerPC Only */
#define OP_FNMADDS			241		/* PowerPC Only */
#define OP_FNMADDS_		       (241|RECORD_ON)	/* PowerPC Only */
#define OP_FNMSUBS			242		/* PowerPC Only */
#define OP_FNMSUBS_		       (242|RECORD_ON)	/* PowerPC Only */
#define OP_FRES				243		/* PowerPC Only */
#define OP_FRES_		       (243|RECORD_ON)	/* PowerPC Only */
#define OP_FSEL				244		/* PowerPC Only */
#define OP_FSEL_		       (244|RECORD_ON)	/* PowerPC Only */

#define OP_M2SPR			245
#define OP_MTCRF2			246
#define OP_MTFSF2			247
#define OP_MTFSF2_		       (247|RECORD_ON)
#define OP_OR_C				248
#define OP_OR_O				249
#define OP_OR_CO			250
#define OP_FMR1				251
#define OP_FMR2				252
#define OP_FMR3				253
#define OP_FMR4				254
#define OP_FMR5				255

#define OP_ILLEGAL			256

#define OP_B				257
#define OP_BA				258
#define OP_BL				259
#define OP_BLA				260
#define OP_BC				261
#define OP_BCA				262
#define OP_BCL				263
#define OP_BCLA				264
#define OP_BCR				265
#define OP_BCRL				266
#define OP_BCC				267
#define OP_BCCL				268

#define OP_BCR2				269
#define OP_BCR2L			270

#define OP_EAE				271
#define OP_EAE_				(271|RECORD_ON)
#define OP_EAEO				272
#define OP_EAEO_			(272|RECORD_ON)
#define OP_ESFE				273
#define OP_ESFE_			(273|RECORD_ON)
#define OP_ESFEO			274
#define OP_ESFEO_			(274|RECORD_ON)
#define OP_EAME				275
#define OP_EAME_			(275|RECORD_ON)
#define OP_EAMEO			276
#define OP_EAMEO_			(276|RECORD_ON)
#define OP_EAZE				277
#define OP_EAZE_			(277|RECORD_ON)
#define OP_EAZEO			278
#define OP_EAZEO_			(278|RECORD_ON)
#define OP_ESFME			279
#define OP_ESFME_			(279|RECORD_ON)
#define OP_ESFMEO			280
#define OP_ESFMEO_			(280|RECORD_ON)
#define OP_ESFZE			281
#define OP_ESFZE_			(281|RECORD_ON)
#define OP_ESFZEO			282
#define OP_ESFZEO_			(282|RECORD_ON)


#define NUM_BASIC_PPC_OPCODES		283
#define NUM_RECORD_OPCODES		136

#define NUM_PPC_OPCODE_SLOTS	(NUM_BASIC_PPC_OPCODES|RECORD_ON)
#define NUM_PPC_OPCODES	(NUM_BASIC_PPC_OPCODES+NUM_RECORD_OPCODES)

#endif
