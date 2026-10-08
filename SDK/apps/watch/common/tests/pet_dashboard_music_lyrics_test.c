#include "pet_dashboard_music.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if !PET_DASHBOARD_BT_LYRICS_MODE
#error "This test requires the explicit Bluetooth lyrics mode"
#endif

static pet_dashboard_music_view_t rendered;
static unsigned notifications, album_resets, cover_requests;
static bool reject_request;
static bool reject_next;

/** @brief 记录应用输出，并模拟一次 UI 消息投递失败。 */
int pet_dashboard_music_notify(const pet_dashboard_music_view_t *view)
{
    notifications++;
    if (reject_next) { reject_next = false; return -1; }
    rendered = *view;
    return 0;
}

/** @brief 统计封面失效次数，检查逐句更新不会重复触发换歌。 */
void pet_dashboard_music_album_reset(void)
{
    album_resets++;
}

/** @brief 记录真正的封面请求，并模拟投递失败。 */
int pet_dashboard_music_album_request(void)
{
    cover_requests++;
    if (reject_request) { reject_request = false; return -1; }
    return 0;
}

/** @brief 注入真实 SDK 同类型字段，不依赖字符串结尾以外的缓冲区。 */
static void text(uint8_t type, const char *value)
{
    assert(pet_dashboard_music_receive_text(type, (const uint8_t *)value, strlen(value)) == 0);
}

/** @brief 重放实机确认的 TITLE→ARTIST 属性返回顺序。 */
static void pair(const char *lyric, const char *song)
{
    text(1, lyric);
    text(2, song);
}

