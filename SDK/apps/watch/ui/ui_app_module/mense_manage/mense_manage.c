#include "app_config.h"
#include "mense_manage.h"
#include "rtc.h"
#include "ui/ui_api.h"
#include "user_cfg_id.h"

#ifdef CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE

static int mense_day_calculation(void);
/***********************************************************************
                        配置信息
参考	cycle 28天
day 01~day 05 易孕期
day 06~day 06 排卵日（易孕期）
day 07~day 10 易孕期（合计10day）
day 11~day 19 安全期
day 20~day 27 经期	（8day）
day 28~day 28 安全期（合计10day）

***********************************************************************/
#define OVULATION_OFFSET            (14)        //排卵日 距 经期首日 差值
#define EASY_PREGNACY_START         (-5)        //易孕期 距 排卵日 差值 往前
#define EASY_PREGNACY_END           (4)         //易孕期 距 排卵日 差值 往后
#define PERSONAL_INFO_NOT_CHANGE    (true)      //个人信息记录是否随月份修改
/* #define VM_MENSE_INFO               0 */
#define ID_WINDOW_PERIOD           	ID_WINDOW_WOMEN_HEALTH_WARNING
MENSE_INFO mense_day_info;
PERSONAL_MENSE mense_personal_info;

#define __day (&mense_day_info)                 //生理周期数据
#define __info (&mense_personal_info)           //生理周期个人信息

static int mense_vm_timer_id = 0;               //存储定时器
/***********************************************************************
                        DEBUG&TEST
***********************************************************************/
#define TEST_DEBUG      0                   //模拟测试模式
#define MENSE_DEBUG     1                   //debug功能，默认关闭
#if MENSE_DEBUG
#define M_LOG_DEBUG(format,...) 	//printf("[MENSE]<%s>"format"\n",__func__,##__VA_ARGS__)
#else
#define M_LOG_DEBUG(format,...)
#endif
/***********************************************************************
                        时间工具函数
    <FUNC>mense_time_offset_day 日期偏移 ntime输入，ptime输出，x前负后正
    <FUNC>mense_time_day_len 计算两个日期相差多少天，time2>time1时为正
***********************************************************************/
static int __sys_time_to_day(struct sys_time *time)
{
    time->hour = 0;
    time->min = 0;
    time->sec = 0;
    int sec = timestamp_mytime_2_utc_sec(time);
    int daycnt = sec / 86400;
    M_LOG_DEBUG("[%04d-%02d-%02d]->[day:%d]", time->year, time->month, time->day, daycnt);
    return daycnt;
}
static void __day_to_sys_time(struct sys_time *time, int day) //
{
    int sec = day * 86400;
    timestamp_utc_sec_2_mytime(sec, time);
    M_LOG_DEBUG("[%04d-%02d-%02d]<-[day:%d]", time->year, time->month, time->day, day);
}

void mense_time_offset_day(struct sys_time *ptime, struct sys_time *ntime, int x)
{
    M_LOG_DEBUG("offset:%d", x);
    int day_tmp = __sys_time_to_day(ntime) + x;
    if (day_tmp < 0) {
        return ;
    }
    __day_to_sys_time(ptime, day_tmp);
}
int mense_time_day_len(struct sys_time *time1, struct sys_time *time2)
{

    int day1 = __sys_time_to_day(time1);
    int day2 = __sys_time_to_day(time2);
    M_LOG_DEBUG("day1:%d day2:%d", day1, day2);
    return day2 - day1;
}

