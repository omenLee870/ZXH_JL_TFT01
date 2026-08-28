/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-06-09 11:57:36
 * @LastEditTime: 2024-01-05 09:28:32
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#ifndef __VC30Fx_SAMPLE_RESULT_H__
#define __VC30Fx_SAMPLE_RESULT_H__

/****************************************************************************
drviver vc30f-s/c RAM:
core+driver: 400bytes + fifo_data_memory <= 1kb
fifo_data_memory static:  128*2 + 64*(2*2+2*1) = 448 bytes
fifo_data_memory dynamic: 128*x + 64*(2*y+2*z) = 0~448 bytes
fifo_data_memory fixdync:
*/
#define VC30Fx_USE_STATIC_MEMORY (0)
#define VC30Fx_USE_FIXDYNC_MEMORY (1) /* memory is allocated when the device is registered and freed when the device is deregistered */
#define VC30Fx_USE_MEMORY_SELECT (VC30Fx_USE_STATIC_MEMORY)
#define VC30Fx_PPGSLOT_BUFFER_NUM (64) /* memory_buffer(40~64) = enable_ppgslot_num * ( VC30Fx_PPGSLOT_BUFFER_NUM * sizeof(unsigned short int) ) */
#define VC30Fx_PS2SLOT_BUFFER_NUM (32) /* memory_buffer(20~32) = enable_bioslot_num * ( VC30Fx_PS2SLOT_BUFFER_NUM * sizeof(unsigned short int) ) + 1*( VC30Fx_PS2SLOT_BUFFER_NUM * sizeof(unsigned short int) ) */

/* ps sample frequency config, if work_fast,the frequency=12.5hz, otherwise is 5hz
[0] ps=  5hz~200ms, fifo_time=1000ms, 800ms, 600ms, 400ms, 200ms
[1] ps=(1000/520)hz~520ms, fifo_time=520ms(25hz), 260ms(50hz), 130ms(100hz), user defined time, ps-fifo working on the same frequency (~9201,VC30F).
    !!!notice: need config bio_param_time @ vc30fx_core_com.h @ core30fx_set_bioslot_config
[2]........
*/
#define VC30Fx_PS_WORK_FREQUENCY_CONFIG (0)

/*the ic type: VC30FS / VC30FC  */
#define VC30Fx_IC_TYPE_VC30FC (0)
#define VC30Fx_IC_TYPE_VC30FS (1)

/* config VC30FC ppg_hz and ps_hz have the same frequency, and bio/ps data don't storefifo, working on HR/SPO2 */
#define VC30Fx_VC30FC_PPGPS_FREQUENCY ( 0 )

/* ppg/ps slot parameter description:
slot_config: structure handle.
enwork: slot configuration enable work, 0-disbale, 1-work_ppg, 2-work_ps.
freqsel: frequency selection, 0-ppg_frequency, 1-ps_frequency. vc30fx_core_com.h:enum@VC30Fx_FREQ_SEL.
storefifo: fifo configuration, 0-nofifo,1-infifo, available when slot_freq_sel is ps_frequency. @VC30Fx_STORE_FIFO.
current: ppg_slot work current set(0x00~0x7f).
resistance: ppg_slot work resistance set(0~13).
*/
#define VC30Fx_SLOT_PARAM_CONFIG(slot_config, enwork, freqsel, storefifo, current, resistance) \
    {                                                                                          \
        slot_config.slot_enwork = enwork;                                                      \
        slot_config.slot_freq_sel = freqsel;                                                   \
        slot_config.slot_storefifo = storefifo;                                                \
        slot_config.slot_current = current;                                                    \
        slot_config.slot_resistance = resistance;                                              \
    }
#define VC30Fx_PPG_SLOT_PARAM_CONFIG(slot_config, current, resistance) VC30Fx_SLOT_PARAM_CONFIG(slot_config, 1, VC30Fx_FREQ_PPG, VC30Fx_IN_STORE, current, resistance)
#define VC30Fx_PS_SLOT_PARAM_CONFIG(slot_config, storefifo, current, resistance) VC30Fx_SLOT_PARAM_CONFIG(slot_config, 2, VC30Fx_FREQ_PS, storefifo, current, resistance)
/* bio slot parameter description:
bio_config: structure handle.
inn_enable: inside slot configuration enable work, @VC30Fx_CFG_FUNC.
ext_enable: external slot configuration enable work, @VC30Fx_CFG_FUNC.
storefifo: fifo configuration, 0-nofifo,1-infifo, @VC30Fx_STORE_FIFO.
ppgmode: enable-ppg_freq workmode,disable-ps_freq workmode .@VC30Fx_CFG_FUNC.
*/
#define VC30Fx_BIO_SLOT_PARAM_CONFIG(bio_config, inn_enable, ext_enable, storefifo, ppgmode) \
    {                                                                                        \
        bio_config.bioinn_enwork = inn_enable;                                               \
        bio_config.bioext_enwork = ext_enable;                                               \
        bio_config.biodata_storefifo = storefifo;                                            \
        bio_config.biomode_work = ppgmode;                                                   \
    }
