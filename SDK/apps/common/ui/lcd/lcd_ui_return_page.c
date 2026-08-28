/**
 * @file lcd_ui_return_page.c
 * @brief jlui 页面跳转管理
 */
#include "app_config.h"
#include "ui_api.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-return-page]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CHAR_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lcd_ui_return_page.data.bss")
#pragma data_seg(".lcd_ui_return_page.data")
#pragma const_seg(".lcd_ui_return_page.text.const")
#pragma code_seg(".lcd_ui_return_page.text")
#endif

#if (defined(CONFIG_JL_UI_ENABLE) && CONFIG_JL_UI_ENABLE)

#include "jlui_app/ui_app_effect.h"
#include "jlgpu_math.h"
#include "gpu_port.h"
#include "gpu_task.h"


/**********************
 *      DEFINES
 *********************/
#define PAGE_RETURN_MAX			16  // 最大回溯记录
#define PAGE_PREEMPTION_MAX		8   // 最大抢占记录

#define UI_RET_PAGE_LOCK()		spin_lock(&ui_ret_page_lock)
#define UI_RET_PAGE_UNLOCK()	spin_unlock(&ui_ret_page_lock)

/**********************
 *      TYPEDEFS
 *********************/
struct ui_page_preemption {
    int page;
    int (*entry)(int); // 返回true，entry回调里面自定义处理；返回false或者回调为NULL，默认显示page
    int (*exit)(int); // 返回true，exit回调里面自定义处理；返回false或者回调为NULL，默认清除记录
    u8  priority;
};

/**********************
 * STATIC VARIABLES
 *********************/
static int page_return_tab[PAGE_RETURN_MAX];
static u8 return_index = 0;
static struct ui_page_preemption page_preemption[PAGE_PREEMPTION_MAX];
DEFINE_SPINLOCK(ui_ret_page_lock);


void ui_return_page_hump(void)
{
    log_debug("return_index:%d \n", return_index);
    for (int i = 0; i < return_index; i++) {
        log_debug("page[%d]:0x%x %d\n", i, page_return_tab[i], ui_id2page(page_return_tab[i]));
    }
}

//=================================================================================//
// @brief:用于处理当前页面左右滑动时是返回上一级页面还是直接滑屏切换页面
//=================================================================================//
u8 ui_return_page_pop(u8 dir)
{

    UI_RET_PAGE_LOCK();
    log_debug("ui_return_page_pop:%d \n", return_index);
    if (return_index == 1) { //pop[index-1]当前，pop[index-2]上一页
        if (ui_get_current_window_id() == page_return_tab[return_index - 1]) {
            log_debug("cur:%x next:%x", ui_get_current_window_id(), page_return_tab[return_index - 1]);
            UI_RET_PAGE_UNLOCK();
            return 2;
        }
    }
    if (return_index) {
        return_index--;
    }
    if (return_index) {
        int ret_page = page_return_tab[return_index - 1];
#if TCFG_CHARGE_ENABLE
        if (ret_page == ID_WINDOW_BEDSIDE_WATCH || ret_page == ID_WINDOW_BATCHARGE) {
            extern u8 get_charge_online_flag(void);
            if (!get_charge_online_flag()) { //不在充电时，过滤床头时钟或者充电页面
                if (return_index) {
                    return_index--;
                }
                if (return_index) {
                    ret_page = page_return_tab[return_index - 1];
                } else {
                    ret_page = ID_WINDOW_DIAL;
                }

            }
        }
#endif
        UI_RET_PAGE_UNLOCK();
        if (ui_return_page_effect_init(ret_page)) {
            ui_hide_curr_main();
            ui_show_main(ret_page);
        }
        return 1; //返回
    } else {
        UI_RET_PAGE_UNLOCK();
        UI_SHOW_WINDOW(ID_WINDOW_DEFAULT);
        /* ui_send_event(KEY_CHANGE_PAGE, dir); */
        return 2; //滑屏
    }
}
//=================================================================================//
// @brief:返回到指定页面
//=================================================================================//
int ui_return_page_pop_spec(u32 page_id)
{
    UI_RET_PAGE_LOCK();
    for (int i = 0; i < return_index; i++) {
        if (page_return_tab[i] == page_id) { // 查找是否有指定页面
            return_index = i;
            break;
        }
    }
    UI_RET_PAGE_UNLOCK();
    ui_hide_curr_main();
    ui_show_main(page_id);
    return true;
}

