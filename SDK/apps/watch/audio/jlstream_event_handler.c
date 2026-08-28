#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".jlstream_event_handler.data.bss")
#pragma data_seg(".jlstream_event_handler.data")
#pragma const_seg(".jlstream_event_handler.text.const")
#pragma code_seg(".jlstream_event_handler.text")
#endif
#include "jlstream.h"
#include "overlay_code.h"
#include "media/audio_base.h"
#include "media/audio_general.h"
#include "clock_manager/clock_manager.h"
#include "a2dp_player.h"
#include "esco_player.h"
#include "app_tone.h"
#include "app_config.h"
#include "effects/effects_adj.h"
#include "audio_manager.h"
#include "file_player.h"
#include "linein_player.h"
#include "fm_player.h"
#include "spdif_player.h"
#include "pc_spk_player.h"
#include "app_main.h"
#include "classic/tws_api.h"
#include "call/call_common.h"

#define PIPELINE_UUID_TONE_NORMAL   0x7674
#define PIPELINE_UUID_A2DP          0xD96F
#define PIPELINE_UUID_ESCO          0xBA11
#define PIPELINE_UUID_LINEIN        0x1207
#define PIPELINE_UUID_SPDIF         0x96BE
#define PIPELINE_UUID_MUSIC         0x2A09
#define PIPELINE_UUID_FM            0x05FB
#define PIPELINE_UUID_MIC_EFFECT    0x9C2D
#define PIPELINE_UUID_MEDIA         0x1408
#define PIPELINE_UUID_A2DP_DUT      0xC9DB
#define PIPELINE_UUID_PC_AUDIO		0xDC8D
#define PIPELINE_UUID_RECODER       0x49EC
#define PIPELINE_UUID_AI_VOICE      0x5475
#define PIPELINE_UUID_INTERCOM      0x26
#define PIPELINE_UUID_AVI_VOICE      0x10BB
#define PIPELINE_UUID_VIDEO         0x5c51


#if TCFG_A2DP_PREEMPTED_ENABLE
const int CONFIG_A2DP_ENERGY_CALC_ENABLE = 0;
#else
const int CONFIG_A2DP_ENERGY_CALC_ENABLE = 1;
#endif

extern int  get_mix_recorder_status(void);

static u8 g_a2dp_slience;
static u32 g_a2dp_slience_begin;

static void a2dp_energy_detect_handler(int *arg)
{
    // cppcheck-suppress knownConditionTrueFalse
    if (CONFIG_A2DP_ENERGY_CALC_ENABLE == 0)  {
        return;
    }
    int energy = arg[0];
    if (energy == 0) {
        if (g_a2dp_slience_begin == 0) {
            g_a2dp_slience_begin = jiffies_msec();
        } else {
            int msec = jiffies_msec2offset(g_a2dp_slience_begin, jiffies_msec());
            if (msec >= 2000 && g_a2dp_slience == 0) {
                g_a2dp_slience = 1;
                audio_event_to_user(AUDIO_EVENT_A2DP_NO_ENERGY);
            }
        }
    } else {
        g_a2dp_slience = 0;
        g_a2dp_slience_begin = 0;
    }
}


int get_system_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_TONE_NORMAL, par);
}

int get_media_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_MEDIA, par);
}

int get_esco_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_ESCO, par);
}

int get_mic_eff_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_MIC_EFFECT, par);
}

int get_usb_audio_stream_bit_width(void *par)
{
    return jlstream_read_bit_width(PIPELINE_UUID_PC_AUDIO, par);
}


static int get_pipeline_uuid(const char *name)
{
#if defined(MEDIA_UNIFICATION_EN)&&MEDIA_UNIFICATION_EN
    if (!strcmp(name, "tone")) {
        return PIPELINE_UUID_TONE_NORMAL;
    }

    if (!strcmp(name, "ring")) {
        clock_alloc("esco", 48 * 1000000UL);
        return PIPELINE_UUID_TONE_NORMAL;
    }
    if (!strcmp(name, "esco")) {
        clock_alloc("esco", 48 * 1000000UL);
        return PIPELINE_UUID_ESCO;
    }

    if (!strcmp(name, "mic_effect")) {
        clock_alloc("mic_effect", 24 * 1000000UL);
        return PIPELINE_UUID_MIC_EFFECT;
    }

    if (!strcmp(name, "pc_spk")) {
        clock_alloc("pc_spk", 96 * 1000000UL);
        return PIPELINE_UUID_PC_AUDIO;
    }
    if (!strcmp(name, "pc_mic")) {
        clock_alloc("pc_mic", 96 * 1000000UL);
        return PIPELINE_UUID_PC_AUDIO;
    }
    if (!strcmp(name, "mix_recorder")) {
        return PIPELINE_UUID_RECODER;
    }

    if (!strcmp(name, "ai_voice")) {
        /* clock_alloc("a2dp", 24 * 1000000UL); */
        return PIPELINE_UUID_AI_VOICE;
    }
    if (!strcmp(name, "intercom")) {
        return PIPELINE_UUID_INTERCOM;
    }
    if (!strcmp(name, "avi_voice")) {
        /* clock_alloc("a2dp", 24 * 1000000UL); */
        return PIPELINE_UUID_AVI_VOICE;
    }
    if (!strcmp(name, "video_voice")) {
        /* clock_alloc("a2dp", 24 * 1000000UL); */
        return PIPELINE_UUID_VIDEO;
    }
    if (!strcmp(name, "a2dp")) {
        clock_alloc("a2dp", 24 * 1000000UL);
#if TCFG_AUDIO_DUT_ENABLE
        if (audio_dec_dut_en_get(0)) {
            return PIPELINE_UUID_A2DP_DUT;
        }
#endif
    }
    if (!strcmp(name, "music")) {
        clock_alloc("music", 24 * 1000000UL);
    }
    if (!strcmp(name, "linein")) {
        //此处设置时钟不低于120M是由于切时钟会停止cpu,多次切会导致DAC缓存少于1ms
        clock_alloc("linein", 120 * 1000000UL);
    }
    if (!strcmp(name, "fm")) {
        clock_alloc("fm", 48 * 1000000UL);
    }

    if (!strcmp(name, "iis")) {
        clock_alloc("iis", 48 * 1000000UL);
    }



    return PIPELINE_UUID_MEDIA;
#else
    if (!strcmp(name, "tone")) {
        if (a2dp_player_runing()) {
            return PIPELINE_UUID_A2DP;
        }
        if (esco_player_runing()) {
            return PIPELINE_UUID_ESCO;
        }
        if (ring_player_runing()) {
            return PIPELINE_UUID_ESCO;
        }
        if (music_player_runing()) {
            return PIPELINE_UUID_MUSIC;
        }
        if (linein_player_runing()) {
            return PIPELINE_UUID_LINEIN;
        }
        return PIPELINE_UUID_TONE_NORMAL;
    }

    if (!strcmp(name, "ring")) {
        clock_alloc("esco", 48 * 1000000UL);
        return PIPELINE_UUID_ESCO;
    }
    if (!strcmp(name, "esco")) {
        clock_alloc("esco", 48 * 1000000UL);
        return PIPELINE_UUID_ESCO;
    }

    if (!strcmp(name, "a2dp")) {
        clock_alloc("a2dp", 24 * 1000000UL);
        return PIPELINE_UUID_A2DP;
    }
    if (!strcmp(name, "music")) {
        clock_alloc("music", 24 * 1000000UL);
        return PIPELINE_UUID_MUSIC;
    }


    if (!strcmp(name, "linein")) {
        struct app_mode *mode = app_get_current_mode();
        if (mode && mode->name == APP_MODE_BT) {
            clock_alloc("a2dp", 24 * 1000000UL);
            return PIPELINE_UUID_A2DP;
        } else {
            clock_alloc("linein", 24 * 1000000UL);
            return PIPELINE_UUID_LINEIN;
        }
    }

    if (!strcmp(name, "mix_recorder")) {
        return PIPELINE_UUID_RECODER;
    }
    if (!strcmp(name, "spdif")) {
        //spdif 192k采样率输入，同步节点硬件SRC，时钟至少为160MHz
        clock_alloc("spdif", 160 * 1000000UL);
        return PIPELINE_UUID_SPDIF;
    }

    if (!strcmp(name, "fm")) {
        clock_alloc("fm", 48 * 1000000UL);
        return PIPELINE_UUID_FM;
    }

    if (!strcmp(name, "mic_effect")) {
        clock_alloc("mic_effect", 24 * 1000000UL);
        return PIPELINE_UUID_MIC_EFFECT;
    }
    if (!strcmp(name, "a2dp_dut")) {
        clock_alloc("a2dp", 24 * 1000000UL);
        return PIPELINE_UUID_A2DP_DUT;
    }

    if (!strcmp(name, "ai_voice")) {
        /* clock_alloc("a2dp", 24 * 1000000UL); */
        return PIPELINE_UUID_AI_VOICE;
    }
#endif
    return 0;
}

