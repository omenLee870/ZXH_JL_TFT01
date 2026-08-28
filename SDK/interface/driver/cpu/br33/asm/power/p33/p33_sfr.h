#ifndef __P33_SFR__
#define __P33_SFR__

#ifdef PMU_SYSTEM
#define P33_ACCESS(x) (*(volatile u32 *)(0x7e20000 + 0x0000 + (x)*4))
#else
#define P33_ACCESS(x) (*(volatile u32 *)(0x7e20000 + 0x0000 + (x)*4))
#endif


//==============================================================================


//............. 0X0000 - 0X000F............         FOR RESET0
#define P3_RST_FLAG                     P33_ACCESS(0x000)
#define P3_RST_CON0                     P33_ACCESS(0x001)
#define P3_RST_CON1                     P33_ACCESS(0x002)
#define P3_RST_CON2                     P33_ACCESS(0x003)
#define P3_RST_CON3                     P33_ACCESS(0x004)
//#define P3_RST_CON4                     P33_ACCESS(0x005)
#define P3_RST_SRC0                     P33_ACCESS(0x006)
#define P3_RST_SRC1                     P33_ACCESS(0x007)

//............. 0X0010 - 0X001F............         FOR RESET1
#define P3_VLVD_CON0                    P33_ACCESS(0x010)
#define P3_VLVD_CON1                    P33_ACCESS(0x011)
#define P3_VLVD_FLT                     P33_ACCESS(0x012)
#define P3_PR_PWR                       P33_ACCESS(0x013)
#define P3_WDT_CON0                     P33_ACCESS(0x014)
#define P3_WDT_CON1                     P33_ACCESS(0x015)

#define P3_PINR_CON                     P33_ACCESS(0x018)
#define P3_PINR_CON1                    P33_ACCESS(0x019)
#define P3_PINR_SAFE                    P33_ACCESS(0x01a)
#define P3_PINR_SAFE1                   P33_ACCESS(0x01b)
#define P3_PINR_PND1                    P33_ACCESS(0x01c)

//............. 0X0020 - 0X002F............         FOR CLOCK0
#define P3_CLK_CON0                     P33_ACCESS(0x020)
#define P3_CLK_CON1                     P33_ACCESS(0x021)
#define P3_DCDC_CLK_CON0                P33_ACCESS(0x022)
#define P3_DCDC_CLK_CON1                P33_ACCESS(0x023)
//#define P3_CLK_CON2                     P33_ACCESS(0x022)
//#define P3_CLK_CON3                     P33_ACCESS(0x023)
#define P3_VLD_KEEP                     P33_ACCESS(0x024)
#define P3_LRC_CON0                     P33_ACCESS(0x025)
#define P3_LRC_CON1                     P33_ACCESS(0x026)
#define P3_STRC32K_CON0                 P33_ACCESS(0x027)
#define P3_LRC24M_CON0                  P33_ACCESS(0x028)
#define P3_LRC24M_CON1                  P33_ACCESS(0x029)
#define P3_LRC24M_CON2                  P33_ACCESS(0x02a)
#define P3_LRC24M_CON3                  P33_ACCESS(0x02b)
#define P3_LRC24M_AUTO0                 P33_ACCESS(0x02c)

#define P3_XOSC32K_CON0                 P33_ACCESS(0x02d)
#define P3_XOSC32K_CON1                 P33_ACCESS(0x02e)

//............. 0X0030 - 0X003F............         FOR CLOCK1
#define P3_FLL24M_CON0                  P33_ACCESS(0x030)
#define P3_FLL24M_CON1                  P33_ACCESS(0x031)
#define P3_FLL24M_CON2                  P33_ACCESS(0x032)
#define P3_FLL24M_CON3                  P33_ACCESS(0x033)
#define P3_FLL24M_CON4                  P33_ACCESS(0x034)
#define P3_FLL24M_CON5                  P33_ACCESS(0x035)
#define P3_FLL24M_CON6                  P33_ACCESS(0x036)
#define P3_FLL24M_AUTO0                 P33_ACCESS(0x037)

//............. 0X0040 - 0X004F............         FOR ANALOG FLOW
#define P3_ANA_FLOW0                    P33_ACCESS(0x040)
#define P3_ANA_FLOW1                    P33_ACCESS(0x041)
#define P3_ANA_FLOW2                    P33_ACCESS(0x042)
#define P3_ANA_FLOW3                    P33_ACCESS(0x043)
#define P3_ANA_FLOW4                    P33_ACCESS(0x044)
#define P3_ANA_FLOW5                    P33_ACCESS(0x045)
#define P3_ANA_FLOW6                    P33_ACCESS(0x046)

#define P3_ANA_KEEP0                    P33_ACCESS(0x048)
#define P3_ANA_KEEP1                    P33_ACCESS(0x049)
#define P3_ANA_KEEP2                    P33_ACCESS(0x04a)
#define P3_ANA_KEEP3                    P33_ACCESS(0x04b)
#define P3_ANA_KEEP4                    P33_ACCESS(0x04c)
#define P3_ANA_KEEP5                    P33_ACCESS(0x04d)
#define P3_ANA_KEEP6                    P33_ACCESS(0x04e)

//............. 0X0050 - 0X005F............         FOR ANALOG CONTROL0
#define P3_VPWR_CON0                    P33_ACCESS(0x050)
#define P3_VPWR_CON1                    P33_ACCESS(0x051)
#define P3_PSW_CON0                     P33_ACCESS(0x052)
#define P3_PSW_CON1                     P33_ACCESS(0x053)
#define P3_PSW_CON2                     P33_ACCESS(0x054)
#define P3_PSW_CON3                     P33_ACCESS(0x055)
#define P3_PMU_ADC0                     P33_ACCESS(0x056)
#define P3_PMU_ADC1                     P33_ACCESS(0x057)
#define P3_PMU_ADC2                     P33_ACCESS(0x058)
#define P3_VBG_CON0                     P33_ACCESS(0x059)
#define P3_VBG_CON1                     P33_ACCESS(0x05a)
#define P3_IOV_CON0                     P33_ACCESS(0x05b)
#define P3_IOV_CON1                     P33_ACCESS(0x05c)
#define P3_IOV_CON2                     P33_ACCESS(0x05d)
#define P3_PVD_CON0                     P33_ACCESS(0x05e)
#define P3_WVD_CON0                     P33_ACCESS(0x05f)

//............. 0X0060 - 0X006F............         FOR ANALOG CONTROL1
#define P3_DVD_CON0                     P33_ACCESS(0x060)
#define P3_DVD_CON1                     P33_ACCESS(0x061)
#define P3_DVD2_CON0                    P33_ACCESS(0x062)
#define P3_RVD_CON0                     P33_ACCESS(0x063)
//#define P3_RVD_CON1                     P33_ACCESS(0x063)
#define P3_RVD2_CON0                    P33_ACCESS(0x064)
#define P3_ANA_READ                     P33_ACCESS(0x065)
#define P3_OCP_CON0                     P33_ACCESS(0x066)
#define P3_DRPG_CON0                    P33_ACCESS(0x067)
#define P3_DRPG_CON1                    P33_ACCESS(0x068)
#define P3_SDPG_CON                     P33_ACCESS(0x069)
#define P3_VBAT_TYPE                    P33_ACCESS(0x06a)
#define P3_RTC_ADC0                     P33_ACCESS(0x06b)
#define P3_DCV_CON0                     P33_ACCESS(0x06c)
#define P3_DCV_CON1                     P33_ACCESS(0x06d)
#define P3_DCV_CON2                     P33_ACCESS(0x06e)
#define P3_SEN_CON0                     P33_ACCESS(0x06f)

