//*********************************************************************************//
// Module name : csfr.h                                                            //
// Description : pmc_q32s core sfr define                                          //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __PMC_Q32S_CSFR__
#define __PMC_Q32S_CSFR__

#define __RW         volatile       // read write
#define __RO         volatile const // only read
#define __WO         volatile       // only write

#define __u8         unsigned int   // u8  to u32 special for struct
#define __u16        unsigned int   // u16 to u32 special for struct
#define __u32        unsigned int

// #define CPU_CORE_NUM         1
#define pmc_csfr_base        0x7e30000

//*********************************************************************************
//
// pcore_sfr
//
//*********************************************************************************

//............. 0x0000 - 0x00ff............
typedef struct {
    __RW __u32 CON0;
    __RO __u32 SWI_PRP;
    __WO __u32 SWI_SET;
    __WO __u32 SWI_CLR;
} JL_PMC_CMNG_TypeDef;

#define JL_PMC_CMNG_BASE                   (pmc_csfr_base + map_adr(0x00, 0x00))
#define JL_PMC_CMNG                        ((JL_PMC_CMNG_TypeDef *)JL_PMC_CMNG_BASE)

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
} JL_PMC_CEMU_TypeDef;

#define JL_PMC_CEMU_BASE                   (pmc_csfr_base + map_adr(0x01, 0x00))
#define JL_PMC_CEMU                        ((JL_PMC_CEMU_TypeDef *)JL_PMC_CEMU_BASE)

//............. 0x0300 - 0x03ff............
#define MPU_INV                        (1<<31)
#define MPU_PWEN                       (1<<16)
#define MPU_PREN                       (1<<8)
#define MPU_PEN                        (MPU_PWEN | MPU_PREN)
#define MPU_XEN                        (1<<2)
#define MPU_WEN                        (1<<1)
#define MPU_REN                        (1<<0)

#define PMC_MPU_IDx_cfg(n, id)             (id<<(n*8))
#define PMC_MPU_IDx_pen(n, pr, pw)         ((pr<<(9+n)) | (pw<<(17+n)))

typedef struct {
    __RW __u32 CON[15]; // 0-4 used in br33
    __RO __u32 REV0;
    __RW __u32 CID[15]; // 0-4 used in br33
    __RO __u32 REV1;
    __RW __u32 BEG[15]; // 0-4 used in br33
    __RO __u32 REV2;
    __RW __u32 END[15]; // 0-4 used in br33
    __RW __u32 WREN;
} JL_PMC_MPU_TypeDef;

#define JL_PMC_MPU_BASE                   (pmc_csfr_base + map_adr(0x02, 0x00))
#define JL_PMC_MPU                        ((JL_PMC_MPU_TypeDef *)JL_PMC_MPU_BASE)

//............. 0x0300 - 0x03ff............
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
} JL_PMC_MBIST_TypeDef;

#define JL_PMC_MBIST_BASE                 (pmc_csfr_base + map_adr(0x03, 0x00))
#define JL_PMC_MBIST                      ((JL_PMC_MBIST_TypeDef *)JL_PMC_MBIST_BASE)

//............. 0x0400 - 0x04ff............
typedef struct {
    __RW __u32 MCFG[63];
    __RW __u32 CKSEL;
} JL_PMC_MEMCFG_TypeDef;

#define JL_PMC_MEMCFG_BASE                (pmc_csfr_base + map_adr(0x04, 0x00))
#define JL_PMC_MEMCFG                     ((JL_PMC_MEMCFG_TypeDef *)JL_PMC_MEMCFG_BASE)

//............. 0x0400 - 0x04ff............
typedef struct {
    __RW __u32 CON;
    __RW __u32 TLB1_BEG;
    __RW __u32 TLB1_END;
} JL_PMC_MMU_TypeDef;

#define JL_PMC_MMU_BASE                   (pmc_csfr_base + map_adr(0x05, 0x00))
#define JL_PMC_MMU                        ((JL_PMC_MMU_TypeDef *)JL_PMC_MMU_BASE)

typedef struct {
    short page: 14;
    short vld: 1;
} JL_PMC_MMU_TLB1_TypeDef;

#define JL_PMC_MMU_TLB1                   ((JL_PMC_MMU_TLB1_TypeDef *)(JL_PMC_MMU->TLB1_BEG))

//*********************************************************************************
//
// pmc_q32s_sfr
//
//*********************************************************************************

//---------------------------------------------//
// pmc_q32s define
//---------------------------------------------//

#define pmc_q32s_sfr_offset  0x000400
#define pmc_q32s_sfr_base    (pmc_csfr_base + 0x3000)

#define pmc_q32s_cpu_base    (pmc_q32s_sfr_base + 0x0000)

#define pmc_q32s(n)          ((JL_TypeDef_pmc_q32s     *)(pmc_q32s_sfr_base + pmc_q32s_sfr_offset*n))

//---------------------------------------------//
// pmc_q32s core sfr
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
    /*    */  __RO __u32 REV_30_26[0x30 - 0x26 - 1];

    /* 30 */  __RW __u32 PMU_CON0;
    /*    */  __RO __u32 REV_3b_30[0x3b - 0x30 - 1];
    /* 3b */  __RW __u8  TTMR_CON;
    /* 3c */  __RW __u32 TTMR_CNT;
    /* 3d */  __RW __u32 TTMR_PRD;
    /*    */  __RO __u32 REV_40_3d[0x40 - 0x3d - 1];

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
    /*    */  __RO __u32 REV_70_6c[0x70 - 0x6c - 1];

    /* 70 */  __RW __u32 ETM_CON;
    /* 71 */  __RO __u32 ETM_PC0;
    /* 72 */  __RO __u32 ETM_PC1;
    /* 73 */  __RO __u32 ETM_PC2;
    /* 74 */  __RO __u32 ETM_PC3;
    /* 75 */  __RW __u32 WP0_ADRH;
    /* 76 */  __RW __u32 WP0_ADRL;
    /* 77 */  __RW __u32 WP0_DATH;
    /* 78 */  __RW __u32 WP0_DATL;
    /* 79 */  __RW __u32 WP0_PC;
    /*    */  __RO __u32 REV_80_79[0x80 - 0x79 - 1];

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
} JL_TypeDef_pmc_q32s;


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

