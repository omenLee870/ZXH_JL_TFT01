#ifndef _CHARGE_H_
#define _CHARGE_H_

#include "typedef.h"
#include "device.h"

/*------充满电电压选择 4.04V-4.66V-------*/
#define CHARGE_FULL_V_4040  0
#define CHARGE_FULL_V_4060  1
#define CHARGE_FULL_V_4080  2
#define CHARGE_FULL_V_4100  3
#define CHARGE_FULL_V_4120  4
#define CHARGE_FULL_V_4140  5
#define CHARGE_FULL_V_4160  6
#define CHARGE_FULL_V_4180  7
#define CHARGE_FULL_V_4200  8
#define CHARGE_FULL_V_4220  9
#define CHARGE_FULL_V_4240  10
#define CHARGE_FULL_V_4260  11
#define CHARGE_FULL_V_4280  12
#define CHARGE_FULL_V_4300  13
#define CHARGE_FULL_V_4320  14
#define CHARGE_FULL_V_4340  15
#define CHARGE_FULL_V_4360  16
#define CHARGE_FULL_V_4380  17
#define CHARGE_FULL_V_4400  18
#define CHARGE_FULL_V_4420  19
#define CHARGE_FULL_V_4440  20
#define CHARGE_FULL_V_4460  21
#define CHARGE_FULL_V_4480  22
#define CHARGE_FULL_V_4500  23
#define CHARGE_FULL_V_4520  24
#define CHARGE_FULL_V_4540  25
#define CHARGE_FULL_V_4560  26
#define CHARGE_FULL_V_4580  27
#define CHARGE_FULL_V_4600  28
#define CHARGE_FULL_V_4620  29
#define CHARGE_FULL_V_4640  30
#define CHARGE_FULL_V_4660  31
#define CHARGE_FULL_V_MAX   32

/*
 	充电电流选择
	恒流：40-460mA
*/
#define CHARGE_mA_40    0
#define CHARGE_mA_60    1
#define CHARGE_mA_80    2
#define CHARGE_mA_100   3
#define CHARGE_mA_120   4
#define CHARGE_mA_140   5
#define CHARGE_mA_160   6
#define CHARGE_mA_180   7
#define CHARGE_mA_200   8
#define CHARGE_mA_220   9
#define CHARGE_mA_240   10
#define CHARGE_mA_260   11
#define CHARGE_mA_280   12
#define CHARGE_mA_300   13
#define CHARGE_mA_320   14
#define CHARGE_mA_340   15
#define CHARGE_mA_360   16
#define CHARGE_mA_380   17
#define CHARGE_mA_400   18
#define CHARGE_mA_420   19
#define CHARGE_mA_440   20
#define CHARGE_mA_460   21

/*
 	充电电流选择
	涓流：4-46mA
*/
#define CHARGE_TC_mA_4     (BIT(5) | CHARGE_mA_40)
#define CHARGE_TC_mA_6     (BIT(5) | CHARGE_mA_60)
#define CHARGE_TC_mA_8     (BIT(5) | CHARGE_mA_80)
#define CHARGE_TC_mA_10    (BIT(5) | CHARGE_mA_100)
#define CHARGE_TC_mA_12    (BIT(5) | CHARGE_mA_120)
#define CHARGE_TC_mA_14    (BIT(5) | CHARGE_mA_140)
#define CHARGE_TC_mA_16    (BIT(5) | CHARGE_mA_160)
#define CHARGE_TC_mA_18    (BIT(5) | CHARGE_mA_180)
#define CHARGE_TC_mA_20    (BIT(5) | CHARGE_mA_200)
#define CHARGE_TC_mA_22    (BIT(5) | CHARGE_mA_220)
#define CHARGE_TC_mA_24    (BIT(5) | CHARGE_mA_240)
#define CHARGE_TC_mA_26    (BIT(5) | CHARGE_mA_260)
#define CHARGE_TC_mA_28    (BIT(5) | CHARGE_mA_280)
#define CHARGE_TC_mA_30    (BIT(5) | CHARGE_mA_300)
#define CHARGE_TC_mA_32    (BIT(5) | CHARGE_mA_320)
#define CHARGE_TC_mA_34    (BIT(5) | CHARGE_mA_340)
#define CHARGE_TC_mA_36    (BIT(5) | CHARGE_mA_360)
#define CHARGE_TC_mA_38    (BIT(5) | CHARGE_mA_380)
#define CHARGE_TC_mA_40    (BIT(5) | CHARGE_mA_400)
#define CHARGE_TC_mA_42    (BIT(5) | CHARGE_mA_420)
#define CHARGE_TC_mA_44    (BIT(5) | CHARGE_mA_440)
#define CHARGE_TC_mA_46    (BIT(5) | CHARGE_mA_460)

