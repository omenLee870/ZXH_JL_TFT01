#include "tzflash_api.h"
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".update.data.bss")
#pragma data_seg(".update.data")
#pragma const_seg(".update.text.const")
#pragma code_seg(".update.text")
#endif
#include "update.h"
#include "update_loader_download.h"
#include "cpu/includes.h"
#include "asm/wdt.h"
#include "os/os_api.h"
#include "os/os_cpu.h"
#include "app_config.h"
#include "cpu.h"
#include "syscfg_id.h"
#include "btcontroller_modules.h"
#include "system/includes.h"
#include "uart_update.h"
#include "dual_bank_updata_api.h"
#include "btstack/avctp_user.h"
#include "poweroff.h"
#include "app_main.h"
#include "vm.h"
#include "bt_common.h"
#include "smart_voice/smart_voice.h"

#if TCFG_UPDATE_ENABLE

#if TCFG_MIC_EFFECT_ENABLE
#include "mic_effect.h"
#endif

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif

#if TCFG_UI_ENABLE
#include "ui/ui_api.h"
#endif

#ifdef UPDATE_VOICE_REMIND
#include "app_tone.h"
#include "audio_config.h"
#endif

#include "custom_cfg.h"

#define LOG_TAG "[APP-UPDATE]"
#define LOG_INFO_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"

#if (JL_RCSP_EXTRA_FLASH_OPT)
#include "rcsp_extra_flash_opt.h"
#endif

#define LOADER_NAME		"LOADER.BIN"
#define DEVICE_UPDATE_KEY_ERR  BIT(30)
#define DEVICE_FIRST_START     BIT(31)

extern void update_module_init(void (*cbk)(update_mode_info_t *, u32, void *));
extern void testbox_update_init(void);
extern void ll_hci_destory(void);
extern void hci_controller_destory(void);
extern const int support_norflash_update_en;
extern void ram_protect_close(void);
extern void hwi_all_close(void);
extern void wifi_det_close();

__attribute__((weak))
void wifi_det_close()
{
    printf("tmp weak func wifi_det_close\n");
}
extern void *dev_update_get_parm(int type);
extern u8 get_ota_status();
extern int get_nor_update_param(void *buf);
extern bool get_tws_phone_connect_state(void);
extern void tws_sniff_controle_check_disable(void);
extern void sys_auto_sniff_controle(u8 enable, u8 *addr);
extern void app_audio_set_wt_volume(s16 volume);
extern u8 get_max_sys_vol(void);
extern u8 get_bt_trim_info_for_update(u8 *res);
extern u32 get_btosc_info_for_update(void *info);


extern const int support_update_backup_btif_addr;
extern const int support_norflash_update_en;
extern const int support_user_file_update_v2_en;
extern const int support_reusable_special_update;

extern void user_file_flash_file_download_init(void);
extern void ex_flash_file_download_init(void);
extern int reusable_loader_region(loader_area_t *loader_area);
extern int res_sdfile_update_loader_region(loader_area_t *loader_area);

const u8 loader_file_path[] = "mnt/norflash/C/"LOADER_NAME"";
//升级文件路径必须是短文件名（8+3）结构，仅支持２层目录
/* const char updata_file_name[] = "/UPDATA/JL_692X.BFU"; */
const char updata_file_name[] = "/*.UFW";
static u32 g_updata_flag = 0;
static volatile u8 ota_status = 0;
static succ_report_t succ_report;
static bool g_write_vm_flag = true;
static u32 g_cur_update_type = 0;
static u8 g_ota_reset_tout = 0;

#include "system/boot.h"
#include "tzflash_api.h"
int norflash_set_write_protect(u32 enable) //temp, todo...
{
    struct vm_info vm = {0};
    int ret = 0;

    tzflash_ioctl(NULL, IOCTL_GET_VM_INFO, (u32)&vm);
    ret = tzflash_set_write_protect(enable, 0, vm.vm_saddr);
    return ret;//<0:error,0:ok
}

bool is_testbox_ble_update_type(void)
{
    return (BLE_TEST_UPDATA == g_cur_update_type);
}

int syscfg_write_update_check(u16 item_id, void *buf, u16 len)
{
    return g_write_vm_flag;
}

