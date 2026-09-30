
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2022-10-09 15:19:57
 * @LastEditTime: 2024-03-28 10:32:12
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
//!!!!!!注意：本头文件不允许用户自己包含及调用相关的API,仅允许vc30fx_driver.c包含,避免出错或者是产生的ROM占用
//!!!!!!注意：本头文件不允许用户自己包含及调用相关的API,仅允许vc30fx_driver.c包含,避免出错或者是产生的ROM占用
#ifndef __VC30Fx_CORE_COM_H__
#define __VC30Fx_CORE_COM_H__

#include "vc30fx_sample_result.h"
#include "vc30fx_driver_config.h"

/* 段放置由 sdk_config.h 的 TCFG_SENSOR_HR_CODE_IN_RAM + 各 .c 顶部 pragma code_seg 控制 */
#define CORE30Fx_DATA_RAM_SECTION
#define CORE30Fx_FUNC_RAM_SECTION
/*============================================================================
 * CORE30Fx_DEBUG ：debug interface
 ===========================================================================*/
#define CORE30Fx_DEBUG_LOG(fmt, ...)
#define CORE30Fx_DEBUG_ERR(fmt, ...)

/*============================================================================
 * IC resources marco
 ===========================================================================*/
/* VC30Fx IC type ID(30FS,30FC)* */
#define VC30FS_DEV_REV_ID 0x25
#define VC30FS_FDY_VER_ID0 0x10 /* TSMC version */
#define VC30FS_FDY_VER_ID1 0x30 /* NexChip version(jinghe) */

#define VC30FC_DEV_REV_ID 0x27
#define VC30FC_FDY_VER_ID0 0x10	/* TSMC version */
#define VC30FC_FDY_VER_ID1 0x30 /* NexChip version(jinghe) */

#define VC30FS_FDY_VER_CHECK(fdy_ver_id) ( VC30FS_FDY_VER_ID0==(fdy_ver_id)||VC30FS_FDY_VER_ID1==(fdy_ver_id) )
#define VC30FC_FDY_VER_CHECK(fdy_ver_id) ( VC30FC_FDY_VER_ID0==(fdy_ver_id)||VC30FC_FDY_VER_ID1==(fdy_ver_id) )

#define VC30Fx_DEFAULT_FREQ (20000) /* 20K */
/* work event */
#define VC30Fx_EVENT_BIO_FLOW (1 << 6)
#define VC30Fx_EVENT_INSAMPLE (1 << 5)
#define VC30Fx_EVENT_CONFLICT (1 << 4)
#define VC30Fx_EVENT_INTPS_RDY (1 << 3)
#define VC30Fx_EVENT_INTFIFO_RDY (1 << 2)
#define VC30Fx_EVENT_INTOVLOAD (1 << 1)
#define VC30Fx_EVENT_INTPPG_STA (1 << 0)
/* work status */
#define VC30Fx_STATUS_CH2_OVERLOAD (1 << 2)
#define VC30Fx_STATUS_CH1_OVERLOAD (1 << 1)
#define VC30Fx_STATUS_CH0_OVERLOAD (1 << 0)
/* enable event */
#define VC30Fx_EN_EVT_FREQ2 (1 << 3)
#define VC30Fx_EN_EVT_FIFO (1 << 2)
#define VC30Fx_EN_EVT_OVLD (1 << 1)
#define VC30Fx_EN_EVT_PPG (1 << 0)

typedef enum {
    VC30Fx_CFG_DISABLE = 0,
    VC30Fx_CFG_ENABLE = 1,
} VC30Fx_CFG_FUNC;

typedef enum {
    VC30Fx_PPG_SLOT_0 = 0,
    VC30Fx_PPG_SLOT_1 = 1,
    VC30Fx_PPG_SLOT_2 = 2, /* fix=ps */
} VC30Fx_PPG_SLOT_I;       /* 逻辑工作通道 */

typedef enum {
    VC30Fx_PPG_TXCH_0 = 0, /* default:green */
    VC30Fx_PPG_TXCH_1 = 1, /* default: red */
    VC30Fx_PPG_TXCH_2 = 2, /* fix=ps,or irled */
} VC30Fx_PPG_TXCH_I;       /* 物理发光通道 */