//=================================================================================//
// @brief:用于获取下一个页面的id
//=================================================================================//
u32 ui_return_page_id(void)
{
    u32 ret_page = 0;
    UI_RET_PAGE_LOCK();
    if (return_index) {
        ret_page = page_return_tab[return_index - 1];
    }
    UI_RET_PAGE_UNLOCK();
    return ret_page;
}

//=================================================================================//
// @brief:用于获取上一个页面的id
//=================================================================================//
u32 ui_return_prev_page_id(void)
{
    u32 ret_page = 0;
    UI_RET_PAGE_LOCK();
    if (return_index > 1) {
        ret_page = page_return_tab[return_index - 2];
    }
    UI_RET_PAGE_UNLOCK();
    return ret_page;
}

//=================================================================================//
// @brief:用于处理页面返回时，返回的页面存储在page_return_tab
//=================================================================================//
void ui_return_page_push(int page_id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("ui_return_page_push:%d, 0x%x \n", return_index, page_id);

    UI_RET_PAGE_LOCK();
#if 0
    // 检查所有
    for (int i = 0; i < return_index; i++) {
        if (page_return_tab[i] == page_id) {
            return_index = i + 1;
            UI_RET_PAGE_UNLOCK();
            log_debug("ui_return_page_push, same id[%d]:0x%x, rets =%x\n", i, page_id, rets);
            return ;
        }
    }
#else
    // 检查最后一个
    if (return_index) {
        if (page_return_tab[return_index - 1] == page_id) {
            UI_RET_PAGE_UNLOCK();
            log_debug("ui_return_page_push, same id[%d]:0x%x, rets =%x\n", return_index - 1, page_id, rets);
            return ;
        }
    }
#endif

    if (return_index == PAGE_RETURN_MAX) {
        log_error("ui_return_page_push over rets =%x,id =%x\n", rets, page_id);
        for (int i = 0; i < return_index; i++) {
            log_debug("[%d],id =%x\n", i, page_return_tab[i]);
        }

        for (int i = 0; i < return_index; i++) {
            for (int j = i + 1; j < return_index; j++) {
                if (page_return_tab[i] == page_return_tab[j]) {
                    memcpy(&page_return_tab[i], &page_return_tab[j], return_index - j);
                    return_index -= (j - i);
                }
            }
        }
        for (int i = 0; i < return_index; i++) {
            log_debug("out [%d],id =%x\n", i, page_return_tab[i]);
        }

    }

#ifdef CONFIG_RELEASE_ENABLE
    if (return_index == PAGE_RETURN_MAX) {
        return_index--;
    }
#endif

    page_return_tab[return_index] = page_id;
    return_index++;

    ASSERT(return_index <= PAGE_RETURN_MAX);

    UI_RET_PAGE_UNLOCK();

    ui_return_page_hump();
}
//=================================================================================//
// @brief:用于清除页面返回记录
//=================================================================================//
void ui_return_page_clear()
{
    UI_RET_PAGE_LOCK();
    return_index = 0;
    UI_RET_PAGE_UNLOCK();
}
//=================================================================================//
// @brief:用于删除页面返回记录
//=================================================================================//
void ui_return_page_del(int page_id)
{
    UI_RET_PAGE_LOCK();
    for (int i = 0; i < return_index; i++) {
        if (page_return_tab[i] == page_id) {
            u8 offset = 1;
            if (i < (return_index - 1)) {
                if (i > 1) {
                    if (page_return_tab[i - 1] == page_return_tab[i + 1]) {
                        offset++;
                    }
                }
                for (; i < return_index - offset; i++) {
                    page_return_tab[i] = page_return_tab[i + offset];
                }
            }
            return_index -= offset;
            break;
        }
    }
    UI_RET_PAGE_UNLOCK();
}
//=================================================================================//
// @brief:跳转到指定page，清空之后的page
//=================================================================================//
void ui_return_page_jump(int page_id, u8 clear_self)
{
    UI_RET_PAGE_LOCK();
    for (int i = 0; i < return_index; i++) {
        if (page_return_tab[i] == page_id) {
            if (clear_self) { // 清除本身
                return_index = i;
            } else { // 保留
                return_index = i + 1;
            }
            break;
        }
    }
    UI_RET_PAGE_UNLOCK();
}
//=================================================================================//
// @brief:用于从page_return_tab表中回退1
//=================================================================================//
u8 ui_return_page_sub(void)
{
    u8 index = 0;
    UI_RET_PAGE_LOCK();
    if (return_index) {
        return_index--;
        index = return_index;
    }
    UI_RET_PAGE_UNLOCK();
    return index;
}

