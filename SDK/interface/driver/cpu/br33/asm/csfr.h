//*********************************************************************************//
// Module name : csfr.h                                                            //
// Description : q32DSP core sfr define                                            //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __Q32DSP_CSFR__
#define __Q32DSP_CSFR__

#define __RW         volatile       // read write
#define __RO         volatile const // only read
#define __WO         volatile       // only write

#define __u8         unsigned int   // u8  to u32 special for struct
#define __u16        unsigned int   // u16 to u32 special for struct
#define __u32        unsigned int

#define csfr_base    0x7cf0000

//*********************************************************************************
//
// hcore_sfr
//
//*********************************************************************************

//............. 0x0000 - 0x00ff............
typedef struct {
    __RW __u32 CON0;
} JL_CMNG_TypeDef;

#define JL_CMNG_BASE                   (csfr_base + map_adr(0x00, 0x00))
#define JL_CMNG                        ((JL_CMNG_TypeDef *)JL_CMNG_BASE)

//............. 0x0100 - 0x01ff............
//typedef struct {
//    __RW __u32 CON;
//    __RW __u32 KEY;
//} JL_SDTAP_TypeDef;

//#define JL_SDTAP_BASE                  (csfr_base + map_adr(0x01, 0x00))
//#define JL_SDTAP                       ((JL_SDTAP_TypeDef *)JL_SDTAP_BASE)

//............. 0x0200 - 0x02ff............
typedef struct {
    __RW __u32 WREN;
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
    __RW __u32 MSG0;
    __RW __u32 MSG1;
    __RW __u32 MSG2;
    __RW __u32 MSG3;
    __RO __u32 ID;
} JL_CEMU_TypeDef;

#define JL_CEMU_BASE                   (csfr_base + map_adr(0x02, 0x00))
#define JL_CEMU                        ((JL_CEMU_TypeDef *)JL_CEMU_BASE)

//............. 0x0300 - 0x03ff............

#define MPU_INV                        (1<<31)
#define MPU_PWEN                       (1<<16)
#define MPU_PREN                       (1<<8)
#define MPU_PEN                        (MPU_PWEN | MPU_PREN)
#define MPU_XEN                        (1<<2)
#define MPU_WEN                        (1<<1)
#define MPU_REN                        (1<<0)

#define MPU_IDx_cfg(n, id)             (id<<(n*8))
#define MPU_IDx_pen(n, pr, pw)         ((pr<<(9+n)) | (pw<<(17+n)))

typedef struct {
    __RW __u32 CON[15]; // 0-7 used in br33
    __RO __u32 REV0;
    __RW __u32 CID[15]; // 0-7 used in br33
    __RO __u32 REV1;
    __RW __u32 BEG[15]; // 0-7 used in br33
    __RO __u32 REV2;
    __RW __u32 END[15]; // 0-7 used in br33
    __RW __u32 WREN;
} JL_MPU_TypeDef;

#define JL_MPU_BASE                   (csfr_base + map_adr(0x03, 0x00))
#define JL_MPU                        ((JL_MPU_TypeDef *)JL_MPU_BASE)

//............. 0x0400 - 0x04ff............

typedef struct {
    __RW __u32 CON;
    __RW __u32 TLB1_BEG;
    __RW __u32 TLB1_END;
} JL_MMU_TypeDef;

#define JL_MMU_BASE                   (csfr_base + map_adr(0x04, 0x00))
#define JL_MMU                        ((JL_MMU_TypeDef *)JL_MMU_BASE)

typedef struct {
    short page: 14;
    short vld: 1;
} JL_MMU_TLB1_TypeDef;

#define JL_MMU_TLB1                   ((JL_MMU_TLB1_TypeDef *)(JL_MMU->TLB1_BEG))

//............. 0x0500 - 0x05ff............
//#define JL_TypeDef_L1P                JL_TypeDef_q32DSP_ICU
#define JL_TypeDef_L1P                JL_TypeDef_q32DSP_DCU
#define JL_L1P_BASE                   (csfr_base + map_adr(0x05, 0x00))
#define JL_L1P                        ((JL_TypeDef_L1P *)JL_L1P_BASE)

