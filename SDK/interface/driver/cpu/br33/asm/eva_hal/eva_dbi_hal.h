//========================================================================================
//
//
//  DBI SFR define
//
//
//========================================================================================
#ifndef __EVA_DBI_HAL_H__
#define __EVA_DBI_HAL_H__

#ifndef __EVA_DMY__
#define __EVA_DMY__
#define eva_dmy(start,end) __RW __u32 reserved##start##_##end[end-start+1]
#endif

//===============================================================================//
//
//      config code mapping
//
//===============================================================================//
//  input format
//**************************************************
typedef enum {
    DBI_IN_ARGB8888     = 0x00
    , DBI_IN_ARGB8565     = 0x01
    , DBI_IN_RGB888       = 0x04
    , DBI_IN_RGB565       = 0x05
    , DBI_IN_YUV422_BT601 = 0x06
    , DBI_IN_YUV422_BT709 = 0x07
    , DBI_IN_YUV444_BT601 = 0x0B
    , DBI_IN_YUV444_BT709 = 0x0C
    , DBI_IN_RAW8         = 0x1A
} DBI_IFMT_ENUM;

//  convert format
//**************************************************
typedef enum {
    DBI_CVT_BYPASS
    , DBI_CVT_YUV2RGB
    , DBI_CVT_RGB2YUV
} DBI_CVT_ENUM;

//  dither mode
//**************************************************
typedef enum {
    DBI_DTH_BYPASS
    , DBI_DTH_RGB565
    , DBI_DTH_RGB666
} DBI_DTH_ENUM;

//  sync mode
//**************************************************
typedef enum {
    DBI_SYN_DLY
    , DBI_SYN_TE_R
    , DBI_SYN_TE_F
} DBI_SYN_ENUM;

//  cflow protocol
//**************************************************
typedef enum {
    DBI_TA
    , DBI_TB
    , DBI_TC_O1
    , DBI_TC_O3
    , DBI_TC_O4
    , DBI_MSPI
} DBI_PROTOCOL_ENUM;

//  cflow line
//**************************************************
typedef enum {
    DBI_LINE1
    , DBI_LINE2
    , DBI_LINE4
    , DBI_LINE8
    , DBI_LINE9
    , DBI_LINE16
    , DBI_LINE18
    , DBI_LINE24
} DBI_LINE_ENUM;

//  cflow pix num
//**************************************************
typedef enum {
    DBI_1_PIX
    , DBI_2_PIX
} DBI_PNUM_ENUM;

//  cflow split num
//**************************************************
typedef enum {
    DBI_1_TRANS
    , DBI_2_TRANS
    , DBI_3_TRANS
} DBI_TNUM_ENUM;

//  cflow output format
//**************************************************
typedef enum {
    DBI_OUT_RGB565
    , DBI_OUT_RGB666
    , DBI_OUT_RGB888
    , DBI_OUT_YUVMOD
} DBI_OFMT0_ENUM;

typedef enum {
    DBI_OUT_YUV444
    , DBI_OUT_YUV422
    , DBI_OUT_BT656
    , DBI_OUT_BT1120
} DBI_OFMT1_ENUM;

//  cflow handle opcode
//**************************************************
typedef enum {
    DBI_OP_SET_START   // 开启CS
    , DBI_OP_SET_END    // 关闭CS
    , DBI_OP_SET_TXC    // 切换到命令发送模式
    , DBI_OP_SET_TXD    // 切换到数据发送模式
    , DBI_OP_SET_RXD    // 切换到数据接收模式
    , DBI_OP_SEND_DLY   // 发送delay，无时钟输出
    , DBI_OP_SEND_DMY   // 发送dummy，有时钟输出
    , RESERVED
    , DBI_OP_SEND_DAT   // 发送数据
    , DBI_OP_RECE_DAT   // 接收数据
    , DBI_OP_SET_BREAK  // 关闭CS后必须跟此操作
} DBI_OPCODE_ENUM;

//===============================================================================//
//
//      SFR bit mapping
//
//===============================================================================//
typedef union {
    struct {
        __u32  dma_cken         :  1; // [0]
        __u32  dma_nrst         :  1; // [1]
        __u32  dbi_cken         :  1; // [2]
        __u32  dbi_nrst         :  1; // [3]
        __u32  susp_req         :  1; // [4]
        __u32  dma_kick         :  1; // [5]
        __u32  /*reserved*/     :  2; // [7:6]
        __u32  fc_busy0         :  1; // [8]
        __u32  fc_busy1         :  1; // [9]
        __u32  fc_busy2         :  1; // [10]
        __u32  fc_busy3         :  1; // [11]
        __u32  fc_busy4         :  1; // [12]
        __u32  fc_busy5         :  1; // [13]
        __u32  fc_all_idle      :  1; // [14]
        __u32  fc_susp_rdy      :  1; // [15]
    };
    __u32  value;
} t_dbi_com_con;

//**************************************************

