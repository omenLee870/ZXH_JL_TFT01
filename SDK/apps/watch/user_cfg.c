#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".user_cfg.data.bss")
#pragma data_seg(".user_cfg.data")
#pragma const_seg(".user_cfg.text.const")
#pragma code_seg(".user_cfg.text")
#endif
#include "app_config.h"
#include "user_cfg.h"
#include "fs.h"
#include "string.h"
#include "system/includes.h"
#include "vm.h"
#include "btcontroller_config.h"
#include "app_main.h"
#include "media/includes.h"
#include "audio_config.h"
#include "audio_cvp.h"
#include "app_power_manage.h"
#include "config/bt_name_parse.h"
#include "scene_switch.h"
#include "volume_node.h"
#include "classic/tws_api.h"
#include "app_version.h"

#if TCFG_VSTREAM_TRANSFER
#include "kid_wt_app.h"
#endif

#define LOG_TAG             "[USER_CFG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

void app_set_sys_vol(s16 vol_l, s16  vol_r);

BT_CONFIG bt_cfg = {
    .edr_name        = {'Y', 'L', '-', 'B', 'R', '3', '0'},
    .mac_addr        = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    .tws_local_addr  = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
    .rf_power        = 10,
    .dac_analog_gain = 25,
    .mic_analog_gain = 7,
    .tws_device_indicate = 0x6688,
};

//======================================================================================//
//                                 		BTIF配置项表                               		//
//	参数1: 配置项名字                                			   						//
//	参数2: 配置项需要多少个byte存储														//
//	说明: 配置项ID注册到该表后该配置项将读写于BTIF区域, 其它没有注册到该表       		//
//		  的配置项则默认读写于VM区域.													//
//======================================================================================//
const struct btif_item btif_table[] = {
// 	 	item id 		   	   len   	//
    {CFG_BT_MAC_ADDR, 			6 },
    {CFG_BT_FRE_OFFSET,   		6 },   //测试盒矫正频偏值
    //{CFG_DAC_DTB,   			2 },
    //{CFG_MC_BIAS,   			1 },
    {0, 						0 },   //reserved cfg
};

//============================= VM 区域空间最大值 ======================================//
/* const int vm_max_size_config = VM_MAX_SIZE_CONFIG; //该宏在app_cfg中配置 */
const int vm_max_page_align_size_config   = TCFG_VM_SIZE; 		//page对齐vm管理空间最大值配置
const int vm_max_sector_align_size_config = TCFG_VM_SIZE; 	//sector对齐vm管理空间最大值配置

//======================================================================================//
#if TCFG_BT_SNIFF_ENABLE
const struct lp_ws_t lp_winsize = {
    .lrc_ws_inc = CONFIG_LRC_WIN_STEP,      //260
    .lrc_ws_init = CONFIG_LRC_WIN_SIZE,
    .bt_osc_ws_inc = CONFIG_OSC_WIN_STEP,
    .bt_osc_ws_init = CONFIG_OSC_WIN_SIZE,
    .osc_change_mode = 1,
};
#endif

u16 bt_get_tws_device_indicate(u8 *tws_device_indicate)
{
    return bt_cfg.tws_device_indicate;
}

const u8 *bt_get_mac_addr()
{
    return bt_cfg.mac_addr;
}

#if TCFG_TWS_PAIR_BY_BOTH_SIDES

void bt_set_pair_code_en(u8 en)
{
    if (en) {
        tws_api_set_pair_code(bt_cfg.tws_device_indicate);
    } else {
        tws_api_set_pair_code(bt_cfg.tws_device_indicate + 1);
    }
}
#endif

void bt_update_mac_addr(u8 *addr)
{
    ASSERT(addr);
    memcpy(bt_cfg.mac_addr, addr, 6);
}

static u8 bt_mac_addr_for_testbox[6] = {0};
void bt_get_vm_mac_addr(u8 *addr)
{
#if 0
    //中断不能调用syscfg_read;
    int ret = 0;

    ret = syscfg_read(CFG_BT_MAC_ADDR, addr, 6);
    if ((ret != 6)) {
        syscfg_write(CFG_BT_MAC_ADDR, addr, 6);
    }
#else
    ASSERT(addr);
    memcpy(addr, bt_mac_addr_for_testbox, 6);
#endif
}

void bt_get_tws_local_addr(u8 *addr)
{
    ASSERT(addr);
    memcpy(addr, bt_cfg.tws_local_addr, 6);
}

const char *sdk_version_info_get(void)
{
    char *version_str = ((char *)&__VERSION_BEGIN) + 4;

    return version_str;
}

const char *bt_get_local_name()
{
    return (const char *)(bt_cfg.edr_name);
}

const char *bt_get_pin_code()
{
    return "0000";
}

