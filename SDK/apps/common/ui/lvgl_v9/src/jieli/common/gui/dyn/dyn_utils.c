/**
 * @file dyn_utils.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "dyn_utils.h"
#include <string.h>

#if LV_USE_GUI_PARSE_DYN != 0

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static size_t dyn_strftime(char *buf, size_t size, const char *fmt, const struct tm *tm);

/**********************
 *  STATIC VARIABLES
 **********************/

static const char *const dyn_wday_abbr[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char *const dyn_wday_full[7] = { "Sunday", "Monday", "Tuesday", "Wednesday",
                                              "Thursday", "Friday", "Saturday"
                                            };
static const char *const dyn_mon_abbr[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                              "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                                            };
static const char *const dyn_mon_full[12] = { "January", "February", "March", "April", "May", "June",
                                              "July", "August", "September", "October", "November", "December"
                                            };

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* 追加一段字符串，返回可写长度；不足则截断（保持 strftime 的"返回总长度"语义） */
static size_t dyn_append(char *buf, size_t size, size_t pos, const char *src, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (pos + 1U < size) {
            buf[pos] = src[i];
        }
        pos++;
    }
    return pos;
}

static size_t dyn_append_num(char *buf, size_t size, size_t pos, int val, int width, char pad)
{
    char tmp[12];
    int n = 0;

    if (val < 0) {
        val = 0;
    }
    do {
        tmp[n++] = (char)('0' + (val % 10));
        val /= 10;
    } while (val > 0 && n < (int)sizeof(tmp));

    while (n < width && n < (int)sizeof(tmp)) {
        tmp[n++] = pad;
    }
    while (n > 0) {
        if (pos + 1U < size) {
            buf[pos] = tmp[--n];
        } else {
            n--;
        }
        pos++;
    }
    return pos;
}

/* 民用历法转儒略日序数（Howard Hinnant 算法），用于推算星期 */
static int dyn_days_from_civil(int y, int m, int d)
{
    int era;
    unsigned yoe, doy, doe;

    y -= (m <= 2);
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = (unsigned)(y - era * 400);
    doy = (unsigned)((153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1);
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int)doe - 719468;
}

/*
 * 精简版 strftime。
 *
 * 本 SDK 的嵌入式 libc 不提供 strftime（符号表里没有），而 .dyn 表盘的
 * digitclock 控件格式串由 GuiBuilder 自定义，必须按格式串渲染，故在此实现。
 * 支持 %a %A %b %B %d %H %I %j %m %M %p %S %w %y %Y %%。
 * tm_wday/tm_yday 若调用方未填（prop.c 里是 memset 清零的），按日期现算。
 */
static size_t dyn_strftime(char *buf, size_t size, const char *fmt, const struct tm *tm)
{
    size_t pos = 0;
    int year, wday;

    if (buf == NULL || size == 0) {
        return 0;
    }
    buf[0] = '\0';
    if (fmt == NULL || tm == NULL) {
        return 0;
    }

    year = tm->tm_year + 1900;
    /* 星期：0=周日。调用方（prop.c）把 struct tm 整体清零，tm_wday=0 既可能是
     * "未填"也可能是"真周日"，无法区分；只要年月日看起来有效就按日期现算，
     * 否则退回调用方给的 tm_wday。 */
    if (tm->tm_year > 0 && tm->tm_mon >= 0 && tm->tm_mon < 12 &&
        tm->tm_mday >= 1 && tm->tm_mday <= 31) {
        wday = (dyn_days_from_civil(year, tm->tm_mon + 1, tm->tm_mday) + 4) % 7;
        if (wday < 0) {
            wday += 7;
        }
    } else {
        wday = tm->tm_wday;
        if (wday < 0 || wday > 6) {
            wday = 0;
        }
    }

    for (const char *p = fmt; *p != '\0'; p++) {
        if (*p != '%') {
            pos = dyn_append(buf, size, pos, p, 1);
            continue;
        }
        p++;
        switch (*p) {
        case 'a':
            pos = dyn_append(buf, size, pos, dyn_wday_abbr[wday], 3);
            break;
        case 'A':
            pos = dyn_append(buf, size, pos, dyn_wday_full[wday], strlen(dyn_wday_full[wday]));
            break;
        case 'b':
        case 'h':
            pos = dyn_append(buf, size, pos, dyn_mon_abbr[tm->tm_mon], 3);
            break;
        case 'B':
            pos = dyn_append(buf, size, pos, dyn_mon_full[tm->tm_mon], strlen(dyn_mon_full[tm->tm_mon]));
            break;
        case 'd':
            pos = dyn_append_num(buf, size, pos, tm->tm_mday, 2, '0');
            break;
        case 'e':
            pos = dyn_append_num(buf, size, pos, tm->tm_mday, 2, ' ');
            break;
        case 'H':
            pos = dyn_append_num(buf, size, pos, tm->tm_hour, 2, '0');
            break;
        case 'I': {
            int h12 = tm->tm_hour % 12;
            pos = dyn_append_num(buf, size, pos, h12 ? h12 : 12, 2, '0');
            break;
        }
        case 'm':
            pos = dyn_append_num(buf, size, pos, tm->tm_mon + 1, 2, '0');
            break;
        case 'M':
            pos = dyn_append_num(buf, size, pos, tm->tm_min, 2, '0');
            break;
        case 'p':
            pos = dyn_append(buf, size, pos, (tm->tm_hour < 12) ? "AM" : "PM", 2);
            break;
        case 'S':
            pos = dyn_append_num(buf, size, pos, tm->tm_sec, 2, '0');
            break;
        case 'w':
            pos = dyn_append_num(buf, size, pos, wday, 1, '0');
            break;
        case 'y':
            pos = dyn_append_num(buf, size, pos, year % 100, 2, '0');
            break;
        case 'Y':
            pos = dyn_append_num(buf, size, pos, year, 4, '0');
            break;
        case '%':
            pos = dyn_append(buf, size, pos, "%", 1);
            break;
        case '\0':
            return pos;     /* 格式串以 '%' 结尾，忽略 */
        default:
            pos = dyn_append(buf, size, pos, "%", 1);
            pos = dyn_append(buf, size, pos, p, 1);
            break;
        }
    }

    if (pos < size) {
        buf[pos] = '\0';
    } else {
        buf[size - 1U] = '\0';
    }
    return pos;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool dyn_read_string_alloc(const uint8_t **p, const uint8_t *end, char **out)
{
    uint32_t len;
    dyn_view_t view;
    if (!dyn_read_le32(p, end, &len) || !dyn_read_bytes_view(p, end, len, &view) || !dyn_skip_padding4(p, end, len)) {
        return false;
    }

    char *text = lv_malloc(len + 1U);
    if (text == NULL) {
        return false;
    }
    lv_memset(text, 0, len + 1U);
    lv_memcpy(text, view.ptr, len);
    *out = text;
    return true;
}

void dyn_digitclock_set_text(lv_obj_t *obj, const char *fmt, const struct tm *time)
{
    char text[128];
    size_t len = dyn_strftime(text, sizeof(text), fmt, time);
    if (len == 0) {
        text[0] = '\0';
    }
    lv_label_set_text(obj, text);
}

#endif /*LV_USE_GUI_PARSE_DYN*/