bool vm_need_recover(void)
{
    if (support_update_backup_btif_addr) {
        // 在这里面进行btif的还原，不然在vm初始哈后还原，对于的flash内容会被擦掉
        after_update_btif_resume(g_updata_flag & 0xffff);
    }
    return (((g_updata_flag & 0xffff) == UPDATA_SUCC) ? true : false
            || ((g_updata_flag & 0xffff) == UPDATA_DEV_ERR) ? true : false);
}

#if TCFG_FLASH_ERASE_WAIT_BLE
extern void ble_busy_wait_idle(u8 type);
int before_erase_in_update_lib(void *priv)
{
#if TCFG_APP_BT_EN
    ble_busy_wait_idle(IOCTL_ERASE_PAGE);
#endif
    return 0;
}
#endif

u16 update_result_get(void)
{
    u16 ret = UPDATA_NON;

    if (CONFIG_UPDATE_ENABLE) {
        UPDATA_PARM *p = UPDATA_FLAG_ADDR;
        u16 crc_cal;
        crc_cal = CRC16(((u8 *)p) + 2, sizeof(UPDATA_PARM) - 2);	//2 : crc_val
        if (crc_cal && crc_cal == p->parm_crc) {
            ret =  p->parm_result;
        }
        g_updata_flag = ret;
        g_updata_flag |= ((u32)(p->magic)) << 16;

        memset(p, 0x00, sizeof(UPDATA_PARM));
    }

    return ret;
}

void update_result_set(u16 result)
{
    if (CONFIG_UPDATE_ENABLE) {
        UPDATA_PARM *p = UPDATA_FLAG_ADDR;

        memset(p, 0x00, sizeof(UPDATA_PARM));
        p->parm_result = result;
        p->parm_crc = CRC16(((u8 *)p) + 2, sizeof(UPDATA_PARM) - 2);
    }
#if (RCSP_UPDATE_EN && RCSP_MODE && JL_RCSP_EXTRA_FLASH_OPT)
    if (UPDATA_SUCC == result) {
        rcsp_eflash_update_flag_set(0);
        rcsp_eflash_flag_set(0);
        extern void set_update_ex_flash_flag(u8 update_flag);
        set_update_ex_flash_flag(0);
    }
#endif
}
void update_clear_result()
{
    g_updata_flag = 0;
}

bool update_success_boot_check(void)
{
    if (CONFIG_UPDATE_ENABLE) {
        u16 result = g_updata_flag & 0xffff;
        u16 up_tpye = g_updata_flag >> 16;
        /* if ((UPDATA_SUCC == result) && ((SD0_UPDATA == up_tpye) || (SD1_UPDATA == up_tpye) || (USB_UPDATA == up_tpye))) { */
        if (UPDATA_SUCC == result) {
            if (support_dual_bank_update_en) {
                return true;
            } else {
                switch (up_tpye) {
                case SD0_UPDATA:
                case SD1_UPDATA:
                case USB_UPDATA:
                case UART_UPDATA:
                case BT_UPDATA:
                case BLE_APP_UPDATA:
                case BLE_TEST_UPDATA:
                case DUAL_BANK_UPDATA:
                    return true;
                }
            }
        }
    }
    return false;
}

bool device_is_first_start()
{
    log_info("g_updata_flag=0x%x\n", g_updata_flag);
    if ((g_updata_flag & DEVICE_FIRST_START) || (g_updata_flag & DEVICE_UPDATE_KEY_ERR) || (g_updata_flag == UPDATA_SUCC)) {
        puts("\n=================device_is_first_start=========================\n");
        return true;
    }
    return false;
}

void led_update_start(void)
{
#ifdef UPDATE_LED_REMIND
    puts("led_update_start\n");
    /* pwm_led_mode_set(PWM_LED_ALL_OFF); */
#endif
}

void led_update_finish(void)
{
#ifdef UPDATE_LED_REMIND
    puts("led_update_finish\n");
    /* pwm_led_mode_set(PWM_LED0_LED1_FAST_FLASH); */
#endif
}

static inline void dev_update_close_ui()
{

#if (TCFG_UI_ENABLE)

    printf("[%s] FIXME warn:TODO dev update close ui ", __func__);
    /*     u8 count = 0; */
    /*     UI_SHOW_WINDOW(ID_WINDOW_POWER_OFF); */
    /* __retry: */
    /*     if (UI_GET_WINDOW_ID() != ID_WINDOW_POWER_OFF) { */
    /*         os_time_dly(10);//增加延时防止没有关显示 */
    /*         count++; */
    /*         if (count < 3) { */
    /*             goto __retry; */
    /*         } */
    /*     } */
#endif
}

