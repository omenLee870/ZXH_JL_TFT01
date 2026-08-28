#ifndef __Q32DSP_DCACHE__
#define __Q32DSP_DCACHE__

//*********************************************************************************//
// Module name : dcache.h                                                          //
// Description : q32DSP dcache control head file                                   //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//
#define  INCLUDE_DCU

#define INCLUDE_DCU_RPT      0
#define INCLUDE_DCU_EMU      0
#define INCLUDE_L1D          0
#define INCLUDE_L2D          0

//------------------------------------------------------//
// peripheral level 1 function
//------------------------------------------------------//

#if (INCLUDE_L1D)

void L1pEnable(void);
void L1pDisable(void);
void L1pInitial(void);
void L1pSetWayNum(unsigned int way);

void L1pInvalidAll(void);
void L1pInvalidRegion(unsigned int *beg, unsigned int len);     // note len!=0
void L1pFlushAll(void);
void L1pFlushRegion(unsigned int *beg, unsigned int len);       // note len!=0
void L1pFlushinvAll(void);
void L1pFlushinvRegion(unsigned int *beg, unsigned int len);    // note len!=0
void L1pPfetchRegion(unsigned int *beg, unsigned int len);      // note len!=0

void L1pReportEnable(void);
void L1pReportDisable(void);
void L1pReportClear(void);
void L1pReportPrintf(void);

void L1pEmuEnable(void);
void L1pEmuDisable(void);
void L1pEmuMessage(void);

void L1pWrThroughRegion(unsigned int num, unsigned int *beg, unsigned int len);
void L1pRwThroughRegion(unsigned int num, unsigned int *beg, unsigned int len);
void L1pPrivateRegion(unsigned int num, unsigned int *beg, unsigned int len);

#else

#define L1pEnable             DcuEnable
#define L1pDisable            DcuDisable
#define L1pInitial            DcuInitial
#define L1pWaitIdle           DcuWaitIdle
#define L1pSetWayNum          DcuSetWayNum

#define L1pInvalidAll         DcuInvalidAll
#define L1pInvalidRegion      DcuInvalidRegion
#define L1pFlushAll           DcuFlushAll
#define L1pFlushRegion        DcuFlushRegion
#define L1pFlushinvAll        DcuFlushinvAll
#define L1pFlushinvRegion     DcuFlushinvRegion
#define L1pPfetchRegion       DcuPfetchRegion

#define L1pReportEnable       DcuReportEnable
#define L1pReportDisable      DcuReportDisable
#define L1pReportClear        DcuReportClear
#define L1pReportPrintf       DcuReportPrintf

#define L1pEmuEnable          DcuEmuEnable
#define L1pEmuDisable         DcuEmuDisable
#define L1pEmuMessage         DcuEmuMessage

#define L1pWrThroughRegion    DcuWrThroughRegion
#define L1pRwThroughRegion    DcuRwThroughRegion
#define L1pPrivateRegion      DcuPrivateRegion

#endif

//------------------------------------------------------//
// dcache level 1 function
//------------------------------------------------------//

void DcuEnable(void);
void DcuDisable(void);
void DcuInitial(void);
void DcuWaitIdle(void);
void DcuSetWayNum(unsigned int way);

void DcuInvalidAll(void);
void DcuInvalidRegion(unsigned int *beg, unsigned int len);     // note len!=0
void DcuFlushAll(void);
void DcuFlushRegion(unsigned int *beg, unsigned int len);       // note len!=0
void DcuFlushinvAll(void);
void DcuFlushinvRegion(unsigned int *beg, unsigned int len);    // note len!=0
void DcuPfetchRegion(unsigned int *beg, unsigned int len);      // note len!=0

void DcuReportEnable(void);
void DcuReportDisable(void);
void DcuReportClear(void);
void DcuReportPrintf(void);

void DcuEmuEnable(void);
void DcuEmuDisable(void);
void DcuEmuMessage(void);

void DcuWrThroughRegion(unsigned int num, unsigned int *beg, unsigned int len);
void DcuRwThroughRegion(unsigned int num, unsigned int *beg, unsigned int len);
void DcuPrivateRegion(unsigned int num, unsigned int *beg, unsigned int len);

//------------------------------------------------------//
// dcache level 2 function
//------------------------------------------------------//

#if (INCLUDE_L2D)

void L2dEnable(void);
void L2dDisable(void);
void L2dInitial(void);
void L2dSetWayNum(unsigned int way);

void L2dInvalidAll(void);
void L2dInvalidRegion(unsigned int *beg, unsigned int len);     // note len!=0
void L2dFlushAll(void);
void L2dFlushRegion(unsigned int *beg, unsigned int len);       // note len!=0
void L2dFlushinvAll(void);
void L2dFlushinvRegion(unsigned int *beg, unsigned int len);    // note len!=0
void L2dPfetchRegion(unsigned int *beg, unsigned int len);      // note len!=0

void L2dReportEnable(void);
void L2dReportDisable(void);
void L2dReportClear(void);
void L2dReportPrintf(void);

void L2dEmuEnable(void);
void L2dEmuDisable(void);
void L2dEmuMessage(void);

void L2dWrThroughRegion(unsigned int num, unsigned int *beg, unsigned int len);
void L2dRwThroughRegion(unsigned int num, unsigned int *beg, unsigned int len);
void L2dPrivateRegion(unsigned int num, unsigned int *beg, unsigned int len);

#endif

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
#endif