//............. 0X0070 - 0X007F............         FOR ANALOG CONTROL2
#define P3_CHG_CON0                     P33_ACCESS(0x070)
#define P3_CHG_CON1                     P33_ACCESS(0x071)
#define P3_CHG_CON2                     P33_ACCESS(0x072)
#define P3_CHG_CON3                     P33_ACCESS(0x073)
#define P3_CHG_CON4                     P33_ACCESS(0x074)
#define P3_CHG_CON5                     P33_ACCESS(0x075)
#define P3_ANA_MFIX                     P33_ACCESS(0x076)
#define P3_MFIX_OPT                     P33_ACCESS(0x077)
#define P3_VPWR_BLOCK_CON0              P33_ACCESS(0x078)
#define P3_VQPS_CON                     P33_ACCESS(0x079)
#define P3_SPDIF_CON0                   P33_ACCESS(0x07a)
#define P3_PAVD_CON0                    P33_ACCESS(0x07b)
#define P3_AUV_CON0                     P33_ACCESS(0x07c)

//............. 0X0080 - 0X008F............         FOR ANALOG CONTROL3

//............. 0X0090 - 0X009F............         FOR ANALOG CONTROL4

//............. 0X00A0 - 0X00AF............         FOR BUCK CIRCUIT0
#define P3_BUCK1_CON0                   P33_ACCESS(0x0a0)
#define P3_BUCK1_CON1                   P33_ACCESS(0x0a1)
#define P3_BUCK1_CON2                   P33_ACCESS(0x0a2)
#define P3_BUCK1_CON3                   P33_ACCESS(0x0a3)
#define P3_BUCK1_CON4                   P33_ACCESS(0x0a4)
#define P3_BUCK1_CON5                   P33_ACCESS(0x0a5)
#define P3_BUCK1_CON6                   P33_ACCESS(0x0a6)
#define P3_BUCK1_CON7                   P33_ACCESS(0x0a7)
#define P3_BUCK2_CON0                   P33_ACCESS(0x0a8)
#define P3_BUCK2_CON1                   P33_ACCESS(0x0a9)
#define P3_BUCK2_CON2                   P33_ACCESS(0x0aa)
#define P3_BUCK2_CON3                   P33_ACCESS(0x0ab)
#define P3_BUCK2_CON4                   P33_ACCESS(0x0ac)
#define P3_BUCK2_CON5                   P33_ACCESS(0x0ad)
#define P3_BUCK2_CON6                   P33_ACCESS(0x0ae)
#define P3_BUCK2_CON7                   P33_ACCESS(0x0af)

//............. 0X00B0 - 0X00BF............         FOR BUCK CIRCUIT1
#define P3_BUCK2_CON8                   P33_ACCESS(0x0b0)
#define P3_BUCK2_CON9                   P33_ACCESS(0x0b1)
#define P3_BUCK2_CONA                   P33_ACCESS(0x0b2)
//#define P3_BUCK3_CON0                   P33_ACCESS(0x0b0)
//#define P3_BUCK3_CON1                   P33_ACCESS(0x0b1)
//#define P3_BUCK3_CON2                   P33_ACCESS(0x0b2)
//#define P3_BUCK3_CON3                   P33_ACCESS(0x0b3)
//#define P3_BUCK3_CON4                   P33_ACCESS(0x0b4)
//#define P3_BUCK3_CON5                   P33_ACCESS(0x0b5)
//#define P3_BUCK3_CON6                   P33_ACCESS(0x0b6)
//#define P3_BUCK3_CON7                   P33_ACCESS(0x0b7)


//............. 0X00C0 - 0X00CF............         FOR LOW PWR FLOW0
#define P3_IVS_RD                       P33_ACCESS(0x0c0)
#define P3_IVS_SET                      P33_ACCESS(0x0c1)
#define P3_IVS_CLR                      P33_ACCESS(0x0c2)
//#define P3_WKUP_DLY                     P33_ACCESS(0x0c3)
#define P3_PMU_CON0                     P33_ACCESS(0x0c4)
//#define P3_PMU_CON1                     P33_ACCESS(0x0c5)
//#define P3_PMU_CON2                     P33_ACCESS(0x0c6)
//#define P3_PMU_CON3                     P33_ACCESS(0x0c7)
//#define P3_PMU_CON4                     P33_ACCESS(0x0c8)
//#define P3_PMU_CON5                     P33_ACCESS(0x0c9)
#define P3_PMC_CPU                      P33_ACCESS(0x0ca)
#define P3_LS_EN0                       P33_ACCESS(0x0cb)
#define P3_LS_EN1                       P33_ACCESS(0x0cc)
#define P3_LS_EN2                       P33_ACCESS(0x0cd)

//............. 0X00D0 - 0X00DF............         FOR LOW PWR FLOW1
#define P3_MLP_CTL0                     P33_ACCESS(0x0d0)
#define P3_MLP_CFG0                     P33_ACCESS(0x0d1)
#define P3_MLP_CFG1                     P33_ACCESS(0x0d2)
#define P3_MLP_CNT0                     P33_ACCESS(0x0d3)
#define P3_MLP_CNT1                     P33_ACCESS(0x0d4)

#define P3_PLP_CTL0                     P33_ACCESS(0x0d8)
#define P3_PLP_CFG0                     P33_ACCESS(0x0d9)
#define P3_PLP_CFG1                     P33_ACCESS(0x0da)
#define P3_PLP_CNT0                     P33_ACCESS(0x0db)
#define P3_PLP_CNT1                     P33_ACCESS(0x0dc)
#define P3_PLP_CNT2                     P33_ACCESS(0x0dd)
#define P3_PLP_CNT3                     P33_ACCESS(0x0de)
#define P3_PLP_CNT4                     P33_ACCESS(0x0df)

//............. 0X00E0 - 0X00EF............         FOR LOW PWR FLOW2
#define P3_WVDD_AUTO0                   P33_ACCESS(0x0e0)
#define P3_WVDD_AUTO1                   P33_ACCESS(0x0e1)
#define P3_WVDD2_AUTO0                  P33_ACCESS(0x0e2)
#define P3_WVDD2_AUTO1                  P33_ACCESS(0x0e3)
#define P3_PVDD_AUTO0                   P33_ACCESS(0x0e4)
#define P3_PVDD_AUTO1                   P33_ACCESS(0x0e5)
#define P3_ULP_VOL_AUTO0                P33_ACCESS(0x0e6)
#define P3_ULP_VOL_AUTO1                P33_ACCESS(0x0e7)

#define P3_MNV_CFG                      P33_ACCESS(0x0e8)
#define P3_PNV_CFG                      P33_ACCESS(0x0e9)
#define P3_MNV_SEL0                     P33_ACCESS(0x0ea)
#define P3_MNV_SEL1                     P33_ACCESS(0x0eb)
#define P3_MNV_SEL2                     P33_ACCESS(0x0ec)
#define P3_MNV_SEL3                     P33_ACCESS(0x0ed)
#define P3_PNV_SEL0                     P33_ACCESS(0x0ee)
#define P3_PNV_SEL1                     P33_ACCESS(0x0ef)

