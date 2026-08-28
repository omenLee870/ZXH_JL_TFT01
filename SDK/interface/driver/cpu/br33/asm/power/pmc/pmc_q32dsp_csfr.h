//*********************************************************************************//
// Module name : csfr.h                                                            //
// Description : q32DSP core sfr define                                            //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __PMC_Q32DSP_CSFR__
#define __PMC_Q32DSP_CSFR__

#define __RW         volatile       // read write
#define __RO         volatile const // only read
#define __WO         volatile       // only write

#define __u8         unsigned int   // u8  to u32 special for struct
#define __u16        unsigned int   // u16 to u32 special for struct
#define __u32        unsigned int

#define pmc_csfr_base    0x7e30000

//*********************************************************************************
//
// pcore_sfr
//
//*********************************************************************************


//*********************************************************************************
//
// q32DSP_sfr
//
//*********************************************************************************

//---------------------------------------------//
// q32DSP define
//---------------------------------------------//

#define pmc_q32DSP_sfr_offset   0x000800
#define pmc_q32DSP_sfr_base    (pmc_csfr_base + 0x2800)

#define pmc_q32DSP_cpu_base    (pmc_q32DSP_sfr_base + 0x0000)
#define pmc_q32DSP_icu_base    (pmc_q32DSP_sfr_base + 0x0400)

#define pmc_q32DSP(n)          ((JL_TypeDef_pmc_q32DSP     *)(pmc_q32DSP_sfr_base + pmc_q32DSP_sfr_offset*n))
#define pmc_q32DSP_icu(n)      ((JL_TypeDef_pmc_q32DSP_ICU *)(pmc_q32DSP_icu_base + pmc_q32DSP_sfr_offset*n))

//---------------------------------------------//
// q32DSP core sfr
//---------------------------------------------//