//............. 0x0600 - 0x06ff............
#define JL_TypeDef_L2I                JL_TypeDef_q32DSP_ICU
#define JL_L2I_BASE                   (csfr_base + map_adr(0x06, 0x00))
#define JL_L2I                        ((JL_TypeDef_L2I *)JL_L2I_BASE)

//............. 0x0700 - 0x07ff............
#define JL_TypeDef_L2D                JL_TypeDef_q32DSP_DCU
#define JL_L2D_BASE                   (csfr_base + map_adr(0x07, 0x00))
#define JL_L2D                        ((JL_TypeDef_L2D *)JL_L2D_BASE)

//............. 0x0800 - 0x08ff............
typedef struct {
    __RO __u32 CHIP_ID;
    __RO __u32 CHIP_VER;
    __RO __u32 REV0;
    __RO __u32 REV1;
    __RO __u32 KEY_MODE;
} JL_SYSTEM_TypeDef;

#define JL_SYSTEM_BASE                (csfr_base + map_adr(0x08, 0x00))
#define JL_SYSTEM                     ((JL_SYSTEM_TypeDef   *)JL_SYSTEM_BASE)

//............. 0x0900 - 0x09ff............
typedef struct {
    __RW __u32 CON;
    __RW __u32 SEL;
    __RW __u32 BEG;
    __RW __u32 END;
    __RW __u32 DAT_VLD0;
    __RW __u32 DAT_VLD1;
    __RW __u32 DAT_VLD2;
    __RW __u32 DAT_VLD3;
    __RO __u32 ROM_CRC;
} JL_MBIST_TypeDef;

#define JL_MBIST_BASE                 (csfr_base + map_adr(0x09, 0x00))
#define JL_MBIST                      ((JL_MBIST_TypeDef *)JL_MBIST_BASE)

//............. 0x0a00 - 0x0aff............
typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 IADR;
    __RW __u32 OADR;
} JL_FFT_TypeDef;

#define JL_FFT_BASE                   (csfr_base + map_adr(0x0a, 0x00))
#define JL_FFT                        ((JL_FFT_TypeDef    *)JL_FFT_BASE)

//............. 0x0b00 - 0x0bff............
//typedef struct {
///* 00 */  __RW __u32 CON0;
///* 01 */  __RW __u32 LPEN_CON;
///* 02 */  __RW __u32 LSEN_CON;
///* 03 */  __RO __u32 LS_PND;
///* 04 */  __RO __u32 CE_PND;
///* 05 */  __RW __u32 LS_PRD_32K;
///* 06 */  __RW __u32 CE_PRD_32K;
///* 07 */  __RW __u32 LS_PRD_64K;
///* 08 */  __RW __u32 CE_PRD_64K;
///* 09 */  __RW __u32 LS_PRD_ROM;
///* 0a */  __RW __u32 CE_PRD_ROM;
//} JL_ATOLP_TypeDef;

//#define JL_ATOLP_BASE                 (csfr_base + map_adr(0x0b, 0x00))
//#define JL_ATOLP                      ((JL_ATOLP_TypeDef  *)JL_ATOLP_BASE)

//............. 0x0c00 - 0x0cff............
typedef struct {
    __RO __u32 UID0;
    __RO __u32 UID1;
    __RO __u32 UID2;
    __RO __u32 UID3;
    __RO __u32 UID4;
} JL_UUID_TypeDef;

#define JL_UUID_BASE                 (csfr_base + map_adr(0x0c, 0x01))
#define JL_UUID                      ((JL_UUID_TypeDef   *)JL_UUID_BASE)

//............. 0x0d00 - 0x0dff............
typedef struct {
    __RW __u32 MCFG[63];
    __RW __u32 CKSEL;
} JL_MEMCFG_TypeDef;

