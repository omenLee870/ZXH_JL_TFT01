//========================================================================================
//
//
//  DSI SFR define
//
//
//========================================================================================
#ifndef __EVA_DSI_HAL_H__
#define __EVA_DSI_HAL_H__

#ifndef __EVA_DMY__
#define __EVA_DMY__
#define eva_dmy(start,end) __RW __u32 reserved##start##_##end[end-start+1]
#endif

//===============================================================================//
//
//      config code mapping
//
//===============================================================================//
////  color code（颜色格式码）
////**************************************************
//typedef enum {
//    DSI_CC_RGB565
//   ,DSI_CC_RGB666
//   ,DSI_CC_RGB666_LOOSELY
//   ,DSI_CC_RGB888
//} DSI_COLOR_MODE;

////  color sort（颜色数据排列）
////**************************************************
//typedef enum {
//    DSI_CS_RGB565_DCS
//   ,DSI_CS_RGB565_VDM
//   ,DSI_CS_RGB666
//   ,DSI_CS_RGB666_LOOSELY
//   ,DSI_CS_RGB888
//} DSI_PIXEL_TYPE;

////  sync mode
////**************************************************
//typedef enum {
//    DSI_SYNC_PULSE
//   ,DSI_SYNC_EVENT
//   ,DSI_BURST_MODE
//} DSI_SYNC_MODE;

////  lane mapping
////**************************************************
//typedef enum {
//    DSI_MAP_DISABLE
//   ,DSI_MAP_CK_LANE
//   ,DSI_MAP_D0_LANE
//   ,DSI_MAP_D1_LANE
//   ,DSI_MAP_D2_LANE
//   ,DSI_MAP_D3_LANE
//} DSI_MAP_TYPE;

//===============================================================================//
//
//      SFR bit mapping
//
//===============================================================================//
typedef union {
    struct {
        __u32  dsi_cken             :  1; // [0]
        __u32  dsi_nrst             :  1; // [1]
        __u32  vdo_done_ie          :  1; // [2]
        __u32  cmd_done_ie          :  1; // [3]
        __u32  bta_done_ie          :  1; // [4]
        __u32  bta_fail_ie          :  1; // [5]
        __u32  bus_err_ie           :  1; // [6]
        __u32  frm_err_ie           :  1; // [7]
        __u32  vdo_done_clr         :  1; // [8]
        __u32  vdo_done_pnd         :  1; // [9]
        __u32  cmd_done_clr         :  1; // [10]
        __u32  cmd_done_pnd         :  1; // [11]
        __u32  bta_done_clr         :  1; // [12]
        __u32  bta_done_pnd         :  1; // [13]
        __u32  bta_fail_clr         :  1; // [14]
        __u32  bta_fail_pnd         :  1; // [15]
        __u32  bus_err_clr          :  1; // [16]
        __u32  bus_err_pnd          :  1; // [17]
        __u32  frm_err_clr          :  1; // [18]
        __u32  frm_err_pnd          :  1; // [19]
        __u32  ud_err_ie            :  1; // [20]
        __u32  ov_err_ie            :  1; // [21]
        __u32  ud_err_clr           :  1; // [22]
        __u32  ud_err_pnd           :  1; // [23]
        __u32  ov_err_clr           :  1; // [24]
        __u32  ov_err_pnd           :  1; // [25]
    };
    __u32  value;
} t_dsi_sys_con;

typedef union {
    struct {
        __u32  VREF_EN              :  1; // [0]
        __u32  VBG_S                :  4; // [4:1]
        __u32  VREF_ITRM            :  4; // [8:5]
        __u32  VREF_V400_S          :  3; // [11:9]
        __u32  VREF_VIH_S           :  2; // [13:12]
        __u32  VREF_VIL_S           :  2; // [15:14]
        __u32  LDO_EN               :  1; // [16]
        __u32  LDO_LDS              :  2; // [18:17]
        __u32  LDO_VS               :  4; // [22:19]
        __u32  CK_EN                :  1; // [23]
        __u32  VBG_TESTEN           :  1; // [24]
        __u32  MP_TEST_EN           :  1; // [25]
        __u32  MP_TEST              :  3; // [28:26]
    };
    __u32  value;
} t_dsi_phy_com;