/* 充电口下拉电阻 50k ~ 200k */
#define CHARGE_PULLDOWN_50K     0
#define CHARGE_PULLDOWN_100K    1
#define CHARGE_PULLDOWN_150K    2
#define CHARGE_PULLDOWN_200K    3

#define CHARGE_CCVOL_V			300		//涓流充电向恒流充电的转换点

#define DEVICE_EVENT_FROM_CHARGE	(('C' << 24) | ('H' << 16) | ('G' << 8) | '\0')

struct charge_platform_data {
    u8 charge_en;	        //内置充电使能
    u8 charge_poweron_en;	//开机充电使能
    u8 charge_full_V;	    //充满电电压大小
    u8 charge_full_mA;	    //充满电电流大小
    u16 charge_mA;	        //恒流充电电流大小
    u16 charge_trickle_mA;  //涓流充电电流大小
    u8 ldo5v_pulldown_en;   //下拉使能位
    u8 ldo5v_pulldown_lvl;	//ldo5v的下拉电阻配置项,若充电舱需要更大的负载才能检测到插入时，请将该变量置为对应阻值
    u8 ldo5v_pulldown_keep; //下拉电阻在softoff时是否保持,ldo5v_pulldown_en=1时有效
    u16 ldo5v_off_filter;	//ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出,对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的充电仓，需要根据实际情况设置该值大小
    u16 ldo5v_on_filter;    //ldo5v>vbat插入过滤值,电压的过滤时间 = (filter*2)ms
    u16 ldo5v_keep_filter;  //1V<ldo5v<vbat维持电压过滤值,过滤时间= (filter*2)ms
    u16 charge_full_filter; //充满过滤值,连续检测充满信号恒为1才认为充满,过滤时间 = (filter*2)ms
};

#define CHARGE_PLATFORM_DATA_BEGIN(data) \
		struct charge_platform_data data  = {

#define CHARGE_PLATFORM_DATA_END()  \
};


enum {
    CHARGE_EVENT_CHARGE_START,
    CHARGE_EVENT_CHARGE_CLOSE,
    CHARGE_EVENT_CHARGE_FULL,
    CHARGE_EVENT_LDO5V_KEEP,
    CHARGE_EVENT_LDO5V_IN,
    CHARGE_EVENT_LDO5V_OFF,
};

void set_charge_event_flag(u8 flag);
void set_charge_online_flag(u8 flag);
void set_charge_event_flag(u8 flag);
u8 get_charge_online_flag(void);
u8 get_charge_poweron_en(void);
void set_charge_poweron_en(u32 onOff);
void charge_start(void);
void charge_close(void);
u8 get_charge_mA_config(void);
void set_charge_mA(u8 charge_mA);
u8 get_ldo5v_pulldown_en(void);
u8 get_ldo5v_pulldown_res(void);
u8 get_ldo5v_online_hw(void);
u8 get_lvcmp_det(void);
void charge_check_and_set_pinr(u8 mode);
u16 get_charge_full_value(void);
void charge_module_stop(void);
void charge_module_restart(void);
void ldoin_wakeup_isr(void);
int charge_init(const struct charge_platform_data *data);
void charge_set_ldo5v_detect_stop(u8 stop);
u8 check_pinr_shutdown_enable(void);
u16 get_charge_constant_current_val(void);
u16 get_charge_constant_current_progi_volt(void);
#endif    //_CHARGE_H_
