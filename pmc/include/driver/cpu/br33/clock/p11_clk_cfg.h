//***********************************************
//    File Name:    p11_clk_cfg.h
//    Author:       Luocong
//    Mail:         cong_luo@zh-jieli.com
//    Create Time:  2024年05月09日 星期四 10时14分13秒
//***********************************************

#ifndef __P11_CLK_CFG__
#define __P11_CLK_CFG__

void p11_lp_lrc24m_cfg(u32 cap, u32 rs, u32 is);
u8 get_lrc24m_caps(void);
u8 get_lrc24m_rs(void);
u8 get_lrc24m_is(void);

void p11_lp_btosc_cfg(u32 pin_mode, u32 ldo, u32 enable);
u8 get_btxosc_pin_mode(void);
u8 get_xosc_ldo(void);
u8 get_lp_btosc_enable(void);

#endif