typedef enum {
    VC30Fx_FREQ_PPG = 0, /* high */
    VC30Fx_FREQ_PS = 1,  /* low */
} VC30Fx_FREQ_SEL;

typedef enum {
    VC30Fx_NO_STORE = 0,
    VC30Fx_IN_STORE = 1,
} VC30Fx_STORE_FIFO;

typedef enum {
    VC30Fx_IO_PUSH_PULL = 0,
    VC30Fx_IO_OPEN_DRAIN = 1,
} VC30Fx_IO_MODE;
typedef enum {
    VC30Fx_IO_DIR_UP_EDGE = 0,
    VC30Fx_IO_DIR_DOWN_EDGE = 1,
} VC30Fx_IO_DIR;

typedef enum {
    VC30Fx_IO_PULSE_TIM_100US = 0,
    VC30Fx_IO_PULSE_TIM_200US = 1,
    VC30Fx_IO_PULSE_TIM_400US = 2,
    VC30Fx_IO_PULSE_TIM_800US = 3,
} VC30Fx_IO_PULSE_TIM;

typedef enum {
    VC30Fx_SAMPLE_TIM_50US = 0,
    VC30Fx_SAMPLE_TIM_100US = 1,
    VC30Fx_SAMPLE_TIM_150US = 2,
    VC30Fx_SAMPLE_TIM_200US = 3,
    VC30Fx_SAMPLE_TIM_250US = 4,
} VC30Fx_SAMPLE_TIM;

typedef enum {
    VC30Fx_STOP_WORK_CMD = 0,
    VC30Fx_START_WORK_CMD,
    VC30Fx_RESET_SOFT_CMD,
    VC30Fx_WORK_CMD_NUM,
} VC30Fx_WORK_CMD;
/*============================================================================
 * VC30Fx_CORE_API
 ===========================================================================*/
/****************************************************************************
 * @description: 获取内核驱动版本信息
 * @param {char} **ppver
 * @return {int} 0(success),-1(read/write/ticks func_pointer error)
 ****************************************************************************/
static inline int core30fx_read_software_version(char **ppver)
{
    extern int __core30fx_read_software_version(char **ppver);
    return __core30fx_read_software_version(ppver);
}

/****************************************************************************
 * @description: 读取ic的ID参数
 * @param {unsigned char} *pid1 版本ID
 * @param {unsigned char} *pid2 扩展ID
 * @return {int} 0(success),-1(read/write error),1(parameter error),2(ic_id error)
 ****************************************************************************/
static inline int core30fx_read_id(unsigned char *pid1, unsigned char *pid2)
{
    extern int __core30fx_read_id(unsigned char *pid1, unsigned char *pid2);
    return __core30fx_read_id(pid1, pid2);
}

/****************************************************
 * @description: 读取中断事件和过载状态
 * @param {int} *pevt 事件状态
 * @param {unsigned char} *povsta 过载状态
 * @return {int} 0(success),-1(read/write error)
 ****************************************************/
static inline int core30fx_read_event_status(vc30fx_sample_info_t *psample_ret, unsigned char *pevt, unsigned char *povsta, unsigned char *pwindex_bak, unsigned char *pwindex_rel)
{
    extern int __core30fx_read_event_status(vc30fx_sample_info_t *psample_ret, unsigned char *pevt, unsigned char *povsta, unsigned char *pwindex_bak, unsigned char *pwindex_rel);
    return __core30fx_read_event_status(psample_ret, pevt, povsta, pwindex_bak, pwindex_rel);
}

/***************************************************************************
 * @description: 读取时钟tick
 * @param {unsigned int} *ptick0
 * @param {unsigned int} *ptick1
 * @return {int} 0(success),-1(read/write error)
 ****************************************************************************/
static inline int core30fx_read_rcosc_tick(vc30fx_sample_info_t *psample_ret, unsigned short int *ptick0, unsigned short int *ptick1)
{
    extern int __core30fx_read_rcosc_tick(vc30fx_sample_info_t *psample_ret, unsigned short int *ptick0, unsigned short int *ptick1);
    return __core30fx_read_rcosc_tick(psample_ret, ptick0, ptick1);
}

/****************************************************************************
 * @description: 读取基础采样数据
 * @param {result_t} *psample_ret
 * @return {int} 0(success),-1(read/write error)
 ****************************************************************************/