//............. 0X00F0 - 0X00FF............         FOR PORT WAKE UP
#define P3_WKUP_FLT_EN0                 P33_ACCESS(0x0f0)
#define P3_WKUP_P_IE0                   P33_ACCESS(0x0f1)
#define P3_WKUP_N_IE0                   P33_ACCESS(0x0f2)
#define P3_WKUP_LEVEL0                  P33_ACCESS(0x0f3)
#define P3_WKUP_P_CPND0                 P33_ACCESS(0x0f4)
#define P3_WKUP_N_CPND0                 P33_ACCESS(0x0f5)
#define P3_WKUP_P_PND0                  P33_ACCESS(0x0f6)
#define P3_WKUP_N_PND0                  P33_ACCESS(0x0f7)
#define P3_WKUP_FLT_EN1                 P33_ACCESS(0x0f8)
#define P3_WKUP_P_IE1                   P33_ACCESS(0x0f9)
#define P3_WKUP_N_IE1                   P33_ACCESS(0x0fa)
#define P3_WKUP_LEVEL1                  P33_ACCESS(0x0fb)
#define P3_WKUP_P_CPND1                 P33_ACCESS(0x0fc)
#define P3_WKUP_N_CPND1                 P33_ACCESS(0x0fd)
#define P3_WKUP_P_PND1                  P33_ACCESS(0x0fe)
#define P3_WKUP_N_PND1                  P33_ACCESS(0x0ff)

//............. 0X0100 - 0X010F............         FOR ANALOG WAKE UP
#define P3_AWKUP_FLT_EN                 P33_ACCESS(0x100)
#define P3_AWKUP_P_IE                   P33_ACCESS(0x101)
#define P3_AWKUP_N_IE                   P33_ACCESS(0x102)
#define P3_AWKUP_LEVEL                  P33_ACCESS(0x103)
#define P3_AWKUP_P_PND                  P33_ACCESS(0x104)
#define P3_AWKUP_N_PND                  P33_ACCESS(0x105)
#define P3_AWKUP_P_CPND                 P33_ACCESS(0x106)
#define P3_AWKUP_N_CPND                 P33_ACCESS(0x107)
#define P3_WKUP_CLK_SEL                 P33_ACCESS(0x108)
#define P3_AWKUP_CLK_SEL                P33_ACCESS(0x109)

//............. 0X0110 - 0X011F............         FOR WAKE UP CFG0

//............. 0X0120 - 0X012F............         FOR WAKE UP CFG1
#define P3_WKUP_SRC0                    P33_ACCESS(0x120)
#define P3_WKUP_SRC1                    P33_ACCESS(0x121)
#define P3_WKUP_SRC2                    P33_ACCESS(0x122)

#define P3_VDET_CON0                    P33_ACCESS(0x124)
#define P3_VDET_CON1                    P33_ACCESS(0x125)
#define P3_VDET_CNT0                    P33_ACCESS(0x126)

#define P3_DBG_CON0                     P33_ACCESS(0x128)
#define P3_DBG_CON1                     P33_ACCESS(0x129)
#define P3_FUNC_EN                      P33_ACCESS(0x12a)
#define P3_FUNC_CTL0                    P33_ACCESS(0x12b)
#define P3_FUNC_CTL1                    P33_ACCESS(0x12c)
#define P3_FUNC_CTL2                    P33_ACCESS(0x12d)

//............. 0X0130 - 0X013F............         FOR MEMORY CFG0
#define P3_MNV_PWR0                     P33_ACCESS(0x130)
#define P3_MNV_PWR1                     P33_ACCESS(0x131)
#define P3_MNV_PWR2                     P33_ACCESS(0x132)
#define P3_MNV_PWR3                     P33_ACCESS(0x133)
#define P3_MNV_PWR4                     P33_ACCESS(0x134)
#define P3_MNV_PWR5                     P33_ACCESS(0x135)
#define P3_MNV_PWR6                     P33_ACCESS(0x136)
#define P3_MNV_PWR7                     P33_ACCESS(0x137)
#define P3_MNV_PWR8                     P33_ACCESS(0x138)
#define P3_MNV_PWR9                     P33_ACCESS(0x139)
#define P3_MNV_PWRA                     P33_ACCESS(0x13a)
#define P3_MNV_PWRB                     P33_ACCESS(0x13b)
#define P3_MNV_PWRC                     P33_ACCESS(0x13c)
#define P3_MNV_PWRD                     P33_ACCESS(0x13d)
#define P3_MNV_PWRE                     P33_ACCESS(0x13e)
#define P3_MNV_PWRF                     P33_ACCESS(0x13f)

//............. 0X0140 - 0X014F............         FOR MEMORY CFG1
#define P3_MNV_PWR10                    P33_ACCESS(0x140)
#define P3_MNV_PWR11                    P33_ACCESS(0x141)
#define P3_MNV_PWR12                    P33_ACCESS(0x142)
#define P3_MNV_PWR13                    P33_ACCESS(0x143)
#define P3_MNV_PWR14                    P33_ACCESS(0x144)
#define P3_MNV_PWR15                    P33_ACCESS(0x145)
#define P3_MNV_PWR16                    P33_ACCESS(0x146)
#define P3_MNV_PWR17                    P33_ACCESS(0x147)
#define P3_MNV_PWR18                    P33_ACCESS(0x148)
#define P3_MNV_PWR19                    P33_ACCESS(0x149)
#define P3_MNV_PWR1A                    P33_ACCESS(0x14a)
#define P3_MNV_PWR1B                    P33_ACCESS(0x14b)
#define P3_MNV_PWR1C                    P33_ACCESS(0x14c)
#define P3_MNV_PWR1D                    P33_ACCESS(0x14d)
#define P3_MNV_PWR1E                    P33_ACCESS(0x14e)
#define P3_MNV_PWR1F                    P33_ACCESS(0x14f)

//............. 0X0150 - 0X015F............         FOR MEMORY CFG2
#define P3_PNV_PWR0                     P33_ACCESS(0x150)
#define P3_PNV_PWR1                     P33_ACCESS(0x151)
#define P3_PNV_PWR2                     P33_ACCESS(0x152)
#define P3_PNV_PWR3                     P33_ACCESS(0x153)
#define P3_PNV_PWR4                     P33_ACCESS(0x154)
#define P3_PNV_PWR5                     P33_ACCESS(0x155)
#define P3_PNV_PWR6                     P33_ACCESS(0x156)
#define P3_PNV_PWR7                     P33_ACCESS(0x157)
#define P3_PNV_PWR8                     P33_ACCESS(0x158)
#define P3_PNV_PWR9                     P33_ACCESS(0x159)
#define P3_MNV_PSEL0                    P33_ACCESS(0x15a)
#define P3_PWR_ISO_EN0                  P33_ACCESS(0x15b)
#define P3_PWR_ISO_EN1                  P33_ACCESS(0x15c)
#define P3_PWR_READY                    P33_ACCESS(0x15d)

//............. 0X0160 - 0X016F............         FOR MEMORY CFG3
#define P3_MNV_SOAR_SEL0                P33_ACCESS(0x160)
#define P3_MNV_SOAR_SEL1                P33_ACCESS(0x161)
#define P3_MNV_SOAR_SEL2                P33_ACCESS(0x162)
#define P3_MNV_SOAR_SEL3                P33_ACCESS(0x163)
#define P3_MNV_SOAR_CEB0                P33_ACCESS(0x164)
#define P3_MNV_SOAR_CEB1                P33_ACCESS(0x165)
#define P3_MNV_SOAR_CEB2                P33_ACCESS(0x166)
#define P3_MNV_SOAR_CEB3                P33_ACCESS(0x167)
#define P3_MNV_SOAR_CLK0                P33_ACCESS(0x168)
#define P3_MNV_SOAR_CLK1                P33_ACCESS(0x169)
#define P3_MNV_SOAR_CLK2                P33_ACCESS(0x16a)
#define P3_MNV_SOAR_CLK3                P33_ACCESS(0x16b)


//............. 0X0170 - 0X017F............         FOR MEMORY CFG2
#define P3_EFUSE_CON0                   P33_ACCESS(0x170)
#define P3_EFUSE_CON1                   P33_ACCESS(0x171)
#define P3_EFUSE_CON2                   P33_ACCESS(0x172)
#define P3_EFUSE_CON3                   P33_ACCESS(0x173)
#define P3_EFUSE_RDAT                   P33_ACCESS(0x174)
#define P3_EFUSE_PU_DAT0                P33_ACCESS(0x175)
#define P3_EFUSE_PU_DAT1                P33_ACCESS(0x176)
#define P3_EFUSE_PU_DAT2                P33_ACCESS(0x177)
#define P3_EFUSE_PU_DAT3                P33_ACCESS(0x178)
#define P3_EFUSE_ANA0                   P33_ACCESS(0x179)

//............. 0X0180 - 0X018F............         FOR SOFT FLAG
#define P3_SFLAG0                       P33_ACCESS(0x180)
#define P3_SFLAG1                       P33_ACCESS(0x181)
#define P3_SFLAG2                       P33_ACCESS(0x182)
#define P3_SFLAG3                       P33_ACCESS(0x183)
#define P3_SFLAG4                       P33_ACCESS(0x184)
#define P3_SFLAG5                       P33_ACCESS(0x185)
#define P3_SFLAG6                       P33_ACCESS(0x186)
#define P3_SFLAG7                       P33_ACCESS(0x187)
#define P3_SFLAG8                       P33_ACCESS(0x188)
#define P3_SFLAG9                       P33_ACCESS(0x189)
#define P3_SFLAGA                       P33_ACCESS(0x18a)
#define P3_SFLAGB                       P33_ACCESS(0x18b)
#define P3_SFLAGC                       P33_ACCESS(0x18c)
#define P3_SFLAGD                       P33_ACCESS(0x18d)
#define P3_SFLAGE                       P33_ACCESS(0x18e)
#define P3_SFLAGF                       P33_ACCESS(0x18f)

//............. 0X0190 - 0X019F............         FOR PORT INPUT SELECT
#define P3_PORT_SEL0                    P33_ACCESS(0x190)
#define P3_PORT_SEL1                    P33_ACCESS(0x191)
#define P3_PORT_SEL2                    P33_ACCESS(0x192)
#define P3_PORT_SEL3                    P33_ACCESS(0x193)
#define P3_PORT_SEL4                    P33_ACCESS(0x194)
#define P3_PORT_SEL5                    P33_ACCESS(0x195)
#define P3_PORT_SEL6                    P33_ACCESS(0x196)
#define P3_PORT_SEL7                    P33_ACCESS(0x197)
#define P3_PORT_SEL8                    P33_ACCESS(0x198)
#define P3_PORT_SEL9                    P33_ACCESS(0x199)
#define P3_PORT_SEL10                   P33_ACCESS(0x19a)
#define P3_PORT_SEL11                   P33_ACCESS(0x19b)
#define P3_PORT_SEL12                   P33_ACCESS(0x19c)
#define P3_PORT_SEL13                   P33_ACCESS(0x19d)
#define P3_PORT_SEL14                   P33_ACCESS(0x19e)
#define P3_PORT_SEL15                   P33_ACCESS(0x19f)

//............. 0X01A0 - 0X01AF............         FOR ANALOG CFG
#define P3_LCO_CON0                     P33_ACCESS(0x1a0)
#define P3_XOSC_CON0                    P33_ACCESS(0x1a1)
#define P3_BTLDO_CON0                   P33_ACCESS(0x1a2)
#define P3_BTLDO_CON1                   P33_ACCESS(0x1a3)
#define P3_BTLDO_CON2                   P33_ACCESS(0x1a4)
#define P3_BTLDO_CON3                   P33_ACCESS(0x1a5)
#define P3_BTLDO_CON4                   P33_ACCESS(0x1a6)
#define P3_FMLDO_CON0                   P33_ACCESS(0x1a7)
#define P3_BTSEL_CON0                   P33_ACCESS(0x1a8)
#define P3_CTMSEL_CON0                  P33_ACCESS(0x1a9)

#define P3_NFC_CON0                     P33_ACCESS(0x1ac)
#define P3_NFC_CON1                     P33_ACCESS(0x1ad)
#define P3_NFC_CON2                     P33_ACCESS(0x1ae)
#define P3_NFC_CON3                     P33_ACCESS(0x1af)

//............. 0X01B0 - 0X01BF............         FOR P33 LP TIMER CFG0
#define P3_LP_RSC00                     P33_ACCESS(0x1b0)
#define P3_LP_RSC01                     P33_ACCESS(0x1b1)
#define P3_LP_RSC02                     P33_ACCESS(0x1b2)
#define P3_LP_RSC03                     P33_ACCESS(0x1b3)
#define P3_LP_RSC04                     P33_ACCESS(0x1b4)
#define P3_LP_RSC05                     P33_ACCESS(0x1b5)
#define P3_LP_PRD00                     P33_ACCESS(0x1b6)
#define P3_LP_PRD01                     P33_ACCESS(0x1b7)
#define P3_LP_PRD02                     P33_ACCESS(0x1b8)
#define P3_LP_PRD03                     P33_ACCESS(0x1b9)
#define P3_LP_PRD04                     P33_ACCESS(0x1ba)
#define P3_LP_PRD05                     P33_ACCESS(0x1bb)
#define P3_LP_TMR0_CLK                  P33_ACCESS(0x1bc)
#define P3_LP_TMR0_CON                  P33_ACCESS(0x1bd)
#define P3_LP_TMR0_CFG                  P33_ACCESS(0x1be)

//............. 0X01C0 - 0X01CF............         FOR P33 LP TIMER CFG1
#define P3_LP_RSC10                     P33_ACCESS(0x1c0)
#define P3_LP_RSC11                     P33_ACCESS(0x1c1)
#define P3_LP_RSC12                     P33_ACCESS(0x1c2)
#define P3_LP_RSC13                     P33_ACCESS(0x1c3)
#define P3_LP_RSC14                     P33_ACCESS(0x1c4)
#define P3_LP_RSC15                     P33_ACCESS(0x1c5)
#define P3_LP_PRD10                     P33_ACCESS(0x1c6)
#define P3_LP_PRD11                     P33_ACCESS(0x1c7)
#define P3_LP_PRD12                     P33_ACCESS(0x1c8)
#define P3_LP_PRD13                     P33_ACCESS(0x1c9)
#define P3_LP_PRD14                     P33_ACCESS(0x1ca)
#define P3_LP_PRD15                     P33_ACCESS(0x1cb)
#define P3_LP_TMR1_CLK                  P33_ACCESS(0x1cc)
#define P3_LP_TMR1_CON                  P33_ACCESS(0x1cd)
#define P3_LP_TMR1_CFG                  P33_ACCESS(0x1ce)

