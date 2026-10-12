#include "pet_dashboard_music.h"
#include <string.h>

#if !defined(PET_DASHBOARD_MUSIC_SIMULATOR) || defined(PET_REVERSE_NOTICE_TEST)
#include "pet_reverse_notice.h"
#endif

#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
#include "app_config.h"
#include "app_msg.h"
#include "system/timer.h"
#include <stdlib.h>
#if TCFG_APP_BT_EN && TCFG_USER_BT_CLASSIC_ENABLE
#include "btstack/avctp_user.h"
#include "bt_event_func.h"
#include "bt.h"
#endif
#else
#include <stdbool.h>
#endif

#define MUSIC_CHECK_PERIOD_MS 100U
#define MUSIC_EXIT_DELAY_MS 3000U

static pet_dashboard_music_view_t view;
static pet_dashboard_music_view_t sent_view;
static bool notify_pending = true;
static bool exit_pending;
static uint32_t stopped_at;
static uint32_t last_tick;
static bool cover_requested;
#if PET_DASHBOARD_BT_LYRICS_MODE
/* SDK 按 TITLE→ARTIST 返回属性，保留本组歌词供随后到达的歌曲信息关联。 */
static char pending_lyric[256];
static bool pending_lyric_valid;
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
/* 保存真实歌名，普通信息补发不能清掉同首歌已经收到的歌词。 */
static char metadata_title[256];
static bool metadata_is_lyrics;
static uint32_t pending_text_at;
static void music_apply_pending_text(void);
#endif
#endif
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
static uint16_t check_timer;
static uint32_t metadata_checked_at;
static bool metadata_requested;
#endif

/** @brief 默认显示出口，UI 适配层提供实际实现。 */
__attribute__((weak)) int pet_dashboard_music_notify(const pet_dashboard_music_view_t *state)
{
    (void)state;
    return 0;
}

/** @brief 未启用 BIP 接收服务时的空实现。 */
__attribute__((weak)) void pet_dashboard_music_album_reset(void)
{
}

/** @brief 未启用封面接收服务时的空出口。 */
__attribute__((weak)) void pet_dashboard_music_album_select(const char *handle)
{
    (void)handle;
}

/** @brief 通过 SDK 请求封面，不在文本更新时调用。 */
__attribute__((weak)) int pet_dashboard_music_album_request(void)
{
#if !defined(PET_DASHBOARD_MUSIC_SIMULATOR) && TCFG_BT_SUPPORT_BIP
    return bt_cmd_prepare(USER_CTRL_BIP_GET_IMAGE, 0, NULL);
#else
    return 0;
#endif
}

/** @brief 只在状态改变或主动刷新时通知显示层。 */
static void music_publish(bool force)
{
    if (force || memcmp(&view, &sent_view, sizeof(view)) != 0) {
        notify_pending = true;
    }
    if (notify_pending && pet_dashboard_music_notify(&view) == 0) {
        sent_view = view;
        notify_pending = false;
    }
}

/** @brief 初始化业务状态，不操作页面控件。 */
void pet_dashboard_music_reset(void)
{
    memset(&view, 0, sizeof(view));
    sent_view = view;
    notify_pending = true;
    exit_pending = false;
    stopped_at = 0;
    last_tick = 0;
    cover_requested = false;
#if PET_DASHBOARD_BT_LYRICS_MODE
    pending_lyric[0] = '\0';
    pending_lyric_valid = false;
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    metadata_title[0] = '\0';
    metadata_is_lyrics = false;
    pending_text_at = 0;
#endif
#endif
}

/** @brief 获取状态快照，仅供应用线程或仿真检查。 */
const pet_dashboard_music_view_t *pet_dashboard_music_get_view(void)
{
    return &view;
}

/** @brief 根据真实连接和播放状态推进显示规则，now_ms 为单调毫秒时钟。 */
void pet_dashboard_music_update(uint8_t connected, uint8_t playing, uint32_t now_ms)
{
    last_tick = now_ms;
    playing = connected && playing;
    if (!connected) {
        if (view.connected) {
            pet_dashboard_music_album_reset();
            view.title[0] = view.artist[0] = view.lyric[0] = '\0';
            view.cover_handle[0] = '\0';
            cover_requested = false;
            view.track_revision++;
        }
        view.music_visible = false;
        exit_pending = false;
#if PET_DASHBOARD_BT_LYRICS_MODE
        pending_lyric[0] = '\0';
        pending_lyric_valid = false;
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
        metadata_title[0] = '\0';
        metadata_is_lyrics = false;
#endif
#endif
    }
#if !defined(PET_DASHBOARD_MUSIC_SIMULATOR) || defined(PET_REVERSE_NOTICE_TEST)
    else if (pet_reverse_notice_preserve_music_view()) {
        /* 倒车播报引起的临时暂停/恢复不改变手动选择的草地或音乐区。 */
        exit_pending = false;
    }
#endif
    else if (playing) {
        exit_pending = false;
        /* 只在开始/恢复播放时进入，手动返回后持续播放不再抢占。 */
        if (!view.playing) view.music_visible = true;
    } else if (view.music_visible) {
        if (!exit_pending) {
            stopped_at = now_ms;
            exit_pending = true;
        } else if ((uint32_t)(now_ms - stopped_at) >= MUSIC_EXIT_DELAY_MS) {
            view.music_visible = false;
            exit_pending = false;
        }
    }
    view.connected = connected;
    view.playing = playing;
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    /* 同组 TITLE 与 ARTIST 一起解释；只有 TITLE 时按已确认格式补发。 */
    if (connected && pending_lyric_valid && (uint32_t)(now_ms - pending_text_at) >= 100U) {
        music_apply_pending_text();
        pending_lyric_valid = false;
    }
#endif
    music_publish(false);
}