/* tmp slot parameter description:
tmp_config: structure handle.
inn_enable: inside slot configuration enable work, @VC30Fx_CFG_FUNC.
ext_enable: external slot configuration enable work, @VC30Fx_CFG_FUNC.
setcur: external slot config work current(0~7).
*/
#define VC30Fx_TMP_SLOT_PARAM_CONFIG(tmp_config, inn_enable, ext_enable, setcur) \
    {                                                                            \
        tmp_config.tempinn_enwork = inn_enable;                                  \
        tmp_config.tempext_enwork = ext_enable;                                  \
        tmp_config.tempext_setcur = setcur;                                      \
    }
/* ppg_mark[0-127] */
#define VC30Fx_PPG_MARK_SET(ppg_mark, ppg_index)           \
    if (ppg_index < 64)                                    \
    {                                                      \
        ppg_mark[ppg_index / 32] |= 1 << (ppg_index % 32); \
    }
#define VC30Fx_PPG_MARK_CLR(ppg_mark, ppg_index)              \
    if (ppg_index < 64)                                       \
    {                                                         \
        ppg_mark[ppg_index / 32] &= ~(1 << (ppg_index % 32)); \
    }
#define VC30Fx_PPG_MARK_GET(ppg_mark, ppg_index) (((ppg_mark)[ppg_index / 32] >> (ppg_index % 32)) & 0x01)
/****************************************************************
 * structures:vc30fx_sample_info_t
 ***************************************************************/
typedef struct {
    /* slot_config */
    unsigned char slot_enwork;    /* slot_enwork, 0-disbale, 1-work_ppg, 2-work_ps. */
    unsigned char slot_freq_sel;  /* 0-ppg_frequency, 1-ps_frequency. vc30fx_core_com.h, @VC30Fx_FREQ_SEL. */
    unsigned char slot_storefifo; /* 0-nofifo,1-infifo, available when slot_freq_sel is ps_frequency. */
    unsigned char slot_led_sel;   /* 0-green led, 1-red led, 2-infrared led */
    /* slot_variable_config */
    unsigned char slot_current;    /* 0~0x7F */
    unsigned char slot_resistance; /* 0~13/0~23 */
    /* slot_result */
    unsigned short int env_data;
    union {
        unsigned short int ps_data;
        unsigned short int pre_data;
    } u;
    unsigned char ppg_num;
    unsigned short int ppg_buffer[VC30Fx_PPGSLOT_BUFFER_NUM];
} vc30fx_sample_slot_t;

typedef struct {
    /* bio_slot config */
    unsigned char bioinn_enwork;
    unsigned char bioext_enwork;
    unsigned char biomode_work;      /* 0-bio operates at PS_frequnecy, 1-bio operates at PPG_frequnecy and accumulation */
    unsigned char biodata_storefifo; /* 0-nofifo,1-infifo, default fifo format:16bit */
    /* temp_slot config */
    unsigned char tempinn_enwork;
    unsigned char tempext_enwork;
    unsigned char tempext_setcur;
    /* extern result */
    unsigned char bio_fifo_num;
    unsigned short int bioinn_data;
    unsigned short int bioext_data;
    unsigned short int bioabs_calc; /* abs(base-ext_now)?thh */
    unsigned short int tempinn_data;
    unsigned short int tempext_data;
    unsigned short int tempref_data;
    unsigned short int bioinn_buffer[VC30Fx_PS2SLOT_BUFFER_NUM];
    unsigned short int bioext_buffer[VC30Fx_PS2SLOT_BUFFER_NUM];
} vc30fx_sample_extra_t;

typedef struct {
    unsigned char ic_type;      /* 1:vc30fs, 0:vc30fc */
    unsigned char ic_FICV;      /* ic_fdyID+Ver, 0x10_old, 0x30_new */
    unsigned short int ppg_div; /* ppg_frequency  = 20K           / ppg_div */
    unsigned char ps_div;       /* ps_frequency   = ppg_frequency / ps_div */
    unsigned char fifo_div;     /* fifo_frequency = ps_frequency  / fifo_div */
    unsigned char fifo_enable;  /* fifo_enable = 1*/
    vc30fx_sample_extra_t extra_result;
    vc30fx_sample_slot_t slot_result[3];

} vc30fx_sample_info_t;

#endif /* __VC30Fx_SAMPLE_RESULT_H__ */