extern void update_tone_event_clear();

int update_result_deal()
{
#ifdef CONFIG_FPGA_ENABLE
    return 0;
#endif

    u8 key_voice_cnt = 0;
    u16 result = 0;
    result = (g_updata_flag & 0xffff);
    log_info("<--------update_result_deal=0x%x %x--------->\n", result, g_updata_flag >> 16);
#if CONFIG_DEBUG_ENABLE
#if TCFG_APP_BT_EN
    u8 check_update_param_len(void);
    ASSERT(check_update_param_len(), "UPDATE_PARAM_LEN ERROR");
#endif
#endif
    if (result == UPDATA_NON || 0 == result) {
        return 0;
    }

#if (CONFIG_REUSABLE_RESERVE)
    if (support_reusable_special_update) {
        extern bool reusable_update_result_deal(bool result);
        if (!reusable_update_result_deal(result == UPDATA_SUCC)) {
            return 0;
        }
    }
#elif (CONFIG_RESFS_UPDATE_ENABLE)
    if (support_reusable_special_update) {
        extern bool resfs_update_result_deal(bool result);
        if (!resfs_update_result_deal(result == UPDATA_SUCC)) {
            return 0;
        }
    }
#endif

#ifdef UPDATE_VOICE_REMIND
#endif
    if (result == UPDATA_SUCC) {
#if((RCSP_MODE == RCSP_MODE_EARPHONE) && RCSP_UPDATE_EN)
        u8 clear_update_flag = 0;
        syscfg_write(VM_UPDATE_FLAG, &clear_update_flag, 1);
#endif
#ifdef UPDATE_LED_REMIND
        led_update_finish();
#endif
#if(RCSP_UPDATE_EN && RCSP_MODE && JL_RCSP_EXTRA_FLASH_OPT)
        rcsp_eflash_update_flag_set(0);
        rcsp_eflash_flag_set(0);
        extern void set_update_ex_flash_flag(u8 update_flag);
        set_update_ex_flash_flag(0);
#endif
    }

    int voice_max_cnt = 5;

    while (1) {
        wdt_clear();

        key_voice_cnt++;
#ifdef UPDATE_VOICE_REMIND
        int msg[8];
        if (result == UPDATA_SUCC) {
            puts("<<<<<<UPDATA_SUCC");
            /* app_audio_set_volume(APP_AUDIO_STATE_WTONE, get_max_sys_vol() / 2, 1); */
            play_tone_file(get_tone_files()->normal);
            for (int i = 0; i < 5; i++) {
                os_time_dly(5);
                os_taskq_accept(8, msg);
            }
            tone_player_stop();
            puts(">>>>>>>>>>>\n");
            update_tone_event_clear();
        } else {
            voice_max_cnt = 20; //区分下升级失败提示音
            log_info("!!!!!!!!!!!!!!!updata waring !!!!!!!!!!!=0x%x\n", result);
            /* app_audio_set_volume(APP_AUDIO_STATE_WTONE, get_max_sys_vol(), 1); */
            play_tone_file(get_tone_files()->normal);
            for (int i = 0; i < 5; i++) {
                os_time_dly(2);
                os_taskq_accept(8, msg);
            }
            tone_player_stop();
            update_tone_event_clear();
        }
#endif
        if (key_voice_cnt > voice_max_cnt) {
            puts("enter_sys_soft_poweroff\n");
            break;
            //注:关机要慎重,要设置开机键
            //enter_sys_soft_poweroff();
        }
    }

    return 1;
}

void clr_update_ram_info(void)
{
    UPDATA_PARM *p = UPDATA_FLAG_ADDR;
    memset(p, 0x00, sizeof(UPDATA_PARM));
}

void update_close_hw(void *filter_name)
{
    const struct update_target *p;
    list_for_each_update_target(p) {
        if (memcmp(filter_name, p->name, strlen(filter_name)) != 0) {
            printf("close Hw Name : %s\n", p->name);
            p->driver_close();
        }
    }
}

