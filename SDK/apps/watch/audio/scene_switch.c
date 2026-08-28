#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".scene_switch.data.bss")
#pragma data_seg(".scene_switch.data")
#pragma const_seg(".scene_switch.text.const")
#pragma code_seg(".scene_switch.text")
#endif
#include "jlstream.h"
#include "effects/effects_adj.h"
#include "node_param_update.h"
#include "scene_switch.h"
#include "app_main.h"
#include "effects/audio_vocal_remove.h"

static u8 music_scene = 0; //记录音乐场景序号
static u8 music_eq_preset_index = 0; //记录 Eq0Media EQ配置序号
static u8 mic_scene = 0; //记录混响场景序号

/* 命名规则：节点名+模式名，如 SurBt、CrossAux、Eq0File */
#if defined(MEDIA_UNIFICATION_EN)&&MEDIA_UNIFICATION_EN
static char *music_mode[] = {"Media", "Media", "Media", "Media", "Media"};
#else
static char *music_mode[] = {"Bt", "Aux", "File", "Fm", "Spd"};
#endif
static char *sur_name[] = {"Sur"};
static char *crossover_name[] = {"Cross"};
static char *band_merge_name[] = {"Band"};
static char *bass_treble_name[] = {"Bass"};
static char *smix_name[] = {"Smix0", "Smix1"};
static char *eq_name[] = {"Eq0", "Eq1", "Eq2", "Eq3"};
static char *drc_name[] = {"Drc0", "Drc1", "Drc2", "Drc3", "Drc4"};
static char *vbass_name[] = {"VBass"};
static char *gain_name[] = {"Gain"};
static char *harmonic_exciter_name[] = {"Hexciter"};
static char *dy_eq_name[] = {"DyEq"};

/* 混响模块命名 */
static char *mic_name = "Eff";
static char *mic_bass_treble_name[] = {"BassTre"};
static char *mic_noisegate_name[] = {"NoiseGate"};
static char *mic_crossover_name[] = {"Crossover"};
static char *mic_band_merge_name[] = {"BMerge1", "BMerge2"};
static char *mic_howling_fs_name[] = {"Fshift"};
static char *mic_howling_supress_name[] = {"Hspress"};
static char *mic_voice_changer_name[] = {"Vchanger"};
static char *mic_autotune_name[] = {"Atune"};
static char *mic_plate_reverb_advance_name[] = {"PReverb"};
static char *mic_echo_name[] = {"Echo"};
static char *mic_eq_name[] = {"Eq1", "Eq2", "Eq3", "Eq4", "Eq5", "Eq6"};
static char *mic_drc_name[] = {"Drc1", "Drc2", "Drc3", "Drc4", "Drc5", "Drc6", "Drc7"};

/* 获取音乐模式场景序号 */
u8 get_current_scene()
{
    return music_scene;
}
/* 获取mic混响模式场景序号 */
u8 get_mic_current_scene()
{
    return mic_scene;
}

void set_default_scene(u8 index)
{
    music_scene = index;
}

/* 获EQ0 配置序号 */
u8 get_music_eq_preset_index(void)
{
    return music_eq_preset_index;
}
void set_music_eq_preset_index(u8 index)
{
    music_eq_preset_index = index;
    syscfg_write(CFG_EQ0_INDEX, &music_eq_preset_index, 1);
}

/* 获取当前处于的模式 */
u8 get_current_mode_index()
{
    struct app_mode *mode;
    mode = app_get_current_mode();
    switch (mode->name) {
    case APP_MODE_BT:
        return BT_MODE;
    case APP_MODE_LINEIN:
        return AUX_MODE;
    case APP_MODE_MUSIC:
        return FILE_MODE;
    case APP_MODE_FM:
        return FM_MODE;
    case APP_MODE_SPDIF:
        return SPDIF_MODE;
    case APP_MODE_PC:
        return PC_MODE;
    default:
        printf("mode not support scene switch %d\n", mode->name);
        return NOT_SUPPORT_MODE;
    }
}

