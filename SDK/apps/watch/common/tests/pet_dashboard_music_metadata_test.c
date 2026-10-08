#include "pet_dashboard_music.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if PET_DASHBOARD_BT_LYRICS_MODE != 2
#error "This test must exercise the default mixed metadata mode"
#endif

static pet_dashboard_music_view_t rendered;
static unsigned notifications, requests, resets;
static bool reject_next;

int pet_dashboard_music_notify(const pet_dashboard_music_view_t *state)
{
    notifications++;
    if (reject_next) { reject_next = false; return -1; }
    rendered = *state;
    return 0;
}
int pet_dashboard_music_album_request(void) { requests++; return 0; }
void pet_dashboard_music_album_reset(void) { resets++; }

static void text(uint8_t type, const char *value)
{
    assert(pet_dashboard_music_receive_text(type, (const uint8_t *)value, strlen(value)) == 0);
}
static void pair(const char *title, const char *artist) { text(1, title); text(2, artist); }

int main(void)
{
    pet_dashboard_music_reset();
    assert(pet_dashboard_music_start() == 0);
    pet_dashboard_music_update(true, true, 100);
    pair("J字情书", "孙睡觉/四七");
    text(8, "0000080");
    assert(!strcmp(rendered.title, "J字情书"));
    assert(!strcmp(rendered.artist, "孙睡觉/四七") && !rendered.lyric[0]);
    uint32_t revision = rendered.track_revision;
    /* 歌词晚到：反复补取普通信息不能把歌名冒充歌词。 */
    unsigned i;
    for (i = 0; i < 8; i++) {
        pet_dashboard_music_poll(2100 + i * 2000);
        pair("J字情书", "孙睡觉/四七");
        assert(!strcmp(rendered.title, "J字情书") && !rendered.lyric[0]);
    }
    unsigned before = notifications;
    text(1, "你要去找太阳");
    assert(notifications == before);
    text(2, "J字情书 - 孙睡觉/四七");
    assert(!strcmp(rendered.title, "J字情书 - 孙睡觉/四七"));
    assert(!strcmp(rendered.artist, "孙睡觉/四七"));
    assert(!strcmp(rendered.lyric, "你要去找太阳"));
    /* 核心回归：同首歌返回普通属性，保留已有歌词，两个区域都不能错位。 */
    pair("J字情书", "孙睡觉/四七");
    assert(!strcmp(rendered.title, "J字情书"));
    assert(!strcmp(rendered.lyric, "你要去找太阳"));
    for (i = 0; i < 5; i++) {
        pair("下一句歌词", "J字情书 - 孙睡觉/四七");
        pair("J字情书", "孙睡觉/四七");
        text(8, "0000080");
        assert(!strcmp(rendered.title, "J字情书"));
        assert(!strcmp(rendered.lyric, "下一句歌词"));
        assert(rendered.track_revision == revision && requests == 1 && resets == 0);
    }
    puts("PASS: delayed lyrics and interleaved ordinary attributes keep song title/artist/lyric separate");

    pair("天气先生", "方大同");
    assert(!strcmp(rendered.title, "天气先生") && !rendered.lyric[0]);
    pair("第一句歌词", "天气先生 - 方大同");
    text(8, "0000081");
    assert(!strcmp(rendered.lyric, "第一句歌词") && requests == 2);
    text(2, "方大同"); /* ARTIST-only 补发不能用作者覆盖歌名或清歌词。 */
    assert(!strcmp(rendered.title, "天气先生 - 方大同"));
    assert(!strcmp(rendered.lyric, "第一句歌词"));
    pair("下一句", "天气先生 - 方大同");
    text(1, "仅 TITLE 的歌词");
    pet_dashboard_music_poll(16199);
    assert(!strcmp(rendered.lyric, "下一句"));
    pet_dashboard_music_poll(16200);
    assert(!strcmp(rendered.lyric, "仅 TITLE 的歌词"));
    pair("", "天气先生 - 方大同");
    assert(!rendered.lyric[0]);
    text(0x80, "单独歌词入口");
    pair("天气先生", "方大同");
    assert(!strcmp(rendered.lyric, "单独歌词入口"));
    reject_next = true;
    pair("通知重试", "天气先生 - 方大同");
    assert(strcmp(rendered.lyric, "通知重试"));
    pet_dashboard_music_poll(16300);
    assert(!strcmp(rendered.lyric, "通知重试"));
    text(1, "尚未提交的旧歌词");
    pet_dashboard_music_update(false, false, 16400);
    assert(!rendered.title[0] && !rendered.lyric[0] && resets == 1);
    pet_dashboard_music_update(true, true, 16500);
    text(1, "重连后的歌名");
    pet_dashboard_music_poll(16600);
    assert(!strcmp(rendered.title, "重连后的歌名") && !rendered.lyric[0]);
    /* 最新实机日志：先发歌名/作者，随后补发歌词/完整歌曲信息。 */
    pair("对半分 (时间是庸医 他让我忏悔)", "Tiko");
    assert(!strcmp(rendered.title, "对半分 (时间是庸医 他让我忏悔)"));
    assert(!strcmp(rendered.artist, "Tiko") && !rendered.lyric[0]);
    pair("熄灭", "Ove");
    pair("作词: Ove", "熄灭 - Ove");
    assert(!strcmp(rendered.title, "熄灭 - Ove") && !strcmp(rendered.lyric, "作词: Ove"));
    pair("熄灭", "Ove");
    assert(!strcmp(rendered.title, "熄灭") && !strcmp(rendered.lyric, "作词: Ove"));
    puts("PASS: track change, ARTIST-only, TITLE-only, blank line, explicit lyric, retry and reconnect");
    return 0;
}
