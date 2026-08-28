#include "app_config.h"
#include "sport_info_opt.h"
#include "ui/ui_api.h"
#include "system/init.h"

#include "sport_info_bt_disconn.h"
#include "sport_info_continuous_heart_rate.h"
#include "sport_info_exercise_heart_rate.h"
#include "sport_info_fall_detection.h"
#include "sport_info_personal_info.h"
#include "sport_info_pressure_detection.h"
#include "sport_info_raise_wrist.h"
#include "sport_info_sedentary.h"
#include "sport_info_sensor_opt.h"
#include "sport_info_sleep_detection.h"


#if JL_RCSP_SENSORS_DATA_OPT

static const attr_get_func target_common_sport_info_get_tab[SPORTS_INFO_OPT_FUNC_ATTR_TYPE_MAX] = {
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_RESERVE] = NULL,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_SENSOR_OPT] = sport_info_sensor_opt_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_SEDENTARY] = sport_info_sedentary_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_CONTINUOUS_HEART_RATE] = sport_info_continuous_heart_rate_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_EXERCISE_HEART_RATE] = sport_info_exercise_heart_rate_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_PRESSURE_DETECTION] = sport_info_pressure_detection_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_SLEEP_DETECTION] = sport_info_sleep_detection_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_FALL_DETECTION] = sport_info_fall_detection_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_RAISE_WRIST] = sport_info_raise_wrist_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_PERSONAL_INFO] = sport_info_personal_info_attr_get,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_BT_DISCONN] = sport_info_bt_disconn_attr_get,
};

static const attr_set_func target_common_sport_info_set_tab[SPORTS_INFO_OPT_FUNC_ATTR_TYPE_MAX] = {
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_RESERVE] = NULL,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_SENSOR_OPT] = sport_info_sensor_opt_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_SEDENTARY] = sport_info_sedentary_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_CONTINUOUS_HEART_RATE] = sport_info_continuous_heart_rate_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_EXERCISE_HEART_RATE] = sport_info_exercise_heart_rate_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_PRESSURE_DETECTION] = sport_info_pressure_detection_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_SLEEP_DETECTION] = sport_info_sleep_detection_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_FALL_DETECTION] = sport_info_fall_detection_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_RAISE_WRIST] = sport_info_raise_wrist_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_PERSONAL_INFO] = sport_info_personal_info_attr_set,
    [SPORTS_INFO_OPT_FUNC_ATTR_TYPE_BT_DISCONN] = sport_info_bt_disconn_attr_set,
};

static const rcsp_sport_info_opt_t rcsp_sport_info_opt = {
    .sport_info_get_tab = target_common_sport_info_get_tab,
    .sport_info_get_tab_num = SPORTS_INFO_OPT_FUNC_ATTR_TYPE_MAX,
    .sport_info_set_tab = target_common_sport_info_set_tab,
    .sport_info_set_tab_num = SPORTS_INFO_OPT_FUNC_ATTR_TYPE_MAX,
};


int rcsp_sport_info_trans(void)
{
    rcsp_register_sport_info_opt_interface((rcsp_sport_info_opt_t *)&rcsp_sport_info_opt);
    return 0;
}

late_initcall(rcsp_sport_info_trans);

#endif /* if (TCFG_SPORT_HEALTH_ENABLE && JL_RCSP_SENSORS_DATA_OPT) */

