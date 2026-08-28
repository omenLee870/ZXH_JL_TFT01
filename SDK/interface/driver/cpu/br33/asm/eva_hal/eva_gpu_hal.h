
//========================================================================================
//
//
//  GPU SFR define
//
//
//========================================================================================
#ifndef __EVA_GPU_HAL_H__
#define __EVA_GPU_HAL_H__

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
    __RO __u32 PND_CON;
    __WO __u32 PND_CLR;
    __RW __u32 PND_IE;
    __RW __u32 COM_CON;
    __RW __u32 STA_CON;
    __RW __u32 OUT_FMT;
    __RW __u32 WIN_X;
    __RW __u32 WIN_Y;
    __RW __u32 OUT_STRIDE;
    __RW __u32 WIN_BASE;
    __RW __u32 NXT_LAYER_CNT;
    __RW __u32 CUR_LAYER_CNT;
    __RW __u32 NXT_INST_ADR;
    __RW __u32 CUR_INST_ADR;
    __RW __u32 CKEY_RGB;
    __RW __u32 UV_BASE_ADR;
    __RW __u32 VTX0_TCNT;
    __RW __u32 VTX1_TCNT;
    __RW __u32 FRAG_TCNT;
    __RW __u32 MARK_TCNT;
    __RW __u32 RAST_TCNT;
    __RW __u32 VTX_CON0;
    __RW __u32 VTX_CON1;
    __RW __u32 EBUF_BASE;
    __RW __u32 CBUF_BASE;
    __RW __u32 GBT_CON;
    __RW __u32 GBT_FW_M00;
    __RW __u32 GBT_FW_M01;
    __RW __u32 GBT_FW_M02;
    __RW __u32 GBT_FW_M10;
    __RW __u32 GBT_FW_M11;
    __RW __u32 GBT_FW_M12;
    __RW __u32 GBT_FW_M20;
    __RW __u32 GBT_FW_M21;
    __RW __u32 GBT_FW_M22;
    __RW __u32 GBT_BW_M00;
    __RW __u32 GBT_BW_M01;
    __RW __u32 GBT_BW_M02;
    __RW __u32 GBT_BW_M10;
    __RW __u32 GBT_BW_M11;
    __RW __u32 GBT_BW_M12;
    __RW __u32 GBT_BW_M20;
    __RW __u32 GBT_BW_M21;
    __RW __u32 GBT_BW_M22;
    __RW __u32 SPH_CON;
    __RW __u32 SPH_INFO;
    __RW __u32 SPH_RD_ADR;
    __RW __u32 SPH_WR_ADR;
    __RW __u32 SPH_M00;
    __RW __u32 SPH_M01;
    __RW __u32 SPH_M02;
    __RW __u32 SPH_M03;
    __RW __u32 SPH_M10;
    __RW __u32 SPH_M11;
    __RW __u32 SPH_M12;
    __RW __u32 SPH_M13;
    __RW __u32 SPH_M20;
    __RW __u32 SPH_M21;
    __RW __u32 SPH_M22;
    __RW __u32 SPH_M23;
    __RW __u32 TOP_CON;
    __RW __u32 TOP_INFO;
    __RW __u32 TOP_OFS;
    __RW __u32 BUF_INFO;
    __RW __u32 BUF_BASE0;
    __RW __u32 BUF_BASE1;
} EVA_GPU_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


