
//========================================================================================
//
//
//  DBI PORT SFR define
//
//
//========================================================================================
#ifndef __EVA_DBI_PORT_HAL_H__
#define __EVA_DBI_PORT_HAL_H__

#ifndef __EVA_DMY__
#define __EVA_DMY__
#define eva_dmy(start,end) __RW __u32 reserved##start##_##end[end-start+1]
#endif

//===============================================================================//
//
//      config code mapping
//
//===============================================================================//
typedef enum {
    DBI_D0
    , DBI_D1
    , DBI_D2
    , DBI_D3
    , DBI_D4
    , DBI_D5
    , DBI_D6
    , DBI_D7
    , DBI_D8
    , DBI_D9
    , DBI_D10
    , DBI_D11
    , DBI_D12
    , DBI_D13
    , DBI_D14
    , DBI_D15
    , DBI_D16
    , DBI_D17
    , DBI_D18
    , DBI_D19
    , DBI_D20
    , DBI_D21
    , DBI_D22
    , DBI_D23
    , DBI_SCL
    , DBI_RWX
    , DBI_DCX
    , DBI_CSX
} DBI_PORT_ENUM;

//===============================================================================//
//
//      SFR address mapping
//
//===============================================================================//
typedef struct {
    __RW __u32 IN0   ;
    __RW __u32 IN1   ;
    __RW __u32 IN2   ;
    __RW __u32 IN3   ;
    __RW __u32 IN4   ;
    __RW __u32 IN5   ;
    __RW __u32 IN6   ;
    __RW __u32 IN7   ;
    __RW __u32 OUT0  ;
    __RW __u32 OUT1  ;
    __RW __u32 OUT2  ;
    __RW __u32 OUT3  ;
    __RW __u32 OUT4  ;
    __RW __u32 OUT5  ;
    __RW __u32 OUT6  ;
    __RW __u32 OUT7  ;
    __RW __u32 OUT8  ;
    __RW __u32 OUT9  ;
    __RW __u32 OUT10 ;
    __RW __u32 OUT11 ;
    __RW __u32 OUT12 ;
    __RW __u32 OUT13 ;
    __RW __u32 OUT14 ;
    __RW __u32 OUT15 ;
    __RW __u32 OUT16 ;
    __RW __u32 OUT17 ;
    __RW __u32 OUT18 ;
    __RW __u32 OUT19 ;
    __RW __u32 OUT20 ;
    __RW __u32 OUT21 ;
    __RW __u32 OUT22 ;
    __RW __u32 OUT23 ;
    __RW __u32 OUT24 ;
    __RW __u32 OUT25 ;
    __RW __u32 OUT26 ;
    __RW __u32 OUT27 ;
    __RW __u32 OUT28 ;
    __RW __u32 OUT29 ;
    __RW __u32 OUT30 ;
} EVA_DBI_PORT_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


