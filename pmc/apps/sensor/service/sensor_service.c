#include "health_manager.h"
#include "sensor_seg.h"
/* L0/L1/L2 → RAM；L3 → Flash（FIFO 类同向，函数级 AT 覆盖） */
#if TCFG_SENSOR_GSENSOR_LEVEL <= 2
#pragma bss_seg(".sensor_bss.L1")
#pragma data_seg(".sensor_data.L1")
#pragma code_seg(".sensor_code.L1")
#pragma const_seg(".sensor_const")
#else
#pragma bss_seg(".sensor_bss.L2")
#pragma data_seg(".sensor_data.L2")
#pragma code_seg(".sensor_code.L2")
#pragma const_seg(".sensor_const")
#endif
#define LOG_TAG_CONST       SENSOR_SERVICE
#define LOG_TAG     		"[SENSOR_SERVICE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#define __DEBUG
#include "log.h"


AT_SENSOR_COLD int sensor_service_init(int mod)
{
    sensor_service_t *service;
    int (*init)(int enable) = NULL;
    list_for_each_sensor_service(service) {
        if (service->mod == mod) {
            init = service->init;
            break;
        }
    }
    if (!init) {
        log_error("mod:%d", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return init(true);
}

AT_SENSOR_COLD int sensor_service_uninit(int mod)
{
    sensor_service_t *service;
    int (*init)(int enable) = NULL;
    list_for_each_sensor_service(service) {
        if (service->mod == mod) {
            init = service->init;
            break;
        }
    }
    if (!init) {
        log_error("mod:%d", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return init(false);
}

AT_SENSOR_COLD int sensor_service_enable(int mod)
{
    sensor_service_t *service;
    int (*enable)(int enable) = NULL;
    list_for_each_sensor_service(service) {
        if (service->mod == mod) {
            enable = service->enable;
            break;
        }
    }
    if (!enable) {
        log_error("mod:%d", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    log_info("%s mod=%d -> call enable", __func__, mod);
    return enable(true);
}

AT_SENSOR_COLD int sensor_service_disable(int mod)
{
    sensor_service_t *service;
    int (*enable)(int enable) = NULL;
    list_for_each_sensor_service(service) {
        if (service->mod == mod) {
            enable = service->enable;
            break;
        }
    }
    if (!enable) {
        log_error("mod:%d", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    log_info("%s mod=%d -> call disable", __func__, mod);
    return enable(false);
}

AT_SENSOR_ALGO int sensor_service_get_result(int mod, int type, void *data)
{
    sensor_service_t *service;
    int (*get_result)(int type, void *p) = NULL;
    list_for_each_sensor_service(service) {
        if (service->mod == mod) {
            get_result = service->get_result;
            break;
        }
    }
    if (!get_result) {
        log_error("mod:%d", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    int ret = get_result(type, data);
    {
        static unsigned int get_print_cnt = 0;
        if ((get_print_cnt++ % 100) == 0) {
            log_debug("%s mod=%d type=%d ret=%d val=%d", __func__, mod, type, ret, data ? * (int *)data : 0);
        }
    }
    return ret;
}

AT_SENSOR_COLD int sensor_service_set_config(int mod, int type, void *data)
{
    sensor_service_t *service;
    int (*set_config)(int type, void *p) = NULL;
    list_for_each_sensor_service(service) {
        if (service->mod == mod) {
            set_config = service->set_config;
            break;
        }
    }
    if (!set_config) {
        log_error("mod:%d", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return set_config(type, data);
}


