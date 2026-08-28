#include "sensor_hub.h"
#include "health_manager.h"

#define LOG_TAG_CONST      	SENSOR_HUB
#define LOG_TAG     		"[SENSOR_SRV_COMPASS]"
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


#if TCFG_SPORT_HEALTH_ENABLE&& TCFG_SENSOR_HUB						&& (TCFG_MAGNETIC_MASTER_ENABLE || TCFG_MAGNETIC_P11_ENABLE)

typedef axis_data_t data_t;
extern short sensor_algorithm_jl_compass_run(short acc_x, short acc_y, short acc_z, short mag_x, short mag_y, short mag_z);

static s16 Azimuth = 0;  //为负数时表示未校准


void sensor_service_compass_get(int *value)
{
    axis_data_t acc[50];
    axis_data_t mag[50];

    u16 i = sensor_hub_get_data(SENSOR_DRV_ACCELER, 1, acc,  sizeof(acc));
    u16 j = sensor_hub_get_data(SENSOR_DRV_MAGNETIC, 0, mag, sizeof(mag));

    if (i > 0 && j > 0) {
        i -= 1;
        j -= 1;
        Azimuth = sensor_algorithm_jl_compass_run(acc[i].x, acc[i].y, acc[i].z, mag[j].x, mag[j].y, mag[j].z);
        log_debug("Azimuth=%d,acc:%d,%d,%d mag:%d,%d,%d", Azimuth, acc[i].x, acc[i].y, acc[i].z, mag[j].x, mag[j].y, mag[j].z);
    }
    *value = Azimuth;
}


static int shm_service_compass_io_crtl(int cmd, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (cmd) {
    case SHM_CMD_ENABLE:
        ret |= sensor_hub_enable(SENSOR_DRV_MAGNETIC, 1);
        ret |= sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, 1, 1);
        break;
    case SHM_CMD_DISBALE:
        ret |= sensor_hub_enable(SENSOR_DRV_MAGNETIC, 0);
        ret |= sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, 1, 0);
        break;
    default:
        /* ret = -SHE_ERR_MOD_NO_THIS_CMD; */
        break;
    }

    log_debug("cmd=%d ret:%d", cmd, ret);
    return ret;
}

static int shm_service_compass_get_value(int type, void *priv)
{
    int ret = SHM_ERR_OK;
    switch (type) {
    case SHM_GET_TYPE_REAL_VALUE:
        sensor_service_compass_get(priv);
        break;
    default:
        ret = -SHM_ERR_MOD_NO_THIS_TYPE;
        break;
    }
    return ret;
}


REGISTER_SPORT_HEALTH_MODULE(compass)
{
    .module = SHM_MOD_COMPASS,
     .io_ctrl = shm_service_compass_io_crtl,
      .get_value = shm_service_compass_get_value,
};

#endif //TCFG_SPORT_HEALTH_ENABLE & (TCFG_MAGNETIC_MASTER_ENABLE | TCFG_MAGNETIC_P11_ENABLE)
