#if !LV_USE_GUIBUILDER_SIMULATOR
#include "lv_ui.h"
#endif
#include "chongwuyibiao_yinyue_donghua.h"

#define NOTE_LIFE_MS 1100U
#define ALBUM_TURN_MS 18000U

static lv_obj_t *album;
static lv_obj_t *notes[2];
static lv_timer_t *animation_timer;
static uint32_t previous_tick;
static uint32_t album_time;
static uint32_t note_time;
static bool running;

/** @brief 根据生命进度设置音符位置和透明度，两个音符交替出现。 */
static void draw_notes(void)
{
    unsigned index = note_time / NOTE_LIFE_MS;
    uint32_t age = note_time % NOTE_LIFE_MS;
    unsigned i;
    for (i = 0; i < 2; i++) {
        if (!notes[i]) continue;
        if (!running || i != index) {
            lv_obj_add_flag(notes[i], LV_OBJ_FLAG_HIDDEN);
            continue;
        }
        lv_obj_remove_flag(notes[i], LV_OBJ_FLAG_HIDDEN);
        int32_t drift = (int32_t)(age * 14U / NOTE_LIFE_MS);
        lv_obj_set_pos(notes[i], i == 0 ? 454 + drift : 478 - drift,
                       78 - (int32_t)(age * 58U / NOTE_LIFE_MS));
        uint32_t opacity = age < 150U ? age * 255U / 150U :
                           age > 600U ? (NOTE_LIFE_MS - age) * 255U / 500U : 255U;
        lv_obj_set_style_image_opa(notes[i], (lv_opa_t)opacity, LV_PART_MAIN);
    }
}

/** @brief 只推进视觉动画，速度以实际经过的毫秒数计算。 */
static void animation_tick(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    uint32_t now = lv_tick_get();
    uint32_t elapsed = now - previous_tick;
    previous_tick = now;
    album_time = (album_time + elapsed) % ALBUM_TURN_MS;
    note_time = (note_time + elapsed) % (NOTE_LIFE_MS * 2U);
    if (album) lv_image_set_rotation(album, album_time * 3600U / ALBUM_TURN_MS);
    draw_notes();
}

/** @brief 播放且页面可见时运行；暂停保留唱片角度并隐藏音符。 */
void chongwuyibiao_yinyue_donghua_yunxu(bool enabled)
{
    if (!animation_timer || running == enabled) return;
    running = enabled;
    note_time = 0;
    if (enabled) {
        previous_tick = lv_tick_get();
        lv_timer_reset(animation_timer);
        lv_timer_resume(animation_timer);
    } else lv_timer_pause(animation_timer);
    draw_notes();
}

/** @brief 换歌或更换图片后重设唱片角度。 */
void chongwuyibiao_yinyue_donghua_chongzhi(void)
{
    album_time = 0;
    if (album) lv_image_set_rotation(album, 0);
}

/** @brief 页面销毁时删除定时器，避免访问已释放控件。 */
void chongwuyibiao_yinyue_donghua_jieshu(void)
{
    if (animation_timer) lv_timer_delete(animation_timer);
    animation_timer = NULL;
    album = notes[0] = notes[1] = NULL;
    running = false;
}

/** @brief 绑定工具生成的图片；等待播放状态开启动画。 */
void chongwuyibiao_yinyue_donghua_bangding(lv_ui_chongwuyibiao *ui)
{
    chongwuyibiao_yinyue_donghua_jieshu();
    album = ui->chongwuyibiao_zhuanjifengmian;
    notes[0] = ui->chongwuyibiao_yinfuyou;
    notes[1] = ui->chongwuyibiao_yinfuzuo;
    album_time = note_time = 0;
    animation_timer = lv_timer_create(animation_tick, 30, NULL);
    if (animation_timer) lv_timer_pause(animation_timer);
    if (album) lv_image_set_pivot(album, 48, 48);
    draw_notes();
}