typedef struct {
    /* 00 */  __RO __u32 DR00;
    /* 01 */  __RO __u32 DR01;
    /* 02 */  __RO __u32 DR02;
    /* 03 */  __RO __u32 DR03;
    /* 04 */  __RO __u32 DR04;
    /* 05 */  __RO __u32 DR05;
    /* 06 */  __RO __u32 DR06;
    /* 07 */  __RO __u32 DR07;
    /* 08 */  __RO __u32 DR08;
    /* 09 */  __RO __u32 DR09;
    /* 0a */  __RO __u32 DR10;
    /* 0b */  __RO __u32 DR11;
    /* 0c */  __RO __u32 DR12;
    /* 0d */  __RO __u32 DR13;
    /* 0e */  __RO __u32 DR14;
    /* 0f */  __RO __u32 DR15;

    /* 10 */  __RO __u32 RETI;
    /* 11 */  __RO __u32 RETE;
    /* 12 */  __RO __u32 RETX;
    /* 13 */  __RO __u32 RETS;
    /* 14 */  __RO __u32 SR04;
    /* 15 */  __RO __u32 PSR;
    /* 16 */  __RO __u32 CNUM;
    /* 17 */  __RO __u32 SR07;
    /* 18 */  __RO __u32 SR08;
    /* 19 */  __RO __u32 SR09;
    /* 1a */  __RO __u32 SR10;
    /* 1b */  __RO __u32 ICFG;
    /* 1c */  __RO __u32 USP;
    /* 1d */  __RO __u32 SSP;
    /* 1e */  __RO __u32 SP;
    /* 1f */  __RO __u32 PCRS;

    /* 20 */  __RW __u32 BPCON;
    /* 21 */  __RW __u32 BSP;
    /* 22 */  __RW __u32 BP0;
    /* 23 */  __RW __u32 BP1;
    /* 24 */  __RW __u32 BP2;
    /* 25 */  __RW __u32 BP3;
    /* 26 */  __WO __u32 CMD_PAUSE;
    /*    */  __RO __u32 REV2a[0x30 - 0x26 - 1];

    /* 30 */  __RW __u32 PMU_CON0;
    /* 31 */  __RW __u32 PMU_CON1;
    /* 32 */  __RO __u32 RST_ADDR;
    /*    */  __RO __u32 REV32[0x3b - 0x32 - 1];
    /* 3b */  __RW __u8  TTMR_CON;
    /* 3c */  __RW __u32 TTMR_CNT;
    /* 3d */  __RW __u32 TTMR_PRD;
    /*    */  __RO __u32 REV3d[0x40 - 0x3d - 1];

    /* 40 */  __RW __u32 ICFG00;
    /* 41 */  __RW __u32 ICFG01;
    /* 42 */  __RW __u32 ICFG02;
    /* 43 */  __RW __u32 ICFG03;
    /* 44 */  __RW __u32 ICFG04;
    /* 45 */  __RW __u32 ICFG05;
    /* 46 */  __RW __u32 ICFG06;
    /* 47 */  __RW __u32 ICFG07;
    /* 48 */  __RW __u32 ICFG08;
    /* 49 */  __RW __u32 ICFG09;
    /* 4a */  __RW __u32 ICFG10;
    /* 4b */  __RW __u32 ICFG11;
    /* 4c */  __RW __u32 ICFG12;
    /* 4d */  __RW __u32 ICFG13;
    /* 4e */  __RW __u32 ICFG14;
    /* 4f */  __RW __u32 ICFG15;

    /* 50 */  __RW __u32 ICFG16;
    /* 51 */  __RW __u32 ICFG17;
    /* 52 */  __RW __u32 ICFG18;
    /* 53 */  __RW __u32 ICFG19;
    /* 54 */  __RW __u32 ICFG20;
    /* 55 */  __RW __u32 ICFG21;
    /* 56 */  __RW __u32 ICFG22;
    /* 57 */  __RW __u32 ICFG23;
    /* 58 */  __RW __u32 ICFG24;
    /* 59 */  __RW __u32 ICFG25;
    /* 5a */  __RW __u32 ICFG26;
    /* 5b */  __RW __u32 ICFG27;
    /* 5c */  __RW __u32 ICFG28;
    /* 5d */  __RW __u32 ICFG29;
    /* 5e */  __RW __u32 ICFG30;
    /* 5f */  __RW __u32 ICFG31;

    /* 60 */  __RO __u32 IPND0;
    /* 61 */  __RO __u32 IPND1;
    /* 62 */  __RO __u32 IPND2;
    /* 63 */  __RO __u32 IPND3;
    /* 64 */  __RO __u32 IPND4;
    /* 65 */  __RO __u32 IPND5;
    /* 66 */  __RO __u32 IPND6;
    /* 67 */  __RO __u32 IPND7;
    /* 68 */  __WO __u32 ILAT_SET;
    /* 69 */  __WO __u32 ILAT_CLR;
    /* 6a */  __RW __u32 IPMASK;
    /* 6b */  __RW __u32 GIEMASK;
    /* 6c */  __RW __u32 IWKUP_NUM;
    /*    */  __RO __u32 REV6c[0x70 - 0x6c - 1];

    /* 70 */  __RW __u32 ETM_CON;
    /* 71 */  __RO __u32 ETM_PC0;
    /* 72 */  __RO __u32 ETM_PC1;
    /* 73 */  __RO __u32 ETM_PC2;
    /* 74 */  __RO __u32 ETM_PC3;
    /* 75 */  __RW __u32 WP0_ADRH;
    /* 76 */  __RW __u32 WP0_ADRL;
    /* 77 */  __RW __u32 WP0_DATH;
    /* 78 */  __RW __u32 WP0_DATL;
    /* 79 */  __RO __u32 WP0_PC;
    /* 7a */  __RO __u32 WP0_AMSG;
    /* 7b */  __RO __u32 ETM_PC4;
    /* 7c */  __RO __u32 ETM_PC5;
    /*    */  __RO __u32 REV7d[0x80 - 0x7c - 1];

    /* 80 */  __RW __u32 EMU_CON;
    /* 81 */  __RW __u32 EMU_MSG;
    /* 82 */  __RW __u32 EMU_SSP_H;
    /* 83 */  __RW __u32 EMU_SSP_L;
    /* 84 */  __RW __u32 EMU_USP_H;
    /* 85 */  __RW __u32 EMU_USP_L;
    /* 86 */  __RW __u32 LIM_PC0_H;
    /* 87 */  __RW __u32 LIM_PC0_L;
    /* 88 */  __RW __u32 LIM_PC1_H;
    /* 89 */  __RW __u32 LIM_PC1_L;
    /* 8a */  __RW __u32 LIM_PC2_H;
    /* 8b */  __RW __u32 LIM_PC2_L;
    /*    */  __RO __u32 REV8c[0x90 - 0x8b - 1];

    /* 90 */  __RW __u32 ESU_CON;
    /* 91 */  __RO __u32 CNT_CHIT;
    /* 92 */  __RO __u32 CNT_CMIS;
    /* 93 */  __RO __u32 CNT_FILL;
    /* 94 */  __RO __u32 CNT_IHIT;
    /* 95 */  __RO __u32 CNT_IMIS;
    /* 96 */  __RO __u32 CNT_RHIT;
    /* 97 */  __RO __u32 CNT_RMIS;
    /* 98 */  __RO __u32 CNT_WHIT;
    /* 99 */  __RO __u32 CNT_WMIS;
    /*    */  __RO __u32 REVa0[0xa0 - 0x99 - 1];

    /* a0 */  __RW __u32 BANK_CON;
    /* a1 */  __RW __u32 BANK_PABEG;
    /* a2 */  __RW __u32 BANK_VABEG;
    /* a3 */  __RW __u32 BANK_VAEND;
    /* a4 */  __RW __u32 BANK_VBASE;
    /*    */  __RO __u32 REVb0[0xb0 - 0xa4 - 1];

    /* b0 */  __RW __u32 IMMU0_CON;
    /* b1 */  __RW __u32 IMMU0_VABEG;
    /* b2 */  __RW __u32 IMMU0_VAEND;
    /* b3 */  __RW __u32 IMMU0_PABEG;
    /* b4 */  __RW __u32 IMMU0_TBBEG;
    /* b5 */  __RW __u32 DMMU0_CON;
    /* b6 */  __RW __u32 DMMU0_VABEG;
    /* b7 */  __RW __u32 DMMU0_VAEND;
    /* b8 */  __RW __u32 DMMU0_PABEG;
    /* b9 */  __RW __u32 DMMU0_TBBEG;
    /*    */  __RO __u32 REVc0[0xc0 - 0xb9 - 1];

    /* c0 */  __RW __u32 IMMU1_CON;
    /* c1 */  __RW __u32 IMMU1_VABEG;
    /* c2 */  __RW __u32 IMMU1_VAEND;
    /* c3 */  __RW __u32 IMMU1_PABEG;
    /* c4 */  __RW __u32 IMMU1_TBBEG;
    /* c5 */  __RW __u32 DMMU1_CON;
    /* c6 */  __RW __u32 DMMU1_VABEG;
    /* c7 */  __RW __u32 DMMU1_VAEND;
    /* c8 */  __RW __u32 DMMU1_PABEG;
    /* c9 */  __RW __u32 DMMU1_TBBEG;
} JL_TypeDef_pmc_q32DSP;

//---------------------------------------------//
// q32DSP icache sfr
//---------------------------------------------//

typedef struct {
    __RW __u32 CON;
    __RW __u32 EMU_CON;
    __RW __u32 EMU_MSG;
    __RW __u32 EMU_ID;
    __RW __u32 CMD_CON;
    __RW __u32 CMD_BEG;
    __RW __u32 CMD_END;
    __RW __u32 CNT_RACK;
    __RW __u32 CNT_RNAK;
} JL_TypeDef_pmc_q32DSP_ICU;


#undef __RW
#undef __RO
#undef __WO

#undef __u8
#undef __u16
#undef __u32

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
#endif

