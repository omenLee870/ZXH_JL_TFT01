
#ifndef __WLA_HAL_H__
#define __WLA_HAL_H__

//typedef struct {
//    __RW __u32 WLA_CON0 ;
//    __RW __u32 WLA_CON1 ;
//    __RW __u32 WLA_CON2 ;
//    __RW __u32 WLA_CON3 ;
//    __RW __u32 WLA_CON4 ;
//    __RW __u32 WLA_CON5 ;
//    __RW __u32 WLA_CON6 ;
//    __RW __u32 WLA_CON7 ;
//    __RW __u32 WLA_CON8 ;
//    __RW __u32 WLA_CON9 ;
//    __RW __u32 WLA_CON10;
//    __RW __u32 WLA_CON11;
//    __RW __u32 WLA_CON12;
//    __RW __u32 WLA_CON13;
//    __RW __u32 WLA_CON14;
//    __RW __u32 WLA_CON15;
//    __RW __u32 WLA_CON16;
//    __RW __u32 WLA_CON17;
//    __RW __u32 WLA_CON18;
//    __RW __u32 WLA_CON19;
//    __RW __u32 WLA_CON20;
//    __RW __u32 WLA_CON21;
//    __RW __u32 WLA_CON22;
//    __RW __u32 WLA_CON23;
//    __RW __u32 WLA_CON24;
//    __RW __u32 WLA_CON25;
//    __RW __u32 WLA_CON26;
//    __RW __u32 WLA_CON27;
//    __RW __u32 WLA_CON28;
//    __RW __u32 WLA_CON29;
//    __RO __u32 WLA_CON30;
//    __RW __u32 WLA_CON31;
//    __RW __u32 WLA_CON32;
//    __RW __u32 WLA_CON33;
//    __RW __u32 WLA_CON34;
//    __RW __u32 WLA_CON35;
//    __RW __u32 WLA_CON36;
//    __RW __u32 WLA_CON37;
//    __RW __u32 WLA_CON38;
//    __RW __u32 WLA_CON39;
//    __RW __u32 WLA_CON40;
//    __RW __u32 WLA_CON41;
//    __RW __u32 WLA_CON42;
//    __RW __u32 WLA_CON43;
//    __RW __u32 WLA_CON44;
//    __RW __u32 WLA_CON45;
//    __RW __u32 WLA_CON46;
//} JL_WLA_TypeDef;

//===================================================//
//  LCO_CNT
//===================================================//
typedef struct {
    __RW __u32 CON0 ;
    __RW __u32 CON1 ;
    __RW __u32 CON2 ;
    __RW __u32 CON3 ;
    __RW __u32 CON4 ;
    __RW __u32 CON5 ;
    __RW __u32 CON6 ;
    __RW __u32 CON7 ;
    __RW __u32 CON8 ;
    __RW __u32 CON9 ;
    __RW __u32 CON10;
    __RW __u32 CON11;
    __RW __u32 CON12;
    __RW __u32 CON13;
    __RO __u32 CON14_RO;
    __RO __u32 CON15_RO;
    __RO __u32 CON16_RO;
    __WO __u32 CON17_WO;
} JL_LCO_CNT_TypeDef;

//===================================================//

//===================================================//
//  TPM_CON
//===================================================//
typedef struct {
    __RW __u32 CON0 ;
    __RW __u32 CON1 ;
    __RW __u32 CON2 ;
    __RW __u32 CON3 ;
    __RW __u32 CON4 ;
    __RW __u32 CON5 ;
    __RW __u32 CON6 ;
    __RW __u32 CON7 ;
    __RW __u32 CON8 ;
    __RW __u32 CON9 ;
    __RW __u32 CON10;
    __RO __u32 CON_RO0;
} JL_TPM_TypeDef;

//===================================================//


#endif


