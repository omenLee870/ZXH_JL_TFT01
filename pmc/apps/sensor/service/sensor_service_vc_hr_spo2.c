
#include "health_manager.h"
#include "sensor_hub.h"
#include "vcHr11Hci.h"
#include "driver/driver_vc_hr_11/algo.h"
#include "driver/driver_vc_hr_11/spo2Algo.h"
#include "driver/driver_vc_hr_11/vcSportMotionIntAlgo.h"


#define LOG_TAG_CONST      	SENSOR_SERVICE
#define LOG_TAG     		"[SENSOR_SRV_HR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"




#if (CONFIG_SPORT_HEALTH_MANAGER_ENABLE	&& TCFG_SENSOR_HUB && TCFG_SENSOR_SERVICE_VC_HR_SPO2_EN	)

#define VC_HR_ODR            25
#define ACCELER_CBUF_INDEX   2

static  vcHr11_t  *vcHr11 = NULL;
static  u8         wear_status = 0;

static s8 vcHr11_init(void)
{
    sensor_info_t *info = sensor_hub_get_info(SENSOR_DRV_HR);

    log_debug("%s info=%x vcHr11=%x", __func__, (u32)info, (u32)vcHr11);
    if (info) {
        u32 addr = 0;
        memcpy(&addr, info->range, sizeof(u32));
        vcHr11 = (vcHr11_t *)(addr + P11_RAM_BASE);
        // log_info("%s vcHr11_t addr:0x%x 0x%x", __func__, addr, vcHr11);
    } else {
        log_error("%s sensor_info is null", __func__);
    }
    return vcHr11 == NULL ? SHM_ERR_MOD_NOT_FIND : SHM_ERR_OK;
}

static s8 vc_wear_status_get(u8 *data)
{
    if (vcHr11 == NULL) {
        return SHM_ERR_MOD_NOT_FIND;
    }

    data[0] = vcHr11->wearStatus;
    return SHM_ERR_OK;
}

static void accelerat_data_alignment(axis_data_t *acc_data, u8 acc_len, u8 ppg_len)
{
    if (acc_len < ppg_len) {
        u8 m = acc_len - 1;
        for (u8 i = acc_len; i < ppg_len; i++) {
            acc_data[i] = acc_data[m];
        }
    }
}

static s8 vc_hrs_calculate(u8 *data)
{
    AlgoInputData_t  algoInputData;
    AlgoOutputData_t algoOutputData;

    axis_data_t acc_data[50];
    u16 ppg_data[50];


    u16 acc_len = sensor_hub_get_data(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, (void *)&acc_data, sizeof(acc_data));
    u16 ppg_len = sensor_hub_get_data(SENSOR_DRV_HR, 0, (void *)&ppg_data, sizeof(ppg_data));
    log_info("%s acc_len:%d ppg_len:%d", __func__, acc_len, ppg_len);

    if (ppg_len == 0) {
        data[0] = 0;
        return SHM_ERR_OK;
    }

    accelerat_data_alignment(acc_data, acc_len, ppg_len);

    algoInputData.envSample = vcHr11->sampleData.envValue[0];
    for (u16 i = 0; i < ppg_len; i++) {
        algoInputData.ppgSample = ppg_data[i];
        algoInputData.axes.x    = acc_data[i].x / 4; //The direction vertical with ARM.
        algoInputData.axes.y    = acc_data[i].y / 4; //The direction parallel with ARM.
        algoInputData.axes.z    = acc_data[i].z / 4; //The direction upside.
        Algo_Input(&algoInputData, 1000 / VC_HR_ODR, SPORT_TYPE_NORMAL, 0, 0);
        // log_info("%s no:%d,ppg=%d,acc=%d %d %d", __func__, i+1, algoInputData.ppgSample,algoInputData.axes.x,algoInputData.axes.y,algoInputData.axes.z);
    }

    Algo_Output(&algoOutputData);
    log_debug("%s out:%dbpm", __func__, algoOutputData.hrData);

    if (algoOutputData.hrData == -1) {
        Algo_Init();
        wear_status = 0;
        return SHM_ERR_OS_ERR;
    }
    data[0] = algoOutputData.hrData;
    return SHM_ERR_OK;
}

