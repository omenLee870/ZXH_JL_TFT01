
//========================================================================================
//
//
//  CORE SFR define
//
//
//========================================================================================
#ifndef __EVA_CORE_H__
#define __EVA_CORE_H__

#ifndef __EVA_DMY__
#define __EVA_DMY__
#define eva_dmy(start,end) __RW __u32 reserved##start##_##end[end-start+1]
#endif

//===============================================================================//
//
//      public
//
//===============================================================================//
#define __RW        volatile        // read write
#define __RO        volatile const  // only read
#define __WO        volatile        // only write

#define __u8        unsigned int    // u8  to u32 special for struct
#define __u16       unsigned int    // u16 to u32 special for struct
#define __u32       unsigned int

#ifdef SIM_DIP
#define eva_base    dip_prp0_base
#else
#define eva_base    vd_base
#endif

#define eva_map_adr(cid,offs)   (eva_base + (cid)*0x800 + (offs)*4)

//  mount device
//******************************
typedef enum {
    CID_XBUS
    , CID_IPU_VBUS
    , CID_VPU_VBUS
    , CID_DPU_VBUS
    , CID_ISC
    , CID_REP
    , CID_IMC
    , CID_JPG
    , CID_JXENC
    , CID_NCHENC
    , CID_GPU
    , CID_GXDEC
    , CID_GXDMA
    , CID_DMM
    , CID_DBI
    , CID_DSI
} XCFG_ENUM;

//===============================================================================//
//
//      xbus
//
//===============================================================================//
typedef struct {
    __RO __u32 PND_CON;
    __WO __u32 PND_CLR;
    __RW __u32 PND_IE;
    __RO __u32 ERR_WMID;
    __RO __u32 ERR_WADR;
    __RO __u32 ERR_RMID;
    __RO __u32 ERR_RADR;
} EVA_XBUS_COM_typedef;

typedef struct {
    __RW __u32 CH0;
    __RW __u32 CH1;
    __RW __u32 CH2;
    __RW __u32 CH3;
    __RW __u32 CH4;
    __RW __u32 CH5;
    __RW __u32 CH6;
    __RW __u32 CH7;
} EVA_XBUS_SLV_typedef;

typedef struct {
    __RW __u32 WID_IMC_DST0;
    __RW __u32 WID_IMC_DST1;
    __RW __u32 WID_JPG;
    __RW __u32 WID_JXENC;
    __RW __u32 WID_GPU_WDMA;
    __RW __u32 WID_GXDMA;
    __RW __u32 WID_DMM;
} EVA_XBUS_WMST_typedef;

typedef struct {
    __RW __u32 RID_REP;
    __RW __u32 RID_IMC_OSD0;
    __RW __u32 RID_JPG;
    __RW __u32 RID_GPU_INST;
    __RW __u32 RID_GPU_TEXT;
    __RW __u32 RID_GPU_MASK;
    __RW __u32 RID_GPU_RDMA;
    __RW __u32 RID_GXDEC;
    __RW __u32 RID_GXDMA;
    __RW __u32 RID_DMM;
    __RW __u32 RID_DBI;
} EVA_XBUS_RMST_typedef;

#define EVA_XBUS_COM    ((EVA_XBUS_COM_typedef  *)(eva_map_adr(CID_XBUS,0)))
#define EVA_XBUS_X2C    ((EVA_XBUS_SLV_typedef  *)(eva_map_adr(CID_XBUS,32)))
#define EVA_XBUS_X2A    ((EVA_XBUS_SLV_typedef  *)(eva_map_adr(CID_XBUS,40)))
#define EVA_XBUS_WMST   ((EVA_XBUS_WMST_typedef *)(eva_map_adr(CID_XBUS,64)))
#define EVA_XBUS_RMST   ((EVA_XBUS_RMST_typedef *)(eva_map_adr(CID_XBUS,96)))

#define x2c_wmst_cfg(mst,ch,grp) SFR(EVA_XBUS_WMST->mst,0,4,(ch<<1)|grp)
#define x2c_rmst_cfg(mst,ch,grp) SFR(EVA_XBUS_RMST->mst,0,4,(ch<<1)|grp)

#define x2a_wmst_cfg(mst,ch,grp) SFR(EVA_XBUS_WMST->mst,4,4,(ch<<1)|grp)
#define x2a_rmst_cfg(mst,ch,grp) SFR(EVA_XBUS_RMST->mst,4,4,(ch<<1)|grp)

//===============================================================================//
//
//      VBUS
//
//===============================================================================//
typedef struct {
    __RO __u32 EXCP_PND_CON;
    __WO __u32 EXCP_PND_CLR;
    __RW __u32 EXCP_PND_IE;
    __RO __u32 SUSP_PND_CON;
    __WO __u32 SUSP_PND_CLR;
    __RW __u32 SUSP_PND_IE;
} EVA_VBUS_COM_typedef;

//**********************************
typedef enum {
    IPU_SID_SEN0
    , IPU_SID_REP0
    , IPU_SID_REP1
} enum_ipu_sid;

typedef struct {
    __RW __u32 IPU_TID_IMC0;
    __RW __u32 IPU_TID_IMC1;
} EVA_IVBUS_TCH_typedef;

#define EVA_IVBUS_COM   ((EVA_VBUS_COM_typedef  *)(eva_map_adr(CID_IPU_VBUS,0)))
#define EVA_IVBUS_TCON  ((EVA_IVBUS_TCH_typedef *)(eva_map_adr(CID_IPU_VBUS,32)))
#define EVA_IVBUS_TSEL  ((EVA_IVBUS_TCH_typedef *)(eva_map_adr(CID_IPU_VBUS,64)))

