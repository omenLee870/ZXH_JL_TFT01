#include "app_config.h"
#include "sport_info_opt.h"
#include "utils/syscfg_id.h"
#include "health_manager/shm_info_storage.h"
#include "health_manager.h"

#define LOG_TAG_CONST       SPORT_HEALTH_MANAGE
#define LOG_TAG     		"[SHM-INFO-STORAGE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".shm_info_storage.data.bss")
#pragma data_seg(".shm_info_storage.data")
#pragma const_seg(".shm_info_storage.text.const")
#pragma code_seg(".shm_info_storage.text")
#endif

/* 抬腕亮屏出厂默认（VM 无开关记录时生效）：1 = 默认开启，避免老机器升级后功能静默消失；
 * 0 = 默认关闭。设置过开关后以 VM 记录为准，本宏不再参与。 */
#define RAISE_WRIST_DEFAULT_ON      1

static u32 g_sport_info_switch_record = 0;
static u8 g_sport_info_mode_record[SPORT_INFO_MODE_TYPE_MAX + 1] = {0};
// 读vm
int sport_info_write_vm(int vm_id, u8 *data, u16 data_len)
{
    u8 *tmp_var = zalloc(data_len);
    if (NULL == tmp_var) {
        goto __sport_info_write_vm_end;
    }
    syscfg_read(vm_id, tmp_var, data_len);
    if (0 != memcmp(tmp_var, data, data_len)) {
        syscfg_write(vm_id, data, data_len);
    }

__sport_info_write_vm_end:
    if (tmp_var) {
        free(tmp_var);
    }
    return 0;
}

// 写vm
int sport_info_read_vm(int vm_id, u8 *data, u16 data_len)
{
    int ret = 0;
    u8 *tmp_var = zalloc(data_len);
    if (NULL == tmp_var) {
        log_error("no enough ram\n");
        goto __sport_info_read_vm_end;
    }
    if (data_len == syscfg_read(vm_id, tmp_var, data_len)) {
        memcpy(data, tmp_var, data_len);
        ret = data_len;
    }

__sport_info_read_vm_end:
    if (tmp_var) {
        free(tmp_var);
    }
    return ret;
}

// 更新开关
void sport_info_switch_record_update(u8 switch_type, u8 switch_state, u8 write_vm)
{
    if (switch_state) {
        g_sport_info_switch_record |= BIT(switch_type);
    } else {
        g_sport_info_switch_record &= ~BIT(switch_type);
    }
    if (write_vm) {
        sport_info_write_vm(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&g_sport_info_switch_record, sizeof(g_sport_info_switch_record));
    }
}

// 获取开关信息
u32 sport_info_swtich_record_get(u8 switch_type)
{
    u32 sport_info_switch_record = 0;
    if (sport_info_read_vm(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&g_sport_info_switch_record, sizeof(g_sport_info_switch_record))) {
        sport_info_switch_record = g_sport_info_switch_record;
    }
    if (switch_type < SPORT_INFO_SWTICH_TYPE_MAX) {
        return (sport_info_switch_record & BIT(switch_type));
    }
    return sport_info_switch_record;
}

/* 强制落盘开关位域，跳过 sport_info_write_vm 的「值未变」判断：VM 从未写过时读回全 0，
 * 「关掉抬腕开关（位域 = 0）」会被判为未变而丢弃，导致记录建不起来、开关关不掉。 */
static void sport_info_switch_record_force_write(void)
{
    syscfg_write(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&g_sport_info_switch_record,
                 sizeof(g_sport_info_switch_record));
}

// 更新模式
void sport_info_mode_record_update(u8 mode_type, u8 mode)
{
    g_sport_info_mode_record[0] = sizeof(g_sport_info_mode_record) - 1;
    g_sport_info_mode_record[mode_type + 1] = mode;
    sport_info_write_vm(VM_SPORT_INFO_MODE_FLAG, g_sport_info_mode_record, sizeof(g_sport_info_mode_record));
}

// 获取模式信息
u16 sport_info_record_get(u8 mode_type, u8 *mode_data[])
{
    u16 data_len = sport_info_read_vm(VM_SPORT_INFO_MODE_FLAG, g_sport_info_mode_record, sizeof(g_sport_info_mode_record));
    *mode_data = g_sport_info_mode_record;
    if (mode_type < SPORT_INFO_MODE_TYPE_MAX) {
        *mode_data += mode_type + 1;
        data_len = 1;
    }
    return data_len;
}

int sport_exercise_heart_rate_get(e_heart_rate *heart_rate)
{
    return sport_info_read_vm(VM_SPORT_INFO_EXERCISE_HEART_RATE, (u8 *)heart_rate, sizeof(e_heart_rate));
}


int sport_fall_detection_get(fall_detection_t *fall_detect)
{
    return sport_info_read_vm(VM_SPORT_INFO_FALL_DETECTION, (u8 *)fall_detect, sizeof(fall_detection_t));

}

