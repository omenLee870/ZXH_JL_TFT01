#include "chongwuyibiao_donghua.h"
#include "neicun_zhuizong.h"
#include "ui_pet_hero.h"
#if !LV_USE_GUIBUILDER_SIMULATOR || defined(YOUYIBAO_UI_TEST)
#include "youyibao_sif_ui.h"
#endif

#define YUN_SHULIANG 4
#define YUN_BUJIN_MS 50U
#define YUN_ZUO 100
#define YUN_DANRU 120
#define YUN_DANCHU 620
#define YUN_YOU 640

typedef enum {
    ZHAYAN_DENGDAI,
    ZHAYAN_BIYAN,
    ZHAYAN_JIANGE
} zhayan_jieduan_t;

typedef struct {
    lv_obj_t *tupian;
    int32_t zhongxin;
    int32_t kuandu;
} yun_zhuangtai_t;

typedef struct {
    lv_obj_t *pingmu;
    lv_obj_t *xiaogou;
    lv_obj_t *chesushuzi;
    lv_timer_t *yun_dingshiqi;
    lv_timer_t *zhayan_dingshiqi;
    yun_zhuangtai_t yun[YUN_SHULIANG];
    uint32_t yun_shijian;
    uint16_t chesu;
    uint8_t zhayan_cishu;
    zhayan_jieduan_t zhayan_jieduan;
    bool yunxing;
    bool paobu;
    bool zanting;
} donghua_zhuangtai_t;

static donghua_zhuangtai_t zhuangtai;

static void huxi_suofang(void *tupian, int32_t bili)
{
    lv_image_set_scale((lv_obj_t *)tupian, (uint32_t)bili);
}

static void huxi_kaishi(void)
{
    lv_anim_t huxi;
    lv_anim_init(&huxi);
    lv_anim_set_var(&huxi, zhuangtai.xiaogou);
    lv_anim_set_exec_cb(&huxi, huxi_suofang);
    lv_anim_set_values(&huxi, 256, 261);
    lv_anim_set_duration(&huxi, 500);
    lv_anim_set_playback_duration(&huxi, 700);
    lv_anim_set_repeat_delay(&huxi, 800);
    lv_anim_set_repeat_count(&huxi, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&huxi, lv_anim_path_linear);
    lv_anim_start(&huxi);
}

static uint16_t chesu_duqu(void)
{
    const char *wenzi = lv_label_get_text(zhuangtai.chesushuzi);
    uint32_t chesu = 0;
    /* P、R 等非数字显示按停车处理，避免错误启动跑步。 */
    while (*wenzi >= '0' && *wenzi <= '9') {
        chesu = chesu * 10U + (uint32_t)(*wenzi++ - '0');
        if (chesu > 65535U) return 65535U;
    }
    return (uint16_t)chesu;
}

