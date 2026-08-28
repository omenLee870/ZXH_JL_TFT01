
//========================================================================================
//
//
//  JPG SFR define
//
//
//========================================================================================
#ifndef __EVA_JPG_HAL_H__
#define __EVA_JPG_HAL_H__

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
    __RW __u32 COM_CON;
    __RW __u32 DUMMY1;
    __RW __u32 DUMMY2;
    __RW __u32 DUMMY3;
    __RO __u32 PND_CON;
    __WO __u32 PND_CLR;
    __RW __u32 PND_IE;
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 YDCVAL;
    __RW __u32 UDCVAL;
    __RW __u32 VDCVAL;
    __RW __u32 MCUCNT;
    __RW __u32 PRECNT;
    __RW __u32 DRICNT;
    __RW __u32 BIT_LEN;
    __RW __u32 CFGRAMADDR;
    __RW __u32 CFGRAMVAL;
    __RW __u32 DMA_CON;
    __RW __u32 BIT_CON;
    __RW __u32 YUV_CON;
    __RW __u32 BADDR;
    __RW __u32 BCNT;
    __RW __u32 PIC_INFO;
    __RW __u32 STRIDE;
    __RW __u32 ST_PTR0;
    __RW __u32 ST_PTR1;
    __RW __u32 BUF_SADR0;
    __RW __u32 BUF_EADR0;
    __RW __u32 BUF_SADR1;
    __RW __u32 BUF_EADR1;
    __RW __u32 VBUF_NUM;
    __RW __u32 VBUF_CNT;
    __RW __u32 VBUF_REC;
    __RW __u32 SOFT_NUM;
} EVA_JPG_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