//............. 0X01D0 - 0X01DF............         FOR P33 LP TIMER CFG2
#define P3_LP_CNTRD0                    P33_ACCESS(0x1d0)
#define P3_LP_CNT0                      P33_ACCESS(0x1d1)
#define P3_LP_CNT1                      P33_ACCESS(0x1d2)
#define P3_LP_CNT2                      P33_ACCESS(0x1d3)
#define P3_LP_CNT3                      P33_ACCESS(0x1d4)
#define P3_LP_CNT4                      P33_ACCESS(0x1d5)
#define P3_LP_CNT5                      P33_ACCESS(0x1d6)

//............. 0X01E0 - 0X01EF............         FOR LP TIMER
#define P3_LP_RSC20                     P33_ACCESS(0x1e0)
#define P3_LP_RSC21                     P33_ACCESS(0x1e1)
#define P3_LP_RSC22                     P33_ACCESS(0x1e2)
#define P3_LP_RSC23                     P33_ACCESS(0x1e3)
#define P3_LP_RSC24                     P33_ACCESS(0x1e4)
#define P3_LP_RSC25                     P33_ACCESS(0x1e5)
#define P3_LP_PRD20                     P33_ACCESS(0x1e6)
#define P3_LP_PRD21                     P33_ACCESS(0x1e7)
#define P3_LP_PRD22                     P33_ACCESS(0x1e8)
#define P3_LP_PRD23                     P33_ACCESS(0x1e9)
#define P3_LP_PRD24                     P33_ACCESS(0x1ea)
#define P3_LP_PRD25                     P33_ACCESS(0x1eb)
#define P3_LP_TMR2_CLK                  P33_ACCESS(0x1ec)
#define P3_LP_TMR2_CON                  P33_ACCESS(0x1ed)
#define P3_LP_TMR2_CFG                  P33_ACCESS(0x1ee)
#define P3_KEY_CON0                     P33_ACCESS(0x1ef)

//............. 0X01F0 - 0X01FF............         FOR SFC SAFE KEY
#define P3_KEY0                         P33_ACCESS(0x1f0)
#define P3_KEY1                         P33_ACCESS(0x1f1)
#define P3_KEY2                         P33_ACCESS(0x1f2)
#define P3_KEY3                         P33_ACCESS(0x1f3)
#define P3_KEY4                         P33_ACCESS(0x1f4)
#define P3_KEY5                         P33_ACCESS(0x1f5)
#define P3_KEY6                         P33_ACCESS(0x1f6)
#define P3_KEY7                         P33_ACCESS(0x1f7)
#define P3_KEY8                         P33_ACCESS(0x1f8)
#define P3_KEY9                         P33_ACCESS(0x1f9)
#define P3_KEYA                         P33_ACCESS(0x1fa)
#define P3_KEYB                         P33_ACCESS(0x1fb)
#define P3_KEYC                         P33_ACCESS(0x1fc)
#define P3_KEYD                         P33_ACCESS(0x1fd)
#define P3_KEYE                         P33_ACCESS(0x1fe)
#define P3_KEYF                         P33_ACCESS(0x1ff)

//............. 0X0200 - 0X020F............         FOR LCDC
//#define P3_LCDC_CON0                    P33_ACCESS(0x200)
//#define P3_LCDC_CON1                    P33_ACCESS(0x201)
//#define P3_LCDC_CON2                    P33_ACCESS(0x202)
//#define P3_LCDC_CON3                    P33_ACCESS(0x203)
//#define P3_LCDC_CON4                    P33_ACCESS(0x204)
//#define P3_LCDC_CON5                    P33_ACCESS(0x205)
//#define P3_LCDC_CON6                    P33_ACCESS(0x206)
//#define P3_LCDC_CON7                    P33_ACCESS(0x207)
//#define P3_LCD_CHG_CON0                 P33_ACCESS(0x208)
//#define P3_LCD_CHG_CON1                 P33_ACCESS(0x209)
//#define P3_LCD_CHG_CON2                 P33_ACCESS(0x20a)

//............. 0X0210 - 0X021F............         FOR LCDC
//#define P3_SEG0_DAT0                    P33_ACCESS(0x210)
//#define P3_SEG0_DAT1                    P33_ACCESS(0x211)
//#define P3_SEG0_DAT2                    P33_ACCESS(0x212)
//#define P3_SEG0_DAT3                    P33_ACCESS(0x213)
//#define P3_SEG1_DAT0                    P33_ACCESS(0x214)
//#define P3_SEG1_DAT1                    P33_ACCESS(0x215)
//#define P3_SEG1_DAT2                    P33_ACCESS(0x216)
//#define P3_SEG1_DAT3                    P33_ACCESS(0x217)
//#define P3_SEG2_DAT0                    P33_ACCESS(0x218)
//#define P3_SEG2_DAT1                    P33_ACCESS(0x219)
//#define P3_SEG2_DAT2                    P33_ACCESS(0x21a)
//#define P3_SEG2_DAT3                    P33_ACCESS(0x21b)
//#define P3_SEG3_DAT0                    P33_ACCESS(0x21c)
//#define P3_SEG3_DAT1                    P33_ACCESS(0x21d)
//#define P3_SEG3_DAT2                    P33_ACCESS(0x21e)
//#define P3_SEG3_DAT3                    P33_ACCESS(0x21f)

//............. 0X0220 - 0X022F............         FOR LCDC
//#define P3_PORTC_PD00                   P33_ACCESS(0x220)
//#define P3_PORTC_PD01                   P33_ACCESS(0x221)
//#define P3_PORTC_PD10                   P33_ACCESS(0x222)
//#define P3_PORTC_PD11                   P33_ACCESS(0x223)
//#define P3_PORTD_PD00                   P33_ACCESS(0x224)
//#define P3_PORTD_PD01                   P33_ACCESS(0x225)
//#define P3_PORTD_PD10                   P33_ACCESS(0x226)
//#define P3_PORTD_PD11                   P33_ACCESS(0x227)

//............. 0X0230 - 0X023F............         FOR LCDC
//#define P3_SEG4_DAT0                    P33_ACCESS(0x230)
//#define P3_SEG4_DAT1                    P33_ACCESS(0x231)
//#define P3_SEG4_DAT2                    P33_ACCESS(0x232)
//#define P3_SEG4_DAT3                    P33_ACCESS(0x233)
//#define P3_SEG5_DAT0                    P33_ACCESS(0x234)
//#define P3_SEG5_DAT1                    P33_ACCESS(0x235)
//#define P3_SEG5_DAT2                    P33_ACCESS(0x236)
//#define P3_SEG5_DAT3                    P33_ACCESS(0x237)
//#define P3_SEG_IO_EN0                   P33_ACCESS(0x238)
//#define P3_SEG_IO_EN1                   P33_ACCESS(0x239)
//#define P3_SEG_IO_EN2                   P33_ACCESS(0x23a)
//#define P3_SEG_IO_EN3                   P33_ACCESS(0x23b)

