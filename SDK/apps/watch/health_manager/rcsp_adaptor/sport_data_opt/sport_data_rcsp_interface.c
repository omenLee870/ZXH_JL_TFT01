#include "app_config.h"
#include "sport_data_func.h"
#include "ui/ui_api.h"
#include "system/init.h"


#if (JL_RCSP_SENSORS_DATA_OPT)

__attribute__((weak))
void data_func_attr_weather_set(void *priv, u8 attr, u8 *data, u16 len, u16 ble_con_handle, u8 *spp_remote_addr)
{
}

static const sport_attr_get_func target_common_sport_func_get_tab[SPORTS_DATA_FUNC_ATTR_TYPE_GET_MAX] = {
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_HEART_RATE 		          ] = sport_data_func_attr_heart_rate_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_AIR_PRESSURE 		      ] = sport_data_func_attr_air_pressure_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_ALTITUDE 		          ] = sport_data_func_attr_altitude_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_EXERCISE_STEPS 		      ] = sport_data_func_attr_exercise_steps_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_PRESSURE_DETECTION        ] = sport_data_func_attr_pressure_detection_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_BLOOD_OXYGEN 	          ] = sport_data_func_attr_blood_oxygen_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_TRAINING_LOAD 	          ] = sport_data_func_attr_training_load_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_MAX_OXYGEN_UPTAKE         ] = sport_data_func_attr_max_oxygen_uptake_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_EXERCISE_RECOVERY_TIME    ] = sport_data_func_attr_exercise_recovery_time_get, */
    /* [SPORTS_DATA_FUNC_ATTR_TYPE_SPORTS_INFORMATION 	      ] = sport_data_func_attr_sports_information_get, */
};

static const attr_set_func target_common_func_set_tab[DATA_FUNC_ATTR_TYPE_SET_MAX] = {
    [DATA_FUNC_ATTR_TYPE_LOCATION	 		          ] = NULL,
    [DATA_FUNC_ATTR_TYPE_WEATHER	 		          ] = data_func_attr_weather_set,
    [DATA_FUNC_ATTR_TYPE_NOTICE 		              ] = NULL,
    [DATA_FUNC_ATTR_TYPE_NOTICE_REMOVE 		          ] = NULL,
};

static const rcsp_sport_data_opt_t rcsp_sport_data_opt = {
    .sport_data_get_tab =  target_common_sport_func_get_tab,
    .sport_data_get_tab_num = SPORTS_DATA_FUNC_ATTR_TYPE_GET_MAX,
    .sport_data_set_tab = target_common_func_set_tab,
    .sport_data_set_tab_num = DATA_FUNC_ATTR_TYPE_SET_MAX,
};


int rcsp_sport_data_trans(void)
{
    rcsp_register_sport_data_opt_interface((rcsp_sport_data_opt_t *)&rcsp_sport_data_opt);
    return 0;
}

late_initcall(rcsp_sport_data_trans);

#endif /* if (TCFG_SPORT_HEALTH_ENABLE && JL_RCSP_SENSORS_DATA_OPT) */

