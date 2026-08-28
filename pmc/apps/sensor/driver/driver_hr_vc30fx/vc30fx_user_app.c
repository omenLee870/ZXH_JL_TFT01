/**********************************************************************************************
 * @Copyright(c) 2023, Chengdu vcaresensor Microelectronics Corporation. All rights reserved.
 * @Author: Tse
 * @Date: 2023-06-09 11:57:36
 * @LastEditTime: 2024-01-19 15:07:52
 * @LastEditors: Tse
 * @Description:
 **********************************************************************************************/
#include "includes.h"
#include "sdk_config.h"
#include "uart.h"
#include "debug.h"
#include "sensor_driver.h"
/* 驱动头文件 */
#include "vc30fx_driver_com.h"
#include "vc30fx_sample_result.h"
#include "vc30fx_user_app.h"
/* 算法头文件 */
#include "algo.h"
#include "spo2Algo_16bit.h"
#include "vcSportMotionIntAlgo.h"

#include "health_manager.h"
#if TCFG_VC30fx_ENABLE
#define  LOG(fmt,...)            printf("[vc30fx] " fmt "\n",##__VA_ARGS__)
#define IIC_DEV_ADDR			0x33
#define IIC_DEV_ADDR_R			(IIC_DEV_ADDR<<1|1)
#define IIC_DEV_ADDR_W			(IIC_DEV_ADDR<<1|0)
static u8 heart_algo_mode = 0;
static u8 spo2_algo_mode = 1;
typedef struct {
    u8 hr_value;
    u8 spo2_value;
    u8 wear;
} hrsensor_algo_info;
static hrsensor_algo_info    algo_info;

/* ble_debug,蓝牙发送原始数据 */
/* vcare_ppg_device */
static vcare_ppg_device_t vc30fx_dev = {"vc30fx_sc", 0};
/* G-Sensor Data */
static gsensor_axes vc30fx_gsensor_data = {0};

static void vc30fx_get_board_gsensor_data(unsigned short int *pgsensor_len, unsigned short int ppg_count);
static unsigned int vc30fx_gsensor_actuating_quantity(int x_axis, int y_axis, int z_axis);
static int vc30fx_heart_rate_calculate(vcare_ppg_device_t *dev);
static int vc30fx_spo2_calculate(vcare_ppg_device_t *dev);

/*========~~~~~~~~~~~~~~~~~~~~~~AK-自定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if 1 //MY_VC30FX_MODIFY
#define VC30FX_SCAN_PERIOD     1000//1000//400   /* ms */
void vc30fx_io_interrupt_handler();
#if !PMC_DEVELOP_ENABLE
//只读配置，用户后续新增io唤醒、ad唤醒注意使用const修饰，节约静态ram
static const struct _p33_io_wakeup_config hr_sensor_irq = {
    .pullup_down_mode  = PORT_INPUT_PULLUP_10K,
    .filter      		= PORT_FLT_DISABLE,
    .edge               = FALLING_EDGE,
    .gpio               = VC30FX_INT_PIN,
    .callback           = vc30fx_io_interrupt_handler,
};

void hr_sensor_irq_init(void)
{
    if (VC30FX_INT_PIN != (-1)) {
        p33_io_wakeup_port_init(&hr_sensor_irq);
        p33_io_wakeup_enable(VC30FX_INT_PIN, 1);
    }
}
#endif
//***********************************************************//
//				获取传感器数据
//***********************************************************//

// static int pt_gsensor_data_get(void *data_buf, int data_len)
// {
//     return sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, data_buf, data_len);
// }
// _WEAK_ int gsensor_data_get(void *data_buf, int data_len)   //获取传感器数据
// {
//     return 0;   //0 获取不到数据   other	数据长度
// }

unsigned char get_hrs_dev_work_status(void)
{
    return vc30fx_dev.dev_work_status;
}

u8 get_hrs_dev_hr_valus(void)
{
    return algo_info.hr_value;
}

u8 get_hrs_dev_spo2_valus(void)
{
    return algo_info.spo2_value;
}
u8 get_hrs_dev_wear_valus(void)
{
    return algo_info.wear;
}
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~AK-自定义-结束~~~~~~~~~~~~~~~~~~========*/

/****************************************************************************
 * @description: 驱动参数保存/读取
 * @param {unsigned int} size
 * @return {*}
 ****************************************************************************/
int vc30fx_param_write_flash(unsigned char *pdata, unsigned int size)
{
    int ret = 0;
#if (!PMC_DEVELOP_ENABLE)
    syscfg_write(VM_SENSOR_PPG_DATA, pdata, size);
#endif
    return ret == size ? 0 : -1;
}
int vc30fx_param_read_flash(unsigned char *pdata, unsigned int size)
{
    int ret = 0;//
#if (!PMC_DEVELOP_ENABLE)
    syscfg_read(VM_SENSOR_PPG_DATA, pdata, size);
#endif
    return ret == size ? 0 : -1;
}

/****************************************************************************
 * @description: 内存申请
 * @param {unsigned int} size
 * @return {*}
 ****************************************************************************/
void *vc30fx_malloc(unsigned int size)
{
#if (!PMC_DEVELOP_ENABLE)
    return malloc(size);
#else
    return NULL;
#endif
}
/****************************************************************************
 * @description: 内存释放
 * @param {void} *ptr
 * @return {*}
 ****************************************************************************/
void vc30fx_free(void *ptr)
{
    if (ptr) {
#if (!PMC_DEVELOP_ENABLE)
        free(ptr);
#endif
    }
}
/****************************************************************************
 * @description:
 * @param {unsigned char} regaddr
 * @param {unsigned char*} pbuf
 * @param {unsigned short int} size
 * @return {*}
 ****************************************************************************/
int vc30fx_read_register(unsigned char regaddr, unsigned char *pbuf, unsigned short int size)
{
    /* iic/spi read */
    /*****************************************************************************
     * When I2C is used, write Slave Address and write flag to vcHrxx firstly,
     * then write len byte data to register(s) start from startAddress
     * to register(s) pointed by pRegisters.
     * return:	0(read success),-1(read fail)
     ******************************************************************************/

    return sensor_read(IIC_DEV_ADDR_R, regaddr,  pbuf, size, 0);
}

