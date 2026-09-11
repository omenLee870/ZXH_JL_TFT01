/**
 * @file transfer_stubs.c
 * @brief Stub implementations for missing SDK functions — replace with real code when available
 */

#include "transfer_stubs.h"

#define LOG_TAG             "[TRANSFER_STUBS]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"




/* bt_emitter_start_search_device 在 bt_emitter.c 中已定义，此处不重复提供 */

/* ==========================================================================
 * ROLE_EMITTER 定义 — 已提供，get_role_type_by_addr 由 SDK btstack.a 提供
 * ========================================================================== */

#ifndef ROLE_EMITTER
#define ROLE_EMITTER    0x03  /* 发射器角色，区别于 REMOTE_SINK(0x01) / REMOTE_SOURCE(0x02) */
#endif

/* get_role_type_by_addr 由 SDK btstack.a 提供，此处不重复定义 */

/* ========================================================================== */
/* Walkie-Talkie UI — 弱符号桩 (仅非 kid_camera 配置链接)                     */
/* 真实实现在 kid_camera UI 模块 walkie_talkie_ui.c, 以强符号覆盖本弱符号。    */
/* 此处保留弱桩是为了: 当 TCFG_VSTREAM_TRANSFER=1 但未编译 kid_camera UI 模块  */
/* (例如 WATCH/AI_GLASSES/DASHBOARD 子项目) 时, vstream_cmd.c 仍能链接。      */
/* ========================================================================== */

__attribute__((weak)) void walkie_talkie_talk_sel_show(void)
{
    log_info("stub: %s called\n", __func__);
}

__attribute__((weak)) void walkie_talkie_hangup_show(void)
{
    log_info("stub: %s called\n", __func__);
}

__attribute__((weak)) void walkie_talkie_blank_remote(void)
{
    log_info("stub: %s called\n", __func__);
}

__attribute__((weak)) void walkie_talkie_unblank_remote(void)
{
    log_info("stub: %s called\n", __func__);
}

__attribute__((weak)) void walkie_talkie_page_return(void)
{
    log_info("stub: %s called\n", __func__);
}

__attribute__((weak)) int walkie_talkie_get_effect(void)
{
    return 0;  /* default: no effect */
}

__attribute__((weak)) int walkie_talkie_talk_voice_pic_show(void)
{
    log_info("stub: %s called\n", __func__);
    return 0;
}

__attribute__((weak)) int walkie_talkie_talk_voice_pic_hide(void)
{
    log_info("stub: %s called\n", __func__);
    return 0;
}

__attribute__((weak)) void walkie_talkie_video_enter(void)
{
    log_info("stub: %s called\n", __func__);
}

/* Audio decoder stub — 原工程 vstream_receiver.c 中仅 local extern，无独立实现 */
/* ========================================================================== */

void kid_audio_dec_resume(void)
{
    /* TODO: resume audio decoder when new data arrives */
}

/* ========================================================================== */
/* Data storage stubs                                                         */
/* ========================================================================== */

int small_file_get_max_item(u8 small_file_type)
{
    /* 默认允许 10 条记录 */
    return 10;
}

/* small_file_delete_by_index 在 data_storage_fdb.c / data_storage_vm.c 中已定义，此处不重复提供 */