//**********************************
typedef enum {
    VPU_SID_IMC0
    , VPU_SID_IMC1
} enum_vpu_sid;

typedef struct {
    __RW __u32 VPU_TID_JPG0;
    __RW __u32 VPU_TID_JPG_V0;
    __RW __u32 VPU_TID_JPG_V1;
} EVA_VVBUS_TCH_typedef;

#define EVA_VVBUS_COM   ((EVA_VBUS_COM_typedef  *)(eva_map_adr(CID_VPU_VBUS,0)))
#define EVA_VVBUS_TCON  ((EVA_VVBUS_TCH_typedef *)(eva_map_adr(CID_VPU_VBUS,32)))
#define EVA_VVBUS_TSEL  ((EVA_VVBUS_TCH_typedef *)(eva_map_adr(CID_VPU_VBUS,64)))

//**********************************
typedef enum {
    DPU_SID_GPU
    , DPU_SID_DMM
} enum_dpu_sid;

typedef struct {
    __RW __u32 DPU_TID_GXDMA;
    __RW __u32 DPU_TID_DBI;
} EVA_DVBUS_TCH_typedef;

#define EVA_DVBUS_COM   ((EVA_VBUS_COM_typedef  *)(eva_map_adr(CID_DPU_VBUS,0)))
#define EVA_DVBUS_TCON  ((EVA_DVBUS_TCH_typedef *)(eva_map_adr(CID_DPU_VBUS,32)))
#define EVA_DVBUS_TSEL  ((EVA_DVBUS_TCH_typedef *)(eva_map_adr(CID_DPU_VBUS,64)))

//===============================================================================//
//
//      IPU
//
//===============================================================================//
//  ISC
//**********************************
#include "eva_hal/eva_isc_hal.h"
#define EVA_ISC_COM     ((EVA_ISC_COM_typedef   *)(eva_map_adr(CID_ISC,0x00)))
#define EVA_ISC_SEN0    ((EVA_ISC_SEN_typedef   *)(eva_map_adr(CID_ISC,0x08)))

//  REP
//**********************************
#include "eva_hal/eva_rep_hal.h"
#define EVA_REP_COM     ((EVA_REP_COM_typedef   *)(eva_map_adr(CID_REP,0x00)))
#define EVA_REP_CH0     ((EVA_REP_CHN_typedef   *)(eva_map_adr(CID_REP,0x10)))
#define EVA_REP_CH1     ((EVA_REP_CHN_typedef   *)(eva_map_adr(CID_REP,0x20)))

//  IMC
//**********************************
#include "eva_hal/eva_imc_hal.h"
#define EVA_IMC_GLOB    ((EVA_IMC_GLOB_typedef  *)(eva_map_adr(CID_IMC,0x00)))
#define EVA_IMC_SRC0    ((EVA_IMC_SRC_typedef   *)(eva_map_adr(CID_IMC,0x20)))
#define EVA_IMC_SRC1    ((EVA_IMC_SRC_typedef   *)(eva_map_adr(CID_IMC,0x40)))
#define EVA_IMC_DST0    ((EVA_IMC_DST_typedef   *)(eva_map_adr(CID_IMC,0x100)))
#define EVA_IMC_DST1    ((EVA_IMC_DST_typedef   *)(eva_map_adr(CID_IMC,0x120)))

//===============================================================================//
//
//      VPU
//
//===============================================================================//
//  JPG
//**********************************
#include "eva_hal/eva_jpg_hal.h"
#define EVA_JPG0        ((EVA_JPG_typedef       *)(eva_map_adr(CID_JPG,0x00)))

//  JXENC
//**********************************
#include "eva_hal/eva_jxenc_hal.h"
#define EVA_JXENC       ((EVA_JXENC_typedef     *)(eva_map_adr(CID_JXENC,0x00)))

//  NCHENC
//**********************************
#include "eva_hal/eva_nchenc_hal.h"
#define EVA_NCHENC_CH0  ((EVA_NCHENC_typedef    *)(eva_map_adr(CID_NCHENC,0)))
#define EVA_NCHENC_CH1  ((EVA_NCHENC_typedef    *)(eva_map_adr(CID_NCHENC,64)))

//===============================================================================//
//
//      DPU
//
//===============================================================================//
//  GPU
//**********************************
#include "eva_hal/eva_gpu_hal.h"
#define EVA_GPU         ((EVA_GPU_typedef       *)(eva_map_adr(CID_GPU,0x00)))

//  GXDEC
//**********************************
#include "eva_hal/eva_gxdec_hal.h"
#define EVA_GXDEC       ((EVA_GXDEC_typedef     *)(eva_map_adr(CID_GXDEC,0x00)))

//  GXDMA
//**********************************
#include "eva_hal/eva_gxdma_hal.h"
#define EVA_GXDMA       ((EVA_GXDMA_typedef     *)(eva_map_adr(CID_GXDMA,0x00)))

//  DMM
//**********************************
#include "eva_hal/eva_dmm_hal.h"
#define EVA_DMM         ((EVA_DMM_typedef       *)(eva_map_adr(CID_DMM,0x00)))

//  DBI
//**********************************
#include "eva_hal/eva_dbi_hal.h"
#define EVA_DBI         ((EVA_DBI_typedef       *)(eva_map_adr(CID_DBI,0x00)))

//  DSI
//**********************************
#include "eva_hal/eva_dsi_hal.h"
#define EVA_DSI         ((EVA_DSI_typedef       *)(eva_map_adr(CID_DSI,0x00)))

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif

