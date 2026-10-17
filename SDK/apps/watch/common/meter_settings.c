/**
 * @file meter_settings.c
 * @brief 在 app_core 串行保存本机设置并处理 BLE 确认；锁内只复制数据。
 * @note 单位只影响显示；清零操作只修改 TRIP，累计里程始终保留。
 */
#include "app_config.h"
#include "user_cfg_id.h"
#include "system/includes.h"
#include "audio_config.h"
#include "app_meter_control.h"
#include "app_vehicle.h"
#include "vehicle_controller.h"
#include "zxh_ble.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "meter_settings.h"
#include <string.h>
#include <stdlib.h>

#define PREF_MAGIC 0x4d535431u
#define PREF_VM CFG_METER_SETTINGS
struct preference_record
{
    uint32_t magic;
    struct meter_preferences value;
    uint32_t crc;
};
static const struct meter_preferences defaults = {70, 50, 30, 1, 0, 0, 1};
static struct meter_preferences preferences = {70, 50, 30, 1, 0, 0, 1};
static OS_SEM gate;
static unsigned ready, result_sequence;
static int last_result;
static const char *keys[] = {"BRIGHTNESS", "MUSIC", "TONE", "KEY_TONE", "UNIT", "SLEEP", "TRIP_RESET", "RESTORE"};
static struct
{
    char id[33], action[20];
    int value, code;
    unsigned used;
} replies[8];
static unsigned reply_next;

/** @brief 校验固定字段布局，不把 revision 写入设备配置。 */
static uint32_t checksum(const struct meter_preferences *p)
{
    return PREF_MAGIC ^ p->brightness ^ (p->music << 7) ^ (p->tone << 14) ^
           (p->key_tone << 21) ^ (p->unit << 22) ^ (p->sleep_minutes << 23);
}
static int valid(const struct meter_preferences *p)
{
    return p->brightness >= 10 && p->brightness <= 100 && p->music <= 100 && p->tone <= 100 &&
           p->key_tone <= 1 && p->unit <= 1 && (p->sleep_minutes == 0 || p->sleep_minutes == 1 || p->sleep_minutes == 3 || p->sleep_minutes == 5);
}
void meter_settings_snapshot(struct meter_preferences *out)
{
    if (!out)
        return;
    if (!ready)
    {
        *out = defaults;
        return;
    }
    os_sem_pend(&gate, 0);
    *out = preferences;
    os_sem_post(&gate);
    /* 蓝牙音乐或物理音量键也能调节通道；读取实际档位，避免设置页显示旧音量。 */
    int music_max = app_audio_volume_max_query(AppVol_BT_MUSIC);
    int tone_max = app_audio_volume_max_query(SysVol_TONE);
    int music = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    int tone = app_audio_get_volume(APP_AUDIO_STATE_WTONE);
    if (music_max > 0 && music >= 0 && music <= music_max && (out->music * music_max + 50) / 100 != music)
        out->music = (music * 100 + music_max / 2) / music_max;
    if (tone_max > 0 && tone >= 0 && tone <= tone_max && (out->tone * tone_max + 50) / 100 != tone)
        out->tone = (tone * 100 + tone_max / 2) / tone_max;

}
int meter_settings_miles(void)
{
    struct meter_preferences p;
    meter_settings_snapshot(&p);
    return p.unit;
}
int meter_settings_key_tone(void)
{
    struct meter_preferences p;
    meter_settings_snapshot(&p);
    return p.key_tone;
}
unsigned meter_settings_result(int *code)
{
    unsigned sequence;
    if (!ready)
        return 0;
    os_sem_pend(&gate, 0);
    sequence = result_sequence;
    if (code)
        * code = last_result;
    os_sem_post(&gate);
    return sequence;
}