typedef union {
    struct {
        __u32  fetch_done       :  1; // [0]
        __u32  update_done      :  1; // [1]
        __u32  susp_done        :  1; // [2]
        __u32  tgen_done        :  1; // [3]
        __u32  cf_hdl_done      :  1; // [4]
        __u32  cf_task_done     :  1; // [5]
        __u32  te_rise          :  1; // [6]
        __u32  te_fall          :  1; // [7]
        __u32  wmark_done       :  1; // [8]
        __u32  task_error       :  1; // [9]
    };
    __u32  value;
} t_dbi_pnd_con;

//**************************************************

typedef union {
    struct {
        __u32  continue_mode    :  1; // [0]
        __u32  linkage_mode     :  1; // [1]
        __u32  connect_dsi      :  1; // [2]
        __u32  format           :  6; // [8:3]
        __u32  aph_end          :  1; // [9]
        __u32  big_end          :  1; // [10]
        __u32  rb_swap          :  1; // [11]
        __u32  ext_mod          :  1; // [12]
        __u32  cvt_mod          :  2; // [14:13]
        __u32  itu_601          :  1; // [15]
        __u32  dth_mod          :  2; // [17:16]
        __u32  sof_flag         :  1; // [18]
        __u32  test_mode        :  1; // [19]
        __u32  sca_en           :  1; // [20]
    };
    __u32  value;
} t_dbi_dma_con;

//**************************************************

typedef union {
    struct {
        __u32  hstp             :  9; // [8:0]
        __u32  /*reserved*/     :  7; // [15:9]
        __u32  vstp             :  9; // [24:16]
    };
    __u32  value;
} t_dbi_sca_stp;

//**************************************************

typedef union {
    struct {
        __u32  width            : 12; // [11:0]
        __u32  /*reserved*/     :  4; // [15:12]
        __u32  high             : 12; // [27:16]
    };
    __u32  value;
} t_dbi_sca_size;

//**************************************************

typedef union {
    struct {
        __u32  /*reserved*/     :  4; // [3:0]
        __u32  xcvr_sta         :  4; // [7:4]
        __u32  cf_hdl_busy      :  1; // [8]
        __u32  cf_task_busy     :  1; // [9]
        __u32  tg_task_busy     :  1; // [10]
    };
    __u32  value;
} t_dbi_sta_con;

//**************************************************

typedef union {
    struct {
        __u32  continue_mode    :  1; // [0]
        __u32  vdo_mode         :  1; // [1]
        __u32  vsyn_oe          :  1; // [2]
        __u32  vbp_oe           :  1; // [3]
        __u32  vfp_oe           :  1; // [4]
        __u32  vact_oe          :  1; // [5]
        __u32  sync_mode        :  2; // [7:6]
    };
    __u32  value;
} t_dbi_tgen_con;

//**************************************************

typedef union {
    struct {
        __u32  protocol         :  3; // [2:0]
        __u32  pix_line         :  3; // [5:3]
        __u32  pix_num          :  1; // [6]
        __u32  pix_spl          :  2; // [8:7]
        __u32  pix_fmt          :  2; // [10:9]
        __u32  cmd_line         :  3; // [13:11]
        __u32  adr_line         :  3; // [16:14]
        __u32  dma_line         :  3; // [19:17]
        __u32  dma_mode         :  1; // [20]
    };
    __u32  value;
} t_dbi_cflow_con0;

//**************************************************

typedef union {
    struct {
        __u32  dmy_cfg0         :  8; // [7:0]
        __u32  dmy_cfg1         :  8; // [15:8]
        __u32  dmy_cfg2         :  8; // [23:16]
        __u32  dly_mod0         :  1; // [24]
        __u32  dly_mod1         :  1; // [25]
        __u32  dly_mod2         :  1; // [26]
    };
    __u32  value;
} t_dbi_cflow_con1;

//**************************************************

typedef union {
    struct {
        __u32  start            :  1; // [0]
        __u32  cmd              :  1; // [1]
        __u32  dmy0             :  1; // [2]
        __u32  dcx1             :  1; // [3]
        __u32  adr0             :  1; // [4]
        __u32  adr1             :  1; // [5]
        __u32  adr2             :  1; // [6]
        __u32  dmy1             :  1; // [7]
        __u32  hbp              :  1; // [8]
        __u32  hact             :  1; // [9]
        __u32  hfp              :  1; // [10]
        __u32  dmy2             :  1; // [11]
        __u32  end              :  1; // [12]
    };
    __u32  value;
} t_dbi_cflow_seg;

//**************************************************

typedef union {
    struct {
        __u32  cmd              :  8; // [7:0]
        __u32  adr0             :  8; // [15:8]
        __u32  adr1             :  8; // [23:16]
        __u32  adr2             :  8; // [31:24]
    };
    __u32  value;
} t_dbi_cflow_cmd;

//**************************************************

typedef union {
    struct {
        __u32  cs_setup         :  8; // [7:0]
        __u32  cs_hold          :  8; // [15:8]
        __u32  sta_setup        :  8; // [23:16]
        __u32  cs_break         :  8; // [31:24]
    };
    __u32  value;
} t_dbi_cflow_con13;

