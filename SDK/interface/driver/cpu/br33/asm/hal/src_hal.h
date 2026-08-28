
#ifndef __SRC_HAL_H__
#define __SRC_HAL_H__

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
    __RW __u32 CON4;
    __RW __u32 IDAT_ADR;
    __RW __u32 IDAT_LEN;
    __RW __u32 ODAT_ADR;
    __RW __u32 ODAT_LEN;
    __RW __u32 FLTB_ADR;
    __WO __u32 ODAT_ADR_START;
    __WO __u32 ODAT_ADR_END;
    __WO __u32 STOP_FLAG;
    __RW __u32 INSR;
    __RW __u32 OUTSR;
    __RW __u32 PHASE;
    __RW __u32 FLTB_COEF;
    __RW __u32 USF_COEF;
    __RW __u32 ODAT_ADR1;
    __RW __u32 ODAT_LEN1;
    __WO __u32 ODAT_ADR1_START;
    __WO __u32 ODAT_ADR1_END;
    __RW __u32 FIR_COEF;
    __RW __u32 FIR_COEF_ADR;
} JL_SRC_TypeDef;

#endif


