
//========================================================================================
//
//
//  GXDMA SFR define
//
//
//========================================================================================
#ifndef __EVA_GXDMA_HAL_H__
#define __EVA_GXDMA_HAL_H__

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
    __RW __u32 SRC_FMT;
    __RW __u32 SRC_PIC;
    __RW __u32 SRC_X_MIN;
    __RW __u32 SRC_X_MAX;
    __RW __u32 SRC_Y_MIN;
    __RW __u32 SRC_Y_MAX;
    __RW __u32 SRC_STRIDE;
    __RW __u32 SRC_BADR;
    __RW __u32 DST_FMT;
    __RW __u32 DST_STRIDE;
    __RW __u32 DST_BADR;
    __RW __u32 DST_BADR_UV;
} EVA_GXDMA_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


