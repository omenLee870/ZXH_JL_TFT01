#include "asm/cpu.h"
#include "sensor_hub.h"
#include "health_manager.h"
#include "vcHr11Hci.h"
#include "algo.h"
#include "spo2Algo.h"
#include "vcSportMotionIntAlgo.h"


#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_SER_HR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".sensor_hub.data.bss")
#pragma data_seg(".sensor_hub.data")
#pragma const_seg(".sensor_hub.text.const")
#pragma code_seg(".sensor_hub.text")
#endif

#if (TCFG_SPORT_HEALTH_ENABLE && TCFG_SENSOR_HUB && (TCFG_VCHR11_MASTER_ENABLE || TCFG_VCHR11_P11_ENABLE))

#define VC_HR_ODR            25
#define ACCELER_CBUF_INDEX   2

static  vcHr11_t  *vcHr11 = NULL;
static  u8         wear_status = 0;

static s8 vcHr11_init(void)
{
    sensor_info_t *info = sensor_hub_get_info(SENSOR_DRV_HR);

    log_debug("%s info=%x vcHr11=%x", __func__, info, vcHr11);
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


static int vc_hr_crtl(int cmd, void *priv)
{
    log_info("%s cmd=%d", __func__, cmd);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_INIT:
        ret = vcHr11_init();
        break;
    case SHM_CMD_ENABLE:
        Algo_Init();
        sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 1);
        ret = sensor_hub_enable(SENSOR_DRV_HR, 1);
        break;
    case SHM_CMD_DISBALE:
        sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 0);
        ret = sensor_hub_enable(SENSOR_DRV_HR, 0);
        break;
    default:
        break;
    }
    return ret;
}

static int vc_hr_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        ret = vc_hrs_calculate(priv);
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(hrm)
{
    .module    = SHM_MOD_HEART_RATE_ALGO,
     .io_ctrl   = vc_hr_crtl,
      .get_value = vc_hr_get,
};



static int vc_spo2_crtl(int cmd, void *priv)
{
    // log_info("%s cmd=%d",__func__,cmd);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        spo2AlgoInit();
        vcSportMotionAlgoInit();
        sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 1);
        ret = sensor_hub_enable(SENSOR_DRV_SPO2, 1);
        break;
    case SHM_CMD_DISBALE:
        sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, ACCELER_CBUF_INDEX, 0);
        ret = sensor_hub_enable(SENSOR_DRV_SPO2, 0);
        break;
    default:
        break;
    }
    return ret;
}

static int vc_spo2_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        ret = vc_spo2_calculate(priv);
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(spo2)
{
    .module    = SHM_MOD_SPO2_ALGO,
     .io_ctrl   = vc_spo2_crtl,
      .get_value = vc_spo2_get,
};


static int vc_wear_crtl(int cmd, void *priv)
{
    // log_info("%s cmd=%d",__func__,cmd);
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        ret = sensor_hub_enable(SENSOR_DRV_WEAR_DETECTION, 1);
        break;
    case SHM_CMD_DISBALE:
        ret = sensor_hub_enable(SENSOR_DRV_WEAR_DETECTION, 0);
        break;
    default:
        break;
    }
    return ret;
}

static int vc_wear_get(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        ret = vc_wear_status_get(priv);
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}

REGISTER_SPORT_HEALTH_MODULE(wear)
{
    .module    = SHM_MOD_WEAR_DETECTION,
     .io_ctrl   = vc_wear_crtl,
      .get_value = vc_wear_get,
};

#endif