typedef union {
    struct {
        __u32  BIAS_EN              :  1; // [0]
        __u32  HSTX_BIAS_EN         :  1; // [1]
        __u32  HSTX_LDO_EN          :  1; // [2]
        __u32  HSTX_LDO_LDS         :  2; // [4:3]
        __u32  HSTX_RTM             :  4; // [8:5]
        __u32  HSTX_TRF             :  2; // [10:9]
        __u32  LPTX_BIAS_EN         :  1; // [11]
        __u32  LPTX_SR              :  1; // [12]
        __u32  LPRX_HYS_SEL         :  1; // [13]
        __u32  LPRX_IB_SEL          :  1; // [14]
        __u32  rsv_cfg0             :  1; // [15]
        __u32  rsv_cfg1             :  1; // [16]
        __u32  rsv_cfg2             :  1; // [17]
        __u32  rsv_cfg3             :  1; // [18]
        __u32  lprx_dp_rb           :  1; // [19]
        __u32  lprx_dn_rb           :  1; // [20]
    };
    __u32  value;
} t_dsi_phy_con;

typedef union {
    struct {
        __u32  video_mode           :  1; // [0]
        __u32  virtual_channel      :  2; // [2:1]
        __u32  color_code           :  2; // [4:3]
        __u32  color_sort           :  3; // [7:5]
        __u32  sync_mode            :  2; // [9:8]
        __u32  bllp0_en             :  1; // [10]
        __u32  bllp1_en             :  1; // [11]
        __u32  bllp2_en             :  1; // [12]
        __u32  eotp_en              :  1; // [13]
    };
    __u32  value;
} t_dsi_vdo_fmt;

typedef union {
    struct {
        __u32  lpx                  : 16; //  9bit [8:0]
        __u32  wkup                 : 16; // 14bit [29:16]
    };
    __u32  value;
} t_dsi_tval_con0;

typedef union {
    struct {
        __u32  pre_c                : 16; //  9bit [8:0]
        __u32  sot_c                : 16; //  9bit [24:16]
    };
    __u32  value;
} t_dsi_tval_con1;

typedef union {
    struct {
        __u32  eot_c                : 16; //  9bit [8:0]
        __u32  brk_c                : 16; //  9bit [24:16]
    };
    __u32  value;
} t_dsi_tval_con2;

typedef union {
    struct {
        __u32  pre_d                : 16; //  9bit [8:0]
        __u32  sot_d                : 16; //  9bit [24:16]
    };
    __u32  value;
} t_dsi_tval_con3;

typedef union {
    struct {
        __u32  eot_d                : 16; //  9bit [8:0]
        __u32  brk_d                : 16; //  9bit [24:16]
    };
    __u32  value;
} t_dsi_tval_con4;

typedef union {
    struct {
        __u32  rdy_c                : 16; //  9bit [8:0]
    };
    __u32  value;
} t_dsi_tval_con5;

typedef union {
    struct {
        __u32  x0_sel               :  3; // [2:0]
        __u32  x0_inv               :  1; // [3]
        __u32  x1_sel               :  3; // [6:4]
        __u32  x1_inv               :  1; // [7]
        __u32  x2_sel               :  3; // [10:8]
        __u32  x2_inv               :  1; // [11]
        __u32  x3_sel               :  3; // [14:12]
        __u32  x3_inv               :  1; // [15]
        __u32  x4_sel               :  3; // [18:16]
        __u32  x4_inv               :  1; // [19]
    };
    __u32  value;
} t_dsi_rmap_con;

