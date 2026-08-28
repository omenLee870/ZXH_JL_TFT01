
#ifndef __AUDIO_HAL_H__
#define __AUDIO_HAL_H__

//===============================================================================//
//
//  ass  sfr address define
//
//===============================================================================//
typedef struct {
    __RW __u32 ASS_CON;
    __RW __u32 AUD_CON0;
    __RW __u32 AUD_CON1;
    __RW __u32 AUD_CON2;
    __RW __u32 AUD_CON3;
    __RW __u32 AUD_CON4;
    __RW __u32 AUD_CON5;
    __RW __u32 AUD_COP;
    __RW __u32 DAC_COP;
} JL_AUD_TypeDef;


typedef struct {
    __RW __u32(DAC_CON0);
    __RW __u32(DAC_CON1);
    __RW __u32(DAC_CON2);
    __RW __u32(DAC_CON3);
    __RW __u32(DAC_CON4);
    __RW __u32(DAC_CON5);
    __RW __u32(DAC_CON6);
    __RW __u32(DAC_CON7);
    __RW __u32(DAC_CON8);
    __RW __u32(DAC_CON9);
    __RW __u32(DAC_CON10);
    __RW __u32(COEF_CON);
    __RW __u32(USF0_COEF);
    __RW __u32(USF0_COEF_ADR);
    __RW __u32(USF1_COEF);
    __RW __u32(USF1_COEF_ADR);
    __RW __u32(SRC_COEF);
    __RW __u32(SRC_COEF_ADR);
    __RW __u32(DAC_VL0);
    __RW __u32(DAC_VL1);
    __RW __u32(DAC_DIT);
    __RW __u32(DAC_TM0);
    __RW __u32(DAC_TM1);
    __WO __u32(DAC_DCW0);
    __WO __u32(DAC_DCW1);
    __RO __u32(DAC_DCR0);
    __RO __u32(DAC_DCR1);

    __RW __u32(DAC_ADR);
    __RW __u16(DAC_LEN);
    __RW __u16(DAC_PNS);
    __RW __u16(DAC_HRP);
    __RW __u16(DAC_SWP);
    __RW __u16(DAC_SWN);
} JL_AUDDAC_TypeDef;


typedef struct {
    __RW __u32(ADC_CON0);
    __RW __u32(ADC_CON1);
    __RW __u32(ADC_CON2);
    __RW __u32(ADC_CON3);
    __RW __u32(ADC_CON4);
    __RW __u32(ADC_CON5);
    __RW __u32(ADC_CON6);
    __RW __u32(ADC_CON7);
    __RW __u32(ADC_CON8);
    __RW __u32(ADC_CON9);
    __RW __u32(ADC_CON10);
    __RW __u32(ADC_CON11);
    __RW __u32(COEF_CON);
    __RW __u32(DSF0_COEF);
    __RW __u32(DSF0_COEF_ADR);
    __RW __u32(DSF1_COEF);
    __RW __u32(DSF1_COEF_ADR);
    __WO __u32(ADC_DCW0);
    __WO __u32(ADC_DCW1);
    __WO __u32(ADC_DCW2);
    __WO __u32(ADC_DCW3);
    __RO __u32(ADC_DCR0);
    __RO __u32(ADC_DCR1);
    __RO __u32(ADC_DCR2);
    __RO __u32(ADC_DCR3);

    __RW __u32(ADC_ADR);
    __RW __u16(ADC_LEN);
    __RW __u16(ADC_PNS);
    __RW __u16(ADC_HWP);
    __RW __u16(ADC_SRP);
    __RW __u16(ADC_SRN);
} JL_AUDADC_TypeDef;


typedef struct {
    __RW __u16 CON0;
    __RW __u16 CON1;
    __RW __u16 CON2;
    __RW __u32 CON3;
    __RW __u32 CON4;
    __RW __u32 CON5;
    __RW __u32 ADR0;
    __RW __u32 ADR1;
    __RW __u32 ADR2;
    __RW __u32 ADR3;
    __RW __u32 LEN01;
    __RW __u32 LEN23;
    __RW __u32 PNS0;
    __RW __u32 PNS1;
    __RW __u32 PNS2;
    __RW __u32 PNS3;
    __RW __u32 HWPTR0;
    __RW __u32 HWPTR1;
    __RW __u32 HWPTR2;
    __RW __u32 HWPTR3;
    __RW __u32 SWPTR0;
    __RW __u32 SWPTR1;
    __RW __u32 SWPTR2;
    __RW __u32 SWPTR3;
    __RW __u32 SHN0;
    __RW __u32 SHN1;
    __RW __u32 SHN2;
    __RW __u32 SHN3;
    __RW __u32 BLOCK0;
    __RW __u32 BLOCK1;
    __RW __u32 BLOCK2;
    __RW __u32 BLOCK3;
} JL_ALNK_TypeDef;



typedef struct {
    __RW __u32 CON;
    //__RW __u8  SMR;
    __RW __u32 ADR;
    __RW __u32 LEN;
    __RW __u16 DOR;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __WO __u32 DCW0;
    __WO __u32 DCW1;
    __RO __u32 DCR0;
    __RO __u32 DCR1;
} JL_PLNK_TypeDef;


typedef struct {
    __RW __u32 DAA_CON0;
    __RW __u32 DAA_CON1;
    __RW __u32 DAA_CON2;
    __RW __u32 DAA_CON3;
    __RW __u32 DAA_CON4;
    __RW __u32 DAA_CON5;
    __RW __u32 DAA_CON6;
    __RO __u32 DAA_CON7;
    __RW __u32 ADA_CON0;
    __RW __u32 ADA_CON1;
    __RW __u32 ADA_CON2;
    __RW __u32 ADA_CON3;
    __RW __u32 ADA_CON4;
    __RW __u32 ADA_CON5;
    __RW __u32 ADA_CON6;
    __RW __u32 ADA_CON7;
    __RW __u32 ADDA_CON0;
    __RW __u32 ADDA_CON1;
} JL_ADDA_TypeDef;


typedef struct {
    __RW __u32 CON0;
    __WO __u32 DEC_SRC_ADR;
    __WO __u32 DEC_DST_ADR;
    __WO __u32 DEC_PCM_WCNT;
    __WO __u32 DEC_INBUF_LEN;
    __WO __u32 ENC_SRC_ADR;
    __WO __u32 ENC_DST_ADR;
    __RO __u32 DEC_DST_BASE;
    __WO __u32 DEC_WEIGHT_BASE;
    __WO __u32 DEC_WEIGHT_ADD;

} JL_SBC_TypeDef;


typedef struct {
    __RW __u32 DATA[384];
} JL_SBCRAM_TypeDef;

#endif