int vc30fx_write_register(unsigned char regaddr, unsigned char *pbuf, unsigned short int size)
{
    /* iic/spi write */
    /*****************************************************************************
     * When I2C is used, write Slave Address and read flag to vcHrxx firstly,
     * then read len byte data from register(s) start from startAddress
     * to register(s) pointed by pRegisters.
     * return:	0(write success),-1(write fail)
     ******************************************************************************/

    return sensor_write(IIC_DEV_ADDR_W, regaddr,  pbuf, size);
}
#include "vc30fx_driver_config.h"
unsigned int vc30fx_get_cputimer_tick(void)
{
    /*========~~~~~~~~~~~~~~~~~~~~~~AK-自定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#if!PMC_DEVELOP_ENABLE
    int cnt = lptmr1_get_pass_us();
#else
    int cnt  = 0;
#endif
    int pass_32bitcnt = cnt / (1000000 / VC30Fx_CFG_MCU_TICK_FREQUENCY);
    //pass_32bitcnt = pass_32bitcnt & (0xffffffff);
    pass_32bitcnt = pass_32bitcnt & (VC30Fx_CFG_MCU_TICK_MAX);
    printf("pass_32bitcnt:%d\n",  pass_32bitcnt);
    return pass_32bitcnt;
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~AK-自定义-结束~~~~~~~~~~~~~~~~~~========*/
}

/*********************************************************************************************************/
/* user gsensor code */
#define ENABLE_GSENSOR 1
typedef struct {
    short AXIS_X;
    short AXIS_Y;
    short AXIS_Z;
} AxesRaw_t;

const unsigned char arry10[] = {0, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12};
const unsigned char arry20[] = {0, 2, 3, 5, 6, 8, 9, 11, 12, 13, 15, 16, 18, 19, 21, 22, 24, 25, 27, 28};
u32 gsensor_cbuf_read(short frame, short *buf);
static void vc30fx_get_board_gsensor_data(unsigned short int *pgsensor_len, unsigned short int ppg_count)
{
    unsigned char gsensor_len = 0;
    // vcare_memset(&vc30fx_gsensor_data, 0, sizeof(vc30fx_gsensor_data));
#if ENABLE_GSENSOR
    int i = 0;
    AxesRaw_t gsensor_buff[40] = {0};

    /* Gsensor精度要求 ReadGsensorFIFO(+-4G11bit or +-8G12bit 256/g) 一个fifo周期为1sec,10组ppg数据，需要获取1sec内10组gsensor数据(100ms*10)，用于匹配10组ppg信号*/
    // gsensor_len = gsensor_data_get(gsensor_buff, sizeof(gsensor_buff));
    /* gsensor_len = gsensor_cbuf_read(ppg_count, (short *)gsensor_buff); */
    u16 point = sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, gsensor_buff,  sizeof(gsensor_buff));
    for (u8 i = 0; i < gsensor_len; i++) {
#if 1  // 将数据移位和调整相应的 XYZ顺序
        gsensor_buff[i].AXIS_X = gsensor_buff[i].AXIS_X / 4;
        gsensor_buff[i].AXIS_Y = gsensor_buff[i].AXIS_Y / 4;
        gsensor_buff[i].AXIS_Z = gsensor_buff[i].AXIS_Z / 4;

        short temp_x = gsensor_buff[i].AXIS_X;
        short temp_y = gsensor_buff[i].AXIS_Y;
        gsensor_buff[i].AXIS_X = temp_y;
        gsensor_buff[i].AXIS_Y = temp_x;
#endif
        /*========~~~~~~~~~~~~~~~~~~~~~~AK-自定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if MY_SENSOR_DEBUG
        // printf("-0- gsensor_len:%d-> x:%d,y:%d,z:%d\n",gsensor_len,gsensor_buff[i].AXIS_X,gsensor_buff[i].AXIS_Y,gsensor_buff[i].AXIS_Z);
#endif
        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~AK-自定义-结束~~~~~~~~~~~~~~~~~~========*/
    }


    VCARE_DBG_LOG("gsensor_len[%d]-ppgsize[%d]", gsensor_len, ppg_count);
    if (0 == ppg_count) {
        /* 不抽样 */
        *pgsensor_len = gsensor_len;
        for (i = 0; i < *pgsensor_len; i++) {
#if (BOARD_VERSION == BOARD_NEW_BOARD)
            vc30fx_gsensor_data.xData[i] = gsensor_buff[i].AXIS_X;
            vc30fx_gsensor_data.yData[i] = gsensor_buff[i].AXIS_Y;
            vc30fx_gsensor_data.zData[i] = gsensor_buff[i].AXIS_Z;
#elif (BOARD_VERSION == BOARD_OLD_BOARD)
            vc30fx_gsensor_data.xData[i] = gsensor_buff[i].AXIS_Y;
            vc30fx_gsensor_data.yData[i] = gsensor_buff[i].AXIS_X;
            vc30fx_gsensor_data.zData[i] = gsensor_buff[i].AXIS_Z;
#endif
            /*========~~~~~~~~~~~~~~~~~~~~~~AK-自定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if MY_SENSOR_DEBUG
            printf("-0- pgsensor_len:%d-> x:%d,y:%d,z:%d\n", *pgsensor_len, vc30fx_gsensor_data.xData[i], vc30fx_gsensor_data.yData[i], vc30fx_gsensor_data.zData[i]);
#endif
            /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~AK-自定义-结束~~~~~~~~~~~~~~~~~~========*/
        }
    }
    /* 此demo中gsensor的采样频率高于ppg采样，所以正常情况下，gsensor的数量一定是大于ppg的数据。特殊情况需要考虑ppg数量大于gsensor数量。需要对gsensor数据进行补长 */
    if ((gsensor_len >= ppg_count) && (ppg_count != 0)) {
        *pgsensor_len = ppg_count;
        /* ppglen = 10 抽样 */
#if VC30FX_SCAN_PERIOD == 400
        if (10 == ppg_count)  // 400ms 25hz
#else
        if (25 == ppg_count) // 1s扫描 25hz
#endif
        {
            for (i = 0; i < ppg_count; i++) {
#if (BOARD_VERSION == BOARD_NEW_BOARD)
#if VC30FX_SCAN_PERIOD == 400
                vc30fx_gsensor_data.xData[i] = gsensor_buff[arry10[i]].AXIS_X;
                vc30fx_gsensor_data.yData[i] = gsensor_buff[arry10[i]].AXIS_Y;
                vc30fx_gsensor_data.zData[i] = gsensor_buff[arry10[i]].AXIS_Z;
#else
                vc30fx_gsensor_data.xData[i] = gsensor_buff[i].AXIS_X;
                vc30fx_gsensor_data.yData[i] = gsensor_buff[i].AXIS_Y;
                vc30fx_gsensor_data.zData[i] = gsensor_buff[i].AXIS_Z;
#endif
#elif (BOARD_VERSION == BOARD_OLD_BOARD)
                vc30fx_gsensor_data.xData[i] = gsensor_buff[arry10[i]].AXIS_Y;
                vc30fx_gsensor_data.yData[i] = gsensor_buff[arry10[i]].AXIS_X;
                vc30fx_gsensor_data.zData[i] = gsensor_buff[arry10[i]].AXIS_Z;
#endif
                /*========~~~~~~~~~~~~~~~~~~~~~~AK-自定义~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
#if MY_SENSOR_DEBUG
                printf("-0- ppg_count:%d-> x:%d,y:%d,z:%d\n", ppg_count, vc30fx_gsensor_data.xData[i], vc30fx_gsensor_data.yData[i], vc30fx_gsensor_data.zData[i]);
#endif
                /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~AK-自定义-结束~~~~~~~~~~~~~~~~~~========*/
            }
        } else { /* ppglen = 20 、25抽样 */
            for (i = 0; i < ppg_count; i++) {
#if (BOARD_VERSION == BOARD_NEW_BOARD)
                vc30fx_gsensor_data.xData[i] = gsensor_buff[i].AXIS_X;
                vc30fx_gsensor_data.yData[i] = gsensor_buff[i].AXIS_Y;
                vc30fx_gsensor_data.zData[i] = gsensor_buff[i].AXIS_Z;

#elif (BOARD_VERSION == BOARD_OLD_BOARD)
                vc30fx_gsensor_data.xData[i] = gsensor_buff[arry20[i]].AXIS_Y;
                vc30fx_gsensor_data.yData[i] = gsensor_buff[arry20[i]].AXIS_X;
                vc30fx_gsensor_data.zData[i] = gsensor_buff[arry20[i]].AXIS_Z;
#endif
            }
        }
    } else if ((gsensor_len < ppg_count) && (gsensor_len != 0)) { /* gsensor不足补全 */
        *pgsensor_len = ppg_count;
        for (i = 0; i < gsensor_len; i++) {
#if (BOARD_VERSION == BOARD_NEW_BOARD)
            vc30fx_gsensor_data.xData[i] = gsensor_buff[i].AXIS_X;
            vc30fx_gsensor_data.yData[i] = gsensor_buff[i].AXIS_Y;
            vc30fx_gsensor_data.zData[i] = gsensor_buff[i].AXIS_Z;
#elif (BOARD_VERSION == BOARD_OLD_BOARD)
            vc30fx_gsensor_data.xData[i] = gsensor_buff[i].AXIS_Y;
            vc30fx_gsensor_data.yData[i] = gsensor_buff[i].AXIS_X;
            vc30fx_gsensor_data.zData[i] = gsensor_buff[i].AXIS_Z;
#endif
        }
        for (i = gsensor_len; i < ppg_count; i++) {
#if (BOARD_VERSION == BOARD_NEW_BOARD)
            vc30fx_gsensor_data.xData[i] = gsensor_buff[gsensor_len - 1].AXIS_X;
            vc30fx_gsensor_data.yData[i] = gsensor_buff[gsensor_len - 1].AXIS_Y;
            vc30fx_gsensor_data.zData[i] = gsensor_buff[gsensor_len - 1].AXIS_Z;
#elif (BOARD_VERSION == BOARD_OLD_BOARD)
            vc30fx_gsensor_data.xData[i] = gsensor_buff[gsensor_len - 1].AXIS_Y;
            vc30fx_gsensor_data.yData[i] = gsensor_buff[gsensor_len - 1].AXIS_X;
            vc30fx_gsensor_data.zData[i] = gsensor_buff[gsensor_len - 1].AXIS_Z;
#endif
        }
    }

