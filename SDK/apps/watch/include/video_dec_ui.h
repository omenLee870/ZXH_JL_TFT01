/**
 * @file video_dec_ui.h
 * @brief 视频解码回放 UI 回调接口 — Coder2 实现, Coder1 调用
 *
 * 本文件声明 video_dec 模式（回放浏览）的 UI 回调接口。
 * 与 playback_ui.h 接口一致，但命名空间为 video_dec_ui_*，
 *
 * @author  Coder2 (接口定义)
 * @date    2026-05-29
 */
#ifndef CONFIG_VIDEO_DEC_UI_H
#define CONFIG_VIDEO_DEC_UI_H

#include "system/includes.h"
#include "video_dec.h"

/** UI 初始化 / 清理 */
void video_dec_ui_init(int width, int height);
void video_dec_ui_deinit(void);
void video_dec_ui_close_video(void);
void video_dec_ui_pause_video_fetch(void);

/** 状态变化通知 */
void video_dec_ui_on_state_change(enum video_dec_state old_state,
                                  enum video_dec_state new_state);

/** 刷新九宫格缩略图列表 */
void video_dec_ui_refresh_grid(const char *focus_name, int count, int focus);

/** 显示照片大图 */
void video_dec_ui_show_photo(const char *path);

/** 显示视频播放界面 */
void video_dec_ui_show_video(const char *path);

/** 清空视频显示控件(隐藏 img_obj, 防止删除所有后残留最后一帧推屏显示) */
void video_dec_ui_clear_video(void);

/** 更新视频播放时间 (elapsed) */
void video_dec_ui_update_time(u32 elapsed_sec);

/** 更新视频播放进度 */
void video_dec_ui_update_progress(u32 current_sec, u32 total_sec);
void video_dec_ui_time_hide(void);

/** 更新存储信息栏 */
void video_dec_ui_update_storage(video_dec_storage_t *info);

/** 删除确认弹窗 */
void video_dec_ui_confirm_delete(const char *path);

/** 警告/提示弹窗: 0=存储满, 1=无SD卡, 2=删除失败, 3=文件已锁定 */
void video_dec_ui_show_alert(int type);
void video_dec_ui_back_to_home(void);
/* 删除菜单(roller_1/roller_2)显示控制, 由 app_core 状态机经 RPC 调用 */
void video_dec_ui_del_menu_show(int sel);
void video_dec_ui_del_menu_set_sel(int sel);
void video_dec_ui_del_menu_confirm(void);
void video_dec_ui_del_menu_hide(void);

#endif /* CONFIG_VIDEO_DEC_UI_H */