#define USE_CONFIG_BIN_FILE                  0

#define USE_CONFIG_STATUS_SETTING            1                          //状态设置，包括灯状态和提示音
#define USE_CONFIG_AUDIO_SETTING             USE_CONFIG_BIN_FILE        //音频设置
#define USE_CONFIG_KEY_SETTING               USE_CONFIG_BIN_FILE        //按键消息设置
#define USE_CONFIG_MIC_TYPE_SETTING          USE_CONFIG_BIN_FILE        //MIC类型设置
#define USE_CONFIG_AUTO_OFF_SETTING          USE_CONFIG_BIN_FILE        //自动关机时间设置
#define USE_CONFIG_VOL_SETTING               1					        //音量表读配置


__BANK_INIT_ENTRY
void cfg_file_parse(u8 idx)
{
    u8 tmp[128] = {0};
    int ret = 0;

    memset(tmp, 0x00, sizeof(tmp));

    /*************************************************************************/
    /*                      CFG READ IN cfg_tools.bin                        */
    /*************************************************************************/


    //-----------------------------CFG_BT_NAME--------------------------------------//
    ret = syscfg_read(CFG_BT_NAME, tmp, 32);
    if (ret < 0) {
        log_info("read bt name err");
    } else if (ret >= LOCAL_NAME_LEN) {
        memset(bt_cfg.edr_name, 0x00, LOCAL_NAME_LEN);
        memcpy(bt_cfg.edr_name, tmp, LOCAL_NAME_LEN);
        bt_cfg.edr_name[LOCAL_NAME_LEN - 1] = 0;
    } else {
        memset(bt_cfg.edr_name, 0x00, LOCAL_NAME_LEN);
        memcpy(bt_cfg.edr_name, tmp, ret);
    }
#if TCFG_BT_NAME_SEL_BY_AD_ENABLE
    bt_name_config_parse((char *)bt_cfg.edr_name);
#endif
#if TCFG_VSTREAM_TRANSFER
    memset(bt_cfg.edr_name, 0x00, LOCAL_NAME_LEN);
    memcpy(bt_cfg.edr_name, KID_WT_NAME, strlen(KID_WT_NAME));
#endif

    /* g_printf("bt name config:%s", bt_cfg.edr_name); */
    log_info("bt name config:%s", bt_cfg.edr_name);

    //-----------------------------CFG_TWS_PAIR_CODE_ID----------------------------//
    ret = syscfg_read(CFG_TWS_PAIR_CODE_ID, &bt_cfg.tws_device_indicate, 2);
    if (ret < 0) {
        log_debug("read pair code err");
        bt_cfg.tws_device_indicate = 0x8888;
    }
    /* g_printf("tws pair code config:"); */
    log_info("tws pair code config:");
    put_buf((void *)&bt_cfg.tws_device_indicate, 2);

    //-----------------------------CFG_BT_RF_POWER_ID----------------------------//
#if BT_PWR_MAX_ADD_LEVEL
    extern void user_set_pwr_level(s8 adj, s8 set_level);
    user_set_pwr_level(BT_PWR_MAX_ADD_LEVEL, -1);
#endif
#if (TCFG_USER_BT_CLASSIC_ENABLE || TCFG_USER_BLE_ENABLE)
    ret = syscfg_read(CFG_BT_RF_POWER_ID, &app_var.rf_power, 1);
    if (ret < 0) {
        log_debug("read rf err");
        app_var.rf_power = 10;
    }
    int ble_power = 9;
#ifdef TCFG_BT_BLE_TX_POWER
    ble_power = TCFG_BT_BLE_TX_POWER;
#endif
    bt_max_pwr_set(app_var.rf_power, 5, 8, ble_power);//last is ble tx_pwer(0~9)
    /* g_printf("rf config:%d\n", app_var.rf_power); */
    log_info("rf config:%d\n", app_var.rf_power);
#endif /* #if (TCFG_USER_BT_CLASSIC_ENABLE || TCFG_USER_BLE_ENABLE) */

    s16 default_volume;
    s16 music_volume = -1;
    s16 tone_volume = -1;
    s16 call_volume = -1;
    struct volume_cfg music_vol_cfg;
    struct volume_cfg call_vol_cfg;
    struct volume_cfg tone_vol_cfg;
    //赋予相关变量初值
    volume_ioc_get_cfg("Vol_BtmMusic", &music_vol_cfg);
    volume_ioc_get_cfg("Vol_BtcCall", &call_vol_cfg);
    volume_ioc_get_cfg("Vol_SysTone", &tone_vol_cfg);

    ret = syscfg_read(CFG_SYS_VOL, &default_volume, 2);
    if (ret < 0) {
        default_volume = music_vol_cfg.cur_vol;
    }
    ret = syscfg_read(CFG_MUSIC_VOL, &music_volume, 2);
    if (ret < 0) {
        printf("CFG_MUSIC_VOL VM null value\n");
        music_volume = -1;
    }
    /* tone/call 同样要掉电记忆: 读不到 VM 才回退到配置默认值。
     * 否则开机后这两个通道恒为默认值, 用户上次调的音量就丢了 */
    ret = syscfg_read(CFG_TONE_VOL, &tone_volume, 2);
    if (ret < 0) {
        printf("CFG_TONE_VOL VM null value\n");
        tone_volume = -1;
    }
    ret = syscfg_read(CFG_CALL_VOL, &call_volume, 2);
    if (ret < 0) {
        printf("CFG_CALL_VOL VM null value\n");
        call_volume = -1;
    }
    //读不到VM，则表示当前音量使用默认状态
    app_var.volume_def_state = music_volume == -1 ? 1 : 0;
    app_var.music_volume = music_volume == -1 ? default_volume : music_volume;
    app_var.wtone_volume = tone_volume == -1 ? tone_vol_cfg.cur_vol : tone_volume;
    app_var.call_volume = call_volume == -1 ? call_vol_cfg.cur_vol : call_volume;
    app_var.aec_dac_gain = call_vol_cfg.cfg_level_max;
    //主要用于BT音量同步
    app_var.opid_play_vol_sync = default_volume * 127 / music_vol_cfg.cfg_level_max;

    printf("vol_init call %d, music %d, tone %d\n", app_var.call_volume, app_var.music_volume, app_var.wtone_volume);


    ret = syscfg_read(CFG_MIC_EFF_VOLUME_INDEX, &app_var.mic_eff_volume, 2);
    if (ret < 0) {
        app_var.mic_eff_volume = 100;
    }
#if TCFG_SYS_LVD_EN && TCFG_POWER_WARN_VOLTAGE
    app_var.warning_tone_v = TCFG_POWER_WARN_VOLTAGE;
#else
    app_var.warning_tone_v = 3600;
#endif
#if TCFG_SYS_LVD_EN && TCFG_POWER_OFF_VOLTAGE
    app_var.poweroff_tone_v = TCFG_POWER_OFF_VOLTAGE;
#else
    app_var.poweroff_tone_v = 3500;
#endif
    log_info("warning_tone_v:%d poweroff_tone_v:%d", app_var.warning_tone_v, app_var.poweroff_tone_v);

#if USE_CONFIG_AUTO_OFF_SETTING
    /* g_printf("auto off time config:"); */
    log_info("auto off time config:");
    AUTO_OFF_TIME_CONFIG auto_off_time;
    ret = syscfg_read(CFG_AUTO_OFF_TIME_ID, &auto_off_time, sizeof(AUTO_OFF_TIME_CONFIG));
    if (ret > 0) {
        app_var.auto_off_time = auto_off_time.auto_off_time * 60;
    }
    log_info("auto_off_time:%d", app_var.auto_off_time);
#else
    app_var.auto_off_time =  TCFG_AUTO_SHUT_DOWN_TIME;
    log_info("auto_off_time:%d", app_var.auto_off_time);
#endif

#if TCFG_APP_MUSIC_EN
    ret = syscfg_read(CFG_MUSIC_MODE, &app_var.cycle_mode, 1);
    if (ret < 0) {
        log_info("read music play mode err\n");
    }
    if (app_var.cycle_mode >= FCYCLE_MAX || app_var.cycle_mode == 0) {
        log_info("\n app_var.cycle_mode:[%d] set to [%d]\n", app_var.cycle_mode, FCYCLE_ALL);
        app_var.cycle_mode = FCYCLE_ALL;
    }
#endif

    u8 scene_num;
    ret = syscfg_read(CFG_SCENE_INDEX, &scene_num, 1);
    if (ret < 0) {
        scene_num = 0;
    }
    set_default_scene(scene_num);
    log_info("\n default scene %d \n", scene_num);
    u8 eq0_cfg_num;
    ret = syscfg_read(CFG_EQ0_INDEX, &eq0_cfg_num, 1);
    if (ret < 0) {
        eq0_cfg_num = 0;
    }
    set_music_eq_preset_index(eq0_cfg_num);
    log_info("\n default eq0 cfg %d \n", eq0_cfg_num);



    /*************************************************************************/
    /*                      CFG READ IN VM                                   */
    /*************************************************************************/
#if (TCFG_USER_BT_CLASSIC_ENABLE || TCFG_USER_BLE_ENABLE)
    u8 mac_buf[6];
    u8 mac_buf_tmp[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    u8 mac_buf_tmp2[6] = {0, 0, 0, 0, 0, 0};
#if TCFG_USER_TWS_ENABLE
    int len = syscfg_read(CFG_TWS_LOCAL_ADDR, bt_cfg.tws_local_addr, 6);
    if (len != 6) {
        get_random_number(bt_cfg.tws_local_addr, 6);
        syscfg_write(CFG_TWS_LOCAL_ADDR, bt_cfg.tws_local_addr, 6);
    }
    log_info("tws_local_mac:");
    put_buf(bt_cfg.tws_local_addr, sizeof(bt_cfg.tws_local_addr));

#if CONFIG_TWS_USE_COMMMON_ADDR
    ret = syscfg_read(CFG_TWS_COMMON_ADDR, mac_buf, 6);
    if (ret != 6 || !memcmp(mac_buf, mac_buf_tmp, 6))
#endif
#endif
        do {
            ret = syscfg_read(CFG_BT_MAC_ADDR, mac_buf, 6);
            if ((ret != 6) || !memcmp(mac_buf, mac_buf_tmp, 6) || !memcmp(mac_buf, mac_buf_tmp2, 6)) {
                get_random_number(mac_buf, 6);
                syscfg_write(CFG_BT_MAC_ADDR, mac_buf, 6);
            }
        } while (0);


    syscfg_read(CFG_BT_MAC_ADDR, bt_mac_addr_for_testbox, 6);
    if (!memcmp(bt_mac_addr_for_testbox, mac_buf_tmp, 6)) {
        get_random_number(bt_mac_addr_for_testbox, 6);
        syscfg_write(CFG_BT_MAC_ADDR, bt_mac_addr_for_testbox, 6);
        log_info(">>>init mac addr!!!\n");
    }

    log_info("mac:");
    put_buf(mac_buf, sizeof(mac_buf));
    memcpy(bt_cfg.mac_addr, mac_buf, 6);

#if (CONFIG_BT_MODE != BT_NORMAL)
#ifdef CONFIG_CPU_BR35
    const u8 dut_name[]  = "AC707N_DUT";
#elif defined CONFIG_CPU_BR33
    const u8 dut_name[]  = "JL380N_DUT";
#else
    const u8 dut_name[]  = "AC693x_DUT";
#endif
    const u8 dut_addr[6] = {0x12, 0x34, 0x56, 0x56, 0x34, 0x12};
    memcpy(bt_cfg.edr_name, dut_name, sizeof(dut_name));
    memcpy(bt_cfg.mac_addr, dut_addr, 6);
#endif

#endif/* #if (TCFG_USER_BT_CLASSIC_ENABLE || TCFG_USER_BLE_ENABLE) */

    /*************************************************************************/
    /*                      CFG READ IN isd_config.ini                       */
    /*************************************************************************/
    /* LRC_CONFIG lrc_cfg; */
    /* ret = syscfg_read(CFG_LRC_ID, &lrc_cfg, sizeof(LRC_CONFIG)); */
    /* if (ret > 0) { */
    /*     log_info("lrc cfg:"); */
    /*     put_buf((void *)&lrc_cfg, sizeof(LRC_CONFIG)); */
    /*     lp_winsize.lrc_ws_inc      = lrc_cfg.lrc_ws_inc; */
    /*     lp_winsize.lrc_ws_init     = lrc_cfg.lrc_ws_init; */
    /*     lp_winsize.bt_osc_ws_inc   = lrc_cfg.btosc_ws_inc; */
    /*     lp_winsize.bt_osc_ws_init  = lrc_cfg.btosc_ws_init; */
    /*     lp_winsize.osc_change_mode = lrc_cfg.lrc_change_mode; */
    /* } */
    /* printf("%d %d %d ",lp_winsize.lrc_ws_inc,lp_winsize.lrc_ws_init,lp_winsize.osc_change_mode); */
#if TCFG_BT_SNIFF_ENABLE
    lp_winsize_init(&lp_winsize);
#endif
}

int bt_modify_name(u8 *new_name)
{
    ASSERT(new_name);
    u8 new_len = strlen((void *)new_name);
    u8 *name_modify = zalloc(LOCAL_NAME_LEN);
    ASSERT(name_modify);
    memcpy(name_modify, new_name, strlen((const char *)new_name));
    if (new_len >= LOCAL_NAME_LEN) {
        name_modify[LOCAL_NAME_LEN - 1] = 0;
    }
    if (strcmp((void *)name_modify, (void *)bt_cfg.edr_name)) {
        syscfg_write(CFG_BT_NAME, name_modify, LOCAL_NAME_LEN);
        memset(bt_cfg.edr_name, 0x00, LOCAL_NAME_LEN);
        memcpy(bt_cfg.edr_name, name_modify, LOCAL_NAME_LEN);
        lmp_hci_write_local_name(bt_get_local_name());
        log_info("mdy_name sucess\n");
        free(name_modify);
        return 1;
    }
    free(name_modify);
    return 0;
}