static inline int core30fx_read_output(vc30fx_sample_info_t *psample_ret)
{
    extern int __core30fx_read_output(vc30fx_sample_info_t *psample_ret);
    return __core30fx_read_output(psample_ret);
}

/****************************************************************************
 * @description: 读取fifo_PPG数据
 * @param {result_t} *psample_ret
 * @param {unsigned char} *pread_fifoaddr
 * @return {int} 0(success),-1(read/write error),1(analysis err)
 ****************************************************************************/
static inline int core30fx_read_fifo_data(vc30fx_sample_info_t *psample_ret, unsigned char *pread_fifoaddr, unsigned char read_windex)
{
    extern int __core30fx_read_fifo_data(vc30fx_sample_info_t *psample_ret, unsigned char *pread_fifoaddr, unsigned char read_windex);
    return __core30fx_read_fifo_data(psample_ret, pread_fifoaddr, read_windex);
}

/* 通道相关配置 */
extern int __core30fx_set_slot_enable_config(VC30Fx_PPG_SLOT_I slot_index, VC30Fx_CFG_FUNC config_enable);
extern int __core30fx_set_slot_fix_param_config(VC30Fx_PPG_SLOT_I slot_index, unsigned char rxgain, VC30Fx_PPG_TXCH_I txslot, VC30Fx_CFG_FUNC deps_en, unsigned char oversample_psdirturb, VC30Fx_SAMPLE_TIM sample_tim, unsigned char ic_type, unsigned char ic_ficv);
extern int __core30fx_set_psslot_storefifo_config(VC30Fx_STORE_FIFO infifo);

extern int __core30fx_read_slot_adjust_param(unsigned char *pmax_res, unsigned char *pmin_res, unsigned char *pmax_curr, unsigned char *pmin_curr);
extern int __core30fx_read_slot_current_res(VC30Fx_PPG_SLOT_I slot_index, unsigned char *pslot_curr, unsigned char *pslot_reslvl);
extern int __core30fx_set_ppgslot_current(VC30Fx_PPG_SLOT_I slot_index, unsigned char current);
extern int __core30fx_set_ppgslot_resistance(VC30Fx_PPG_SLOT_I slot_index, unsigned char res_level, unsigned char allow_oversample, unsigned char type);

extern int __core30fx_set_common_config(vc30fx_sample_info_t *psample_ret, VC30Fx_CFG_FUNC deenv_en, VC30Fx_CFG_FUNC fifo_en, unsigned char adcspeed, unsigned char cpd);
extern int __core30fx_set_sample_frequency(vc30fx_sample_info_t *psample_ret);
extern int __core30fx_set_div_frequency(unsigned char div_index, unsigned short int div);
extern int __core30fx_set_threshold_config(unsigned char ppgth_h, unsigned char ppgth_l);
extern int __core30fx_set_interrupt_config(VC30Fx_IO_MODE int_mode, VC30Fx_IO_DIR int_dir, VC30Fx_IO_PULSE_TIM int_len, unsigned char evt_en);

extern int __core30fx_set_bioslot_config(VC30Fx_CFG_FUNC bioinn_en, VC30Fx_CFG_FUNC bioext_en, VC30Fx_STORE_FIFO biofifo_store, VC30Fx_CFG_FUNC biomode, unsigned char biofreq, unsigned char biocap, unsigned char biotime);
extern int __core30fx_set_bioinn_addcap(unsigned char biocap);
extern int __core30fx_set_temperature_config(VC30Fx_CFG_FUNC tempext_en, VC30Fx_CFG_FUNC tempinn_en, unsigned char current);

extern int __core30fx_write_work_cmd(VC30Fx_WORK_CMD cmd);

static inline int core30fx_set_enable_config(VC30Fx_PPG_SLOT_I slot_index, VC30Fx_CFG_FUNC config_enable)
{
    return __core30fx_set_slot_enable_config(slot_index, config_enable);
}