/***********************************************************************
            个人信息vm区的存储和获取
    <FUNC>mense_info_get_vm 从vm获取个人信息
    <FUNC>mense_info_set_vm 把个人信息写入vm
    <FUNC>mense_info_set_vm_timer_do 延时写入vm
***********************************************************************/
static int mense_info_get_vm(void)
{
    M_LOG_DEBUG();
#if VM_MENSE_INFO
    int ret = 0;
    ret = syscfg_read(VM_MENSE_INFO, __info, sizeof(PERSONAL_MENSE));
    if (sizeof(PERSONAL_MENSE) != ret) {
        M_LOG_DEBUG("%s err\n", __func__);
        return -1;
    }
    return ret;
#else
    return -1;
#endif
}
static int mense_info_set_vm(void)
{
    M_LOG_DEBUG();
#if VM_MENSE_INFO
    int ret = 0;
    ret = syscfg_write(VM_MENSE_INFO, __info, sizeof(PERSONAL_MENSE));
    if (sizeof(PERSONAL_MENSE) != ret) {
        M_LOG_DEBUG("%s err\n", __func__);
        return -1;
    }
    return ret;
#else
    return  -1;
#endif
}
static void mense_info_set_vm_timer_do(void *p)
{
    M_LOG_DEBUG();
    if (mense_vm_timer_id) {
        sys_timer_del(mense_vm_timer_id);
        mense_vm_timer_id = 0;
    }
    mense_info_set_vm();
}
/***********************************************************************
                个人生理信息设置与获取
                    用于APP协议,或本地设置
    <FUNC>mense_personal_info_get   获取个人信息
    <FUNC>mense_personal_info_set   设置个人信息
***********************************************************************/
int mense_personal_info_get(void *param)
{
    M_LOG_DEBUG();
    PERSONAL_MENSE *info = (PERSONAL_MENSE *)param;
    memcpy(info, __info, sizeof(PERSONAL_MENSE));
    if ((!__info->physiological_days) || (!__info->mense_period_days)) {
        M_LOG_DEBUG("physiological_days:%d mense_period_days:%d\n", __info->physiological_days, __info->mense_period_days);
        return -1;
    }
    return 0;
}
int mense_personal_info_set(void *param)
{
    M_LOG_DEBUG();
    PERSONAL_MENSE *info = (PERSONAL_MENSE *)param;
    /* if ((!__info->physiological_days) || (!__info->mense_period_days)) { */
    /* M_LOG_DEBUG("physiological_days:%d mense_period_days:%d\n", __info->physiological_days, __info->mense_period_days); */
    /* return -1; */
    /* } */
    memcpy(__info, info, sizeof(PERSONAL_MENSE));
    mense_day_calculation();
    //延时存储
    if (!mense_vm_timer_id) {
        mense_vm_timer_id = sys_timer_add(NULL, mense_info_set_vm_timer_do, 1000);
    } else {
        sys_timer_re_run(mense_vm_timer_id);
    }
    return 0;
}

/***********************************************************************
                生理周期预测
    <FUNC>mense_day_dump            日期打印 调试用
    <FUNC>mense_status_calculation  生理周期状态计算
    <FUNC>mense_day_calculation     生理周期日期计算
***********************************************************************/
static void mense_day_dump(void)
{
    /* M_LOG_DEBUG(); */
    /* struct sys_time time; */
    /* struct sys_time *ptime; */
    /* rtc_read_time(&time); */
    /* ptime = &time; */
    /* printf("[toady] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */
    /* ptime = &__info->menstruation_sday; */
    /* printf("[info menstruation_sday] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */

    /* ptime = &__day->menstruation_sday; */
    /* printf("[menstruation_sday] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */

    /* ptime = &__day->menstruation_eday; */
    /* printf("[menstruation_eday] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */
    /* ptime = &__day->ovulation_day; */
    /* printf("[ovulation_day] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */
    /* ptime = &__day->easy_pregnancy_sday; */
    /* printf("[easy_pregnancy_sday] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */
    /* ptime = &__day->easy_pregnancy_eday; */
    /* printf("[easy_pregnancy_eday] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */

    /* struct sys_time next_time; */
    /* mense_time_offset_day(&next_time, &__day->menstruation_sday, __info->physiological_days); */
    /* ptime = &next_time; */
    /* printf("[next_menstruation] %04d-%02d-%02d", ptime->year, ptime->month, ptime->day); */

}

