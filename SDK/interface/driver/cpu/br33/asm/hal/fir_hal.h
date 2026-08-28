
#ifndef __FIR_HAL_H__
#define __FIR_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 ADR;
} JL_FIR_TypeDef;

typedef struct {
    unsigned int    FIRBUF_ADR;
    unsigned int    CHL_CON;
    unsigned int    IBUF_ADR;  // idat buf adr
    unsigned short  IBUF_LEN;  // idat buf len
    unsigned char   IBUF_INC;  // idac buf inc
    unsigned char   FIR_DSF;
    unsigned int    CBUF_ADR;  // coef buf adr
    unsigned short  CBUF_TAP;  // coef buf tap
    unsigned char   OBUF_INC;  // odat buf inc
    unsigned char   RESERVE1;
    unsigned int    OBUF_ADR;  // odat buf adr
    unsigned short  IBUF_EPTR; // idat buf end ptr
    unsigned short  RESERVE2;
} JL_FIRPARA_TypeDef;

typedef struct {
    unsigned short  IBUF_PTR;
    unsigned short  CBUF_PTR;
    unsigned int    LOOP_CNT;
    unsigned int    FIR_BUF[68];
} JL_FIRBUF_TypeDef;





#endif


