#include "health_manager.h"
#if 1
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


int sensor_service_init(int mod)
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
        log_error("mod:%d\n", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return init(true);
}

int sensor_service_uninit(int mod)
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
        log_error("mod:%d\n", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return init(false);
}

int sensor_service_enable(int mod)
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
        log_error("mod:%d\n", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return enable(true);
}

int sensor_service_disable(int mod)
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
        log_error("mod:%d\n", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return enable(false);
}

int sensor_service_get_result(int mod, int type, void *data)
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
        log_error("mod:%d\n", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return get_result(type, data);
}

int sensor_service_set_config(int mod, int type, void *data)
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
        log_error("mod:%d\n", mod);
        return -SHM_ERR_MOD_NOT_FIND;
    }
    return set_config(type, data);
}