static int mense_day_calculation(void)
{
    M_LOG_DEBUG();
    if ((!__info->physiological_days) || (!__info->mense_period_days)) {
        M_LOG_DEBUG("physiological_days:%d mense_period_days:%d\n", __info->physiological_days, __info->mense_period_days);
        return -1;
    }
    rtc_read_time(&__day->today);
    //计算预测时间
    int menstruation_day_offset = mense_time_day_len(&__info->menstruation_sday, &__day->today) / __info->physiological_days * __info->physiological_days;
    //正常计算生理周期
    M_LOG_DEBUG("menstruation_day_offset:%d\n mense_period_days:%d \n physiological_days:%d\n", menstruation_day_offset, __info->mense_period_days, __info->physiological_days);
    mense_time_offset_day(&__day->menstruation_day, & __info->menstruation_sday, menstruation_day_offset);
    mense_time_offset_day(&__day->post_menstruation_day, &__day->menstruation_day, __info->mense_period_days);
    mense_time_offset_day(&__day->ovulation_day, &__day->menstruation_day, OVULATION_OFFSET);
    mense_time_offset_day(&__day->prev_ovulation_day, &__day->ovulation_day, EASY_PREGNACY_START);
    mense_time_offset_day(&__day->post_ovulation_day, &__day->ovulation_day, 1);
    mense_time_offset_day(&__day->prev_menstruation_day, &__day->ovulation_day, EASY_PREGNACY_END + 1);
    int curr_day  = __sys_time_to_day(&__day->today);
    for (int status = 1; status < MENSE_TYPE_MAX; status++) {
        struct sys_time begin_time;
        struct sys_time end_time;
        mense_status_begin_time_get(status, &begin_time);
        mense_status_end_time_get(status, &end_time);
        int begin_day = __sys_time_to_day(&begin_time);
        int end_day = __sys_time_to_day(&end_time);
        M_LOG_DEBUG("status:%d cur(%d %d) bgn(%d %d) end(%d %d )", status, __day->today.month, __day->today.day, begin_time.month, begin_time.day, end_time.month, end_time.day);
        if ((end_day < begin_day) || (end_day < curr_day)) {
            //更新周期
            mense_status_time_update(mense_next_status_get(status));
            status --;//重算
            continue;
        }
        if ((curr_day >= begin_day) && (curr_day <= end_day)) {
            __day->mense_type = status;
        }
    }

    return __day->mense_type;
}
/***********************************************************************
                生理周期时间获取
    <FUNC>mense_day_info_get    获取生理日期结构体
    <FUNC>mense_now_status_get  获取当前生理状态
    <FUNC>mense_next_status_get 计算下一生理状态
***********************************************************************/
int mense_day_info_get(void *param)
{
    M_LOG_DEBUG();
    if ((!__info->physiological_days) || (!__info->mense_period_days)) {
        M_LOG_DEBUG("physiological_days:%d mense_period_days:%d\n", __info->physiological_days, __info->mense_period_days);
        return -1;
    }
    int result = mense_day_calculation();
    MENSE_INFO *info = (MENSE_INFO *)param;
    memcpy(info, __day, sizeof(MENSE_INFO));
    return result;
}
int mense_now_status_get(void)
{
    M_LOG_DEBUG();
    return __day->mense_type;
}
int mense_next_status_get(int status)
{
    M_LOG_DEBUG();
    int next_status  = status + 1;
    next_status %= 6;
    return next_status;
}
int mense_status_begin_time_get(int curr_type, struct sys_time *time)
{
    if (curr_type >= MENSE_TYPE_MAX) {
        return -1;
    }
    switch (curr_type) {
    case MENSE_TYPE_PREV_MENSTRUATION:
        memcpy(time, &__day->prev_menstruation_day, sizeof(struct sys_time));
        break;
    case MENSE_TYPE_MENSTRUATION:
        memcpy(time, &__day->menstruation_day, sizeof(struct sys_time));
        break;
    case MENSE_TYPE_POST_MENSTRUATION:
        memcpy(time, &__day->post_menstruation_day, sizeof(struct sys_time));
        break;
    case MENSE_TYPE_PREV_OVULATION:
        memcpy(time, &__day->prev_ovulation_day, sizeof(struct sys_time));
        break;
    case MENSE_TYPE_OVULATION:
        memcpy(time, &__day->ovulation_day, sizeof(struct sys_time));
        break;
    case MENSE_TYPE_POST_OVULATION:
        memcpy(time, &__day->post_ovulation_day, sizeof(struct sys_time));
        break;
    }
    M_LOG_DEBUG("status:%d (%04d-%02d-%02d )", curr_type, time->year, time->month, time->day);
    return curr_type;
}
int mense_status_end_time_get(int curr_status, struct sys_time *time)
{
    if (curr_status >= MENSE_TYPE_MAX) {
        return -1;
    }
    int next_status = mense_next_status_get(curr_status);
    mense_status_begin_time_get(next_status, time);
    mense_time_offset_day(time, time, -1);
    M_LOG_DEBUG("status:%d (%04d-%02d-%02d )", curr_status, time->year, time->month, time->day);
    return curr_status;
}
int mense_status_time_update(int curr_type)
{
    struct sys_time *time = NULL;
    if (curr_type >= MENSE_TYPE_MAX) {
        return -1;
    }
    switch (curr_type) {
    case MENSE_TYPE_PREV_MENSTRUATION:
        time = &__day->prev_menstruation_day;
        break;
    case MENSE_TYPE_MENSTRUATION:
        time = &__day->menstruation_day;
        break;
    case MENSE_TYPE_POST_MENSTRUATION:
        time = &__day->post_menstruation_day;
        break;
    case MENSE_TYPE_PREV_OVULATION:
        time = &__day->prev_ovulation_day;
        break;
    case MENSE_TYPE_OVULATION:
        time = &__day->ovulation_day;
        break;
    case MENSE_TYPE_POST_OVULATION:
        time = &__day->post_ovulation_day;
        break;
    }
    if (time) {
        mense_time_offset_day(time, time, __info->physiological_days);
    }
    M_LOG_DEBUG("status:%d (%04d-%02d-%02d )", curr_type, time->year, time->month, time->day);
    return curr_type;
}
int mense_status_map_show_type(int status)
{
    if (status == MENSE_TYPE_PREV_MENSTRUATION || status == MENSE_TYPE_POST_MENSTRUATION) {
        return MENSE_SHOW_TYPE_SAFETY_PREIOD;
    } else if (status == MENSE_TYPE_MENSTRUATION) {

        return MENSE_SHOW_TYPE_MENSTRUATION;
    } else if (status == MENSE_TYPE_OVULATION) {
        return MENSE_SHOW_TYPE_OVULATION;
    } else if (status == MENSE_TYPE_PREV_OVULATION || status == MENSE_TYPE_POST_OVULATION) {
        return MENSE_SHOW_TYPE_EASY_PREGNANCY;
    }
    ASSERT(0);
    return 0;
}

