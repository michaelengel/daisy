/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          © Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#ifndef PSOpcodes_h
#define PSOpcodes_h

#define	PS_CMPL			0
#define	PS_AND			1
#define	PS_STWCR		2
#define	PS_SUBFZE		3
#define	PS_LTOCWZ		4
#define	PS_SUBF			5
#define	PS_STW			6
#define	PS_COPYTO_CR		7
#define	PS_CMP			8
#define	PS_SLW			9
#define	PS_MCRF			10
#define	PS_SRAW			11
#define	PS_MULHW		12
#define	PS_EIU			13
#define	PS_LHZ			14
#define	PS_MCRFI		15
#define	PS_MTCRF		16
#define	PS_ADDZE		17
#define	PS_FADD			18
#define	PS_LWZCR		19
#define	PS_CNTLZW		20
#define	PS_GOTO			21
#define	PS_MGOTO		22
#define	PS_CALL			23
#define	PS_CALLF		24
#define	PS_FABS			25
#define	PS_STFD			26
#define	PS_FDIV			27
#define	PS_SELECT		28
#define	PS_FMADD		29
#define	PS_CRXOR		30
#define	PS_SUBF_CA		31
#define	PS_LBZ			32
#define	PS_LWZ			33
#define	PS_LLR			34
#define	PS_LFLR			35
#define	PS_LFD			36
#define	PS_FRSP			37
#define	PS_CREQV		38
#define	PS_EXTSH		39
#define	PS_FMUL			40
#define	PS_FNMSUB		41
#define	PS_FCMPU		42
#define	PS_STFS			43
#define	PS_CROR			44
#define	PS_OR			45
#define	PS_MULLW		46
#define	PS_STWLR		47
#define	PS_STB			48
#define	PS_LHA			49
#define	PS_FSUB			50
#define	PS_COPY			51
#define	PS_MFCRF		52
#define	PS_MFCR			53
#define	PS_NOP			54
#define	PS_FNEG			55
#define	PS_STH			56
#define	PS_NOR			57
#define	PS_LFS			58
#define	PS_RLWNM		59
#define	PS_LWZLR		60
#define	PS_SUBFC		61
#define	PS_FMSUB		62
#define	PS_ADD			63
#define	PS_ADD_CA		64
#define	PS_SRAW_CA		65
#define	PS_SRW			66
#define	PS_DIVW			67
#define	PS_XERX			68
#define	PS_LWA			69
#define	PS_ANDC			70
#define	PS_XOR			71
#define	PS_NAND			72
#define	PS_DIVWU		73

#define	PS_STW_ACB		74
#define	PS_STH_ACB		75
#define	PS_STB_ACB		76
#define	PS_STFD_ACB		77
#define	PS_STFS_ACB		78
#define	PS_LWA_ACB		79
#define	PS_LWZ_ACB		80
#define	PS_LHA_ACB		81
#define	PS_LHZ_ACB		82
#define	PS_LBZ_ACB		83
#define	PS_LFD_ACB		84
#define	PS_LFS_ACB		85

#define	PS_TRAP_ACB		86
#define	PS_ITRAP_ACB		87
#define	PS_EQV			88
#define	PS_NEG			89
#define	PS_ORC			90
#define	PS_CRAND		91
#define	PS_CRANDC		92
#define	PS_CRNAND		93
#define	PS_CRNOR		94
#define	PS_CRORC		95

#define	PS_LD_TOUCH		96
#define	PS_ST_TOUCH		97
#define	PS_COPY_ACB		98
#define	PS_COPYWA_ACB		99
#define	PS_COPYWZ_ACB		100
#define	PS_COPYHA_ACB		101
#define	PS_COPYHZ_ACB		102
#define	PS_COPYBZ_ACB		103
#define	PS_COPYFD_ACB		104
#define	PS_COPYFS_ACB		105
#define	PS_LD_VERIFY		106
#define	PS_FCTIWZ		107
#define	PS_FCFID		108
#define	PS_FNABS		109
#define	PS_FNMADD		110
#define	PS_FSELECT		111
#define	PS_FSQRT		112
#define	PS_DELAY		113
#define	PS_MULL			114
#define	PS_DOZ			115
#define	PS_RLWIMI		116
#define	PS_COPYX		117
#define	PS_MUL			118
#define	PS_COPYFROM_CRS		119
#define	PS_TW			120
#define	PS_LTOC			121
#define	PS_MTCR			122
#define	PS_STB_COND		123
#define	PS_STH_COND		124
#define	PS_STW_COND		125
#define	PS_STFS_COND		126
#define	PS_STFD_COND		127
#define	PS_STWCR_COND		128
#define	PS_STWLR_COND		129

#define	PS_LWA_VERIFY		130
#define	PS_LWZ_VERIFY		131
#define	PS_LHA_VERIFY		132
#define	PS_LHZ_VERIFY		133
#define	PS_LBZ_VERIFY		134
#define	PS_LFD_VERIFY		135
#define	PS_LFS_VERIFY		136

/* size of dispatch table */
#define DISPATCH_SIZE	((int) PS_LFS_VERIFY + 1)

#endif