//**************************************************

typedef union {
    struct {
        __u32  op               :  4; // [3:0]
        __u32  line             :  3; // [6:4]
        __u32  num              :  2; // [8:7]
        __u32  rx_mode          :  1; // [9]
        __u32  rx_div           :  8; // [17:10]
        __u32  rx_bsel          :  1; // [18]
        __u32  rx_phase         :  1; // [19]
        __u32  ddr_en           :  1; // [20]
        __u32  /*reserved*/     : 10; // [30:21]
        __u32  kick             :  1; // [31]
    };
    __u32  value;
} t_dbi_hdl_con;

//**************************************************

typedef union {
    struct {
        __u32  pix_fmt          :  2; // [1:0]
        __u32  pix_cyc          :  2; // [3:2]
        __u32  blk_mod          :  1; // [4]
        __u32  rb_swap          :  1; // [5]
        __u32  big_end          :  1; // [6]
        __u32  clk_mod          :  1; // [7]
        __u32  itu_mod          :  2; // [9:8]
        __u32  yuv_lmt          :  1; // [10]
    };
    __u32  value;
} t_dbi_vflow_con;

//**************************************************

typedef union {
    struct {
        __u32                   : 18; // [17:0]
        __u32                   :  1; // [18]
        __u32                   :  1; // [19]
        __u32                   :  1; // [20]
        __u32                   :  1; // [21]
        __u32 csx_pol           :  1; // [22]
        __u32 dcx_pol           :  1; // [23]
        __u32 rwx_pol           :  1; // [24]
        __u32 ck_pol            :  1; // [25]
        __u32 ck_phase          :  1; // [26]
        __u32 force_cken        :  1; // [27]
        __u32 ddr_mode          :  1; // [28]
    };
    __u32  value;
} t_dbi_port_con;

//===============================================================================//
//
//      SFR address mapping
//
//===============================================================================//
typedef struct {
    __RW t_dbi_com_con      COM_CON         ;
    __RW t_dbi_pnd_con      PND_CON         ;
    __RW t_dbi_pnd_con      PND_CLR         ;
    __RW t_dbi_pnd_con      PND_IE          ;
    eva_dmy(0x04, 0x07);
    __RW t_dbi_dma_con      DMA_CON         ;
    __RW __u32              DMA_ADR         ;
    __RW __u32              DMA_LINE        ;
    __RW __u32              DMA_BURST       ;
    __RW __u32              DMA_STRIDE      ;
    __RW __u32              DMA_WMARK       ;
    __RW t_dbi_sca_stp      SCA_STP         ;
    __RW t_dbi_sca_size     SCA_SIZE        ;
    __RW t_dbi_sta_con      STA_CON         ;
    __RW t_dbi_tgen_con     TGEN_CON        ;
    __RW __u32              TGEN_VSYN       ;
    __RW __u32              TGEN_VBP        ;
    __RW __u32              TGEN_VFP        ;
    __RW __u32              TGEN_VACT       ;
    __RW __u32              TGEN_HTTL       ;
    __RW __u32              TGEN_HSYN       ;
    __RW __u32              TGEN_HBP        ;
    __RW __u32              TGEN_HFP        ;
    __RW __u32              TGEN_HACT       ;
    __RW __u32              SYNC_DLY        ;
    __RW __u32              SYNC_WTH        ;
    eva_dmy(0x1d, 0x1f);
    __RW t_dbi_cflow_con0   CFLOW_CON0      ;
    __RW t_dbi_cflow_con1   CFLOW_CON1      ;
    __RW t_dbi_cflow_seg    CFLOW_DDR_SEG   ;
    __RW t_dbi_cflow_seg    CFLOW_SLINE_SEG ;
    __RW t_dbi_cflow_cmd    CFLOW_SLINE_CMD ;
    __RW t_dbi_cflow_seg    CFLOW_PLINE_SEG ;
    __RW t_dbi_cflow_cmd    CFLOW_PLINE_CMD ;
    __RW t_dbi_cflow_seg    CFLOW_ACT_S_SEG ;
    __RW t_dbi_cflow_cmd    CFLOW_ACT_S_CMD ;
    __RW t_dbi_cflow_seg    CFLOW_ACT_E_SEG ;
    __RW t_dbi_cflow_cmd    CFLOW_ACT_E_CMD ;
    __RW t_dbi_cflow_seg    CFLOW_ACT_N_SEG ;
    __RW t_dbi_cflow_cmd    CFLOW_ACT_N_CMD ;
    __RW t_dbi_cflow_con13  CFLOW_CON13     ;
    eva_dmy(0x2e, 0x2f);
    __RW t_dbi_hdl_con      HDL_CON         ;
    __RW __u32              HDL_BUF         ;
    __RW t_dbi_vflow_con    VFLOW_CON       ;
    __RW t_dbi_port_con     PORT_CON        ;
} EVA_DBI_typedef;

//========================================================================================
//
//
//  end of this file
//
//
//========================================================================================
#endif


