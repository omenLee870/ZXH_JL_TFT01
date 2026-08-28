#include "time.h"
#include <sys/time.h>
#include "system/timer.h"
#include "os/os_api.h"
#include "perf_counter/perf_counter.h"
#include "device/device.h"
#include "rtc.h"

static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", ""
};

static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""
};

/** 时间补偿值(毫秒) */
static u32 t_compensate_ms;

/** 时间补偿值(秒) */
static time_t t_compensate;

/** 源计时器值 */
static time_t source_timer;

/** 本地时间初始化标志 */
static u8 local_time_init_flag;

/** 本地时区设置(默认东八区) */
static s8 local_timezone = 8;

extern unsigned long jiffies_msec();

/* uint32_t timer_get_ms(void) */
/* { */
/* return jiffies_msec(); */
/* } */

/* uint32_t timer_get_sec(void) */
/* { */
/* return timer_get_ms() / 1000; */
/* } */

void set_sys_source_timer(time_t time)
{
    source_timer = time;
}

void set_local_time_init_flag(u8 flag)
{
    local_time_init_flag = flag;
}

void set_time_compensate_ms(u32 ms)
{
    t_compensate_ms = ms;
}

void set_time_compensate_sec(u32 sec)
{
    t_compensate = sec;
}

void set_local_timezone(s8 timezone)
{
    local_timezone = timezone;
}

s8 get_local_timezone(void)
{
    return local_timezone;
}


time_t rtc_time_change_to_unix_timestamp(struct sys_time *rtc_time)
{
    struct tm t = {0};
    t.tm_year = rtc_time->year - 1900;
    t.tm_mon = rtc_time->month - 1;
    t.tm_mday = rtc_time->day;
    t.tm_hour = rtc_time->hour;
    t.tm_min = rtc_time->min;
    t.tm_sec = rtc_time->sec;
    return mktime(&t) - local_timezone * 3600;
}

__attribute__((weak))
time_t time(time_t *timer)
{

    time_t t;
    struct sys_time st;
    struct tm pt;

    if (!local_time_init_flag) {
        memset(&st, 0, sizeof(struct sys_time));
        rtc_read_time(&st);
        printf("get_sys_time : %d-%d-%d,%d:%d:%d\n", st.year, st.month, st.day, st.hour, st.min, st.sec);
        pt.tm_year = st.year - 1900;
        pt.tm_mon = st.month - 1;
        pt.tm_mday = st.day;
        pt.tm_hour = st.hour;
        pt.tm_min = st.min;
        pt.tm_sec = st.sec;
        source_timer = mktime(&pt) - 28800;
        t_compensate = (time_t)timer_get_sec();
        t_compensate_ms = timer_get_ms();
        local_time_init_flag = 1;
    }

    t = source_timer + (time_t)timer_get_sec() - t_compensate;

    if (timer != (time_t *)NULL) {
        *timer = t;
    }

    return (time_t)t;
}

u64 time_ms(void)
{
    u32 differ = 0;

    time(NULL);

    differ = timer_get_ms();

    if (differ < t_compensate_ms) {
        differ = 0xffffffff - t_compensate_ms + differ + 1;
        t_compensate_ms = timer_get_ms();
    } else {
        differ -= t_compensate_ms;
    }

    return (u64)time(NULL) * 1000 + differ % 1000;
}

static const u16 yday_list[12] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335};
static const u8 mon_list[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const u8 leap_mon_list[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static struct tm s_tm;

struct tm *localtime(const time_t *clock)	//已弃用
{
    const u8 *pDays = NULL;
    struct tm *pT = &s_tm;
    time_t tim = *clock;

    u8 is_leap_year;
    u16 index = 0;

    //year initialization
    if (tim > 0x5685C180L) {            // 2016-1-1 0:0:0
        pT->tm_year = 2016;
        tim -= 0x5685C180L;
    } else if (tim > 0x4B3D3B00L) {     // 2010-1-1 0:0:0
        pT->tm_year = 2010;
        tim -= 0x4B3D3B00L;
    } else if (tim > 0x386D4380L) {     // 2000-1-1 0:0:0
        pT->tm_year = 2000;
        tim -= 0x386D4380L;
    } else {
        pT->tm_year = 1970;
    }

    //now have year
    while (tim >= 366L * 24 * 60 * 60) {
        if ((pT->tm_year % 4 == 0) && ((pT->tm_year  % 100 != 0) || (pT->tm_year  % 400 == 0))) {
            tim -= 366L * 24 * 60 * 60;
        } else {
            tim -= 365L * 24 * 60 * 60;
        }
        pT->tm_year++;
    }

    // then 365 * 24 * 60 * 60 < tim < 366 * 24 * 60 * 60
    if (!(((pT->tm_year % 4 == 0) && ((pT->tm_year  % 100 != 0) || (pT->tm_year  % 400 == 0))))
        && (tim > 365L * 24 * 60 * 60)) {
        tim -= 365L * 24 * 60 * 60;
        pT->tm_year++;
    }

    // this year is a leap year?
    if (((pT->tm_year % 4 == 0) && ((pT->tm_year  % 100 != 0) || (pT->tm_year  % 400 == 0)))) {
        pDays = leap_mon_list;
        is_leap_year = 1;
    } else {
        pDays = mon_list;
        is_leap_year = 0;
    }

    pT->tm_mon = 1;
    // now have mon
    while (tim > pDays[index] * 24L * 60 * 60) {
        tim -= pDays[index] * 24L * 60 * 60;
        index++;
        pT->tm_mon++;
    }

    // now have days
    pT->tm_mday = tim / (24L * 60 * 60) + 1;
    tim = tim % (24L * 60 * 60);

    // now have hour
    pT->tm_hour = tim / (60 * 60);
    tim = tim % (60 * 60);

    // now have min
    pT->tm_min = tim / 60;
    tim = tim % 60;

    pT->tm_sec = tim;

    pT->tm_yday = yday_list[pT->tm_mon - 1] + pT->tm_mday;
    if (pT->tm_mon > 2 && is_leap_year) {
        --pT->tm_yday;
    }
    pT->tm_isdst = -1;

    u16 y = pT->tm_year;
    u8 m = pT->tm_mon;
    u8 d = pT->tm_mday;

    if (m == 1 || m == 2) {
        m += 12;
        y--;
    }
    pT->tm_wday = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7 + 1; //1~7  星期1 ~星期7
    if (pT->tm_wday == 7) {
        pT->tm_wday = 0;
    }

    return pT;
}

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
    const u8 *pDays = NULL;
    struct tm *pT = result;
    time_t tim = *timep;

    u8 is_leap_year;
    u16 index = 0;

    //year initialization
    if (tim > 0x5685C180L) {            // 2016-1-1 0:0:0
        pT->tm_year = 2016;
        tim -= 0x5685C180L;
    } else if (tim > 0x4B3D3B00L) {     // 2010-1-1 0:0:0
        pT->tm_year = 2010;
        tim -= 0x4B3D3B00L;
    } else if (tim > 0x386D4380L) {     // 2000-1-1 0:0:0
        pT->tm_year = 2000;
        tim -= 0x386D4380L;
    } else {
        pT->tm_year = 1970;
    }

    //now have year
    while (tim >= 366L * 24 * 60 * 60) {
        if ((pT->tm_year % 4 == 0) && ((pT->tm_year  % 100 != 0) || (pT->tm_year  % 400 == 0))) {
            tim -= 366L * 24 * 60 * 60;
        } else {
            tim -= 365L * 24 * 60 * 60;
        }
        pT->tm_year++;
    }

    // then 365 * 24 * 60 * 60 < tim < 366 * 24 * 60 * 60
    if (!(((pT->tm_year % 4 == 0) && ((pT->tm_year  % 100 != 0) || (pT->tm_year  % 400 == 0))))
        && (tim > 365L * 24 * 60 * 60)) {
        tim -= 365L * 24 * 60 * 60;
        pT->tm_year++;
    }

    // this year is a leap year?
    if (((pT->tm_year % 4 == 0) && ((pT->tm_year  % 100 != 0) || (pT->tm_year  % 400 == 0)))) {
        pDays = leap_mon_list;
        is_leap_year = 1;
    } else {
        pDays = mon_list;
        is_leap_year = 0;
    }

    pT->tm_mon = 1;
    // now have mon
    while (tim > pDays[index] * 24L * 60 * 60) {
        tim -= pDays[index] * 24L * 60 * 60;
        index++;
        pT->tm_mon++;
    }

    // now have days
    pT->tm_mday = tim / (24L * 60 * 60) + 1;
    tim = tim % (24L * 60 * 60);

    // now have hour
    pT->tm_hour = tim / (60 * 60);
    tim = tim % (60 * 60);

    // now have min
    pT->tm_min = tim / 60;
    tim = tim % 60;

    pT->tm_sec = tim;

    pT->tm_yday = yday_list[pT->tm_mon - 1] + pT->tm_mday;
    if (pT->tm_mon > 2 && is_leap_year) {
        --pT->tm_yday;
    }
    pT->tm_isdst = -1;

    u16 y = pT->tm_year;
    u8 m = pT->tm_mon;
    u8 d = pT->tm_mday;

    if (m == 1 || m == 2) {
        m += 12;
        y--;
    }
    pT->tm_wday = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400) % 7 + 1; //1~7  星期1 ~星期7
    if (pT->tm_wday == 7) {
        pT->tm_wday = 0;
    }

    pT->tm_year -= 1900;
    pT->tm_mon -= 1;

    return pT;
}


#if 0
struct tm *gmtime(const time_t *time)
{
    return localtime(time);
}
struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
    return localtime_r(timep, result);
}
#endif

int gettimeofday(struct timeval *tv, void *tz)
{
    if (tv) {
        time_t t = (time_t)timer_get_ms();

        tv->tv_sec = t / 1000;
        tv->tv_usec = t % 1000 * 1000;
    }
    if (tz) {
        ((struct timezone *)tz)->tz_minuteswest = 0;
        ((struct timezone *)tz)->tz_dsttime = 0;
    }

    return 0;
}

size_t strftime_2(char *ptr, size_t maxsize, const char *format, const struct tm *timeptr)
{
    if ((timeptr == NULL) || (ptr == NULL)) {
        return -1;
    }

    if (timeptr->tm_mday < 10) {
        return snprintf(ptr, maxsize, "%s, 0%d %s %d %02d:%02d:%02d GMT",
                        wday_name[timeptr->tm_wday],
                        timeptr->tm_mday,
                        mon_name[timeptr->tm_mon],
                        timeptr->tm_year + 1900,
                        timeptr->tm_hour,
                        timeptr->tm_min,
                        timeptr->tm_sec);
    } else {
        return snprintf(ptr, maxsize, "%s, %d %s %d %02d:%02d:%02d GMT",
                        wday_name[timeptr->tm_wday],
                        timeptr->tm_mday,
                        mon_name[timeptr->tm_mon],
                        timeptr->tm_year + 1900,
                        timeptr->tm_hour,
                        timeptr->tm_min,
                        timeptr->tm_sec);
    }
}

int asctime_s(char *_Buf, unsigned int numberOfElements, const struct tm *_tm)
{
    if ((_tm == NULL) || (_Buf == NULL)) {
        return 1;
    }

    snprintf(_Buf, numberOfElements, "%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",
             wday_name[_tm->tm_wday],
             mon_name[_tm->tm_mon],
             _tm->tm_mday, _tm->tm_hour,
             _tm->tm_min, _tm->tm_sec,
             1900 + _tm->tm_year);

    return 0;
}

char *timezone1(void)
{
    puts("not yet define timezone !!\n");
    return "timezone";
}

void tzset(void)
{
//    puts("not yet define tzset !!\n");
}

int cal_days(int year, int month)
{
    int days;

    switch (month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        days = 31;
        break;

    case 4:
    case 6:
    case 9:
    case 11:
        days = 30;
        break;

    case 2:
        days = 28 + ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
        break;
    }

    return days;
}

time_t mktime(struct tm *pT)
{
    unsigned int year = pT->tm_year + 1900;
    unsigned int mon = pT->tm_mon + 1;
    unsigned int day = pT->tm_mday;
    unsigned int hour = pT->tm_hour;
    unsigned int min = pT->tm_min;
    unsigned int sec = pT->tm_sec;

    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int)(mon -= 2)) {
        mon += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((unsigned long)
              (year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) +
              year * 365 - 719499
             ) * 24 + hour /* now have hours */
            ) * 60 + min /* now have minutes */
           ) * 60 + sec; /* finally seconds */
}

void set_rtc_time_base_on_unix_timestamp(time_t time)
{
    struct tm pt = {0};
    struct sys_time st = {0};
    localtime_r(&time, &pt);
    st.year = pt.tm_year + 1900;
    st.month = pt.tm_mon + 1;
    st.day = pt.tm_mday;
    st.hour = pt.tm_hour;
    st.min = pt.tm_min;
    st.sec = pt.tm_sec;
    printf("set_sys_time : %d-%d-%d,%d:%d:%d\n", st.year, st.month, st.day, st.hour, st.min, st.sec);
    rtc_write_time(&st);
}

