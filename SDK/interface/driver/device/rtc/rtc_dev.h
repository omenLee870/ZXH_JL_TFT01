#ifndef __RTC_DEV_H__
#define __RTC_DEV_H__

#include "typedef.h"
#include "utils/sys_time.h"
extern const bool control_rtc_enable;
extern const u32 control_rtc_clk_sel;


#if (defined(CONFIG_CPU_BR28) && CONFIG_CPU_BR28 || \
      defined(CONFIG_CPU_BR27) && CONFIG_CPU_BR27)

#define RTC_USE_HW_SFR  1
#else
#define RTC_USE_HW_SFR  0
#endif

enum {
    RTC_TIMER_WKUP = 1,
    RTC_TIMER_OVERFLOW_WKUP,
};

#define CLK_SEL_32K 1
#define CLK_SEL_12M 2
#define CLK_SEL_24M 3
#define CLK_SEL_LRC 4

enum {
    RTC_UNACCESSIBLE = 0,
    RTC_ACCESSIBLE_TIME_UNRELIABLE,
    RTC_ACCESSIBLE_TIME_RELIABLE,
    RTC_SUCC,
    RTC_ERROR_ALM_DIS,
    RTC_ERROR_ALM_NO_EFFECT,
};


enum rtc_event {
    RTC_ALARM_EVENT,
    RTC_WKUP_EVENT,
    RTC_1HZ_EVENT,
    RTC_TRIM_EVENT,
};

enum rtc_msys2pmc_type {
    RTC_RESET_TYPE,
    RTC_WKUP_TYPE,
    RTC_INIT_LRC_TYPE,
    RTC_INIT_32K_TYPE,
    RTC_WRITE_ALARM_TYPE,
};

struct rtc_m2p_post_envnt {
    u64 value;
    u8 type;
};

struct rtc_event_t {
    enum rtc_event event;
};

struct rtc_dev_platform_data {
    const struct sys_time *default_sys_time;
    void (*cbfun)(const struct rtc_event_t *);
    u8 trim_interval;  //单位：分钟
};

#define RTC_DEV_PLATFORM_DATA_BEGIN(data) \
	const struct rtc_dev_platform_data data = {

#define RTC_DEV_PLATFORM_DATA_END()  \
};

struct sys_time_ms {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u16 sec: 6;
    u16 msec: 10;
};

/*----------------------------------------------------------------------------*/
/**@brief	判断是否为闰年
   @param 	year: 年份
   @return	true 表示闰年，false 表示平年
------------------------------------------------------------------------------*/
bool is_leap_year(u32 year);
/*----------------------------------------------------------------------------*/
/**@brief	判断是否闰年，返回当前年份天数
   @param 	year:年份
   @return  当年天数
------------------------------------------------------------------------------*/
u32 days_in_year(u32 year);
/*----------------------------------------------------------------------------*/
/**@brief	月份换算为天数
   @param 	year:年份
   @param 	month:月份
   @return  当月天数
------------------------------------------------------------------------------*/
u32 days_in_month(u32 year, u32 month);
/*----------------------------------------------------------------------------*/
/**@brief	将UTC时间转换为自1970-01-01 00:00:00起的时间戳（秒）
   @param 	time: 当前UTC时间
   @return	自1970-01-01起累计的秒数（时间戳）
------------------------------------------------------------------------------*/
u64 datetime_to_timestamp(const struct sys_time *time);

/*----------------------------------------------------------------------------*/
/**@brief	将UTC时间转换为自1970-01-01 00:00:00起的时间戳（秒）
   @param 	time: 当前UTC时间
   @return	自1970-01-01起累计的微秒数（时间戳）
------------------------------------------------------------------------------*/
u64 datetime_to_timestamp_us(const struct sys_time *time);
/*----------------------------------------------------------------------------*/
/**@brief	将时间戳(自1970-01-01 00:00:00起的秒数)转换为UTC时间
   @param 	total_seconds: 自1970-01-01起的总秒数
   @param 	time: 输出的时间结构体 (年月日时分秒)
------------------------------------------------------------------------------*/
void timestamp_to_datetime(u64 total_seconds, struct sys_time *time);

