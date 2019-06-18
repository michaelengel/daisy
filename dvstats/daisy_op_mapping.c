/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          � Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "dis_tbl.h"
#include "parse.h"
#include "op_mapping.h"

/* This is just _op_mapping sorted so op_map_table[i]->opcode = opcode */
       OP_MAPPING *op_map_table[NUM_PPC_OPCODE_SLOTS];

static OP_MAPPING _op_mapping[NUM_PPC_OPCODES] = {
    {"b",	OP_B,		0,	BRA_OP},
    {"ba",	OP_BA,		0,	BRA_OP},
    {"bl",	OP_BL,		0,	BRA_OP},
    {"bla",	OP_BLA,		0,	BRA_OP},
    {"bc",	OP_BC,		0,	BRA_OP|CR_OP},
    {"bca",	OP_BCA,		0,	BRA_OP|CR_OP},
    {"bcl",	OP_BCL,		0,	BRA_OP|CR_OP},
    {"bcla",	OP_BCLA,	0,	BRA_OP|CR_OP},
    {"bcr",	OP_BCR,		0,	BRA_OP},
    {"bcr2",	OP_BCR2,	0,	BRA_OP},
    {"bcrl",	OP_BCRL,	0,	BRA_OP},
    {"bcr2l",	OP_BCR2L,	0,	BRA_OP},
    {"bcc",	OP_BCC,		0,	BRA_OP},
    {"bccl",	OP_BCCL,	0,	BRA_OP},
    {"svc",	OP_SVC,		0,	BRA_OP},
    {"svcl",	OP_SVCL,	0,	BRA_OP},
    {"svca",	OP_SVCA,	0,	BRA_OP},
    {"svcla",	OP_SVCLA,	0,	BRA_OP},
    {"rfsvc",	OP_RFSVC,	0,	BRA_OP},
    {"rfi",	OP_RFI,		0,	BRA_OP},
    {"ti",	OP_TI,		0,	ALU_OP|INT_OP|BRA_OP|TRAP_OP},
    {"t",	OP_T,		0,	ALU_OP|INT_OP|BRA_OP|TRAP_OP},

    {"mcrf",	OP_MCRF,	0,	ALU_OP|CR_OP},
    {"creqv",	OP_CREQV,	0,	ALU_OP|CR_OP},
    {"crand",	OP_CRAND,	0,	ALU_OP|CR_OP},
    {"crxor",	OP_CRXOR,	0,	ALU_OP|CR_OP},
    {"cror",	OP_CROR,	0,	ALU_OP|CR_OP},
    {"crandc",	OP_CRANDC,	0,	ALU_OP|CR_OP},
    {"crnand",	OP_CRNAND,	0,	ALU_OP|CR_OP},
    {"crorc",	OP_CRORC,	0,	ALU_OP|CR_OP},
    {"crnor",	OP_CRNOR,	0,	ALU_OP|CR_OP},

    {"lbz",	OP_LBZ,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lbzx",	OP_LBZX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhz",	OP_LHZ,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhzx",	OP_LHZX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lha",	OP_LHA,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhax",	OP_LHAX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"l",	OP_L,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lx",	OP_LX,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhbrx",	OP_LHBRX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lbrx",	OP_LBRX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lm",	OP_LM,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lbzu",	OP_LBZU,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lbzux",	OP_LBZUX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhzu",	OP_LHZU,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhzux",	OP_LHZUX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhau",	OP_LHAU,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lhaux",	OP_LHAUX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lu",	OP_LU,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lux",	OP_LUX,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lsx",	OP_LSX,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lsi",	OP_LSI,		0,	MEM_OP|LOAD_OP|INT_OP},
    {"lscbx",	OP_LSCBX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"lscbx.",	OP_LSCBX_,	0,	MEM_OP|LOAD_OP|INT_OP|CR_OP},
    {"lfs",	OP_LFS,		0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfsx",	OP_LFSX,	0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfd",	OP_LFD,		0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfdx",	OP_LFDX,	0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfsu",	OP_LFSU,	0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfsux",	OP_LFSUX,	0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfdu",	OP_LFDU,	0,	MEM_OP|LOAD_OP|FLT_OP},
    {"lfdux",	OP_LFDUX,	0,	MEM_OP|LOAD_OP|FLT_OP},

    {"stb",	OP_STB,		0,	MEM_OP|STORE_OP|INT_OP},
    {"stbx",	OP_STBX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"sth",	OP_STH,		0,	MEM_OP|STORE_OP|INT_OP},
    {"sthx",	OP_STHX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"st",	OP_ST,		0,	MEM_OP|STORE_OP|INT_OP},
    {"stx",	OP_STX,		0,	MEM_OP|STORE_OP|INT_OP},
    {"sthbrx",	OP_STHBRX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stbrx",	OP_STBRX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stm",	OP_STM,		0,	MEM_OP|STORE_OP|INT_OP},
    {"stbu",	OP_STBU,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stbux",	OP_STBUX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"sthu",	OP_STHU,	0,	MEM_OP|STORE_OP|INT_OP},
    {"sthux",	OP_STHUX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stu",	OP_STU,		0,	MEM_OP|STORE_OP|INT_OP},
    {"stux",	OP_STUX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stsx",	OP_STSX,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stsi",	OP_STSI,	0,	MEM_OP|STORE_OP|INT_OP},
    {"stfs",	OP_STFS,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfsx",	OP_STFSX,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfd",	OP_STFD,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfdx",	OP_STFDX,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfsu",	OP_STFSU,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfsux",	OP_STFSUX,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfdu",	OP_STFDU,	0,	MEM_OP|STORE_OP|FLT_OP},
    {"stfdux",	OP_STFDUX,	0,	MEM_OP|STORE_OP|FLT_OP},

    {"a_imm",	OP_ADD_IMM,	0,	ALU_OP|INT_OP},
    {"cal",	OP_CAL,		0,	ALU_OP|INT_OP},
    {"cau",	OP_CAU,		0,	ALU_OP|INT_OP},
    {"cax",	OP_CAX,		0,	ALU_OP|INT_OP},
    {"cax.",	OP_CAX_,	0,	ALU_OP|INT_OP|CR_OP},
    {"caxo",	OP_CAXO,	0,	ALU_OP|INT_OP},
    {"caxo.",	OP_CAXO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"ai",	OP_AI,		0,	ALU_OP|INT_OP},
    {"ai.",	OP_AI_,		0,	ALU_OP|INT_OP|CR_OP},
    {"sfi",	OP_SFI,		0,	ALU_OP|INT_OP},
    {"a",	OP_A,		0,	ALU_OP|INT_OP},
    {"a.",	OP_A_,		0,	ALU_OP|INT_OP|CR_OP},
    {"ao",	OP_AO,		0,	ALU_OP|INT_OP},
    {"ao.",	OP_AO_,		0,	ALU_OP|INT_OP|CR_OP},
    {"ae",	OP_AE,		0,	ALU_OP|INT_OP},
    {"ae.",	OP_AE_,		0,	ALU_OP|INT_OP|CR_OP},
    {"aeo",	OP_AEO,		0,	ALU_OP|INT_OP},
    {"aeo.",	OP_AEO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sf",	OP_SF,		0,	ALU_OP|INT_OP},
    {"sf.",	OP_SF_,		0,	ALU_OP|INT_OP|CR_OP},
    {"sfo",	OP_SFO,		0,	ALU_OP|INT_OP},
    {"sfo.",	OP_SFO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sfe",	OP_SFE,		0,	ALU_OP|INT_OP},
    {"sfe.",	OP_SFE_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sfeo",	OP_SFEO,	0,	ALU_OP|INT_OP},
    {"sfeo.",	OP_SFEO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"ame",	OP_AME,		0,	ALU_OP|INT_OP},
    {"ame.",	OP_AME_,	0,	ALU_OP|INT_OP|CR_OP},
    {"ameo",	OP_AMEO,	0,	ALU_OP|INT_OP},
    {"ameo.",	OP_AMEO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"aze",	OP_AZE,		0,	ALU_OP|INT_OP},
    {"aze.",	OP_AZE_,	0,	ALU_OP|INT_OP|CR_OP},
    {"azeo",	OP_AZEO,	0,	ALU_OP|INT_OP},
    {"azeo.",	OP_AZEO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sfme",	OP_SFME,	0,	ALU_OP|INT_OP},
    {"sfme.",	OP_SFME_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sfmeo",	OP_SFMEO,	0,	ALU_OP|INT_OP},
    {"sfmeo.",	OP_SFMEO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sfze",	OP_SFZE,	0,	ALU_OP|INT_OP},
    {"sfze.",	OP_SFZE_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sfzeo",	OP_SFZEO,	0,	ALU_OP|INT_OP},
    {"sfzeo.",	OP_SFZEO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"dozi",	OP_DOZI,	0,	ALU_OP|INT_OP},
    {"doz",	OP_DOZ,		0,	ALU_OP|INT_OP},
    {"doz.",	OP_DOZ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"dozo",	OP_DOZO,	0,	ALU_OP|INT_OP},
    {"dozo.",	OP_DOZO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"abs",	OP_ABS,		0,	ALU_OP|INT_OP},
    {"abs.",	OP_ABS_,	0,	ALU_OP|INT_OP|CR_OP},
    {"abso",	OP_ABSO,	0,	ALU_OP|INT_OP},
    {"abso.",	OP_ABSO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"neg",	OP_NEG,		0,	ALU_OP|INT_OP},
    {"neg.",	OP_NEG_,	0,	ALU_OP|INT_OP|CR_OP},
    {"nego",	OP_NEGO,	0,	ALU_OP|INT_OP},
    {"nego.",	OP_NEGO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"nabs",	OP_NABS,	0,	ALU_OP|INT_OP},
    {"nabs.",	OP_NABS_,	0,	ALU_OP|INT_OP|CR_OP},
    {"nabso",	OP_NABSO,	0,	ALU_OP|INT_OP},
    {"nabso.",	OP_NABSO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mul",	OP_MUL,		0,	ALU_OP|INT_OP},
    {"mul.",	OP_MUL_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulo",	OP_MULO,	0,	ALU_OP|INT_OP},
    {"mulo.",	OP_MULO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"muli",	OP_MULI,	0,	ALU_OP|INT_OP},
    {"muls",	OP_MULS,	0,	ALU_OP|INT_OP},
    {"muls.",	OP_MULS_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulso",	OP_MULSO,	0,	ALU_OP|INT_OP},
    {"mulso.",	OP_MULSO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"div",	OP_DIV,		0,	ALU_OP|INT_OP},
    {"div.",	OP_DIV_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divo",	OP_DIVO,	0,	ALU_OP|INT_OP},
    {"divo.",	OP_DIVO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divs",	OP_DIVS,	0,	ALU_OP|INT_OP},
    {"divs.",	OP_DIVS_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divso",	OP_DIVSO,	0,	ALU_OP|INT_OP},
    {"divso.",	OP_DIVSO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"cmpi",	OP_CMPI,	0,	ALU_OP|INT_OP},
    {"cmpli",	OP_CMPLI,	0,	ALU_OP|INT_OP},
    {"cmp",	OP_CMP,		0,	ALU_OP|INT_OP},
    {"cmpl",	OP_CMPL,	0,	ALU_OP|INT_OP},
    {"andil",	OP_ANDIL,	0,	ALU_OP|INT_OP},
    {"andiu",	OP_ANDIU,	0,	ALU_OP|INT_OP},
    {"and",	OP_AND,		0,	ALU_OP|INT_OP},
    {"and.",	OP_AND_,	0,	ALU_OP|INT_OP|CR_OP},
    {"oril",	OP_ORIL,	0,	ALU_OP|INT_OP},
    {"oriu",	OP_ORIU,	0,	ALU_OP|INT_OP},
    {"or",	OP_OR,		0,	ALU_OP|INT_OP},
    {"or.",	OP_OR_,		0,	ALU_OP|INT_OP|CR_OP},
    {"or_c",	OP_OR_C,	0,	ALU_OP|INT_OP},
    {"or_o",	OP_OR_O,	0,	ALU_OP|INT_OP},
    {"or_co",	OP_OR_CO,	0,	ALU_OP|INT_OP},
    {"xoril",	OP_XORIL,	0,	ALU_OP|INT_OP},
    {"xoriu",	OP_XORIU,	0,	ALU_OP|INT_OP},
    {"xor",	OP_XOR,		0,	ALU_OP|INT_OP},
    {"xor.",	OP_XOR_,	0,	ALU_OP|INT_OP|CR_OP},
    {"andc",	OP_ANDC,	0,	ALU_OP|INT_OP},
    {"andc.",	OP_ANDC_,	0,	ALU_OP|INT_OP|CR_OP},
    {"eqv",	OP_EQV,		0,	ALU_OP|INT_OP},
    {"eqv.",	OP_EQV_,	0,	ALU_OP|INT_OP|CR_OP},
    {"orc",	OP_ORC,		0,	ALU_OP|INT_OP},
    {"orc.",	OP_ORC_,	0,	ALU_OP|INT_OP|CR_OP},
    {"nor",	OP_NOR,		0,	ALU_OP|INT_OP},
    {"nor.",	OP_NOR_,	0,	ALU_OP|INT_OP|CR_OP},
    {"exts",	OP_EXTS,	0,	ALU_OP|INT_OP},
    {"exts.",	OP_EXTS_,	0,	ALU_OP|INT_OP|CR_OP},
    {"nand",	OP_NAND,	0,	ALU_OP|INT_OP},
    {"nand.",	OP_NAND_,	0,	ALU_OP|INT_OP|CR_OP},
    {"cntlz",	OP_CNTLZ,	0,	ALU_OP|INT_OP},
    {"cntlz.",	OP_CNTLZ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"rlimi",	OP_RLIMI,	0,	ALU_OP|INT_OP},
    {"rlimi.",	OP_RLIMI_,	0,	ALU_OP|INT_OP|CR_OP},
    {"rlinm",	OP_RLINM,	0,	ALU_OP|INT_OP},
    {"rlinm.",	OP_RLINM_,	0,	ALU_OP|INT_OP|CR_OP},
    {"rlmi",	OP_RLMI,	0,	ALU_OP|INT_OP},
    {"rlmi.",	OP_RLMI_,	0,	ALU_OP|INT_OP|CR_OP},
    {"rlnm",	OP_RLNM,	0,	ALU_OP|INT_OP},
    {"rlnm.",	OP_RLNM_,	0,	ALU_OP|INT_OP|CR_OP},
    {"rrib",	OP_RRIB,	0,	ALU_OP|INT_OP},
    {"rrib.",	OP_RRIB_,	0,	ALU_OP|INT_OP|CR_OP},
    {"maskg",	OP_MASKG,	0,	ALU_OP|INT_OP},
    {"maskg.",	OP_MASKG_,	0,	ALU_OP|INT_OP|CR_OP},
    {"maskir",	OP_MASKIR,	0,	ALU_OP|INT_OP},
    {"maskir.",	OP_MASKIR_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sl",	OP_SL,		0,	ALU_OP|INT_OP},
    {"sl.",	OP_SL_,		0,	ALU_OP|INT_OP|CR_OP},
    {"sr",	OP_SR,		0,	ALU_OP|INT_OP},
    {"sr.",	OP_SR_,		0,	ALU_OP|INT_OP|CR_OP},
    {"slq",	OP_SLQ,		0,	ALU_OP|INT_OP},
    {"slq.",	OP_SLQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"srq",	OP_SRQ,		0,	ALU_OP|INT_OP},
    {"srq.",	OP_SRQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sliq",	OP_SLIQ,	0,	ALU_OP|INT_OP},
    {"sliq.",	OP_SLIQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"slliq",	OP_SLLIQ,	0,	ALU_OP|INT_OP},
    {"slliq.",	OP_SLLIQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sriq",	OP_SRIQ,	0,	ALU_OP|INT_OP},
    {"sriq.",	OP_SRIQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"srliq",	OP_SRLIQ,	0,	ALU_OP|INT_OP},
    {"srliq.",	OP_SRLIQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sllq",	OP_SLLQ,	0,	ALU_OP|INT_OP},
    {"sllq.",	OP_SLLQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"srlq",	OP_SRLQ,	0,	ALU_OP|INT_OP},
    {"srlq.",	OP_SRLQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sle",	OP_SLE,		0,	ALU_OP|INT_OP},
    {"sle.",	OP_SLE_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sleq",	OP_SLEQ,	0,	ALU_OP|INT_OP},
    {"sleq.",	OP_SLEQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sre",	OP_SRE,		0,	ALU_OP|INT_OP},
    {"sre.",	OP_SRE_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sreq",	OP_SREQ,	0,	ALU_OP|INT_OP},
    {"sreq.",	OP_SREQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"srai",	OP_SRAI,	0,	ALU_OP|INT_OP},
    {"srai.",	OP_SRAI_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sra",	OP_SRA,		0,	ALU_OP|INT_OP},
    {"sra.",	OP_SRA_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sraiq",	OP_SRAIQ,	0,	ALU_OP|INT_OP},
    {"sraiq.",	OP_SRAIQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"sraq",	OP_SRAQ,	0,	ALU_OP|INT_OP},
    {"sraq.",	OP_SRAQ_,	0,	ALU_OP|INT_OP|CR_OP},
    {"srea",	OP_SREA,	0,	ALU_OP|INT_OP},
    {"srea.",	OP_SREA_,	0,	ALU_OP|INT_OP|CR_OP},

    {"mtspr",	OP_MTSPR,	0,	ALU_OP|INT_OP},
    {"mfspr",	OP_MFSPR,	0,	ALU_OP|INT_OP},
    {"m2spr",	OP_M2SPR,	0,	ALU_OP|INT_OP},
    {"mtcrf",	OP_MTCRF,	0,	ALU_OP|INT_OP|CR_OP},
    {"mtcrf2",	OP_MTCRF2,	0,	ALU_OP|INT_OP|CR_OP},
    {"mfcr",	OP_MFCR,	0,	ALU_OP|INT_OP|CR_OP},
    {"mcrxr",	OP_MCRXR,	0,	ALU_OP|INT_OP},
    {"mtmsr",	OP_MTMSR,	0,	ALU_OP|INT_OP},
    {"mfmsr",	OP_MFMSR,	0,	ALU_OP|INT_OP},

    {"fmr",	OP_FMR,		0,	ALU_OP|FLT_OP},
    {"fmr.",	OP_FMR_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fmr1",	OP_FMR1,	0,	ALU_OP|FLT_OP},
    {"fmr2",	OP_FMR2,	0,	ALU_OP|FLT_OP},
    {"fmr3",	OP_FMR3,	0,	ALU_OP|FLT_OP},
    {"fmr4",	OP_FMR4,	0,	ALU_OP|FLT_OP},
    {"fmr5",	OP_FMR5,	0,	ALU_OP|FLT_OP},
    {"fabs",	OP_FABS,	0,	ALU_OP|FLT_OP},
    {"fabs.",	OP_FABS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fneg",	OP_FNEG,	0,	ALU_OP|FLT_OP},
    {"fneg.",	OP_FNEG_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fnabs",	OP_FNABS,	0,	ALU_OP|FLT_OP},
    {"fnabs.",	OP_FNABS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fa",	OP_FA,		0,	ALU_OP|FLT_OP},
    {"fa.",	OP_FA_,		0,	ALU_OP|FLT_OP|CR_OP},
    {"fs",	OP_FS,		0,	ALU_OP|FLT_OP},
    {"fs.",	OP_FS_,		0,	ALU_OP|FLT_OP|CR_OP},
    {"fm",	OP_FM,		0,	ALU_OP|FLT_OP},
    {"fm.",	OP_FM_,		0,	ALU_OP|FLT_OP|CR_OP},
    {"fd",	OP_FD,		0,	ALU_OP|FLT_OP},
    {"fd.",	OP_FD_,		0,	ALU_OP|FLT_OP|CR_OP},
    {"frsp",	OP_FRSP,	0,	ALU_OP|FLT_OP},
    {"frsp.",	OP_FRSP_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fma",	OP_FMA,		0,	ALU_OP|FLT_OP},
    {"fma.",	OP_FMA_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fms",	OP_FMS,		0,	ALU_OP|FLT_OP},
    {"fms.",	OP_FMS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fnma",	OP_FNMA,	0,	ALU_OP|FLT_OP},
    {"fnma.",	OP_FNMA_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fnms",	OP_FNMS,	0,	ALU_OP|FLT_OP},
    {"fnms.",	OP_FNMS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fcmpu",	OP_FCMPU,	0,	ALU_OP|FLT_OP},
    {"fcmpo",	OP_FCMPO,	0,	ALU_OP|FLT_OP},
    {"mffs",	OP_MFFS,	0,	ALU_OP|FLT_OP},
    {"mffs.",	OP_MFFS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"mcrfs",	OP_MCRFS,	0,	ALU_OP|FLT_OP},
    {"mtfsf",	OP_MTFSF,	0,	ALU_OP|FLT_OP},
    {"mtfsf.",	OP_MTFSF_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"mtfsf2",	OP_MTFSF2,	0,	ALU_OP|FLT_OP},
    {"mtfsf2.",	OP_MTFSF2_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"mtfsfi",	OP_MTFSFI,	0,	ALU_OP|FLT_OP},
    {"mtfsfi.",	OP_MTFSFI_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"mtfsb1",	OP_MTFSB1,	0,	ALU_OP|FLT_OP},
    {"mtfsb1.",	OP_MTFSB1_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"mtfsb0",	OP_MTFSB0,	0,	ALU_OP|FLT_OP},
    {"mtfsb0.",	OP_MTFSB0_,	0,	ALU_OP|FLT_OP|CR_OP},

    {"rac",	OP_RAC,		0,	ALU_OP|INT_OP},
    {"rac.",	OP_RAC_,	0,	ALU_OP|INT_OP|CR_OP},
    {"tlbi",	OP_TLBI,	0,	ALU_OP|INT_OP},
    {"mtsr",	OP_MTSR,	0,	ALU_OP|INT_OP},
    {"mtsri",	OP_MTSRI,	0,	ALU_OP|INT_OP},
    {"mfsr",	OP_MFSR,	0,	ALU_OP|INT_OP},
    {"mfsri",	OP_MFSRI,	0,	ALU_OP|INT_OP},
    {"dclz",	OP_DCLZ,	0,	MEM_OP|INT_OP},
    {"clf",	OP_CLF,		0,	MEM_OP|INT_OP},
    {"cli",	OP_CLI,		0,	MEM_OP|INT_OP},
    {"dclst",	OP_DCLST,	0,	MEM_OP|INT_OP},
    {"dcs",	OP_DCS,		0,	MEM_OP},
    {"ics",	OP_ICS,		0,	MEM_OP},
    {"clcs",	OP_CLCS,	0,	MEM_OP|INT_OP},

/* New PowerPC Ops */
    {"mftb",	OP_MFTB,	0,	ALU_OP|INT_OP},		

    {"mulhw",	OP_MULHW,	0,	ALU_OP|INT_OP},
    {"mulhw.",	OP_MULHW_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulhwu",	OP_MULHWU,	0,	ALU_OP|INT_OP},
    {"mulhwu.",	OP_MULHWU_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulld",	OP_MULLD,	0,	ALU_OP|INT_OP},
    {"mulld.",	OP_MULLD_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulldo",	OP_MULLDO,	0,	ALU_OP|INT_OP},
    {"mulldo.",	OP_MULLDO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulhd",	OP_MULHD,	0,	ALU_OP|INT_OP},
    {"mulhd.",	OP_MULHD_,	0,	ALU_OP|INT_OP|CR_OP},
    {"mulhdu",	OP_MULHDU,	0,	ALU_OP|INT_OP},
    {"mulhdu.",	OP_MULHDU_,	0,	ALU_OP|INT_OP|CR_OP},

    {"divd",	OP_DIVD,	0,	ALU_OP|INT_OP},
    {"divd.",	OP_DIVD_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divdo",	OP_DIVDO,	0,	ALU_OP|INT_OP},
    {"divdo.",	OP_DIVDO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divw",	OP_DIVW,	0,	ALU_OP|INT_OP},
    {"divw.",	OP_DIVW_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divwo",	OP_DIVWO,	0,	ALU_OP|INT_OP},
    {"divwo.",	OP_DIVWO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divdu",	OP_DIVDU,	0,	ALU_OP|INT_OP},
    {"divdu.",	OP_DIVDU_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divduo",	OP_DIVDUO,	0,	ALU_OP|INT_OP},
    {"divduo.",	OP_DIVDUO_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divwu",	OP_DIVWU,	0,	ALU_OP|INT_OP},
    {"divwu.",	OP_DIVWU_,	0,	ALU_OP|INT_OP|CR_OP},
    {"divwuo",	OP_DIVWUO,	0,	ALU_OP|INT_OP},
    {"divwuo.",	OP_DIVWUO_,	0,	ALU_OP|INT_OP|CR_OP},

    {"subf",	OP_SUBF,	0,	ALU_OP|INT_OP},
    {"subf.",	OP_SUBF_,	0,	ALU_OP|INT_OP|CR_OP},
    {"subfo",	OP_SUBFO,	0,	ALU_OP|INT_OP},
    {"subfo.",	OP_SUBFO_,	0,	ALU_OP|INT_OP|CR_OP},

    {"dcbf",	OP_DCBF,	0,	MEM_OP|INT_OP},
    {"dcbi",	OP_DCBI,	0,	MEM_OP|INT_OP},
    {"dcbst",	OP_DCBST,	0,	MEM_OP|INT_OP},
    {"dcbt",	OP_DCBT,	0,	MEM_OP|INT_OP},
    {"dcbtst",	OP_DCBTST,	0,	MEM_OP|INT_OP},

    {"eieio",	OP_EIEIO,	0,	ALU_OP|INT_OP},

    {"extsb",	OP_EXTSB,	0,	ALU_OP|INT_OP},
    {"extsb.",	OP_EXTSB_,	0,	ALU_OP|INT_OP|CR_OP},

    {"fadds",	OP_FADDS,	0,	ALU_OP|FLT_OP},
    {"fadds.",	OP_FADDS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fsubs",	OP_FSUBS,	0,	ALU_OP|FLT_OP},
    {"fsubs.",	OP_FSUBS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fmuls",	OP_FMULS,	0,	ALU_OP|FLT_OP},
    {"fmuls.",	OP_FMULS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fdivs",	OP_FDIVS,	0,	ALU_OP|FLT_OP},
    {"fdivs.",	OP_FDIVS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fmadds",	OP_FMADDS,	0,	ALU_OP|FLT_OP},
    {"fmadds.",	OP_FMADDS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fmsubs",	OP_FMSUBS,	0,	ALU_OP|FLT_OP},
    {"fmsubs.",	OP_FMSUBS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fnmadds",	OP_FNMADDS,	0,	ALU_OP|FLT_OP},
    {"fnmadds.",OP_FNMADDS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fnmsubs",	OP_FNMSUBS,	0,	ALU_OP|FLT_OP},
    {"fnmsubs.",OP_FNMSUBS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fres",	OP_FRES,	0,	ALU_OP|FLT_OP},
    {"fres.",	OP_FRES_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fsel",	OP_FSEL,	0,	ALU_OP|FLT_OP},
    {"fsel.",	OP_FSEL_,	0,	ALU_OP|FLT_OP|CR_OP},

    {"fsqrt",	OP_FSQRT,	0,	ALU_OP|FLT_OP},
    {"fsqrt.",	OP_FSQRT_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fsqrts",	OP_FSQRTS,	0,	ALU_OP|FLT_OP},
    {"fsqrts.",	OP_FSQRTS_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fsqrte",	OP_FSQRTE,	0,	ALU_OP|FLT_OP},
    {"fsqrte.",	OP_FSQRTE_,	0,	ALU_OP|FLT_OP|CR_OP},

    {"fctiw",	OP_FCTIW,	0,	ALU_OP|FLT_OP},
    {"fctiw.",	OP_FCTIW_,	0,	ALU_OP|FLT_OP|CR_OP},
    {"fctiwz",	OP_FCTIWZ,	0,	ALU_OP|FLT_OP},
    {"fctiwz.",	OP_FCTIWZ_,	0,	ALU_OP|FLT_OP|CR_OP},

    {"icbi",	OP_ICBI,	0,	ALU_OP|INT_OP},
    {"lwarx",	OP_LWARX,	0,	MEM_OP|LOAD_OP|INT_OP},
    {"stwcx.",	OP_STWCX_,	0,	MEM_OP|STORE_OP|INT_OP|CR_OP},
    {"stfiwx",	OP_STFIWX,	0,	MEM_OP|FLT_OP},

    {"tlbia",	OP_TLBIA,	0,	MEM_OP},
    {"tlbsync",	OP_TLBSYNC,	0,	MEM_OP},
    {"mfsrin",	OP_MFSRIN,	0,	ALU_OP|INT_OP},

    {"eae",     OP_EAE,         0,      ALU_OP|INT_OP},
    {"eae.",    OP_EAE_,        0,      ALU_OP|INT_OP|CR_OP},
    {"eaeo",    OP_EAEO,        0,      ALU_OP|INT_OP},
    {"eaeo.",   OP_EAEO_,       0,      ALU_OP|INT_OP|CR_OP},
    {"esfe",    OP_ESFE,        0,      ALU_OP|INT_OP},
    {"esfe.",   OP_ESFE_,       0,      ALU_OP|INT_OP|CR_OP},
    {"esfeo",   OP_ESFEO,       0,      ALU_OP|INT_OP},
    {"esfeo.",  OP_ESFEO_,      0,      ALU_OP|INT_OP|CR_OP},
    {"eame",    OP_EAME,        0,      ALU_OP|INT_OP},
    {"eame.",   OP_EAME_,       0,      ALU_OP|INT_OP|CR_OP},
    {"eameo",   OP_EAMEO,       0,      ALU_OP|INT_OP},
    {"eameo.",  OP_EAMEO_,      0,      ALU_OP|INT_OP|CR_OP},
    {"eaze",    OP_EAZE,        0,      ALU_OP|INT_OP},
    {"eaze.",   OP_EAZE_,       0,      ALU_OP|INT_OP|CR_OP},
    {"eazeo",   OP_EAZEO,       0,      ALU_OP|INT_OP},
    {"eazeo.",  OP_EAZEO_,      0,      ALU_OP|INT_OP|CR_OP},
    {"esfme",   OP_ESFME,       0,      ALU_OP|INT_OP},
    {"esfme.",  OP_ESFME_,      0,      ALU_OP|INT_OP|CR_OP},
    {"esfmeo",  OP_ESFMEO,      0,      ALU_OP|INT_OP},
    {"esfmeo.", OP_ESFMEO_,     0,      ALU_OP|INT_OP|CR_OP},
    {"esfze",   OP_ESFZE,       0,      ALU_OP|INT_OP},
    {"esfze.",  OP_ESFZE_,      0,      ALU_OP|INT_OP|CR_OP},
    {"esfzeo",  OP_ESFZEO,      0,      ALU_OP|INT_OP},
    {"esfzeo.", OP_ESFZEO_,     0,      ALU_OP|INT_OP|CR_OP},

    {"illegal", OP_ILLEGAL,     0,      ALU_OP}
};

/************************************************************************
 *									*
 *				set_sort_op_mapping			*
 *				-------------------			*
 *									*
 ************************************************************************/

set_sort_op_mapping ()
{
   int i;
   int opcode;

   for (i = 0; i < NUM_PPC_OPCODES; i++) {
      opcode = (int) _op_mapping[i].opcode;
      op_map_table[opcode] = &_op_mapping[i];
   }
}