static void update_before_jump_common_handle(UPDATA_TYPE up_type)
{

#if TCFG_AUDIO_ANC_ENABLE
    extern void audio_anc_hw_close();
    audio_anc_hw_close();
#endif

#if (CPU_CORE_NUM == 1)         //双核在跳转前关中断lock_set后会和maskrom 初始化中断冲突导致ilock err
    local_irq_disable();
#endif

    hwi_all_close();

#ifdef CONFIG_SUPPORT_WIFI_DETECT
    wifi_det_close();
#endif
    /*跳转的时候遇到死掉的情况很可能是硬件模块没关导致，加上保护可以判断哪个异常，保护的地址根据不同SDK而定*/
    /* u8 inv = 0; */
    /* mpu_set(1, (u32)&test_pro_addr, (u32)test_pro_addr, inv, "0r", DBG_FM); */

}

//ota.bin 放到exflash升级的方式，parm_priv存放了norflash的参数，对应实际升级方式的参数需要放在norflash参数之后
void update_param_priv_fill(UPDATA_PARM *p, void *priv, u16 priv_len)
{
    int parm_offset = 0;
    if (support_norflash_update_en) {
        parm_offset = get_nor_update_param(p->parm_priv);          //如果loader放在外挂norflash,parm_priv前面放norflash参数，后面才是升级类型本身的参数
    }
    memcpy(p->parm_priv + parm_offset, priv, priv_len);
}

void update_param_ext_fill(UPDATA_PARM *p, u8 ext_type, u8 *ext_data, u8 ext_len)
{
    struct ext_arg_t ext_arg;

    ext_arg.type = ext_type;
    ext_arg.len  = ext_len;
    ext_arg.data = ext_data;

    ASSERT(((u32)p + sizeof(UPDATA_PARM) + p->ext_arg_len + ext_len + 2 - (u32)p) <= UPDATA_PARM_SIZE, "update param overflow!\n");
    memcpy((u8 *)p + sizeof(UPDATA_PARM) + p->ext_arg_len, &ext_arg, 2);          //2byte:type + len
    memcpy((u8 *)p + sizeof(UPDATA_PARM) + p->ext_arg_len + 2, ext_arg.data, ext_arg.len);
    log_info("ext_fill :");
    log_info_hexdump((u8 *)p + sizeof(UPDATA_PARM) + p->ext_arg_len, ext_arg.len + 2);
    p->ext_arg_len += (2 + ext_arg.len);
    p->ext_arg_crc = CRC16((u8 *)p + sizeof(UPDATA_PARM), p->ext_arg_len);
}

extern int user_api_bin_special_handle_info_get(u32 *start_addr);
static void user_api_bin_special_handle_info_fill(UPDATA_PARM *p)
{
    u32 start_addr = 0;
    if (user_api_bin_special_handle_info_get(&start_addr)) {
        update_param_ext_fill(p, EXT_USER_API_BIN_INFO, (u8 *)&start_addr, sizeof(start_addr));
    }
}

