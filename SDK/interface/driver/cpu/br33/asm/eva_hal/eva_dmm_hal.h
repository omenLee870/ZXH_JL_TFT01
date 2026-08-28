
//========================================================================================
//
//
//  DMM SFR define
//
//
//========================================================================================
#ifndef __EVA_DMM_HAL_H__
#define __EVA_DMM_HAL_H__

#ifndef __EVA_DMY__
#define __EVA_DMY__
#define eva_dmy(start,end) __RW __u32 reserved##start##_##end[end-start+1]
#endif

//===============================================================================//
//
//      SFR address mapping
//
//===============================================================================//
typedef struct {
    __RW __u32 CON0;
    __RW __u32 PND_IE;
    __WO __u32 PND_CLR;
    __RO __u32 PND;
    __RW __u32 LINE_TAR;
    __RW __u32 SLICE_CON;
    __RW __u32 SLICE_ADR;
    eva_dmy(0x07, 0x7f);
    __RW __u32 CON1;
    __RW __u32 IN0_ST;
    __RW __u32 IN0_END;
    __RW __u32 IN0_STRIDE;
    __RW __u32 IN0_BASE;
    __RW __u32 IN0_BASE_UV;
    __RW __u32 IN1_ST;
    __RW __u32 IN1_END;
    __RW __u32 IN1_STRIDE;
    __RW __u32 IN1_BASE;
    __RW __u32 IN1_BASE_UV;
    __RW __u32 OUT_STRIDE;
    __RW __u32 OUT_BASE0;
    __RW __u32 OUT_BASE1;
    __RW __u32 CCM0_CON0;
    __RW __u32 CCM0_CON1;
    __RW __u32 CCM0_CON2;
    __RW __u32 CCM1_CON0;
    __RW __u32 CCM1_CON1;
    __RW __u32 CCM1_CON2;
    __RW __u32 CCM2_CON0;
    __RW __u32 CCM2_CON1;
    __RW __u32 CCM2_CON2;
    __RW __u32 GAMMA_BASE;
} EVA_DMM_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


