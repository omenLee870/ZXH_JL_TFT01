#if !LV_USE_GUIBUILDER_SIMULATOR
#include "lv_ui.h"
#include "app_config.h"
#include "lv_port_disp.h"
#include "asm/dcache.h"
#if TCFG_PSRAM_DEV_ENABLE
#include "asm/psram_api.h"
#endif
#endif
#include "chongwuyibiao_yinyue.h"
#include "chongwuyibiao_donghua.h"
#include "chongwuyibiao_yinyue_donghua.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if LV_USE_GUIBUILDER_SIMULATOR
#include "../../../SDK/apps/watch/common/pet_dashboard_music.h"
#else
#include "pet_dashboard_music.h"
#include "pet_reverse_notice.h"
void ui_music_album_pic_handler(void);
#endif

#define VIEW_CONNECTED 1U
#define VIEW_PLAYING 2U
#define VIEW_MUSIC 4U
#define COVER_BYTES (96U * 96U * 2U)
#define COVER_ALIGN (LV_DRAW_BUF_ALIGN > 32 ? LV_DRAW_BUF_ALIGN : 32)
#define LYRIC_WAIT_MS 1000U

typedef struct {
    lv_obj_t *pingmu;
    lv_obj_t *caodi;
    lv_obj_t *xiaogou;
    lv_obj_t *yinyue;
    lv_obj_t *tinggexiaogou;
    lv_obj_t *lanyatubiao;
    lv_obj_t *bofangjian;
    lv_obj_t *zantingjian;
    lv_obj_t *shangyiqu;
    lv_obj_t *xiayiqu;
    lv_obj_t *title;
    lv_obj_t *lyric;
    lv_obj_t *cover;
    lv_timer_t *lyric_wait_timer;
    const void *default_cover;
    char default_cover_path[256];
    bool loaded;
    bool xianshi;
    bool cover_draw_pending;
    bool lyric_state_known;
} yinyue_shitu_t;

static yinyue_shitu_t shitu;
static pet_dashboard_music_view_t latest_view;
static lv_image_dsc_t cover_descriptor;
/* 独立封面缓存只申请一次，页面切换和 demo 缓冲区扩容不会使其失效。 */
static void *cover_storage;
static uint16_t *cover_pixels;
static bool cover_ready;
static lv_obj_t *daoche_return_page;

/** @brief 退出本次倒车页后取消往返标记，不影响其他入口进入的视频页。 */
static void daoche_unload(lv_event_t *event)
{
    LV_UNUSED(event);
#if !LV_USE_GUIBUILDER_SIMULATOR
    if (daoche_return_page) pet_reverse_notice_request(false);
#endif
    daoche_return_page = NULL;
}

