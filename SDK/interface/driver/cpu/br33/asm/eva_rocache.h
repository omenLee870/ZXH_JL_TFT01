
#ifndef __ECU_ROCACHE__
#define __ECU_ROCACHE__

//*********************************************************************************//
// Module name : eva_rocache.h                                                     //
// Description : eva rocache control head file                                     //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#define INCLUDE_ECU_RPT      0
#define INCLUDE_ECU_EMU      0

//------------------------------------------------------//
// dcache function
//------------------------------------------------------//

void EcuEnable(void);
void EcuDisable(void);
void EcuInitial(void);
void EcuWaitIdle(void);
void EcuSetWayNum(u32 way);

void EcuFlushinvAll(void);
void EcuFlushinvRegion(u32 *beg, u32 len);            // note len!=0
void EcuUnlockAll(void);
void EcuUnlockRegion(u32 *beg, u32 len);              // note len!=0
void EcuPfetchRegion(u32 *beg, u32 len);              // note len!=0
void EcuLockRegion(u32 *beg, u32 len);                // note len!=0

void EcuReportEnable(void);
void EcuReportDisable(void);
void EcuReportPrintf(void);
void EcuReportClear(void);

void EcuEmuEnable(void);
void EcuEmuDisable(void);
void EcuEmuMessage(void);

#define EcuInvalidAll      EcuFlushinvAll
#define EcuInvalidRegion   EcuFlushinvRegion

#define PSRAM_EVA_CACHE_ADDR(x)                                                      \
  ((u32)(((u32)(x) > 0x14000000) ? (((u32)(x)) + 0x10000000) : (u32)(x)))

#define FLASH_EVA_CACHE_ADDR(x)                                                      \
  ((u32)(((u32)(x) > 0x8000000) ? (((u32)(x)) + 0x10000000) : (u32)(x)))

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
#endif
