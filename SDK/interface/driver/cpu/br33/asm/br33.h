
#ifndef __BR33__
#define __BR33__

//===============================================================================//
//
//      sfr define
//
//===============================================================================//

#define vd_base            0x7c30000
#define as_base            0x7c10000
#define wl_base            0x7e00000

#define __RW               volatile       // read write
#define __RO               volatile const // only read
#define __WO               volatile       // only write

#define __u8               unsigned int   // u8  to u32 special for struct
#define __u16              unsigned int   // u16 to u32 special for struct
#define __u32              unsigned int

#define __s8(x)            char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define __s16(x)           short(x); short(reserved_1_##x)
#define __s32(x)           int(x)

#define map_adr(grp, adr)  ((64 * grp + adr) * 4)     // grp(0x0-0xff), adr(0x0-0x3f)
#define as_map_adr(grp, adr)  (as_base + map_adr(grp, adr))

//===============================================================================//
//
//      msys sfr address define
//
//===============================================================================//
#include "./msys/msys_sfr.h"

//===============================================================================//
//
//  ass  sfr address define
//
//===============================================================================//

#define JL_ASS_MEMCFG   ((JL_COM_MEMCFG_TypeDef *)(as_map_adr(0x00, 0x00)))
#define JL_AUD          ((JL_AUD_TypeDef        *)(as_map_adr(0x01, 0x00)))
#define JL_AUDDAC       ((JL_AUDDAC_TypeDef     *)(as_map_adr(0x02, 0x00)))
#define JL_AUDADC       ((JL_AUDADC_TypeDef     *)(as_map_adr(0x03, 0x00)))
//#define JL_AD2DA        ((JL_AD2DA_TypeDef      *)(as_map_adr(0x04, 0x00)))
//#define JL_ANC          ((JL_ANC_TypeDef        *)(as_map_adr(0x05, 0x00)))
#define JL_ALNK0        ((JL_ALNK_TypeDef       *)(as_map_adr(0x06, 0x00)))
#define JL_ALNK1        ((JL_ALNK_TypeDef       *)(as_map_adr(0x07, 0x00)))
#define JL_PLNK         ((JL_PLNK_TypeDef       *)(as_map_adr(0x08, 0x00)))
//#define                 ((                      *)(as_map_adr(0x09, 0x00)))
#define JL_ADDA         ((JL_ADDA_TypeDef       *)(as_map_adr(0x10, 0x00)))
//#define JL_APA          ((JL_APA_TypeDef        *)(as_map_adr(0x11, 0x00)))
//#define JL_SBC          ((JL_SBC_TypeDef        *)(as_map_adr(0x20, 0x00)))
//#define JL_SBCRAM       ((JL_SBCRAM_TypeDef     *)(as_map_adr(0x21, 0x00)))
#define JL_AEMU         ((JL_AEMU_TypeDef       *)(as_map_adr(0x31, 0x00)))

//===============================================================================//
//
//  wl_top
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ wl_memcfg
#define JL_WL_MEMCFG_BASE          (wl_base + map_adr(0x00, 0x00))
#define JL_WL_MEMCFG               ((JL_COM_MEMCFG_TypeDef  *)JL_WL_MEMCFG_BASE)

typedef struct {
    __RW __u32 CON0;
} JL_WL_AUD_TypeDef;
#define JL_WL_AUD_BASE             (wl_base + map_adr(0x01, 0x00))
#define JL_WL_AUD                  ((JL_WL_AUD_TypeDef *)JL_WL_AUD_BASE)

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON3;
    __RW __u32 LOFC_CON;
    __RW __u32 LOFC_RES;
    __RW __u32 DBG_CON;
} JL_WL_TypeDef;
#define JL_WL_BASE                 (wl_base + map_adr(0x01, 0x01))
#define JL_WL                      ((JL_WL_TypeDef     *)JL_WL_BASE)

typedef struct {
    __RW __u32 CON;
    __RW __u32 DABUF;
    __RO __u32 DASPACE;
    __RO __u32 OFFEND;
    __RO __u32 OFFSUM;
    __RO __u32 PCMNUM;
    __RO __u32 BTTIME;
    __WO __u32 CON1  ;
    __RW __u32 CON2  ;
    __WO __u32 BT_OFFSET;
    __WO __u32 MIN_SLOT;
    __RW __u32 OUTSR;
    __WO __u32 INSR;
    __RW __u32 CON3  ;
} JL_SYNC_TypeDef;
#define JL_SYNC_BASE               (wl_base + map_adr(0x0e, 0x00))
#define JL_SYNC                    ((JL_SYNC_TypeDef *)JL_SYNC_BASE)

typedef struct {
    __RW __u32 CON0;
    __RO __u32 TIMEBASE_CNT;
    __RO __u32 SUBTIMEBASE_CNT;
    __RO __u32 CLKNCNT;
    __RO __u32 FINECNT;
    __RO __u32 MICROSECCNT;
    __RO __u32 AUDIO_NUM;
} JL_SYNC_MSGWB_TypeDef;

#define JL_SYNC_MSGWB_BASE         (wl_base + map_adr(0x0f, 0x00))
#define JL_SYNC_MSGWB              ((JL_SYNC_MSGWB_TypeDef     *)JL_SYNC_MSGWB_BASE)


#define JL_WEMU_BASE                (wl_base + map_adr(0x10, 0x00))
#define JL_WEMU                     ((JL_WEMU_TypeDef    *)JL_WEMU_BASE)

//   typedef struct {
//       __RW __u32 CON;
//       __RW __u32 IBASE;
//       __RW __u32 OBASE;
//       __RW __u32 FBASE;
//       __RW __u32 CON0;
//       __RW __u32 CON1;
//       __RW __u32 CON2;
//       __RW __u32 CON3;
//   } JL_FEC_TypeDef;
//   #define JL_FEC_BASE                 (wl_base + map_adr(0x11, 0x00))
//   #define JL_FEC                      ((JL_FEC_TypeDef     *)JL_FEC_BASE)

//===============================================================================//
//
//  video
//
//===============================================================================//

#include "eva_hal/eva_dbi_port_hal.h"
#define EVA_DBI_PORT    ((EVA_DBI_PORT_typedef*)(ls_base + map_adr(0x3f,0x00)))

#endif