#endif
}

/****************************************************************************
 * @description: gsensor判断是否存在运动
 * @param {int} x_axis
 * @param {int} y_axis
 * @param {int} z_axis
 * @return {*}
 ****************************************************************************/
static unsigned int vc30fx_gsensor_actuating_quantity(int x_axis, int y_axis, int z_axis)
{
    unsigned int ret_atc = 0;
    unsigned int abs_sum_current = 0;
    static unsigned int abs_sum_last = 0;
    if (x_axis < 0) {
        x_axis = -x_axis;
    }
    if (y_axis < 0) {
        y_axis = -y_axis;
    }
    if (z_axis < 0) {
        z_axis = -z_axis;
    }
    abs_sum_current = x_axis + y_axis + z_axis;
    if (abs_sum_last && abs_sum_current) {
        if (((abs_sum_last > abs_sum_current) ? (abs_sum_last - abs_sum_current) : (abs_sum_current - abs_sum_last)) >= 40) {
            VCARE_DBG_LOG("ACTIVE...");
            ret_atc = 1;
        }
    }
    abs_sum_last = abs_sum_current;
    vc30fx_drv_set_acce_wear_status(&vc30fx_dev, ret_atc);
    return ret_atc;
}

/****************************************************************************
 * @description: ppg数据加入心率算法
 * @param {vcare_ppg_device_t} *pdev
 * @return {*}
 ****************************************************************************/