static s8 vc_spo2_calculate(u8 *data)
{
    u8 vcSportFlag = 0;
    AlgoInputData_t  algoInputData;
    AlgoOutputData_t algoOutputData;

    axis_data_t acc_data[50];
    u16 ppg_data[50];


    u16 acc_len = sensor_hub_get_data(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, (void *)&acc_data, sizeof(acc_data));
    u16 ppg_len = sensor_hub_get_data(SENSOR_DRV_SPO2, 0, (void *)&ppg_data, sizeof(ppg_data)) / 2;
    log_info("%s acc_len:%d ppg_len:%d", __func__, acc_len, ppg_len);

    accelerat_data_alignment(acc_data, acc_len, ppg_len);

    for (u16 i = 0; i < ppg_len; i++) {
        s32 vcIrPPG     = ppg_data[i * 2];
        s32 vcRedPPG    = ppg_data[i * 2 + 1];
        s32 vcSpo2Value = spo2Algo(vcRedPPG, vcIrPPG, 0);
        s16 accX = acc_data[i].x >> 2;
        s16 accY = acc_data[i].y >> 2;
        s16 accZ = acc_data[i].z >> 2;

        vcSportFlag = vcSportMotionCalculate(accX, accY, accZ);

        if ((!vcSportFlag) && (vcSpo2Value > 0)) {
            data[0] = vcSpo2Value;
        }
        log_info("%s no:%d,ppg=%d %d,acc=%d %d %d", __func__, i + 1, vcIrPPG, vcRedPPG, accX, accY, accZ);
    }
    log_info("%s out:%d", __func__, data[0]);
    return SHM_ERR_OK;
}
/***********************************
  对外api。
 **********************************/
int sensor_service_heart_rate_init()
{
    int ret = SHM_ERR_OK;
    ret = vcHr11_init();
    return ret;
}

int sensor_service_heart_rate_enable()
{
    int ret = SHM_ERR_OK;
    Algo_Init();
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 1);
    ret = sensor_hub_enable(SENSOR_DRV_HR, 1);
    return ret;
}

int sensor_service_heart_rate_disable()
{
    int ret = SHM_ERR_OK;
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 0);
    ret = sensor_hub_enable(SENSOR_DRV_HR, 0);
    return ret;
}

int sensor_service_heart_rate_get_result(u8 *data)
{
    int ret = SHM_ERR_OK;
    ret = vc_hrs_calculate(data);
    return ret;
}

int sensor_service_spo2_init()
{
    int ret = SHM_ERR_OK;
    return ret;
}

int sensor_service_spo2_enable()
{
    int ret = SHM_ERR_OK;
    spo2AlgoInit();
    vcSportMotionAlgoInit();
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 1);
    ret = sensor_hub_enable(SENSOR_DRV_SPO2, 1);
    return ret;
}

int sensor_service_spo2_disable()
{
    int ret = SHM_ERR_OK;
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 0);
    ret = sensor_hub_enable(SENSOR_DRV_SPO2, 0);
    return ret;
}

int sensor_service_spo2_get_result(u8 *data)
{
    int ret = SHM_ERR_OK;
    ret = vc_spo2_calculate(data);
    return ret;
}

int sensor_service_wear_enable()
{
    int ret = SHM_ERR_OK;

    ret = sensor_hub_enable(SENSOR_DRV_WEAR_DETECTION, 1);
    return ret;
}

int sensor_service_wear_disable()
{
    int ret = SHM_ERR_OK;

    ret = sensor_hub_enable(SENSOR_DRV_WEAR_DETECTION, 0);
    return ret;
}

int sensor_service_wear_get_result(u8 *data)
{
    int ret = SHM_ERR_OK;

    ret = vc_wear_status_get(data);
    return ret;
}

#endif