/** @brief 查询 A2DP 状态；未初始化或未连接时按停止处理。 */
void pet_dashboard_music_poll(uint32_t now_ms)
{
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
    pet_reverse_notice_tick(now_ms);
    bool connected = false;
    bool playing = false;
#if TCFG_APP_BT_EN && TCFG_USER_BT_CLASSIC_ENABLE
    if (get_bt_init_status()) {
        connected = bt_get_total_connect_dev() > 0;
        playing = connected && bt_a2dp_get_status() == BT_MUSIC_STATUS_STARTING;
        /* 主动补取信息，兼容没有及时发送歌曲变更通知的手机。 */
        if (connected && (!metadata_requested || (playing && !view.playing) ||
                          (playing && (uint32_t)(now_ms - metadata_checked_at) >= 2000U))) {
            bt_cmd_prepare(USER_CTRL_AVCTP_OPID_GET_MUSIC_INFO, 0, NULL);
            metadata_checked_at = now_ms;
            metadata_requested = true;
        }
    }
    if (!connected) metadata_requested = false;
#endif
    pet_dashboard_music_update(connected, playing, now_ms);
#else
    pet_dashboard_music_update(view.connected, view.playing, now_ms);
#endif
}

/** @brief 限长复制完整 UTF-8 字符，丢弃无效字节和控制字符。 */
static void music_copy_text(char *out, unsigned capacity, const uint8_t *in, unsigned length)
{
    unsigned i = 0, used = 0;
    while (i < length && in[i] != 0 && used + 1 < capacity) {
        unsigned n = in[i] < 0x80 ? 1 : in[i] >= 0xc2 && in[i] <= 0xdf ? 2 :
                     in[i] >= 0xe0 && in[i] <= 0xef ? 3 :
                     in[i] >= 0xf0 && in[i] <= 0xf4 ? 4 : 0;
        if (!n) { i++; continue; }
        if (i + n > length || used + n >= capacity) break;
        unsigned j;
        for (j = 1; j < n && (in[i + j] & 0xc0) == 0x80; j++) {}
        if (j != n || (n == 3 && ((in[i] == 0xe0 && in[i+1] < 0xa0) ||
                                  (in[i] == 0xed && in[i+1] >= 0xa0))) ||
            (n == 4 && ((in[i] == 0xf0 && in[i+1] < 0x90) ||
                       (in[i] == 0xf4 && in[i+1] >= 0x90)))) { i++; continue; }
        if (n == 1 && (in[i] < 0x20 || in[i] == 0x7f)) {
            if (in[i] == '\n' || in[i] == '\r' || in[i] == '\t') out[used++] = ' ';
        } else {
            memcpy(out + used, in + i, n);
            used += n;
        }
        i += n;
    }
    out[used] = '\0';
}

/** @brief 仅在封面句柄改变时取图，文字补全不打断传输。 */
static void music_cover_changed(const char *handle)
{
    if (strcmp(view.cover_handle, handle)) {
        strcpy(view.cover_handle, handle);
        view.track_revision++;
        cover_requested = false;
    } else if (cover_requested) return;
    if (!handle[0]) return;
    int ret = pet_dashboard_music_album_request();
    cover_requested = ret == 0;
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
    printf("[PET_MUSIC] cover request rev=%u ret=%d handle=%s title=%s\n",
           view.track_revision, ret, handle, view.title);
#else
    (void)ret;
#endif
}

#if PET_DASHBOARD_BT_LYRICS_MODE == 2
static void music_apply_normal_title(const char *title)
{
    if (strcmp(metadata_title, title)) {
        view.lyric[0] = '\0';
        view.artist[0] = '\0';
    }
    strcpy(metadata_title, title);
    strcpy(view.title, title);
}