#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
u8 ui_return_page_add(int page_id)
{
    u8 index = 0;
    UI_RET_PAGE_LOCK();
    page_return_tab[return_index] = page_id;
    return_index++;
    index = return_index;
    UI_RET_PAGE_UNLOCK();
    return index;
}
#endif

//=================================================================================//
// @brief:ui_window弱函数重定义
//=================================================================================//
int ui_window_event_deal(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE: {
        // 返回上一个page
        struct window *window = (struct window *)ctr;
        log_info("win[0x%x] rmove \n", window->elm.id);
        UI_RET_PAGE_LOCK();
        int preempt_page = page_preemption[0].page;
        UI_RET_PAGE_UNLOCK();
        if (preempt_page == window->elm.id) { // 打断返回
            ui_preemption_page_pop(window->elm.id);
        } else {
            ui_return_page_pop(2);
        }
        return true;
    }
    break;
    }
    return false;
}


void ui_preemption_page_hump(void)
{
    log_debug("preempt page hump \n");
    for (int i = 0; i < PAGE_PREEMPTION_MAX; i++) {
        if (page_preemption[i].page == 0) {
            break;
        }
        log_debug("pree[%d] : 0x%x \n", i, page_preemption[i].page);
    }
}


//=================================================================================//
// @brief: 判断是否有抢断page
//=================================================================================//
int ui_preemption_page_check(void)
{
    int ret = false;
    UI_RET_PAGE_LOCK();
    if (page_preemption[0].page) {
        /* ret = true; */
        ret = page_preemption[0].page;
    }
    UI_RET_PAGE_UNLOCK();
    return ret;
}

//=================================================================================//
// @brief: 清空抢断记录
//=================================================================================//
void ui_preemption_page_clean(void)
{
    UI_RET_PAGE_LOCK();
    page_preemption[0].page = 0;
    UI_RET_PAGE_UNLOCK();
}

//=================================================================================//
// @brief: 删除抢断记录
//=================================================================================//
int ui_preemption_page_del(int page_id)
{
    int i;
    UI_RET_PAGE_LOCK();
    for (i = 0; i < PAGE_PREEMPTION_MAX; i++) {
        if (page_preemption[i].page == 0) { // 最后记录
            break;
        }
        if (page_preemption[i].page == page_id) {
            for (; i < PAGE_PREEMPTION_MAX - 1; i++) {
                memcpy(&page_preemption[i], &page_preemption[i + 1], sizeof(struct ui_page_preemption));
            }
            page_preemption[i].page = 0;
            UI_RET_PAGE_UNLOCK();
            return true;
        }
    }
    UI_RET_PAGE_UNLOCK();
    return false;
}


//=================================================================================//
// @brief:推送一个抢断页面
//  可以显示时通过entry()回调执行
//=================================================================================//
int ui_preemption_page_push(int page_id, int (*entry)(int), int (*exit)(int), u8 priority)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("__func__ %s %x  page_id:%x\n", __func__, rets, page_id);
    ui_return_page_effect_set_en(false);
    int i;
    struct ui_page_preemption push_pree = {0};
    struct ui_page_preemption exit_page = {0};
    push_pree.page = page_id;
    push_pree.entry = entry;
    push_pree.exit = exit;
    push_pree.priority = priority;
    UI_RET_PAGE_LOCK();
