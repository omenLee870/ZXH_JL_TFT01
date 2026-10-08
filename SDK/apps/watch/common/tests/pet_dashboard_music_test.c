#include "pet_dashboard_music.h"
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static pet_dashboard_music_view_t rendered;
static unsigned notifications;
static bool reject_next;

/** @brief 记录显示通知，模拟一次消息投递失败。 */
int pet_dashboard_music_notify(const pet_dashboard_music_view_t *view)
{
    notifications++;
    if (reject_next) {
        reject_next = false;
        return -1;
    }
    rendered = *view;
    return 0;
}

/** @brief 检查应用状态和显示通知一致。 */
static void expect(bool connected, bool playing, bool music)
{
    const pet_dashboard_music_view_t *view = pet_dashboard_music_get_view();
    assert(view->connected == connected && rendered.connected == connected);
    assert(view->playing == playing && rendered.playing == playing);
    assert(view->music_visible == music && rendered.music_visible == music);
}

/** @brief 覆盖自动切换、手动切换、计时回绕和通知重试。 */
int main(void)
{
    pet_dashboard_music_reset();
    assert(pet_dashboard_music_start() == 0);
    expect(false, false, false);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    expect(false, false, false);

    pet_dashboard_music_update(true, false, 0);
    expect(true, false, false);
    pet_dashboard_music_update(true, true, 100);
    expect(true, true, true);
    const uint8_t title[] = {0xe6, 0x99, 0xb4, 0xe5, 0xa4, 0xa9}; /* 晴天，不带结尾零。 */
    assert(pet_dashboard_music_receive_text(1, title, sizeof(title)) == 0);
    assert(strcmp(rendered.title, "\xe6\x99\xb4\xe5\xa4\xa9") == 0);
    uint32_t revision = rendered.track_revision;
    pet_dashboard_music_receive_text(2, (const uint8_t *)"artist", 6);
    pet_dashboard_music_receive_text(0x80, (const uint8_t *)"line one", 8);
    pet_dashboard_music_receive_text(1, title, sizeof(title));
    assert(rendered.track_revision == revision && strcmp(rendered.lyric, "line one") == 0);
    pet_dashboard_music_receive_text(1, (const uint8_t *)"next", 4);
    assert(rendered.track_revision == revision + 1 && !rendered.artist[0] && !rendered.lyric[0]);
    uint8_t long_title[300];
    memset(long_title, 'a', sizeof(long_title));
    long_title[254] = 0xe6; long_title[255] = 0x99; long_title[256] = 0xb4;
    pet_dashboard_music_receive_text(1, long_title, sizeof(long_title));
    assert(strlen(rendered.title) == 254); /* 超长字符串不截断中文码点。 */
    const uint8_t invalid[] = {0xff, 'A', '\n', 'B', 0xc0, 0xaf};
    pet_dashboard_music_receive_text(0x80, invalid, sizeof(invalid));
    assert(strcmp(rendered.lyric, "A B") == 0);
    assert(pet_dashboard_music_receive_text(1, NULL, 1) == -1);
    assert(pet_dashboard_music_receive_text(3, title, sizeof(title)) == -1);
    pet_dashboard_music_update(false, false, 5000);
    assert(!rendered.title[0] && !rendered.lyric[0]);
    pet_dashboard_music_receive_text(1, title, sizeof(title));
    assert(!rendered.title[0]); /* 断开后丢弃晚到的数据。 */
    pet_dashboard_music_update(true, true, 5100);
    pet_dashboard_music_update(true, false, 1000);
    pet_dashboard_music_poll(3999);
    expect(true, false, true);
    pet_dashboard_music_poll(4000);
    expect(true, false, false);

    pet_dashboard_music_update(true, true, 5000);
    pet_dashboard_music_update(true, false, 6000);
    pet_dashboard_music_update(true, true, 8999);
    pet_dashboard_music_poll(10000);
    expect(true, true, true);

    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    expect(true, true, false);
    pet_dashboard_music_poll(20000);
    pet_dashboard_music_request(PET_MUSIC_REFRESH);
    expect(true, true, false);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    expect(true, true, true);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    pet_dashboard_music_update(true, false, 21000);
    pet_dashboard_music_update(true, true, 22000);
    expect(true, true, true);

    pet_dashboard_music_update(false, true, 23000);
    expect(false, false, false);
    pet_dashboard_music_update(true, true, 24000);
    expect(true, true, true);
    pet_dashboard_music_update(true, false, 25000);
    pet_dashboard_music_poll(28000);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    pet_dashboard_music_poll(30999);
    expect(true, false, true);
    pet_dashboard_music_poll(31000);
    expect(true, false, false);

    pet_dashboard_music_update(true, true, UINT32_MAX - 2000U);
    pet_dashboard_music_update(true, false, UINT32_MAX - 1000U);
    pet_dashboard_music_poll(1998U);
    expect(true, false, true);
    pet_dashboard_music_poll(1999U);
    expect(true, false, false);

    reject_next = true;
    unsigned before = notifications;
    pet_dashboard_music_update(true, true, 4000);
    assert(notifications == before + 1);
    pet_dashboard_music_poll(4100);
    assert(notifications == before + 2);
    expect(true, true, true);
    before = notifications;
    pet_dashboard_music_poll(4200);
    assert(notifications == before);

    pet_dashboard_music_request(PET_MUSIC_PAUSE);
    expect(true, true, true); /* 发命令不伪造手机播放状态。 */
    assert(pet_dashboard_music_request((pet_dashboard_music_request_t)99) == -1);
    expect(true, true, true);
    puts("PASS: transitions, boundaries, metadata, UTF-8 limits, track reset, disconnect, notification retry");
    return 0;
}