//............. 0X0240 - 0X024F............         FOR LPCTMU
#define P3_LPCTMU_CON0                  P33_ACCESS(0x240)
#define P3_LPCTMU_CON1                  P33_ACCESS(0x241)
#define P3_LPCTMU_CON2                  P33_ACCESS(0x242)
#define P3_LPCTMU_CON3                  P33_ACCESS(0x243)
#define P3_LPCTMU_CON4                  P33_ACCESS(0x244)
#define P3_LPCTMU_CON5                  P33_ACCESS(0x245)
#define P3_LPCTMU_CON6                  P33_ACCESS(0x246)
#define P3_LPCTMU_CON7                  P33_ACCESS(0x247)
#define P3_LPCTMU_CON8                  P33_ACCESS(0x248)
#define P3_LPCTMU_CON9                  P33_ACCESS(0x249)
#define P3_LPCTMU_CONA                  P33_ACCESS(0x24a)
#define P3_LPCTMU_CONB                  P33_ACCESS(0x24b)
#define P3_LPCTMU_RES_POS               P33_ACCESS(0x24c)
#define P3_LPCTMU_L2_RES_POS            P33_ACCESS(0x24d)

//............. 0X0250 - 0X025F............         FOR LPCTMU
#define P3_LPCTMU_RES00                 P33_ACCESS(0x250)
#define P3_LPCTMU_RES01                 P33_ACCESS(0x251)
#define P3_LPCTMU_RES02                 P33_ACCESS(0x252)
#define P3_LPCTMU_RES03                 P33_ACCESS(0x253)
#define P3_LPCTMU_RES04                 P33_ACCESS(0x254)
#define P3_LPCTMU_RES05                 P33_ACCESS(0x255)
#define P3_LPCTMU_RES06                 P33_ACCESS(0x256)
#define P3_LPCTMU_RES07                 P33_ACCESS(0x257)
#define P3_LPCTMU_RES08                 P33_ACCESS(0x258)
#define P3_LPCTMU_RES09                 P33_ACCESS(0x259)
#define P3_LPCTMU_RES0A                 P33_ACCESS(0x25a)
#define P3_LPCTMU_RES0B                 P33_ACCESS(0x25b)
#define P3_LPCTMU_RES0C                 P33_ACCESS(0x25c)
#define P3_LPCTMU_RES0D                 P33_ACCESS(0x25d)
#define P3_LPCTMU_RES0E                 P33_ACCESS(0x25e)
#define P3_LPCTMU_RES0F                 P33_ACCESS(0x25f)

//............. 0X0260 - 0X026F............         FOR LPCTMU
#define P3_LPCTMU_RES10                 P33_ACCESS(0x260)
#define P3_LPCTMU_RES11                 P33_ACCESS(0x261)
#define P3_LPCTMU_RES12                 P33_ACCESS(0x262)
#define P3_LPCTMU_RES13                 P33_ACCESS(0x263)
#define P3_LPCTMU_RES14                 P33_ACCESS(0x264)
#define P3_LPCTMU_RES15                 P33_ACCESS(0x265)
#define P3_LPCTMU_RES16                 P33_ACCESS(0x266)
#define P3_LPCTMU_RES17                 P33_ACCESS(0x267)
#define P3_LPCTMU_RES18                 P33_ACCESS(0x268)
#define P3_LPCTMU_RES19                 P33_ACCESS(0x269)
#define P3_LPCTMU_RES1A                 P33_ACCESS(0x26a)
#define P3_LPCTMU_RES1B                 P33_ACCESS(0x26b)
#define P3_LPCTMU_RES1C                 P33_ACCESS(0x26c)
#define P3_LPCTMU_RES1D                 P33_ACCESS(0x26d)
#define P3_LPCTMU_RES1E                 P33_ACCESS(0x26e)
#define P3_LPCTMU_RES1F                 P33_ACCESS(0x26f)

//............. 0X0270 - 0X027F............         FOR LPCTMU
#define P3_RES_LIMH0                    P33_ACCESS(0x270)
#define P3_RES_LIMH1                    P33_ACCESS(0x271)
#define P3_RES_LIML0                    P33_ACCESS(0x272)
#define P3_RES_LIML1                    P33_ACCESS(0x273)

//............. 0X0280 - 0X028F............         FOR LPUART
//#define P3_LPUART_RXCON0                P33_ACCESS(0x280)
//#define P3_LPUART_CHKCON                P33_ACCESS(0x281)
//#define P3_LPUART_BAUD_L                P33_ACCESS(0x282)
//#define P3_LPUART_BAUD_H                P33_ACCESS(0x283)
//#define P3_LPUART_RXCTL0                P33_ACCESS(0x284)
//#define P3_LPUART_RXCTL1                P33_ACCESS(0x285)
//#define P3_LPUART_RXCTL2                P33_ACCESS(0x286)
//#define P3_LPUART_RXADDR0               P33_ACCESS(0x287)
//#define P3_LPUART_RXADDR1               P33_ACCESS(0x288)
//#define P3_LPUART_RXIDLE0               P33_ACCESS(0x289)
//#define P3_LPUART_FFCTL0                P33_ACCESS(0x28a)
//#define P3_LPUART_FFCTL1                P33_ACCESS(0x28b)
//#define P3_LPUART_FFCTL2                P33_ACCESS(0x28c)
//#define P3_LPUART_FFCTL3                P33_ACCESS(0x28d)

//............. 0X0290 - 0X029F............         FOR LPUART
//#define P3_LPUART_FFDATA0_L             P33_ACCESS(0x290)
//#define P3_LPUART_FFDATA0_H             P33_ACCESS(0x291)
//#define P3_LPUART_FFDATA1_L             P33_ACCESS(0x292)
//#define P3_LPUART_FFDATA1_H             P33_ACCESS(0x293)
//#define P3_LPUART_FFDATA2_L             P33_ACCESS(0x294)
//#define P3_LPUART_FFDATA2_H             P33_ACCESS(0x295)
//#define P3_LPUART_FFDATA3_L             P33_ACCESS(0x296)
//#define P3_LPUART_FFDATA3_H             P33_ACCESS(0x297)
//#define P3_LPUART_FFDATA4_L             P33_ACCESS(0x298)
//#define P3_LPUART_FFDATA4_H             P33_ACCESS(0x299)
//#define P3_LPUART_FFDATA5_L             P33_ACCESS(0x29a)
//#define P3_LPUART_FFDATA5_H             P33_ACCESS(0x29b)
//#define P3_LPUART_FFDATA6_L             P33_ACCESS(0x29c)
//#define P3_LPUART_FFDATA6_H             P33_ACCESS(0x29d)
//#define P3_LPUART_FFDATA7_L             P33_ACCESS(0x29e)
//#define P3_LPUART_FFDATA7_H             P33_ACCESS(0x29f)

//............. 0X02A0 - 0X02AF............         FOR INDEPENDENT WKUP
//#define P3_AIWKUP_FLT_EN                P33_ACCESS(0x2a0)
//#define P3_AIWKUP_P_IE                  P33_ACCESS(0x2a1)
//#define P3_AIWKUP_N_IE                  P33_ACCESS(0x2a2)
//#define P3_AIWKUP_LEVEL                 P33_ACCESS(0x2a3)
//#define P3_AIWKUP_P_PND                 P33_ACCESS(0x2a4)
//#define P3_AIWKUP_N_PND                 P33_ACCESS(0x2a5)
//#define P3_AIWKUP_P_CPND                P33_ACCESS(0x2a6)
//#define P3_AIWKUP_N_CPND                P33_ACCESS(0x2a7)
//#define P3_AIWKUP_CLK_SEL0              P33_ACCESS(0x2a8)
//#define P3_AIWKUP_CLK_SEL1              P33_ACCESS(0x2a9)
//#define P3_AIWKUP_CLK_SEL2              P33_ACCESS(0x2aa)
//#define P3_AIWKUP_CLK_SEL3              P33_ACCESS(0x2ab)
//#define P3_AIWKUP_CLK_SEL4              P33_ACCESS(0x2ac)
//#define P3_AIWKUP_CLK_SEL5              P33_ACCESS(0x2ad)
//#define P3_AIWKUP_CLK_SEL6              P33_ACCESS(0x2ae)
//#define P3_AIWKUP_CLK_SEL7              P33_ACCESS(0x2af)

