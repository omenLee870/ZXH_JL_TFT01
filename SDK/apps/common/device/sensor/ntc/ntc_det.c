#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ntc_det.data.bss")
#pragma data_seg(".ntc_det.data")
#pragma const_seg(".ntc_det.text.const")
#pragma code_seg(".ntc_det.text")
#endif
//*********************************************************************************//
//                                NTC det 									       //
//*********************************************************************************//
//
//
#include "gpadc.h"
#include "ntc_det_api.h"
#include "system/includes.h"
#include "asm/charge.h"
#include "app_msg.h"
#include "asm/efuse.h"

#define NTC_USED_INPUT_100K  1//使用内部上拉

#if NTC_USED_INPUT_100K
static u32 efuse_pb00_100k;
#endif


#if NTC_DET_EN

#define NTC_DET_BAD_RES      0    //分压电阻损坏关闭检测

#ifndef NTC_DET_DUTY1
#define NTC_DET_DUTY1        5000 //检测周期
#endif

#ifndef NTC_DET_DUTY2
#define NTC_DET_DUTY2        10   //检测小周期
#endif

#ifndef NTC_DET_CNT
#define NTC_DET_CNT          3    //检测次数
#endif

#ifndef NTC_DET_UPPER
#define NTC_DET_UPPER        322*1000  //正常范围AD值上限，0度时电阻值
#endif

#ifndef NTC_DET_LOWER
#define NTC_DET_LOWER        44*1000  //正常范围AD值下限，45度时电阻值
#endif

#define NTC_REDCOEFF         10*1000  //温度恢复一定范围后才算正常，防止临界状态,默认设置10k的安全范围,参考100k的电阻


#define NTC_IS_NORMAL(value, offset) (value >= NTC_DET_LOWER+(offset) && value <= NTC_DET_UPPER-(offset))
#define NTC_IS_BAD_RES(value) (value >= 3273*1000 || value <= 2197) //大于142度或者低于-40




enum {
    NTC_STATE_NORMAL = 0,
    NTC_STATE_ABNORMAL,
};

struct ntc_det_t {
    u16 normal_cnt : 4; //温度正常的次数
    u16 cnt : 4; //温度检测的次数
    u16 res_cnt : 4; //分压电阻脱落或损坏
    u16 state : 1; //是否超出范围
    u16 timer;
};
static struct ntc_det_t ntc_det = {0};
extern u8 get_charge_full_flag(void);

u16 ntc_det_working()
{
    return ntc_det.timer;
}

#define NTC_AD_MAX 4096

static u32 ntc_ad_ohm(u32 adc)
{
    return (u32)((adc * efuse_pb00_100k) / ((float)(NTC_AD_MAX - adc)));
}


static void ntc_det_timer_deal(void *priv)
{
    u32 value;

#if NTC_DET_CNT
    if (ntc_det.cnt == 0) {
        sys_timer_modify(ntc_det.timer, NTC_DET_DUTY2);
    }
#endif

    value = adc_get_value(NTC_DET_AD_CH);

    value = ntc_ad_ohm(value);
    printf("adc %d ohm \n", value);


    ntc_det.cnt++;

    if (NTC_IS_NORMAL(value, (ntc_det.state * NTC_REDCOEFF))) { //温度恢复一定范围后才算正常，防止临界状态,默认设置10k的安全范围
        ntc_det.normal_cnt++;
    } else if (NTC_IS_BAD_RES(value)) {
        ntc_det.res_cnt++;
    }
    if (ntc_det.cnt >= NTC_DET_CNT) {
        if (ntc_det.normal_cnt > NTC_DET_CNT / 2) {
            if (ntc_det.state == NTC_STATE_ABNORMAL) {
                printf("temperature recover, start charge");
                ntc_det.state = NTC_STATE_NORMAL;
                charge_start();
            }
        }
#if NTC_DET_BAD_RES
        else if (ntc_det.res_cnt > NTC_DET_CNT / 2) {
            printf("bad res, stop det");
            ntc_det_stop();
        }
#endif
        else {
            if (ntc_det.state == NTC_STATE_NORMAL) {
                printf("temperature is abnormall, stop charge");
                ntc_det.state = NTC_STATE_ABNORMAL;
                charge_close();
                CHARGE_EN(0);
            }
            /* power_set_soft_poweroff(); */
        }
        ntc_det.cnt = 0;
        ntc_det.res_cnt = 0;
        ntc_det.normal_cnt = 0;
        sys_timer_modify(ntc_det.timer, NTC_DET_DUTY1);
    }
}


