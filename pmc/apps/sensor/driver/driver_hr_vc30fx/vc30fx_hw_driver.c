
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2022-09-30 09:50:56
 * @LastEditTime: 2024-07-08 15:39:03
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#include "includes.h"
#include "uart.h"
#include "debug.h"
#include "vc30fx_core_com.h"
#include "vc30fx_driver_config.h"
#include "vc30fx_hw_driver.h"

#if TCFG_VC30fx_ENABLE
#if (VC30Fx_CFG_WEAR_BIOALGO_EN==VC30Fx_FUNC_ENABLE)
#include "BIODetect_V14_1.h"
#endif

#ifndef VC_ABS_MAX
#define VC_ABS_MAX(a, b, max) (((a) > (b)) ? ((a) - (b)) : (max + (a) - (b)))
#endif
#ifndef VC_ABS
#define VC_ABS(a, b) (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#endif
#ifndef VC30FS_ENV_CONVERT
#define VC30FS_ENV_CONVERT( hex_env ) (((hex_env)>>8)*50+((hex_env)&0xff))
#endif
#ifndef VC30FC_ENV_CONVERT
#define VC30FC_ENV_CONVERT( hex_env ) (((hex_env)>>4)*12.5f+((hex_env)&0xf))
#endif

unsigned short int VC30Fx_CFG_MCU_TICK_FREQUENCY = 25000; /* extern unsigned short int VC30Fx_CFG_MCU_TICK_FREQUENCY; */
unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_HOLD = 200; /* extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_HOLD;  */
unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_DROP = 160; /* extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_DROP;  */

static unsigned short startup_inntemperature = 0;
static unsigned int check_cpu_tick[2] = {0}, check_dev_tick[2] = {0};
/* [dichotomy] startup will calc rcosc */
static unsigned char rcosc_max = 127;
static unsigned char rcosc_min = 0;
static unsigned char rcosc_mid = 63;
static unsigned char rcosc_max_thh = 127;
static unsigned char rcosc_min_thh = 0;

/* [dichotomy] reset bio param */
#define VC30Fx_CALC_BIOINN_OFFSET 100
static unsigned short int last_ext_bio = 0; /* 判断外部bio的稳定性 */
/* 二分夹为二选一时，分别测试记录得到最合适得值 */
static unsigned char remember_biomax_mark = 0;
static unsigned char remember_biomin_mark = 0;
static unsigned short int remember_biomax_value = 0;
static unsigned short int remember_biomin_value = 0;
/* 二分中值及采样标志 */
static unsigned short int last_inn_bio_mid = 0;
static unsigned char set_flg_mid = 0;
/* 二分初始数据 */
static unsigned char set_cap_max = 31;
static unsigned char set_cap_mid = 16;
static unsigned char set_cap_min = 1;

#if (VC30Fx_USE_MEMORY_SELECT == VC30Fx_USE_STATIC_MEMORY)
static vc30fx_sample_info_t sample_info = {0};	/* sample config & sample result */
static vc30fx_adjust_info adjust_info[2] = {0}; /* slot_adjust_infomation */
static vc30fx_wear_info wear_info = {0};		/* wear_status_infomation */
static vc30fx_clk_info clk_info = {0};			/* oscclk_calibration_infomation */
static vc30fx_drv_info drv_info = {0};			/* driver_param_infomation */

static vc30fx_sample_info_t *sample_info_handle = &sample_info;
static vc30fx_adjust_info *adjust_info_handle[2] = {&adjust_info[0], &adjust_info[1]};
static vc30fx_wear_info *wear_info_handle = &wear_info;
static vc30fx_clk_info *clk_info_handle = &clk_info;
static vc30fx_drv_info *drv_info_handle = &drv_info;
#elif VC30Fx_USE_MEMORY_SELECT == VC30Fx_USE_FIXDYNC_MEMORY
static vc30fx_sample_info_t *sample_info_handle = NULL;
static vc30fx_adjust_info *adjust_info_handle[2] = {NULL, NULL};
static vc30fx_wear_info *wear_info_handle = NULL;
static vc30fx_clk_info *clk_info_handle = NULL;
static vc30fx_drv_info *drv_info_handle = NULL;
#endif

static void drv_calibration_clk_clear(void);
static int drv_read_sample_param(vc30fx_sample_info_t *psample_ret);
static int drv_read_fifo_data(vc30fx_sample_info_t *psample_result);
static int drv_read_event_status(vc30fx_drv_info *pdrv_info);
static void drv_fifo_error_softreset(void);

/********************************************************************************************************
 * 驱动参数内存申请/释放，初始化/结束使用
 ********************************************************************************************************/
int vc30fx_drv_memroy_alloc(void)
{
#if (VC30Fx_USE_MEMORY_SELECT == VC30Fx_USE_FIXDYNC_MEMORY)
    sample_info_handle = vc30fx_malloc(sizeof(vc30fx_sample_info_t));
    if (NULL == sample_info_handle) {
        VCARE_DBG_ERR("sample_info_handle malloc failed");
        goto SAMPLE_INFO_MEM_ERROR;
    }
    vcare_memset(sample_info_handle, 0, sizeof(vc30fx_sample_info_t));

    adjust_info_handle[0] = vc30fx_malloc(sizeof(vc30fx_adjust_info));
    if (NULL == adjust_info_handle[0]) {
        VCARE_DBG_ERR("adjust_info_handle[0] malloc failed");
        goto ADJ_INFO0_MEM_ERROR;
    }
    vcare_memset(adjust_info_handle[0], 0, sizeof(vc30fx_adjust_info));

    adjust_info_handle[1] = vc30fx_malloc(sizeof(vc30fx_adjust_info));
    if (NULL == adjust_info_handle[0]) {
        VCARE_DBG_ERR("adjust_info_handle[1] malloc failed");
        goto ADJ_INFO1_MEM_ERROR;
    }
    vcare_memset(adjust_info_handle[1], 0, sizeof(vc30fx_adjust_info));

    wear_info_handle = vc30fx_malloc(sizeof(vc30fx_wear_info));
    if (NULL == wear_info_handle) {
        VCARE_DBG_ERR("wear_info_handle malloc failed");
        goto WEAR_INFO_MEM_ERROR;
    }
    vcare_memset(wear_info_handle, 0, sizeof(vc30fx_wear_info));

    clk_info_handle = vc30fx_malloc(sizeof(vc30fx_clk_info));
    if (NULL == clk_info_handle) {
        VCARE_DBG_ERR("clk_info_handle malloc failed");
        goto CLK_INFO_MEM_ERROR;
    }
    vcare_memset(clk_info_handle, 0, sizeof(vc30fx_clk_info));

    drv_info_handle = vc30fx_malloc(sizeof(vc30fx_drv_info));
    if (NULL == drv_info_handle) {
        VCARE_DBG_ERR("drv_info_handle malloc failed");
        goto DRV_INFO_MEM_ERROR;
    }
    vcare_memset(drv_info_handle, 0, sizeof(vc30fx_drv_info));
    return 0;

DRV_INFO_MEM_ERROR:
    vc30fx_free(clk_info_handle);
CLK_INFO_MEM_ERROR:
    vc30fx_free(wear_info_handle);
WEAR_INFO_MEM_ERROR:
    vc30fx_free(adjust_info_handle[1]);
ADJ_INFO1_MEM_ERROR:
    vc30fx_free(adjust_info_handle[0]);
ADJ_INFO0_MEM_ERROR:
    vc30fx_free(sample_info_handle);
SAMPLE_INFO_MEM_ERROR:
    return -1;
#else
    return 0;
#endif
}
int vc30fx_drv_memroy_free(void)
{
#if (VC30Fx_USE_MEMORY_SELECT == VC30Fx_USE_FIXDYNC_MEMORY)
    vc30fx_free(drv_info_handle);
    vc30fx_free(clk_info_handle);
    vc30fx_free(wear_info_handle);
    vc30fx_free(adjust_info_handle[1]);
    vc30fx_free(adjust_info_handle[0]);
    vc30fx_free(sample_info_handle);
#endif
    return 0;
}
static void drv_sample_result_memroy_finished(vc30fx_sample_info_t *presult_info)
{
    unsigned char i = 0;
    if (drv_info_handle->fifo_read_data_valid) {
        /* ppg ps slot */
        for (i = 0; i < 3; i++) {
            presult_info->slot_result[i].ppg_num = 0;
            vcare_memset(presult_info->slot_result[i].ppg_buffer, 0, sizeof(presult_info->slot_result[i].ppg_buffer));
        }
        /* bioinn bioext */
        presult_info->extra_result.bio_fifo_num = 0;
        vcare_memset(presult_info->extra_result.bioinn_buffer, 0, sizeof(presult_info->extra_result.bioinn_buffer));
        vcare_memset(presult_info->extra_result.bioext_buffer, 0, sizeof(presult_info->extra_result.bioext_buffer));
    }
}

/********************************************************************************************************
 * 驱动参数保存部分
 ********************************************************************************************************/
static int drv_infomation_param_save(void)
{
    int ret = 0;
    unsigned char save_buf[8] = {0};
    save_buf[0] = 0xAA;
    save_buf[1] = drv_info_handle->optimal_bio_valid = 1;
    save_buf[2] = drv_info_handle->optimal_cap;

    save_buf[3] = (drv_info_handle->optimal_bio_gap >> 24) & 0xff;
    save_buf[4] = (drv_info_handle->optimal_bio_gap >> 16) & 0xff;
    save_buf[5] = (drv_info_handle->optimal_bio_gap >> 8) & 0xff;
    save_buf[6] = (drv_info_handle->optimal_bio_gap >> 0) & 0xff;
    ret = vc30fx_param_write_flash(save_buf, sizeof(save_buf));
    VCARE_DBG_LOG("save bio param: %d,%d,%d", drv_info_handle->optimal_bio_valid, drv_info_handle->optimal_cap, drv_info_handle->optimal_bio_gap);
    return ret; /* successed return 0 */
}
static int drv_infomation_param_load(void)
{
    int ret = 0;
    unsigned char load_buf[8] = {0};
    ret = vc30fx_param_read_flash(load_buf, sizeof(load_buf));
    if (load_buf[0] == 0xAA) {
        drv_info_handle->optimal_bio_valid = load_buf[1];
        drv_info_handle->optimal_cap = load_buf[2];
        drv_info_handle->optimal_bio_gap = (int)((load_buf[3] << 24) + (load_buf[4] << 16) + (load_buf[5] << 8) + (load_buf[6]));
    } else {
        drv_info_handle->optimal_bio_valid = 0;
        drv_info_handle->optimal_cap = 0;
        drv_info_handle->optimal_bio_gap = 0;
        ret = 0;
    }
    VCARE_DBG_LOG("load bio param: %d,%d,%d", drv_info_handle->optimal_bio_valid, drv_info_handle->optimal_cap, drv_info_handle->optimal_bio_gap);
    return ret; /* successed return 0 */
}
static int drv_infomation_param_reset(void)
{
    drv_info_handle->optimal_bio_valid = 0;
    drv_info_handle->optimal_cap = 0;
    drv_info_handle->optimal_bio_gap = 0;
    /* init [dichotomy]param */
    last_ext_bio = 0;
    remember_biomax_mark = 0;
    remember_biomin_mark = 0;
    remember_biomax_value = 0;
    remember_biomin_value = 0;
    /* 二分中值及采样标志 */
    last_inn_bio_mid = 0;
    set_flg_mid = 0;
    /* 二分初始数据 */
    set_cap_max = 31;
    set_cap_mid = 16;
    set_cap_min = 1;
    return 0;
}
/*****************************************************************************
 * driver_work_mode table:
 * {WORK_MODE, MODE_INIT_FUNC, MODE_HANDLER_FUNC }
 *****************************************************************************/
static const vc30fx_mode_ops *sel_mode_ops = NULL;
static int drv_heartrate_mode_init(void);
static int drv_spo2_mode_init(void);
static int drv_bloodpressure_mode_init(void);
static int drv_wear_mode_init(void);
static int drv_temperature_mode_init(void);
static int drv_factory_green_ps_mode_init(void);
static int drv_factory_ir_red_mode_init(void);
static int drv_undefined_mode_init(void);
/*handler*/
static int drv_ppg_mode_handler(const void *ops_self, vcare_ppg_device_t *pdev);
static int drv_factory_mode_handler(const void *ops_self, vcare_ppg_device_t *pdev);
static int drv_undefine_mode_handler(const void *ops_self, vcare_ppg_device_t *pdev);