static int vc30fx_heart_rate_calculate(vcare_ppg_device_t *pdev)
{
    int heartRate = 0;
    unsigned char reliability = 0;
    AlgoInputData_t algoInputData_t = {0};
    AlgoOutputData_t algoOutputData = {0};
    vc30fx_sample_info_t *result_info = NULL;
    vc30fx_sample_slot_t *result = NULL;
    int i = 0;
    if (!pdev) {
        return -1;
    }
    heartRate = pdev->heart_rate;
    result_info = (vc30fx_sample_info_t *)pdev->result;
    result = &result_info->slot_result[0]; /* 驱动中初始化slot0为心率绿光ppg */
    for (i = 0; i < result->ppg_num; i++) {
        algoInputData_t.axes.x = vc30fx_gsensor_data.xData[i];
        algoInputData_t.axes.y = vc30fx_gsensor_data.yData[i];
        algoInputData_t.axes.z = vc30fx_gsensor_data.zData[i];
        algoInputData_t.envSample = result->env_data;
        algoInputData_t.ppgSample = result->ppg_buffer[i];
        Algo_Input(&algoInputData_t, 1000 / 25, (AlgoSportMode)pdev->heart_algo_mode, 1, 1);
        VCARE_DBG_LOG("[%d] ppg=%d, x=%d, y=%d, z=%d", i, algoInputData_t.ppgSample,
                      algoInputData_t.axes.x, algoInputData_t.axes.y, algoInputData_t.axes.z);
    }
    Algo_Output(&algoOutputData);
    heartRate = algoOutputData.hrData;
    reliability = algoOutputData.reliability;
    if (-1 == heartRate) {
        Algo_Init();
        /* call heart algo wear status check,[-1] */
        vc30fx_drv_set_algo_wear_status(pdev, heartRate);
        VCARE_DBG_LOG("ALGO_STA=-1");
    }
    VCARE_DBG_LOG("ALGO-HR[%d][%d]", heartRate, reliability);
    return heartRate;
}

/****************************************************************************
 * @description: 血氧PPG数据加入血氧算法
 * @param {vcare_ppg_device_t} *pdev
 * @return {*}
 ****************************************************************************/
static int vc30fx_spo2_calculate(vcare_ppg_device_t *pdev)
{
    vc30fx_sample_info_t *result_info = NULL;
    vc30fx_sample_slot_t *result_ir = NULL, *result_red = NULL;
    int spo2CalValue = -1;
    int spo2Value = 0;
    unsigned char sportFlag = 0;
    if (!pdev) {
        return -1;
    }
    spo2Value = pdev->spo2;
    result_info = (vc30fx_sample_info_t *)pdev->result;
    result_ir = &result_info->slot_result[0];  /* 驱动中初始化slot0为红外ppg */
    result_red = &result_info->slot_result[1]; /* 驱动中初始化slot1为红光ppg */
    for (int i = 0; i < result_ir->ppg_num; i++) {
        spo2CalValue = spo2Algo_16bit(result_red->ppg_buffer[i], result_ir->ppg_buffer[i], 1);
        sportFlag = vcSportMotionCalculate(vc30fx_gsensor_data.xData[i], vc30fx_gsensor_data.yData[i], vc30fx_gsensor_data.zData[i]);
        if (-1 != (int)spo2CalValue && spo2CalValue <= 100 && 0 == sportFlag) {
            spo2Value = (int)spo2CalValue;
        }
        VCARE_DBG_LOG("[%d] ppg_ir=%d, ppg_red=%d, x=%d, y=%d, z=%d", i, result_ir->ppg_buffer[i], result_red->ppg_buffer[i],
                      vc30fx_gsensor_data.xData[i], vc30fx_gsensor_data.yData[i], vc30fx_gsensor_data.zData[i]);
    }
    VCARE_DBG_LOG("ALGO-SPO2[%d][%d]", (unsigned short int)spo2Value, spo2CalValue);
    return spo2Value;
}

/********************************************************************************************************
 * 温度功能部分，需要进行对数运算，必须包含math库
 ********************************************************************************************************/
#include "math.h"
/* fix Rref = 10K R */
#define REF_R 10
/* NTC parameter */
#define NTC_B 3380
#define NTC_T (273.15f + 25) /* 25摄氏度下的绝对温度 */
#define NTC_R 10			 /* 10K */
/* vc30fx parameter */
const unsigned char vc30fx_temperature_current[] = {0, 2, 4, 8, 12, 16, 20, 24};
static signed short int get_ntc_temperature(float ntc_value)
{
    signed short int temperature = 0;
    float tmp = 0.0f;
    tmp = 1 / (1 / NTC_T + (log(ntc_value / NTC_R) / NTC_B)) - 273.15f;
    temperature = (signed short int)(tmp * 10);
    VCARE_DBG_LOG("[CALC_TMP]=%d(%4.2f)", temperature, tmp);
    return temperature;
}
static void vc30fx_temperature_ntc_calculate(vcare_ppg_device_t *pdev)
{
    unsigned char ext_I = 0;
    float ext_R = 0.0f;
    vc30fx_sample_info_t *presult_t = (vc30fx_sample_info_t *)pdev->result;
    if (presult_t->extra_result.tempext_enwork) {
        /* 采样值转换为电阻值 */
        VCARE_DBG_LOG("reg_tempinn=%d, reg_tempext=%d, reg_tempref=%d", presult_t->extra_result.tempinn_data, presult_t->extra_result.tempext_data, presult_t->extra_result.tempref_data);
        ext_I = vc30fx_temperature_current[presult_t->extra_result.tempext_setcur];
        if (0 != ext_I) {
            ext_R = (float)(20 * REF_R * presult_t->extra_result.tempext_data) / (float)(presult_t->extra_result.tempref_data * ext_I);
            VCARE_DBG_LOG("ext_R=%f", ext_R);
            pdev->temperature1 = get_ntc_temperature(ext_R);
        }
    }
}

/*********************************************************************************************************
 * USER_API
 ********************************************************************************************************/
/****************************************************************************
 * @description: 初始化IC并启动工作
 * @param {InitParamTypeDef} *pinitconfig
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_device_init(InitParamTypeDef *pinitconfig)
{
    unsigned short int ppgdiv = 0;
    unsigned char psdiv = 0;
    unsigned char fifodiv = 0;
    unsigned char frequency = 25; /* default frequency=25hz */
    /* ppg_freq=20K/ppgdiv, ps_freq=ppg_freq/psdiv, fifodiv=psfreq/fifodiv
     *  fifo_max=128byte. The amount of byte by once sampling must not be greater than fifo_max(128bytes).
     */
    extern unsigned short int VC30Fx_CFG_MCU_TICK_FREQUENCY; /* mcu tick frequency parameter */
    extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_HOLD; /* factory calibration parameter-hold */
    extern unsigned short int VC30Fx_WEAR_PARAM_BIO_ABS_DROP; /* factory calibration parameter-drop */
#if MY_VC30FX_MODIFY
    //NULL