static void player_close_handler(const char *name)
{
    if (!strcmp(name, "a2dp") || !strcmp(name, "a2dp_dut")) {
        clock_free("a2dp");
    } else if (!strcmp(name, "esco") || !strcmp(name, "ring")) {
        clock_free("esco");
    } else if (!strcmp(name, "linein")) {
        clock_free("linein");
    } else if (!strcmp(name, "music")) {
        clock_free("music");
    } else if (!strcmp(name, "mic_effect")) {
        clock_free("mic_effect");
    } else if (!strcmp(name, "fm")) {
        clock_free("fm");
    } else if (!strcmp(name, "pc_spk")) {
        clock_free("pc_spk");
    } else if (!strcmp(name, "pc_mic")) {
        clock_free("pc_mic");
    }
    if (!strcmp(name, "iis")) {
        clock_free("iis");
    }

}

#if TCFG_CODE_RUN_RAM_AAC_CODE
static void *aac_code_run_addr = NULL;
extern u32 __aac_movable_slot_start[];
extern u32 __aac_movable_slot_end[];
extern u8 __aac_movable_region_start[];
extern u8 __aac_movable_region_end[];
#endif

#if TCFG_CODE_RUN_RAM_AEC_CODE
static void *aec_code_run_addr = NULL;
extern u32 __aec_movable_slot_start[];
extern u32 __aec_movable_slot_end[];
extern u8 __aec_movable_region_start[];
extern u8 __aec_movable_region_end[];
#endif

void aac_code_movable_load(void)
{
#if TCFG_CODE_RUN_RAM_AAC_CODE
    int aac_code_size = __aac_movable_region_end - __aac_movable_region_start;
    mem_stats();
    if (aac_code_size && !aac_code_run_addr) {
        aac_code_run_addr = phy_malloc(aac_code_size);
    }

    if (!aac_code_run_addr) {
        return;
    }
    code_movable_load(__aac_movable_region_start, aac_code_size, aac_code_run_addr, __aac_movable_slot_start, __aac_movable_slot_end);
    printf("aac code load addr : 0x%x, size : %d\n", (u32)aac_code_run_addr, aac_code_size);
    mem_stats();
#endif
}

void aac_code_movable_unload(void)
{
#if TCFG_CODE_RUN_RAM_AAC_CODE
    if (aac_code_run_addr) {
        mem_stats();
        code_movable_unload(__aac_movable_region_start, __aac_movable_slot_start, __aac_movable_slot_end);
        phy_free(aac_code_run_addr);
        aac_code_run_addr = NULL;
        printf("aac code unload\n");
        mem_stats();
    }
#endif

}

void aec_code_movable_load(void)
{
#if TCFG_CODE_RUN_RAM_AEC_CODE
    int aec_code_size = __aec_movable_region_end - __aec_movable_region_start;
    mem_stats();
    if (aec_code_size && !aec_code_run_addr) {
        aec_code_run_addr = phy_malloc(aec_code_size);
    }

    if (!aec_code_run_addr) {
        return;
    }
    code_movable_load(__aec_movable_region_start, aec_code_size, aec_code_run_addr, __aec_movable_slot_start, __aec_movable_slot_end);
    printf("aec code load addr : 0x%x, size : %d\n", (u32)aec_code_run_addr, aec_code_size);
    mem_stats();
#endif
}

void aec_code_movable_unload(void)
{
#if TCFG_CODE_RUN_RAM_AEC_CODE
    if (aec_code_run_addr) {
        mem_stats();
        code_movable_unload(__aec_movable_region_start, __aec_movable_slot_start, __aec_movable_slot_end);
        phy_free(aec_code_run_addr);
        aec_code_run_addr = NULL;
        printf("aec code unload\n");
        mem_stats();
    }
#endif
}

static int load_decoder_handler(struct stream_decoder_info *info)
{
    if (info->coding_type == AUDIO_CODING_AAC) {
        /*printf("overlay_lode_code: aac\n");*/
        aac_code_movable_load();
    }

    if (info->scene == STREAM_SCENE_A2DP) {
        g_a2dp_slience = 0;
        g_a2dp_slience_begin = 0;
        info->task_name = "a2dp_dec";
    }

    if (info->scene == STREAM_SCENE_ESCO) {	//AEC PLC共用overlay
    }
    if (info->scene == STREAM_SCENE_MUSIC) {
        info->task_name = "file_dec";
    }

    return 0;
}

static void unload_decoder_handler(u32 coding_type)
{
    if (coding_type == AUDIO_CODING_AAC) {
        aac_code_movable_unload();
    }
}