//fill common content \ private content \ crc16
static void update_param_content_fill(int type, UPDATA_PARM *p, void (*priv_param_fill_hdl)(UPDATA_PARM *P))
{
    u8 ext_len = 0;
    u8 *ext_data = NULL;

    memset((u8 *)p, 0x00, sizeof(UPDATA_PARM));

    if (support_norflash_update_en) {
        p->parm_type = NORFLASH_UPDATA;                                //uboot通过该标识从外挂flash读取ota.bin
        *((u16 *)((u8 *)p + sizeof(UPDATA_PARM) + 32)) = (u16)type;    //将实际的升级类型保存到UPDATA_PARM后
    } else {
        p->parm_type = (u16)type;
    }

    p->parm_result = (u16)UPDATA_READY;
    p->magic = UPDATE_PARAM_MAGIC;
    p->ota_addr = succ_report.loader_saddr;
    if (support_update_backup_btif_addr && succ_report.priv_param) {
        // 更新ext_arg_len，更新对应内容
        p->ext_arg_len += update_res_param_opt(succ_report.priv_param, 0, (u8 *)p + sizeof(UPDATA_PARM), 0);
    }

    //支持loader放到外挂flash里ota_addr为0
    if (0 == p->ota_addr && !support_norflash_update_en) {
        log_error("ota addr err\n");
        return;
    }

    if (priv_param_fill_hdl) {
        priv_param_fill_hdl(p);
    }

#if (defined CONFIG_CPU_BR36 || defined CONFIG_CPU_BR27 || defined CONFIG_CPU_BR50 || defined CONFIG_WATCH_CASE_ENABLE)
#if (TCFG_USER_BT_CLASSIC_ENABLE || TCFG_USER_BLE_ENABLE)
    if (type == BT_UPDATA || type == BLE_APP_UPDATA || type == SPP_APP_UPDATA || type == BLE_TEST_UPDATA) {     //D版芯片蓝牙相关的升级需要保存LDO_TRIM_RES
        ext_data = malloc(256);
        if (ext_data != NULL) {
            ext_len = get_bt_trim_info_for_update(ext_data);
            log_info("trim_len:%d\n", ext_len);
            update_param_ext_fill(p, EXT_LDO_TRIM_RES, ext_data, ext_len);

            ext_len = get_btosc_info_for_update(ext_data);
            if (ext_len > 0) {
                log_info("btosc_len:%d\n", ext_len);
                log_info_hexdump(ext_data, ext_len);
                update_param_ext_fill(p, EXT_BT_WLA_INFO, ext_data, ext_len);
            }
            free(ext_data);
        }
        update_param_ext_fill(p, EXT_BT_MAC_ADDR, (u8 *)bt_get_mac_addr(), 6);
    }
#endif
#endif
    u8 ext_flag = 0;
    ext_len = 1;
#if CONFIG_UPDATE_JUMP_TO_MASK
    ext_flag = 1;
#endif
    update_param_ext_fill(p, EXT_JUMP_FLAG, &ext_flag, ext_len);
    /* u8 *flag = update_param_ext_get(p, EXT_JUMP_FLAG); */
    /* r_printf(">>>[test]:flag = %d\n", flag[0]); */

#if USER_FILE_UPDATE_V2_EN
    extern u8 reserve_file_flag_get(void);
    ext_flag = reserve_file_flag_get();
    update_param_ext_fill(p, EXT_RESERVED_UPDATE, &ext_flag, ext_len);

#if (CONFIG_REUSABLE_RESERVE)
    if (support_reusable_special_update) {
        u8 new_sdk_update_again_flag_get(void);
        ext_flag = new_sdk_update_again_flag_get();
        if (ext_flag) {
            update_param_ext_fill(p, EXT_NEW_SDK_UPD_AGAIN, &ext_flag, ext_len);
        }
    }
#elif (CONFIG_RESFS_UPDATE_ENABLE)
    if (support_reusable_special_update) {
        u8 resfs_new_sdk_update_again_flag_get(void);
        ext_flag = resfs_new_sdk_update_again_flag_get();
        if (ext_flag) {
            update_param_ext_fill(p, EXT_NEW_SDK_UPD_AGAIN, &ext_flag, ext_len);
        }
    }
#endif
#endif

    if (g_ota_reset_tout) {
        update_param_ext_fill(p, EXT_RESET_TIME_WITHOUT_CONN, &g_ota_reset_tout, sizeof(g_ota_reset_tout));
    }
    user_api_bin_special_handle_info_fill(p);

    p->parm_crc = CRC16(((u8 *)p) + 2, sizeof(UPDATA_PARM) - 2);	//2 : crc_val
}

static void update_param_ram_set(u8 *buf, u16 len)
{
    u8 *update_ram = UPDATA_FLAG_ADDR;
    memcpy(update_ram, (u8 *)buf, len);
}

__attribute__((weak))
void update_process_before_cpu_reset(void)
{

}

