
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "sdk_config.h"
#include "effects/eq_config.h"
#include "effects/effects_adj.h"
#include "audio_config.h"
#include "jlstream.h"

#define LOG_TAG_CONST EFFECTS
#define LOG_TAG     "[EFFECTS_ADJ]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

#if TCFG_CFG_TOOL_ENABLE

#if defined(TCFG_CFG_USER_EFF_ADJUST_ENABLE)&& TCFG_CFG_USER_EFF_ADJUST_ENABLE
void user_eq_online_update(void *par, unsigned int len)
{
    char name[16];
    memcpy(name, par, 16);
    log_debug("eq_online_name:%s\n", par);
    char *addr = (char *)par;
    int *data = (int *)&addr[16];
    if (data[0] == EQ_SPECIAL_CMD) { //type bypass global_gain  sen_num
        int is_bypass = data[1];
        float global_gain;
        memcpy(&global_gain, &data[2], 4);//总增益是浮点
        int seg_num = data[3];
        log_debug("is_bypass %d\n", is_bypass);

        //cppcheck-suppress  invalidPointerCast
        log_debug("global_gain 0x%x\n", *(int *)&global_gain);
        log_debug("seg_num %d\n", seg_num);

        u8 nsection = eq_get_table_nsection(EQ_MODE_CUSTOM);
        if (seg_num > nsection)	{
            //用户段数比默认的10段更大，需将eq_config.c文件内eq的自定义表调到对应的段数
            //(自定义表指的是：struct eq_seg_info eq_tab_custom[])
            log_error("user_eq section %d > %d error\n", seg_num, nsection);
        }
        cur_eq_set_bypass(is_bypass);

        set_global_gain(EQ_MODE_CUSTOM, global_gain);
    } else if (data[0] == EQ_SPECIAL_SEG_CMD) { //type seg[n]
        struct eq_seg_info *src_seg = (struct eq_seg_info *)&data[1];
        int tar_size = len - 16; // uuid name
        u8 nsection = eq_get_table_nsection(EQ_MODE_CUSTOM);
        for (int i = 0; i < nsection; i++) {
            if (tar_size >= sizeof(struct eq_seg_info)) {
                if (src_seg[i].index < nsection) {
                    struct eq_seg_info *seg = eq_mode_get_seg(EQ_MODE_CUSTOM, src_seg[i].index);//获取某段eq系数地址
                    memcpy(seg, &src_seg[i], sizeof(struct eq_seg_info));
                    log_debug("multi soure idx:%d, iir:%d, freq:%d, gain:0x%08x, q:0x%08x ", seg->index, seg->iir_type, seg->freq, *(int *)&seg->gain, *(int *)&seg->q);
                } else {
                    log_error("cur eq idx error %d, %d\n", src_seg[i].index, nsection);
                }

                tar_size -= sizeof(struct eq_seg_info);
            }
        }
        eq_mode_set(EQ_MODE_CUSTOM);//设置更新系数
    }
}

REGISTER_USER_EFF_ONLINE_ADJUST_TARGET(user_eq) = {
    .uuid = NODE_UUID_EQ,
    .eff_handler = user_eq_online_update,
};



#endif
#endif