static inline int core30fx_set_slot_fix_param_config(VC30Fx_PPG_SLOT_I slot_index, unsigned char rxgain, VC30Fx_PPG_TXCH_I txslot, VC30Fx_CFG_FUNC deps_en, unsigned char oversample_psdirturb, VC30Fx_SAMPLE_TIM sample_tim, unsigned char ic_type, unsigned char ic_ficv)
{
    return  __core30fx_set_slot_fix_param_config(slot_index, rxgain, txslot, deps_en, oversample_psdirturb, sample_tim, ic_type, ic_ficv);
}
static inline int core30fx_set_psslot_storefifo_config(VC30Fx_STORE_FIFO infifo)
{
    return __core30fx_set_psslot_storefifo_config(infifo);
}

static inline int core30fx_read_slot_adjust_param(unsigned char *pmax_res, unsigned char *pmin_res, unsigned char *pmax_curr, unsigned char *pmin_curr)
{
    return __core30fx_read_slot_adjust_param(pmax_res, pmin_res, pmax_curr, pmin_curr);
}
static inline int core30fx_read_slot_current_res(VC30Fx_PPG_SLOT_I slot_index, unsigned char *pslot_curr, unsigned char *pslot_reslvl)
{
    return __core30fx_read_slot_current_res(slot_index, pslot_curr, pslot_reslvl);
}

static inline int core30fx_set_ppgslot_current(VC30Fx_PPG_SLOT_I slot_index, unsigned char current)
{
    return __core30fx_set_ppgslot_current(slot_index, current);
}

static inline int core30fx_set_ppgslot_resistance(VC30Fx_PPG_SLOT_I slot_index, unsigned char res_level, unsigned char allow_oversample, unsigned char ic_type)
{
    return __core30fx_set_ppgslot_resistance(slot_index, res_level, allow_oversample, ic_type);
}
/*--------------------------------------------------------------*/
static inline int core30fx_set_common_config(vc30fx_sample_info_t *psample_ret)
{
    VC30Fx_CFG_FUNC enable_cfg = (psample_ret->fifo_enable) ? VC30Fx_CFG_ENABLE : VC30Fx_CFG_DISABLE;
    return __core30fx_set_common_config(psample_ret, VC30Fx_CFG_ENABLE, enable_cfg, 0, 0);
}
/* 频率设置相关API */
/****************************************************************************
 * @description: (ppg=20K/div1,ps=ppg/div2,fifo=ps/div3)
 * @param {unsigned short int} div1
 * @param {unsigned char} div2
 * @param {unsigned char} div3
 * @return {int} 0(success),-1(read/write error)
 ****************************************************************************/
static inline int core30fx_set_sample_frequency(vc30fx_sample_info_t *psample_ret)
{
    return __core30fx_set_sample_frequency(psample_ret);
}
/****************************************************************************
 * @description: (ppg=20K/div1,ps=ppg/div2,fifo=ps/div3)
 * @param {unsigned char} div_index = 1,2,3
 * @param {unsigned short int} div
 * @return {int} 0(success),-1(read/write error),1(parameter error)
 ****************************************************************************/
static inline int core30fx_set_div_frequency(unsigned char div_index, unsigned short int div)
{
    return __core30fx_set_div_frequency(div_index, div);
}