#else
#if !PMC_DEVELOP_ENABLE
    VC30Fx_CFG_MCU_TICK_FREQUENCY = __get_lrc_hz() / 4;
#endif
#endif
    VC30Fx_WEAR_PARAM_BIO_ABS_HOLD = 200; /* save_diff*8/10 */
    VC30Fx_WEAR_PARAM_BIO_ABS_DROP = 160; /* save_diff*5/10 */

    vc30fx_dev.workmode = (work_mode)pinitconfig->work_mode;
    switch (pinitconfig->work_mode) {
    case WORK_MODE_HR:
    case WORK_MODE_SPO2:
    case WORK_MODE_WEAR:
    case WORK_MODE_FACTORY0:
    case WORK_MODE_FACTORY1:
        frequency = 25;
        break;
    case WORK_MODE_STRESS:
        frequency = 50;
        break;
    case WORK_MODE_HRV:
    case WORK_MODE_BLOODPRESSURE:
        frequency = 100;
        break;
    default:
        break;
    }
    ppgdiv = 20000 / frequency; /* ppg_freq = frequency */
#if (VC30Fx_PS_WORK_FREQUENCY_CONFIG==1)
    psdiv = pinitconfig->sampling_gap_time * 25 / 1000; /* 520ms ~ ps_div=13, fifo_div=1, 注意设置初始化参数的时间时，要估算一次fifo长度和fifo总长度的关系 */
    fifodiv = 1;
    if (WORK_MODE_FACTORY0 == pinitconfig->work_mode || WORK_MODE_FACTORY1 == pinitconfig->work_mode) {
        frequency = 50;
        ppgdiv = 20000 / frequency;
        psdiv = pinitconfig->sampling_gap_time * 25 / 1000; /* 注意：别修改psdiv，在有活体的配置里面，会影响活体的时间参数.所以此处通过提高一倍ppg频率，进而加快FAT模式的中断数据更新 */
        fifodiv = 1;
    }
#elif(VC30Fx_PS_WORK_FREQUENCY_CONFIG==0) /* ps=5hz */
    if (frequency == 25) {
        psdiv = 5;
        fifodiv = 4; /* ppg=25hz, psbio=5hz, fifo=1.25hz,fifotime=800ms */
        if (WORK_MODE_SPO2 == pinitconfig->work_mode) {
            fifodiv = 2; /* 提高fifo的频率，到400ms，留有一次丢fifo没处理的余量，不然发生异常时覆盖后解析会异常，可能会导致摘下重新佩戴 */
        }
    } else if (frequency == 50) {
        psdiv = 10;
        fifodiv = 2; /* ppg=50hz, psbio=5hz, fifo=2.5hz,fifotime=400ms */
    } else if (frequency == 100) {
        psdiv = 20;
        fifodiv = 1; /* ppg=100hz, psbio=5hz, fifo=5hz, fifotime=200ms */
    }
    if (WORK_MODE_HR == pinitconfig->work_mode || WORK_MODE_WEAR == pinitconfig->work_mode) {
        fifodiv = pinitconfig->sampling_gap_time * 50 / 10000;
    }
    if (WORK_MODE_FACTORY0 == pinitconfig->work_mode || WORK_MODE_FACTORY1 == pinitconfig->work_mode) {
        fifodiv = 1;
    }
    if (pinitconfig->work_mode == WORK_MODE_WEAR) {
        ppgdiv = 800; /* 25hz */
        psdiv  = 5;   /* 5hz  ~ 400ms */
        fifodiv = 10; /* 0.5hz ~2sec  */
    }
#endif
    vc30fx_drv_init_start_work(&vc30fx_dev, pinitconfig->work_mode, ppgdiv, psdiv, fifodiv);
    vc30fx_dev.dev_work_status = 1;
    /* 如果需要保持测试值，比如24h的心率模式，初始化时则不清除记录值心率值，避免初始化后出现一段为0的情况 */
    vc30fx_dev.heart_rate = 0;
    vc30fx_dev.stress = 0;
    vc30fx_dev.hrv = 0;
    vc30fx_dev.spo2 = 0;
    vc30fx_dev.blood_pressure = 0;
    vc30fx_dev.ecg = 0;
    vc30fx_dev.blood_pressure = 0;
    Algo_Init();
    spo2AlgoInit_16bit();
    vcSportMotionAlgoInit();
    VCARE_DBG_LOG("work_mode=%d", pinitconfig->work_mode);
    /* 如果使用定时器，必须适配模式的定时器执行时间，否则无法正确解析数据 */
    //unsigned short int timer_set_ms=(1000/frequency)*psdiv*fifodiv;
}

/****************************************************************************
 * @description: 获取IC及驱动相关参数
 * @param {unsigned char} *pchip_id
 * @param {unsigned char} *pvers_id
 * @param {char} *
 * @param {char} *
 * @return {*}
 ****************************************************************************/
u8 vc30fx_online(void)
{
    unsigned char chip_id = 0;
    unsigned char vers_id = 0;
    unsigned char online;
    char *drv_name;

    vc30fx_drv_get_hwsw_infomation(&drv_name, &chip_id, &vers_id);
    /*
    VC30FS:chip_id=0x25, vers_id=0x10
    VC12S :chip_id=0x27, vers_id=0x10, 0x30
    */
    VCARE_DBG_LOG(" -0- VC30Fx_drv_ver:%s", drv_name);
    //online = (chip_id==0x25 && vers_id==0x10) || (chip_id==0x27 && (vers_id==0x10 || vers_id==0x30));
    online = (chip_id == 0x25 && vers_id == 0x30) || (chip_id == 0x25 && vers_id == 0x10) || (chip_id == 0x27 && (vers_id == 0x10 || vers_id == 0x30)); // --AK
    VCARE_DBG_LOG("VC30Fx_online:chip_id=0x%02x, vers_id=0x%02x online=%d", chip_id, vers_id, online);
    return online;
}
/****************************************************************************
 * @description: 开始工作
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_start_work(void)
{
    vc30fx_drv_wakeup_work(&vc30fx_dev);
    vc30fx_dev.dev_work_status = 1;
}
/****************************************************************************
 * @description: 停止工作
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_stop_work(void)
{
    vc30fx_drv_sleep_work(&vc30fx_dev);
    vc30fx_dev.dev_work_status = 0;
}
/****************************************************************************
 * @description: 软复位
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_soft_reset(void)
{
    vc30fx_drv_software_reset(&vc30fx_dev);
    vc30fx_dev.dev_work_status = 0;
}
/****************************************************************************
 * @description: 活体IC参数重置校准
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_reset_bio_param(void)
{
    vc30fx_drv_clear_reset_bio_param(&vc30fx_dev);
}
/****************************************************************************
 * @description: 获取佩戴状态
 * @return {*}
 ****************************************************************************/
