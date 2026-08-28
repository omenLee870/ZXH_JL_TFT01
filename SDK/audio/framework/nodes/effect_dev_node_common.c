#include "jlstream.h"
#include "media/audio_base.h"
#include "effects/effects_adj.h"
#include "app_config.h"
#include "effect_dev_node.h"

#if (TCFG_EFFECT_DEV0_NODE_ENABLE || \
          TCFG_EFFECT_DEV1_NODE_ENABLE || \
          TCFG_EFFECT_DEV2_NODE_ENABLE || \
          TCFG_EFFECT_DEV3_NODE_ENABLE || \
          TCFG_EFFECT_DEV4_NODE_ENABLE)

void user_effect_tool_param_cfg_printf(struct user_effect_tool_param *cfg)
{
    if (!cfg) {
        return;
    }
    printf("int param: \n");
    for (int i = 0; i < ARRAY_SIZE(cfg->int_param); i++) {
        printf("int[%d] %d\n", i, cfg->int_param[i]);
    }
    if (cfg->ext_int_param_len && cfg->ext_int_param) {
        for (int i = 0; i < (cfg->ext_int_param_len / sizeof(int)); i++) {
            printf("ext int[%d] %d\n", i, cfg->ext_int_param[i]);
        }
    }
    printf("float param: \n");
    for (int i = 0; i < ARRAY_SIZE(cfg->float_param); i++) {
        printf("float[%d] %d.%d\n", i, (int)cfg->float_param[i], debug_digital(cfg->float_param[i]));
    }
    if (cfg->ext_float_param_len && cfg->ext_float_param) {
        for (int i = 0; i < (cfg->ext_float_param_len / sizeof(float)); i++) {
            printf("ext float[%d] %d.%d\n", i, (int)cfg->ext_float_param[i], debug_digital(cfg->ext_float_param[i]));
        }
    }
}

void user_effect_tool_param_cfg_get(struct user_effect_tool_param *cfg, u8 *data_buf)
{
    //int
    memcpy(cfg->int_param, data_buf, sizeof(cfg->int_param));
    data_buf += sizeof(cfg->int_param);
    u16 ext_int_len = 0;
    memcpy(&ext_int_len, data_buf, sizeof(u16));
    data_buf += 2;
    if (cfg->ext_int_param_len != ext_int_len) {
        if (cfg->ext_int_param) {
            free(cfg->ext_int_param);
            cfg->ext_int_param = NULL;
        }
        cfg->ext_int_param_len = ext_int_len;
    }
    if (!cfg->ext_int_param && ext_int_len) {
        cfg->ext_int_param = zalloc(ext_int_len);
        ASSERT(cfg->ext_int_param);
    }
    if (ext_int_len) {
        memcpy(cfg->ext_int_param, data_buf, ext_int_len);
        data_buf += ext_int_len;
    }
    //float
    memcpy(cfg->float_param, data_buf, sizeof(cfg->float_param));
    data_buf += sizeof(cfg->float_param);
    u16 ext_float_len = 0;
    memcpy(&ext_float_len, data_buf, sizeof(u16));
    data_buf += 2;
    if (cfg->ext_float_param_len != ext_float_len) {
        if (cfg->ext_float_param) {
            free(cfg->ext_float_param);
            cfg->ext_float_param = NULL;
        }
        cfg->ext_float_param_len = ext_float_len;
    }
    if (!cfg->ext_float_param && ext_float_len) {
        cfg->ext_float_param = zalloc(ext_float_len);
        ASSERT(cfg->ext_float_param);
    }
    if (ext_float_len) {
        memcpy(cfg->ext_float_param, data_buf, ext_float_len);
        data_buf += ext_float_len;
    }
}

void effect_dev_file_param_read(struct effect_dev_node_hdl *hdl)
{
    /*
     *获取配置文件内的参数,及名字
     * */
    jlstream_read_node_data(hdl_node(hdl)->uuid, hdl_node(hdl)->subid, (u8 *)hdl->name);

    struct eff_default_parm get_eff_parm = {0};
    memcpy(get_eff_parm.name, hdl->name, sizeof(hdl->name));
    jlstream_event_notify(STREAM_EVENT_GET_NODE_PARM, (int)&get_eff_parm);

    struct cfg_info info = {0};
    if (!jlstream_read_form_node_info_base(get_eff_parm.mode_index, hdl->name, get_eff_parm.cfg_index, &info)) {
        u8 *data_buf = zalloc(info.size);
        if (data_buf) {
            jlstream_read_form_cfg_data(&info, data_buf);
            printf("%s read file cfg\n", __func__);
            user_effect_tool_param_cfg_get(&hdl->cfg, data_buf);
            user_effect_tool_param_cfg_printf(&hdl->cfg);
            free(data_buf);
        }
    }

    /*
     *获取在线调试的临时参数
     * */
    if (config_audio_cfg_online_enable) {
        u16 ext_int_len = 0;
        u16 ext_float_len = 0;

        //获取ext_int_len
        u32 online_param_size = sizeof(int) * 8 + sizeof(u16);
        u8 *online_cfg = zalloc(online_param_size);
        ASSERT(online_cfg);
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, online_cfg, online_param_size)) {
            //存储在最后2byte
            memcpy(&ext_int_len, online_cfg + (online_param_size - sizeof(u16)), sizeof(u16));
        }
        free(online_cfg);
        online_cfg = NULL;

        //获取ext_float_len
        online_param_size += ext_int_len + sizeof(float) * 8 + sizeof(u16);
        online_cfg = zalloc(online_param_size);
        ASSERT(online_cfg);
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, online_cfg, online_param_size)) {
            //存储在最后2byte
            memcpy(&ext_float_len, online_cfg + (online_param_size - sizeof(u16)), sizeof(u16));
        }
        free(online_cfg);
        online_cfg = NULL;

        //获取完整大小的数据
        printf("%s read online cfg, ext int_len %d, float_len %d\n", __func__, ext_int_len, ext_float_len);
        online_param_size += ext_float_len;
        online_cfg = zalloc(online_param_size);
        ASSERT(online_cfg);
        if (jlstream_read_effects_online_param(hdl_node(hdl)->uuid, hdl->name, online_cfg, online_param_size)) {
            user_effect_tool_param_cfg_get(&hdl->cfg, online_cfg);
            user_effect_tool_param_cfg_printf(&hdl->cfg);
        }
        free(online_cfg);
    }
}


#endif