/** @brief 音量按平台最大档位换算，保存值由本模块维护，避免百分比反复换算漂移。 */
static void apply_audio(const struct meter_preferences *p, int key)
{
    int music_max = app_audio_volume_max_query(AppVol_BT_MUSIC);
    int tone_max = app_audio_volume_max_query(SysVol_TONE);
    if (key == METER_MUSIC || key == METER_RESTORE)
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, (p->music * music_max + 50) / 100, 1);
    if (key == METER_TONE || key == METER_RESTORE)
        app_audio_set_volume(APP_AUDIO_STATE_WTONE, (p->tone * tone_max + 50) / 100, 1);
    app_audio_volume_save_now();
}
/** @brief 本机操作和 BLE 共用执行入口，先校验并落盘，再发布新状态。 */
static int apply(enum meter_setting_key key, int value)
{
    struct meter_preferences next;
    struct preference_record record;
    struct app_meter_state power;
    extern int ui_update_page_is_active(void);
    if (!ready || strcmp(os_current_task(), "app_core"))
        return ZXH_EXECUTION_FAILED;
    if (ui_update_page_is_active())
        return ZXH_BUSY;
    if (app_meter_control_snapshot(&power) != ZXH_OK || !power.power_on)
        return ZXH_POWER_OFF;
    meter_settings_snapshot(&next);
    if (value < 0)
        return ZXH_PARAMETER_ERROR;
    switch (key)
    {
    case METER_BRIGHTNESS:
        next.brightness = value;
        break;
    case METER_MUSIC:
        next.music = value;
        break;
    case METER_TONE:
        next.tone = value;
        break;
    case METER_KEY_TONE:
        next.key_tone = value;
        break;
    case METER_UNIT:
        next.unit = value;
        break;
    case METER_SLEEP:
        next.sleep_minutes = value;
        break;
    case METER_TRIP_RESET:
    {
        struct vehicle_state v;
        if (value)
            return ZXH_PARAMETER_ERROR;
        app_vehicle_snapshot(&v);
        if (v.motion == VEHICLE_MOVING)
            return ZXH_SAFETY_BLOCKED;
        int code = app_vehicle_reset_trip_now();
        return code == 0 ? ZXH_OK : ZXH_EXECUTION_FAILED;
    }
    case METER_RESTORE:
        if (value)
            return ZXH_PARAMETER_ERROR;
        next = defaults;
        break;
    default:
        return ZXH_PARAMETER_ERROR;
    }
    if (!valid(&next))
        return ZXH_PARAMETER_ERROR;
    struct meter_preferences before;
    meter_settings_snapshot(&before);
    if (before.brightness == next.brightness && before.music == next.music && before.tone == next.tone &&
            before.key_tone == next.key_tone && before.unit == next.unit && before.sleep_minutes == next.sleep_minutes)
        return ZXH_OK;
    record.magic = PREF_MAGIC;
    record.value = next;
    record.value.revision = 0;
    record.crc = checksum(&next);
    int written = syscfg_write(PREF_VM, &record, sizeof(record));
    int flushed = VM_ERR_NONE;
    /* 普通任务可能直接写 Flash；仅启用 RAM 缓存时刷新，空缓存不是写入失败。
     * 当前平台只缓存中断中的写入，app_core 保存后通常返回 VM_RAM_FLUSH_NONE。
     * 参数 0 保持后续写入能力，不得在设置保存时关闭 VM。
     */
    if (written == (int)sizeof(record) &&
            (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()))
        flushed = vm_flush2flash(0);
    if (written != (int)sizeof(record) || (flushed != VM_ERR_NONE && flushed != VM_RAM_FLUSH_NONE))
    {
        /* VM 写入缓存后仍可能落盘失败；恢复旧记录，避免别的模块稍后 flush 时提交失败操作。 */
        record.value = before;
        record.value.revision = 0;
        record.crc = checksum(&before);
        int restored = syscfg_write(PREF_VM, &record, sizeof(record));
        printf("[METER_SETTINGS] save key=%u write=%d flush=%d rollback=%d\n", (unsigned)key, written, flushed, restored);
        return ZXH_EXECUTION_FAILED;
    }
    os_sem_pend(&gate, 0);
    next.revision = preferences.revision + 1;
    preferences = next;
    os_sem_post(&gate);
    if (key == METER_MUSIC || key == METER_TONE || key == METER_RESTORE)
        apply_audio(&next, key);
    return ZXH_OK;
}
static void local_apply(int key, int value)
{
    int code = apply(key, value);
    os_sem_pend(&gate, 0);
    last_result = code;
    result_sequence++;
    os_sem_post(&gate);
}
int meter_settings_post(enum meter_setting_key key, int value)
{
    int argv[4] = {(int)local_apply, 2, key, value};
    if (!ready || (unsigned)key >= METER_SETTING_COUNT)
        return -1;
    return os_taskq_post_type("app_core", Q_CALLBACK, 4, argv) ? -1 : 0;
}
static const char *str(struct json_object *p, const char *key)
{
    struct json_object *v = json_object_object_get(p, key);
    return v && json_object_is_type(v, json_type_string) ? json_object_get_string(v) : "";
}
static void field(struct json_object *p, const char *key, const char *value)
{
    json_object_object_add(p, key, json_object_new_string(value));
}
static void number(struct json_object *p, const char *key, int value)
{
    char b[24];
    snprintf(b, sizeof(b), "%d", value);
    field(p, key, b);
}
/** @brief 64 位里程以十进制字符串传输，避免累计里程被 32 位截断。 */
static void distance_field(struct json_object *p, const char *key, uint64_t value)
{
    char buffer[21],*end = buffer + 20;
    *end = 0;
    do
    {
        *--end = (char)('0' + value % 10);
        value /= 10;
    }
    while (value);
    field(p, key, end);
}
static void reply(void *client, const char *id, const char *action, int code)
{
    struct meter_preferences p;
    struct vehicle_state v;
    struct json_object *root = json_object_new_object(),*data = json_object_new_object();
    meter_settings_snapshot(&p);
    app_vehicle_snapshot(&v);
    field(root, "topic", "ZXH_SETTINGS");
    field(root, "op", "NOTIFY");
    json_object_object_add(root, "errno", json_object_new_int(0));
    field(data, "v", "1");
    field(data, "id", id);
    field(data, "action", action);
    field(data, "phase", "result");
    number(data, "code", code);
    number(data, "confirmed", !code);
    if (!code)
    {
        number(data, "brightness", p.brightness);
        number(data, "music", p.music);
        number(data, "tone", p.tone);
        number(data, "key_tone", p.key_tone);
        number(data, "unit", p.unit);
        number(data, "sleep", p.sleep_minutes);
        number(data, "revision", p.revision);
        distance_field(data, "trip_m", v.values.trip_mm / 1000);
        distance_field(data, "odometer_m", v.values.odometer_mm / 1000);
        field(data, "name", "JL380N");
        field(data, "model", "ZXH-E02");
        field(data, "firmware", __DATE__ " " __TIME__);
    }
    json_object_object_add(root, "param", data);
    zxh_ble_send(client, json_object_to_json_string(root));
    json_object_put(root);
}
/** @brief 专用 BLE 设置协议；随机请求编号去重，断线重发也不会重复清零或写 Flash。 */
int meter_settings_receive(void *client, const char *topic, const char *json)
{
    struct json_object *root,*p;
    const char *id,*action,*text;
    int key = -1, value = 0, code = ZXH_PARAMETER_ERROR;
    unsigned i;
    if (strcmp(topic, "ZXH_SETTINGS"))
        return 0;
    if (!zxh_ble_client_active(client))
        return 1;
    root = json_tokener_parse(json);
    p = root ? json_object_object_get(root, "param") : NULL;
    if (!p || !json_object_is_type(p, json_type_object))
    {
        if (root)
            json_object_put(root);
        return 1;
    }
    id = str(p, "id");
    action = str(p, "action");
    text = str(p, "value");
    if (strlen(id) != 32 || strspn(id, "0123456789abcdef") != 32)
    {
        id = "";
        action = "";
        goto done;
    }
    if (strlen(action) > 19)
    {
        action = "";
        goto done;
    }
    if (strcmp(str(p, "v"), "1"))
    {
        code = ZXH_VERSION_ERROR;
        goto done;
    }
    if (strcmp(str(root, "op"), "PUT"))
        goto done;
    if (!strcmp(action, "GET"))
    {
        code = ZXH_OK;
        goto done;
    }
    for (i = 0; i < METER_SETTING_COUNT; i++)
        if (!strcmp(action, keys[i]))
            key = i;
    if (key < 0)
    {
        code = ZXH_ACTION_ERROR;
        goto done;
    }
    if (!*text || strlen(text) > 3 || strspn(text, "0123456789") != strlen(text))
        goto done;
    value = atoi(text);
    for (i = 0; i < 8; i++)
        if (replies[i].used && !strcmp(replies[i].id, id))
        {
            code = !strcmp(replies[i].action, action) && replies[i].value == value ? replies[i].code : ZXH_ID_CONFLICT;
            goto done;
        }
    code = apply(key, value);
    i = reply_next++ % 8;
    replies[i].used = 1;
    strcpy(replies[i].id, id);
    strcpy(replies[i].action, action);
    replies[i].value = value;
    replies[i].code = code;
done:
    reply(client, id, action, code);
    json_object_put(root);
    return 1;
}
int meter_settings_init(void)
{
    struct preference_record record;
    if (ready)
        return 0;
    if (strcmp(os_current_task(), "app_core"))
        return -1;
    if (os_sem_create(&gate, 1))
        return -1;
    if (syscfg_read(PREF_VM, &record, sizeof(record)) == (int)sizeof(record) && record.magic == PREF_MAGIC &&
            valid(&record.value) && record.crc == checksum(&record.value))
        preferences = record.value;
    preferences.revision = 1;
    ready = 1;
    apply_audio(&preferences, METER_RESTORE);
    return 0;
}