int sport_personal_info_get(personal_information *info)
{
    return sport_info_read_vm(VM_SPORT_INFO_PERSONAL_INFO_FLAG, (u8 *)info, sizeof(personal_information));
}

int sport_raise_wrist_get(raise_wrist_t *raise_wrist)
{
    return sport_info_read_vm(VM_SPORT_INFO_RAISE_WRIST, (u8 *)raise_wrist, sizeof(raise_wrist_t));
}


int sport_sedentary_get(sedentary_t *sedentary)
{
    return sport_info_read_vm(VM_SPORT_INFO_SEDENTARY, (u8 *)sedentary, sizeof(sedentary_t));
}

int sport_sleep_detection_get(sleep_detection_t *sleep_detection)
{
    return sport_info_read_vm(VM_SPORT_INFO_SLEEP_DETECTION, (u8 *)sleep_detection, sizeof(sleep_detection_t));
}

int sport_info_sedentary_switch_get(void)
{
    return sport_info_swtich_record_get(SPORT_INFO_SWTICH_TYPE_SEDENTARY) ? 1 : 0;
}

int sport_info_sedentary_switch_set(u8 on)
{
    sport_info_switch_record_update(SPORT_INFO_SWTICH_TYPE_SEDENTARY, on ? 1 : 0, 1);
    return 0;
}

int sport_info_raise_wrist_switch_get(void)
{
    return sport_info_swtich_record_get(SPORT_INFO_SWTICH_TYPE_RAISE_WRIST) ? 1 : 0;
}

int sport_info_raise_wrist_enabled(void)
{
    u32 rec = 0;
    /* VM 里没有开关记录 = 用户从未设置过（首次开机），按出厂默认处理；
     * 有记录则严格跟随，全天开启位或定时时段位任一置位即为开启。 */
    if (sport_info_read_vm(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&rec, sizeof(rec)) <= 0) {
        return RAISE_WRIST_DEFAULT_ON;
    }
    return ((rec & BIT(SPORT_INFO_SWTICH_TYPE_RAISE_WRIST))
            || (rec & BIT(SPORT_INFO_SWTICH_TYPE_RAISE_WRIST_CUSTOMIZE))) ? 1 : 0;
}

int sport_info_raise_wrist_switch_apply(void)
{
    int on = sport_info_raise_wrist_enabled();
    /* need_pend = 0：异步投递，不阻塞 UI / RCSP 回调线程；
     * 使能/关闭在 health_manager 任务上下文由 wrist_tilt_io_crtl 完成。 */
    int ret = sport_health_manager_msg_post(SHM_MOD_DET_WRIST,
                                            on ? SHM_CMD_ENABLE : SHM_CMD_DISABLE, NULL, 0);
    log_info("%s on:%d ret:%d", __func__, on, ret);
    return ret;
}

int sport_info_raise_wrist_switch_set(u8 on)
{
    u32 rec = 0;
    /* 先探一次 VM：有记录 = 走常规「变了才写」路径；无记录 = 必须强制落盘，
     * 否则下面把位域清成 0 会被 write_vm 的 memcmp 判成「未变」而丢弃。 */
    int has_record = (sport_info_read_vm(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&rec, sizeof(rec)) > 0);

    if (on) {
        // 开 = 全天开启
        sport_info_switch_record_update(SPORT_INFO_SWTICH_TYPE_RAISE_WRIST, 1, 0);
    } else {
        // 关 = 清除全天/定时两个位
        sport_info_switch_record_update(SPORT_INFO_SWTICH_TYPE_RAISE_WRIST, 0, 0);
        sport_info_switch_record_update(SPORT_INFO_SWTICH_TYPE_RAISE_WRIST_CUSTOMIZE, 0, 0);
    }
    if (has_record) {
        sport_info_write_vm(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&g_sport_info_switch_record,
                            sizeof(g_sport_info_switch_record));
    } else {
        sport_info_switch_record_force_write();
    }
    /* 写 VM 后立刻把新状态下发到检测链路，否则开关只改记录、检测行为不变 */
    sport_info_raise_wrist_switch_apply();
    return 0;
}

int sport_info_raise_wrist_switch_migrate(u8 legacy_on)
{
    u32 rec = 0;
    /* 有记录 = 用户或 App 设置过，权威源已存在，不能覆盖 */
    if (sport_info_read_vm(VM_SPORT_INFO_SWITCH_FLAG, (u8 *)&rec, sizeof(rec)) > 0) {
        log_debug("%s skip, record exist:0x%x", __func__, rec);
        return 0;
    }
    /* 无记录（首次开机 / 老机器升级）：把 UI 侧旧开关迁过来做初值。
     * switch_set 内部会写 VM 并 apply，无需在此重复下发。 */
    log_info("%s no record, migrate legacy:%d", __func__, legacy_on);
    sport_info_raise_wrist_switch_set(legacy_on ? 1 : 0);
    return 1;
}


