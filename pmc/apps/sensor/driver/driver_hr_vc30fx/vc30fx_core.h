
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-02-28 10:10:43
 * @LastEditTime: 2024-01-05 10:07:32
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#ifndef __VC30Fx_CORE_H__
#define __VC30Fx_CORE_H__

#ifndef VC_PACKED
#define VC30Fx_PACKED //__attribute__((packed))
#endif

/*===============================================================
regist_list_map start...
================================================================*/
#define CORE30Fx_DEV_VER_ID_H_ADDR (0x00)
#define CORE30Fx_FDY_EXT_ID_L_ADDR (0x01)
#define CORE30Fx_WORK_STATUS1_ADDR (0x02)

/****************************************************************
 * ic sample param structure, 0x08 - 0x23 = 28byte
 ***************************************************************/
typedef struct {
    /* env data, 4byte_10bit[9,0] */
    unsigned char slot0_envh : 2;
    unsigned char slot1_envh : 2;
    unsigned char slot2_envh : 2;
    unsigned char reserved : 1;
    unsigned char slot0_envl;
    unsigned char slot1_envl;
    unsigned char slot2_envl;
    /* ps data  3byte */
    unsigned char slot0_ps;
    unsigned char slot1_ps;
    unsigned char slot2_ps;
    /* bio data 4byte */
    unsigned char bioinn_valh;
    unsigned char bioinn_vall;
    unsigned char bioext_valh;
    unsigned char bioext_vall;
    /* temp data 6byte */
    unsigned char tempinn_valh; /* temp0-inside */
    unsigned char tempinn_vall;
    unsigned char tempext_valh; /* temp1-external */
    unsigned char tempext_vall;
    unsigned char tempref_valh; /* temp_ext_ref */
    unsigned char tempref_vall;
} VC30Fx_PACKED  core30fx_reg_result_t; //core30fx_reg_result_t

typedef struct {
    /* env data  3byte */
    unsigned char slot0_envl;
    unsigned char slot1_envl;
    unsigned char slot2_envl;
    /* ps data  3byte */
    unsigned char slot0_ps;
    unsigned char slot1_ps;
    unsigned char slot2_ps;
} VC30Fx_PACKED  core30fc_reg_result_t; //core30fc_reg_result_t [12S]

/****************************************************************
 * ic config param structure.[SLOT_CONFIG]
 ***************************************************************/
#define CORE30Fx_CONFIG_SLOT_ADDR_BASE (0x40)
#define CORE30Fx_CONFIG_SLOT_SIZE (0x04) // 4*2=8,(cfg0-cfg7)

typedef struct {
    unsigned char oversample : 3;
    unsigned char reserved : 1;
    unsigned char rxgain : 3;
    unsigned char enable : 1;
} VC30Fx_PACKED core30fx_slot_ppg_cfg0bit; //core30fx_slot_ppg_cfg0bit
typedef struct {
    unsigned char deps_dirturb : 3; /* vc30fc:[3:3]enable :[2:0]dirturb, vc30fs: [3:2]reserved, [1:0] dirturb */
    unsigned char deps_enable : 1;
    unsigned char rxgain : 3;
    unsigned char enable : 1;
} VC30Fx_PACKED core30fx_slot_ps_cfg0bit;
typedef struct {
    unsigned char env_time : 4;
    unsigned char sample_time : 2;
    unsigned char afech : 2;
} VC30Fx_PACKED core30fx_slot_ppg_cfg1bit;
typedef struct {
    unsigned char env_time : 4;
    unsigned char sample_time : 2;
    unsigned char reserved : 1;
    unsigned char storefifo : 1;
} VC30Fx_PACKED core30fx_slot_ps_cfg1bit;
typedef struct {
    unsigned char current : 7;
    unsigned char reserved : 1;
} VC30Fx_PACKED core30fx_slot_cfg2bit;

typedef struct {
    unsigned char sc : 4;
    unsigned char resistance : 4;
} VC30Fx_PACKED core30fx_slot_cfg3bit;
/*---[slot regist map]---*/
typedef struct {
    union { /* 1_byte */
        unsigned char byte;
        core30fx_slot_ppg_cfg0bit bits;
        core30fx_slot_ps_cfg0bit ps_bits;
    } slot_cfg0;
    union { /* 2_byte */
        unsigned char byte;
        core30fx_slot_ppg_cfg1bit bits;
        core30fx_slot_ps_cfg1bit ps_bits;
    } slot_cfg1;
    union { /* 3_byte */
        unsigned char byte;
        core30fx_slot_cfg2bit bits;
    } slot_cfg2;
    union { /* 4_byte */
        unsigned char byte;
        core30fx_slot_cfg3bit bits;
    } slot_cfg3;
} VC30Fx_PACKED core30fx_slot_config_t;

/****************************************************************
 * ic config param structure.[IRQ_RATE_CONFIG+IRQ_CONFIG]
 ***************************************************************/
#define CORE30Fx_INT_RATE_IRQ_CONFIG_ADDR_BASE (0x4C)
#define CORE30Fx_INT_RATE_IRQ_CONFIG_SIZE (7)

