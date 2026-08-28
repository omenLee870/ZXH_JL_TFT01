/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-02-23 17:47:18
 * @LastEditTime: 2023-11-14 17:38:46
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#ifndef __VCARE_DEVICE_COM_H__
#define __VCARE_DEVICE_COM_H__

/* parameter type sleep and retain */
#ifndef PARAM_RETAIN
#define PARAM_RETAIN
#endif
/* define NULL 0 */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* gsensor struct,user peripheral data */
typedef struct {
    short int xData[40];
    short int yData[40];
    short int zData[40];
} gsensor_axes;

typedef enum {
    WEAR_STA_NONE = 0,
    WEAR_STA_DROP,
    WEAR_STA_HOLD,
} wear_status;

typedef enum {
    WORK_MODE_HR = 0,
    WORK_MODE_SPO2,
    WORK_MODE_HRV,
    WORK_MODE_STRESS,
    WORK_MODE_BLOODPRESSURE,
    WORK_MODE_WEAR,
    WORK_MODE_TEMPERATURE,
    WORK_MODE_FACTORY0,
    WORK_MODE_FACTORY1,
    WORK_MODE_HRSPO2,

    WORK_MODE_UNDEFINED,
    WORK_MODE_NUMBER,
} work_mode;

typedef struct {
    unsigned short sampling_gap_time; /* reserved. */
    unsigned char  work_mode; /* work_mode,WorkModeTypeDef/unsigned char */
} InitParamTypeDef;


typedef void (*FUNC_DEV_INIT)(InitParamTypeDef *pInitconfig);
typedef void (*FUNC_GET_DEV_INFO)(unsigned char *pchip_id, unsigned char *pvers_id, char **drv_name, char **dev_name);
typedef void (*FUNC_START_WORK)(void);
typedef void (*FUNC_STOP_WORK)(void);
typedef void (*FUNC_SOFT_RESET)(void);
typedef int (*FUNC_GET_EVENTS)(void);
typedef int (*FUNC_GET_WEAR_STATUS)(void);
typedef void (*FUNC_SET_SLOT_ENABLE)(unsigned char slot_index, unsigned char slot_enable);
typedef void (*FUNC_BIO_RESET_PARAM)(void);
typedef void (*FUNC_DEV_IRQ_HANDLER)(unsigned char heart_algo_mode, unsigned char spo2_algo_mode);
typedef void (*FUNC_DEV_CHK_TEMP_ERR)(void);

typedef struct {
    FUNC_DEV_INIT dev_init;
    FUNC_GET_DEV_INFO get_dev_info; /* reserved */
    FUNC_START_WORK start_work;
    FUNC_STOP_WORK stop_work;
    FUNC_SOFT_RESET soft_reset;
    FUNC_GET_EVENTS get_events; /* reserved */
    FUNC_GET_WEAR_STATUS get_wear_status;
    FUNC_SET_SLOT_ENABLE set_slot_enable; /* reserved */
    FUNC_BIO_RESET_PARAM reset_bio_param;
    FUNC_DEV_IRQ_HANDLER dev_irq_handler;
    FUNC_DEV_CHK_TEMP_ERR dev_check_temp_error;
} dev_ops;

typedef struct {
    char *dev_name;
    unsigned char id1;
    unsigned char id2;
    unsigned char dev_work_status; /* 0-don't work, 1-working */

    unsigned char heart_algo_mode;
    unsigned char spo2_algo_mode;
    work_mode workmode;
    wear_status wear;
    /* raw result */
    void *result;
    /* device ops */
    const dev_ops *ops;
    /* calc result */
    int heart_rate;
    int spo2;
    int hrv;
    int stress;
    int ecg;
    int temperature0;
    int temperature1;
    int blood_pressure;
} vcare_ppg_device_t;

/**-------------------- defined functions --------------------**/
#define ENABLE_MY_MEMFUNC 1
#if ENABLE_MY_MEMFUNC
static inline void *vcare_memset(void *dest, int set, unsigned int n)
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

static inline void *vcare_memcpy(void *dest, void *src, unsigned int n)
{
    if (dest == NULL || src == NULL) {
        return NULL;
    }
    char *pdest = (char *)dest;
    char *psrc = (char *)src;
    while (n--) {
        *pdest++ = *psrc++;
    }
    return dest;
}

/* 2^(n/16) */
static const float exponent_tab[] = {1.000000, 1.044274, 1.090508, 1.138789, 1.189207, 1.241858, 1.296840, 1.354256, 1.414214,
                                     1.476826, 1.542211, 1.610490, 1.681793, 1.756252, 1.834008, 1.915207, 2.000000
                                    };
static inline float vcare_I_exponent_func(float base_i, unsigned char data_i)
{
    unsigned char data_h = (data_i >> 4) & 0xf;
    unsigned char data_l = (data_i >> 0) & 0xf;
    return (base_i * (1 << data_h) * exponent_tab[data_l]);
}
static inline float vcare_R_exponent_func(float base_r, unsigned char data_r)
{
    unsigned char data_h = data_r / 2;
    unsigned char data_l = data_r % 2;
    float data_l_value = (data_l) ? (exponent_tab[8]) : (1.0f);
    return (base_r * (1 << data_h) * data_l_value);
}


#endif

/**-------------------- debug functions --------------------**/
extern void log_direct(unsigned int info, const char *fmt, ...);
#define VCARE_DBG_LEVEL 0//2

#if (VCARE_DBG_LEVEL == 0)
#define VCARE_DBG_LOG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#define VCARE_DBG_ERR(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#elif (VCARE_DBG_LEVEL == 1)
#define VCARE_DBG_LOG(fmt, ...)
#define VCARE_DBG_ERR(fmt, ...)                                                             \
	do                                                                                      \
	{                                                                                       \
		printf("[VCARE_ERR](%d)" fmt, __LINE__, ##__VA_ARGS__); \
	} while (0)
#elif (VCARE_DBG_LEVEL == 2)
#define VCARE_DBG_LOG(fmt, ...)                                                             \
	do                                                                                      \
	{                                                                                       \
		printf("[VCARE_LOG](%d)" fmt, __LINE__, ##__VA_ARGS__); \
	} while (0)
#define VCARE_DBG_ERR(fmt, ...)                                                             \
	do                                                                                      \
	{                                                                                       \
		printf("[VCARE_ERR](%d)" fmt, __LINE__, ##__VA_ARGS__); \
	} while (0)
#endif

/*
extern int p_debuglog_user(const char *fmt, ...);
#define VCARE_DBG_LOG(fmt, ...) do{ if(p_debuglog_user)p_debuglog_user("[VCARE_LOG]%s(%d)"fmt, __func__, __LINE__, ##__VA_ARGS__); }while(0)
#define VCARE_DBG_ERR(fmt, ...) do{ if(p_debuglog_user)p_debuglog_user("[VCARE_ERR]%s(%d)"fmt, __func__, __LINE__, ##__VA_ARGS__); }while(0)
*/

#endif
