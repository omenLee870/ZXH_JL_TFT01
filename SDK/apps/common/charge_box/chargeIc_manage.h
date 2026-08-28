#ifndef _CHARGEIC_MANAGE_H
#define _CHARGEIC_MANAGE_H

#include "printf.h"
#include "cpu.h"
#include "timer.h"
#include "app_config.h"
#include "event.h"
#include "system/includes.h"

enum {
    CHARGE_IC_CMD_INIT = 0x0,
    CHARGE_IC_CMD_CHARGE,
    CHARGE_IC_CMD_BOOST,
    CHARGE_IC_CMD_PWR,
    //公版拓展（0x0~0xff）
    //···
    CHARGE_IC_CMD_USER = 0XFF,
    //用户拓展(0x0100 ~ 0x1ff)
    //···

    CHARGE_IC_STATUS_INIT = 0x200,
    CHARGE_IC_STATUS_CHARGE,
    CHARGE_IC_STATUS_BOOST,
    CHARGE_IC_STATUS_PWR,
    //公版拓展（0x200~0x2ff）
    //···
    CHARGE_IC_STATUS_USER = 0X2FF,
    //用户拓展(0x0300 ~ 0x3ff)
    //···

};

struct charge_ic_mod {
    int (*io_ctrl)(u32 cmd, u32 arg);
};
typedef struct charge_ic_mod charge_ic_t;
#define REGISTER_CHAREG_IC_MODULE(mod) \
	const struct charge_ic_mod charge_ic##mod sec(.charge_ic) =
extern const struct charge_ic_mod charge_ic_begin[];
extern const struct charge_ic_mod charge_ic_end[];

#define list_for_each_charge_ic(p) \
    for (p =(struct charge_ic_mod*)charge_ic_begin; p < (struct charge_ic_mod*)charge_ic_end; p++)


void chargeIc_init(void);
void chargeIc_boost_ctrl(u8 en);
void chargeIc_pwr_ctrl(u8 en);
void chargebox_charge_start(void);
void chargebox_charge_close(void);
u8 chargebox_get_charge_en(void);

#endif