static const vc30fx_mode_ops mode_ops[] = {
    /* work_mode, work_mode_init, work_mode_handler */
    {WORK_MODE_HR, drv_heartrate_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_SPO2, drv_spo2_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_HRV, drv_heartrate_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_STRESS, drv_heartrate_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_BLOODPRESSURE, drv_bloodpressure_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_WEAR, drv_wear_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_TEMPERATURE, drv_temperature_mode_init, drv_ppg_mode_handler},
    {WORK_MODE_FACTORY0, drv_factory_green_ps_mode_init, drv_factory_mode_handler},
    {WORK_MODE_FACTORY1, drv_factory_ir_red_mode_init, drv_factory_mode_handler},
    {WORK_MODE_HRSPO2, NULL, NULL},
    {WORK_MODE_UNDEFINED, drv_undefined_mode_init, drv_undefine_mode_handler},
};

/******************************************************************************
 * 初始化部分
 ******************************************************************************/
static void drv_infomation_param_init(vcare_ppg_device_t *pdev)
{
    int i;
    unsigned char max_cur = 0, min_cur = 0, max_res_lvl = 0, min_res_lvl = 0;
    /* slot_adjust param */
    core30fx_read_slot_adjust_param(&max_res_lvl, &min_res_lvl, &max_cur, &min_cur);
    for (i = 0; i < 2; i++) {
        adjust_info_handle[i]->const_max_current = max_cur;
        adjust_info_handle[i]->const_min_current = min_cur;
        adjust_info_handle[i]->max_current = max_cur;
        adjust_info_handle[i]->const_max_res_level = max_res_lvl;
        adjust_info_handle[i]->const_min_res_level = min_res_lvl;
        adjust_info_handle[i]->max_res_level = max_res_lvl;
        adjust_info_handle[i]->adj_param.current_step = VC30Fx_ADJ_I_STEP_DFT;
        adjust_info_handle[i]->adj_param.direction = VC30Fx_ADJUST_DIR_NONE;
        adjust_info_handle[i]->adj_param.direction_last = VC30Fx_ADJUST_DIR_NONE;
    }
    /* dev_wear param */
    drv_info_handle->optimal_bio_valid = 1; /* 先置1，等待在factory模式下主动清除后进行配平 */
#if (VC30Fx_WEAR_DFT_HOLD == VC30Fx_CFG_WEAR_DEFAULT_STA)
    pdev->wear = WEAR_STA_HOLD;
    wear_info_handle->last_status = (WEAR_STA_NONE == wear_info_handle->last_status) ? WEAR_STA_HOLD : wear_info_handle->last_status;
#else
    pdev->wear = WEAR_STA_DROP;
    wear_info_handle->last_status = (WEAR_STA_NONE == wear_info_handle->last_status) ? WEAR_STA_DROP : wear_info_handle->last_status;
#endif
#if (VC30Fx_FUNC_ENABLE != VC30Fx_CFG_WEAR_DETECTION)
    pdev->wear = WEAR_STA_HOLD;
    wear_info_handle->last_status = WEAR_STA_HOLD;
    wear_info_handle->wear_status = WEAR_STA_HOLD;
#endif
    /* 切换模式如果为摘下时，需要关灯 */
    if (wear_info_handle->last_status == WEAR_STA_DROP && (drv_info_handle->work_mode != WORK_MODE_FACTORY0 && drv_info_handle->work_mode != WORK_MODE_FACTORY1)) {
        for (i = 0; i < 2; i++) {
            if (sample_info_handle->slot_result[i].slot_enwork == 1) {
                core30fx_set_enable_config((VC30Fx_PPG_SLOT_I)i, VC30Fx_CFG_DISABLE);
                sample_info_handle->slot_result[i].slot_enwork = 0;
            }
        }
    }
    wear_info_handle->wear_status = wear_info_handle->last_status;
    pdev->wear = wear_info_handle->last_status;

    wear_info_handle->drop_cnt = VC30Fx_CFG_WEAR_DROP_CNT;
    wear_info_handle->hold_cnt = VC30Fx_CFG_WEAR_HOLD_CNT;
    wear_info_handle->acce_wear_status = WEAR_STA_HOLD;
    wear_info_handle->algo_wear_status = WEAR_STA_HOLD;
    /* dev_clk param */
    drv_calibration_clk_clear();
    /* read_fifo_address */
    drv_info_handle->read_fifo_address = 0;
    if (drv_info_handle->work_mode == WORK_MODE_SPO2) {
        drv_info_handle->ppg_max_thval = VC30Fx_PPG_SPO2_MAX_DEFT_THVAL;
        drv_info_handle->ppg_min_thval = VC30Fx_PPG_SPO2_MIN_DEFT_THVAL;
    } else {
        drv_info_handle->ppg_max_thval = VC30Fx_PPG_MAX_DEFT_THVAL;
        drv_info_handle->ppg_min_thval = VC30Fx_PPG_MIN_DEFT_THVAL;
    }
    if (wear_info_handle->bio_wear_result == VC30Fx_BIO_STA_NONE) {
#if (VC30Fx_CFG_WEAR_BIOALGO_EN==VC30Fx_FUNC_ENABLE)
        f_BIOInit(VC30Fx_WEAR_PARAM_BIO_DELTA, VC30Fx_WEAR_PARAM_PS_DELTA);
        /* VCARE_DBG_LOG("f_BIOedgeinit(%d, %d, %d)", VC30Fx_WEAR_PARAM_BIO_DELTA, VC30Fx_WEAR_PARAM_PS_DELTA ); */
#endif
    }
}

static int drv_calc_bioinn_auto_addcap(unsigned char status, vc30fx_sample_info_t *psample_info)
{
    unsigned char finished_set = 0;
    unsigned char finished_mark = 0;
    if (VC30Fx_IC_TYPE_VC30FC == psample_info->ic_type) {
        return 0;
    }
    if (status & VC30Fx_EVENT_INTPS_RDY) {
        if (drv_info_handle->optimal_bio_valid) {
            return 0;
        }
        /* 判断是否bioext发生变化，如果变化太大，那么直接校准失败 */
        if (last_ext_bio != 0 && VC_ABS(last_ext_bio, psample_info->extra_result.bioext_data) >= 20) {
            VCARE_DBG_ERR("calc bioinn error, bioext running...");
        }
        last_ext_bio = psample_info->extra_result.bioext_data;
        /* 二分法校准，循环赋值，设置参数与采样值呈负相关 */
        if (set_flg_mid == 1) {
            set_flg_mid = 0;
            last_inn_bio_mid = psample_info->extra_result.bioinn_data;
        }
        if (last_inn_bio_mid == 0) {
            core30fx_set_bioinn_addcap(psample_info, set_cap_mid);
            set_flg_mid = 1;
            return 0;
        }
        /* 在合理范围内快速退出 */
        if ((psample_info->extra_result.bioext_data - VC30Fx_CALC_BIOINN_OFFSET) <= last_inn_bio_mid && (psample_info->extra_result.bioext_data + VC30Fx_CALC_BIOINN_OFFSET) >= last_inn_bio_mid) {
            finished_set = set_cap_mid;
            finished_mark = 1;
            VCARE_DBG_LOG("(+-100)now set cap[%d,%d,%d],now_bio_val[%d](%d) ->set[%d]", set_cap_min, set_cap_mid, set_cap_max,
                          last_inn_bio_mid,  psample_info->extra_result.bioext_data, last_inn_bio_mid);
        } else { /* 不在范围，范围比较大，继续二分 */
            /* 检索完成，需要再min和max中取一个值 */
            if ((set_cap_max - set_cap_min) <= 1) {
                if (remember_biomax_mark == 1) {
                    remember_biomax_mark = 0;
                    remember_biomax_value = psample_info->extra_result.bioinn_data;
                }
                if (remember_biomin_mark == 1) {
                    remember_biomin_mark = 0;
                    remember_biomin_value = psample_info->extra_result.bioinn_data;
                }
                if (remember_biomax_value == 0) {
                    remember_biomax_mark = 1;
                    core30fx_set_bioinn_addcap(psample_info, set_cap_max);
                    return 0;
                }
                if (remember_biomin_value == 0) {
                    remember_biomin_mark = 1;
                    core30fx_set_bioinn_addcap(psample_info, set_cap_min);
                    return 0;
                }
                if (VC_ABS(remember_biomin_value, psample_info->extra_result.bioext_data) <= VC_ABS(remember_biomax_value, psample_info->extra_result.bioext_data)) {
                    finished_set = set_cap_min; /* 取min,结束 */
                    last_inn_bio_mid = remember_biomin_value;
                } else {
                    finished_set = set_cap_max; /* 取max，结束 */
                    last_inn_bio_mid = remember_biomax_value;
                }
                finished_mark = 1;
                VCARE_DBG_LOG("mingap(%d)[%d],maxgap(%d)[%d]->(%d)", set_cap_min, VC_ABS(remember_biomin_value, psample_info->extra_result.bioext_data),
                              set_cap_max, VC_ABS(remember_biomax_value, psample_info->extra_result.bioext_data), finished_set);
            }
            /* 继续执行二分逼近 */
            if (last_inn_bio_mid < psample_info->extra_result.bioext_data) {
                VCARE_DBG_LOG("(mid<bioext)now set cap[%d,%d,%d],now_bio_val[%d](%d) ->set[%d]", set_cap_min, set_cap_mid, set_cap_max,
                              last_inn_bio_mid, psample_info->extra_result.bioext_data, (set_cap_min + set_cap_mid) / 2);
                set_cap_max = set_cap_mid;
                set_cap_mid = (set_cap_min + set_cap_mid) / 2;
                set_flg_mid = 1;
                core30fx_set_bioinn_addcap(psample_info, set_cap_mid);
            } else if (last_inn_bio_mid > psample_info->extra_result.bioext_data) {
                VCARE_DBG_LOG("(mid>bioext)now set cap[%d,%d,%d],now_bio_val[%d](%d) ->set[%d]", set_cap_min, set_cap_mid, set_cap_max,
                              last_inn_bio_mid, psample_info->extra_result.bioext_data, (set_cap_max + set_cap_mid) / 2);
                set_cap_min = set_cap_mid;
                set_cap_mid = (set_cap_max + set_cap_mid) / 2;
                set_flg_mid = 1;
                core30fx_set_bioinn_addcap(psample_info, set_cap_mid);
            } else if (last_inn_bio_mid == psample_info->extra_result.bioext_data) { /* 完全相等，直接结束 */
                VCARE_DBG_LOG("(mid=bioext)now set cap[%d,%d,%d],now_bio_val[%d](%d) ->set[%d]", set_cap_min, set_cap_mid, set_cap_max,
                              last_inn_bio_mid, psample_info->extra_result.bioext_data, set_cap_mid);
                finished_set = set_cap_mid;
                finished_mark = 1;
            }
        }
        if (finished_mark) { /* 查找结束处理 */
            drv_info_handle->optimal_cap = finished_set;
            drv_info_handle->optimal_bio_gap = last_inn_bio_mid - psample_info->extra_result.bioext_data;
            /* set optimal_cap */
            core30fx_set_bioinn_addcap(psample_info, drv_info_handle->optimal_cap);
            VCARE_DBG_LOG("optimal_cap=%d, optimal_bio_gap=%d", drv_info_handle->optimal_cap, drv_info_handle->optimal_bio_gap);
            /* save flash_ok */
            drv_infomation_param_save();
        }
    }
    return -1;
}

static void drv_drop_modify_div_irq_config(vc30fx_drv_info *pdrv_info)
{
#if(VC30Fx_VC30FC_PPGPS_FREQUENCY)
    if (pdrv_info->work_mode != WORK_MODE_HR && pdrv_info->work_mode != WORK_MODE_SPO2) {
        return;
    }
    if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FC) {
        sample_info_handle->ppg_div = pdrv_info->ppg_div;
        sample_info_handle->ps_div = pdrv_info->ps_div * pdrv_info->fifo_div;
        sample_info_handle->fifo_div = 1;
        core30fx_set_sample_frequency(sample_info_handle);
        core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
    }
#endif
    return;
}

static void drv_calibration_entry_fast_irq(void)
{
    core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
}