void update_mode_api_v2(UPDATA_TYPE type, void (*priv_param_fill_hdl)(UPDATA_PARM *p), void (*priv_update_jump_handle)(int type))
{
    u16 update_param_len = UPDATA_PARM_SIZE;//sizeof(UPDATA_PARM) + UPDATE_PRIV_PARAM_LEN;

    UPDATA_PARM *p = malloc(update_param_len);

    if (p) {
        update_param_content_fill(type, p, priv_param_fill_hdl);

        if (succ_report.update_param_write_hdl) {
            succ_report.update_param_write_hdl(succ_report.priv_param, (u8 *)p, update_param_len);
        }

#ifdef UPDATE_LED_REMIND
        led_update_start();
#endif

        update_param_ram_set((u8 *)p, update_param_len);

#if TCFG_MIC_EFFECT_ENABLE
        if (mic_effect_player_runing()) {
            mic_effect_player_close();
        }
#endif

        if (type == BT_UPDATA) {
#if TCFG_BT_BACKGROUND_ENABLE
            if (!app_in_mode(APP_MODE_BT)) {
                //这里需要切到蓝牙模式再切时钟，如果再Music等模式需要较高主频才能跑得过来的任务，切换时钟会导致系统跑不过来导致异常
                g_bt_hdl.background.backmode = BACKGROUND_GOBACK_WITH_OTHER;
                app_send_message(APP_MSG_GOTO_MODE, APP_MODE_BT);
                while (!app_in_mode(APP_MODE_BT)) {
                    os_time_dly(1);
                }
            }
#endif
            clk_set_api("sys", 24000000);       //测试盒跳转升级需要强制设置时钟为24M
        }


#if CPU_CORE_NUM > 1            //双核需要把CPU1关掉
        printf("Before Suspend Current Cpu ID:%d Cpu In Irq?:%d\n", current_cpu_id(),  cpu_in_irq());
        if (current_cpu_id() == 1) {
            os_suspend_other_core();
        }
        ASSERT(current_cpu_id() == 0);          //确保跳转前CPU1已经停止运行
        cpu_suspend_other_core(CPU_SUSPEND_TYPE_UPDATE);
        printf("After Suspend Current Cpu ID:%d\n", current_cpu_id());
#endif
        update_before_jump_common_handle(type);

        if (priv_update_jump_handle) {
            priv_update_jump_handle(type);
        }
        free(p);
    } else {
        ASSERT(p, "malloc update param err \n");
    }
}

int update_check_sniff_en(void)

{
    if (CONFIG_UPDATE_ENABLE) {
        if (get_ota_status()) {
            log_info("ota ing...");
            return 0;
        } else {
            return 1;
        }
    }
    return 1;
}


void set_ota_status(u8 stu)
{
    ota_status = stu;
}

u8 get_ota_status()
{
    return ota_status;
}

static u8 ota_idle_query(void)
{
    return !ota_status;
}

//防止升级过程进入powerdown
REGISTER_LP_TARGET(ota_lp_target) = {
    .name = "ota",
    .is_idle = ota_idle_query,
};

extern void tws_sync_update_api_register(const update_op_tws_api_t *op);
extern update_op_tws_api_t *get_tws_update_api(void);

extern const int support_dual_bank_update_en;
extern int tws_ota_init(void);
extern void tws_api_auto_role_switch_disable();
extern void tws_api_auto_role_switch_enable();

static void update_init_common_handle(int type)
{
    ota_status = 1;
    g_cur_update_type = type;
    g_ota_reset_tout = 0;
#if TCFG_SMART_VOICE_ENABLE && !TCFG_AUDIO_ASR_DEVELOP
    //OTA开启后，关闭离线语音识别，再次开启留给OTA完后芯片复位时候，随着本身初始化开启
    audio_smart_voice_detect_close();
#endif
#if FLASH_PROTECT_OPT_BEFORE_UPDATE
    // 兼容双备份，升级开始就解除flash保护
    printf("%s, remove_flash_protect\n", __func__);
    norflash_set_write_protect(0);
#endif
    if (UPDATE_DUAL_BANK_IS_SUPPORT()) {
#if TCFG_AUTO_SHUT_DOWN_TIME
        sys_auto_shut_down_disable();
#endif

#if OTA_TWS_SAME_TIME_ENABLE
        tws_api_auto_role_switch_disable();
        tws_sync_update_api_register(get_tws_update_api());
        tws_ota_init();
#endif
    }
}