/** @brief 特殊动作显示时隐藏坐姿，恢复静止时沿用原图、眨眼和呼吸参数。 */
static void xiaogou_qiehuan(bool qiangzhi)
{
    bool paobu = ui_pet_hero_active();
    lv_obj_t *xiaogou = zhuangtai.xiaogou;
    /* 音乐模块也会切换原图显隐，每次恢复时先修复互斥，避免两只狗重叠。 */
    if (paobu) lv_obj_add_flag(xiaogou, LV_OBJ_FLAG_HIDDEN);
    else if (!zhuangtai.zanting) lv_obj_remove_flag(xiaogou, LV_OBJ_FLAG_HIDDEN);
    if (!qiangzhi && zhuangtai.paobu == paobu) return;

    zhuangtai.paobu = paobu;
    zhuangtai.zhayan_cishu = 0;
    zhuangtai.zhayan_jieduan = ZHAYAN_DENGDAI;
    lv_timer_pause(zhuangtai.zhayan_dingshiqi);
    lv_anim_delete(xiaogou, huxi_suofang);
    lv_image_set_scale(xiaogou, 256);

    if (paobu) {
        lv_obj_add_flag(xiaogou, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(xiaogou, LV_OBJ_FLAG_HIDDEN);
        lv_image_set_src(xiaogou, gui_get_res_path(GUI_RES_DOG_OPEN_PNG));
        lv_timer_set_period(zhuangtai.zhayan_dingshiqi, lv_rand(4000, 7000));
        lv_timer_reset(zhuangtai.zhayan_dingshiqi);
        lv_timer_resume(zhuangtai.zhayan_dingshiqi);
        huxi_kaishi();
    }
}

void chongwuyibiao_chesu_gengxin(uint16_t chesu)
{
    if (zhuangtai.chesushuzi == NULL) return;
    zhuangtai.chesu = chesu;
    lv_label_set_text_fmt(zhuangtai.chesushuzi, "%u", (unsigned int)chesu);
    ui_pet_hero_speed((uint16_t)LV_MIN((uint32_t)chesu * 10u, 65535u));
    if (zhuangtai.yunxing) xiaogou_qiehuan(false);
}

/** @brief 使用原始有效车速与已校验状态，显示单位变化不改变动作判定。 */
void chongwuyibiao_cheliang_gengxin(uint16_t speed_dkph, bool valid, uint32_t flags)
{
    ui_pet_hero_update(speed_dkph, valid, flags);
    if (zhuangtai.yunxing) xiaogou_qiehuan(false);
}

static void zhayan_gengxin(lv_timer_t *dingshiqi)
{
    lv_obj_t *xiaogou = zhuangtai.xiaogou;
    if (zhuangtai.zhayan_jieduan == ZHAYAN_DENGDAI) {
        zhuangtai.zhayan_cishu = lv_rand(1, 100) <= 25 ? 2 : 1;
        lv_image_set_src(xiaogou, gui_get_res_path(xiaogou_biyan));
        zhuangtai.zhayan_jieduan = ZHAYAN_BIYAN;
        lv_timer_set_period(dingshiqi, 100);
    } else if (zhuangtai.zhayan_jieduan == ZHAYAN_BIYAN) {
        lv_image_set_src(xiaogou, gui_get_res_path(GUI_RES_DOG_OPEN_PNG));
        zhuangtai.zhayan_cishu--;
        if (zhuangtai.zhayan_cishu > 0) {
            zhuangtai.zhayan_jieduan = ZHAYAN_JIANGE;
            lv_timer_set_period(dingshiqi, 140);
        } else {
            zhuangtai.zhayan_jieduan = ZHAYAN_DENGDAI;
            lv_timer_set_period(dingshiqi, lv_rand(4000, 7000));
        }
    } else {
        lv_image_set_src(xiaogou, gui_get_res_path(xiaogou_biyan));
        zhuangtai.zhayan_jieduan = ZHAYAN_BIYAN;
        lv_timer_set_period(dingshiqi, 100);
    }
    lv_timer_reset(dingshiqi);
}

static void yun_gengxin(lv_timer_t *dingshiqi)
{
    uint32_t bu = lv_tick_elaps(zhuangtai.yun_shijian) / YUN_BUJIN_MS;
    unsigned int i;
    LV_UNUSED(dingshiqi);

    /* 跟随车速标签的更新，工具绑定消息更新标签时也能同步动作。 */
    zhuangtai.chesu = chesu_duqu();
#if LV_USE_GUIBUILDER_SIMULATOR && !defined(YOUYIBAO_UI_TEST) && !defined(PET_HERO_TEST)
    ui_pet_hero_speed((uint16_t)LV_MIN((uint32_t)zhuangtai.chesu * 10u, 65535u));
#endif
    ui_pet_hero_tick();
    xiaogou_qiehuan(false);
    if (bu == 0) return;
    zhuangtai.yun_shijian += bu * YUN_BUJIN_MS;

    for (i = 0; i < YUN_SHULIANG; i++) {
        yun_zhuangtai_t *yun = &zhuangtai.yun[i];
        int32_t toumingdu = LV_OPA_COVER;
        /* 保持云间距；越过右边后从左边继续，而非回到各自初始位置。 */
        yun->zhongxin += (int32_t)(bu % (YUN_YOU - YUN_ZUO));
        if (yun->zhongxin > YUN_YOU) yun->zhongxin -= YUN_YOU - YUN_ZUO;
        if (yun->zhongxin < YUN_DANRU) {
            toumingdu = (yun->zhongxin - YUN_ZUO) * LV_OPA_COVER / (YUN_DANRU - YUN_ZUO);
        } else if (yun->zhongxin > YUN_DANCHU) {
            toumingdu = (YUN_YOU - yun->zhongxin) * LV_OPA_COVER / (YUN_YOU - YUN_DANCHU);
        }
        lv_obj_set_style_opa(yun->tupian, (lv_opa_t)LV_CLAMP(0, toumingdu, LV_OPA_COVER), 0);
        lv_obj_set_x(yun->tupian, yun->zhongxin - yun->kuandu / 2);
    }
}

static void donghua_tingzhi(void)
{
    bool zhiqian_yunxing = zhuangtai.yunxing;
    if (zhuangtai.yun_dingshiqi != NULL) lv_timer_delete(zhuangtai.yun_dingshiqi);
    if (zhuangtai.zhayan_dingshiqi != NULL) lv_timer_delete(zhuangtai.zhayan_dingshiqi);
    zhuangtai.yun_dingshiqi = NULL;
    zhuangtai.zhayan_dingshiqi = NULL;
    ui_pet_hero_visible(false);
    if (zhuangtai.xiaogou != NULL) {
        lv_anim_delete(zhuangtai.xiaogou, huxi_suofang);
    }
    zhuangtai.yunxing = false;
    if (zhiqian_yunxing) neicun_zhuizong_jieduan("donghua_tingzhi");
}

static void donghua_kaishi(void)
{
    unsigned int i;
    if (zhuangtai.yunxing || zhuangtai.zanting) return;
    lv_obj_update_layout(zhuangtai.pingmu);
    for (i = 0; i < YUN_SHULIANG; i++) {
        zhuangtai.yun[i].kuandu = lv_obj_get_width(zhuangtai.yun[i].tupian);
        zhuangtai.yun[i].zhongxin = lv_obj_get_x(zhuangtai.yun[i].tupian) + zhuangtai.yun[i].kuandu / 2;
        zhuangtai.yun[i].zhongxin = LV_CLAMP(YUN_ZUO, zhuangtai.yun[i].zhongxin, YUN_YOU);
    }
    /* 呼吸缩放支点靠近脚底，保留 UI 工具设置的 Y=200。 */
    lv_image_set_pivot(zhuangtai.xiaogou, 115, 220);
    zhuangtai.yun_shijian = lv_tick_get();
    zhuangtai.yun_dingshiqi = lv_timer_create(yun_gengxin, YUN_BUJIN_MS, NULL);
    zhuangtai.zhayan_dingshiqi = lv_timer_create(zhayan_gengxin, lv_rand(4000, 7000), NULL);
    if (zhuangtai.yun_dingshiqi == NULL || zhuangtai.zhayan_dingshiqi == NULL) {
        donghua_tingzhi();
        return;
    }
    zhuangtai.yunxing = true;
    zhuangtai.chesu = chesu_duqu();
    ui_pet_hero_visible(true);
    ui_pet_hero_tick();
    xiaogou_qiehuan(true);
    neicun_zhuizong_jieduan("donghua_kaishi");
}

static void pingmu_shijian(lv_event_t *shijian)
{
    lv_event_code_t leixing = lv_event_get_code(shijian);
    if (lv_event_get_target(shijian) != zhuangtai.pingmu) return;
    if (leixing == LV_EVENT_SCREEN_LOADED) {
        donghua_kaishi();
    } else if (leixing == LV_EVENT_SCREEN_UNLOAD_START || leixing == LV_EVENT_SCREEN_UNLOADED) {
        donghua_tingzhi();
    } else if (leixing == LV_EVENT_DELETE) {
        donghua_tingzhi();
        lv_memset(&zhuangtai, 0, sizeof(zhuangtai));
    }
}

static void xiaogou_shijian(lv_event_t *shijian)
{
    if (lv_event_get_target(shijian) != zhuangtai.xiaogou) return;
    if (lv_event_get_code(shijian) == LV_EVENT_DELETE) {
        /* SDK 可先释放页面结构再删除控件，因此运行状态只保存控件指针。 */
        donghua_tingzhi();
        lv_memset(&zhuangtai, 0, sizeof(zhuangtai));
    }
}

/** @brief 音乐区显示时停止主界面动画，返回时恢复。 */
void chongwuyibiao_donghua_yunxu(bool yunxu)
{
    zhuangtai.zanting = !yunxu;
    if (!yunxu) donghua_tingzhi();
    else if (zhuangtai.pingmu != NULL && lv_screen_active() == zhuangtai.pingmu) {
        donghua_kaishi();
        if (zhuangtai.yunxing) xiaogou_qiehuan(false);
    }
}

/**
 * @brief 绑定宠物页动画与页面生命周期。
 * @param[in] jiemian 已创建的宠物页控件，允许为空。
 * @retval 无。
 * @note 硬件构建先更新车辆显示，再按有效车速启动动画；工具仿真保留预览数据。
 */
void chongwuyibiao_donghua_bangding(lv_ui_chongwuyibiao *jiemian)
{
    if (jiemian == NULL || jiemian->chongwuyibiao == NULL) return;
    if (zhuangtai.pingmu == jiemian->chongwuyibiao) return;
    donghua_tingzhi();
    lv_memset(&zhuangtai, 0, sizeof(zhuangtai));
    zhuangtai.pingmu = jiemian->chongwuyibiao;
    zhuangtai.xiaogou = jiemian->chongwuyibiao_xiaogou;
    zhuangtai.chesushuzi = jiemian->chongwuyibiao_chesushuzi;
    ui_pet_hero_bind(jiemian);
    zhuangtai.yun[0].tupian = jiemian->chongwuyibiao_yun1;
    zhuangtai.yun[1].tupian = jiemian->chongwuyibiao_yun2;
    zhuangtai.yun[2].tupian = jiemian->chongwuyibiao_yun3;
    zhuangtai.yun[3].tupian = jiemian->chongwuyibiao_yun4;
    lv_obj_add_event_cb(zhuangtai.pingmu, pingmu_shijian, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(zhuangtai.xiaogou, xiaogou_shijian, LV_EVENT_ALL, NULL);
#if !LV_USE_GUIBUILDER_SIMULATOR || defined(YOUYIBAO_UI_TEST)
    /* 首次启动动画前写入真实车速，避免把工具预览的 46 km/h 当作运行状态。 */
    youyibao_sif_ui_bind_pet(jiemian);
#endif
    if (lv_screen_active() == zhuangtai.pingmu) donghua_kaishi();
}