static void drv_calibration_exit_fast_irq(void)
{
    if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FS) {
        if (drv_info_handle->work_mode == WORK_MODE_FACTORY0 || drv_info_handle->work_mode == WORK_MODE_FACTORY1) {
            core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD);
        } else {
            core30fx_set_interrupt_config(VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
        }
    } else if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FC) {
        if (drv_info_handle->work_mode == WORK_MODE_FACTORY0 || drv_info_handle->work_mode == WORK_MODE_FACTORY1) {
            core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD);
        } else {
            core30fx_set_interrupt_config(VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
#if VC30Fx_VC30FC_PPGPS_FREQUENCY /* 30FC在需要开同频的模式并且是摘下情况下就需要使能PS中断，因为ps不存入fifo,没有fifo中断 */
            if ((drv_info_handle->work_mode == WORK_MODE_HR || drv_info_handle->work_mode == WORK_MODE_SPO2) && (WEAR_STA_DROP == wear_info_handle->last_status)) {
                core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
            }
#endif
        }
    }
}

static void drv_common_register_config(vc30fx_drv_info *pdrv_info)
{
    sample_info_handle->ppg_div = pdrv_info->ppg_div;
    sample_info_handle->ps_div = pdrv_info->ps_div;
    sample_info_handle->fifo_div = pdrv_info->fifo_div;
    sample_info_handle->fifo_enable = 1;
    if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FC) {
#if(VC30Fx_VC30FC_AUTO_UNDERCLOCKING==VC30Fx_FUNC_ENABLE)
        sample_info_handle->ppg_div = pdrv_info->ppg_div;
        sample_info_handle->ps_div = pdrv_info->ps_div * pdrv_info->fifo_div;
        sample_info_handle->fifo_div = 1;
#endif
    }
    core30fx_set_common_config(sample_info_handle);
    core30fx_set_sample_frequency(sample_info_handle);
    core30fx_set_interrupt_config(VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
    /* check_abnormal */
    VC30Fx_TMP_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*tmp_innenable*/, VC30Fx_CFG_ENABLE /*tmp_extenable*/, 6 /*tmp_setcur*/);
    core30fx_set_temperature_config(sample_info_handle);
}
static int drv_heartrate_mode_init(void)
{
    VC30Fx_STORE_FIFO PS_BIO_FIFO = VC30Fx_IN_STORE;
#if(VC30Fx_VC30FC_PPGPS_FREQUENCY)
    if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FC && drv_info_handle->work_mode == WORK_MODE_HR) {
        PS_BIO_FIFO = VC30Fx_NO_STORE;
        sample_info_handle->ppg_div = drv_info_handle->ppg_div;
        sample_info_handle->ps_div = 1;
        sample_info_handle->fifo_div = drv_info_handle->ps_div * drv_info_handle->fifo_div;
        /* 如果开启PS同频，并且为摘下模式时，初始化时就需要打开PS中断,并且修改PS频率为FIFO频率 */
        if (WEAR_STA_DROP == wear_info_handle->last_status) {
            sample_info_handle->ps_div = sample_info_handle->fifo_div;
            sample_info_handle->fifo_div = 1;
            core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
        }
        core30fx_set_sample_frequency(sample_info_handle);
    }
#endif
    /* ppg0 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[0] /*slot0_ppg*/, VC30Fx_PPG_CURT /*current*/, VC30Fx_PPG_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot0_config(sample_info_handle, VC30Fx_PPG_TXCH_0, VC30Fx_PPG_GAIN /*rxgain*/, 0 /*oversample,2^N*/, VC30Fx_SAMPLE_TIM_100US);
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, PS_BIO_FIFO /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, PS_BIO_FIFO /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    return 0;
}

static int drv_spo2_mode_init(void)
{
    VC30Fx_SAMPLE_TIM spo2_sample_time = VC30Fx_SAMPLE_TIM_100US;
    VC30Fx_STORE_FIFO PS_BIO_FIFO = VC30Fx_IN_STORE;
#if(VC30Fx_VC30FC_PPGPS_FREQUENCY)
    if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FC && drv_info_handle->work_mode == WORK_MODE_SPO2) {
        PS_BIO_FIFO = VC30Fx_NO_STORE;
        sample_info_handle->ppg_div = drv_info_handle->ppg_div;
        sample_info_handle->ps_div = 1;
        sample_info_handle->fifo_div = drv_info_handle->ps_div * drv_info_handle->fifo_div;
        /* 如果开启PS同频，并且为摘下模式时，初始化时就需要打开PS中断,并且修改PS频率为FIFO频率 */
        if (WEAR_STA_DROP == wear_info_handle->last_status) {
            sample_info_handle->ps_div = sample_info_handle->fifo_div;
            sample_info_handle->fifo_div = 1;
            core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD | VC30Fx_EN_EVT_PPG);
        }
        core30fx_set_sample_frequency(sample_info_handle);
    }
#endif
    if (sample_info_handle->ic_type == VC30Fx_IC_TYPE_VC30FC) {
        spo2_sample_time = VC30Fx_SAMPLE_TIM_200US;
    }
    /* ppg0 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[0] /*slot0_ppg*/, VC30Fx_PPG_CURT /*current*/, VC30Fx_PPG_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot0_config(sample_info_handle, VC30Fx_PPG_TXCH_2, VC30Fx_PPG_GAIN /*rxgain*/, 4 /*oversample,2^N*/, spo2_sample_time);
    /* ppg1 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[1] /*slot0_ppg*/, VC30Fx_PPG_CURT /*current*/, VC30Fx_PPG_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot1_config(sample_info_handle, VC30Fx_PPG_TXCH_1, VC30Fx_PPG_GAIN /*rxgain*/, 4 /*oversample,2^N*/, spo2_sample_time);
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, PS_BIO_FIFO /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, PS_BIO_FIFO /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    return 0;
}
static int drv_bloodpressure_mode_init(void)
{
    VC30Fx_STORE_FIFO PS_BIO_FIFO = VC30Fx_IN_STORE;
#if(VC30Fx_VC30FC_PPGPS_FREQUENCY)
    //if( sample_info_handle->ic_type==VC30Fx_IC_TYPE_VC30FC ) PS_BIO_FIFO=VC30Fx_NO_STORE;
#endif
    /* ppg0 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[0] /*slot0_ppg*/, VC30Fx_PPG_CURT /*current*/, VC30Fx_PPG_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot0_config(sample_info_handle, VC30Fx_PPG_TXCH_2, VC30Fx_PPG_GAIN /*rxgain*/, 0 /*oversample,2^N*/, VC30Fx_SAMPLE_TIM_100US);
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, PS_BIO_FIFO /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, PS_BIO_FIFO /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    return 0;
}
static int drv_wear_mode_init(void)
{
    VC30Fx_STORE_FIFO PS_BIO_FIFO = VC30Fx_IN_STORE;
#if(VC30Fx_VC30FC_PPGPS_FREQUENCY)
    //if( sample_info_handle->ic_type==VC30Fx_IC_TYPE_VC30FC ) PS_BIO_FIFO=VC30Fx_NO_STORE;
#endif
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, PS_BIO_FIFO /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, PS_BIO_FIFO /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    return 0;
}
static int drv_factory_green_ps_mode_init(void)
{
    sample_info_handle->fifo_enable = 0;
    core30fx_set_common_config(sample_info_handle);
    /* ppg0 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[0] /*slot0_ppg*/, VC30Fx_FAT_GREEN_MAX_CURT /*current*/, VC30Fx_FAT_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot0_config(sample_info_handle, VC30Fx_PPG_TXCH_0, VC30Fx_FAT_GAIN /*rxgain*/, 0 /*oversample,2^N*/, VC30Fx_SAMPLE_TIM_100US);
    /* ppg1 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[1] /*slot0_ppg*/, VC30Fx_FAT_RED_MAX_CURT /*current*/, VC30Fx_FAT_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot1_config(sample_info_handle, VC30Fx_PPG_TXCH_1, VC30Fx_FAT_GAIN /*rxgain*/, 0 /*oversample,2^N*/, VC30Fx_SAMPLE_TIM_100US);
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, VC30Fx_IN_STORE /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio/temp */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, VC30Fx_IN_STORE /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    /* += enable freq2ps irq */
    core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD);
    return 0;
}
static int drv_factory_ir_red_mode_init(void)
{
    sample_info_handle->fifo_enable = 0;
    core30fx_set_common_config(sample_info_handle);
    /* ppg0 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[0] /*slot0_ppg*/, VC30Fx_FAT_IR_MAX_CURT /*current*/, VC30Fx_FAT_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot0_config(sample_info_handle, VC30Fx_PPG_TXCH_2, VC30Fx_FAT_GAIN /*rxgain*/, 0 /*oversample,2^N*/, VC30Fx_SAMPLE_TIM_100US);
    /* ppg1 */
    VC30Fx_PPG_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[1] /*slot0_ppg*/, VC30Fx_FAT_RED_MAX_CURT /*current*/, VC30Fx_FAT_RESISTANCE /*resistance*/);
    core30fx_sample_ppgslot1_config(sample_info_handle, VC30Fx_PPG_TXCH_1, VC30Fx_FAT_GAIN /*rxgain*/, 0 /*oversample,2^N*/, VC30Fx_SAMPLE_TIM_100US);
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, VC30Fx_IN_STORE /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio/temp */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, VC30Fx_IN_STORE /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    /* += enable freq2ps irq */
    core30fx_set_interrupt_config(VC30Fx_EN_EVT_FREQ2 | VC30Fx_EN_EVT_FIFO | VC30Fx_EN_EVT_OVLD);
    return 0;
}
static int drv_temperature_mode_init(void)
{
    VC30Fx_STORE_FIFO PS_BIO_FIFO = VC30Fx_IN_STORE;
#if(VC30Fx_VC30FC_PPGPS_FREQUENCY)
    //if( sample_info_handle->ic_type==VC30Fx_IC_TYPE_VC30FC ) PS_BIO_FIFO=VC30Fx_NO_STORE;
#endif
    /* ps2 */
    VC30Fx_PS_SLOT_PARAM_CONFIG(sample_info_handle->slot_result[2] /*slot2_ps*/, PS_BIO_FIFO /*storefifo*/, VC30Fx_WEAR_PARAM_PS_CURRENT /*current*/, VC30Fx_WEAR_PARAM_PS_RESISTANCE /*resistance*/);
    core30fx_sample_psslot2_config(sample_info_handle, VC30Fx_WEAR_PARAM_PS_GAIN /*rxgain*/, VC30Fx_SAMPLE_TIM_100US);
    /* bio/temp */
#if (VC30Fx_FUNC_ENABLE == VC30Fx_CFG_WEAR_BIO_EN)
    VC30Fx_BIO_SLOT_PARAM_CONFIG(sample_info_handle->extra_result, VC30Fx_CFG_ENABLE /*bio_innenable*/, VC30Fx_CFG_ENABLE /*bio_extenable*/, PS_BIO_FIFO /*bio_storefifo*/, VC30Fx_CFG_ENABLE /*bio_ppgmode*/);
    core30fx_set_bioslot_config(sample_info_handle, drv_info_handle->optimal_cap);
#endif
    return 0;
}
static int drv_undefined_mode_init(void)
{
    return 0;
}

