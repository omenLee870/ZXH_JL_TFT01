#ifndef _PMC_ROM_API_H_
#define _PMC_ROM_API_H_
//---------------------------------------------//
// rom functions
//---------------------------------------------//

void idle(void);
void standby(volatile u32 *pwr_con_sfr);
void standby_ext(volatile u32 *pwr_con_sfr);
void sleep_ext(volatile u32 *pwr_con_sfr);
void sleep(volatile u32 *pwr_con_sfr);
void deep_sleep(volatile u32 *nvpwr_sfr, volatile u32 *pvdd_sfr, volatile u32 *pwr_con_sfr, u32 dly, u32 pvdd_set, u8 nvpwr_set);


#define CALL_IDLE()               //idle()
#define CALL_STANDBY()            //standby(&PMC_CLOCK->PWR_CON)
#define CALL_SLEEP()              //sleep(&PMC_CLOCK->PWR_CON)
#define CALL_STANDBY_EXT()        //standby_ext(&PMC_CLOCK->PWR_CON)
#define CALL_SLEEP_EXT()          //sleep_ext(&PMC_CLOCK->PWR_CON)
#define CALL_DEEP_SLEEP(x,y,z)    //deep_sleep(&P3_NVRAM_PWR, &P3_PVDD1_AUTO, &PMC_CLOCK->PWR_CON, x, y, z)

#endif /* #ifndef _ROM_API_H_ */