//............. 0X02B0 - 0X02BF............         FOR INDEPENDENT WKUP
//#define P3_IWKUP_FLT_EN                 P33_ACCESS(0x2b0)
//#define P3_IWKUP_P_IE                   P33_ACCESS(0x2b1)
//#define P3_IWKUP_N_IE                   P33_ACCESS(0x2b2)
//#define P3_IWKUP_LEVEL                  P33_ACCESS(0x2b3)
//#define P3_IWKUP_P_PND                  P33_ACCESS(0x2b4)
//#define P3_IWKUP_N_PND                  P33_ACCESS(0x2b5)
//#define P3_IWKUP_P_CPND                 P33_ACCESS(0x2b6)
//#define P3_IWKUP_N_CPND                 P33_ACCESS(0x2b7)
//#define P3_IWKUP_CLK_SEL0               P33_ACCESS(0x2b8)
//#define P3_IWKUP_CLK_SEL1               P33_ACCESS(0x2b9)
//#define P3_IWKUP_CLK_SEL2               P33_ACCESS(0x2ba)
//#define P3_IWKUP_CLK_SEL3               P33_ACCESS(0x2bb)
//#define P3_IWKUP_CLK_SEL4               P33_ACCESS(0x2bc)
//#define P3_IWKUP_CLK_SEL5               P33_ACCESS(0x2bd)
//#define P3_IWKUP_CLK_SEL6               P33_ACCESS(0x2be)
//#define P3_IWKUP_CLK_SEL7               P33_ACCESS(0x2bf)

//............. 0X02C0 - 0X02CF............         FOR LPAVAD
#define P3_LPAVAD_CON0                  P33_ACCESS(0x2c0)
#define P3_LPAVAD_CON1                  P33_ACCESS(0x2c1)
#define P3_LPAVAD_CON2                  P33_ACCESS(0x2c2)
#define P3_LPAVAD_CON3                  P33_ACCESS(0x2c3)
#define P3_LPAVAD_CON4                  P33_ACCESS(0x2c4)
#define P3_LPAVAD_CON5                  P33_ACCESS(0x2c5)
#define P3_LPAVAD_CON6                  P33_ACCESS(0x2c6)
#define P3_LPAVAD_CON7                  P33_ACCESS(0x2c7)
#define P3_LPAVAD_DATA0                 P33_ACCESS(0x2c8)
#define P3_LPAVAD_DATA1                 P33_ACCESS(0x2c9)
#define P3_LPAVAD_DATA2                 P33_ACCESS(0x2ca)
#define P3_LPAVAD_DATA3                 P33_ACCESS(0x2cb)
#define P3_LPAVAD_CLK_CON0              P33_ACCESS(0x2cc)

//............. 0X02D0 - 0X02DF............         RESERVED

//............. 0X02E0 - 0X02EF............         FOR PCNT
#define P3_PCNT0_FLT                    P33_ACCESS(0x2e0)
#define P3_PCNT1_FLT                    P33_ACCESS(0x2e1)
#define P3_PCNT2_FLT                    P33_ACCESS(0x2e2)
#define P3_PCNT3_FLT                    P33_ACCESS(0x2e3)
#define P3_PCNT4_FLT                    P33_ACCESS(0x2e4)
#define P3_PCNT5_FLT                    P33_ACCESS(0x2e5)
#define P3_PCNT6_FLT                    P33_ACCESS(0x2e6)
#define P3_PCNT7_FLT                    P33_ACCESS(0x2e7)
#define P3_PCNT0_CON                    P33_ACCESS(0x2e8)
#define P3_PCNT1_CON                    P33_ACCESS(0x2e9)
#define P3_PCNT2_CON                    P33_ACCESS(0x2ea)
#define P3_PCNT3_CON                    P33_ACCESS(0x2eb)
#define P3_PCNT4_CON                    P33_ACCESS(0x2ec)
#define P3_PCNT5_CON                    P33_ACCESS(0x2ed)
#define P3_PCNT6_CON                    P33_ACCESS(0x2ee)
#define P3_PCNT7_CON                    P33_ACCESS(0x2ef)

//............. 0X02F0 - 0X02FF............         FOR PCNT
#define P3_PCNT_SET0                    P33_ACCESS(0x2f0)
#define P3_PCNT_SET1                    P33_ACCESS(0x2f1)
#define P3_PCNT_DAT0                    P33_ACCESS(0x2f2)
#define P3_PCNT_DAT1                    P33_ACCESS(0x2f3)
#define P3_PCNT_DAT_SEL                 P33_ACCESS(0x2f4)

//............. 0X0300 - 0X030F............         FOR PWR
#define P3_VIO18_CON0                   P33_ACCESS(0x300)
#define P3_VIO18_CON1                   P33_ACCESS(0x301)
#define P3_VIO28_CON0                   P33_ACCESS(0x302)
#define P3_VIO28_CON1                   P33_ACCESS(0x303)
#define P3_WVD2_CON0                    P33_ACCESS(0x304)

//............. 0X0310 - 0X031F............         FOR LS
#define P3_LS_CON00                     P33_ACCESS(0x310)
#define P3_LS_CON01                     P33_ACCESS(0x311)
#define P3_LS_CON02                     P33_ACCESS(0x312)
#define P3_LS_CON03                     P33_ACCESS(0x313)
#define P3_LS_CON04                     P33_ACCESS(0x314)
#define P3_LS_CON05                     P33_ACCESS(0x315)
#define P3_LS_CON06                     P33_ACCESS(0x316)
#define P3_LS_CON07                     P33_ACCESS(0x317)
#define P3_LS_CON08                     P33_ACCESS(0x318)
#define P3_LS_CON09                     P33_ACCESS(0x319)
#define P3_LS_CON0A                     P33_ACCESS(0x31a)
#define P3_LS_CON0B                     P33_ACCESS(0x31b)
#define P3_LS_CON0C                     P33_ACCESS(0x31c)
#define P3_LS_CON0D                     P33_ACCESS(0x31d)
#define P3_LS_CON0E                     P33_ACCESS(0x31e)
#define P3_LS_CON0F                     P33_ACCESS(0x31f)

//............. 0X0320 - 0X032F............         FOR LS
#define P3_LS_CON10                     P33_ACCESS(0x320)
#define P3_LS_CON11                     P33_ACCESS(0x321)
#define P3_LS_CON12                     P33_ACCESS(0x322)
#define P3_LS_CON13                     P33_ACCESS(0x323)
#define P3_LS_CON14                     P33_ACCESS(0x324)
#define P3_LS_CON15                     P33_ACCESS(0x325)
#define P3_LS_CON16                     P33_ACCESS(0x326)
#define P3_LS_CON17                     P33_ACCESS(0x327)
#define P3_LS_CON18                     P33_ACCESS(0x328)
#define P3_LS_CON19                     P33_ACCESS(0x329)
#define P3_LS_CON1A                     P33_ACCESS(0x32a)
#define P3_LS_CON1B                     P33_ACCESS(0x32b)
#define P3_LS_CON1C                     P33_ACCESS(0x32c)
#define P3_LS_CON1D                     P33_ACCESS(0x32d)
#define P3_LS_CON1E                     P33_ACCESS(0x32e)
#define P3_LS_CON1F                     P33_ACCESS(0x32f)

