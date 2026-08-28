#ifndef	__SENSOR_SERVICE__
#define	__SENSOR_SERVICE__


typedef struct {
    s8 mod;
    int(*init)(int enable);
    int(*enable)(int enable);
    int(*get_result)(int type, void *p);
    int(*set_config)(int type, void *p);
} sensor_service_t;

extern sensor_service_t sensor_service_begin[];
extern sensor_service_t sensor_service_end[];

#define REGISTER_SENSOR_SERVICE(service) \
	static const sensor_service_t service SEC_USED(.sensor_service) =

#define list_for_each_sensor_service(c) \
	for (c= sensor_service_begin; c< sensor_service_end; c++)


int sensor_service_init(int mod);
int sensor_service_uninit(int mod);
int sensor_service_enable(int mod);
int sensor_service_disable(int mod);
int sensor_service_get_result(int mod, int type, void *data);
int sensor_service_set_config(int mod, int type, void *data);
#endif