void timestamp_to_datetime_ms(u64 us, struct sys_time_ms *time);
/*----------------------------------------------------------------------------*/
/**@brief	根据日期计算星期几
   @param 	time: 当前日期
   @return  星期几 (1~7，星期一~星期日)
------------------------------------------------------------------------------*/
u32 get_weekday_from_time(const struct sys_time *time);
/*----------------------------------------------------------------------------*/
/**@brief	计算两个时间之间的秒差
   @param 	old_time: 旧时间
   @param 	curr_time: 当前时间
   @return	秒差 (curr_time - old_time)
------------------------------------------------------------------------------*/
s32 get_seconds_delta(const struct sys_time *old_time, const struct sys_time *curr_time);
/*----------------------------------------------------------------------------*/
/**@brief	给当前时间增加指定秒数
   @param 	time: 输入输出时间结构体
   @param 	sec: 要增加的秒数（可为负数）
------------------------------------------------------------------------------*/
void datetime_add_seconds(struct sys_time *time, s32 sec);

void printf_datetime(const struct sys_time *time);

/*----------------------------------------------------------------------------*/
/**@brief	总天数换算为年份/月份/日期
   @param 	day：总天数
   @param 	time：当前日期
   @note	void day_to_ymd(u16 day,sstruct sys_time *sys_time)
*/
/*----------------------------------------------------------------------------*/
void day_to_ymd(u32 day, struct sys_time *sys_time);

/*----------------------------------------------------------------------------*/
/**@brief	当前年份/月份/日期换算成总天数
   @param 	time:当前日期
   @return  天数
   @note	u16 ymd_to_day(sstruct sys_time *time,u16 day)
*/
/*----------------------------------------------------------------------------*/
u32 ymd_to_day(const struct sys_time *time);

extern const struct device_operations rtc_dev_ops;


int rtc_port_pr_read(u32 port);
int rtc_port_pr_out(u32 port, u32 value);
int rtc_port_pr_dir(u32 port, u32 dir);
int rtc_port_pr_die(u32 port, u32 die);
int rtc_port_pr_pu(u32 port, u32 value);
int rtc_port_pr_pu1(u32 port, u32 value);
int rtc_port_pr_pd(u32 port, u32 value);
int rtc_port_pr_pd1(u32 port, u32 value);
int rtc_port_pr_hd0(u32 port, u32 value);
int rtc_port_pr_hd1(u32 port, u32 value);

/**
 * @brief 设置rtc定时器溢出后的回调函数 必须在rtc_init函数前调用
 * @param 回调函数skip_softoff_callback 返回1:跳过rtc定时器溢出后自动关机  返回0:rtc定时器溢出后自动关机
 */
void rtc_set_overflow_callback(u32(*skip_softoff_callback)(void));

int rtc_init(const struct rtc_dev_platform_data *arg);
u32 rtc_dev_deinit(void);
void rtc_wakup_source();
u32 rtc_read_time(struct sys_time *curr_time);
u32 rtc_read_time_ms(struct sys_time_ms *curr_time);
u32 rtc_write_time(const struct sys_time *curr_time);
u32 rtc_read_alarm(struct sys_time *alarm_time);
u32 rtc_write_alarm(const struct sys_time *alarm_time);
void rtc_debug_dump(void);
void rtc_alarm_switch(u32 en);//开关
u32 rtc_is_alarm_en(void);
u32 rtc_is_soff_need_keep_clk(void);
void rtc_save_context_to_vm(void);

void rtc_lptmr_wakeup_enable(uint32_t wkup_ms);
u64 rtc_sys_timestamp_us(void);
void rtc_1hz_wkup_switch(u32 enable);

#endif // __RTC_API_H__
