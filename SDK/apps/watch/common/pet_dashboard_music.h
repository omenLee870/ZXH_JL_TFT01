#ifndef PET_DASHBOARD_MUSIC_H
#define PET_DASHBOARD_MUSIC_H

#include <stdint.h>

/* 1：已验证的蓝牙歌词模式（TITLE=歌词，ARTIST=歌曲信息）；0：普通歌曲模式。 */
#ifndef PET_DASHBOARD_BT_LYRICS_MODE
#define PET_DASHBOARD_BT_LYRICS_MODE 1
#endif
#if PET_DASHBOARD_BT_LYRICS_MODE != 0 && PET_DASHBOARD_BT_LYRICS_MODE != 1
#error "PET_DASHBOARD_BT_LYRICS_MODE must be 0 or 1"
#endif

/* 应用层输出，UI 只按这些状态显示。 */
typedef struct {
    uint8_t connected;
    uint8_t playing;
    uint8_t music_visible;
    uint32_t track_revision; /* 封面更新序号；无句柄的普通模式仍按歌名清理。 */
    char cover_handle[8];
    char title[256];
    char artist[128];
    char lyric[256];
} pet_dashboard_music_view_t;

typedef enum {
    PET_MUSIC_REFRESH,
    PET_MUSIC_TOGGLE_VIEW,
    PET_MUSIC_PREVIOUS,
    PET_MUSIC_NEXT,
    PET_MUSIC_PLAY,
    PET_MUSIC_PAUSE
} pet_dashboard_music_request_t;

/* 在 app_core 启动；检测和状态判断均在该线程执行。 */
int pet_dashboard_music_start(void);
/* UI 线程投递操作，不直接修改业务状态。 */
int pet_dashboard_music_request(pet_dashboard_music_request_t request);
/* TITLE(1)/ARTIST(2) 按配置解释；8 为 AVRCP 封面句柄，0x80 为额外歌词。 */
int pet_dashboard_music_receive_text(uint8_t type, const uint8_t *text, uint16_t length);
/* 封面接收服务实现，用于换歌/断开时使旧缓存失效。 */
void pet_dashboard_music_album_reset(void);
/* 接收服务同步记录协议栈的封面句柄，防止旧传输覆盖新封面。 */
void pet_dashboard_music_album_select(const char *handle);
/* 请求封面，返回命令投递结果；仿真可替换此出口。 */
int pet_dashboard_music_album_request(void);
/* 以下接口由应用线程串行调用，仿真也使用同一状态机。 */
void pet_dashboard_music_reset(void);
void pet_dashboard_music_update(uint8_t connected, uint8_t playing, uint32_t now_ms);
void pet_dashboard_music_poll(uint32_t now_ms);
const pet_dashboard_music_view_t *pet_dashboard_music_get_view(void);
/* 显示适配层实现；0 表示通知成功，失败时应用层稍后重试。 */
int pet_dashboard_music_notify(const pet_dashboard_music_view_t *view);

#endif