typedef union {
    struct {
        __u32  EN                   :  1; // [0]
        __u32  RN                   :  1; // [1]
        __u32  MODE                 :  1; // [2]
        __u32  /*reserved*/         :  1; // [3]
        __u32  PFDS                 :  3; // [6:4]
        __u32  ICPS                 :  2; // [8:7]
        __u32  /*reserved*/         :  1; // [9]
        __u32  LPFR2S               :  3; // [12:10]
        __u32  IVCOS                :  3; // [15:13]
        __u32  LDO11A_S             :  4; // [19:16]
        __u32  LDO_BYPASS           :  1; // [20]
        __u32  KVCOS                :  2; // [22:21]
        __u32  VCO_CAPS             :  5; // [27:23]
        __u32  VC_OE                :  1; // [28]
        __u32  VC_DRVEN             :  1; // [29]
        __u32  VC_DRVS              :  1; // [30]
    };
    __u32  value;
} t_dsi_pll_con0;

typedef union {
    struct {
        __u32  REFDS                :  7; // [6:0]
        __u32  /*reserved*/         :  1; // [7]
        __u32  refck_sel            :  4; // [11:8]
        __u32  /*reserved*/         :  1; // [12]
        __u32  REFDSEN              :  1; // [13]
        __u32  /*reserved*/         :  2; // [15:14]
        __u32  CP_IOCSEL            :  1; // [16]
        __u32  CP_IOC_EN            :  1; // [17]
        __u32  CP_IOC_S             :  2; // [19:18]
        __u32  TEST_EN              :  1; // [20]
        __u32  TEST_S               :  2; // [22:21]
        __u32  /*reserved*/         :  1; // [23]
        __u32  BIAS_EN              :  1; // [24]
        __u32  RESERV1_3            :  3; // [27:25]
        __u32  OPENLOOP_EN          :  1; // [28]
    };
    __u32  value;
} t_dsi_pll_con1;

typedef union {
    struct {
        __u32  CKOUT_D1P0_OE        :  1; // [0]
        __u32  CKOUT_D1P5_OE        :  1; // [1]
        __u32  CKOUT_D2P0_OE        :  1; // [2]
        __u32  CKOUT_D2P5_OE        :  1; // [3]
        __u32  CKOUT_D3P5_OE        :  1; // [4]
        __u32  CKSYN_CORE_OE        :  1; // [5]
        __u32  CKDSM_CORE_OE        :  1; // [6]
        __u32  pll_cken             :  1; // [7]
        __u32  pll_dsm_cken         :  1; // [8]
        __u32  pll_nr_cken          :  1; // [9]
        __u32  DY                   :  2; // [11:10]
        __u32  FREQ_RANGE           :  2; // [13:12]
    };
    __u32  value;
} t_dsi_pll_con2;

typedef union {
    struct {
        __u32  task_nrst            :  1; // [0]
        __u32  task_en              :  1; // [1]
        __u32  /*reserved*/         :  2; // [3:2]
        __u32  tri_kick             :  1; // [4]
        __u32  cmd_kick             :  1; // [5]
        __u32  /*reserved*/         :  2; // [7:6]
        __u32  cmd_busy             :  1; // [8]
        __u32  vdo_busy             :  1; // [9]
    };
    __u32  value;
} t_dsi_task_con;

typedef union {
    struct {
        __u32  manu_cken            :  1; // [0]
        __u32  lpdt_cken            :  1; // [1]
        __u32  ulps_mode            :  1; // [2]
        __u32  hs_ck_rdy            :  1; // [3]
        __u32  ck_busy              :  1; // [4]
        __u32  da_busy              :  1; // [5]
    };
    __u32  value;
} t_dsi_bus_con;

typedef union {
    struct {
        __u32  tx_code              :  8; // [7:0]
        __u32  tx_prm0              :  8; // [15:8]
        __u32  tx_prm1              :  8; // [23:26]
        __u32  tx_type              :  1; // [24]
        __u32  tx_wbta              :  1; // [25]
    };
    __u32  value;
} t_dsi_cmd_con0;

typedef union {
    struct {
        __u32  rx_prm0              :  8; // [7:0]
        __u32  rx_prm1              :  8; // [15:8]
        __u32  rx_te                :  1; // [16]
        __u32  rx_ack               :  1; // [17]
        __u32  rx_err               :  1; // [18]
        __u32  rx_eot               :  1; // [19]
        __u32  rx_gsr1              :  1; // [20]
        __u32  rx_gsr2              :  1; // [21]
        __u32  rx_glr               :  1; // [22]
        __u32  rx_dsr1              :  1; // [23]
        __u32  rx_dsr2              :  1; // [24]
        __u32  rx_dlr               :  1; // [25]
        __u32  cmd_err              :  1; // [26]
        __u32  rx_tag_clr           :  1; // [27]
    };
    __u32  value;
} t_dsi_cmd_con1;

