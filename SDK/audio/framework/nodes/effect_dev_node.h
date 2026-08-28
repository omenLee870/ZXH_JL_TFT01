#ifndef __EFFECT_DEV_NODE__H
#define __EFFECT_DEV_NODE__H


#include "system/includes.h"
#include "config/config_interface.h"
// #include "crc.h"
#include "fs/resfile.h"
#include "effects/AudioEffect_DataType.h"
#include "media/framework/include/jlstream.h"
#include "media_memory.h"
#include "effects_dev.h"

struct user_effect_tool_param {
    int   int_param[8];
    u16 ext_int_param_len;
    int *ext_int_param;
    float float_param[8];
    u16 ext_float_param_len;
    float *ext_float_param;
};

struct effect_dev_node_hdl {
    char name[16];
    void *user_priv;  //用户可使用该变量做模块指针传递
    struct user_effect_tool_param cfg;//工具界面参数
    struct packet_ctrl dev;
};

#define debug_digital(x)  __builtin_abs((int)((x - (int)x) * 100)) //此处使用该宏定义，获取浮点两位小数，转int型后打印

void user_effect_tool_param_cfg_printf(struct user_effect_tool_param *cfg);
void user_effect_tool_param_cfg_get(struct user_effect_tool_param *cfg, u8 *data_buf);
void effect_dev_file_param_read(struct effect_dev_node_hdl *hdl);

#endif
