/**
 * @file transfer_stubs.h
 * @brief Stub declarations for missing SDK functions needed by video/transfer module
 */
#ifndef TRANSFER_STUBS_H
#define TRANSFER_STUBS_H

#include "system/includes.h"

/* ============================================================================
 * BT management stubs
 * ============================================================================ */



/* ============================================================================
 * Walkie-Talkie UI — 弱符号桩声明; 真实实现在 kid_camera UI 模块
 * (walkie_talkie_ui.c) 以强符号覆盖本弱符号, 此处仅在非 kid_camera 配置下生效
 * ============================================================================ */

void walkie_talkie_talk_sel_show(void);
void walkie_talkie_hangup_show(void);
void walkie_talkie_blank_remote(void);
void walkie_talkie_unblank_remote(void);
void walkie_talkie_page_return(void);
int walkie_talkie_get_effect(void);
int walkie_talkie_talk_voice_pic_show(void);
int walkie_talkie_talk_voice_pic_hide(void);
void walkie_talkie_video_enter(void);

/* ============================================================================
 * vstream_chat.c / vstream_cmd.c — 已有完整实现，无需桩
 *    vstream_file_save               → vstream_chat.c
 *    vstream_cmd_ask_chat / recv_chat / send_chat / send_chat_save  → vstream_chat.c
 *    kid_wt_chat_save_dump / char_get_rec_data_total / chat_get_rec_data_by_index → vstream_chat.c
 * ============================================================================ */

/* ============================================================================
 * Audio decoder stub — 原工程中仅 local extern，无独立实现文件
 * ============================================================================ */

/** Resume audio decoder after receiving data. */
void kid_audio_dec_resume(void);

/* ============================================================================
 * FPS counter stub
 * ============================================================================ */

/** Update video TX frame-per-second counter. */
int fps_win_counter_update(u32 now_ms);

/* ============================================================================
 * Data storage stubs — 目标 SDK data_storage 模块未提供 small_file_get_max_item
 * ============================================================================ */

/** Get max item count for small file storage type. */
int small_file_get_max_item(u8 small_file_type);

/** Delete small file record by index. */
int small_file_delete_by_index(u8 small_file_type, int index);

/* ============================================================================
 * BT PA/RF stubs — old SDK GPIO API wrappers (gpio_set_die/gpio_set_hd/gpio_set_hd0)
 * ============================================================================ */

/** BT RF PA control IO remap. Stub — SDK btctrler 库未包含此符号 */
void bt_rf_PA_control_io_remap(u16 tx_io, u16 rx_io);

/** GPIO digital input enable (old SDK API). */
int gpio_set_die(u32 gpio, u32 value);

/** GPIO high drive strength (old SDK API). */
int gpio_set_hd(u32 gpio, u32 value);

/** GPIO high drive strength pin 0 (old SDK API). */
int gpio_set_hd0(u32 gpio, u32 value);

/** GPIO direction output (old SDK API). */
int gpio_direction_output(u32 gpio, u32 value);

/* ============================================================================
 * Music player stubs — music_player.c 仅在 LVGL UI 模式下编译
 * ============================================================================ */

/** Get first music file player. Stub returns NULL when music_player.c not linked. */
struct file_player *get_music_file_player(void);

/** Get music file player status. Stub returns 0 (not playing). */
int music_file_get_player_status(struct file_player *music_player);

/* ============================================================================
 * BT role query — ROLE_EMITTER 定义在此，get_role_type_by_addr 由 SDK btstack.a 提供
 * ============================================================================ */

/** 发射器角色，区别于 REMOTE_SINK(0x01) / REMOTE_SOURCE(0x02) */
#ifndef ROLE_EMITTER
#define ROLE_EMITTER    0x03
#endif

/* get_role_type_by_addr 由 SDK btstack.a 提供，仅声明原型 */
int get_role_type_by_addr(u8 *addr);

#endif /* TRANSFER_STUBS_H */