/***********************************************************************
                生理周期提醒

***********************************************************************/
int mense_resp_status_get()
{
    return __day->mense_resp_type;
}
int mense_resp_status_set(int type)
{
    __day->mense_resp_type = type;
    return __day->mense_resp_type;
}
int memset_resp_status_clr()
{
    __day->mense_resp_type = MENSE_RESP_TYPE_NONE;
    return __day->mense_resp_type;
}
int mense_resp_check()
{
    if (!__info->resp.enable) {
        M_LOG_DEBUG("%s enable:%d", __func__, __info->resp.enable);
        return -1;
    }
    struct sys_time cur_time;
    rtc_read_time(&cur_time);
    if ((cur_time.hour == __info->resp.resp_hour) && (cur_time.min == __info->resp.resp_min)) {
        if (!__day->mense_resp_flag) {
            int cur_day = __sys_time_to_day(&cur_time);
            int mense_resp_day = __sys_time_to_day(&__day->prev_menstruation_day) - __info->resp.mense_prev_day;
            int ovulation_resp_day = __sys_time_to_day(&__day->ovulation_day) - __info->resp.ovulation_prev_day;
            int easy_pregnancy_resp_day =  __sys_time_to_day(&__day->prev_ovulation_day) - __info->resp.easy_prev_day;
            if (cur_day == mense_resp_day) {
                __day->mense_resp_type = MENSE_RESP_TYPE_MENSTRUATION;
            } else if (cur_day == ovulation_resp_day) {
                __day->mense_resp_type = MENSE_RESP_TYPE_OVULATION;
            } else if (cur_day == easy_pregnancy_resp_day) {
                __day->mense_resp_type = MENSE_RESP_TYPE_EASY_PREGNANCY;
            }
            UI_SHOW_WINDOW(ID_WINDOW_PERIOD);
            __day->mense_resp_flag = 1;
        }
    } else {
        __day->mense_resp_type = MENSE_RESP_TYPE_NONE;
        __day->mense_resp_flag = 0;
    }
    return 0;
}

/***********************************************************************
                生理周期初始化与释放
    <FUNC>mense_manage_init         初始化生理周期
    <FUNC>mense_manage_release      释放生理周期
***********************************************************************/
int mense_manage_init(void)
{
    M_LOG_DEBUG();
    int ret = mense_info_get_vm();
    if (ret == -1) {
        __info->mense_period_days = 5;
        __info->physiological_days = 28;
        __info->menstruation_sday.year = 2022;
        __info->menstruation_sday.month = 10;
        __info->menstruation_sday.day = 18;
        //提醒
        __info->resp.enable = 0;
        __info->resp.mense_prev_day = 1;
        __info->resp.ovulation_prev_day = 1;
    }
    __info->resp.resp_hour = 9;
    __info->resp.resp_min = 0;
    if (mense_day_calculation() == -1) {
        M_LOG_DEBUG("err");
    }
    mense_day_dump();

    return 0;
}
int mense_manage_release(void)
{
    M_LOG_DEBUG();
//    mense_info_set_vm();
    return 0;
}

module_initcall(mense_manage_init);

#endif