static void music_apply_pending_text(void)
{
    if (metadata_is_lyrics) strcpy(view.lyric, pending_lyric);
    else music_apply_normal_title(pending_lyric);
}

/* 当前实机歌词扩展用 ARTIST="歌名 - 歌手"，TITLE 为歌词；保留组合原文。 */
static const char *music_heading_separator(const char *heading)
{
    const char *separator = strstr(heading, " - ");
    return separator && separator != heading && separator[3] ? separator : NULL;
}

static void music_apply_metadata_pair(const char *artist)
{
    const char *separator = music_heading_separator(artist);
    metadata_is_lyrics = separator != NULL;
    if (separator) {
        char title[256];
        unsigned title_len = (unsigned)(separator - artist);
        memcpy(title, artist, title_len);
        title[title_len] = '\0';
        if (strcmp(metadata_title, title) && !pending_lyric_valid) view.lyric[0] = '\0';
        strcpy(metadata_title, title);
        strcpy(view.title, artist);
        if (pending_lyric_valid) strcpy(view.lyric, pending_lyric);
        artist = separator + 3;
    } else if (pending_lyric_valid) {
        music_apply_normal_title(pending_lyric);
    }
    music_copy_text(view.artist, sizeof(view.artist), (const uint8_t *)artist, strlen(artist));
    pending_lyric_valid = false;
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
    printf("[PET_MUSIC] metadata format=%s title=%s lyric=%s\n",
           metadata_is_lyrics ? "lyrics" : "normal", view.title, view.lyric);
#endif
}
#endif

/** @brief 普通歌曲信息与歌词独立更新，所有处理均在应用线程完成。 */
static void music_apply_text(uint8_t type, const uint8_t *text, uint16_t length)
{
    if (!view.connected) return;
    if (type == 8) {
        char handle[8] = {0};
        if (length) memcpy(handle, text, length);
        music_cover_changed(handle);
        music_publish(false);
        return;
    }
    char copy[256];
#if PET_DASHBOARD_BT_LYRICS_MODE == 2
    music_copy_text(copy, sizeof(copy), text, length);
    if (type == 1) {
        strcpy(pending_lyric, copy);
        pending_lyric_valid = true;
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
        pending_text_at = sys_timer_get_ms();
#else
        pending_text_at = last_tick;
#endif
        return;
    } else if (type == 2) {
        if (copy[0]) music_apply_metadata_pair(copy);
    } else if (type == 0x80) {
        strcpy(view.lyric, copy);
    }
#elif PET_DASHBOARD_BT_LYRICS_MODE == 1
    music_copy_text(copy, sizeof(copy), text, length);
    if (type == 1) {
        strcpy(pending_lyric, copy);
        pending_lyric_valid = true;
        if (!strcmp(view.lyric, copy)) return;
        strcpy(view.lyric, copy);
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
        printf("[PET_MUSIC] lyric rev=%u source=TITLE text=%s\n", view.track_revision, copy);
#endif
    } else if (type == 2) {
        /* 歌曲与歌手组合保持原文，不猜分隔符，也不拆除 remix 等信息。 */
        if (!copy[0]) {
            pending_lyric_valid = false;
            return;
        }
        bool changed_track = strcmp(view.title, copy) != 0;
        if (changed_track) {
            strcpy(view.title, copy);
            strcpy(view.lyric, pending_lyric_valid ? pending_lyric : "");
        }
        music_copy_text(view.artist, sizeof(view.artist), (const uint8_t *)copy, strlen(copy));
        pending_lyric_valid = false;
    } else if (type == 0x80) {
        strcpy(view.lyric, copy);
        pending_lyric_valid = false;
    }
#else
    music_copy_text(copy, type == 2 ? sizeof(view.artist) : sizeof(copy), text, length);
    if (type == 1) {
        if (strcmp(view.title, copy) == 0) return;
        strcpy(view.title, copy);
        view.artist[0] = view.lyric[0] = '\0';
        /* 无封面句柄的普通模式仍保留换歌清理，但不猜测取图时机。 */
        if (!view.cover_handle[0]) {
            view.track_revision++;
            pet_dashboard_music_album_reset();
        }
    } else if (type == 2) strcpy(view.artist, copy);
    else if (type == 0x80) strcpy(view.lyric, copy);
#endif
    music_publish(false);
}

#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
typedef struct {
    uint16_t length;
    uint8_t type;
    uint8_t data[256];
} music_text_message_t;

/** @brief 消费跨线程的独立文本副本，释放消息所有权。 */
static void music_text_callback(void *priv)
{
    music_text_message_t *message = priv;
    music_apply_text(message->type, message->data, message->length);
    free(message);
}
#endif

