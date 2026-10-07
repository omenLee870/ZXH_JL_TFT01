#if !LV_USE_GUIBUILDER_SIMULATOR
#include "app_config.h"
#include "malloc.h"
#include "asm/psram_api.h"
#endif
#include "neicun_zhuizong.h"

#if !LV_USE_GUIBUILDER_SIMULATOR
#include "../generated/gui_guider.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define ZHUIZONG_JIANGE_MS 2000U
#define HUANCHONG_JILU_SHULIANG 64U

extern char HEAP_BEGIN[];
extern char HEAP_END[];
extern char PSRAM_HEAP_BEGIN[];
extern char PSRAM_HEAP_END[];

typedef struct {
    const lv_draw_buf_t *huanchong;
    uint32_t zijie;
} huanchong_jilu_t;

typedef struct {
    huanchong_jilu_t huanchong[HUANCHONG_JILU_SHULIANG];
    uint32_t ram0_zong;
    uint32_t ram1_zong;
    uint32_t ram0_shengyu;
    uint32_t ram1_shengyu;
    uint32_t zuidi_shengyu;
    uint32_t chuangkou_zuodi;
    uint32_t uiqian_shengyu;
    uint32_t zitikaishi_shengyu;
    uint32_t ziti_cishu;
    uint32_t huanchong_zijie;
    uint32_t huanchong_fengzhi;
    uint32_t huanchong_fenpei;
    uint32_t huanchong_shifang;
    uint32_t huanchong_shibai;
    uint32_t huanchong_weijilu;
    uint32_t shangci_baogao;
    bool kaiqi;
    bool shouci_huizhi;
} neicun_jilu_t;

static neicun_jilu_t jilu;

static uint32_t neicun_quyang(void)
{
    uint32_t shengyu;
    jilu.ram0_shengyu = (uint32_t)memory_get_size(P_MEMORY_UNUSED);
    jilu.ram1_shengyu = (uint32_t)memory_get_size(P_VLT_MEMORY_UNUSED);
    shengyu = jilu.ram0_shengyu + jilu.ram1_shengyu;
    if (shengyu < jilu.zuidi_shengyu) jilu.zuidi_shengyu = shengyu;
    if (shengyu < jilu.chuangkou_zuodi) jilu.chuangkou_zuodi = shengyu;
    return shengyu;
}

void neicun_zhuizong_jieduan(const char *jieduan)
{
    uint32_t shengyu;
    gui_scr_t *pingmu;
    if (!jilu.kaiqi) return;
    shengyu = neicun_quyang();
    pingmu = gui_scr_get_act();
    printf("[UI_RAM] t=%u phase=%s page=%d ram0_total=%u ram0_free=%u ram1_total=%u ram1_free=%u free=%u min_sampled=%u interval_min=%u net_since_ui=%d\n",
           (unsigned)lv_tick_get(), jieduan, pingmu ? pingmu->id : -1,
           (unsigned)jilu.ram0_zong, (unsigned)jilu.ram0_shengyu,
           (unsigned)jilu.ram1_zong, (unsigned)jilu.ram1_shengyu,
           (unsigned)shengyu, (unsigned)jilu.zuidi_shengyu,
           (unsigned)jilu.chuangkou_zuodi,
           (int)((int32_t)jilu.uiqian_shengyu - (int32_t)shengyu));
    printf("[UI_BUF] current=%u peak=%u alloc=%u free=%u failed=%u untracked=%u\n",
           (unsigned)jilu.huanchong_zijie, (unsigned)jilu.huanchong_fengzhi,
           (unsigned)jilu.huanchong_fenpei, (unsigned)jilu.huanchong_shifang,
           (unsigned)jilu.huanchong_shibai, (unsigned)jilu.huanchong_weijilu);
#if TCFG_PSRAM_DEV_ENABLE
    /* SDK 官方统计输出实际空闲、最低空闲及最大空闲块；块数量不是字节数。 */
    printf("[UI_PSRAM] phase=%s begin\n", jieduan);
    psram_heap_stats();
    printf("[UI_PSRAM] phase=%s end\n", jieduan);
#endif
}

static void neicun_dingshi(lv_timer_t *dingshiqi)
{
    LV_UNUSED(dingshiqi);
    neicun_quyang();
    if (lv_tick_elaps(jilu.shangci_baogao) >= ZHUIZONG_JIANGE_MS) {
        jilu.shangci_baogao = lv_tick_get();
        neicun_zhuizong_jieduan("yunxing");
        jilu.chuangkou_zuodi = jilu.ram0_shengyu + jilu.ram1_shengyu;
    }
}

static void shouci_huizhi(lv_event_t *shijian)
{
    LV_UNUSED(shijian);
    if (jilu.shouci_huizhi) return;
    jilu.shouci_huizhi = true;
    neicun_zhuizong_jieduan("shouci_refr_hou");
}