int vc30fx_usr_get_wear_status(void)
{
    if (WEAR_STA_HOLD == vc30fx_dev.wear) {
        return 1;
    }
    return 0;
}

/****************************************************************************
 * @description: 定时器校验当前IC状态,如果发生异常，则重启LDO后初始化
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_check_temperature_abnormal(void)
{
    unsigned char check_same_ret = 0, check_freq_ret = 0, check_temp_ret = 0;
    unsigned short check_cnt1 = 0, check_cnt2 = 0;
    check_same_ret = vc30fx_drv_read_check_temperature_abnormal(&vc30fx_dev, &check_freq_ret, &check_temp_ret, &check_cnt1, &check_cnt2);
    vc30fx_temperature_ntc_calculate(&vc30fx_dev);
    vc30fx_sample_info_t *result_info = (vc30fx_sample_info_t *)vc30fx_dev.result;
    VCARE_DBG_LOG("status(%d),psbio_0(%d,%d),cnt=%d,%d,same_ret=%d,freq_ret=%d,temp_ret=%d, temp_ext=%d", vc30fx_dev.dev_work_status,
                  result_info->slot_result[2].u.ps_data, result_info->extra_result.bioext_data, check_cnt1, check_cnt2,
                  check_same_ret, check_freq_ret, check_temp_ret, vc30fx_dev.temperature1);

    if (1 == vc30fx_dev.dev_work_status &&															/* IC处于工作状态 */
        ((result_info->slot_result[2].u.ps_data == 0 && (result_info->extra_result.bioext_data == 0 && result_info->extra_result.bioext_enwork)) /* ps和bio同时异常，或者RCOSC异常 */
         || (check_cnt1 == 0 && check_cnt2 == 0)
         || (1 == check_same_ret)							/* bio-cnt其中之一已经不再变化 */
         || (1 == check_freq_ret)																	/* 频率温度异常 */
         || (1 == check_temp_ret)																	/* 内部温度异常 */
         || vc30fx_dev.temperature1 >= 600)) {													/* NTC温度异常，如果硬件NC则会固定250 */
        /* 1.close ldo, 2.delay(200ms), 3.open ldo, 4.delay(10ms), 5.init_dev */
        //extern void vcare_ldo_power_ctrl(unsigned char onoff);
        //vcare_ldo_power_ctrl(0);
        //platform_delay_ms(200);
        //vcare_ldo_power_ctrl(1);
        //platform_delay_ms(10);
        //vc30fx_drv_init_start_work(&vc30fx_dev, vc30fx_dev.workmode, result_info->ppg_div, result_info->ps_div, result_info->fifo_div );
    }
}

/****************************************************************************
 * @description: 中断事件处理事件
 * @param {unsigned char} heart_algo_mode
 * @param {unsigned char} spo2_algo_mode
 * @return {*}
 ****************************************************************************/