/** @brief 拷贝蓝牙临时数据后投递应用线程，避免保存回调指针。 */
int pet_dashboard_music_receive_text(uint8_t type, const uint8_t *text, uint16_t length)
{
    if (type != 1 && type != 2 && type != 8 && type != 0x80) return -1;
    if (!text && length) return -1;
    if (type == 8) {
        if (length != 0 && length != 7) return -1;
        unsigned i;
        for (i = 0; i < length; i++) if (text[i] < '0' || text[i] > '9') return -1;
        char handle[8] = {0};
        if (length) memcpy(handle, text, length);
        /* SDK 随后会处理句柄和启动 BIP，先同步标记接收缓存。 */
        pet_dashboard_music_album_select(handle);
    }
    if (length > 255) length = 255;
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
    music_text_message_t *message = malloc(sizeof(*message));
    if (!message) return -1;
    message->length = length;
    message->type = type;
    if (length) memcpy(message->data, text, length);
    int argv[3] = {(int)music_text_callback, 1, (int)message};
    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
    if (ret) free(message);
    return ret;
#else
    music_apply_text(type, text, length);
    return 0;
#endif
}

/** @brief 手动切换显示，暂停时打开音乐区重新计时三秒。 */
static void music_toggle(uint32_t now_ms)
{
    if (!view.connected) return;
    view.music_visible = !view.music_visible;
    exit_pending = view.music_visible && !view.playing;
    if (exit_pending) stopped_at = now_ms;
    music_publish(false);
}

/** @brief 发送手机媒体控制命令，显示状态以蓝牙实际反馈为准。 */
static void music_control(pet_dashboard_music_request_t request)
{
#if !defined(PET_DASHBOARD_MUSIC_SIMULATOR) && TCFG_APP_BT_EN && TCFG_USER_BT_CLASSIC_ENABLE
    if (!view.connected || !get_bt_init_status() || bt_get_total_connect_dev() == 0) return;
    USER_CMD_TYPE op;
    switch (request) {
    case PET_MUSIC_PREVIOUS: op = USER_CTRL_AVCTP_OPID_PREV; break;
    case PET_MUSIC_NEXT: op = USER_CTRL_AVCTP_OPID_NEXT; break;
    case PET_MUSIC_PLAY: op = USER_CTRL_AVCTP_OPID_PLAY; break;
    case PET_MUSIC_PAUSE: op = USER_CTRL_AVCTP_OPID_PAUSE; break;
    default: return;
    }
    bt_cmd_prepare(op, 0, 0);
#else
    (void)request;
#endif
}

/** @brief 在应用线程执行输入请求，先同步最新播放状态。 */
static void music_handle_request(pet_dashboard_music_request_t request, uint32_t now_ms)
{
    pet_dashboard_music_poll(now_ms);
    if (request == PET_MUSIC_REFRESH) music_publish(true);
    else if (request == PET_MUSIC_TOGGLE_VIEW) music_toggle(now_ms);
    else music_control(request);
}

#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
/** @brief 应用线程的周期检测，UI 不承担检测和计时。 */
static void music_timer(void *priv)
{
    (void)priv;
    pet_dashboard_music_poll(sys_timer_get_ms());
}

/** @brief 截获仪表音乐请求，适用于所有应用模式。 */
static int music_app_message(int *msg)
{
    if (msg[0] != APP_MSG_PET_DASHBOARD_MUSIC) return 0;
    music_handle_request((pet_dashboard_music_request_t)msg[1], sys_timer_get_ms());
    return 1;
}

APP_MSG_PROB_HANDLER(pet_dashboard_music_message) = {
    .owner = 0xff,
    .from = MSG_FROM_APP,
    .handler = music_app_message,
};
#endif

/** @brief 启动应用层检测；重复调用不会重置手动选择。 */
int pet_dashboard_music_start(void)
{
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
    if (check_timer != 0) return 0;
    check_timer = sys_timer_add(0, music_timer, MUSIC_CHECK_PERIOD_MS);
    if (check_timer == 0) return -1;
    printf("[PET_MUSIC] text mode=%s\n", PET_DASHBOARD_BT_LYRICS_MODE == 2 ? "mixed-v2" :
           PET_DASHBOARD_BT_LYRICS_MODE ? "lyrics" : "normal");
    pet_dashboard_music_poll(sys_timer_get_ms());
#else
    music_publish(true);
#endif
    return 0;
}

/** @brief 跨线程投递用户操作，仿真直接调用相同处理流程。 */
int pet_dashboard_music_request(pet_dashboard_music_request_t request)
{
    if (request < PET_MUSIC_REFRESH || request > PET_MUSIC_PAUSE) return -1;
#ifndef PET_DASHBOARD_MUSIC_SIMULATOR
    return app_send_message(APP_MSG_PET_DASHBOARD_MUSIC, request);
#else
    music_handle_request(request, last_tick);
    return 0;
#endif
}