/* 获取当前模式中场景个数 */
int get_mode_scene_num()
{
    struct app_mode *mode;
    mode = app_get_current_mode();
    u16 uuid;
    u8 scene_num;
    switch (mode->name) {
    case APP_MODE_BT:
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"a2dp");
        break;
    case APP_MODE_LINEIN:
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"linein");
        break;
    case APP_MODE_MUSIC:
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"music");
        break;
    case APP_MODE_FM:
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"fm");
        break;
    case APP_MODE_SPDIF:
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"spdif");
        break;
    case APP_MODE_PC:
        uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"pc_spk");
        break;
    default:
        printf("mode not support scene switch %d\n", mode->name);
        return -1;
    }
    jlstream_read_pipeline_param_group_num(uuid, &scene_num);
    return scene_num;
}

/* 音乐模式：根据参数组序号进行场景切换 */
void effect_scene_set(u8 scene)
{
    int scene_num = get_mode_scene_num();
    if (scene >= scene_num) {
        printf("err : without this scene %d\n", scene);
        return;
    }

    music_scene = scene;
    printf("current music scene : %d\n", scene);
    syscfg_write(CFG_SCENE_INDEX, &music_scene, 1);
    char tar_name[16];
    u8 cur_mode = get_current_mode_index();
    for (int i = 0; i < ARRAY_SIZE(sur_name); i++) {
        sprintf(tar_name, "%s%s", sur_name[i], music_mode[cur_mode]);
        surround_effect_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(crossover_name); i++) {
        sprintf(tar_name, "%s%s", crossover_name[i], music_mode[cur_mode]);
        crossover_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(band_merge_name); i++) {
        sprintf(tar_name, "%s%s", band_merge_name[i], music_mode[cur_mode]);
        band_merge_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(bass_treble_name); i++) {
        sprintf(tar_name, "%s%s", bass_treble_name[i], music_mode[cur_mode]);
        bass_treble_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(smix_name); i++) {
        sprintf(tar_name, "%s%s", smix_name[i], music_mode[cur_mode]);
        stereo_mix_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(eq_name); i++) {
        sprintf(tar_name, "%s%s", eq_name[i], music_mode[cur_mode]);
        if (i == 0) {
            eq_update_parm(scene, tar_name, music_eq_preset_index);
        } else {
            eq_update_parm(scene, tar_name, 0);
        }
    }
    for (int i = 0; i < ARRAY_SIZE(drc_name); i++) {
        sprintf(tar_name, "%s%s", drc_name[i], music_mode[cur_mode]);
        drc_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(vbass_name); i++) {
        sprintf(tar_name, "%s%s", vbass_name[i], music_mode[cur_mode]);
        virtual_bass_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(gain_name); i++) {
        sprintf(tar_name, "%s%s", gain_name[i], music_mode[cur_mode]);
        gain_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(harmonic_exciter_name); i++) {
        sprintf(tar_name, "%s%s", harmonic_exciter_name[i], music_mode[cur_mode]);
        harmonic_exciter_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(dy_eq_name); i++) {
        sprintf(tar_name, "%s%s", dy_eq_name[i], music_mode[cur_mode]);
        dynamic_eq_update_parm(scene, tar_name, 0);
    }
}

/* 音乐模式：根据参数组个数顺序切换场景 */
void effect_scene_switch()
{
    int scene_num = get_mode_scene_num();
    if (!scene_num) {
        puts("scene switch err\n");
        return;
    }
    music_scene++;
    if (music_scene >= scene_num) {
        music_scene = 0;
    }
    effect_scene_set(music_scene);
}

/* mic混响：获取场景个数 */
static u8 get_mic_effect_scene_num()
{
    u8 scene_num;
    u16 uuid = jlstream_event_notify(STREAM_EVENT_GET_PIPELINE_UUID, (int)"mic_effect");
    jlstream_read_pipeline_param_group_num(uuid, &scene_num);
    return scene_num;
}