static int load_encoder_handler(struct stream_encoder_info *info)
{
    if (info->scene == STREAM_SCENE_ESCO) {
        //AEC overlay归节点自己管理, 不依赖编码
        /* printf("overlay_lode_code: aec\n"); */
        /* overlay_load_code(OVERLAY_AEC); */
        /* aec_code_movable_load(); */
    }

    return 0;
}

static void unload_encoder_handler(struct stream_encoder_info *info)
{
    if (info->scene == STREAM_SCENE_ESCO) {
        /* aec_code_movable_unload(); */
    }
}

/*
 *获取需要指定得默认配置
 * */
static int get_node_parm(int arg)
{
    int ret = 0;
    ret = get_eff_default_param(arg);
    return ret ;
}
/*
*获取ram内在线音效参数
*/
static int get_eff_online_parm(int arg)
{
    int ret = 0;
#if TCFG_CFG_TOOL_ENABLE
    ASSERT(arg);
    struct eff_parm {
        int uuid;
        char name[16];
        u8 data[0];
    };
    struct eff_parm *parm = (struct eff_parm *)arg;
    /* printf("eff_online_uuid %x, %s\n", parm->uuid, parm->name); */
    ret = get_eff_online_param(parm->uuid, parm->name, (void *)arg);
#endif
    return ret;
}

static int tws_switch_get_status()
{
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_DISCONNECTED) {
        return 0;
    }
    return 1;
}

static int a2dp_switch_get_status()
{
#if TCFG_USER_EMITTER_ENABLE
    extern u8 *get_cur_connect_emitter_mac_addr(void);
    void *bt_addr = get_cur_connect_emitter_mac_addr();
    if (bt_addr) {
        return 1;
    }
#endif
    return 0;
}

static int dac_switch_get_status()
{
    if (!a2dp_switch_get_status()) {
        return 1;
    }
    return 0;
}


static int get_switch_node_callback(const char *arg)
{
    if (!strcmp(arg, "TWS_Switch")) {
        return (int)tws_switch_get_status;
    }
    if (!strcmp(arg, "Switch_a2dp")) {
        return (int)a2dp_switch_get_status;
    }
    if (!strcmp(arg, "Switch_dac")) {
        return (int)dac_switch_get_status;
    }
#if TCFG_MIX_RECORD_ENABLE
    if (!strncmp(arg, "SW_Rec", strlen("SW_Rec"))) {
        return (int)get_mix_recorder_status;
    }
#endif
    return 0;
}

static int jlstream_get_cvp_mode(void)
{
#if TCFG_SIRI_MODE_AEC_BYPASS /*SIRI模式省ram方式*/
    if (call_ctrl_get_status() != BT_SIRI_STATE) {
        return 0;
    }
    return 1;
#else
    return 0;
#endif
}

int jlstream_event_notify(enum stream_event event, int arg)
{
    int ret = 0;

    switch (event) {
    case STREAM_EVENT_LOAD_DECODER:
        ret = load_decoder_handler((struct stream_decoder_info *)arg);
        break;
    case STREAM_EVENT_UNLOAD_DECODER:
        unload_decoder_handler((u32)arg);
        break;
    case STREAM_EVENT_LOAD_ENCODER:
        ret = load_encoder_handler((struct stream_encoder_info *)arg);
        break;
    case STREAM_EVENT_UNLOAD_ENCODER:
        unload_encoder_handler((struct stream_encoder_info *)arg);
        break;
    case STREAM_EVENT_GET_PIPELINE_UUID:
        ret = get_pipeline_uuid((const char *)arg);
        r_printf("pipeline_uuid: %x\n", ret);
        clock_refurbish();
        break;
    case STREAM_EVENT_CLOSE_PLAYER:
        player_close_handler((const char *)arg);
        break;
    case STREAM_EVENT_INC_SYS_CLOCK:
        clock_refurbish();
        break;
    case STREAM_EVENT_GET_NODE_PARM:
        ret = get_node_parm(arg);
        break;
    case STREAM_EVENT_GET_EFF_ONLINE_PARM:
        ret = get_eff_online_parm(arg);
        break;
    case STREAM_EVENT_A2DP_ENERGY:
        a2dp_energy_detect_handler((int *)arg);
        break;
#if TCFG_SWITCH_NODE_ENABLE
    case STREAM_EVENT_GET_SWITCH_CALLBACK:
        ret = get_switch_node_callback((const char *)arg);
        break;
#endif
#if TCFG_SIRI_MODE_AEC_BYPASS
    case STREAM_EVENT_GET_CVP_MODE:
        ret = jlstream_get_cvp_mode();
        break;
#endif
    default:
        break;
    }

    return ret;
}
