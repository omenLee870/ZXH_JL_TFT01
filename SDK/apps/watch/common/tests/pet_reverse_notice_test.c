#include "pet_reverse_notice.h"
#include "pet_dashboard_music.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool connected, playing, ready;
static unsigned pauses, resumes, starts, stops;
static int pause_result, resume_result, play_result;
static uint8_t phone[6];
static uint32_t now, current_generation;

bool pet_reverse_notice_media_state(uint8_t address[6], bool *is_playing)
{
    memcpy(address, phone, 6);
    *is_playing = playing;
    return connected;
}
int pet_reverse_notice_media_command(bool play)
{
    if (play) { resumes++; return resume_result; }
    pauses++;
    return pause_result;
}
bool pet_reverse_notice_resource_ready(void) { return ready; }
int pet_reverse_notice_play(uint32_t generation)
{
    starts++;
    current_generation = generation;
    return play_result;
}
void pet_reverse_notice_stop(uint32_t generation)
{
    assert(generation == current_generation);
    stops++;
    pet_reverse_notice_done(generation);
}
static void tick(unsigned elapsed)
{
    now += elapsed;
    pet_reverse_notice_tick(now);
    pet_dashboard_music_update(connected, playing, now);
}
static void setup(bool is_playing)
{
    pet_reverse_notice_handle(false, now);
    playing = false;
    pet_reverse_notice_done(current_generation);
    tick(3000);
    tick(3000);
    assert(!pet_reverse_notice_preserve_music_view());
    connected = ready = true;
    playing = is_playing;
    memset(phone, 1, sizeof(phone));
    pauses = resumes = starts = stops = 0;
    pause_result = resume_result = play_result = 0;
    pet_dashboard_music_reset();
    pet_dashboard_music_update(connected, playing, now);
}
static void enter(void) { pet_reverse_notice_handle(true, now); }
static void leave(void) { pet_reverse_notice_handle(false, now); }
static void pause_ack(void) { playing = false; tick(100); }
static void done(void) { pet_reverse_notice_done(current_generation); tick(100); }

