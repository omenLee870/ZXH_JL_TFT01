#ifndef __POWER_GATE_H__
#define __POWER_GATE_H__


#include "typedef.h"

//STPG define bit
#define     STPG_A              0   //STPG output bit
#define     STPG_HD0            2   //STPG HD0
#define     STPG_HD1            3   //STPG HD1
#define     STPG_OE             4   //STPG output enable bit
#define     STPG_PD             5   //STPG pull down enable bit
#define     STPG_PD1            6   //STPG pull down enable bit

/*
 *@brief  开启stpg供电
 *@param  ms : ms延时
*/
void fspg2_poweron(u32 ms);

/*
 *@brief  关闭stpg供电
 *@param  ms : ms延时
*/
void fspg2_poweroff(u32 ms);


#endif