/** @brief 验证两首歌、多句歌词、缺失字段、空行、截断和重连。 */
int main(void)
{
    pet_dashboard_music_reset();
    assert(pet_dashboard_music_start() == 0);
    pet_dashboard_music_update(true, true, 100);
    const char *song_a = "冰化了缘分就没了Vol.125 - 种一捧玫瑰/DJ阿智";
    const char *song_b = "无名的人（DJ阿智 remix） - DJ阿智";
    pair("编曲: DJ阿智", song_a);
    text(8, "0000052");
    assert(!strcmp(rendered.title, song_a) && !strcmp(rendered.lyric, "编曲: DJ阿智"));
    assert(album_resets == 0 && cover_requests == 1);
    uint32_t revision = rendered.track_revision;
    unsigned before = notifications;
    pair("编曲: DJ阿智", song_a);
    text(8, "0000052");
    assert(notifications == before && rendered.track_revision == revision && cover_requests == 1);
    pair("作词: DJ阿智", song_b);
    text(8, "0000053");
    assert(!strcmp(rendered.title, song_b) && !strcmp(rendered.lyric, "作词: DJ阿智"));
    assert(cover_requests == 2 && rendered.track_revision == revision + 1);
    revision = rendered.track_revision;
    const char *lines[] = {"作曲: DJ阿智", "编曲: DJ阿智", "当家乡入冬的时候", "列车到站以后", "小时候的风再吹过"};
    unsigned i;
    for (i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        pair(lines[i], song_b);
        assert(!strcmp(rendered.title, song_b) && !strcmp(rendered.lyric, lines[i]));
        text(8, "0000053");
        assert(rendered.track_revision == revision && cover_requests == 2);
    }
    puts("PASS: two cover handles request twice; repeated lyrics/handles leave album revision unchanged");

    text(1, "TITLE-only lyric update");
    assert(!strcmp(rendered.lyric, "TITLE-only lyric update") && cover_requests == 2);
    text(2, song_b); /* 消费本组歌词，后续 ARTIST-first 换歌不能带入旧句。 */
    text(2, "song C - singer");
    assert(!strcmp(rendered.title, "song C - singer") && !rendered.lyric[0] && cover_requests == 2);
    text(1, "C first lyric");
    text(2, "song C - singer");
    assert(!strcmp(rendered.lyric, "C first lyric") && cover_requests == 2);
    /* 下一首开头有相同歌词，也必须保留这次 TITLE，不能因文字相同而丢掉。 */
    pair("C first lyric", "song D - singer");
    assert(!strcmp(rendered.lyric, "C first lyric") && cover_requests == 2);
    text(1, ""); text(2, "song D - singer");
    assert(!rendered.lyric[0] && rendered.title[0] && cover_requests == 2);
    pair("valid line", "");
    assert(!strcmp(rendered.title, "song D - singer") && cover_requests == 2);
    text(2, "song E - singer");
    assert(!rendered.lyric[0] && cover_requests == 2);
    puts("PASS: TITLE-only/ARTIST-first, duplicate first lyric, empty lyric and missing ARTIST handled");

    /* 实机先返回歌手，再补全歌曲信息；封面句柄保持不变。 */
    text(8, "0000054");
    revision = rendered.track_revision;
    unsigned requests = cover_requests;
    pair("歌名", "歌者达达");
    pair("第一句歌词", "给我一个理由忘记（DJ车载版） - 歌者达达");
    text(8, "0000054");
    assert(rendered.track_revision == revision && cover_requests == requests && album_resets == 0);
    assert(!strcmp(rendered.title, "给我一个理由忘记（DJ车载版） - 歌者达达"));
    text(8, "0000055");
    assert(rendered.track_revision == revision + 1 && cover_requests == requests + 1);
    assert(pet_dashboard_music_receive_text(8, (const uint8_t *)"not-art", 7) == -1);
    assert(pet_dashboard_music_receive_text(8, (const uint8_t *)"000005", 6) == -1);
    reject_request = true;
    text(8, "0000056");
    revision = rendered.track_revision;
    text(8, "0000056");
    assert(rendered.track_revision == revision && cover_requests == requests + 3);
    text(8, "0000056");
    assert(cover_requests == requests + 3);
    puts("PASS: singer/heading supplementation keeps cover; new handle changes once; failed command retries");

    char long_heading[300];
    memset(long_heading, 'a', sizeof(long_heading));
    long_heading[254] = (char)0xe6; long_heading[255] = (char)0x99; long_heading[256] = (char)0xb4;
    pet_dashboard_music_receive_text(2, (const uint8_t *)long_heading, sizeof(long_heading));
    assert(strlen(rendered.title) == 254 && strlen(rendered.artist) == 127);
    revision = rendered.track_revision;
    unsigned resets = album_resets;
    pet_dashboard_music_receive_text(1, (const uint8_t *)long_heading, sizeof(long_heading));
    assert(strlen(rendered.lyric) == 254 && rendered.track_revision == revision && album_resets == resets);
    const uint8_t invalid[] = {0xff, 'A', '\n', 'B', 0xc0, 0xaf};
    pet_dashboard_music_receive_text(1, invalid, sizeof(invalid));
    assert(!strcmp(rendered.lyric, "A B"));
    text(0x80, "explicit extra lyric");
    assert(!strcmp(rendered.lyric, "explicit extra lyric"));
    assert(pet_dashboard_music_receive_text(1, NULL, 1) == -1);
    assert(pet_dashboard_music_receive_text(3, NULL, 0) == -1);
    reject_next = true;
    text(1, "retry lyric");
    assert(strcmp(rendered.lyric, "retry lyric"));
    pet_dashboard_music_poll(200);
    assert(!strcmp(rendered.lyric, "retry lyric") && album_resets == resets);
    pet_dashboard_music_update(false, false, 300);
    assert(!rendered.title[0] && !rendered.artist[0] && !rendered.lyric[0] && !rendered.cover_handle[0]);
    assert(album_resets == resets + 1);
    text(1, "late disconnected text");
    assert(!rendered.lyric[0]);
    pet_dashboard_music_update(true, true, 400);
    text(2, "reconnected song - singer");
    text(8, "0000056");
    assert(cover_requests == requests + 4);
    assert(!rendered.lyric[0]);
    pair("reconnected lyric", "reconnected song - singer");
    assert(!strcmp(rendered.lyric, "reconnected lyric"));
    puts("PASS: full heading/UTF-8 limits, retry, disconnect/reconnect and extra lyric input");
    return 0;
}
