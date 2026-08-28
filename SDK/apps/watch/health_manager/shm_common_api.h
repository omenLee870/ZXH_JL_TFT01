#ifndef __SHM_COMMON_API__
#define __SHM_COMMON_API__


/* ------------------------------------------------------------------------------------*/
/**
 * @TYPE 			活动量
 */
/* ------------------------------------------------------------------------------------*/
//获取活动量数据
int sport_health_get_value_daily_active(struct daily_active *data);
//活动量--步数
int sport_health_get_value_daily_steps();
//活动量--热量
int sport_health_get_value_daily_calories();
//活动量--距离
int sport_health_get_value_daily_distance();

//设置活动量目标
int sport_health_set_daily_active_target(struct daily_active *data);
//获取活动量目标
int sport_health_get_value_daily_active_target(struct daily_active *data);
//活动量目标--步数
int sport_health_get_value_daily_target_steps();
//活动量目标--热量
int sport_health_get_value_daily_target_calories();
//活动量目标--距离
int sport_health_get_value_daily_target_distance();
/* ------------------------------------------------------------------------------------*/
/**
 * @TYPE 			运动
 */
/* ------------------------------------------------------------------------------------*/
//设置运动类型
int sport_health_set_sport_type(int type);
//开启运动附带运动类型
int sport_health_ctrl_sport_start_with_type(int type);
//开启运动
int sport_health_ctrl_sport_start();
//暂停运动
int sport_health_ctrl_sport_pause();
//继续运动
int sport_health_ctrl_sport_continue();
//结束运动
int sport_health_ctrl_sport_stop();

//获取运动时的信息
int sport_health_get_sport_info(struct sport_value *data);
//运动--状态：开始、暂停、继续、结束
int sport_health_get_sport_status();
//运动--运动类型
int sport_health_get_sport_type();
//运动--步数
int sport_health_get_sport_steps();
//运动--热量
int sport_health_get_sport_calories();
//运动--距离
int sport_health_get_sport_distance();
//运动--步频
int sport_health_get_sport_freq();
//运动--时长（不含暂停时间）
int sport_health_get_sport_time();
//运动--步幅
int sport_health_get_sport_step_stride();
//运动--速度
int sport_health_get_sport_speed();
//运动--心率均值
int sport_health_get_sport_hr_arg();
//运动--心率最小值
int sport_health_get_sport_hr_min();
//运动--心率最大值
int sport_health_get_sport_hr_max();
//运动--心率实时值
int sport_health_get_sport_hr_real();
//运动类型转换
int sport_type_map_get_type(u8 sport_mode, u8 list_index);
//运动类型所需的数据配置
u32 sport_type_map_get_data_cfg(u8 sport_type);
//运动文件数量
int sport_health_get_sport_file_total();
//运动文件数据
int sport_health_get_sport_file_value(struct sport_value *value);
//运动文件id（new）
int sport_health_get_sport_file_id();
//运动文件大小（new）
int sport_health_get_sport_file_size();
//更新算法配置
int sport_health_gsensor_algo_cfg_update();
/* ------------------------------------------------------------------------------------*/
/**
 * @TYPE 			心率
 */
/* ------------------------------------------------------------------------------------*/
//开启心率测量
int sport_health_heart_rate_module_enable();
//关闭心率测量
int sport_health_heart_rate_module_disable();
//清除心率
int sport_health_heart_rate_module_clr();
//心率--获取当前值
int sport_health_heart_rate_get_cur();
//心率--获取最小值
int sport_health_heart_rate_get_min();
//心率--获取最大值
int sport_health_heart_rate_get_max();
//心率--获取平均值
int sport_health_heart_rate_get_avg();
//心率--获取心率文件数据
int sport_health_heart_rate_get_rec(struct health_file_data_info *info);
/* ------------------------------------------------------------------------------------*/
/**
 * @TYPE 			血氧
 */
/* ------------------------------------------------------------------------------------*/
//开启血氧测量
int sport_health_blood_oxygen_module_enable();
//关闭血氧测量
int sport_health_blood_oxygen_module_disable();
//清除血氧
int sport_health_blood_oxygen_module_clr();
//血氧--获取当前值
int sport_health_blood_oxygen_get_cur();
//血氧--获取最小值
int sport_health_blood_oxygen_get_min();
//血氧--获取最大值
int sport_health_blood_oxygen_get_max();
//血氧--获取平均值
int sport_health_blood_oxygen_get_avg();
//血氧--获取血氧文件数据
int sport_health_blood_oxygen_get_rec(struct health_file_data_info *info);
/* ------------------------------------------------------------------------------------*/
/**
 * @TYPE 			睡眠
 */
/* ------------------------------------------------------------------------------------*/
//获取睡眠数据
int sport_health_sleep_data_get(struct sleep_data_analysis *info, int analysis_en, struct sys_time *time);
/* ------------------------------------------------------------------------------------*/
/**
 * @TYPE 			其他
 */
/* ------------------------------------------------------------------------------------*/
void sport_health_common_swapX(const uint8_t *src, uint8_t *dst, int32_t len);




#endif// __SHM_COMMON_API__
