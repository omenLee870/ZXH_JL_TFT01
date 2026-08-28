
//========================================================================================
//
//
//  NCHENC SFR define
//
//
//========================================================================================
#ifndef __EVA_NCHENC_HAL_H__
#define __EVA_NCHENC_HAL_H__

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
    __RW __u32 CON;
    __RW __u32 PIC_INFO;
    __RW __u32 STRIDE;
    __RW __u32 BIT_BADR;
    __RW __u32 ST_PTR0;
    __RW __u32 ST_PTR1;
    __RW __u32 BUF_SADR0;
    __RW __u32 BUF_EADR0;
    __RW __u32 BUF_SADR1;
    __RW __u32 BUF_EADR1;
    __RW __u32 VBUF_NUM;
    __RW __u32 SOFT_NUM;
    __RW __u32 BIT_LEN;
    __RW __u32 VBUF_CNT;
    __RW __u32 VBUF_REC;
} EVA_NCHENC_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