int main(void)
{
    setup(true);
    enter();
    assert(pauses == 1 && starts == 0);
    tick(1000);
    assert(starts == 0 && resumes == 0);
    pause_ack();
    assert(starts == 1 && resumes == 0);
    tick(4000);
    assert(pet_dashboard_music_get_view()->music_visible);
    uint32_t first_generation = current_generation;
    done();
    assert(resumes == 0 && stops == 0 && starts == 1);
    tick(2999);
    assert(starts == 1 && resumes == 0);
    tick(1);
    assert(starts == 2 && pauses == 1 && current_generation != first_generation);
    pet_reverse_notice_done(first_generation);
    tick(3100);
    assert(starts == 2 && resumes == 0);
    done(); tick(3000);
    assert(starts == 3 && pauses == 1 && resumes == 0);
    leave();
    assert(resumes == 1 && stops == 1);
    playing = true; tick(100); tick(2100);
    assert(!pet_reverse_notice_preserve_music_view());

    /* 手动草地选择不被临时暂停和恢复抢回音乐页。 */
    setup(true);
    pet_dashboard_music_request(PET_MUSIC_TOGGLE_VIEW);
    assert(!pet_dashboard_music_get_view()->music_visible);
    enter(); pause_ack(); tick(4000); done();
    tick(3000); done();
    assert(resumes == 0 && !pet_dashboard_music_get_view()->music_visible);
    leave();
    assert(resumes == 1 && stops == 0); /* 间隔中退出不再操作已结束的播放器。 */
    playing = true; tick(100); tick(2100);
    assert(!pet_dashboard_music_get_view()->music_visible);

    setup(false); enter();
    assert(starts == 1 && pauses == 0);
    done(); tick(3000); assert(starts == 2 && resumes == 0 && pauses == 0);
    leave(); tick(4000); assert(starts == 2 && resumes == 0);

    setup(false); connected = false; enter();
    assert(starts == 1 && pauses == 0);
    done(); tick(3000); assert(starts == 2 && resumes == 0);
    leave(); tick(4000); assert(starts == 2);

    setup(true); ready = false; enter();
    assert(pauses == 0 && starts == 0 && !pet_reverse_notice_preserve_music_view());

    setup(true); pause_result = -1; enter();
    assert(pauses == 1 && starts == 0 && resumes == 0);

    setup(true); play_result = -1; enter(); pause_ack();
    assert(starts == 1 && resumes == 1);

    setup(true); enter(); pause_ack(); connected = false; tick(100); done();
    leave();
    assert(resumes == 0);

    setup(true); enter(); pause_ack(); phone[0] = 2; done();
    leave();
    assert(resumes == 0);

    setup(true); enter(); leave();
    assert(resumes == 0 && starts == 0);
    pause_ack();
    assert(resumes == 1 && starts == 0);

    setup(true); enter(); pause_ack(); leave();
    assert(stops == 1 && resumes == 1);
    tick(100); assert(resumes == 1);

    setup(true); enter(); tick(1600);
    assert(starts == 0 && resumes == 1);

    setup(true); enter(); pause_ack(); tick(12000);
    assert(stops == 1 && resumes == 1);

    /* 恢复等待期间重新进入，等 PLAY 反馈后重新 PAUSE；旧回调不能结束新播报。 */
    setup(true); enter(); pause_ack();
    uint32_t old_generation = current_generation;
    leave(); enter();
    assert(pauses == 1 && starts == 1);
    playing = true; tick(100);
    assert(pauses == 2 && starts == 1);
    pause_ack();
    assert(starts == 2 && current_generation != old_generation);
    pet_reverse_notice_done(old_generation); tick(100);
    assert(resumes == 1);
    done(); assert(resumes == 1);
    leave(); assert(resumes == 2);

    setup(true); enter(); leave(); enter(); pause_ack();
    assert(pauses == 1 && starts == 1 && resumes == 0);
    enter(); assert(starts == 1);
    playing = true; done(); assert(resumes == 0);
    leave(); assert(resumes == 0);

    setup(true); enter(); pause_ack(); resume_result = -1; done();
    assert(resumes == 0);
    leave();
    assert(resumes == 1 && !pet_reverse_notice_preserve_music_view());

    /* 间隔遇到其他提示音时不排队，音乐保持暂停，资源空闲后再循环。 */
    setup(true); enter(); pause_ack(); done(); ready = false;
    tick(3000); tick(3000);
    assert(starts == 1 && resumes == 0);
    ready = true; tick(3000);
    assert(starts == 2 && pauses == 1);
    leave(); assert(resumes == 1);

    /* 重播启动失败也要释放本次暂停所有权，不继续空转。 */
    setup(true); enter(); pause_ack(); done(); play_result = -1;
    tick(3000);
    assert(starts == 2 && resumes == 1);
    tick(3000); assert(starts == 2);

    setup(true);
    now = UINT32_MAX - 200U;
    enter(); pause_ack(); done();
    tick(2999); assert(starts == 1);
    tick(1); assert(starts == 2 && resumes == 0);
    leave(); assert(resumes == 1);

    setup(true);
    now = UINT32_MAX - 100U;
    enter(); pause_ack(); tick(12000);
    assert(stops == 1 && resumes == 1);

    /* 模拟消息队列投递失败：播放中退出仍由周期检测停止并恢复音乐。 */
    setup(true); enter(); pause_ack();
    assert(pet_reverse_notice_request(false) == -1);
    tick(100);
    assert(stops == 1 && resumes == 1);

    /* 间隔中退出投递失败也不能再启动下一轮。 */
    setup(true);
    assert(pet_reverse_notice_request(true) == -1);
    enter(); pause_ack(); done();
    assert(pet_reverse_notice_request(false) == -1);
    tick(100); tick(4000);
    assert(starts == 1 && stops == 0 && resumes == 1);
    puts("reverse notice tests passed (repeat interval/boundary/wrap, pause held across repeats, exit during playback/gap, page state, resource failure, peer change, stale callback, timeouts)");
    return 0;
}