void neicun_zhuizong_kaishi(void)
{
    lv_timer_t *dingshiqi;
    lv_display_t *xianshi;
    if (jilu.kaiqi) return;
    lv_memset(&jilu, 0, sizeof(jilu));
    jilu.kaiqi = true;
    jilu.ram0_zong = (uint32_t)memory_get_size(P_MEMORY_TOTAL);
    jilu.ram1_zong = (uint32_t)memory_get_size(P_VLT_MEMORY_TOTAL);
    jilu.zuidi_shengyu = UINT32_MAX;
    jilu.chuangkou_zuodi = UINT32_MAX;
    jilu.uiqian_shengyu = neicun_quyang();
    jilu.shangci_baogao = lv_tick_get();
    printf("[UI_RAM_CFG] units=bytes main_sram=%u psram_config=%u heap_capacity=%u psram_heap_capacity=%u lvgl_from_psram=%d trace_static=%u baseline_includes_display_driver=1\n",
           (unsigned)CONFIG_PHYSIC_RAM_SIZE, (unsigned)TCFG_PSRAM_SIZE,
           (unsigned)((uintptr_t)HEAP_END - (uintptr_t)HEAP_BEGIN),
           (unsigned)((uintptr_t)PSRAM_HEAP_END - (uintptr_t)PSRAM_HEAP_BEGIN),
           LV_USE_STDLIB_MALLOC_FROM_PSRAM, (unsigned)sizeof(jilu));
    neicun_zhuizong_jieduan("xitong_uiqian");
    dingshiqi = lv_timer_create(neicun_dingshi, 100, NULL);
    if (dingshiqi == NULL) printf("[UI_RAM] trace_timer_create_failed\n");
    xianshi = lv_display_get_default();
    if (xianshi != NULL) {
        lv_display_add_event_cb(xianshi, shouci_huizhi, LV_EVENT_REFR_READY, NULL);
    }
}

void lv_font_ram_trace(const char *lujing, bool kaishi)
{
    uint32_t shengyu;
    const char *mingcheng;
    char shougexinziti[24];
    if (!jilu.kaiqi) return;
    mingcheng = strrchr(lujing, '/');
    if (mingcheng == NULL) mingcheng = strrchr(lujing, '\\');
    mingcheng = mingcheng ? mingcheng + 1 : lujing;
    if (kaishi) {
        /* 当前生成器先加载原 demo 字体，再加载宠物页的首个 Lato 字体。 */
        snprintf(shougexinziti, sizeof(shougexinziti), "%x.rle", GUI_RES_LATO_REGULAR_20_RLE_FLASH);
        if (strcmp(mingcheng, shougexinziti) == 0) {
            neicun_zhuizong_jieduan("yuandemo_zitihou");
        }
        jilu.zitikaishi_shengyu = neicun_quyang();
        return;
    }
    shengyu = neicun_quyang();
    jilu.ziti_cishu++;
    printf("[UI_FONT] n=%u file=%s ram_net=%d ram_free=%u\n",
           (unsigned)jilu.ziti_cishu, mingcheng,
           (int)((int32_t)jilu.zitikaishi_shengyu - (int32_t)shengyu), (unsigned)shengyu);
}

void lv_draw_buf_ram_trace(const lv_draw_buf_t *huanchong, bool fenpei)
{
    unsigned i;
    if (!jilu.kaiqi) return;
    if (huanchong == NULL) {
        if (fenpei) jilu.huanchong_shibai++;
        neicun_quyang();
        return;
    }
    if (fenpei) {
        for (i = 0; i < HUANCHONG_JILU_SHULIANG; i++) {
            if (jilu.huanchong[i].huanchong == NULL) {
                jilu.huanchong[i].huanchong = huanchong;
                jilu.huanchong[i].zijie = huanchong->data_size;
                jilu.huanchong_zijie += huanchong->data_size;
                if (jilu.huanchong_zijie > jilu.huanchong_fengzhi) {
                    jilu.huanchong_fengzhi = jilu.huanchong_zijie;
                }
                jilu.huanchong_fenpei++;
                break;
            }
        }
        if (i == HUANCHONG_JILU_SHULIANG) jilu.huanchong_weijilu++;
    } else {
        for (i = 0; i < HUANCHONG_JILU_SHULIANG; i++) {
            if (jilu.huanchong[i].huanchong == huanchong) {
                jilu.huanchong_zijie -= jilu.huanchong[i].zijie;
                jilu.huanchong[i].huanchong = NULL;
                jilu.huanchong[i].zijie = 0;
                jilu.huanchong_shifang++;
                break;
            }
        }
    }
    /* 在真正绘制分配发生时采样，不能只依赖周期日志抓短暂峰值。 */
    neicun_quyang();
}

#else
void neicun_zhuizong_kaishi(void) {}
void neicun_zhuizong_jieduan(const char *jieduan) { LV_UNUSED(jieduan); }
void lv_font_ram_trace(const char *lujing, bool kaishi) { LV_UNUSED(lujing); LV_UNUSED(kaishi); }
void lv_draw_buf_ram_trace(const lv_draw_buf_t *huanchong, bool fenpei) { LV_UNUSED(huanchong); LV_UNUSED(fenpei); }
#endif
