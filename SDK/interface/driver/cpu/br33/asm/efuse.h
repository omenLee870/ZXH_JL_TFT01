#ifndef  __EFUSE_H__
#define  __EFUSE_H__

#define CHIP_VERSION_A      0x00
#define CHIP_VERSION_B      0x01
#define CHIP_VERSION_C      0x02



u32 efuse_get_gpadc_vbg_trim();
u32 get_chip_version();

/************************  p33 efuse page0  ***************************/
u8 get_en_act();
u8 get_vddio_lvd_en();
u8 efuse_get_lvd_act();
u8 efuse_get_vddio_lvd_lev();
u8 efuse_get_vio_act();
u8 efuse_get_vddio_lev();
u8 efuse_get_mclr_en();
u8 efuse_get_pin_reset_en();
u8 efuse_get_fast_up();
u8 efuse_get_vad_en();
u8 efuse_get_dcvdd_ovp_dis();
u8 efuse_get_dcvdd_ovp_flt_sel();
u8 efuse_get_vbg_act();
u8 efuse_get_lvd_bg_trim();
u8 efuse_get_mvbg_lev();
u8 efuse_get_dcvdd_ovp_s();
u8 efuse_get_x32k_dis();

/************************  p33 efuse page1  ***************************/
u8 efuse_get_en_wvbg_lev();
u8 efuse_get_vpwr_det_vrefs();
u8 efuse_get_vpwr_det_hyst();
u8 efuse_get_efuse_version();
u8 efuse_get_msys_efusee_block_en();

/************************  msys efuse page0  ***************************/
u8 efuse_get_xosc_ldo();
u8 efuse_get_cp_pass();
u8 efuse_get_ft_pass();
u8 efuse_get_wvdd_level_trim();
u8 efuse_get_res_current_trim();
u8 efuse_get_xosc_pin_mode();
u8 efuse_get_maskrom_pu_clk_sel();
u8 efuse_get_safe_startup();
u8 efuse_get_bttx_pwr_trim();
u8 efuse_get_usb_upgrade_mode();
u8 efuse_get_sfc_startup_sel();
u8 efuse_get_flash_reset();
u8 efuse_get_flash_io_select();
u8 efuse_get_audio_bg(void);
u8 efuse_get_xosc_ext_init();
u8 efuse_get_fll_res_s(void);
u8 efuse_get_tfll_act();

/************************  msys efuse page1  ***************************/
u8 efuse_get_tchg_act();
u8 efuse_get_ctrim_act();
u8 efuse_get_vbat_trim_4p2(void);
u8 efuse_get_vbat_trim_4p4(void);
u8 efuse_get_charge_cur_trim(void);
u8 efuse_get_pll_vco_caps(void);
u8 efuse_get_pll_ldo11a_s(void);
u8 efuse_get_fll_vr11_s(void);
u8 efuse_get_fll_cap_s(void);

/************************  msys efuse page2  ***************************/
u8 efuse_get_io_pu_200k(void);
u8 efuse_get_vad_bg(void);
u8 efuse_get_atrim_act();
u8 efuse_get_dcdc18_vp17_vol_trim(void);
u8 efuse_get_dcdc12_vp17_vol_trim(void);
u8 efuse_get_io_pu_10k(void);
u8 efuse_get_vbg_ft_act();
u8 efuse_get_mvbg_trim_ft();
u8 efuse_get_htrim_act();

/************************  msys efuse page3  ***************************/
u8 efuse_get_mipipll_vco_caps();
u8 efuse_get_mipipll_freq_range();
u8 efuse_get_mipipll_ldo11a_s();
u8 efuse_get_psrampll_vco_caps();
u8 efuse_get_psrampll_ldo11a_s();
u8 efuse_get_usbpll_vco_caps();
u8 efuse_get_usbpll_ldo11a_s();
u8 efuse_get_hstx_res();

/************************  msys efuse page4  ***************************/
u8 efuse_get_chg_ft_act();
u32 efuse_get_vbat_3700();
u32 efuse_get_extern_voltage_trim();


u32 efuse_get_chip_id();
u16 get_chip_id();

void efuse_init();
void efuse_test();


#endif  /*EFUSE_H*/