static void update_exit_common_handle(int type, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;
    if (UPDATE_RESULT_ERR_NONE != ret_code->err_code && (UPDATE_RESULT_FLAG_BITMAP | UPDATE_RESULT_ERR_NONE) != ret_code->err_code \
        && UPDATE_RESULT_BT_UPDATE_OVER != ret_code->err_code) {
#if FLASH_PROTECT_OPT_BEFORE_UPDATE
        // 兼容双备份，升级错误就添加flash保护
        printf("%s, %x, add_flash_protect\n", __func__, ret_code->err_code);
        norflash_set_write_protect(1);
#endif
        if (!UPDATE_DUAL_BANK_IS_SUPPORT()) {
            g_write_vm_flag = true;
            if (support_update_backup_btif_addr) {
                // 还原btif信息
                after_update_btif_resume(0);
            }
#if (CONFIG_REUSABLE_RESERVE)
            if (support_reusable_special_update) {
                extern void reusable_update_flag_clear(void);
                reusable_update_flag_clear();
            }
#elif (CONFIG_RESFS_UPDATE_ENABLE)
            if (support_reusable_special_update) {
                extern void resfs_update_flag_clear(void);
                resfs_update_flag_clear();
            }
#endif
        }
    }

#if TCFG_AUTO_SHUT_DOWN_TIME
    sys_auto_shut_down_enable();
#endif

#if OTA_TWS_SAME_TIME_ENABLE
    if (UPDATE_DUAL_BANK_IS_SUPPORT()) {
        tws_api_auto_role_switch_enable();
    }
#endif

    g_cur_update_type = 0;
    ota_status = 0;
}

static void update_common_state_cbk(update_mode_info_t *info, u32 state, void *priv)
{
    int type = info->type;

    log_info("type:%x state:%x code:%x\n", type, state, priv);

    switch (state) {
    case UPDATE_CH_INIT:
        //如果开启了VM配置项暂存RAM功能则在每次触发升级前保存数据到vm_flash
        if (get_vm_ram_storage_enable() || get_vm_ram_storage_in_irq_enable()) {
            vm_flush2flash(1);
        }

        memset((u8 *)&succ_report, 0x00, sizeof(succ_report_t));
        update_init_common_handle(info->type);
        dev_update_close_ui();
        break;

    case UPDATE_CH_SUCESS_REPORT:
        log_info("succ report stored\n");
        memcpy((u8 *)&succ_report, (u8 *)priv, sizeof(succ_report_t));
        break;
    }

    if (info->state_cbk) {
        info->state_cbk(type, state, priv);
    }

    switch (state) {
    case UPDATE_CH_EXIT:
        update_exit_common_handle(info->type, priv);
        break;
    case EX_UPDATE_CH_UPDATE_START:
#if (CONFIG_REUSABLE_RESERVE)
        if (priv) {
            if (reusable_loader_region((loader_area_t *)priv)) {
                log_error("default loader to vm\n");
            }
        }
#elif (CONFIG_RESFS_UPDATE_ENABLE)
        if (priv) {
            if (res_sdfile_update_loader_region((loader_area_t *)priv)) {
                log_error("default loader to vm\n");
            }
        }
#endif
        if (!UPDATE_DUAL_BANK_IS_SUPPORT()) {
            g_write_vm_flag = false;
        }
        break;
    case EX_UPDATE_CH_OTA_RT_OUT:
        if (!UPDATE_DUAL_BANK_IS_SUPPORT()) {
            update_ret_code_t *tout_info = (update_ret_code_t *)priv;
            g_ota_reset_tout = *(u8 *)tout_info->priv;
        }
        break;
    }
}


static int app_update_init(void)
{
    update_module_init(update_common_state_cbk);
    testbox_update_init();
    user_file_flash_file_download_init();
#if (TCFG_VIRFAT_INSERT_FLASH_ENABLE || TCFG_NORFLASH_DEV_ENABLE || TCFG_NANDFLASH_DEV_ENABLE || (TCFG_NORFLASH_SFC_DEV_ENABLE && TCFG_NOR_FS))
    if (support_user_file_update_v2_en) {
        user_file_flash_file_download_init();
    } else {
        ex_flash_file_download_init();
    }
#endif
    return 0;
}

__initcall(app_update_init);


void update_start_exit_sniff(void)
{
#if TCFG_APP_BT_EN
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_tws_state() & TWS_STA_PHONE_CONNECTED) {
        g_printf("exit sniff mode...\n");
        bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
    } else {
        tws_api_tx_unsniff_req();
    }
    tws_sniff_controle_check_disable();
#else
    bt_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
#endif
    sys_auto_sniff_controle(0, NULL);
#endif
}

#else // TCFG_UPDATE_ENABLE

void set_ota_status(u8 stu)
{

}

int update_check_sniff_en(void)
{
    return 1;
}

int update_result_deal()
{
    return 0;
}

bool update_success_boot_check(void)
{
    return false;
}

void update_result_set(u16 result)
{

}

u16 update_result_get(void)
{
    return 0;
}

#endif // TCFG_UPDATE_ENABLE

