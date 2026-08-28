#ifndef _TP_API_H_
#define _TP_API_H_

#include "app_config.h"

#if TCFG_TOUCH_PANEL_ENABLE


#if TCFG_TP_CST816D_ENABLE
extern void cst816d_init();
extern void cst816d_deinit();
#define  TP_INIT()  cst816d_init()
#define  TP_DEINIT() cst816d_deinit()
#endif


#if TCFG_TP_FT3X68_ENABLE
extern void fts_ts_init();
extern void fts_ts_deinit();
#define  TP_INIT()  fts_ts_init()
#define  TP_DEINIT()  fts_ts_deinit()
#endif
#if TCFG_TP_JD9165T_ENABLE
extern void jd9165t_init();
extern void jd9165t_deinit();
#define  TP_INIT()  jd9165t_init()
#define  TP_DEINIT() jd9165t_deinit()
#endif

#else

#define  TP_INIT()
#define  TP_DEINIT()
#endif /* #if TCFG_TOUCH_PANEL_ENABLE */

#endif