/* mic混响：根据参数组序号进行场景切换 */
void mic_effect_scene_set(u8 scene)
{
    u8 scene_num = get_mic_effect_scene_num();
    if (scene >= scene_num) {
        printf("err : without this scene %d\n", scene);
        return;
    }
    mic_scene = scene;
    char tar_name[16];
    for (int i = 0; i < ARRAY_SIZE(mic_noisegate_name); i++) {
        sprintf(tar_name, "%s%s", mic_noisegate_name[i], mic_name);
        noisegate_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_crossover_name); i++) {
        sprintf(tar_name, "%s%s", mic_crossover_name[i], mic_name);
        crossover_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_band_merge_name); i++) {
        sprintf(tar_name, "%s%s", mic_band_merge_name[i], mic_name);
        band_merge_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_howling_fs_name); i++) {
        sprintf(tar_name, "%s%s", mic_howling_fs_name[i], mic_name);
        howling_frequency_shift_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_howling_supress_name); i++) {
        sprintf(tar_name, "%s%s", mic_howling_supress_name[i], mic_name);
        howling_suppress_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_voice_changer_name); i++) {
        sprintf(tar_name, "%s%s", mic_voice_changer_name[i], mic_name);
        voice_changer_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_autotune_name); i++) {
        sprintf(tar_name, "%s%s", mic_autotune_name[i], mic_name);
        autotune_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_plate_reverb_advance_name); i++) {
        sprintf(tar_name, "%s%s", mic_plate_reverb_advance_name[i], mic_name);
        reverb_advance_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_echo_name); i++) {
        sprintf(tar_name, "%s%s", mic_echo_name[i], mic_name);
        echo_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_eq_name); i++) {
        sprintf(tar_name, "%s%s", mic_eq_name[i], mic_name);
        eq_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_drc_name); i++) {
        sprintf(tar_name, "%s%s", mic_drc_name[i], mic_name);
        drc_update_parm(scene, tar_name, 0);
    }
    for (int i = 0; i < ARRAY_SIZE(mic_bass_treble_name); i++) {
        sprintf(tar_name, "%s%s", mic_bass_treble_name[i], mic_name);
        bass_treble_update_parm(scene, tar_name, 0);
    }
}

/* mic混响：根据参数组个数顺序切换场景 */
void mic_effect_scene_switch()
{
    int scene_num = get_mic_effect_scene_num();
    if (!scene_num) {
        puts("scene switch err\n");
        return;
    }
    mic_scene++;
    if (mic_scene >= scene_num) {
        mic_scene = 0;
    }
    mic_effect_scene_set(mic_scene);
}


static u8 vocla_remove_mark = 0xff;//
//实时更新media数据流中人声消除bypass参数,启停人声消除功能
void music_vocal_remover_switch(void)
{
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
    vocal_remover_param_tool_set cfg = {0};
    char *vocal_node_name = "VocalRemovMedia";
    int ret = jlstream_read_form_data(0, vocal_node_name, 0, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, vocal_node_name);
        return;
    }
    if (vocla_remove_mark == 0xff) {
        vocla_remove_mark = cfg.is_bypass;
    }
    vocla_remove_mark ^= 1;
    cfg.is_bypass = vocla_remove_mark;
    jlstream_set_node_param(NODE_UUID_VOCAL_REMOVER, vocal_node_name, &cfg, sizeof(cfg));
#endif
}
//media数据流启动后更新人声消除bypass参数
void music_vocal_remover_update_parm()
{
#if TCFG_VOCAL_REMOVER_NODE_ENABLE
    vocal_remover_param_tool_set cfg = {0};
    char *vocal_node_name = "VocalRemovMedia";
    int ret = jlstream_read_form_data(0, vocal_node_name, 0, &cfg);
    if (!ret) {
        printf("read parm err, %s, %s\n", __func__, vocal_node_name);
        return;
    }
    if (vocla_remove_mark == 0xff) {
        vocla_remove_mark = cfg.is_bypass;
    }
    cfg.is_bypass = vocla_remove_mark;
    jlstream_set_node_param(NODE_UUID_VOCAL_REMOVER, vocal_node_name, &cfg, sizeof(cfg));
#endif
}
u8 get_music_vocal_remover_statu(void)
{
    return vocla_remove_mark ;
}