__match:
    for (i = 0; i < PAGE_PREEMPTION_MAX; i++) {
        if (page_preemption[i].page == 0) { // 最后记录
            break;
        }
        if (page_preemption[i].page == page_id) { // 已经有记录
            if (page_preemption[i].priority == priority) { // 优先级相同
                memcpy(&page_preemption[i], &push_pree, sizeof(struct ui_page_preemption));
                UI_RET_PAGE_UNLOCK();
                return false;
            } else { // 删除重新匹配
                for (; i < PAGE_PREEMPTION_MAX - 1; i++) {
                    memcpy(&page_preemption[i], &page_preemption[i + 1], sizeof(struct ui_page_preemption));
                }
                page_preemption[i].page = 0;
                goto __match;
            }
            break;
        }
        if (page_preemption[i].priority < priority) { // 优先级更低
            break;
        }
    }
    log_debug("[%s] i:%d", __func__, i);
    if (i >= PAGE_PREEMPTION_MAX) {
        log_error("page preempt full !!! \n");
        i = PAGE_PREEMPTION_MAX - 1; // 覆盖最后一个
    }
    // 重新排序
    for (int j = PAGE_PREEMPTION_MAX - 1; j > i; j--) {
        memcpy(&page_preemption[j], &page_preemption[j - 1], sizeof(struct ui_page_preemption));
    }
    memcpy(&page_preemption[i], &push_pree, sizeof(struct ui_page_preemption));
    // 是否有抢断
    if (i == 0) {
        memcpy(&exit_page, &page_preemption[1], sizeof(struct ui_page_preemption));
    }
    ui_preemption_page_hump();
    UI_RET_PAGE_UNLOCK();

    // 被打断的page处理
    if (exit_page.page) {
        int ret = false;
        if (exit_page.exit) {
            ret = exit_page.exit(push_pree.page);
        }
        if (ret == false) { // 删除记录
            ui_preemption_page_del(exit_page.page);
        }
        // 跳转到打断时的page
        ui_return_page_jump(exit_page.page, 1);
    }

    if (i == 0) { // 处在最前面，可以执行
        int ret = false;
        if (push_pree.entry) {
            ret = push_pree.entry(push_pree.page);
        }
        if (ret == false) { // 显示
            ui_hide_curr_main();
            ui_show_main(push_pree.page);
        }
        return true;
    }
    return false;
}

//=================================================================================//
// @brief:退出一个抢断页面
//  如果正在执行，会调用exit()回调
//=================================================================================//
int ui_preemption_page_pop(int page_id)
{
    int i;
    struct ui_page_preemption entry_page = {0};
    struct ui_page_preemption exit_page = {0};
    UI_RET_PAGE_LOCK();
    if (page_preemption[0].page == 0) { // 没有记录
        UI_RET_PAGE_UNLOCK();
        return false;
    }
    if (page_preemption[0].page != page_id) { // 不是第一个，删除记录就可以
        UI_RET_PAGE_UNLOCK();
        ui_preemption_page_del(page_id);
        return true;
    }
    memcpy(&exit_page, &page_preemption[0], sizeof(struct ui_page_preemption));
    // 重排序
    for (i = 0; i < PAGE_PREEMPTION_MAX - 1; i++) {
        memcpy(&page_preemption[i], &page_preemption[i + 1], sizeof(struct ui_page_preemption));
    }
    page_preemption[i].page = 0;
    memcpy(&entry_page, &page_preemption[0], sizeof(struct ui_page_preemption));
    ui_preemption_page_hump();
    UI_RET_PAGE_UNLOCK();

    // 退出当前的page
    int ret;
    if (exit_page.exit) {
        exit_page.exit(exit_page.page);
    }

    // 有可以执行的page处理
    if (entry_page.page) {
        // 跳转到打断时的page
        ui_return_page_jump(exit_page.page, 1);

        ret = false;
        if (entry_page.entry) {
            ret = entry_page.entry(entry_page.page);
        }
        if (ret == false) { // 显示
            ui_hide_curr_main();
            ui_show_main(entry_page.page);
        }
    } else { // 没有就显示回正常的处理
        // 跳转到打断时的page
        ui_return_page_jump(exit_page.page, 0);

        // 回溯
        ui_return_page_pop(2);
    }
    return true;
}

#if (defined CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE)
extern bool cpc_is_card_page(int mem_id);
/**
 * @brief 彩屏仓接口，针对弹窗应用场景，比如充电页面，开关盖页面
 *        在卡片页面弹出时候，再返回时，还可以返回当前卡片页面
 *
 * @param page_id
 */
void cs_ui_popup_page(u32 page_id)
{
    u32 rets;//, reti;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_info("cs_ui_popup_page rets:%x page_id:%x\n", rets, page_id);
    u32 cur_id = UI_GET_WINDOW_ID();
    if (cpc_is_card_page(cur_id)) {
        /*如果当前页面是卡片页面，则是压栈进入page_return_tab
        后面则返回该页面 */
        ui_return_page_push(cur_id);
    }

    UI_HIDE_CURR_WINDOW();
    UI_SHOW_WINDOW(page_id);
}
#endif

#endif /* #if (defined(CONFIG_JL_UI_ENABLE)) */