static inline int core30fx_set_threshold_config(unsigned char ppgth_h, unsigned char ppgth_l)
{
    return __core30fx_set_threshold_config(ppgth_h, ppgth_l);
}
static inline int core30fx_set_interrupt_config(unsigned char evt_en)
{
#if (VC30Fx_CFG_IRQPIN_MODE==VC30Fx_CFG_IRQPIN_PUSHPULL_3V3DOWN)
    return __core30fx_set_interrupt_config(VC30Fx_IO_PUSH_PULL, VC30Fx_IO_DIR_DOWN_EDGE, VC30Fx_CFG_IRQPIN_PULSE_TIM, evt_en);
#elif (VC30Fx_CFG_IRQPIN_MODE==VC30Fx_CFG_IRQPIN_OPENDRAIN_1V8DOWN)
    return __core30fx_set_interrupt_config(VC30Fx_IO_OPEN_DRAIN, VC30Fx_IO_DIR_DOWN_EDGE, VC30Fx_CFG_IRQPIN_PULSE_TIM, evt_en);
#else
    return __core30fx_set_interrupt_config(VC30Fx_IO_PUSH_PULL, VC30Fx_IO_DIR_UP_EDGE, VC30Fx_CFG_IRQPIN_PULSE_TIM, evt_en);
#endif
}
/* 2023.30FS,bio_time[7-3]~f1,[2]~f2,[1]~f3,[0]~f4. */
//当biotime跨维度去配置时，会产生bio的一点偏差，因为f1~f4之间不完全成线性关系
static inline int core30fx_set_bioslot_config(vc30fx_sample_info_t *psample_info, unsigned char addcap)
{
    unsigned char bio_time = 6;
    if (VC30Fx_IC_TYPE_VC30FS != psample_info->ic_type) {
        psample_info->extra_result.bio_fifo_num = 0;
        psample_info->extra_result.bioinn_enwork = 0;
        psample_info->extra_result.bioext_enwork = 0;
        psample_info->extra_result.biodata_storefifo = 0;
        return 0;
    }
    if (VC30Fx_CFG_ENABLE == psample_info->extra_result.biomode_work) {
#if (VC30Fx_PS_WORK_FREQUENCY_CONFIG==1)
        bio_time = 3; /* (ps=(25/13)hz~520ms,ps_div=13,fifo_div=1) 5200us */
#else
        switch (psample_info->ps_div) {
        /*********5hz ~ 4000us **********/
        case 5: /* ppg=25，ps=5 */
            bio_time = 4;
            break;
        case 10:/* ppg=50，ps=5 */
            bio_time = 3;
            break;
        case 20:/* ppg=100，ps=5 */
            bio_time = 2;
            break;
        case 40:
            bio_time = 2;
            break;
        case 80:
            bio_time = 1;
        default:
            break;
        }
#endif
    }
    return __core30fx_set_bioslot_config((VC30Fx_CFG_FUNC)psample_info->extra_result.bioinn_enwork,
                                         (VC30Fx_CFG_FUNC)psample_info->extra_result.bioext_enwork,
                                         (VC30Fx_STORE_FIFO)psample_info->extra_result.biodata_storefifo,
                                         (VC30Fx_CFG_FUNC)psample_info->extra_result.biomode_work,
                                         2, addcap, bio_time);
}
static inline int core30fx_set_bioinn_addcap(vc30fx_sample_info_t *psample_info, unsigned char biocap)
{
    if (VC30Fx_IC_TYPE_VC30FS != psample_info->ic_type) {
        return 0;
    }
    return __core30fx_set_bioinn_addcap(biocap);
}

static inline int core30fx_set_temperature_config(vc30fx_sample_info_t *psample_info)
{
    if (VC30Fx_IC_TYPE_VC30FS != psample_info->ic_type) {
        psample_info->extra_result.tempinn_enwork = 0;
        psample_info->extra_result.tempext_enwork = 0;
        return 0;
    }
    return __core30fx_set_temperature_config((VC30Fx_CFG_FUNC)psample_info->extra_result.tempext_enwork,
            (VC30Fx_CFG_FUNC)psample_info->extra_result.tempinn_enwork,
            psample_info->extra_result.tempext_setcur);
}

/****************************************************************************
 * @description: 设置RCOSC校准值
 * @param {unsigned char} oscfreq
 * @return {int} 0(success),-1(read/write error)
 ****************************************************************************/
static inline int core30fx_set_rcosc_frequency(unsigned char oscfreq, unsigned char ic_type)
{
    extern int __core30fx_set_rcosc_frequency(unsigned char oscset, unsigned char type);
    return __core30fx_set_rcosc_frequency(oscfreq, ic_type);
}

/****************************************************************************
 * @description:
 * @param {VC30Fx_WORK_CMD} cmd
 * @return {int} 0(success),-1(read/write error),1(parameter error)
 ****************************************************************************/
static inline int core30fx_write_work_cmd(VC30Fx_WORK_CMD cmd)
{
    extern int __core30fx_write_work_cmd(VC30Fx_WORK_CMD cmd);
    return __core30fx_write_work_cmd(cmd);
}

/*============================================================================
 * IC config functions & macros
 ===========================================================================*/
