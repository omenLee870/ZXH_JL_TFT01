
//========================================================================================
//
//
//  IMC SFR define
//
//
//========================================================================================
#ifndef __EVA_IMC_HAL_H__
#define __EVA_IMC_HAL_H__

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
    __RW __u32 GLOB_CON;
    __RW __u32 GLOB_STA;
    __RW __u32 GLOB_LCK;
} EVA_IMC_GLOB_typedef;

typedef struct {
    __RW __u32 CON;
    __RW __u32 SHAPE;
    __RW __u32 RBU;
} EVA_IMC_SRC_typedef;

typedef struct {
    __RW __u32 CON;
    __RW __u32 RS_STEP;
    __RW __u32 RS_HOFF;
    __RW __u32 RS_VOFF;
    __RW __u32 OSD_CON;
    __RW __u32 OSD_COL0;
    __RW __u32 OSD_COL1;
    __RW __u32 OSD_COL2;
    __RW __u32 OSD_COL3;
    __RW __u32 OSD_L0H;
    __RW __u32 OSD_L0V;
    __RW __u32 OSD_L1H;
    __RW __u32 OSD_L1V;
    __RW __u32 OSD_ADR0;
    __RW __u32 OSD_ADR1;
    __RW __u32 OSD_STRB;
    __RW __u32 DMA_CON;
    __RW __u32 DMA_SHAPE;
    __RW __u32 DMA_YST;
    __RW __u32 DMA_YED;
    __RW __u32 DMA_CST;
    __RW __u32 DMA_CED;
    __RW __u32 DMA_WSTRB;
    __RW __u32 DMA_VST;
    __RW __u32 DMA_VED;
    __RW __u32 DMA_CUR_Y;
    __RW __u32 DMA_CUR_C;
    __RW __u32 DMA_Y;
    __RW __u32 DMA_C;
    //debug
    __RW __u32 DST_Y;
    __RW __u32 DST_Y_BW;
} EVA_IMC_DST_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