void vc30fx_usr_device_handler(unsigned char heart_algo_mode, unsigned char spo2_algo_mode)
{
    unsigned short int ppg_num = 0;
    unsigned short int gsensor_num = 0;
    vc30fx_sample_info_t *sample_result_info_ptr = (vc30fx_sample_info_t *)vc30fx_dev.result;
    if (!vc30fx_dev.dev_work_status) {
        return;
    }
    VCARE_DBG_LOG("------------------");
    vc30fx_dev.heart_algo_mode = heart_algo_mode;
    vc30fx_dev.spo2_algo_mode = spo2_algo_mode;
    vc30fx_drv_get_result_handler(&vc30fx_dev);
    vc30fx_dev.wear = vc30fx_drv_get_wear_status(&vc30fx_dev);
    if (vc30fx_drv_is_ps_event(&vc30fx_dev) || vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
        //disp_data.wear_sta = vc30fx_dev.wear;
        //disp_data.disp_bio_val = sample_result_info_ptr->extra_result.bioext_data;
        //disp_data.disp_bio_abs = sample_result_info_ptr->extra_result.bioabs_calc;
        //VCARE_DBG_LOG("bio_absgap=%d, bioext=%d", disp_data.disp_bio_abs, disp_data.disp_bio_val );
    }
    switch (vc30fx_dev.workmode) {
    case WORK_MODE_HR:
        if (vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
            ppg_num = sample_result_info_ptr->slot_result[0].ppg_num;
            if (ppg_num > 40) {
                break;
            }
            vc30fx_get_board_gsensor_data(&gsensor_num, ppg_num);
            if (gsensor_num) {
                vc30fx_gsensor_actuating_quantity(vc30fx_gsensor_data.xData[0], vc30fx_gsensor_data.yData[0], vc30fx_gsensor_data.zData[0]);
            }
            if (WEAR_STA_HOLD == vc30fx_dev.wear) {
                vc30fx_dev.heart_rate = vc30fx_heart_rate_calculate(&vc30fx_dev);
                //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num);

#if MY_SENSOR_DEBUG //通过ble 发送400byte 原始数据
                extern void vc30fx_send_orginal_data(vcare_ppg_device_t *pdev, gsensor_axes * pgsensor, unsigned char ppgsize);
                vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num);
#endif
            } else {
                //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num?ppg_num:1);
                vc30fx_dev.heart_rate = 0;
                Algo_Init();
            }
            //disp_data.hr_rate = vc30fx_dev.heart_rate;
        }
        break;
    case WORK_MODE_SPO2:
        if (vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
            ppg_num = sample_result_info_ptr->slot_result[1].ppg_num;
            if (ppg_num > 40) {
                break;
            }
            vc30fx_get_board_gsensor_data(&gsensor_num, ppg_num);
            if (WEAR_STA_HOLD == vc30fx_dev.wear) {
                vc30fx_dev.spo2 = vc30fx_spo2_calculate(&vc30fx_dev);
                //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num);
            } else {
                vc30fx_dev.spo2 = 0;
                spo2AlgoInit_16bit();
                vcSportMotionAlgoInit();
            }
            //disp_data.spo2_val = vc30fx_dev.spo2;
        }
        break;
    case WORK_MODE_HRV:
        if (vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
            ppg_num = sample_result_info_ptr->slot_result[0].ppg_num;
            if (ppg_num > 40) {
                break;
            }
            vc30fx_get_board_gsensor_data(&gsensor_num, ppg_num);
            if (WEAR_STA_HOLD == vc30fx_dev.wear) {
                /*wear is hold,call_algo_func*/
                //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num);
            } else {
                /*wear is drop,clear_algo_data*/
            }
        }
        break;
    case WORK_MODE_STRESS:
        if (vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
            ppg_num = sample_result_info_ptr->slot_result[0].ppg_num;
            if (ppg_num > 40) {
                break;
            }
            vc30fx_get_board_gsensor_data(&gsensor_num, ppg_num);
            if (WEAR_STA_HOLD == vc30fx_dev.wear) {
                /*wear is hold,call_algo_func*/
                //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num);
            } else {
                /*wear is drop,clear_algo_data*/
            }
            //disp_data.hr_rate = vc30fx_dev.heart_rate;
        }
        break;
    case WORK_MODE_BLOODPRESSURE:
        if (vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
            ppg_num = sample_result_info_ptr->slot_result[0].ppg_num;
            if (ppg_num > 40) {
                break;
            }
            vc30fx_get_board_gsensor_data(&gsensor_num, ppg_num);
            if (WEAR_STA_HOLD == vc30fx_dev.wear) {
                /*wear is hold,call_algo_func*/
                //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, ppg_num);
            } else {
                /*wear is drop,clear_algo_data*/

            }
        }
        break;
    case WORK_MODE_WEAR:
        if (vc30fx_drv_is_fifo_event(&vc30fx_dev)) {
            vc30fx_get_board_gsensor_data(&gsensor_num, ppg_num);
            if (gsensor_num) {
                vc30fx_gsensor_actuating_quantity(vc30fx_gsensor_data.xData[0], vc30fx_gsensor_data.yData[0], vc30fx_gsensor_data.zData[0]);
            }
            //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, sample_result_info_ptr->slot_result[2].ppg_num);
        }
        break;
    case WORK_MODE_TEMPERATURE:
        vc30fx_temperature_ntc_calculate(&vc30fx_dev);
        //vc30fx_send_orginal_data(&vc30fx_dev, &vc30fx_gsensor_data, 1);
        //disp_data.temperature_value0 = vc30fx_dev.temperature1;
        break;
    case WORK_MODE_FACTORY0:
        //disp_data.gImx = sample_result_info_ptr->slot_result[0].slot_current;
        //disp_data.gPre = sample_result_info_ptr->slot_result[0].u.pre_data;
        //disp_data.ps = sample_result_info_ptr->slot_result[2].u.ps_data;
        //disp_data.rImx = sample_result_info_ptr->slot_result[1].slot_current;
        //disp_data.rPre = sample_result_info_ptr->slot_result[1].u.pre_data;


        //bio活体bio参数获取对空bio以及导电快上的bio值，二者作差，差值存人f1ash
        //ps放蓝色玻璃片上要求大于210
        //pre 放黑色硅胶快上要求小于48
        //cur 要求大于100
        printf("bio:%d, ps:%d, gPre=%d, gImx=%d", \
               sample_result_info_ptr->extra_result.bioext_buffer[0], \
               sample_result_info_ptr->slot_result[2].u.ps_data, \
               sample_result_info_ptr->slot_result[0].u.pre_data, \
               sample_result_info_ptr->slot_result[0].slot_current);

        break;
    case WORK_MODE_FACTORY1:
        //disp_data.iImx = sample_result_info_ptr->slot_result[0].slot_current;
        //disp_data.iPre = sample_result_info_ptr->slot_result[0].u.pre_data;
        //disp_data.rImx = sample_result_info_ptr->slot_result[1].slot_current;
        //disp_data.rPre = sample_result_info_ptr->slot_result[1].u.pre_data;
        break;
    default:
        break;
    }
    /* 同步处理数据，清理fifo_buffer内存，为下次采样做准备 */
    vc30fx_drv_finished_sync(&vc30fx_dev);
}

u8 vc30fx_init()
{
    return vc30fx_online();
}

unsigned char vc30fx_irq_flg = 0;
u8 vc30fx_process()
{
    if (vc30fx_irq_flg) {
        vc30fx_irq_flg = 0;

        vc30fx_usr_device_handler(heart_algo_mode, spo2_algo_mode);

        algo_info.hr_value   = vc30fx_dev.heart_rate;
        algo_info.spo2_value = vc30fx_dev.spo2;
        algo_info.wear       = vc30fx_dev.wear;
        //VCARE_DBG_LOG("hr=%d, spo2=%d, wear=%d", algo_info.hr_value, algo_info.spo2_value, algo_info.wear);
        printf("hr=%d, spo2=%d, wear=%d", algo_info.hr_value, algo_info.spo2_value, algo_info.wear);
    }
    return 0;
}
#if MY_VC30FX_MODIFY
u8 vc30fx_irq_handler()
{
    extern void local_irq_disable();
    extern void local_irq_enable();
    //local_irq_disable();
    VCARE_DBG_LOG("vc30fx_irq_handler");
    extern int refresh_watch_data_check_cb(int *parm);
    extern unsigned char vc30fx_drv_clk_calc_status(void);

    vc30fx_irq_flg = 1;
    if (vc30fx_drv_clk_calc_status()) {
        VCARE_DBG_LOG("-1-");
        vc30fx_process();
    } else {
        vc30fx_process();
    }
    //local_irq_enable();
    return 0;
}
#else
u8 vc30fx_irq_handler()
{
    /* VCARE_DBG_LOG("vc30fx_irq_handler"); */
    extern int refresh_watch_data_check_cb(int *parm);
    extern unsigned char vc30fx_drv_clk_calc_status(void);

    vc30fx_irq_flg = 1;
    if (vc30fx_drv_clk_calc_status()) {
        /* refresh_watch_data_check_cb(NULL); */
    } else {
        vc30fx_process();
    }
    // local_irq_enable();
    return 0;
}
#endif

static int work_module = 0;
void vc30fx_io_interrupt_handler()
{
    sport_health_manager_msg_post(work_module, SHM_CMD_UPDATE, 0, 0);
}