typedef struct {
    unsigned char cpd : 3;
    unsigned char fifo_en : 1;
    unsigned char adcspeed : 1;
    unsigned char com_bit : 2; /* 30FS(0x10)-[1:0]~[enI2CRelease:deflick], 30FC-12S(0x10)-[1:0]~[reserved:envDiv], 30FS-12S(0x30)-[1:0]~[PDsel] */
    unsigned char deenv_en : 1;
} VC30Fx_PACKED core30fx_com_cfgbit;
/* samp_freq1H,samp_freq1L,samp_freq2,fifo_freq3 */
typedef struct {
    unsigned char fifo_div : 6;
    unsigned char reserved : 1;
    unsigned char env_clkdiv : 1;
} VC30Fx_PACKED core30fx_fifodiv_cfgbit;

typedef struct {
    unsigned char ppgthl : 4;
    unsigned char ppgthh : 4;
} VC30Fx_PACKED core30fx_ppgth_cfgbit;

typedef struct {
    unsigned char intppg_en : 1;
    unsigned char intov_en : 1;
    unsigned char intfifo_en : 1;
    unsigned char intfreq2_en : 1;
    unsigned char intlen : 2;
    unsigned char intdir : 1;
    unsigned char intmode : 1;
} VC30Fx_PACKED core30fx_int_cfgbit;

typedef struct {
    /* 1_byte:com&pdcfg */
    union {
        unsigned char byte;
        core30fx_com_cfgbit bits;
    } com_cfg;
    /* 2~5_byte */
    unsigned char samp_freq1H;
    unsigned char samp_freq1L;  /* ppg_freq = 20K / rete1 */
    unsigned char samp_freq2;   /* ps_freq = ppg_freq / rate2 */
    union {
        unsigned char byte;
        core30fx_fifodiv_cfgbit bits;
    } fifo_freq3;               /* fifo_freq = ps_freq / rate3 */
    /* 6_byte */
    union {
        unsigned char byte;
        core30fx_ppgth_cfgbit bits;
    } ppgth_cfg;
    /* 5_byte */
    union {
        unsigned char byte;
        core30fx_int_cfgbit bits;
    } int_cfg;
} VC30Fx_PACKED core30fx_common_config_t;

/****************************************************************
 * ic config param structure.[EXTRA_CONFIG]
 ***************************************************************/
#define CORE30Fx_EXTRA_CONFIG_ADDR_BASE (0x53)
#define CORE30Fx_EXTRA_CONFIG_SIZE (3)

typedef struct {
    /* byte1 */
    unsigned char bio_freq : 2;
    unsigned char bio_reserved_1 : 2;
    unsigned char bio_mode : 1;  /*  0：为TP⼯作在PS频率下; 1: 为TP⼯作在ppg频率下，少量多次求和，仍在PS频率下发出中断。 */
    unsigned char bio_infifo : 1;
    unsigned char bio_ext_en : 1;
    unsigned char bio_inn_en : 1;
} VC30Fx_PACKED core30fx_bio_cfg0bit;
typedef struct {
    unsigned char bio_time : 3;
    unsigned char bio_capadd : 5;
} VC30Fx_PACKED core30fx_bio_cfg1bit;

typedef struct {
    unsigned char temp_currset : 3;
    unsigned char temp_reserved_1 : 1;
    unsigned char temp_ext_en: 1;
    unsigned char temp_inn_en: 1;
    unsigned char temp_reserved_2 : 2;
} VC30Fx_PACKED core30fx_temp_cfgbit;

typedef struct {
    union { /* 1_byte */
        unsigned char byte;
        core30fx_bio_cfg0bit bits;
    } bio_cfg0;
    union { /* 2_byte */
        unsigned char byte;
        core30fx_bio_cfg1bit bits;
    } bio_cfg1;
    union { /* 3_byte */
        unsigned char byte;
        core30fx_temp_cfgbit bits;
    } temp_cfg;
} VC30Fx_PACKED core30fx_extra_config_t; //core30fx_extra_config_t

/****************************************************************
 * work_cmd & fifo_data structure
 ***************************************************************/
#define CORE30Fx_WORK_CMD_ADDR_BASE (0X7B)
#define CORE30Fx_WORK_CMD_SIZE (1)
/* fifo addr */
#define CORE30Fx_FIFO_READADDR_INDEX (0xFE)
#define CORE30Fx_FIFO_DATA_ADDR (0xff)

/****************************************************************
* inline func: vc30fx_core_memset,vc30fx_core_memcpy
 ***************************************************************/
#ifndef NULL
#define NULL (void*)0
#endif
static inline void *vc30fx_core_memset(void *dest, int set, unsigned int n)
{
    if (dest == NULL || n <= 0) {
        return NULL;
    }
    char *pdest = (char *)dest;
    while (n-- > 0) {
        *pdest++ = set;
    }
    return dest;
}
static inline void *vc30fx_core_memcpy(void *dest, void *src, unsigned int n)
{
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    char *pdest = (char *) dest;
    char *psrc = (char *)src;
    while (n--) {
        *pdest++ = *psrc++;
    }
    return dest;
}

#endif /* __VC30Fx_CORE_H__ */