/** @brief 暂时切到现有摄像头页，返回复用原宠物页面及业务状态。 */
bool chongwuyibiao_daoche_toggle(bool clicked)
{
    gui_scr_t *current = gui_scr_get_act();
    if (!current) return false;
    if (current->id == GUI_SCREEN_CHONGWUYIBIAO) {
        if (!clicked) return true;
        gui_scr_t *video = ui_get_setup_scr(GUI_SCREEN_PAGE_VIDEO);
        if (!video || !video->scr) {
            printf("[PET_KEY] OK camera page create failed\n");
            return true;
        }
        lv_obj_remove_event_cb(video->scr, daoche_unload);
        lv_obj_add_event_cb(video->scr, daoche_unload, LV_EVENT_SCREEN_UNLOAD_START, NULL);
        daoche_return_page = current->scr;
        /* 不清控件、不删除页面、不改音乐状态，摄像头沿用页面加载回调。 */
        ui_load_scr_anim(&guider_ui, video, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
        if (gui_scr_get_act() != video) daoche_return_page = NULL;
#if !LV_USE_GUIBUILDER_SIMULATOR
        else pet_reverse_notice_request(true);
#endif
        printf("[PET_KEY] OK pet -> camera switched=%u\n", gui_scr_get_act() == video);
        return true;
    }
    if (current->id == GUI_SCREEN_PAGE_VIDEO && daoche_return_page) {
        if (!clicked) return true;
        gui_scr_t *pet = gui_scr_get(GUI_SCREEN_CHONGWUYIBIAO);
        if (pet && pet->scr == daoche_return_page && lv_obj_is_valid(pet->scr)) {
            ui_load_scr_anim(&guider_ui, pet, LV_SCR_LOAD_ANIM_NONE, 0, 0, false, false, false);
            printf("[PET_KEY] OK camera -> pet switched=%u\n", gui_scr_get_act() == pet);
        } else {
            printf("[PET_KEY] OK saved pet page missing\n");
        }
        return true;
    }
    return false;
}

static bool cover_wait_idle(void)
{
#if !LV_USE_GUIBUILDER_SIMULATOR && LV_USE_DRAW_JLGPU_LIST == 2
    return lv_jlgpu_run_disp_wait_idle();
#else
    return true;
#endif
}

/** @brief 与 demo 使用相同的 PSRAM 分配方式，并保留独立、对齐的像素缓存。 */
static bool cover_buffer_prepare(void)
{
    if (cover_pixels) return true;
#if !LV_USE_GUIBUILDER_SIMULATOR && TCFG_PSRAM_DEV_ENABLE
    cover_storage = malloc_psram(COVER_BYTES + COVER_ALIGN - 1U);
#else
    cover_storage = malloc(COVER_BYTES + COVER_ALIGN - 1U);
#endif
    if (!cover_storage) return false;
    cover_pixels = (uint16_t *)LV_ROUND_UP((uintptr_t)cover_storage, COVER_ALIGN);
    return true;
}

/** @brief 设置默认唱片或收到的专辑封面，统一使用 96 像素旋转中心。 */
static void cover_refresh(void)
{
    if (!shitu.cover) return;
    const void *source = cover_ready ? &cover_descriptor : shitu.default_cover;
    if (!cover_wait_idle()) return;
    /* 先清理旧资源，避免 SDK 切回 BIN 时把静态图片描述符当成字符串释放。 */
    if (lv_image_get_src(shitu.cover) != source) lv_image_set_src(shitu.cover, NULL);
    lv_image_set_src(shitu.cover, source);
    lv_image_set_scale(shitu.cover, 256);
    lv_image_set_pivot(shitu.cover, 48, 48);
}

/** @brief 接收解码器的 RGB565 图片，复制像素避免复用旧页缓冲区。 */
void chongwuyibiao_yinyue_fengmian(const lv_image_dsc_t *image)
{
    if (!latest_view.connected || !image || !image->data || image->header.w != 96 || image->header.h != 96 ||
        image->data_size < COVER_BYTES || image->header.cf != LV_COLOR_FORMAT_RGB565) {
#if !LV_USE_GUIBUILDER_SIMULATOR
        printf("[PET_MUSIC] cover reject connected=%u w=%u h=%u cf=%u bytes=%u\n",
               latest_view.connected, image ? image->header.w : 0,
               image ? image->header.h : 0, image ? image->header.cf : 0,
               image ? image->data_size : 0);
#endif
        return;
    }
    if (!cover_buffer_prepare()) {
#if !LV_USE_GUIBUILDER_SIMULATOR
        printf("[PET_MUSIC] cover buffer alloc failed bytes=%u\n", COVER_BYTES);
#endif
        return;
    }
    /* LIST==2 在另一线程读这块像素；等上一帧真正完成后再覆盖。 */
    if (!cover_wait_idle()) return;
    lv_image_cache_drop(&cover_descriptor);
    memcpy(cover_pixels, image->data, COVER_BYTES);
#if !LV_USE_GUIBUILDER_SIMULATOR
    /* CPU 写入后再交给 GPU，刷新像素缓存而非仅清理 LVGL 图片缓存。 */
    unsigned int in_psram = 0;
#if TCFG_PSRAM_DEV_ENABLE
    in_psram = psram_cache_check(cover_pixels) || psram_no_cache_check(cover_pixels);
    if (psram_cache_check(cover_pixels)) psram_flush_cache(cover_pixels, COVER_BYTES);
    else
#endif
        DcuFlushRegion((unsigned int *)cover_pixels, COVER_BYTES);
    uint16_t minimum = 0xffff, maximum = 0;
    unsigned int i;
    for (i = 0; i < COVER_BYTES / sizeof(cover_pixels[0]); i++) {
        if (cover_pixels[i] < minimum) minimum = cover_pixels[i];
        if (cover_pixels[i] > maximum) maximum = cover_pixels[i];
    }
    printf("[PET_MUSIC] cover buffer addr=%x align=%u bytes=%u psram=%u min=%04x max=%04x\n",
           (unsigned int)cover_pixels, (unsigned int)cover_pixels % LV_DRAW_BUF_ALIGN,
           COVER_BYTES, in_psram, minimum, maximum);
#endif
    memset(&cover_descriptor, 0, sizeof(cover_descriptor));
    cover_descriptor.header.magic = LV_IMAGE_HEADER_MAGIC;
    cover_descriptor.header.cf = LV_COLOR_FORMAT_RGB565;
    cover_descriptor.header.w = cover_descriptor.header.h = 96;
    cover_descriptor.header.stride = 96 * 2;
    cover_descriptor.data_size = COVER_BYTES;
    cover_descriptor.data = (const uint8_t *)cover_pixels;
    cover_ready = true;
    shitu.cover_draw_pending = true;
    cover_refresh();
#if !LV_USE_GUIBUILDER_SIMULATOR
    printf("[PET_MUSIC] cover applied rev=%u loaded=%u visible=%u widget=%u source=%u\n",
           latest_view.track_revision, shitu.loaded,
           shitu.cover ? lv_obj_is_visible(shitu.cover) : 0, shitu.cover != NULL,
           shitu.cover && lv_image_get_src(shitu.cover) == &cover_descriptor);
#endif
    chongwuyibiao_yinyue_donghua_chongzhi();
}

/** @brief 只接受当前状态对应的封面，旧解码结果不覆盖新歌曲。 */
void chongwuyibiao_yinyue_fengmian_identified(const lv_image_dsc_t *image, const char *handle)
{
    if (!handle || strcmp(latest_view.cover_handle, handle)) return;
    chongwuyibiao_yinyue_fengmian(image);
}

#if !LV_USE_GUIBUILDER_SIMULATOR
/** @brief 每张封面只记录一次绘制任务，核对圆形图片是否直接绘入主图层。 */
static void cover_draw_event(lv_event_t *event)
{
    if (!shitu.cover_draw_pending || lv_event_get_target(event) != shitu.cover) return;
    lv_draw_task_t *task = lv_event_get_draw_task(event);
    if (!task || lv_draw_task_get_type(task) != LV_DRAW_TASK_TYPE_IMAGE) return;
    const lv_draw_image_dsc_t *draw = lv_draw_task_get_draw_dsc(task);
    if (!draw || draw->src != &cover_descriptor) return;
    lv_area_t area;
    lv_draw_task_get_area(task, &area);
    printf("[PET_MUSIC] cover draw rev=%u direct=%u radius=%d angle=%d area=%d,%d,%d,%d\n",
           latest_view.track_revision, draw->base.layer && !draw->base.layer->parent,
           (int)draw->clip_radius, (int)draw->rotation,
           (int)area.x1, (int)area.y1, (int)area.x2, (int)area.y2);
    shitu.cover_draw_pending = false;
}
#endif

/** @brief 转交应用请求；工具独立预览没有应用任务。 */
static void yingyong_qingqiu(pet_dashboard_music_request_t request)
{
#if !LV_USE_GUIBUILDER_SIMULATOR || defined(PET_DASHBOARD_MUSIC_LINKED)
    pet_dashboard_music_request(request);
#else
    LV_UNUSED(request);
#endif
}

/** @brief 设置单个控件的可见性。 */
static void xianshi_kongzhi(lv_obj_t *kongjian, bool xianshi)
{
    if (xianshi) lv_obj_remove_flag(kongjian, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(kongjian, LV_OBJ_FLAG_HIDDEN);
}

/** @brief 按应用层决定切换控件组，并暂停或恢复主界面动画。 */
static void jiemian_qiehuan(bool yinyue)
{
    shitu.xianshi = yinyue;
    xianshi_kongzhi(shitu.yinyue, yinyue);
    xianshi_kongzhi(shitu.caodi, !yinyue);
    xianshi_kongzhi(shitu.xiaogou, !yinyue);
    chongwuyibiao_donghua_yunxu(!yinyue);
}

/** @brief 歌词到达、断开或删除页面时取消等待，避免回调访问旧控件。 */
static void lyric_wait_cancel(void)
{
    if (!shitu.lyric_wait_timer) return;
    lv_timer_delete(shitu.lyric_wait_timer);
    shitu.lyric_wait_timer = NULL;
}

/** @brief 空歌词只表示尚未收到，短暂等待后才显示无歌词提示。 */
static void lyric_wait_timeout(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    lyric_wait_cancel();
    if (shitu.lyric && latest_view.connected && !latest_view.lyric[0]) {
        lv_label_set_text(shitu.lyric, "暂无歌词");
    }
}

/** @brief 在 LVGL 线程绘制状态，页面未创建时仅保存快照。 */
static void jiemian_shuaxin(const pet_dashboard_music_view_t *view)
{
    bool changed_track = latest_view.track_revision != view->track_revision;
    bool wait_for_lyric = !shitu.lyric_state_known || !latest_view.connected ||
                          strcmp(latest_view.title, view->title) ||
                          (latest_view.lyric[0] && !view->lyric[0]);
    latest_view = *view;
    if (changed_track || !view->connected) {
        cover_ready = false;
        shitu.cover_draw_pending = false;
        cover_refresh();
        chongwuyibiao_yinyue_donghua_chongzhi();
    }
    if (shitu.pingmu == NULL) return;
    shitu.lyric_state_known = true;
    if (!view->connected || view->lyric[0]) {
        lyric_wait_cancel();
    } else if (wait_for_lyric) {
        lyric_wait_cancel();
        shitu.lyric_wait_timer = lv_timer_create(lyric_wait_timeout, LYRIC_WAIT_MS, NULL);
    }
    uint32_t flags = (view->connected ? VIEW_CONNECTED : 0) |
                     (view->playing ? VIEW_PLAYING : 0) |
                     (view->music_visible ? VIEW_MUSIC : 0);
    const char *title = view->title[0] ? view->title : "等待歌曲信息";
    const char *lyric = view->lyric[0] ? view->lyric :
                        (shitu.lyric_wait_timer ? "" : "暂无歌词");
    if (strcmp(lv_label_get_text(shitu.title), title)) lv_label_set_text(shitu.title, title);
    if (strcmp(lv_label_get_text(shitu.lyric), lyric)) lv_label_set_text(shitu.lyric, lyric);
    xianshi_kongzhi(shitu.lanyatubiao, (flags & VIEW_CONNECTED) != 0);
    xianshi_kongzhi(shitu.zantingjian, (flags & VIEW_PLAYING) != 0);
    xianshi_kongzhi(shitu.bofangjian, (flags & VIEW_PLAYING) == 0);
    if (shitu.xianshi != ((flags & VIEW_MUSIC) != 0)) {
        jiemian_qiehuan((flags & VIEW_MUSIC) != 0);
    }
    chongwuyibiao_yinyue_donghua_yunxu(shitu.loaded && view->playing && view->music_visible);
#if !LV_USE_GUIBUILDER_SIMULATOR
    /* 图片可能先于状态快照完成；状态到达后补用同句柄的接收缓存。 */
    if (changed_track && shitu.loaded && view->connected) ui_music_album_pic_handler();
#endif
}

#if !LV_USE_GUIBUILDER_SIMULATOR
/** @brief 接收应用线程投递的值快照，不跨线程传控件指针。 */
static void jiemian_rpc(void *snapshot, ...)
{
    extern int app_meter_control_media_allowed(void);
    if (app_meter_control_media_allowed()) jiemian_shuaxin(snapshot);
    free(snapshot);
}
#endif

/** @brief 应用层的显示出口，只转发快照，不读取蓝牙或判断业务。 */
int pet_dashboard_music_notify(const pet_dashboard_music_view_t *view)
{
#if LV_USE_GUIBUILDER_SIMULATOR
    jiemian_shuaxin(view);
    return 0;
#else
    pet_dashboard_music_view_t *snapshot = malloc(sizeof(*snapshot));
    if (!snapshot) return -1;
    *snapshot = *view;
    int ret = lvgl_rpc_post_func(jiemian_rpc, 1, (int)snapshot);
    if (ret) free(snapshot);
    return ret;
#endif
}

/** @brief 转交小狗点击，由应用层决定是否切换显示。 */
static void xiaogou_dianji(lv_event_t *shijian)
{
    LV_UNUSED(shijian);
    yingyong_qingqiu(PET_MUSIC_TOGGLE_VIEW);
}

/** @brief 转交音乐按钮点击，不提前改变播放状态。 */
static void bofang_kongzhi(lv_event_t *shijian)
{
    lv_obj_t *anniu = lv_event_get_target(shijian);
    pet_dashboard_music_request_t mingling;
    if (anniu == shitu.shangyiqu) mingling = PET_MUSIC_PREVIOUS;
    else if (anniu == shitu.xiayiqu) mingling = PET_MUSIC_NEXT;
    else if (anniu == shitu.zantingjian) mingling = PET_MUSIC_PAUSE;
    else mingling = PET_MUSIC_PLAY;
    yingyong_qingqiu(mingling);
}

/** @brief 页面重入请求当前状态，删除时释放显示层引用。 */
static void pingmu_shijian(lv_event_t *shijian)
{
    if (lv_event_get_target(shijian) != shitu.pingmu) return;
    switch (lv_event_get_code(shijian)) {
    case LV_EVENT_SCREEN_LOADED:
        shitu.loaded = true;
        jiemian_shuaxin(&latest_view);
        yingyong_qingqiu(PET_MUSIC_REFRESH);
#if !LV_USE_GUIBUILDER_SIMULATOR
        /* 页面外收到的 JPEG 在回来时补显，缓存已由换歌事件失效。 */
        if (!cover_ready && latest_view.connected) ui_music_album_pic_handler();
#endif
        break;
    case LV_EVENT_SCREEN_UNLOAD_START:
        shitu.loaded = false;
        lyric_wait_cancel();
        chongwuyibiao_yinyue_donghua_yunxu(false);
        break;
    case LV_EVENT_DELETE:
        lyric_wait_cancel();
        chongwuyibiao_yinyue_donghua_jieshu();
        lv_memset(&shitu, 0, sizeof(shitu));
        break;
    default:
        break;
    }
}

/** @brief 绑定工具生成的控件，业务状态不随页面重建而重置。 */
void chongwuyibiao_yinyue_bangding(lv_ui_chongwuyibiao *jiemian)
{
    if (jiemian == NULL || jiemian->chongwuyibiao == NULL) return;
    if (shitu.pingmu == jiemian->chongwuyibiao) return;
    lyric_wait_cancel();
    lv_memset(&shitu, 0, sizeof(shitu));
    shitu.pingmu = jiemian->chongwuyibiao;
    shitu.caodi = jiemian->chongwuyibiao_caodirongqi;
    shitu.xiaogou = jiemian->chongwuyibiao_xiaogou;
    shitu.yinyue = jiemian->chongwuyibiao_yinyuerongqi;
    shitu.tinggexiaogou = jiemian->chongwuyibiao_tinggexiaogou;
    shitu.lanyatubiao = jiemian->chongwuyibiao_lanyalianjietubiao;
    shitu.bofangjian = jiemian->chongwuyibiao_bofangjian;
    shitu.zantingjian = jiemian->chongwuyibiao_zantingjian;
    shitu.shangyiqu = jiemian->chongwuyibiao_shangyiqu;
    shitu.xiayiqu = jiemian->chongwuyibiao_xiayiqu;
    shitu.title = jiemian->chongwuyibiao_gequmingcheng;
    shitu.lyric = jiemian->chongwuyibiao_geciwenzi;
    shitu.cover = jiemian->chongwuyibiao_zhuanjifengmian;
#if !LV_USE_GUIBUILDER_SIMULATOR
    lv_obj_add_flag(shitu.cover, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(shitu.cover, cover_draw_event, LV_EVENT_DRAW_TASK_ADDED, NULL);
#endif
    shitu.default_cover = lv_image_get_src(shitu.cover);
    if (shitu.default_cover && lv_image_src_get_type(shitu.default_cover) == LV_IMAGE_SRC_FILE) {
        snprintf(shitu.default_cover_path, sizeof(shitu.default_cover_path), "%s",
                 (const char *)shitu.default_cover);
        shitu.default_cover = shitu.default_cover_path;
    }
    shitu.loaded = lv_screen_active() == shitu.pingmu;
    chongwuyibiao_yinyue_donghua_bangding(jiemian);
    cover_refresh();

    /* 工具保留音乐预览；运行时由容器统一控制草地显示。 */
    xianshi_kongzhi(jiemian->chongwuyibiao_caodi, true);
    xianshi_kongzhi(jiemian->chongwuyibiao_yun1, true);
    xianshi_kongzhi(jiemian->chongwuyibiao_yun2, true);
    xianshi_kongzhi(jiemian->chongwuyibiao_yun3, true);
    xianshi_kongzhi(jiemian->chongwuyibiao_yun4, true);
    jiemian_qiehuan(false);
    jiemian_shuaxin(&latest_view);

    lv_obj_t *anniu[] = { shitu.shangyiqu, shitu.bofangjian,
                         shitu.zantingjian, shitu.xiayiqu };
    unsigned int i;
    for (i = 0; i < sizeof(anniu) / sizeof(anniu[0]); i++) {
        lv_obj_add_flag(anniu[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_ext_click_area(anniu[i], 12);
        lv_obj_add_event_cb(anniu[i], bofang_kongzhi, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_add_flag(shitu.xiaogou, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(shitu.tinggexiaogou, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(shitu.xiaogou, xiaogou_dianji, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(shitu.tinggexiaogou, xiaogou_dianji, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(shitu.pingmu, pingmu_shijian, LV_EVENT_ALL, NULL);
    yingyong_qingqiu(PET_MUSIC_REFRESH);
}
