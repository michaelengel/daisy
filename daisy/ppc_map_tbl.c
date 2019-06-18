/**********************************************************************/
/*                                                                    */
/*                 Licensed Materials - Property of IBM               */
/*          � Copyright IBM Corp. 2000   All Rights Reserved          */
/*                                                                    */
/**********************************************************************/

#include "resrc_map.h"
#include "resrc_offset.h"
#include "resrc_list.h"

/* The fact that R3-R12 are "real" PowerPC registers must match what is
   in r13.s (the "xlate_entry_raw" routine).  Real registers must be
   saved and restored by that routine.
*/
       RESRC_MAP *resrc_map[NUM_MACHINE_REGS];
static RESRC_MAP _resrc_map[] = {
/* General Purpose Registers
   To aid consistency use 
     PPC R0     (immediate 0 for some memory ops)
     PPC R1     (the stack) 
     PPC R2     (the TOC) 
     PPC R3-R12 (parm passing, return values)
*/
{R13_OFFSET, R13,	INTREG},
{R14_OFFSET, R14,	INTREG},
{R15_OFFSET, R15,	INTREG},
{R16_OFFSET, R16,	INTREG},
{R17_OFFSET, R17,	INTREG},
{R18_OFFSET, R18,	INTREG},
{R19_OFFSET, R19,	INTREG},
{R20_OFFSET, R20,	INTREG},
{R21_OFFSET, R21,	INTREG},
{R22_OFFSET, R22,	INTREG},
{R23_OFFSET, R23,	INTREG},
{R24_OFFSET, R24,	INTREG},
{R25_OFFSET, R25,	INTREG},
{R26_OFFSET, R26,	INTREG},
{R27_OFFSET, R27,	INTREG},
{R28_OFFSET, R28,	INTREG},
{R29_OFFSET, R29,	INTREG},
{R30_OFFSET, R30,	INTREG},
{R31_OFFSET, R31,	INTREG},
{R32_OFFSET, R32,	INTREG},
{R33_OFFSET, R33,	INTREG},
{R34_OFFSET, R34,	INTREG},
{R35_OFFSET, R35,	INTREG},
{R36_OFFSET, R36,	INTREG},
{R37_OFFSET, R37,	INTREG},
{R38_OFFSET, R38,	INTREG},
{R39_OFFSET, R39,	INTREG},
{R40_OFFSET, R40,	INTREG},
{R41_OFFSET, R41,	INTREG},
{R42_OFFSET, R42,	INTREG},
{R43_OFFSET, R43,	INTREG},
{R44_OFFSET, R44,	INTREG},
{R45_OFFSET, R45,	INTREG},
{R46_OFFSET, R46,	INTREG},
{R47_OFFSET, R47,	INTREG},
{R48_OFFSET, R48,	INTREG},
{R49_OFFSET, R49,	INTREG},
{R50_OFFSET, R50,	INTREG},
{R51_OFFSET, R51,	INTREG},
{R52_OFFSET, R52,	INTREG},
{R53_OFFSET, R53,	INTREG},
{R54_OFFSET, R54,	INTREG},
{R55_OFFSET, R55,	INTREG},
{R56_OFFSET, R56,	INTREG},
{R57_OFFSET, R57,	INTREG},
{R58_OFFSET, R58,	INTREG},
{R59_OFFSET, R59,	INTREG},
{R60_OFFSET, R60,	INTREG},
{R61_OFFSET, R61,	INTREG},
{R62_OFFSET, R62,	INTREG},
{R63_OFFSET, R63,	INTREG},
{R64_OFFSET, R64,	INTREG},
{R65_OFFSET, R65,	INTREG},
{R66_OFFSET, R66,	INTREG},
{R67_OFFSET, R67,	INTREG},
{R68_OFFSET, R68,	INTREG},
{R69_OFFSET, R69,	INTREG},
{R70_OFFSET, R70,	INTREG},
{R71_OFFSET, R71,	INTREG},
{R72_OFFSET, R72,	INTREG},
{R73_OFFSET, R73,	INTREG},
{R74_OFFSET, R74,	INTREG},
{R75_OFFSET, R75,	INTREG},
{R76_OFFSET, R76,	INTREG},
{R77_OFFSET, R77,	INTREG},
{R78_OFFSET, R78,	INTREG},
{R79_OFFSET, R79,	INTREG},
{R80_OFFSET, R80,	INTREG},
{R81_OFFSET, R81,	INTREG},
{R82_OFFSET, R82,	INTREG},
{R83_OFFSET, R83,	INTREG},
{R84_OFFSET, R84,	INTREG},
{R85_OFFSET, R85,	INTREG},
{R86_OFFSET, R86,	INTREG},
{R87_OFFSET, R87,	INTREG},
{R88_OFFSET, R88,	INTREG},
{R89_OFFSET, R89,	INTREG},
{R90_OFFSET, R90,	INTREG},
{R91_OFFSET, R91,	INTREG},
{R92_OFFSET, R92,	INTREG},
{R93_OFFSET, R93,	INTREG},
{R94_OFFSET, R94,	INTREG},
{R95_OFFSET, R95,	INTREG},
{R96_OFFSET, R96,	INTREG},
{R97_OFFSET, R97,	INTREG},
{R98_OFFSET, R98,	INTREG},
{R99_OFFSET, R99,	INTREG},
{R100_OFFSET, R100,	INTREG},
{R101_OFFSET, R101,	INTREG},
{R102_OFFSET, R102,	INTREG},
{R103_OFFSET, R103,	INTREG},
{R104_OFFSET, R104,	INTREG},
{R105_OFFSET, R105,	INTREG},
{R106_OFFSET, R106,	INTREG},
{R107_OFFSET, R107,	INTREG},
{R108_OFFSET, R108,	INTREG},
{R109_OFFSET, R109,	INTREG},
{R110_OFFSET, R110,	INTREG},
{R111_OFFSET, R111,	INTREG},
{R112_OFFSET, R112,	INTREG},
{R113_OFFSET, R113,	INTREG},
{R114_OFFSET, R114,	INTREG},
{R115_OFFSET, R115,	INTREG},
{R116_OFFSET, R116,	INTREG},
{R117_OFFSET, R117,	INTREG},
{R118_OFFSET, R118,	INTREG},
{R119_OFFSET, R119,	INTREG},
{R120_OFFSET, R120,	INTREG},
{R121_OFFSET, R121,	INTREG},
{R122_OFFSET, R122,	INTREG},
{R123_OFFSET, R123,	INTREG},
{R124_OFFSET, R124,	INTREG},
{R125_OFFSET, R125,	INTREG},
{R126_OFFSET, R126,	INTREG},
{R127_OFFSET, R127,	INTREG},
{R128_OFFSET, R128,	INTREG},
{R129_OFFSET, R129,	INTREG},
{R130_OFFSET, R130,	INTREG},
{R131_OFFSET, R131,	INTREG},
{R132_OFFSET, R132,	INTREG},
{R133_OFFSET, R133,	INTREG},
{R134_OFFSET, R134,	INTREG},
{R135_OFFSET, R135,	INTREG},
{R136_OFFSET, R136,	INTREG},
{R137_OFFSET, R137,	INTREG},
{R138_OFFSET, R138,	INTREG},
{R139_OFFSET, R139,	INTREG},
{R140_OFFSET, R140,	INTREG},
{R141_OFFSET, R141,	INTREG},
{R142_OFFSET, R142,	INTREG},
{R143_OFFSET, R143,	INTREG},
{R144_OFFSET, R144,	INTREG},
{R145_OFFSET, R145,	INTREG},
{R146_OFFSET, R146,	INTREG},
{R147_OFFSET, R147,	INTREG},
{R148_OFFSET, R148,	INTREG},
{R149_OFFSET, R149,	INTREG},
{R150_OFFSET, R150,	INTREG},
{R151_OFFSET, R151,	INTREG},
{R152_OFFSET, R152,	INTREG},
{R153_OFFSET, R153,	INTREG},
{R154_OFFSET, R154,	INTREG},
{R155_OFFSET, R155,	INTREG},
{R156_OFFSET, R156,	INTREG},
{R157_OFFSET, R157,	INTREG},
{R158_OFFSET, R158,	INTREG},
{R159_OFFSET, R159,	INTREG},
{R160_OFFSET, R160,	INTREG},
{R161_OFFSET, R161,	INTREG},
{R162_OFFSET, R162,	INTREG},
{R163_OFFSET, R163,	INTREG},
{R164_OFFSET, R164,	INTREG},
{R165_OFFSET, R165,	INTREG},
{R166_OFFSET, R166,	INTREG},
{R167_OFFSET, R167,	INTREG},
{R168_OFFSET, R168,	INTREG},
{R169_OFFSET, R169,	INTREG},
{R170_OFFSET, R170,	INTREG},
{R171_OFFSET, R171,	INTREG},
{R172_OFFSET, R172,	INTREG},
{R173_OFFSET, R173,	INTREG},
{R174_OFFSET, R174,	INTREG},
{R175_OFFSET, R175,	INTREG},
{R176_OFFSET, R176,	INTREG},
{R177_OFFSET, R177,	INTREG},
{R178_OFFSET, R178,	INTREG},
{R179_OFFSET, R179,	INTREG},
{R180_OFFSET, R180,	INTREG},
{R181_OFFSET, R181,	INTREG},
{R182_OFFSET, R182,	INTREG},
{R183_OFFSET, R183,	INTREG},
{R184_OFFSET, R184,	INTREG},
{R185_OFFSET, R185,	INTREG},
{R186_OFFSET, R186,	INTREG},
{R187_OFFSET, R187,	INTREG},
{R188_OFFSET, R188,	INTREG},
{R189_OFFSET, R189,	INTREG},
{R190_OFFSET, R190,	INTREG},
{R191_OFFSET, R191,	INTREG},
{R192_OFFSET, R192,	INTREG},
{R193_OFFSET, R193,	INTREG},
{R194_OFFSET, R194,	INTREG},
{R195_OFFSET, R195,	INTREG},
{R196_OFFSET, R196,	INTREG},
{R197_OFFSET, R197,	INTREG},
{R198_OFFSET, R198,	INTREG},
{R199_OFFSET, R199,	INTREG},
{R200_OFFSET, R200,	INTREG},
{R201_OFFSET, R201,	INTREG},
{R202_OFFSET, R202,	INTREG},
{R203_OFFSET, R203,	INTREG},
{R204_OFFSET, R204,	INTREG},
{R205_OFFSET, R205,	INTREG},
{R206_OFFSET, R206,	INTREG},
{R207_OFFSET, R207,	INTREG},
{R208_OFFSET, R208,	INTREG},
{R209_OFFSET, R209,	INTREG},
{R210_OFFSET, R210,	INTREG},
{R211_OFFSET, R211,	INTREG},
{R212_OFFSET, R212,	INTREG},
{R213_OFFSET, R213,	INTREG},
{R214_OFFSET, R214,	INTREG},
{R215_OFFSET, R215,	INTREG},
{R216_OFFSET, R216,	INTREG},
{R217_OFFSET, R217,	INTREG},
{R218_OFFSET, R218,	INTREG},
{R219_OFFSET, R219,	INTREG},
{R220_OFFSET, R220,	INTREG},
{R221_OFFSET, R221,	INTREG},
{R222_OFFSET, R222,	INTREG},
{R223_OFFSET, R223,	INTREG},
{R224_OFFSET, R224,	INTREG},
{R225_OFFSET, R225,	INTREG},
{R226_OFFSET, R226,	INTREG},
{R227_OFFSET, R227,	INTREG},
{R228_OFFSET, R228,	INTREG},
{R229_OFFSET, R229,	INTREG},
{R230_OFFSET, R230,	INTREG},
{R231_OFFSET, R231,	INTREG},
{R232_OFFSET, R232,	INTREG},
{R233_OFFSET, R233,	INTREG},
{R234_OFFSET, R234,	INTREG},
{R235_OFFSET, R235,	INTREG},
{R236_OFFSET, R236,	INTREG},
{R237_OFFSET, R237,	INTREG},
{R238_OFFSET, R238,	INTREG},
{R239_OFFSET, R239,	INTREG},
{R240_OFFSET, R240,	INTREG},
{R241_OFFSET, R241,	INTREG},
{R242_OFFSET, R242,	INTREG},
{R243_OFFSET, R243,	INTREG},
{R244_OFFSET, R244,	INTREG},
{R245_OFFSET, R245,	INTREG},
{R246_OFFSET, R246,	INTREG},
{R247_OFFSET, R247,	INTREG},
{R248_OFFSET, R248,	INTREG},
{R249_OFFSET, R249,	INTREG},
{R250_OFFSET, R250,	INTREG},
{R251_OFFSET, R251,	INTREG},
{R252_OFFSET, R252,	INTREG},
{R253_OFFSET, R253,	INTREG},
{R254_OFFSET, R254,	INTREG},
{R255_OFFSET, R255,	INTREG},

/* Floating Point Registers:  Leave FP0 - FP23 to use real PPC regs */
{FP24_OFFSET, FP24,	FLTREG},
{FP25_OFFSET, FP25,	FLTREG},
{FP26_OFFSET, FP26,	FLTREG},
{FP27_OFFSET, FP27,	FLTREG},
{FP28_OFFSET, FP28,	FLTREG},
{FP29_OFFSET, FP29,	FLTREG},
{FP30_OFFSET, FP30,	FLTREG},
{FP31_OFFSET, FP31,	FLTREG},
{FP32_OFFSET, FP32,	FLTREG},
{FP33_OFFSET, FP33,	FLTREG},
{FP34_OFFSET, FP34,	FLTREG},
{FP35_OFFSET, FP35,	FLTREG},
{FP36_OFFSET, FP36,	FLTREG},
{FP37_OFFSET, FP37,	FLTREG},
{FP38_OFFSET, FP38,	FLTREG},
{FP39_OFFSET, FP39,	FLTREG},
{FP40_OFFSET, FP40,	FLTREG},
{FP41_OFFSET, FP41,	FLTREG},
{FP42_OFFSET, FP42,	FLTREG},
{FP43_OFFSET, FP43,	FLTREG},
{FP44_OFFSET, FP44,	FLTREG},
{FP45_OFFSET, FP45,	FLTREG},
{FP46_OFFSET, FP46,	FLTREG},
{FP47_OFFSET, FP47,	FLTREG},
{FP48_OFFSET, FP48,	FLTREG},
{FP49_OFFSET, FP49,	FLTREG},
{FP50_OFFSET, FP50,	FLTREG},
{FP51_OFFSET, FP51,	FLTREG},
{FP52_OFFSET, FP52,	FLTREG},
{FP53_OFFSET, FP53,	FLTREG},
{FP54_OFFSET, FP54,	FLTREG},
{FP55_OFFSET, FP55,	FLTREG},
{FP56_OFFSET, FP56,	FLTREG},
{FP57_OFFSET, FP57,	FLTREG},
{FP58_OFFSET, FP58,	FLTREG},
{FP59_OFFSET, FP59,	FLTREG},
{FP60_OFFSET, FP60,	FLTREG},
{FP61_OFFSET, FP61,	FLTREG},
{FP62_OFFSET, FP62,	FLTREG},
{FP63_OFFSET, FP63,	FLTREG},
{FP64_OFFSET, FP64,	FLTREG},
{FP65_OFFSET, FP65,	FLTREG},
{FP66_OFFSET, FP66,	FLTREG},
{FP67_OFFSET, FP67,	FLTREG},
{FP68_OFFSET, FP68,	FLTREG},
{FP69_OFFSET, FP69,	FLTREG},
{FP70_OFFSET, FP70,	FLTREG},
{FP71_OFFSET, FP71,	FLTREG},
{FP72_OFFSET, FP72,	FLTREG},
{FP73_OFFSET, FP73,	FLTREG},
{FP74_OFFSET, FP74,	FLTREG},
{FP75_OFFSET, FP75,	FLTREG},
{FP76_OFFSET, FP76,	FLTREG},
{FP77_OFFSET, FP77,	FLTREG},
{FP78_OFFSET, FP78,	FLTREG},
{FP79_OFFSET, FP79,	FLTREG},
{FP80_OFFSET, FP80,	FLTREG},
{FP81_OFFSET, FP81,	FLTREG},
{FP82_OFFSET, FP82,	FLTREG},
{FP83_OFFSET, FP83,	FLTREG},
{FP84_OFFSET, FP84,	FLTREG},
{FP85_OFFSET, FP85,	FLTREG},
{FP86_OFFSET, FP86,	FLTREG},
{FP87_OFFSET, FP87,	FLTREG},
{FP88_OFFSET, FP88,	FLTREG},
{FP89_OFFSET, FP89,	FLTREG},
{FP90_OFFSET, FP90,	FLTREG},
{FP91_OFFSET, FP91,	FLTREG},
{FP92_OFFSET, FP92,	FLTREG},
{FP93_OFFSET, FP93,	FLTREG},
{FP94_OFFSET, FP94,	FLTREG},
{FP95_OFFSET, FP95,	FLTREG},
{FP96_OFFSET, FP96,	FLTREG},
{FP97_OFFSET, FP97,	FLTREG},
{FP98_OFFSET, FP98,	FLTREG},
{FP99_OFFSET, FP99,	FLTREG},
{FP100_OFFSET, FP100,	FLTREG},
{FP101_OFFSET, FP101,	FLTREG},
{FP102_OFFSET, FP102,	FLTREG},
{FP103_OFFSET, FP103,	FLTREG},
{FP104_OFFSET, FP104,	FLTREG},
{FP105_OFFSET, FP105,	FLTREG},
{FP106_OFFSET, FP106,	FLTREG},
{FP107_OFFSET, FP107,	FLTREG},
{FP108_OFFSET, FP108,	FLTREG},
{FP109_OFFSET, FP109,	FLTREG},
{FP110_OFFSET, FP110,	FLTREG},
{FP111_OFFSET, FP111,	FLTREG},
{FP112_OFFSET, FP112,	FLTREG},
{FP113_OFFSET, FP113,	FLTREG},
{FP114_OFFSET, FP114,	FLTREG},
{FP115_OFFSET, FP115,	FLTREG},
{FP116_OFFSET, FP116,	FLTREG},
{FP117_OFFSET, FP117,	FLTREG},
{FP118_OFFSET, FP118,	FLTREG},
{FP119_OFFSET, FP119,	FLTREG},
{FP120_OFFSET, FP120,	FLTREG},
{FP121_OFFSET, FP121,	FLTREG},
{FP122_OFFSET, FP122,	FLTREG},
{FP123_OFFSET, FP123,	FLTREG},
{FP124_OFFSET, FP124,	FLTREG},
{FP125_OFFSET, FP125,	FLTREG},
{FP126_OFFSET, FP126,	FLTREG},
{FP127_OFFSET, FP127,	FLTREG},
{FP128_OFFSET, FP128,	FLTREG},
{FP129_OFFSET, FP129,	FLTREG},
{FP130_OFFSET, FP130,	FLTREG},
{FP131_OFFSET, FP131,	FLTREG},
{FP132_OFFSET, FP132,	FLTREG},
{FP133_OFFSET, FP133,	FLTREG},
{FP134_OFFSET, FP134,	FLTREG},
{FP135_OFFSET, FP135,	FLTREG},
{FP136_OFFSET, FP136,	FLTREG},
{FP137_OFFSET, FP137,	FLTREG},
{FP138_OFFSET, FP138,	FLTREG},
{FP139_OFFSET, FP139,	FLTREG},
{FP140_OFFSET, FP140,	FLTREG},
{FP141_OFFSET, FP141,	FLTREG},
{FP142_OFFSET, FP142,	FLTREG},
{FP143_OFFSET, FP143,	FLTREG},
{FP144_OFFSET, FP144,	FLTREG},
{FP145_OFFSET, FP145,	FLTREG},
{FP146_OFFSET, FP146,	FLTREG},
{FP147_OFFSET, FP147,	FLTREG},
{FP148_OFFSET, FP148,	FLTREG},
{FP149_OFFSET, FP149,	FLTREG},
{FP150_OFFSET, FP150,	FLTREG},
{FP151_OFFSET, FP151,	FLTREG},
{FP152_OFFSET, FP152,	FLTREG},
{FP153_OFFSET, FP153,	FLTREG},
{FP154_OFFSET, FP154,	FLTREG},
{FP155_OFFSET, FP155,	FLTREG},
{FP156_OFFSET, FP156,	FLTREG},
{FP157_OFFSET, FP157,	FLTREG},
{FP158_OFFSET, FP158,	FLTREG},
{FP159_OFFSET, FP159,	FLTREG},
{FP160_OFFSET, FP160,	FLTREG},
{FP161_OFFSET, FP161,	FLTREG},
{FP162_OFFSET, FP162,	FLTREG},
{FP163_OFFSET, FP163,	FLTREG},
{FP164_OFFSET, FP164,	FLTREG},
{FP165_OFFSET, FP165,	FLTREG},
{FP166_OFFSET, FP166,	FLTREG},
{FP167_OFFSET, FP167,	FLTREG},
{FP168_OFFSET, FP168,	FLTREG},
{FP169_OFFSET, FP169,	FLTREG},
{FP170_OFFSET, FP170,	FLTREG},
{FP171_OFFSET, FP171,	FLTREG},
{FP172_OFFSET, FP172,	FLTREG},
{FP173_OFFSET, FP173,	FLTREG},
{FP174_OFFSET, FP174,	FLTREG},
{FP175_OFFSET, FP175,	FLTREG},
{FP176_OFFSET, FP176,	FLTREG},
{FP177_OFFSET, FP177,	FLTREG},
{FP178_OFFSET, FP178,	FLTREG},
{FP179_OFFSET, FP179,	FLTREG},
{FP180_OFFSET, FP180,	FLTREG},
{FP181_OFFSET, FP181,	FLTREG},
{FP182_OFFSET, FP182,	FLTREG},
{FP183_OFFSET, FP183,	FLTREG},
{FP184_OFFSET, FP184,	FLTREG},
{FP185_OFFSET, FP185,	FLTREG},
{FP186_OFFSET, FP186,	FLTREG},
{FP187_OFFSET, FP187,	FLTREG},
{FP188_OFFSET, FP188,	FLTREG},
{FP189_OFFSET, FP189,	FLTREG},
{FP190_OFFSET, FP190,	FLTREG},
{FP191_OFFSET, FP191,	FLTREG},
{FP192_OFFSET, FP192,	FLTREG},
{FP193_OFFSET, FP193,	FLTREG},
{FP194_OFFSET, FP194,	FLTREG},
{FP195_OFFSET, FP195,	FLTREG},
{FP196_OFFSET, FP196,	FLTREG},
{FP197_OFFSET, FP197,	FLTREG},
{FP198_OFFSET, FP198,	FLTREG},
{FP199_OFFSET, FP199,	FLTREG},
{FP200_OFFSET, FP200,	FLTREG},
{FP201_OFFSET, FP201,	FLTREG},
{FP202_OFFSET, FP202,	FLTREG},
{FP203_OFFSET, FP203,	FLTREG},
{FP204_OFFSET, FP204,	FLTREG},
{FP205_OFFSET, FP205,	FLTREG},
{FP206_OFFSET, FP206,	FLTREG},
{FP207_OFFSET, FP207,	FLTREG},
{FP208_OFFSET, FP208,	FLTREG},
{FP209_OFFSET, FP209,	FLTREG},
{FP210_OFFSET, FP210,	FLTREG},
{FP211_OFFSET, FP211,	FLTREG},
{FP212_OFFSET, FP212,	FLTREG},
{FP213_OFFSET, FP213,	FLTREG},
{FP214_OFFSET, FP214,	FLTREG},
{FP215_OFFSET, FP215,	FLTREG},
{FP216_OFFSET, FP216,	FLTREG},
{FP217_OFFSET, FP217,	FLTREG},
{FP218_OFFSET, FP218,	FLTREG},
{FP219_OFFSET, FP219,	FLTREG},
{FP220_OFFSET, FP220,	FLTREG},
{FP221_OFFSET, FP221,	FLTREG},
{FP222_OFFSET, FP222,	FLTREG},
{FP223_OFFSET, FP223,	FLTREG},
{FP224_OFFSET, FP224,	FLTREG},
{FP225_OFFSET, FP225,	FLTREG},
{FP226_OFFSET, FP226,	FLTREG},
{FP227_OFFSET, FP227,	FLTREG},
{FP228_OFFSET, FP228,	FLTREG},
{FP229_OFFSET, FP229,	FLTREG},
{FP230_OFFSET, FP230,	FLTREG},
{FP231_OFFSET, FP231,	FLTREG},
{FP232_OFFSET, FP232,	FLTREG},
{FP233_OFFSET, FP233,	FLTREG},
{FP234_OFFSET, FP234,	FLTREG},
{FP235_OFFSET, FP235,	FLTREG},
{FP236_OFFSET, FP236,	FLTREG},
{FP237_OFFSET, FP237,	FLTREG},
{FP238_OFFSET, FP238,	FLTREG},
{FP239_OFFSET, FP239,	FLTREG},
{FP240_OFFSET, FP240,	FLTREG},
{FP241_OFFSET, FP241,	FLTREG},
{FP242_OFFSET, FP242,	FLTREG},
{FP243_OFFSET, FP243,	FLTREG},
{FP244_OFFSET, FP244,	FLTREG},
{FP245_OFFSET, FP245,	FLTREG},
{FP246_OFFSET, FP246,	FLTREG},
{FP247_OFFSET, FP247,	FLTREG},
{FP248_OFFSET, FP248,	FLTREG},
{FP249_OFFSET, FP249,	FLTREG},
{FP250_OFFSET, FP250,	FLTREG},
{FP251_OFFSET, FP251,	FLTREG},
{FP252_OFFSET, FP252,	FLTREG},
{FP253_OFFSET, FP253,	FLTREG},
{FP254_OFFSET, FP254,	FLTREG},
{FP255_OFFSET, FP255,	FLTREG},

/* Condition Code Registers Bits */
{CRF0_OFFSET,  CR0,	CONDREG},
{CRF0_OFFSET,  CR1,	CONDREG},
{CRF0_OFFSET,  CR2,	CONDREG},
{CRF0_OFFSET,  CR3,	CONDREG},
{CRF1_OFFSET,  CR4,	CONDREG},
{CRF1_OFFSET,  CR5,	CONDREG},
{CRF1_OFFSET,  CR6,	CONDREG},
{CRF1_OFFSET,  CR7,	CONDREG},
{CRF2_OFFSET,  CR8,	CONDREG},
{CRF2_OFFSET,  CR9,	CONDREG},
{CRF2_OFFSET, CR10,	CONDREG},
{CRF2_OFFSET, CR11,	CONDREG},
{CRF3_OFFSET, CR12,	CONDREG},
{CRF3_OFFSET, CR13,	CONDREG},
{CRF3_OFFSET, CR14,	CONDREG},
{CRF3_OFFSET, CR15,	CONDREG},
{CRF4_OFFSET, CR16,	CONDREG},
{CRF4_OFFSET, CR17,	CONDREG},
{CRF4_OFFSET, CR18,	CONDREG},
{CRF4_OFFSET, CR19,	CONDREG},
{CRF5_OFFSET, CR20,	CONDREG},
{CRF5_OFFSET, CR21,	CONDREG},
{CRF5_OFFSET, CR22,	CONDREG},
{CRF5_OFFSET, CR23,	CONDREG},
{CRF6_OFFSET, CR24,	CONDREG},
{CRF6_OFFSET, CR25,	CONDREG},
{CRF6_OFFSET, CR26,	CONDREG},
{CRF6_OFFSET, CR27,	CONDREG},
{CRF7_OFFSET, CR28,	CONDREG},
{CRF7_OFFSET, CR29,	CONDREG},
{CRF7_OFFSET, CR30,	CONDREG},
{CRF7_OFFSET, CR31,	CONDREG},
{CRF8_OFFSET, CR32,	CONDREG},
{CRF8_OFFSET, CR33,	CONDREG},
{CRF8_OFFSET, CR34,	CONDREG},
{CRF8_OFFSET, CR35,	CONDREG},
{CRF9_OFFSET, CR36,	CONDREG},
{CRF9_OFFSET, CR37,	CONDREG},
{CRF9_OFFSET, CR38,	CONDREG},
{CRF9_OFFSET, CR39,	CONDREG},
{CRF10_OFFSET, CR40,	CONDREG},
{CRF10_OFFSET, CR41,	CONDREG},
{CRF10_OFFSET, CR42,	CONDREG},
{CRF10_OFFSET, CR43,	CONDREG},
{CRF11_OFFSET, CR44,	CONDREG},
{CRF11_OFFSET, CR45,	CONDREG},
{CRF11_OFFSET, CR46,	CONDREG},
{CRF11_OFFSET, CR47,	CONDREG},
{CRF12_OFFSET, CR48,	CONDREG},
{CRF12_OFFSET, CR49,	CONDREG},
{CRF12_OFFSET, CR50,	CONDREG},
{CRF12_OFFSET, CR51,	CONDREG},
{CRF13_OFFSET, CR52,	CONDREG},
{CRF13_OFFSET, CR53,	CONDREG},
{CRF13_OFFSET, CR54,	CONDREG},
{CRF13_OFFSET, CR55,	CONDREG},
{CRF14_OFFSET, CR56,	CONDREG},
{CRF14_OFFSET, CR57,	CONDREG},
{CRF14_OFFSET, CR58,	CONDREG},
{CRF14_OFFSET, CR59,	CONDREG},
{CRF15_OFFSET, CR60,	CONDREG},
{CRF15_OFFSET, CR61,	CONDREG},
{CRF15_OFFSET, CR62,	CONDREG},
{CRF15_OFFSET, CR63,	CONDREG},
{CRF16_OFFSET, CR64,	CONDREG},
{CRF16_OFFSET, CR65,	CONDREG},
{CRF16_OFFSET, CR66,	CONDREG},
{CRF16_OFFSET, CR67,	CONDREG},
{CRF17_OFFSET, CR68,	CONDREG},
{CRF17_OFFSET, CR69,	CONDREG},
{CRF17_OFFSET, CR70,	CONDREG},
{CRF17_OFFSET, CR71,	CONDREG},
{CRF18_OFFSET, CR72,	CONDREG},
{CRF18_OFFSET, CR73,	CONDREG},
{CRF18_OFFSET, CR74,	CONDREG},
{CRF18_OFFSET, CR75,	CONDREG},
{CRF19_OFFSET, CR76,	CONDREG},
{CRF19_OFFSET, CR77,	CONDREG},
{CRF19_OFFSET, CR78,	CONDREG},
{CRF19_OFFSET, CR79,	CONDREG},
{CRF20_OFFSET, CR80,	CONDREG},
{CRF20_OFFSET, CR81,	CONDREG},
{CRF20_OFFSET, CR82,	CONDREG},
{CRF20_OFFSET, CR83,	CONDREG},
{CRF21_OFFSET, CR84,	CONDREG},
{CRF21_OFFSET, CR85,	CONDREG},
{CRF21_OFFSET, CR86,	CONDREG},
{CRF21_OFFSET, CR87,	CONDREG},
{CRF22_OFFSET, CR88,	CONDREG},
{CRF22_OFFSET, CR89,	CONDREG},
{CRF22_OFFSET, CR90,	CONDREG},
{CRF22_OFFSET, CR91,	CONDREG},
{CRF23_OFFSET, CR92,	CONDREG},
{CRF23_OFFSET, CR93,	CONDREG},
{CRF23_OFFSET, CR94,	CONDREG},
{CRF23_OFFSET, CR95,	CONDREG},
{CRF24_OFFSET, CR96,	CONDREG},
{CRF24_OFFSET, CR97,	CONDREG},
{CRF24_OFFSET, CR98,	CONDREG},
{CRF24_OFFSET, CR99,	CONDREG},
{CRF25_OFFSET, CR100,	CONDREG},
{CRF25_OFFSET, CR101,	CONDREG},
{CRF25_OFFSET, CR102,	CONDREG},
{CRF25_OFFSET, CR103,	CONDREG},
{CRF26_OFFSET, CR104,	CONDREG},
{CRF26_OFFSET, CR105,	CONDREG},
{CRF26_OFFSET, CR106,	CONDREG},
{CRF26_OFFSET, CR107,	CONDREG},
{CRF27_OFFSET, CR108,	CONDREG},
{CRF27_OFFSET, CR109,	CONDREG},
{CRF27_OFFSET, CR110,	CONDREG},
{CRF27_OFFSET, CR111,	CONDREG},
{CRF28_OFFSET, CR112,	CONDREG},
{CRF28_OFFSET, CR113,	CONDREG},
{CRF28_OFFSET, CR114,	CONDREG},
{CRF28_OFFSET, CR115,	CONDREG},
{CRF29_OFFSET, CR116,	CONDREG},
{CRF29_OFFSET, CR117,	CONDREG},
{CRF29_OFFSET, CR118,	CONDREG},
{CRF29_OFFSET, CR119,	CONDREG},
{CRF30_OFFSET, CR120,	CONDREG},
{CRF30_OFFSET, CR121,	CONDREG},
{CRF30_OFFSET, CR122,	CONDREG},
{CRF30_OFFSET, CR123,	CONDREG},
{CRF31_OFFSET, CR124,	CONDREG},
{CRF31_OFFSET, CR125,	CONDREG},
{CRF31_OFFSET, CR126,	CONDREG},
{CRF31_OFFSET, CR127,	CONDREG},
{CRF32_OFFSET, CR128,	CONDREG},
{CRF32_OFFSET, CR129,	CONDREG},
{CRF32_OFFSET, CR130,	CONDREG},
{CRF32_OFFSET, CR131,	CONDREG},
{CRF33_OFFSET, CR132,	CONDREG},
{CRF33_OFFSET, CR133,	CONDREG},
{CRF33_OFFSET, CR134,	CONDREG},
{CRF33_OFFSET, CR135,	CONDREG},
{CRF34_OFFSET, CR136,	CONDREG},
{CRF34_OFFSET, CR137,	CONDREG},
{CRF34_OFFSET, CR138,	CONDREG},
{CRF34_OFFSET, CR139,	CONDREG},
{CRF35_OFFSET, CR140,	CONDREG},
{CRF35_OFFSET, CR141,	CONDREG},
{CRF35_OFFSET, CR142,	CONDREG},
{CRF35_OFFSET, CR143,	CONDREG},
{CRF36_OFFSET, CR144,	CONDREG},
{CRF36_OFFSET, CR145,	CONDREG},
{CRF36_OFFSET, CR146,	CONDREG},
{CRF36_OFFSET, CR147,	CONDREG},
{CRF37_OFFSET, CR148,	CONDREG},
{CRF37_OFFSET, CR149,	CONDREG},
{CRF37_OFFSET, CR150,	CONDREG},
{CRF37_OFFSET, CR151,	CONDREG},
{CRF38_OFFSET, CR152,	CONDREG},
{CRF38_OFFSET, CR153,	CONDREG},
{CRF38_OFFSET, CR154,	CONDREG},
{CRF38_OFFSET, CR155,	CONDREG},
{CRF39_OFFSET, CR156,	CONDREG},
{CRF39_OFFSET, CR157,	CONDREG},
{CRF39_OFFSET, CR158,	CONDREG},
{CRF39_OFFSET, CR159,	CONDREG},
{CRF40_OFFSET, CR160,	CONDREG},
{CRF40_OFFSET, CR161,	CONDREG},
{CRF40_OFFSET, CR162,	CONDREG},
{CRF40_OFFSET, CR163,	CONDREG},
{CRF41_OFFSET, CR164,	CONDREG},
{CRF41_OFFSET, CR165,	CONDREG},
{CRF41_OFFSET, CR166,	CONDREG},
{CRF41_OFFSET, CR167,	CONDREG},
{CRF42_OFFSET, CR168,	CONDREG},
{CRF42_OFFSET, CR169,	CONDREG},
{CRF42_OFFSET, CR170,	CONDREG},
{CRF42_OFFSET, CR171,	CONDREG},
{CRF43_OFFSET, CR172,	CONDREG},
{CRF43_OFFSET, CR173,	CONDREG},
{CRF43_OFFSET, CR174,	CONDREG},
{CRF43_OFFSET, CR175,	CONDREG},
{CRF44_OFFSET, CR176,	CONDREG},
{CRF44_OFFSET, CR177,	CONDREG},
{CRF44_OFFSET, CR178,	CONDREG},
{CRF44_OFFSET, CR179,	CONDREG},
{CRF45_OFFSET, CR180,	CONDREG},
{CRF45_OFFSET, CR181,	CONDREG},
{CRF45_OFFSET, CR182,	CONDREG},
{CRF45_OFFSET, CR183,	CONDREG},
{CRF46_OFFSET, CR184,	CONDREG},
{CRF46_OFFSET, CR185,	CONDREG},
{CRF46_OFFSET, CR186,	CONDREG},
{CRF46_OFFSET, CR187,	CONDREG},
{CRF47_OFFSET, CR188,	CONDREG},
{CRF47_OFFSET, CR189,	CONDREG},
{CRF47_OFFSET, CR190,	CONDREG},
{CRF47_OFFSET, CR191,	CONDREG},
{CRF48_OFFSET, CR192,	CONDREG},
{CRF48_OFFSET, CR193,	CONDREG},
{CRF48_OFFSET, CR194,	CONDREG},
{CRF48_OFFSET, CR195,	CONDREG},
{CRF49_OFFSET, CR196,	CONDREG},
{CRF49_OFFSET, CR197,	CONDREG},
{CRF49_OFFSET, CR198,	CONDREG},
{CRF49_OFFSET, CR199,	CONDREG},
{CRF50_OFFSET, CR200,	CONDREG},
{CRF50_OFFSET, CR201,	CONDREG},
{CRF50_OFFSET, CR202,	CONDREG},
{CRF50_OFFSET, CR203,	CONDREG},
{CRF51_OFFSET, CR204,	CONDREG},
{CRF51_OFFSET, CR205,	CONDREG},
{CRF51_OFFSET, CR206,	CONDREG},
{CRF51_OFFSET, CR207,	CONDREG},
{CRF52_OFFSET, CR208,	CONDREG},
{CRF52_OFFSET, CR209,	CONDREG},
{CRF52_OFFSET, CR210,	CONDREG},
{CRF52_OFFSET, CR211,	CONDREG},
{CRF53_OFFSET, CR212,	CONDREG},
{CRF53_OFFSET, CR213,	CONDREG},
{CRF53_OFFSET, CR214,	CONDREG},
{CRF53_OFFSET, CR215,	CONDREG},
{CRF54_OFFSET, CR216,	CONDREG},
{CRF54_OFFSET, CR217,	CONDREG},
{CRF54_OFFSET, CR218,	CONDREG},
{CRF54_OFFSET, CR219,	CONDREG},
{CRF55_OFFSET, CR220,	CONDREG},
{CRF55_OFFSET, CR221,	CONDREG},
{CRF55_OFFSET, CR222,	CONDREG},
{CRF55_OFFSET, CR223,	CONDREG},
{CRF56_OFFSET, CR224,	CONDREG},
{CRF56_OFFSET, CR225,	CONDREG},
{CRF56_OFFSET, CR226,	CONDREG},
{CRF56_OFFSET, CR227,	CONDREG},
{CRF57_OFFSET, CR228,	CONDREG},
{CRF57_OFFSET, CR229,	CONDREG},
{CRF57_OFFSET, CR230,	CONDREG},
{CRF57_OFFSET, CR231,	CONDREG},
{CRF58_OFFSET, CR232,	CONDREG},
{CRF58_OFFSET, CR233,	CONDREG},
{CRF58_OFFSET, CR234,	CONDREG},
{CRF58_OFFSET, CR235,	CONDREG},
{CRF59_OFFSET, CR236,	CONDREG},
{CRF59_OFFSET, CR237,	CONDREG},
{CRF59_OFFSET, CR238,	CONDREG},
{CRF59_OFFSET, CR239,	CONDREG},
{CRF60_OFFSET, CR240,	CONDREG},
{CRF60_OFFSET, CR241,	CONDREG},
{CRF60_OFFSET, CR242,	CONDREG},
{CRF60_OFFSET, CR243,	CONDREG},
{CRF61_OFFSET, CR244,	CONDREG},
{CRF61_OFFSET, CR245,	CONDREG},
{CRF61_OFFSET, CR246,	CONDREG},
{CRF61_OFFSET, CR247,	CONDREG},
{CRF62_OFFSET, CR248,	CONDREG},
{CRF62_OFFSET, CR249,	CONDREG},
{CRF62_OFFSET, CR250,	CONDREG},
{CRF62_OFFSET, CR251,	CONDREG},
{CRF63_OFFSET, CR252,	CONDREG},
{CRF63_OFFSET, CR253,	CONDREG},
{CRF63_OFFSET, CR254,	CONDREG},
{CRF63_OFFSET, CR255,	CONDREG},

/* Other registers */
{LR_OFFSET, LR,		LINKREG}
};

RESRC_MAP real_resrc;

static int num_dup_resrc = sizeof (_resrc_map) / sizeof (_resrc_map[0]);

/************************************************************************
 *									*
 *				sort_resources				*
 *				--------------				*
 *									*
 ************************************************************************/

sort_resources ()
{
   int i;

   /* Initialize all state to correspond to actual resources */
   for (i = 0; i < NUM_MACHINE_REGS; i++)
      resrc_map[i] = &real_resrc;

   /* For state given in table, keep independent copy of resources */
   for (i = 0; i < num_dup_resrc; i++)
      resrc_map[_resrc_map[i].orig] = &_resrc_map[i];
}
