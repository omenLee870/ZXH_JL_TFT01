#include "sdk_config.h"
#include "includes.h"

#if CONFIG_UART_DEBUG_ENABLE
const char libs_debug AT(.LOG_TAG_CONST) = TRUE; //打印总开关
#else
const char libs_debug AT(.LOG_TAG_CONST) = FALSE; //打印总开关
#endif

#define  CONFIG_DEBUG_LIBS(X)   (X & libs_debug)


const int config_asser_log = libs_debug;
const int config_exception_log = libs_debug;



const char log_tag_const_i_MAIN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_MAIN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_MAIN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_MAIN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_MAIN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_MAIN AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_UBOOT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_d_UBOOT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_UBOOT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_v_UBOOT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_UBOOT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_UBOOT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);

const char log_tag_const_i_UPGRADE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_d_UPGRADE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_UPGRADE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_v_UPGRADE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_UPGRADE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_UPGRADE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);

const char log_tag_const_i_FLASH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_d_FLASH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_FLASH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_v_FLASH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_FLASH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_FLASH AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);

const char log_tag_const_i_FS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_d_FS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_FS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_v_FS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_FS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_FS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);

const char log_tag_const_i_SFC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_d_SFC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_SFC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_v_SFC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_SFC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_SFC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);

const char log_tag_const_i_PMU AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_PMU AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_PMU AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_PMU AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_PMU AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_PMU AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_LP_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_LP_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_LP_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_LP_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_LP_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_LP_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_TIMER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_CLOCK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_CLOCK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_CLOCK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_CLOCK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_CLOCK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_CLOCK AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_EXCEPTION AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_EXCEPTION AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_EXCEPTION AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_EXCEPTION AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_EXCEPTION AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_EXCEPTION AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_BT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_BT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_BT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_v_BT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_BT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_BT AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_BATTERY_PMC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_BATTERY_PMC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_BATTERY_PMC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_v_BATTERY_PMC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_BATTERY_PMC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_c_BATTERY_PMC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);

const char log_tag_const_i_SENSOR_DRIVER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_SENSOR_DRIVER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
/* warn 默认关闭：本 tag 下仅 vc30fx FIFO 空读等轮询中间态，无需默认输出。
 * 注意 0 & libs_debug 恒为 0，改回 CONFIG_DEBUG_LIBS(1) 需重新编译才生效。 */
const char log_tag_const_w_SENSOR_DRIVER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_e_SENSOR_DRIVER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_SENSOR_HAL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_SENSOR_HAL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_SENSOR_HAL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_e_SENSOR_HAL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_SENSOR_SERVICE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_SENSOR_SERVICE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_SENSOR_SERVICE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_e_SENSOR_SERVICE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_SENSOR_MODULE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_SENSOR_MODULE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_SENSOR_MODULE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_e_SENSOR_MODULE AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

const char log_tag_const_i_SENSOR_MANAGER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_d_SENSOR_MANAGER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(0);
const char log_tag_const_w_SENSOR_MANAGER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);
const char log_tag_const_e_SENSOR_MANAGER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIBS(1);