void vc30fxStart(u8 work_mode)
{
    VCARE_DBG_LOG(" work_mode = %d", work_mode);
    InitParamTypeDef initconfig = {VC30FX_SCAN_PERIOD/* 400 */, work_mode};
    vc30fx_usr_device_init(&initconfig);
    work_module = work_mode;
    switch (work_mode) {
    case WORK_MODE_HR:
        heart_algo_mode = work_mode;
        work_module = SHM_MOD_HEART_RATE_ALGO;
        break;
    case WORK_MODE_SPO2:
        spo2_algo_mode = work_mode;
        work_module = SHM_MOD_SPO2_ALGO;
        break;
    case WORK_MODE_WEAR:
        work_module = SHM_MOD_WEAR_DETECTION;
        break;
    default:
        break;
    }
#if !PMC_DEVELOP_ENABLE
    hr_sensor_irq_init();    //配置中断唤醒
#endif
}
// 条件编译: 使用新分层架构时不编译原有模块注册
#if 0
#include "sensor_hub.h"
#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[30FX]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"




static int heart_rate_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    u8 *mode = (u8 *)priv;

    switch (cmd) {
    case SHM_CMD_ENABLE:
        heart_algo_mode = *mode;
        vc30fxStart(WORK_MODE_HR);
        break;
    case SHM_CMD_DISABLE:
        vc30fx_usr_stop_work();
        break;
    case SHM_CMD_UPDATE:
        vc30fx_irq_handler();
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }

    log_debug("cmd=%d ret:%d", cmd, ret);
    return ret;
}

static int heart_rate_get(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_GET_TYPE_REAL_VALUE:
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}
REGISTER_SPORT_HEALTH_MODULE(vc30fx_hr)
{
    .module = SHM_MOD_HEART_RATE_ALGO,
     .io_ctrl = heart_rate_crtl,
      .get_value = heart_rate_get,
};


static int spo2_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    u8 *mode = (u8 *)priv;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        spo2_algo_mode = *mode;
        vc30fxStart(WORK_MODE_SPO2);
        break;
    case SHM_CMD_DISABLE:
        vc30fx_usr_stop_work();
        break;
    case SHM_CMD_UPDATE:
        vc30fx_irq_handler();
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }

    log_debug("cmd=%d ret:%d", cmd, ret);
    return ret;
}

static int spo2_get(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_GET_TYPE_REAL_VALUE:
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}
REGISTER_SPORT_HEALTH_MODULE(vc30fx_spo2)
{
    .module = SHM_MOD_SPO2_ALGO,
     .io_ctrl = spo2_crtl,
      .get_value = spo2_get,
};

static int wear_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        vc30fxStart(WORK_MODE_WEAR);
        break;
    case SHM_CMD_DISABLE:
        vc30fx_usr_stop_work();
        break;
    case SHM_CMD_UPDATE:
        vc30fx_irq_handler();
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }

    log_debug("cmd=%d ret:%d", cmd, ret);
    return ret;
}

static int wear_get(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_GET_TYPE_REAL_VALUE:
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}
REGISTER_SPORT_HEALTH_MODULE(vc30fx_wear)
{
    .module = SHM_MOD_WEAR_DETECTION,
     .io_ctrl = wear_crtl,
      .get_value = wear_get,
};



#include "circular_buf.h"

#define GSENSER_CBUF_SIZE  (30*2*3)
static cbuffer_t *gsensor_cbuf = NULL;
static short     *gsensor_buff = NULL;
void gsensor_cbuf_write(short frame, short *buf)
{
    printf("gsensor_cbuf_write %d frame\n", frame);
    u32 w_size = 2 * 3 * frame;

    if (gsensor_cbuf == NULL) {
        gsensor_buff = zalloc(GSENSER_CBUF_SIZE);
        if (gsensor_buff == NULL) {
            log_e("malloc gsensor buff error1!\n");
            return;
        }

        gsensor_cbuf = zalloc(sizeof(cbuffer_t));
        if (gsensor_cbuf == NULL) {
            free(gsensor_buff);
            log_e("malloc gsensor cbuf error2!\n");
            return;
        }
        cbuf_init(gsensor_cbuf, gsensor_buff, GSENSER_CBUF_SIZE);
    }

    //如果cbuf满了丢掉旧数据，写入新数据，以保证数据最新
    u32 e_size = gsensor_cbuf->total_len - gsensor_cbuf->data_len;
    if (e_size <= w_size) {
        u32 p_size = w_size - e_size;
        p_size = p_size < 2 * 3 ? 2 * 3 : p_size;
        cbuf_read_alloc_len_updata(gsensor_cbuf, p_size);
    }

    w_size = cbuf_write(gsensor_cbuf, buf, w_size);
    if (w_size == 0) {
        printf("data_w_cbuf_full");
    }
}

u32 gsensor_cbuf_read(short frame, short *buf)
{
    u32 r_size = 2 * 3 * frame;
#if 0
    char *r_buff = zalloc(GSENSER_CBUF_SIZE);
    u32   buff_size =  cbuf_read_alloc_len(gsensor_cbuf, r_buff, GSENSER_CBUF_SIZE - 6);
    u32   offset    =  0;
    if (buff_size > r_size) {
        offset    =  buff_size - r_size;
    }
    memcpy(buf, &r_buff[offset], r_size);
    free(r_buff);
    u32 gsen_frame = MIN(buff_size, r_size);
    return gsen_frame / 6;
#else
    u32 gsen_frame = cbuf_read_alloc_len(gsensor_cbuf, buf, r_size);
    return gsen_frame / 6;
#endif
}








// static u8 iic0_init = 0;
// static u8 heart_rate_enter_deepsleep(void)
// {
//     iic0_init = 0;
// }

// static u8 heart_rate_exit_deepsleep(void)
// {
//     if(!iic0_init){
//         system_iic_init(0, get_hw_iic_config(0));
//     }
// }

// DEEPSLEEP_TARGET_REGISTER(heart_rate) = {
//     .name   = "heart_rate",
//     .enter  = heart_rate_enter_deepsleep,
//     .exit   = heart_rate_exit_deepsleep,
// };



#endif // USE_NEW_LAYER_ARCH

#endif//TCFG_VC30fx_ENABLE