// /* 调试使用：不用for,直接代码中遍历所有的slot配置，方便后期再修改，因为现在还有很多硬件相关的参数不能确定 */
static inline int core30fx_sample_ppgslot0_config(vc30fx_sample_info_t *psample_info, VC30Fx_PPG_TXCH_I ppg_tx, unsigned char rxgain, unsigned char oversample, VC30Fx_SAMPLE_TIM sampletime)
{
    /* ppg slot0 config */
    if (1 == psample_info->slot_result[0].slot_enwork) {
        core30fx_set_ppgslot_current(VC30Fx_PPG_SLOT_0, psample_info->slot_result[0].slot_current);
        core30fx_set_ppgslot_resistance(VC30Fx_PPG_SLOT_0, psample_info->slot_result[0].slot_resistance, 0, psample_info->ic_type);
        core30fx_set_slot_fix_param_config(VC30Fx_PPG_SLOT_0, rxgain, ppg_tx, VC30Fx_CFG_DISABLE, oversample, sampletime, psample_info->ic_type, psample_info->ic_FICV);
        core30fx_set_enable_config(VC30Fx_PPG_SLOT_0, VC30Fx_CFG_ENABLE);
    }
    return 0;
}
static inline int core30fx_sample_ppgslot1_config(vc30fx_sample_info_t *psample_info, VC30Fx_PPG_TXCH_I ppg_tx, unsigned char rxgain, unsigned char oversample, VC30Fx_SAMPLE_TIM sampletime)
{
    /* ppg slot1 config */
    if (1 == psample_info->slot_result[1].slot_enwork) {
        core30fx_set_ppgslot_current(VC30Fx_PPG_SLOT_1, psample_info->slot_result[1].slot_current);
        core30fx_set_ppgslot_resistance(VC30Fx_PPG_SLOT_1, psample_info->slot_result[1].slot_resistance, 0, psample_info->ic_type);
        core30fx_set_slot_fix_param_config(VC30Fx_PPG_SLOT_1, rxgain, ppg_tx, VC30Fx_CFG_DISABLE, oversample, sampletime, psample_info->ic_type, psample_info->ic_FICV);
        core30fx_set_enable_config(VC30Fx_PPG_SLOT_1, VC30Fx_CFG_ENABLE);
    }
    return 0;
}
static inline int core30fx_sample_psslot2_config(vc30fx_sample_info_t *psample_info, unsigned char rxgain, VC30Fx_SAMPLE_TIM sampletime)
{
    /* ps slot2 config */
    unsigned char deps_dirturb = 0;
    if (psample_info->ic_type == VC30Fx_IC_TYPE_VC30FS) {
        deps_dirturb = 3;
    } else if (psample_info->ic_type == VC30Fx_IC_TYPE_VC30FC) {
        deps_dirturb = 3;
    }
    if (2 == psample_info->slot_result[2].slot_enwork) {
        core30fx_set_ppgslot_current(VC30Fx_PPG_SLOT_2, psample_info->slot_result[2].slot_current);
        core30fx_set_ppgslot_resistance(VC30Fx_PPG_SLOT_2, psample_info->slot_result[2].slot_resistance, 0, psample_info->ic_type);
        core30fx_set_slot_fix_param_config(VC30Fx_PPG_SLOT_2, rxgain, VC30Fx_PPG_TXCH_2 /*storefifo,reserved*/, VC30Fx_CFG_ENABLE, deps_dirturb /*EnDePSDirturb*/, sampletime, psample_info->ic_type, psample_info->ic_FICV);
        core30fx_set_psslot_storefifo_config((VC30Fx_STORE_FIFO)psample_info->slot_result[2].slot_storefifo);
        core30fx_set_enable_config(VC30Fx_PPG_SLOT_2, VC30Fx_CFG_ENABLE);
    }
    return 0;
}

/*=====================================================================================================================================*/

/****************************************************************
 * CORE_API.write()/set()
 * common interface to write/read configuration
 * write or set config registers, debug or other function.
 ***************************************************************/

extern int vc30fx_param_write_flash(unsigned char *pdata, unsigned int size);
extern int vc30fx_param_read_flash(unsigned char *pdata, unsigned int size);

extern void *vc30fx_malloc(unsigned int size);
extern void vc30fx_free(void *ptr);

extern int vc30fx_read_register(unsigned char regaddr, unsigned char *pbuf, unsigned short int size);
extern int vc30fx_write_register(unsigned char regaddr, unsigned char *pbuf, unsigned short int size);
extern unsigned int vc30fx_get_cputimer_tick(void);

#endif /* __VC30Fx_CORE_COM_H__ */