//............. 0X0330 - 0X033F............         FOR LS
#define P3_LS_CON20                     P33_ACCESS(0x330)
#define P3_LS_CON21                     P33_ACCESS(0x331)
#define P3_LS_CON22                     P33_ACCESS(0x332)
#define P3_LS_CON23                     P33_ACCESS(0x333)
#define P3_LS_CON24                     P33_ACCESS(0x334)
#define P3_LS_CON25                     P33_ACCESS(0x335)
#define P3_LS_CON26                     P33_ACCESS(0x336)
#define P3_LS_CON27                     P33_ACCESS(0x337)
#define P3_LS_CON28                     P33_ACCESS(0x338)
#define P3_LS_CON29                     P33_ACCESS(0x339)
#define P3_LS_CON2A                     P33_ACCESS(0x33a)
#define P3_LS_CON2B                     P33_ACCESS(0x33b)
#define P3_LS_CON2C                     P33_ACCESS(0x33c)
#define P3_LS_CON2D                     P33_ACCESS(0x33d)
#define P3_LS_CON2E                     P33_ACCESS(0x33e)
#define P3_LS_CON2F                     P33_ACCESS(0x33f)


//======================================================
//SFR RENAME
//======================================================
//............. LSE SFR ............
#define P3_LS_IO_USR                    P3_LS_CON00
#define P3_LS_IO_ROM                    P3_LS_CON01
#define P3_LS_IO_PINR                   P3_LS_CON02
#define P3_LS_IO_SHA                    P3_LS_CON03
#define P3_LS_IO_USR_PSEL0              P3_LS_CON04
#define P3_LS_IO_USR_PSEL1              P3_LS_CON05
#define P3_LS_IO_USR_PSEL2              P3_LS_CON06
#define P3_LS_IO_USR_PSEL3              P3_LS_CON07
#define P3_LS_IO_USR_PSEL4              P3_LS_CON08
#define P3_LS_IO_USR_PSEL5              P3_LS_CON09
#define P3_LS_IO_USR_PSEL6              P3_LS_CON0A
#define P3_LS_IO_USR_PSEL7              P3_LS_CON0B
#define P3_LS_IO_USR_PSEL8              P3_LS_CON0C
#define P3_LS_IO_USR_PSEL9              P3_LS_CON0D
#define P3_LS_IO_USR_PSEL10             P3_LS_CON0E
#define P3_LS_IO_USR_PSEL11             P3_LS_CON0F
#define P3_LS_IO_USR_PSEL12             P3_LS_CON10
#define P3_LS_IO_USR_PSEL13             P3_LS_CON11
#define P3_LS_IO_USR_PSEL14             P3_LS_CON12
#define P3_LS_IO_USR_PSEL15             P3_LS_CON13
#define P3_LS_IO_USR_PSEL16             P3_LS_CON14
#define P3_LS_IO_USR_PSEL17             P3_LS_CON15
#define P3_LS_IO_USR_PSEL18             P3_LS_CON16
#define P3_LS_IO_USR_PSEL19             P3_LS_CON17
#define P3_LS_IO_USR_DCV18              P3_LS_CON18
#define P3_LS_IO_ROM_P3V                P3_LS_CON19
#define P3_LS_IO_SHA_IOV2               P3_LS_CON1A
#define P3_LS_IO_SHA_P3V                P3_LS_CON1B
#define P3_LS_USB                       P3_LS_CON1C
#define P3_LS_AUDIO                     P3_LS_CON1D
#define P3_LS_LPADC                     P3_LS_CON1E
#define P3_LS_BTLDO                     P3_LS_CON1F
#define P3_LS_BTPA                      P3_LS_CON20
#define P3_LS_DSI                       P3_LS_CON21
#define P3_LS_WAT                       P3_LS_CON22
#define P3_LS_CTMU                      P3_LS_CON23
#define P3_LS_NFC                       P3_LS_CON24
#define P3_LS_ADC                       P3_LS_CON25
#define P3_LS_SDDR                      P3_LS_CON26
#define P3_LS_XPLL                      P3_LS_CON27
#define P3_LS_SPDIF                     P3_LS_CON28
#define P3_LS_SYSPLL                    P3_LS_CON29
#define P3_LS_XOSC                      P3_LS_CON2A
#define P3_LS_SDPG                      P3_LS_CON2B

//............. LPCTMU SFR ............
#define P3_LPCTMU_CON                   P3_LPCTMU_CON0
#define P3_LPCTMU_PPRD                  P3_LPCTMU_CON3
#define P3_LPCTMU_DPRD0                 P3_LPCTMU_CON4
#define P3_LPCTMU_DPRD1                 P3_LPCTMU_CON5
#define P3_LPCTMU_WCON                  P3_LPCTMU_CON7
#define P3_LPCTMU_PND                   P3_LPCTMU_CON8
#define P3_LPCTMU_WKUP                  P3_LPCTMU_CON9
#define P3_LPCTMU_PDEN                  P3_LPCTMU_CONA

#define P3_LPCTMU_RES0L                 P3_LPCTMU_RES00
#define P3_LPCTMU_RES0H                 P3_LPCTMU_RES01
#define P3_LPCTMU_RES1L                 P3_LPCTMU_RES02
#define P3_LPCTMU_RES1H                 P3_LPCTMU_RES03
#define P3_LPCTMU_RES2L                 P3_LPCTMU_RES04
#define P3_LPCTMU_RES2H                 P3_LPCTMU_RES05
#define P3_LPCTMU_RES3L                 P3_LPCTMU_RES06
#define P3_LPCTMU_RES3H                 P3_LPCTMU_RES07
#define P3_LPCTMU_RES4L                 P3_LPCTMU_RES08
#define P3_LPCTMU_RES4H                 P3_LPCTMU_RES09

#define P3_LPCTMU_L2_RES0L              P3_LPCTMU_RES0A
#define P3_LPCTMU_L2_RES0H              P3_LPCTMU_RES0B
#define P3_LPCTMU_L2_RES1L              P3_LPCTMU_RES0C
#define P3_LPCTMU_L2_RES1H              P3_LPCTMU_RES0D
#define P3_LPCTMU_L2_RES2L              P3_LPCTMU_RES0E
#define P3_LPCTMU_L2_RES2H              P3_LPCTMU_RES0F
#define P3_LPCTMU_L2_RES3L              P3_LPCTMU_RES10
#define P3_LPCTMU_L2_RES3H              P3_LPCTMU_RES11
#define P3_LPCTMU_L2_RES4L              P3_LPCTMU_RES12
#define P3_LPCTMU_L2_RES4H              P3_LPCTMU_RES13

#define P3_LPCTMU_ANA0                  P3_LPCTMU_CON6
#define P3_LPCTMU_ANA1                  P3_LPCTMU_CON1
#define P3_LPCTMU_ANA2                  P3_LPCTMU_CON2
#define P3_LPCTMU_ANA3                  P3_LPCTMU_CONA




#endif