#define JL_MEMCFG_BASE                (csfr_base + map_adr(0x0d, 0x00))
#define JL_MEMCFG                     ((JL_MEMCFG_TypeDef *)JL_MEMCFG_BASE)

//............. 0x0e00 - 0x0fff............
typedef struct {
    __WO __u32 ANPU_RST_SET0;
    __RW __u32 ANPU_CON0;
    __WO __u32 ANPU_RST_SET1;
    __RW __u32 CNV_ACC_CON0;
    __RW __u32 CNV_ACC_CON1;
    __RW __u32 CNV_ACC_CFG0;
    __RW __u32 CNV_ACC_CFG1;
    __RW __u32 CNV_ACC_CFG2;
    __RW __u32 CNV_ACC_CFG3;
    __RW __u32 CNV_ACC_CFG4;
    __RW __u32 CNV_ACC_CFG5;
    __RW __u32 CNV_ACC_NUM;
    __RW __u32 CNV_ACC_SET;
    __RW __u32 CNV_RCOEF_CON0;
    __RW __u32 CNV_RCOEF_CFG0;
    __RW __u32 CNV_RCOEF_CFG1;
    __RW __u32 CNV_RCOEF_CFG2;
    __RW __u32 CNV_RCOEF_CFG3;
    __RW __u32 CNV_RCOEF_CFG4;
    __RW __u32 CNV_RDAT_CON0;
    __RW __u32 CNV_RDAT_CFG0;
    __RW __u32 CNV_RDAT_CFG1;
    __RW __u32 CNV_RDAT_CFG2;
    __RW __u32 CNV_RDAT_CFG3;
    __RW __u32 CNV_RDAT_CFG4;
    __RW __u32 CNV_RDAT_CFG5;
    __RW __u32 CNV_RDAT_CFG6;
    __RW __u32 CNV_RDAT_CFG7;
    __RW __u32 CNV_RDAT_CFG8;
    __RW __u32 CNV_RDAT_CFG9;
    __RW __u32 CNV_RDAT_CFG10;
    __WO __u32 CNV_RDAT_IADR0;
    __WO __u32 CNV_RDAT_IADR1;
    __WO __u32 CNV_RDAT_IADR2;
    __WO __u32 CNV_RDAT_IADR3;
    __WO __u32 CNV_RDAT_W_OS0;
    __WO __u32 CNV_RDAT_W_OS1;
    __WO __u32 CNV_RDAT_W_OS2;
    __WO __u32 CNV_RDAT_W_OS3;
    __RW __u32 CNV_RDAT_W_CFG0;
    __RW __u32 CNV_RDAT_W_CFG1;
    __RW __u32 CNV_RDAT_V_CFG0;
    __RW __u32 CNV_RDAT_V_CFG1;
    __RW __u32 CNV_RDAT_P_CFG0;
    __RW __u32 CNV_RDAT_P_CFG1;
    __RW __u32 CNV_ODAT_CON0;
    __RW __u32 SDP_CON0;
    __RW __u32 SDP_RDAT_CON0;
    __RW __u32 SDP_RDAT_CFG0;
    __RW __u32 SDP_RDAT_CFG1;
    __RW __u32 SDP_RDAT_CFG2;
    __RW __u32 SDP_RDAT_CFG3;
    __RW __u32 SDP_RDAT_CFG4;
    __RW __u32 SDP_RDAT_CFG5;
    __RW __u32 SDP_RDAT_CFG6;
    __RW __u32 SDP_RDAT_CFG7;
    __RW __u32 SDP_RDAT_CFG8;
    __RW __u32 SDP_RDAT_CFG9;
    __RW __u32 SDP_RDAT_CFG10;
    __RW __u32 SDP_RDAT_CFG11;
    __RW __u32 SDP_ODAT_CON0;
    __RW __u32 SDP_ODAT_CFG0;
    __RW __u32 SDP_ODAT_CFG1;
    __RW __u32 SDP_ODAT_CFG2;
    __RW __u32 SDP_ODAT_CFG3;
    __RW __u32 SDP_ODAT_CFG4;
    __RW __u32 SDP_BS_CFG0;
    __RW __u32 SDP_BS_CFG1;
    __RW __u32 SDP_BS_CFG2;
    __RW __u32 SDP_BN_CFG0;
    __RW __u32 SDP_BN_CFG1;
    __RW __u32 SDP_BN_CFG2;
    __RW __u32 SDP_ACT_CFG0;
    __RW __u32 SDP_ACT_CFG1;
    __RW __u32 SDP_ACT_CFG2;
    __RW __u32 SDP_ACT_CFG3;
    __RW __u32 SDP_ACT_CFG4;
    __RW __u32 SDP_ACT_CFG5;
    __RW __u32 SDP_ACT_CFG6;
    __RW __u32 SDP_ACT_CFG7;
    __RW __u32 SDP_ACT_CFG8;
    __RW __u32 SDP_ACT_CFG9;
    __RW __u32 SDP_ACT_CFG10;
    __RW __u32 SDP_ACT_CFG11;
    __RW __u32 SDP_QT0_CFG0;
    __RW __u32 SDP_QT0_CFG1;
    __RW __u32 SDP_QT0_CFG2;
    __RW __u32 SDP_QT0_CFG3;
    __RW __u32 SDP_QT1_CFG0;
    __RW __u32 SDP_QT1_CFG1;
    __RW __u32 SDP_QT1_CFG2;
    __RW __u32 SDP_QT1_CFG3;
    __RW __u32 ANPU_KST_SET;
    __WO __u32 ANPU_PND_CLR;
    __RO __u32 ANPU_PND_CON;
    __RO __u32 ANPU_RST_CON;
} JL_ANPU_TypeDef;