typedef union {
    struct {
        __u32  rx_err0              :  8; // [7:0]
        __u32  rx_err1              :  8; // [15:8]
    };
    __u32  value;
} t_dsi_cmd_con2;

typedef union {
    struct {
        __u32  d0_lane_en           :  1; // [0]
        __u32  d1_lane_en           :  1; // [1]
        __u32  d2_lane_en           :  1; // [2]
        __u32  d3_lane_en           :  1; // [3]
        __u32  ck_lane_en           :  1; // [4]
        __u32  force_lp_en          :  1; // [5]
        __u32  force_lp_dp          :  1; // [6]
        __u32  force_lp_dn          :  1; // [7]
        __u32  test_mod             :  1; // [8]
    };
    __u32  value;
} t_dsi_lane_con;

//===============================================================================//
//
//      SFR address mapping
//
//===============================================================================//
typedef struct {
    __RW t_dsi_sys_con      SYS_CON         ;
    __RW t_dsi_phy_com      PHY_COM         ;
    __RW t_dsi_phy_con      PHY_CON0        ;
    __RW t_dsi_phy_con      PHY_CON1        ;
    __RW t_dsi_phy_con      PHY_CON2        ;
    __RW t_dsi_phy_con      PHY_CON3        ;
    __RW t_dsi_phy_con      PHY_CON4        ;
    eva_dmy(0x07, 0x0f);
    __RW t_dsi_vdo_fmt      VDO_FMT         ;
    __RW __u32              VDO_VSA         ;
    __RW __u32              VDO_VBP         ;
    __RW __u32              VDO_VACT        ;
    __RW __u32              VDO_VFP         ;
    __RW __u32              VDO_HSA         ;
    __RW __u32              VDO_HBP         ;
    __RW __u32              VDO_HACT        ;
    __RW __u32              VDO_HFP         ;
    __RW __u32              VDO_BLLP0       ;
    __RW __u32              VDO_BLLP1       ;
    __RW __u32              FRM_TVAL        ;
    __RW __u32              VDO_WTH         ;
    eva_dmy(0x1d, 0x1f);
    __RW t_dsi_tval_con0    TVAL_CON0       ;
    __RW t_dsi_tval_con1    TVAL_CON1       ;
    __RW t_dsi_tval_con2    TVAL_CON2       ;
    __RW t_dsi_tval_con3    TVAL_CON3       ;
    __RW t_dsi_tval_con4    TVAL_CON4       ;
    __RW t_dsi_tval_con5    TVAL_CON5       ;
    __RW t_dsi_rmap_con     RMAP_CON        ;
    eva_dmy(0x27, 0x2f);
    __RW t_dsi_pll_con0     PLL_CON0        ;
    __RW t_dsi_pll_con1     PLL_CON1        ;
    __RW t_dsi_pll_con2     PLL_CON2        ;
    __RW __u32              PLL_NR          ;
    __RW __u32              PLL_DSM         ;
    __RW __u32              PLL_INTF_L      ;
    __RW __u32              PLL_INTF_H      ;
    __RW __u32              PLL_SSC         ;
    __RW __u32              PLL_DMIN        ;
    __RW __u32              PLL_DMAX        ;
    __RW __u32              PLL_DSTP        ;
    eva_dmy(0x3b, 0x3f);
    __RW t_dsi_task_con     TASK_CON        ;
    __RW t_dsi_bus_con      BUS_CON         ;
    __RW t_dsi_cmd_con0     CMD_CON0        ;
    __RW t_dsi_cmd_con1     CMD_CON1        ;
    __RW t_dsi_cmd_con2     CMD_CON2        ;
    __RW __u32              CMD_FIFO        ;
    __RW t_dsi_lane_con     LANE_CON        ;
} EVA_DSI_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif

