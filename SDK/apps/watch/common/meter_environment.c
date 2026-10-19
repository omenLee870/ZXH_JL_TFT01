/**
 * @file meter_environment.c
 * @brief 经 BLE 接收手机时间、时区和天气，维护 RTC 校时与两小时天气有效期。
 * @note 不把天气写入 Flash；时钟有效标记最多每日保存一次，避免频繁擦写。
 */
#include "app_config.h"
#include "system/includes.h"
#include "rtc.h"
#include "user_cfg_id.h"
#include "zxh_ble.h"
#include "zxh_vehicle_protocol.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "meter_environment.h"
#include <string.h>
#include <stdlib.h>
#define CLOCK_MAGIC 0x434c4b31u
#define MIN_UTC 1577836800u
#define MAX_UTC 4102444799u
#define WEATHER_TTL 7200u
static OS_SEM gate;
static volatile unsigned ready; /**< app_core 初始化完成后向 UI 线程发布。 */
static uint32_t tick_ms, weather_age_ms;
static struct meter_environment state;
static struct clock_record { uint32_t magic, utc; int32_t zone; uint32_t crc; } saved;
static struct { char id[33]; int64_t a, b, c; char action; int code; } replies[8];
static unsigned reply_next;
static unsigned local_result_sequence;
static int local_result_code;
static int leap(unsigned y) { return !(y % 4) && (y % 100 || !(y % 400)); }
static unsigned days(unsigned y, unsigned m) {
    static const unsigned char d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return d[m - 1] + (m == 2 && leap(y));
}
/** @brief 使用固定公历换算，避免 SDK 的整数小时时区接口截断半小时、四十五分钟时区。 */
static void calendar(uint32_t local, struct sys_time *out, unsigned *weekday) {
    uint32_t day = local / 86400u, remainder = local % 86400u;
    unsigned year = 1970, month = 1;
    *weekday = (day + 4) % 7;
    while (day >= (unsigned)(365 + leap(year))) day -= 365 + leap(year++);
    while (day >= days(year, month)) day -= days(year, month++);
    out->year = year; out->month = month; out->day = day + 1;
    out->hour = remainder / 3600; out->min = remainder / 60 % 60; out->sec = remainder % 60;
}
static int rtc_epoch(const struct sys_time *t, int zone, uint32_t *utc) {
    uint32_t day = 0; unsigned y, m;
    if (t->year < 2020 || t->year > 2100 || !t->month || t->month > 12 || !t->day ||
        t->day > days(t->year, t->month) || t->hour > 23 || t->min > 59 || t->sec > 59) return -1;
    for (y = 1970; y < t->year; y++) day += 365 + leap(y);
    for (m = 1; m < t->month; m++) day += days(t->year, m);
    int64_t value = (int64_t)(day + t->day - 1) * 86400 + t->hour * 3600 + t->min * 60 + t->sec - zone * 60;
    if (value < MIN_UTC || value > MAX_UTC) return -1;
    *utc = (uint32_t)value; return 0;
}
static void format(struct meter_environment *p) {
    static const char *week[] = {"周日","周一","周二","周三","周四","周五","周六"};
    struct sys_time t; unsigned w;
    if (!p->clock_valid) { strcpy(p->clock, "--:--"); strcpy(p->date, "-- --"); return; }
    calendar((uint32_t)((int64_t)p->utc + p->timezone_minutes * 60), &t, &w);
    p->hour = t.hour;
    snprintf(p->clock, sizeof(p->clock), "%02u:%02u", t.hour, t.min);
    snprintf(p->date, sizeof(p->date), "%02u-%02u %s", t.month, t.day, week[w]);
}
/** @brief 单调时间推进使蓝牙断连、仪表待机、系统毫秒计数回绕均不停止计时。 */
static void tick(void *unused) {
    uint32_t now = sys_timer_get_ms();
    (void)unused;
    os_sem_pend(&gate, 0);
    uint32_t elapsed = now - tick_ms;
    unsigned seconds = elapsed / 1000;
    tick_ms += seconds * 1000;
    if (state.clock_valid && seconds) {
        if (MAX_UTC - state.utc < seconds) state.clock_valid = 0;
        else state.utc += seconds;
    }
    if (state.weather_valid && ((uint32_t)(now - weather_age_ms) >= WEATHER_TTL * 1000u ||
        (state.clock_valid && (state.utc < state.observed_utc - 300u || state.utc >= state.observed_utc + WEATHER_TTL))))
        state.weather_valid = 0;
    format(&state);
    os_sem_post(&gate);
}
void meter_environment_snapshot(struct meter_environment *out) {
    if (!out) return;
    if (!ready) { memset(out, 0, sizeof(*out)); format(out); return; }
    os_sem_pend(&gate, 0); *out = state; os_sem_post(&gate);
}
static uint32_t record_crc(const struct clock_record *p) { return p->magic ^ p->utc ^ (uint32_t)p->zone ^ 0x9e3779b9u; }
static int set_clock(uint32_t utc, int zone) {
    struct sys_time t, check; unsigned w; uint32_t actual;
    calendar((uint32_t)((int64_t)utc + zone * 60), &t, &w);
    rtc_write_time(&t); memset(&check, 0, sizeof(check)); rtc_read_time(&check);
    if (rtc_epoch(&check, zone, &actual) || actual < utc || actual - utc > 2) return ZXH_EXECUTION_FAILED;
    os_sem_pend(&gate, 0);
    state.utc = actual; state.timezone_minutes = zone; state.clock_valid = 1; tick_ms = sys_timer_get_ms(); format(&state);
    os_sem_post(&gate);
    extern void set_local_time_init_flag(unsigned char);
    set_local_time_init_flag(0);
    if (saved.magic != CLOCK_MAGIC || saved.zone != zone || saved.utc / 86400 != utc / 86400) {
        struct clock_record record = {CLOCK_MAGIC, utc, zone, 0}; record.crc = record_crc(&record);
        int written = syscfg_write(CFG_METER_CLOCK, &record, sizeof(record)), flushed = VM_ERR_NONE;
        if (written == sizeof(record) && (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable())) flushed = vm_flush2flash(0);
        if (written == sizeof(record) && (flushed == VM_ERR_NONE || flushed == VM_RAM_FLUSH_NONE)) saved = record;
        else printf("[METER_ENV] clock marker write=%d flush=%d\n", written, flushed);
    }
    return ZXH_OK;
}
int meter_environment_local_time(struct meter_local_time *out) {
    struct meter_environment p; struct sys_time t; unsigned weekday;
    if (!out) return -1;
    meter_environment_snapshot(&p);
    if (!p.clock_valid) return -1;
    calendar((uint32_t)((int64_t)p.utc + p.timezone_minutes * 60), &t, &weekday);
    out->year=t.year; out->month=t.month; out->day=t.day; out->hour=t.hour; out->minute=t.min;
    return 0;
}
/** @brief 日期和时分分别打包为值传递，不能把 UI 栈上的地址放进异步任务队列。 */
static void local_time_apply(int date, int time) {
    struct sys_time t; struct meter_environment p; uint32_t utc;
    extern int ui_update_page_is_active(void);
    int code=ZXH_PARAMETER_ERROR;
    memset(&t,0,sizeof(t));
    t.year=(unsigned)date>>16; t.month=((unsigned)date>>8)&255; t.day=date&255;
    t.hour=((unsigned)time>>8)&255; t.min=time&255;
    meter_environment_snapshot(&p);
    if (ui_update_page_is_active()) code=ZXH_BUSY;
    else if (!strcmp(os_current_task(),"app_core") && !rtc_epoch(&t,p.timezone_minutes,&utc))
        code=set_clock(utc,p.timezone_minutes);
    os_sem_pend(&gate,0);
    local_result_code=code; local_result_sequence++;
    os_sem_post(&gate);
}
int meter_environment_time_post(const struct meter_local_time *p) {
    if (!ready || !p || p->year<2020 || p->year>2099 || p->month<1 || p->month>12 ||
        p->day<1 || p->day>days(p->year,p->month) || p->hour>23 || p->minute>59) return -1;
    int date=(p->year<<16)|(p->month<<8)|p->day, time=(p->hour<<8)|p->minute;
    int argv[4]={(int)local_time_apply,2,date,time};
    return os_taskq_post_type("app_core",Q_CALLBACK,4,argv)?-1:0;
}
unsigned meter_environment_time_result(int *code) {
    unsigned sequence;
    if (!ready) return 0;
    os_sem_pend(&gate,0); sequence=local_result_sequence;
    if (code) *code=local_result_code;
    os_sem_post(&gate); return sequence;
}
static const char *text(struct json_object *p, const char *key) {
    struct json_object *v = p ? json_object_object_get(p, key) : NULL;
    return v && json_object_is_type(v, json_type_string) ? json_object_get_string(v) : "";
}
static int integer(struct json_object *p, const char *key, int64_t low, int64_t high, int64_t *value) {
    const char *s = text(p, key), *q = s; int negative = *q == '-'; int64_t v = 0;
    if (negative) q++;
    if (!*q || strlen(q) > 10) return -1;
    for (; *q; q++) { if (*q < '0' || *q > '9') return -1; v = v * 10 + *q - '0'; }
    if (negative) v = -v;
    if (v < low || v > high) return -1;
    *value = v; return 0;
}
static void field(struct json_object *p, const char *key, const char *value) { json_object_object_add(p, key, json_object_new_string(value)); }
static void reply(void *client, const char *id, const char *action, int code) {
    struct json_object *r = json_object_new_object(), *p = json_object_new_object(); char number[16];
    if (!r || !p) { if (r) json_object_put(r); if (p) json_object_put(p); return; }
    field(r, "topic", "ZXH_ENV"); field(r, "op", "NOTIFY"); json_object_object_add(r, "errno", json_object_new_int(0));
    field(p, "v", "1"); field(p, "id", id); field(p, "action", action); field(p, "phase", "result");
    snprintf(number, sizeof(number), "%d", code); field(p, "code", number); field(p, "confirmed", code ? "0" : "1");
    json_object_object_add(r, "param", p); zxh_ble_send(client, json_object_to_json_string(r)); json_object_put(r);
}
/** @brief 严格校验及请求去重，旧消息不能重复把 RTC 拨回过去，也不能延长旧天气寿命。 */
int meter_environment_receive(void *client, const char *topic, const char *json) {
    if (strcmp(topic, "ZXH_ENV")) return 0;
    if (!ready || !zxh_ble_client_active(client)) return 1;
    struct json_object *root = json_tokener_parse(json), *p = root ? json_object_object_get(root, "param") : NULL;
    const char *id = text(p, "id"), *action = text(p, "action");
    int code = ZXH_PARAMETER_ERROR; int64_t a, b, c;
    if (!p || !json_object_is_type(p, json_type_object)) goto done;
    if (strlen(id) != 32 || strspn(id, "0123456789abcdef") != 32) { id = ""; action = ""; goto done; }
    if (strcmp(text(root, "op"), "PUT") || strcmp(text(p, "v"), "1")) goto done;
    if (!strcmp(action, "TIME")) {
        if (integer(p, "utc_s", MIN_UTC, MAX_UTC, &a) || integer(p, "tz_min", -720, 840, &b)) goto done;
        c = 0;
    } else if (!strcmp(action, "WEATHER")) {
        if (integer(p, "observed_utc_s", MIN_UTC, MAX_UTC, &a) || integer(p, "temp_dc", -1000, 700, &b) || integer(p, "weather", 0, 8, &c)) goto done;
    } else { action = ""; goto done; }
    unsigned i;
    for (i = 0; i < 8; i++) if (!strcmp(replies[i].id, id)) {
        code = replies[i].a == a && replies[i].b == b && replies[i].c == c && replies[i].action == action[0] ? replies[i].code : ZXH_ID_CONFLICT; goto done;
    }
    if (!strcmp(action, "TIME")) {
        /* 升级期间不竞争 Flash；保持原来的单调计时，手机稍后会重新校准。 */
        extern int ui_update_page_is_active(void);
        code = ui_update_page_is_active() ? ZXH_BUSY : set_clock((uint32_t)a, (int)b);
    }
    else {
        tick(NULL);
        os_sem_pend(&gate, 0);
        if (state.clock_valid && a <= (int64_t)state.utc + 300 && (int64_t)state.utc - a < WEATHER_TTL) {
            unsigned age = a < state.utc ? state.utc - (uint32_t)a : 0;
            state.observed_utc = a; state.temperature_dc = b; state.weather = c; state.weather_valid = 1;
            weather_age_ms = sys_timer_get_ms() - age * 1000; code = ZXH_OK;
        }
        os_sem_post(&gate);
    }
    i = reply_next++ % 8; strcpy(replies[i].id, id); replies[i].a = a; replies[i].b = b; replies[i].c = c; replies[i].action = action[0]; replies[i].code = code;
done:
    reply(client, id, action, code); if (root) json_object_put(root); return 1;
}
int meter_environment_init(void) {
    if (ready) return 0;
    if (strcmp(os_current_task(), "app_core") || os_sem_create(&gate, 1)) return -1;
    memset(&state, 0, sizeof(state)); state.timezone_minutes = 480;
    struct sys_time rtc; uint32_t utc;
    if (syscfg_read(CFG_METER_CLOCK, &saved, sizeof(saved)) == sizeof(saved) && saved.magic == CLOCK_MAGIC &&
        saved.crc == record_crc(&saved) && saved.zone >= -720 && saved.zone <= 840 && saved.utc >= MIN_UTC && saved.utc <= MAX_UTC) {
        memset(&rtc, 0, sizeof(rtc)); rtc_read_time(&rtc);
        if (!rtc_epoch(&rtc, saved.zone, &utc) && utc >= saved.utc) {
            state.utc = utc; state.timezone_minutes = saved.zone; state.clock_valid = 1;
        }
    } else memset(&saved, 0, sizeof(saved));
    tick_ms = sys_timer_get_ms(); format(&state);
    if (!sys_timer_add(NULL, tick, 1000)) return -1;
    ready = 1; return 0;
}
