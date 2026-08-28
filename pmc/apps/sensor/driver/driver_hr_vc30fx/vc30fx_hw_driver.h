
/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2022-09-30 09:50:56
 * @LastEditTime: 2023-12-15 20:47:37
 * @LastEditors: Tse
 * @Description: hardware driver layer
 **********************************************************************************************/

#ifndef __VC30Fx_DRIVER_H__
#define __VC30Fx_DRIVER_H__

#include "vcare_device_com.h"
/****************************************************************************
 * WearStatus_t
 ***************************************************************************/
#define VC30Fx_BIO_STA_NONE 0
#define VC30Fx_BIO_STA_DROP 1
#define VC30Fx_BIO_STA_HOLD 2

typedef enum {
    VC30Fx_WEAR_NO_CHANGE = 0,
    VC30Fx_HOLD_TO_DROP,
    VC30Fx_DROP_TO_HOLD,
} vc30fx_wear_change;

typedef struct {
    unsigned char hold_cnt;
    unsigned char drop_cnt;
    wear_status wear_status;
    wear_status last_status;
    wear_status algo_wear_status;
    wear_status acce_wear_status;
    int bio_wear_result;
    unsigned short int last_ps_data[2];

} vc30fx_wear_info;

/****************************************************************************
 * oscclk_t
 ***************************************************************************/
typedef struct {
    /* calc real frequency */
    unsigned int osc_count[6];
    unsigned int clk_count[6];
    unsigned char count_index;
    unsigned char osc_gap_cnt;
    unsigned short int rcosc_frequency;
    /* check interrupt gaptime */
    unsigned short int last_check_ticks;
    /* startup need get calc value */
    unsigned char rcosc_calibration;
    unsigned char clk_calc_freq_status; /* calculate frequency finished */
    unsigned char clk_calc_status;      /* calculate rcsoc status finished */

} vc30fx_clk_info;

/****************************************************************************
 * adjust_t
 ***************************************************************************/
#define VC30Fx_ADJUST_DIR_NONE 0
#define VC30Fx_ADJUST_DIR_UP 1
#define VC30Fx_ADJUST_DIR_DOWN 2

typedef struct {
    unsigned char direction;
    unsigned char direction_last;
    unsigned short int current_step;
} vc30fx_adjust_param;

typedef struct {
    unsigned char const_max_current; /* ic参数确定的最大电流电阻，固定 */
    unsigned char const_min_current;
    unsigned char const_max_res_level;
    unsigned char const_min_res_level;
    unsigned char max_current; /* ic工作环境的最大电流电阻，可变 */
    unsigned char max_res_level;
    unsigned char current; /* ic当前工作的最大电流电阻，可变 */
    unsigned char res_level;

    vc30fx_adjust_param adj_param;

} vc30fx_adjust_info;
typedef struct {
    int work_mode;
    unsigned short ppg_div;
    unsigned char ps_div;
    unsigned char fifo_div;

    unsigned char int_event;
    unsigned char ov_status;
    unsigned char read_fifo_address;
    unsigned char write_fifo_address_bak; /* write_addr when fifo event */
    unsigned char write_fifo_address_rel; /* write_addr at present */

    int optimal_bio_gap;
    unsigned char optimal_cap;
    unsigned char optimal_bio_valid;
    unsigned char ppg_max_thval;
    unsigned char ppg_min_thval;

    unsigned char param_read_finished_sync;
    unsigned char fifo_read_finished_sync;
    unsigned char fifo_read_data_valid; /* 0-havn't fifo data, 1-normal fifo data, 2-read fifo data when fifo_windex offset */
    unsigned char event_read_finished_sync;
} vc30fx_drv_info;

typedef struct {
    work_mode mode;
    int (*mode_init)(void);
    int (*mode_handler)(const void *ops_self, vcare_ppg_device_t *pdev);
} vc30fx_mode_ops;

#endif /* __VC9211x_DRIVER_H__ */