static void drv_work_init(vcare_ppg_device_t *pdev, int mode, unsigned short int ppg_div, unsigned char ps_div, unsigned short int fifo_div)
{
    if (VC30FS_DEV_REV_ID == pdev->id1 && VC30FS_FDY_VER_CHECK(pdev->id2)) {
        sample_info_handle->ic_type = VC30Fx_IC_TYPE_VC30FS;
        sample_info_handle->ic_FICV = pdev->id2;
        VCARE_DBG_LOG("VC30Fx_IC_TYPE_VC30FS");
    } else if (VC30FC_DEV_REV_ID == pdev->id1 && VC30FC_FDY_VER_CHECK(pdev->id2)) {
        sample_info_handle->ic_type = VC30Fx_IC_TYPE_VC30FC;
        sample_info_handle->ic_FICV = pdev->id2;
        VCARE_DBG_LOG("VC30Fx_IC_TYPE_VC30FC");
    } else {
        VCARE_DBG_ERR("vc30fx id error");
        return ;
    }
    /* set & init param */
    drv_info_handle->work_mode = mode;
    drv_info_handle->ppg_div = ppg_div;
    drv_info_handle->ps_div = ps_div;
    drv_info_handle->fifo_div = fifo_div;
    drv_infomation_param_load();
    core30fx_write_work_cmd(VC30Fx_RESET_SOFT_CMD);
    /* 公共配置初始化 */
    drv_common_register_config(drv_info_handle);
    /* 模式配置初始化 */
    if (sel_mode_ops) {
        if (sel_mode_ops->mode_init) {
            sel_mode_ops->mode_init();
        }
    } else {
        return;
    }
    /* 驱动参数初始化*/
    drv_infomation_param_init(pdev);
#if 1
    unsigned char config_debug_reg[24] = {0};
    unsigned char debug_index = 0;
    vc30fx_read_register(0x40, config_debug_reg, 24);
    VCARE_DBG_LOG("[0x40]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
    VCARE_DBG_LOG("[0x48]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
    VCARE_DBG_LOG("[0x50]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
#endif
    core30fx_set_threshold_config(drv_info_handle->ppg_max_thval, drv_info_handle->ppg_min_thval);
    core30fx_write_work_cmd(VC30Fx_START_WORK_CMD);
}

/********************************************************************************************************
 *佩戴检测部分代码
 ********************************************************************************************************/
#if (VC30Fx_CFG_WEAR_BIOALGO_EN==VC30Fx_FUNC_ENABLE)
static int drv_calc_biology_sense(vc30fx_wear_info *pwear_info, vc30fx_sample_info_t *presult, unsigned short int bioinn_data, unsigned short int bioext_data, unsigned short int ps_data)
{
    static unsigned int calc_ps = 0;
    static int calc_bio_ext = 0;
    int ret = 0;
    if (presult->extra_result.bioext_enwork != 1) {
        return VC30Fx_BIO_STA_NONE;
    }
    /* bio ps 有效判断 */
    if (bioext_data && ps_data) {
        calc_bio_ext = bioext_data;
        calc_ps = ps_data;
    }
    ret = f_BIOdetect(calc_bio_ext, calc_ps, VC30Fx_WEAR_PARAM_PSTH);
    VCARE_DBG_LOG("__f_TPedge(%d,%d,%d)=%d =>[%d]", calc_bio_ext, calc_ps, VC30Fx_WEAR_PARAM_PSTH, ret, pwear_info->bio_wear_result);
    if (VC30Fx_BIO_STA_HOLD == ret) {
        pwear_info->bio_wear_result = VC30Fx_BIO_STA_HOLD;
        pwear_info->algo_wear_status = WEAR_STA_HOLD; /* bio重新佩戴重置心率算法佩戴状态 */
    } else if (VC30Fx_BIO_STA_DROP == ret) {
        pwear_info->bio_wear_result = VC30Fx_BIO_STA_DROP;
    }
    return pwear_info->bio_wear_result;
}
#else
static int drv_calc_biology_sense_absolute(vc30fx_wear_info *pwear_info, vc30fx_sample_info_t *presult, unsigned short int bioinn_data, unsigned short int bioext_data, unsigned short int ps_data)
{
    unsigned short int bio_refence_base = 0;
    unsigned char bio_ps_decline = 0; /* 1-单次下降满足，2-连续下降满足 */
    if (presult->extra_result.bioinn_enwork != 1 && presult->extra_result.bioext_enwork != 1) {
        return VC30Fx_BIO_STA_NONE;
    }
    /* ps 有效判断 */
    if (ps_data) {
        /* 单次下降达到阈值 */
        if (pwear_info->last_ps_data[0] > ps_data && (pwear_info->last_ps_data[0] - ps_data) >= VC30Fx_WEAR_PARAM_PS_DELTA) {
            bio_ps_decline = 1;
        }
        /*连续两次供下降达到了阈值的1.5倍,避免刚好遇到两次采样缓慢下降的情况*/
        if ((pwear_info->last_ps_data[1] > pwear_info->last_ps_data[0]) && (pwear_info->last_ps_data[0] > ps_data) && (pwear_info->last_ps_data[1] - ps_data) >= (VC30Fx_WEAR_PARAM_PS_DELTA * 15 / 10)) {
            bio_ps_decline = 2;
        }
        /* buffer move */
        pwear_info->last_ps_data[1] = pwear_info->last_ps_data[0];
        pwear_info->last_ps_data[0] = ps_data;
    }
    /* bio 有效判断 */
    if (bioinn_data && bioext_data) {
        bio_refence_base = bioinn_data - drv_info_handle->optimal_bio_gap;
        VCARE_DBG_LOG("bio_gap=%d, bio_base=%d, bio_ext=%d, abs_gap(%d?[%d,%d])",
                      drv_info_handle->optimal_bio_gap, bio_refence_base,
                      bioext_data, VC_ABS(bioext_data, bio_refence_base), VC30Fx_WEAR_PARAM_BIO_ABS_DROP, VC30Fx_WEAR_PARAM_BIO_ABS_HOLD);
        presult->extra_result.bioabs_calc = VC_ABS(bioext_data, bio_refence_base);
        if (VC_ABS(bioext_data, bio_refence_base) >= VC30Fx_WEAR_PARAM_BIO_ABS_HOLD) {
            pwear_info->bio_wear_result = VC30Fx_BIO_STA_HOLD;
            pwear_info->algo_wear_status = WEAR_STA_HOLD; /* bio重新佩戴重置心率算法佩戴状态 */
        } else if ((VC_ABS(bioext_data, bio_refence_base) <= VC30Fx_WEAR_PARAM_BIO_ABS_DROP) && bio_ps_decline) {
            pwear_info->bio_wear_result = VC30Fx_BIO_STA_DROP;
        }
    }
    return pwear_info->bio_wear_result;
}
#endif

static vc30fx_wear_change drv_wear_status_identify(vc30fx_wear_info *pwear_info, vc30fx_sample_info_t *presult)
{
    vc30fx_wear_change ret_change = VC30Fx_WEAR_NO_CHANGE;
    int ret_bio_algo = VC30Fx_BIO_STA_NONE;
    /* 根据是否存入fifo对参数进行取值 和 采样次数进行判断 */
    unsigned char data_index = 0;
    unsigned char sample_num = 1;
    unsigned short int ps = 0;
    unsigned short int env = 0;
    unsigned short int bioinn = 0;
    unsigned short int bioext = 0;
    if (presult->extra_result.biodata_storefifo == 1) {
        sample_num = presult->extra_result.bio_fifo_num;
    }
    if (presult->slot_result[2].slot_storefifo == 1) {
        sample_num = presult->slot_result[2].ppg_num;
    }
    VCARE_DBG_LOG("bio_n=%d, slot2ps_n=%d, sample_num=%d", presult->extra_result.bio_fifo_num, presult->slot_result[2].ppg_num, sample_num);
    if (sample_num >= VC30Fx_PS2SLOT_BUFFER_NUM) {
        return ret_change;
    }
    for (data_index = 0; data_index < sample_num; data_index++) {
        if (presult->slot_result[2].slot_storefifo == 1) {
            ps = presult->slot_result[2].ppg_buffer[data_index];
        } else {
            ps = presult->slot_result[2].u.ps_data << 8;
        }
        if (presult->extra_result.biodata_storefifo == 1) {
            bioinn = presult->extra_result.bioinn_buffer[data_index];
            bioext = presult->extra_result.bioext_buffer[data_index];
        } else {
            bioinn = presult->extra_result.bioinn_data;
            bioext = presult->extra_result.bioext_data;
        }
        env = presult->slot_result[2].env_data;
#if (VC30Fx_FUNC_DISABLE == VC30Fx_CFG_WEAR_BIO_EN)
        ret_bio_algo = VC30Fx_BIO_STA_NONE;
#else
#if VC30Fx_CFG_WEAR_BIOALGO_EN
        ret_bio_algo = drv_calc_biology_sense(pwear_info, presult, bioinn, bioext, ps);
#else
        ret_bio_algo = drv_calc_biology_sense_absolute(pwear_info, presult, bioinn, bioext, ps);
#endif
#endif
        VCARE_DBG_LOG("status[%d] ps=%d,env=%d,bio_inn ext=(%d,%d)>bioret=%d", pwear_info->wear_status, ps, env, bioinn, bioext, ret_bio_algo);
        VCARE_DBG_LOG("extern heart_algo_ret=%d,gsnesor_axis_ret=%d", pwear_info->algo_wear_status, pwear_info->acce_wear_status);
#if (VC30Fx_FUNC_DISABLE == VC30Fx_CFG_WEAR_ALG_EN)
        pwear_info->algo_wear_status = WEAR_STA_HOLD;
#endif
#if (VC30Fx_FUNC_DISABLE == VC30Fx_CFG_WEAR_ACC_EN)
        pwear_info->acce_wear_status = WEAR_STA_HOLD;
#endif
        switch (pwear_info->wear_status) {
        case WEAR_STA_DROP:
            if ((ps >= VC30Fx_WEAR_PARAM_PSTH) && (env <= VC30Fx_WEAR_PARAM_ENVMIN) && (VC30Fx_BIO_STA_NONE == ret_bio_algo || VC30Fx_BIO_STA_HOLD == ret_bio_algo) &&
                (WEAR_STA_HOLD == pwear_info->algo_wear_status) && (WEAR_STA_HOLD == pwear_info->acce_wear_status)) {
                VCARE_DBG_LOG("drop->hold_cnt=%d", pwear_info->hold_cnt);
                if (--pwear_info->hold_cnt <= 0) {
                    pwear_info->hold_cnt = VC30Fx_CFG_WEAR_HOLD_CNT;
                    pwear_info->drop_cnt = VC30Fx_CFG_WEAR_DROP_CNT;
                    pwear_info->wear_status = WEAR_STA_HOLD;
                    ret_change = VC30Fx_DROP_TO_HOLD;
                    goto CHANGE_RETURN;
                }
            } else {
                pwear_info->hold_cnt = VC30Fx_CFG_WEAR_HOLD_CNT;
            }
            break;
        case WEAR_STA_HOLD:
            if ((ps < VC30Fx_WEAR_PARAM_PSTH) || (env > VC30Fx_WEAR_PARAM_ENVMAX) || (VC30Fx_BIO_STA_DROP == ret_bio_algo) || (WEAR_STA_DROP == pwear_info->algo_wear_status)) {
                if ((WEAR_STA_DROP == pwear_info->algo_wear_status) || (VC30Fx_BIO_STA_DROP == ret_bio_algo)) {
                    pwear_info->drop_cnt = 1;
                }
                VCARE_DBG_LOG("hold->drop_cnt=%d", pwear_info->drop_cnt);
                if (--pwear_info->drop_cnt <= 0) {
                    pwear_info->hold_cnt = VC30Fx_CFG_WEAR_HOLD_CNT;
                    pwear_info->drop_cnt = VC30Fx_CFG_WEAR_DROP_CNT;
                    pwear_info->wear_status = WEAR_STA_DROP;
                    ret_change = VC30Fx_HOLD_TO_DROP;
                    goto CHANGE_RETURN;
                }
            } else {
                pwear_info->drop_cnt = VC30Fx_CFG_WEAR_DROP_CNT;
            }
            break;
        default:
            break;
        }
    }
CHANGE_RETURN:
    return ret_change;
}

static int drv_read_wear_sample_data(vc30fx_sample_info_t *pwear_result_info)
{
    drv_read_sample_param(pwear_result_info);
    drv_read_fifo_data(pwear_result_info);
    if (!clk_info_handle->clk_calc_status) {
        pwear_result_info->slot_result[0].ppg_num = 0; /* clear ppg_data number, the clock_frequency wait calibration */
        pwear_result_info->slot_result[1].ppg_num = 0;
    }
    return 0;
}

static wear_status drv_get_wear_status(vcare_ppg_device_t *pdev)
{
    wear_status calc_status = wear_info_handle->last_status;
    vc30fx_wear_change wear_change = VC30Fx_WEAR_NO_CHANGE;
    vc30fx_sample_info_t *result_info_ptr = pdev->result;
    unsigned char wear_event = 0;
#if (VC30Fx_CFG_WEAR_DETECTION == VC30Fx_FUNC_DISABLE)
    return WEAR_STA_HOLD;
#endif
    if (result_info_ptr->slot_result[2].slot_enwork == 0) {
        return WEAR_STA_HOLD;
    }
    drv_read_event_status(drv_info_handle);
    if (drv_info_handle->int_event & (VC30Fx_EVENT_CONFLICT)) {
        VCARE_DBG_ERR("wear_work_status=%x", drv_info_handle->int_event);
        return calc_status;
    }
    drv_read_wear_sample_data(result_info_ptr);
    VCARE_DBG_LOG("fifo_read_data_valid=%d", drv_info_handle->fifo_read_data_valid);
#if VC30Fx_VC30FC_PPGPS_FREQUENCY /* 同频情况下，当摘下时PS不存fifo时，无fifo数据，fifo_read_data_valid无法置1，需要根据中断原因来执行佩戴检测 */
    if (1 != result_info_ptr->ps_div && drv_info_handle->int_event & VC30Fx_EVENT_INTPS_RDY) {
        wear_event = 1;
    }
#endif
    /* fifo data valid,check wear status */
    if (drv_info_handle->fifo_read_data_valid || wear_event) {
        wear_change = drv_wear_status_identify(wear_info_handle, result_info_ptr);
        if (VC30Fx_DROP_TO_HOLD == wear_change) {
            calc_status = WEAR_STA_HOLD;
            wear_info_handle->wear_status = calc_status;
            wear_info_handle->last_status = calc_status;
            pdev->wear = calc_status;
            drv_calibration_clk_clear();
            drv_work_init(pdev, drv_info_handle->work_mode, drv_info_handle->ppg_div, drv_info_handle->ps_div, drv_info_handle->fifo_div);
        } else if (VC30Fx_HOLD_TO_DROP == wear_change) {
            calc_status = WEAR_STA_DROP;
            wear_info_handle->wear_status = calc_status;
            wear_info_handle->last_status = calc_status;
            pdev->wear = calc_status;
            /* 必须重新初始化，重置windex,否则摘下的瞬间，会出现windex错误偏移，导致数据解析出错 */
            drv_work_init(pdev, drv_info_handle->work_mode, drv_info_handle->ppg_div, drv_info_handle->ps_div, drv_info_handle->fifo_div);
            /* 如果开启同频配置，修改分频设置和中断使能配置 */
            drv_drop_modify_div_irq_config(drv_info_handle);
        }
    }
    return calc_status;
}

/********************************************************************************************************
 * 自动化调整部分代码
 ********************************************************************************************************/
static int drv_adjust_overload_max_current(vc30fx_sample_info_t *psample_result_info, unsigned char overload, int event)
{
    int ret = 0;
    unsigned char slot_i = 0;
    vc30fx_adjust_info *padjust_info = NULL;
    unsigned char ovload_mark[3] = {overload & VC30Fx_STATUS_CH0_OVERLOAD, overload & VC30Fx_STATUS_CH1_OVERLOAD, overload & VC30Fx_STATUS_CH2_OVERLOAD};
    for (slot_i = 0; slot_i < 2; slot_i++) {
        padjust_info = adjust_info_handle[slot_i];
        if (ovload_mark[slot_i]) {
            VCARE_DBG_LOG("ovload_mark[%d] now_cur=%d", slot_i, padjust_info->current);
            if (padjust_info->current != 0) {
                padjust_info->current--;
                padjust_info->max_current = padjust_info->current;
                /* set adjust mark & adjust param */
                core30fx_set_ppgslot_current((VC30Fx_PPG_SLOT_I)slot_i, padjust_info->current);
            } else {
                ret = -1;
            }
        }
    }
    if (ovload_mark[2]) {
        ret = -1;
    }
    return ret;
}
static int drv_adjust_slot_current_res(vc30fx_adjust_info *padjust_info, unsigned char slot_index, unsigned char thh_res_level, unsigned char thh_current)
{
    unsigned char allow_oversample = 0;
    unsigned char old_res_level = padjust_info->res_level;
    unsigned char old_current = padjust_info->current;
    unsigned short int old_current_step = padjust_info->adj_param.current_step;
    //VCARE_DBG_LOG("ADF_SLOT[%d]-dir(%d)-I_R thh[%d,%d] I_R now(%d,%d)", slot_index, padjust_info->adj_param.direction, thh_res_level, thh_current, old_res_level, old_current);
    if (padjust_info->current == thh_current) {
        if (padjust_info->res_level == thh_res_level) {
            VCARE_DBG_ERR("RESLVL-OFFSET-LOW IS MAX/MIN--[%d]>=[%d]", old_res_level, thh_res_level);
            return -1;
        } else {
            if (padjust_info->adj_param.direction == VC30Fx_ADJUST_DIR_UP) {
                padjust_info->res_level = (old_res_level >= thh_res_level) ? (thh_res_level) : (old_res_level + 1);
            } else {
                padjust_info->res_level = (old_res_level <= thh_res_level) ? (thh_res_level) : (old_res_level - 1);
            }
        }
    } else {
        /* calc step */
        if (padjust_info->adj_param.direction_last == VC30Fx_ADJUST_DIR_NONE) {
            old_current_step *= 16;
        } else if (padjust_info->adj_param.direction_last == padjust_info->adj_param.direction) {
            old_current_step = (old_current_step == 1 || old_current_step == 2) ? ((old_current_step + 1) * 16) : (old_current_step * VC30Fx_ADJ_I_INCREASE); /* 同向加大步长，但是在1和2的时候，1.4倍无法使整型增长,就直接+1 */
        } else {
            old_current_step = old_current_step * VC30Fx_ADJ_I_DECREASE;
        }
        padjust_info->adj_param.current_step = old_current_step >> 4;
        padjust_info->adj_param.current_step = (padjust_info->adj_param.current_step <= VC30Fx_ADJ_I_STEP_MIN) ? VC30Fx_ADJ_I_STEP_MIN : padjust_info->adj_param.current_step;
        padjust_info->adj_param.current_step = (padjust_info->adj_param.current_step >= VC30Fx_ADJ_I_STEP_MAX) ? VC30Fx_ADJ_I_STEP_MAX : padjust_info->adj_param.current_step;
        /* adjust current */
        if (VC30Fx_ADJUST_DIR_UP == padjust_info->adj_param.direction) {
            padjust_info->current = ((old_current + padjust_info->adj_param.current_step) > thh_current) ? thh_current : (old_current + padjust_info->adj_param.current_step);
        } else if (VC30Fx_ADJUST_DIR_DOWN == padjust_info->adj_param.direction) {
            padjust_info->current = (old_current < padjust_info->adj_param.current_step) ? thh_current : old_current - padjust_info->adj_param.current_step;
        }
        padjust_info->adj_param.direction_last = padjust_info->adj_param.direction;
    }
    VCARE_DBG_LOG("index=%d,res_level=%d,current=%d,step=%d", slot_index, padjust_info->res_level, padjust_info->current, padjust_info->adj_param.current_step);
    if (WORK_MODE_HR == drv_info_handle->work_mode) { /* 暂定得是只有心率模式下支持修改过采配置 */
        allow_oversample = 1;
    }
    core30fx_set_ppgslot_resistance((VC30Fx_PPG_SLOT_I)slot_index, padjust_info->res_level, allow_oversample, sample_info_handle->ic_type);
    core30fx_set_ppgslot_current((VC30Fx_PPG_SLOT_I)slot_index, padjust_info->current);
    return 0;
}

static int drv_adjust_slot_ppg_overflow(vc30fx_sample_info_t *psample_result_info, int event)
{
    unsigned char slot_i = 0;
    unsigned short int over_ppgdata = 0;
    vc30fx_adjust_info *padjust_info = NULL;
    vc30fx_sample_slot_t *pslot_ret = NULL;
    for (slot_i = 0; slot_i < 2; slot_i++) {
        padjust_info = adjust_info_handle[slot_i];
        pslot_ret = &psample_result_info->slot_result[slot_i];
        over_ppgdata = pslot_ret->u.pre_data;
        if (1 == pslot_ret->slot_enwork) {
            VCARE_DBG_LOG("ppg_overflow(%d)=4bit[%d]", over_ppgdata, VC30Fx_PPG_CONV_BIT4(over_ppgdata));
            if (VC30Fx_PPG_CONV_BIT4(over_ppgdata) > drv_info_handle->ppg_max_thval) {
                padjust_info->adj_param.direction = VC30Fx_ADJUST_DIR_DOWN;
                drv_adjust_slot_current_res(padjust_info, slot_i, padjust_info->const_min_res_level, padjust_info->const_min_current);
            } else if (VC30Fx_PPG_CONV_BIT4(over_ppgdata) < drv_info_handle->ppg_min_thval) {
                padjust_info->adj_param.direction = VC30Fx_ADJUST_DIR_UP;
                drv_adjust_slot_current_res(padjust_info, slot_i, padjust_info->max_res_level, padjust_info->max_current);
            } else {
                padjust_info->adj_param.direction = VC30Fx_ADJUST_DIR_NONE;
                padjust_info->adj_param.direction_last = VC30Fx_ADJUST_DIR_NONE;
                VCARE_DBG_LOG("slot[%d] in the range,clear slot adjustment direction", slot_i);
            }
        }
    }
    return 0;
}

static int drv_auto_adjust_slot_config(int event, unsigned char ov_status)
{
    if (wear_info_handle->wear_status == WEAR_STA_DROP) {
        return -1;
    }
    if ((event & VC30Fx_EVENT_INTOVLOAD) || VC30Fx_CFG_IRQ_BY_TIMER_EN) {
        drv_adjust_overload_max_current(sample_info_handle, ov_status, event);
    }
    if ((event & VC30Fx_EVENT_INTPPG_STA) || VC30Fx_CFG_IRQ_BY_TIMER_EN) {
        drv_adjust_slot_ppg_overflow(sample_info_handle, event);
    } else {
        /* clear adjust_param */
        adjust_info_handle[0]->adj_param.direction = VC30Fx_ADJUST_DIR_NONE;
        adjust_info_handle[0]->adj_param.direction_last = VC30Fx_ADJUST_DIR_NONE;
        adjust_info_handle[1]->adj_param.direction = VC30Fx_ADJUST_DIR_NONE;
        adjust_info_handle[1]->adj_param.direction_last = VC30Fx_ADJUST_DIR_NONE;
    }
    return 0;
}

/********************************************************************************************************
 * 时钟校准部分
 *******************************************************************************************************/
static int drv_calibration_oscclk_frequency(vc30fx_clk_info *pclk_info)
{
    unsigned char count_vaild = 1;
    unsigned char i = 0;
    unsigned int clock_tick[2] = {0};
    unsigned short int osc_tick[2] = {0};
    unsigned short osc_frequency[2];

    clock_tick[0] = vc30fx_get_cputimer_tick();
    core30fx_read_rcosc_tick(sample_info_handle, &osc_tick[0], &osc_tick[1]);
    clock_tick[1] = vc30fx_get_cputimer_tick();
    /*========~~~~~~~~~~~~~~~~~~~~~~AK-自定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
    printf(" -0-[MY]--------------  [%s]:%d  --------------osc:%d %d  clock:%d %d\n", __func__, __LINE__, osc_tick[0], osc_tick[1], clock_tick[0], clock_tick[1]);
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~AK-自定义-结束~~~~~~~~~~~~~~~~~~========*/
    if ((osc_tick[0] > osc_tick[1]) || (clock_tick[0] > clock_tick[1])) {
        pclk_info->count_index = 0;
        VCARE_DBG_ERR("osc/clk ticks is error\n");
        return 1;
    }
    pclk_info->osc_count[pclk_info->count_index * 2] = osc_tick[0];
    pclk_info->osc_count[pclk_info->count_index * 2 + 1] = osc_tick[1];
    pclk_info->clk_count[pclk_info->count_index * 2] = clock_tick[0];
    pclk_info->clk_count[pclk_info->count_index * 2 + 1] = clock_tick[1];
    VCARE_DBG_LOG("[%d]osc_count=%d, %d, %d, %d, %d, %d\n", pclk_info->count_index, pclk_info->osc_count[0], pclk_info->osc_count[1], pclk_info->osc_count[2], pclk_info->osc_count[3], pclk_info->osc_count[4], pclk_info->osc_count[5]);
    VCARE_DBG_LOG("[%d]clk_count=%d, %d, %d, %d, %d, %d\n", pclk_info->count_index, pclk_info->clk_count[0], pclk_info->clk_count[1], pclk_info->clk_count[2], pclk_info->clk_count[3], pclk_info->clk_count[4], pclk_info->clk_count[5]);
    pclk_info->count_index++;
    if (pclk_info->count_index >= 3) {
        pclk_info->count_index = 0;
        for (i = 0; i < 2; i++) {
            count_vaild = count_vaild && (VC_ABS(VC_ABS_MAX(pclk_info->clk_count[2 * i + 2], pclk_info->clk_count[2 * i], 0xffff), VC_ABS_MAX(pclk_info->clk_count[2 * i + 3], pclk_info->clk_count[2 * i + 1], 0xffff)) < VC30Fx_CFG_CLK_CALC_DEVIATION);
            count_vaild = count_vaild && (VC_ABS(VC_ABS_MAX(pclk_info->osc_count[2 * i + 2], pclk_info->osc_count[2 * i], VC30Fx_CFG_MCU_TICK_MAX), VC_ABS_MAX(pclk_info->osc_count[2 * i + 3], pclk_info->osc_count[2 * i + 1], VC30Fx_CFG_MCU_TICK_MAX)) < VC30Fx_CFG_CLK_CALC_DEVIATION);
            count_vaild = count_vaild && (!(pclk_info->clk_count[2 * i + 2] == pclk_info->clk_count[2 * i])); /* avoid divisor==0, diffCountIsOk set false */
            count_vaild = count_vaild && (!(pclk_info->osc_count[2 * i + 2] == pclk_info->osc_count[2 * i])); /* avoid divisor==0, diffCountIsOk set false */
        }
        printf(" -0- count_vaild=%d\n", count_vaild);
        if (count_vaild) {
#if MY_VC30FX_MODIFY
            // null
#else
#if !PMC_DEVELOP_ENABLE
            VC30Fx_CFG_MCU_TICK_FREQUENCY = __get_lrc_hz() / 4;
#endif
#endif
            for (i = 0; i < 2; i++) {
                osc_frequency[i] = VC_ABS_MAX((unsigned int)pclk_info->osc_count[2 * i + 2], pclk_info->osc_count[2 * i], 0xffff) * VC30Fx_CFG_MCU_TICK_FREQUENCY / VC_ABS_MAX((unsigned int)pclk_info->clk_count[2 * i + 2], pclk_info->clk_count[2 * i], VC30Fx_CFG_MCU_TICK_MAX);

                VCARE_DBG_LOG("f20kOsc[20K_%d]=(%d)", i, osc_frequency[i]);
            }
            if (VC_ABS(osc_frequency[0], osc_frequency[1]) <= 20) {
                pclk_info->rcosc_frequency = (osc_frequency[0] + osc_frequency[1]) / 2;
                pclk_info->clk_calc_freq_status = 1;
            }
            return 0;
        }
    }
    return 2;
}

static int drv_auto_calibration_oscclk(vc30fx_clk_info *pclk_info)
{
    /* calibration the osc frequency by method of bisection */
    if (pclk_info->clk_calc_freq_status && pclk_info->rcosc_frequency) {
        pclk_info->clk_calc_freq_status = 0;
        if (pclk_info->rcosc_frequency >= VC30Fx_FREQUENCY_MAX) {
            rcosc_min = (rcosc_mid < rcosc_max_thh) ? rcosc_mid + 1 : rcosc_max_thh;
        } else if (pclk_info->rcosc_frequency <= VC30Fx_FREQUENCY_MIN) {
            rcosc_max = (rcosc_mid > rcosc_min_thh) ? (rcosc_mid - 1) : rcosc_min_thh;
        } else {
            pclk_info->clk_calc_status = 1; /* calibration finished mark */
            pclk_info->osc_gap_cnt = 0;
            rcosc_max = rcosc_mid + 2; /* 修正成功后，扩张一个调整点的范围，以便温漂时调整余量 */
            rcosc_min = rcosc_mid - 2;
            VCARE_DBG_LOG("clk_calc_status=%d,osc_gap_cnt=%d", pclk_info->clk_calc_status, pclk_info->osc_gap_cnt);
        }
        rcosc_mid = (rcosc_max + rcosc_min) / 2;
        VCARE_DBG_LOG("min,max[%d,%d]~min(%d)", rcosc_min, rcosc_max, rcosc_mid);
        core30fx_set_rcosc_frequency(rcosc_mid, sample_info_handle->ic_type);
        pclk_info->rcosc_calibration = rcosc_mid;
        /* 如果发生温漂，修改收缩的上下限，进行缓慢修正时钟 */
        if ((pclk_info->rcosc_frequency <= VC30Fx_FREQUENCY_MIN) && VC_ABS(rcosc_max, rcosc_min) <= 2) {
            rcosc_min -= 2;
            VCARE_DBG_LOG("!!!temp->min,max[%d,%d]~min(%d)", rcosc_min, rcosc_max, rcosc_mid);
        }
        if ((pclk_info->rcosc_frequency >= VC30Fx_FREQUENCY_MAX) && VC_ABS(rcosc_max, rcosc_min) <= 2) {
            rcosc_max += 2;
            VCARE_DBG_LOG("!!!temp->min,max[%d,%d]~min(%d)", rcosc_min, rcosc_max, rcosc_mid);
        }
    }
    return 0;
}

static int drv_calibration_oscclk_config(vcare_ppg_device_t *pdev)
{
    vc30fx_clk_info *pclk_info = clk_info_handle;
#if 1 /* 温度校验只会在校验一次的情况生效，否则会因为后续对主频的修正导致温度异常校验失效 */
    if (pclk_info->clk_calc_status)
#else
    if (pclk_info->clk_calc_status && pclk_info->osc_gap_cnt <= VC30Fx_CFG_CLK_CALC_GAPTIME)
#endif
    {
        VCARE_DBG_LOG("status[%d],osc_gap_cnt[%d]<=%d", pclk_info->clk_calc_status, pclk_info->osc_gap_cnt, VC30Fx_CFG_CLK_CALC_GAPTIME);
        pclk_info->osc_gap_cnt++;
        return 0;
    }
    /* calibration clk running, enter fast irq config */
    drv_calibration_entry_fast_irq();
    drv_calibration_oscclk_frequency(pclk_info);
    drv_auto_calibration_oscclk(pclk_info);
    if (pclk_info->clk_calc_status) { /* calibration clk finished, exit fast irq config */
        startup_inntemperature = sample_info_handle->extra_result.tempinn_data;
        drv_calibration_exit_fast_irq();
    }
    return 1;
}
static void drv_calibration_clk_clear(void)
{
    static unsigned char is_first_run = 1;
    vc30fx_clk_info *pclk_info = clk_info_handle;
    pclk_info->last_check_ticks = 0;
    pclk_info->osc_gap_cnt = 0;
    pclk_info->count_index = 0;
    pclk_info->clk_calc_freq_status = 0;
    /* 校验频率温度清空 */
    check_cpu_tick[0] = 0, check_cpu_tick[1] = 0;
    check_dev_tick[0] = 0, check_dev_tick[1] = 0;
    if (is_first_run == 1) {
        is_first_run = 0;
        if (VC30FS_FDY_VER_ID0 == sample_info_handle->ic_FICV || VC30FC_FDY_VER_ID0 == sample_info_handle->ic_FICV) {
            rcosc_max_thh = 127, rcosc_min_thh = 0;
        } else if (VC30FS_FDY_VER_ID1 == sample_info_handle->ic_FICV || VC30FC_FDY_VER_ID1 == sample_info_handle->ic_FICV) {
            rcosc_max_thh = 255, rcosc_min_thh = 0;
        }
        rcosc_max = rcosc_max_thh;
        rcosc_mid = (rcosc_max_thh + rcosc_min_thh) / 2;
        rcosc_min = rcosc_min_thh;
        pclk_info->clk_calc_status = 0;
        core30fx_set_rcosc_frequency(rcosc_mid, sample_info_handle->ic_type);
    } else {
        if (pclk_info->rcosc_calibration == 0) {
            rcosc_max = rcosc_max_thh;
            rcosc_mid = (rcosc_max_thh + rcosc_min_thh) / 2;
            rcosc_min = rcosc_min_thh;
            pclk_info->clk_calc_status = 0;
            pclk_info->rcosc_calibration = rcosc_mid;
        }
        core30fx_set_rcosc_frequency(pclk_info->rcosc_calibration, sample_info_handle->ic_type);
    }
}

static int drv_check_running_gaptime(void)
{
#if VC30Fx_CFG_CLK_CLAC_IRQGAP_EN
    int ret = 0;
    unsigned short int curr_ticks = 0;
    unsigned short int ticks_chk, ticks_gap = 0;
    unsigned short int ticks1 = 0, ticks2 = 0;
    unsigned char times_gap = 0;
    core30fx_read_rcosc_tick(sample_info_handle, &ticks1, &ticks2);
    times_gap = (1000 * drv_info_handle->ppg_div / VC30Fx_DEFAULT_FREQ) * VC30Fx_IRQ_GAP_RATIO;
    ticks_chk = (ticks2 >= ticks1) ? (ticks2 - ticks1) : (0xffff + ticks2 - ticks1);
    if (ticks_chk <= 10) {
        curr_ticks = ticks1;
    } else {
        /* IC问题，可能出现其中一个tick低位出现异常，如果出现异常则取另外一共tick作为运算准 */
        curr_ticks = (ticks2 & 0xff) ? (ticks2) : (ticks1);
        VCARE_DBG_ERR("read check tick failed(%d,%d)", ticks1, ticks2);
    }
    // ticks_gap = 2000;
    if (0 != curr_ticks && 0 != clk_info_handle->last_check_ticks) {
        ticks_gap = (curr_ticks >= clk_info_handle->last_check_ticks) ? (curr_ticks - clk_info_handle->last_check_ticks) : (0xffff + curr_ticks - clk_info_handle->last_check_ticks);
    }
    if (ticks_gap <= 20 * times_gap) { /* 20k,1ms~20ticks */
        VCARE_DBG_ERR("last(%d)now(%d)gap[%d<%d]", clk_info_handle->last_check_ticks, curr_ticks, ticks_gap, 20 * times_gap);
        ret = -1;
    }
    clk_info_handle->last_check_ticks = curr_ticks;
    return ret;
#else
    return 0;
#endif
}

static void drv_fifo_error_softreset(void)
{
    static unsigned char last_add_mark = 0;
    unsigned char save_config[24] = {0};
    /* 软复位，清空所有数据，避免传入算法或者引起其他异常 */
    drv_sample_result_memroy_finished(sample_info_handle);
    vc30fx_read_register(0x40, save_config, sizeof(save_config));
    core30fx_write_work_cmd(VC30Fx_STOP_WORK_CMD);
    core30fx_write_work_cmd(VC30Fx_RESET_SOFT_CMD);
    drv_info_handle->read_fifo_address = 0;
    vc30fx_write_register(0x40, save_config, sizeof(save_config));
    core30fx_write_work_cmd(VC30Fx_START_WORK_CMD);
    if (last_add_mark == 0) {
        last_add_mark = 1;
        core30fx_set_rcosc_frequency(clk_info_handle->rcosc_calibration - 1, sample_info_handle->ic_type);
    } else {
        last_add_mark = 0;
        core30fx_set_rcosc_frequency(clk_info_handle->rcosc_calibration + 1, sample_info_handle->ic_type);
    }
#if 0
    unsigned char config_debug_reg[24] = {0};
    unsigned char debug_index = 0;
    vc30fx_read_register(0x40, config_debug_reg, 24);
    VCARE_DBG_LOG("[0x40]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
    VCARE_DBG_LOG("[0x48]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
    VCARE_DBG_LOG("[0x50]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
#endif
}

/********************************************************************************************************
 *数据读取部分
 ********************************************************************************************************/
static int drv_read_fifo_data(vc30fx_sample_info_t *psample_result)
{
    int read_fifo_ret = 0;
    vc30fx_sample_info_t *sample_result_ptr = psample_result;
    if (!sample_result_ptr) {
        return -1;
    }
    /* fifo获取 */
    if (0 != drv_info_handle->fifo_read_finished_sync) {
        return 0;
    }
    drv_info_handle->fifo_read_finished_sync = 1;
    read_fifo_ret = core30fx_read_fifo_data(sample_result_ptr, &drv_info_handle->read_fifo_address, drv_info_handle->write_fifo_address_bak);
    VCARE_DBG_LOG("read_fifo updata read address: %d , write_address_bak:%d, write_address_rel:%d ", drv_info_handle->read_fifo_address, drv_info_handle->write_fifo_address_bak, drv_info_handle->write_fifo_address_rel);
    if (read_fifo_ret >= 0) {
        drv_info_handle->fifo_read_data_valid = 1;
        if (VC30FS_FDY_VER_ID1 == sample_result_ptr->ic_FICV || VC30FC_FDY_VER_ID1 == sample_result_ptr->ic_FICV) {
            if (drv_info_handle->read_fifo_address != drv_info_handle->write_fifo_address_bak) {
                drv_info_handle->fifo_read_data_valid = 2;
                drv_info_handle->read_fifo_address = drv_info_handle->write_fifo_address_bak;
                drv_sample_result_memroy_finished(sample_info_handle);
            }
        } else if (VC30FS_FDY_VER_ID0 == sample_result_ptr->ic_FICV || VC30FC_FDY_VER_ID0 == sample_result_ptr->ic_FICV) {
            if (read_fifo_ret == 1 && drv_info_handle->int_event & VC30Fx_EVENT_INTFIFO_RDY) { /* 当出现了fifo中断，且并不是fifo数量的倍数时，可以认定为错位，进行纠错处理 */
                drv_info_handle->read_fifo_address = drv_info_handle->write_fifo_address_bak;
                drv_info_handle->fifo_read_data_valid = 2;
                drv_sample_result_memroy_finished(sample_info_handle);
                VCARE_DBG_LOG("delay response read_addr updata:%d=write_addr:%d", drv_info_handle->read_fifo_address, drv_info_handle->write_fifo_address_bak);
            }
            /* 20240119.校验RO_PS与fifo_PS是否异常（如果异常则可以判断为fifo异常错位：1.过度延后导致的fifo覆盖反转，2.ICfifo数据异常丢失） */
            if (sample_result_ptr->slot_result[2].slot_enwork && sample_result_ptr->slot_result[2].ppg_num != 0) {
                if (sample_result_ptr->slot_result[2].ppg_buffer[sample_result_ptr->slot_result[2].ppg_num - 1] >> 8 != sample_result_ptr->slot_result[2].u.ps_data) {
                    VCARE_DBG_LOG("===PS_FIFO_ERROR===") ;
                    drv_fifo_error_softreset();
                }
            }
        }
        /* 在insample的时候，如果出现中断延时处理，获取到的RO_bio的值可能还未完全采样计算完成，需要从fifo中更新数据 */
        if (drv_info_handle->int_event & VC30Fx_EVENT_INSAMPLE && 1 == sample_result_ptr->extra_result.biodata_storefifo) {
            if (sample_result_ptr->extra_result.bioext_data != sample_result_ptr->extra_result.bioext_buffer[sample_result_ptr->extra_result.bio_fifo_num - 1]) {
                sample_result_ptr->extra_result.bioext_data = sample_result_ptr->extra_result.bioext_buffer[sample_result_ptr->extra_result.bio_fifo_num - 1];
                VCARE_DBG_ERR("INSAMPLE_ERR_EXTBIO-modify=%d", sample_result_ptr->extra_result.bioext_data);
            }
            if (sample_result_ptr->extra_result.bioinn_data != sample_result_ptr->extra_result.bioinn_buffer[sample_result_ptr->extra_result.bio_fifo_num - 1]) {
                sample_result_ptr->extra_result.bioinn_data = sample_result_ptr->extra_result.bioinn_buffer[sample_result_ptr->extra_result.bio_fifo_num - 1];
                VCARE_DBG_ERR("INSAMPLE_ERR_INNBIO-modify=%d", sample_result_ptr->extra_result.bioinn_data);
            }
        }
    } else if (read_fifo_ret < 0) {
        VCARE_DBG_ERR("read_fifo_ret error(%d)", read_fifo_ret);
        return read_fifo_ret;
    }
    return 0;
}

static int drv_read_sample_param(vc30fx_sample_info_t *psample_ret)
{
    unsigned char slot_i = 0;
    unsigned char slot_current = 0, slot_res_lvl = 0;
    if (0 == drv_info_handle->param_read_finished_sync) {
        core30fx_read_output(psample_ret);
        VCARE_DBG_LOG("env_hex=%x-%x-%x", psample_ret->slot_result[0].env_data, psample_ret->slot_result[1].env_data, psample_ret->slot_result[2].env_data);
        VCARE_DBG_LOG("ppg/ps_data=%d-%d-%d", psample_ret->slot_result[0].u.ps_data, psample_ret->slot_result[1].u.ps_data, psample_ret->slot_result[2].u.ps_data);
        VCARE_DBG_LOG("bio01_data=%d-%d", psample_ret->extra_result.bioinn_data, psample_ret->extra_result.bioext_data);
        VCARE_DBG_LOG("temp01_data=%d-%d-ref(%d)", psample_ret->extra_result.tempinn_data, psample_ret->extra_result.tempext_data, psample_ret->extra_result.tempref_data);
        if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FS) {
            psample_ret->slot_result[0].env_data = VC30FS_ENV_CONVERT(psample_ret->slot_result[0].env_data);
            psample_ret->slot_result[1].env_data = VC30FS_ENV_CONVERT(psample_ret->slot_result[1].env_data);
            psample_ret->slot_result[2].env_data = VC30FS_ENV_CONVERT(psample_ret->slot_result[2].env_data);
        } else if (psample_ret->ic_type == VC30Fx_IC_TYPE_VC30FC) {
            psample_ret->slot_result[0].env_data = VC30FC_ENV_CONVERT(psample_ret->slot_result[0].env_data);
            psample_ret->slot_result[1].env_data = VC30FC_ENV_CONVERT(psample_ret->slot_result[1].env_data);
            psample_ret->slot_result[2].env_data = VC30FC_ENV_CONVERT(psample_ret->slot_result[2].env_data);
        }
        for (slot_i = 0; slot_i < 2; slot_i++) {
            core30fx_read_slot_current_res((VC30Fx_PPG_SLOT_I)slot_i, &slot_current, &slot_res_lvl);
            psample_ret->slot_result[slot_i].slot_current = slot_current;
            psample_ret->slot_result[slot_i].slot_resistance = slot_res_lvl;
            adjust_info_handle[slot_i]->current = slot_current;
            adjust_info_handle[slot_i]->res_level = slot_res_lvl;
        }
        drv_info_handle->param_read_finished_sync = 1;
        VCARE_DBG_LOG("slot[0]cur%d,res%d ^ slot[1]cur%d,res%d ^ slot[2]cur%d,res%d",
                      psample_ret->slot_result[0].slot_current, psample_ret->slot_result[0].slot_resistance,
                      psample_ret->slot_result[1].slot_current, psample_ret->slot_result[1].slot_resistance,
                      psample_ret->slot_result[2].slot_current, psample_ret->slot_result[2].slot_resistance);
    }
    return 0;
}

static int drv_read_event_status(vc30fx_drv_info *pdrv_info)
{
    if (0 == pdrv_info->event_read_finished_sync) {
        core30fx_read_event_status(sample_info_handle, &pdrv_info->int_event, &pdrv_info->ov_status, &pdrv_info->write_fifo_address_bak, &pdrv_info->write_fifo_address_rel);
        pdrv_info->event_read_finished_sync = 1;
        VCARE_DBG_LOG("read_event=[%x], read_status=[%x], read_Windex=bak[%x]~rel[%x]", pdrv_info->int_event, pdrv_info->ov_status, pdrv_info->write_fifo_address_bak, pdrv_info->write_fifo_address_rel);
    }
    return 0;
}

/********************************************************************************************************
 * 中断事件处理部分调用逻辑
 ********************************************************************************************************/
static int drv_ppg_mode_handler(const void *ops_self, vcare_ppg_device_t *pdev)
{
    int ret = 0;
    vc30fx_sample_info_t *sample_result_ptr = pdev->result;
    vc30fx_mode_ops *self_ptr = (vc30fx_mode_ops *)ops_self;
    drv_calibration_oscclk_config(pdev);
    if (1 == clk_info_handle->clk_calc_status) {
        ret = drv_auto_adjust_slot_config(drv_info_handle->int_event, drv_info_handle->ov_status);
    }
    if (ret >= 0 && 1 == clk_info_handle->clk_calc_status) {
        drv_read_fifo_data(sample_result_ptr);
    } else {
        VCARE_DBG_ERR("WORK_MODE_[%d] clk_status[%d]", self_ptr->mode, clk_info_handle->clk_calc_status);
    }
    return ret;
}
static int drv_factory_mode_handler(const void *ops_self, vcare_ppg_device_t *pdev)
{
    int ret = 0;
    unsigned short int base = 0;
    vc30fx_sample_info_t *sample_result_ptr = pdev->result;
    drv_calibration_oscclk_config(pdev);
    if (drv_info_handle->int_event & VC30Fx_EVENT_INTOVLOAD) {
        ret = drv_adjust_overload_max_current(sample_result_ptr, drv_info_handle->ov_status, drv_info_handle->int_event);
    }
    drv_read_fifo_data(sample_result_ptr);
    if (1 == clk_info_handle->clk_calc_status) {
        drv_calc_bioinn_auto_addcap(drv_info_handle->int_event, sample_result_ptr);
    }
    base = sample_result_ptr->extra_result.bioinn_data - drv_info_handle->optimal_bio_gap;
    sample_result_ptr->extra_result.bioabs_calc = VC_ABS(sample_result_ptr->extra_result.bioext_data, base);
    return ret;
}
static int drv_undefine_mode_handler(const void *ops_self, vcare_ppg_device_t *pdev)
{
    return 0;
}

/*********************************************************************************************************
 * DRV_API 对外功能模块部分
 ********************************************************************************************************/
int vc30fx_drv_init_start_work(void *pdevice, int workmode, unsigned short int ppgdiv, unsigned char psdiv, unsigned char fifodiv)
{
    vcare_ppg_device_t *pdev = (vcare_ppg_device_t *)pdevice;
    char *drv_ver = NULL;
    unsigned char mode_count = 0;
    unsigned char mode_index = 0;
    vcare_memset(sample_info_handle, 0, sizeof(vc30fx_sample_info_t));
    pdev->result = sample_info_handle; /* 采样指针传递 */
    core30fx_read_software_version(&drv_ver);
    core30fx_read_id(&pdev->id1, &pdev->id2);
    VCARE_DBG_LOG("[%s]devID0-2=0x%02x 0x%02x", drv_ver, pdev->id1, pdev->id2);
    mode_count = sizeof(mode_ops) / sizeof(mode_ops[0]);
    sel_mode_ops = NULL;
    for (mode_index = 0; mode_index < mode_count; mode_index++) {
        if (workmode == mode_ops[mode_index].mode) {
            sel_mode_ops = &mode_ops[mode_index];
            break;
        }
    }
    if (!sel_mode_ops) {
        VCARE_DBG_ERR("work_mode is error(NULL),please code mode");
        return -1;
    }
    drv_work_init(pdev, workmode, ppgdiv, psdiv, fifodiv);
    return 0;
}
int vc30fx_drv_get_hwsw_infomation(char **str_drv_ver, unsigned char *pid1, unsigned char *pid2)
{
    core30fx_read_software_version(str_drv_ver);
    core30fx_read_id(pid1, pid2);
    return 0;
}
int vc30fx_drv_exit_stop_work(void *pdevice)
{
    core30fx_write_work_cmd(VC30Fx_RESET_SOFT_CMD);
    core30fx_write_work_cmd(VC30Fx_STOP_WORK_CMD);
    return 0;
}
int vc30fx_drv_software_reset(void *pdevice)
{
    return core30fx_write_work_cmd(VC30Fx_RESET_SOFT_CMD);
}
int vc30fx_drv_wakeup_work(void *pdevice)
{
    return core30fx_write_work_cmd(VC30Fx_START_WORK_CMD);
}
int vc30fx_drv_sleep_work(void *pdevice)
{
    return core30fx_write_work_cmd(VC30Fx_STOP_WORK_CMD);
}
int vc30fx_drv_set_algo_wear_status(void *pdevice, int status)
{
    wear_info_handle->algo_wear_status = (status == -1) ? (WEAR_STA_DROP) : (WEAR_STA_HOLD);
    return 0;
}
int vc30fx_drv_set_acce_wear_status(void *pdevice, int status)
{
    /* 产生运动量，清除心率摘下标志 */
    if (0 == status) {
        wear_info_handle->acce_wear_status = WEAR_STA_DROP;
    } else {
        wear_info_handle->acce_wear_status = WEAR_STA_HOLD;
        wear_info_handle->algo_wear_status = WEAR_STA_HOLD;
    }
    return 0;
}
wear_status vc30fx_drv_get_wear_status(void *pdevice)
{
    vcare_ppg_device_t *pdev = (vcare_ppg_device_t *)pdevice;
    if (!sel_mode_ops) {
        return WEAR_STA_HOLD;
    }
    if (pdev->workmode == WORK_MODE_FACTORY0 || pdev->workmode == WORK_MODE_FACTORY1) {
        return WEAR_STA_HOLD;
    }
    return drv_get_wear_status(pdevice);
}

void vc30fx_drv_get_wear_detail_result(unsigned char *bio_ret, unsigned char *algo_ret, unsigned char *acce_ret)
{
    *bio_ret  = (VC30Fx_BIO_STA_HOLD == wear_info_handle->bio_wear_result) ? 1 : 0;
    *algo_ret = (WEAR_STA_HOLD == wear_info_handle->algo_wear_status) ? 1 : 0;
    *acce_ret = (WEAR_STA_HOLD == wear_info_handle->acce_wear_status) ? 1 : 0;
}

/* irq/timer callback interface. */
int vc30fx_drv_get_result_handler(void *pdevice)
{
    vcare_ppg_device_t *pdev = (vcare_ppg_device_t *)pdevice;
    pdev->result = sample_info_handle;
    vc30fx_sample_info_t *sample_result_ptr = pdev->result;
    if (!sample_result_ptr) {
        return -2;
    }
    if (!sel_mode_ops) {
        return -2;
    }
    /* 读取基础采样数据 */
    drv_read_event_status(drv_info_handle);
    if (drv_info_handle->int_event & (VC30Fx_EVENT_CONFLICT)) {
        VCARE_DBG_ERR("work_status=%x", drv_info_handle->int_event);
        return -1;
    }
    /* check gap time */
    if (0 != drv_check_running_gaptime()) {
        VCARE_DBG_ERR("running gaptime");
        return -1;
    }
    drv_read_sample_param(sample_result_ptr);
    /* 模式数据处理 */
    if (sel_mode_ops->mode_handler) {
        sel_mode_ops->mode_handler(sel_mode_ops, pdev);
    }
    return 0;
}

void vc30fx_drv_read_event_infomation_byIRQ(void)
{
    drv_read_event_status(drv_info_handle);
}

int vc30fx_drv_is_fifo_event(void *pdevice)
{
    if (drv_info_handle->fifo_read_data_valid) {
        return 1;
    }
    return 0;
}
int vc30fx_drv_is_ps_event(void *pdevice)
{
    if (drv_info_handle->int_event & (VC30Fx_EVENT_INTPS_RDY)) {
        return 1;
    }
    return 0;
}
int vc30fx_drv_is_ppg_event(void *pdevice)
{
    if (drv_info_handle->int_event & (VC30Fx_EVENT_INTPPG_STA)) {
        return 1;
    }
    return 0;
}
int vc30fx_drv_clear_reset_bio_param(void *pdevice)
{
    drv_infomation_param_reset();
    core30fx_set_bioinn_addcap(sample_info_handle, set_cap_mid);
    return 0;
}

static unsigned char drv_calc_check_frequency_abnormal(unsigned short dev_tick1, unsigned short dev_tick2, unsigned int cpu_tick1, unsigned int cpu_tick2)
{
    static unsigned short calc_freq[2] = {0};
    unsigned int cpu_tick_diff = 0, dev_tick_diff = 0;
    float gap_time_diff = 0.0f;
    if (!clk_info_handle->clk_calc_status) {
        return 0;
    }
    check_cpu_tick[1] = check_cpu_tick[0];
    check_dev_tick[1] = check_dev_tick[0];
    check_cpu_tick[0] = cpu_tick1;
    check_dev_tick[0] = dev_tick1;
    VCARE_DBG_LOG("cpu_tick:%d,%d", check_cpu_tick[0], check_cpu_tick[1]);
    VCARE_DBG_LOG("dev_tick:%d,%d", check_dev_tick[0], check_dev_tick[1]);
    if (VC_ABS_MAX(dev_tick2, dev_tick1, 0xffff) >= 20 || VC_ABS_MAX(cpu_tick2, cpu_tick1, VC30Fx_CFG_MCU_TICK_MAX) >= 32) {
        check_cpu_tick[0] = 0, check_cpu_tick[1] = 0;
        check_dev_tick[0] = 0, check_dev_tick[1] = 0;
        return 0;
    }
    if (0 == check_cpu_tick[1] || 0 == check_cpu_tick[0]) {
        return 0;
    }
    if (0 == check_dev_tick[1] || 0 == check_dev_tick[0]) {
        return 0;
    }
    cpu_tick_diff = VC_ABS_MAX(check_cpu_tick[0], check_cpu_tick[1], VC30Fx_CFG_MCU_TICK_MAX);
    dev_tick_diff = VC_ABS_MAX(check_dev_tick[0], check_dev_tick[1], 0xFFFF);
    gap_time_diff = (float)cpu_tick_diff / (float)VC30Fx_CFG_MCU_TICK_FREQUENCY;
    calc_freq[1] = calc_freq[0];
    calc_freq[0] = (float)dev_tick_diff / gap_time_diff;
    if (calc_freq[0] && calc_freq[1] && VC_ABS(calc_freq[0], calc_freq[1]) <= 100) {
        VCARE_DBG_LOG("check temp freq=[%d][%d](%d,%d)", calc_freq[0], calc_freq[1], cpu_tick_diff, dev_tick_diff);
        if (20000 >= calc_freq[0] && (20000 - calc_freq[0]) / 40 >= 40) { /* 相较于上电时的频率温升40即返回出错 */
            check_cpu_tick[0] = 0, check_cpu_tick[1] = 0;
            check_dev_tick[0] = 0, check_dev_tick[1] = 0;
            return 1;
        }
    }
    return 0;
}

unsigned char vc30fx_drv_clk_calc_status(void)
{
    return clk_info_handle->clk_calc_status;
}

#define CHECK_BUFF_SIZE 20
int vc30fx_drv_read_check_temperature_abnormal(void *pdevice, unsigned char *p_freq_ret, unsigned char *p_temp_ret, unsigned short *pdev_cnt1, unsigned short *pdev_cnt2)
{
    static unsigned char  fiil_mark = 0;
    static unsigned char  fill_index = 0;
    static unsigned short check_innbio_buff[CHECK_BUFF_SIZE] = {0};
    static unsigned short check_extbio_buff[CHECK_BUFF_SIZE] = {0};
    static unsigned short check_tick1_buff[CHECK_BUFF_SIZE] = {0};
    static unsigned short check_tick2_buff[CHECK_BUFF_SIZE] = {0};
    unsigned int check_clk_tick[2] = {0};
    unsigned char i = 0;
    int check_ret = 0;
    *p_freq_ret = 0;
    *p_temp_ret = 0;

    drv_read_sample_param(sample_info_handle);
    drv_info_handle->param_read_finished_sync = 0; /* 读取完成之后不置同步标志，该数据仅做异常检测 */
    /****** 进入临界区 ******/
    check_clk_tick[0] = vc30fx_get_cputimer_tick();
    core30fx_read_rcosc_tick(sample_info_handle, pdev_cnt1, pdev_cnt2);
    check_clk_tick[1] = vc30fx_get_cputimer_tick();
    /****** 退出临界区 ******/
    check_innbio_buff[fill_index] = sample_info_handle->extra_result.bioinn_data;
    check_extbio_buff[fill_index] = sample_info_handle->extra_result.bioext_data;
    check_tick1_buff[fill_index] = *pdev_cnt1;
    check_tick2_buff[fill_index] = *pdev_cnt2;
    /* 检验频率温度是否正常 */
    *p_freq_ret = drv_calc_check_frequency_abnormal(*pdev_cnt1, *pdev_cnt2, check_clk_tick[0], check_clk_tick[1]);
    /* 检验内部温度是否正常 */
#if 1 /* 使用上电时的内部温度和当前的相对温度进行比较 或者是 内部一致就使用绝对比较 */
    VCARE_DBG_LOG("startup_inntemperature=%d", startup_inntemperature);
    if (sample_info_handle->extra_result.tempinn_data > 0 && startup_inntemperature > sample_info_handle->extra_result.tempinn_data && (startup_inntemperature - sample_info_handle->extra_result.tempinn_data) >= 1000)
#else
    if (sample_info_handle->extra_result.tempinn_data > 0 && sample_info_handle->extra_result.tempinn_data <= 18000) /* 如果客户那边内部得温度常温都在19000左右，就可以直接判断绝对值 */
#endif
    {
        *p_temp_ret = 1;
    }
    /* 校验bio/rctick是否卡死且不为0 */
    if (++fill_index >= CHECK_BUFF_SIZE) {
        fill_index = 0;
        fiil_mark = 1;
    }
//	VCARE_DBG_LOG("innbio:%d,%d,%d,%d,%d  %d,%d,%d,%d,%d",check_innbio_buff[0],check_innbio_buff[1],check_innbio_buff[2],check_innbio_buff[3],check_innbio_buff[4],
//	check_innbio_buff[5],check_innbio_buff[6],check_innbio_buff[7],check_innbio_buff[8],check_innbio_buff[9] );
//	VCARE_DBG_LOG("extbio:%d,%d,%d,%d,%d  %d,%d,%d,%d,%d",check_extbio_buff[0],check_extbio_buff[1],check_extbio_buff[2],check_extbio_buff[3],check_extbio_buff[4],
//	check_extbio_buff[5],check_extbio_buff[6],check_extbio_buff[7],check_extbio_buff[8],check_extbio_buff[9] );
//	VCARE_DBG_LOG("tick1:%d,%d,%d,%d,%d  %d,%d,%d,%d,%d",check_tick1_buff[0],check_tick1_buff[1],check_tick1_buff[2],check_tick1_buff[3],check_tick1_buff[4],
//	check_tick1_buff[5],check_tick1_buff[6],check_tick1_buff[7],check_tick1_buff[8],check_tick1_buff[9] );
//	VCARE_DBG_LOG("tick2:%d,%d,%d,%d,%d  %d,%d,%d,%d,%d",check_tick2_buff[0],check_tick2_buff[1],check_tick2_buff[2],check_tick2_buff[3],check_tick2_buff[4],
//	check_tick2_buff[5],check_tick2_buff[6],check_tick2_buff[7],check_tick2_buff[8],check_tick2_buff[9] );
    if (fiil_mark) {
        for (i = 0; i < CHECK_BUFF_SIZE - 1; i++) {
            if (0 == sample_info_handle->extra_result.bioinn_enwork || 0 == sample_info_handle->extra_result.bioext_enwork) {
                check_ret++;
                break; /* bio未使能，忽略++ */
            }
            if (check_innbio_buff[i] != check_innbio_buff[i + 1] || check_extbio_buff[i] != check_extbio_buff[i + 1]) {
                check_ret++;
                break; /* bio未卡死，通过++ */
            }
        }
        for (i = 0; i < CHECK_BUFF_SIZE - 1; i++) {
            if (check_tick1_buff[i] != check_tick1_buff[i + 1] || check_tick2_buff[i] != check_tick2_buff[i + 1]) {
                check_ret++;
                break; /* cnt未卡死，通过++ */
            }
        }
        check_ret = (check_ret < 2) ? 1 : 0; /* 至少一项已经卡死了,结果置1 */
    }
    return check_ret;
}

int vc30fx_drv_finished_sync(void *pdevice)
{
    drv_sample_result_memroy_finished(sample_info_handle);
    drv_info_handle->param_read_finished_sync = 0;
    drv_info_handle->fifo_read_finished_sync = 0;
    drv_info_handle->fifo_read_data_valid = 0;
    drv_info_handle->event_read_finished_sync = 0;

#if 1
    unsigned char config_debug_reg[24] = {0};
    unsigned char debug_index = 0;
    vc30fx_read_register(0x40, config_debug_reg, 24);
    VCARE_DBG_LOG("[0x40]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
    VCARE_DBG_LOG("[0x48]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
    debug_index += 8;
    VCARE_DBG_LOG("[0x50]0x%02x,0x%02x,0x%02x,0x%02x 0x%02x,0x%02x,0x%02x,0x%02x", config_debug_reg[0 + debug_index], config_debug_reg[1 + debug_index], config_debug_reg[2 + debug_index], config_debug_reg[3 + debug_index],
                  config_debug_reg[4 + debug_index], config_debug_reg[5 + debug_index], config_debug_reg[6 + debug_index], config_debug_reg[7 + debug_index]);
#endif
    return 0;
}

#endif//TCFG_VC30fx_ENABLE