void ntc_det_start(void)
{
    if (ntc_det.timer == 0) {
        printf("ntc det start");
        memset(&ntc_det, 0, sizeof(ntc_det));
        gpio_write(NTC_POWER_IO, 1);
#if NTC_USED_INPUT_100K
        if (!efuse_pb00_100k) {
            u8 efuse_100k = efuse_get_io_pu_100k();
            if (efuse_100k != 0xff) {
                s8 offset = (efuse_100k & 0x80) ? -(efuse_100k & 0x7F) : efuse_100k;
                efuse_pb00_100k = (u32)((3072 - (524 + offset) * 3) / 0.015f);
            } else {
                printf("PB0_100k not trim\n");
                efuse_pb00_100k = 10 * 1000;
            }
        }
        ASSERT(efuse_pb00_100k > 50 * 1000 && efuse_pb00_100k < 150 * 1000);
        ASSERT(NTC_DETECT_IO == IO_PORTB_00);
        gpio_set_mode(IO_PORT_SPILT(NTC_DETECT_IO), PORT_HIGHZ);
        gpio_hw_set_pull_up(IO_PORT_SPILT(NTC_DETECT_IO), GPIO_PULLUP_100K);
#else
        gpio_set_mode(IO_PORT_SPILT(NTC_DETECT_IO), PORT_HIGHZ);
#endif

        adc_add_sample_ch(NTC_DET_AD_CH);
        adc_delay_set(NTC_DET_AD_CH, 0, 9);//采用不准可以延长采样时间0~15
        ntc_det.timer = sys_timer_add(NULL, ntc_det_timer_deal, NTC_DET_DUTY1);
    }
}

void ntc_det_stop(void)
{
    if (!get_charge_full_flag() && get_charge_online_flag() && ntc_det.state == NTC_STATE_ABNORMAL) {
        printf("charge protecting, wait recover");
        return;
    }
    if (ntc_det.timer) {
        printf("ntc det stop");
        sys_timer_del(ntc_det.timer);
        ntc_det.timer = 0;
        adc_delete_ch(NTC_DET_AD_CH);
        gpio_set_mode(IO_PORT_SPILT(NTC_POWER_IO), PORT_HIGHZ);
    }
}

#if TCFG_CHARGE_ENABLE

static int ntc_msg_entry(int *msg)
{
    switch (msg[0]) {
    case CHARGE_EVENT_CHARGE_START:
        ntc_det_start();
        break;
    case CHARGE_EVENT_CHARGE_CLOSE:
        ntc_det_stop();
        break;
    case CHARGE_EVENT_LDO5V_KEEP:
        ntc_det_stop();
    }
    return 0;
}

APP_MSG_PROB_HANDLER(ntc_msg_entry) = {
    .owner      = 0xff,
    .from       = MSG_FROM_BATTERY,
    .handler    = ntc_event_handler,
};




#endif


/* --------------------------------------------------------------------------*/
/**
 * @brief ntc否可进低功耗查询函数
 *
 * @return 0：不可进
 *         1：可进
 */
/* ----------------------------------------------------------------------------*/
static u8 ntc_idle_query(void)
{
    if (ntc_det.timer) {
        return 0;
    } else {
        return 1;
    }
}

/* REGISTER_LP_TARGET(ntc_driver_target) = { */
/*     .name = "ntc", */
/*     .is_idle = ntc_idle_query, */
/* }; */



#endif