#define JL_ANPU_BASE                 (csfr_base + map_adr(0x0e, 0x00))
#define JL_ANPU                      ((JL_ANPU_TypeDef  *)JL_ANPU_BASE)

//*********************************************************************************
//
// q32DSP_sfr
//
//*********************************************************************************

//---------------------------------------------//
// q32DSP define
//---------------------------------------------//

#define q32DSP_sfr_offset   0x000800
#define q32DSP_sfr_base    (csfr_base + 0xe000)

#define q32DSP_cpu_base    (q32DSP_sfr_base + 0x0000)
#define q32DSP_icu_base    (q32DSP_sfr_base + 0x0400)

#define q32DSP(n)          ((JL_TypeDef_q32DSP     *)(q32DSP_sfr_base + q32DSP_sfr_offset*n))
#define q32DSP_icu(n)      ((JL_TypeDef_q32DSP_ICU *)(q32DSP_icu_base + q32DSP_sfr_offset*n))

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
    /*    */  __RO __u32 REV22[0x26 - 0x23 - 1];
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
    /* 8c */  __RW __u32 LIM_PC3_H;
    /* 8d */  __RW __u32 LIM_PC3_L;
    /* 8e */  __RW __u32 LIM_PC4_H;
    /* 8f */  __RW __u32 LIM_PC4_L;

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
} JL_TypeDef_q32DSP;

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
} JL_TypeDef_q32DSP_ICU;

//---------------------------------------------//
// q32DSP dcache sfr
//---------------------------------------------//

typedef struct {
    __RW __u32 CON;
    __RW __u32 EMU_CON;
    __RW __u32 EMU_MSG;
    __RW __u32 EMU_ID;
    __RW __u32 CNT_WACK;
    __RW __u32 CNT_WNAK;
    __RW __u32 CNT_RACK;
    __RW __u32 CNT_RNAK;
    __RW __u32 CMD_CON[4];
    __RW __u32 CMD_BEG[4];
    __RW __u32 CMD_END[4];
    __RO __u32 REV1[0x20 - 0x13 - 1];
    __WO __u32 CMO[32];
} JL_TypeDef_q32DSP_DCU;

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

