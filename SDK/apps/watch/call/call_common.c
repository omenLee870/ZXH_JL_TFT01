#include "system/includes.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_common.h"
#include "app_main.h"
#include "avctp_user.h"
#include "audio_cvp.h"
#include "data_storage.h"
#include "app_config.h"

static int call_sel = CALL_SEL_BT;

void call_dial_number(int number_len, char *number)
{
#if TCFG_APP_CAT1_EN
    int ret = false;
    if ((call_sel == CALL_SEL_AUTO) || (call_sel == CALL_SEL_CAT1)) {
        ret = cat1_call_number(number_len, number);
    }
    if (ret == true) {
        set_call_log_call_sel(CALL_SEL_CAT1);
        return ;
    }
    if (call_sel == CALL_SEL_CAT1) {
        return ;
    }
#endif /* #if TCFG_APP_CAT1_EN */

#if TCFG_APP_BT_EN
    if (bt_get_connect_status() !=  BT_STATUS_WAITINT_CONN) {
        small_file_call_log_set_sel(CALL_SEL_BT);
#if TCFG_BT_CALL_PHONE_BY_WATCH
        set_bt_esco_by_watch(1);
#endif
        if (number != NULL) {
            bt_cmd_prepare(USER_CTRL_HFP_DIAL_NUMBER, strlen(number), (u8 *)number);
        }
    }
#endif /* #if TCFG_APP_BT_EN */
}

void call_ctrl_answer()
{
#if TCFG_APP_CAT1_EN
    int ret = false;
    if ((call_sel == CALL_SEL_AUTO) || (call_sel == CALL_SEL_CAT1)) {
        ret = cat1_call_answer();
    }
    if (ret == true) {
        set_call_log_call_sel(CALL_SEL_CAT1);
        return ;
    }
    if (call_sel == CALL_SEL_CAT1) {
        return ;
    }
#endif /* #if TCFG_APP_CAT1_EN */

#if TCFG_APP_BT_EN
    small_file_call_log_set_sel(CALL_SEL_BT);
#if TCFG_BT_CALL_PHONE_BY_WATCH
    set_bt_esco_by_watch(1);
#endif
    bt_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
#endif /* #if TCFG_APP_BT_EN */
}

void call_ctrl_hangup()
{
#if TCFG_APP_CAT1_EN
    int ret = false;
    if ((call_sel == CALL_SEL_AUTO) || (call_sel == CALL_SEL_CAT1)) {
        ret = cat1_call_hangup();
    }
    if (ret == true) {
        set_call_log_call_sel(CALL_SEL_CAT1);
        return ;
    }
    if (call_sel == CALL_SEL_CAT1) {
        return ;
    }
#endif

#if TCFG_APP_BT_EN
    small_file_call_log_set_sel(CALL_SEL_BT);
    bt_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
#endif
}

char *call_ctrl_get_phone_num(void)
{
#if TCFG_APP_CAT1_EN
    char *phone_num = NULL;
    int ret = false;
    if ((call_sel == CALL_SEL_AUTO) || (call_sel == CALL_SEL_CAT1)) {
        ret = cat1_get_call_phone_num(&phone_num);
    }
    if (ret == true) {
        return phone_num;
    }
    if (call_sel == CALL_SEL_CAT1) {
        return NULL;
    }
#endif /* #if TCFG_APP_CAT1_EN */

#if TCFG_APP_BT_EN
    if (g_bt_hdl.phone_num_flag) {
        return (char *)g_bt_hdl.income_phone_num;
    }
#endif /* #if TCFG_APP_BT_EN */
    return NULL;
}

u8 call_ctrl_get_status(void)
{

#if TCFG_APP_CAT1_EN
    u8 status;
    int ret = false;
    if ((call_sel == CALL_SEL_AUTO) || (call_sel == CALL_SEL_CAT1)) {
        ret = cat1_get_call_status(&status);
    }
    if (ret == true) {
        return status;
    }
    if (call_sel == CALL_SEL_CAT1) {
        return BT_CALL_HANGUP;
    }
#endif /* #if TCFG_APP_CAT1_EN */

#if TCFG_APP_BT_EN
    return bt_get_call_status();
#endif /* #if TCFG_APP_BT_EN */

    return BT_CALL_HANGUP;
}

void call_ctrl_set_call_sel(int sel)
{
    if (sel >= CALL_SEL_MAX) {
        return ;
    }
    call_sel = sel;
    if (call_sel != CALL_SEL_AUTO) {
        small_file_call_log_set_sel(call_sel);
    }
}

int call_ctrl_get_call_sel(void)
{
    return call_sel;
}

void call_ctrl_input_mute(u8 mute)
{
#if TCFG_APP_CAT1_EN
    if (true == cat1_check_call_enable()) {
        cat1_call_input_mute(mute);
    } else
#endif /* #if TCFG_APP_CAT1_EN */
    {
        aec_input_clear_enable(mute);
    }
}

int call_ctrl_get_input_mute(void)
{
#if TCFG_APP_CAT1_EN
    if (true == cat1_check_call_enable()) {
        return cat1_call_get_input_mute_status();
    } else
#endif /* #if TCFG_APP_CAT1_EN */
    {
        return aec_get_input_clear_status();
    }
}


