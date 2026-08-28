//算法调试
const unsigned char jl_motion_debug_enable  = 1; //调试开关，0为关闭，1为打开
const unsigned char jl_motion_debug_acceler = 0; //打印加速度原始数据

//算法开关，关闭部分算法可以节省内存，1为打开，0为关闭
const unsigned char jl_motion_step_counter     = 1; //计步器
const unsigned char jl_motion_distance         = 1; //距离
const unsigned char jl_motion_step_frequency   = 1; //步频
const unsigned char jl_motion_calories         = 1; //卡路里 = 基础代谢率（BMR） + 活动代谢率（AMR）
const unsigned char jl_motion_calories_amr     = 1; //活动代谢率（AMR）：活动代谢率是指在运动和日常活动中所消耗的能量
const unsigned char jl_motion_sedentary        = 1; //久坐提醒
const unsigned char jl_motion_sleep            = 1; //睡眠
const unsigned char jl_motion_shake            = 1; //摇一摇

//调整计步器的参数
const unsigned char jl_motion_brush_teeth      = 0; //过滤刷牙产生的计步
const unsigned char jl_motion_step_filter      = 50;//过滤日常活动产生的计步

//睡眠算法参数
const unsigned char jl_motion_sleep_conditional_end        = 0; //通过某些条件结束睡眠
const unsigned char jl_motion_sleep_wake_refer_to_samsung  = 1;   //清醒状态参照三星
