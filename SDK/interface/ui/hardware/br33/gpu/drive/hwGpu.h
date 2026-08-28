#ifndef __HWGPU_H
#define __HWGPU_H

//sfr
typedef struct {
    volatile unsigned int lay_done_pnd : 1;
        volatile unsigned int win_done_pnd : 1;
        volatile unsigned int break_pnd : 1;
        volatile unsigned int pause_pnd : 1;
        volatile unsigned int rle_err_pnd : 1;
        volatile unsigned int wcnt_ov_pnd : 1;
        volatile unsigned int ecnt_ov_pnd : 1;
        volatile unsigned int sph_done_pnd : 1;
        volatile unsigned int gb_trans_err_pnd : 1;
        volatile unsigned int top_down_pnd : 1;
        volatile unsigned int unuse0 : 22;
    } PND_CON;

    typedef struct {
    volatile unsigned int lay_done_clr : 1;
        volatile unsigned int win_done_clr : 1;
        volatile unsigned int break_clr : 1;
        volatile unsigned int pause_clr : 1;
        volatile unsigned int rle_err_clr : 1;
        volatile unsigned int wcnt_ov_clr : 1;
        volatile unsigned int ecnt_ov_clr : 1;
        volatile unsigned int sph_done_clr : 1;
        volatile unsigned int gb_trans_err_clr : 1;
        volatile unsigned int top_down_clr : 1;
        volatile unsigned int unuse0 : 22;
    } PND_CLR;

    typedef struct {
    volatile unsigned int lay_done_ie : 1;
        volatile unsigned int win_done_ie : 1;
        volatile unsigned int break_ie : 1;
        volatile unsigned int pause_ie : 1;
        volatile unsigned int rle_err_ie : 1;
        volatile unsigned int wcnt_ov_ie : 1;
        volatile unsigned int ecnt_ov_ie : 1;
        volatile unsigned int sph_done_ie : 1;
        volatile unsigned int gb_trans_err_ie : 1;
        volatile unsigned int top_down_ie : 1;
        volatile unsigned int unuse0 : 22;
    } PND_IE;

    typedef struct {
    volatile unsigned int kick_start : 1;
        volatile unsigned int set_pause : 1;
        volatile unsigned int status : 2;
        volatile unsigned int scan_opt_ca : 2;
        volatile unsigned int scan_opt_re : 2;
        volatile unsigned int scan_size : 2;
        volatile unsigned int unuse0 : 1;
        volatile unsigned int fg_mask_en : 1;
        volatile unsigned int unuse1 : 16;
        volatile unsigned int scan_key : 4;
    } STA_CON;

    typedef struct {
    volatile unsigned int color_fmt : 5;
        volatile unsigned int big_end : 1;
        volatile unsigned int alpna_end : 1;
        volatile unsigned int rb_swap : 1;
        volatile unsigned int unuse0 : 24;
    } OUT_FMT;

    typedef struct {
    volatile unsigned int win_x_min : 12;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int win_x_max : 12;
        volatile unsigned int unuse1 : 4;
    } WIN_X;

    typedef struct {
    volatile unsigned int win_y_min : 12;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int win_y_max : 12;
        volatile unsigned int unuse1 : 4;
    } WIN_Y;


    typedef struct {
    volatile unsigned int out_stride : 15;
        volatile unsigned int unuse0 : 17;
    } OUT_STRIDE;

    typedef struct {
    volatile unsigned int win_base_adr : 32;
    } WIN_BASE;


    typedef struct {
    volatile unsigned int next_layer_cnt : 11;
        volatile unsigned int unuse0 : 21;
    } NEXT_LAYER_CNT;

    typedef struct {
    volatile unsigned int curr_layer_cnt : 11;
        volatile unsigned int unuse0 : 21;
    } CURR_LAYER_CNT;

    typedef struct {
    volatile unsigned int next_inst_base_adr : 32;
    } NEXT_INST_INFO;

    typedef struct {
    volatile unsigned int curr_inst_base_adr : 32;
    } CURR_INST_INFO;

    typedef struct {
    volatile unsigned int colorkey_b : 8;
        volatile unsigned int colorkey_g : 8;
        volatile unsigned int colorkey_r : 8;
        volatile unsigned int unuse0 : 8;
    } COLOR_KEY;

    typedef struct {
    volatile unsigned int uv_base_adr : 32;
    } UV_BASE_ADR;

    typedef struct {
    volatile unsigned int vtx0_cnt : 26;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int vtx0_cnt_ov : 1;
        volatile unsigned int vtx0_cnt_clr : 1;
    } VTX0_TCNT;

    typedef struct {
    volatile unsigned int vtx1_cnt : 26;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int vtx1_cnt_ov : 1;
        volatile unsigned int vtx1_cnt_clr : 1;
    } VTX1_TCNT;


    typedef struct {
    volatile unsigned int frag_cnt : 26;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int frag_cnt_ov : 1;
        volatile unsigned int frag_cnt_clr : 1;
    } FRAG_TCNT;

    typedef struct {
    volatile unsigned int mark_cnt : 26;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int mark_cnt_ov : 1;
        volatile unsigned int mark_cnt_clr : 1;
    } MARK_TCNT;

    typedef struct {
    volatile unsigned int rast_cnt : 26;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int rast_cnt_ov : 1;
        volatile unsigned int rast_cnt_clr : 1;
    } RAST_TCNT;

    typedef struct {
    volatile unsigned int max_msaa_lvl : 2;
        volatile unsigned int max_bez_div : 3;
        volatile unsigned int cbuf_split : 3;
        volatile unsigned int full_frag : 1;
        volatile unsigned int unuse0 : 7;
        volatile unsigned int cbuf_stride : 12;
        volatile unsigned int unuse1 : 4;
    } VTX_CON0;

    typedef struct {
    volatile unsigned int max_edge_cfg : 13;
        volatile unsigned int unuse0 : 3;
        volatile unsigned int max_edge_record : 13;
        volatile unsigned int unuse1 : 2;
        volatile unsigned int max_edge_clr : 1;
    } VTX_CON1;

    typedef struct {
    volatile unsigned int ebuf_base : 32;
    } EBUF_BASE;

    typedef struct {
    volatile unsigned int cbuf_base : 32;
    } CBUF_BASE;

    typedef struct {
    volatile unsigned int gb_trans_en : 1;
        volatile unsigned int unuse0 : 31;
    } GB_TRANS_CON;

    typedef struct {
    float GB_FW_M00;
    float GB_FW_M01;
    float GB_FW_M02;

    float GB_FW_M10;
    float GB_FW_M11;
    float GB_FW_M12;

    float GB_FW_M20;
    float GB_FW_M21;
    float GB_FW_M22;
} GB_FW_MAT;

typedef struct {
    float GB_BW_M00;
    float GB_BW_M01;
    float GB_BW_M02;

    float GB_BW_M10;
    float GB_BW_M11;
    float GB_BW_M12;

    float GB_BW_M20;
    float GB_BW_M21;
    float GB_BW_M22;
} GB_BW_MAT;

typedef struct {
    PND_CON pnd_con;
    PND_CLR pnd_clr;
    PND_IE pnd_ie;
    unsigned int RESERVED_03;
    STA_CON sta_con;

    OUT_FMT out_fmt;
    WIN_X win_x;
    WIN_Y win_y;
    OUT_STRIDE out_stride;
    WIN_BASE win_base;

    NEXT_LAYER_CNT next_layer_cnt;
    CURR_LAYER_CNT curr_layer_cnt;
    NEXT_INST_INFO next_inst_info;
    CURR_INST_INFO curr_inst_info;
    COLOR_KEY      colorkey;
    UV_BASE_ADR    uv_base_adr;

    VTX0_TCNT       vtx0_tcnt;
    VTX1_TCNT       vtx1_tcnt;
    FRAG_TCNT       frag_tcnt;
    MARK_TCNT       mark_tcnt;
    RAST_TCNT       rast_tcnt;


    VTX_CON0       vtx_con0;
    VTX_CON1       vtx_con1;
    EBUF_BASE      ebuf_base;
    CBUF_BASE      cbuf_base;

    GB_TRANS_CON   gb_trans_con;
    GB_FW_MAT      gb_fw_mat;
    GB_BW_MAT      gb_bw_mat;
} JLGPU_SFR_TypeDef;

typedef struct {
    OUT_FMT out_fmt;
    WIN_X win_x;
    WIN_Y win_y;
    OUT_STRIDE out_stride;
    WIN_BASE win_base;
} JLGPU_SFR_WIN_TypeDef;

typedef struct {
    GB_TRANS_CON   gb_trans_con;
    GB_FW_MAT      gb_fw_mat;
    GB_BW_MAT      gb_bw_mat;
} JLGPU_SFR_TRANS_TypeDef;

typedef struct {
    COLOR_KEY      colorkey;
} JLGPU_SFR_COLORKEY_TypeDef;

typedef struct {
    UV_BASE_ADR    uv_base_adr;
} JLGPU_SFR_UVADR_TypeDef;

typedef struct {
    VTX_CON0       vtx_con0;
    VTX_CON1       vtx_con1;
    EBUF_BASE      ebuf_base;
    CBUF_BASE      cbuf_base;
} JLGPU_SFR_VTX_TypeDef;

//add GX_DEC
typedef struct {
    volatile unsigned int img_base_adr : 32;
    } IMG_BASE_ADR;

    typedef struct {
    volatile unsigned int mmu_en : 1;
        volatile unsigned int mmu_page_size : 2;
        volatile unsigned int unused : 29;
    } MMU_CON;

    typedef struct {
    volatile unsigned int mmu_tbl_adr : 32;
    } MMU_TBL_ADR;

    typedef struct {
    IMG_BASE_ADR    img_base_adr;
    MMU_CON         mmu_con;
    MMU_TBL_ADR     mmu_tbl_adr;
} JLGPU_EXTEND_COMMAND_GXDEC_TypeDef;


//sph
typedef struct {
    volatile unsigned int sph_kick : 1;
        volatile unsigned int sph_en : 1;
        volatile unsigned int sph_mode : 1;
        volatile unsigned int unuse : 29;
    } SPH_CON;

    typedef struct {
    volatile unsigned int sph_rd_bytes : 14;
        volatile unsigned int unuse0 : 2;
        volatile unsigned int sph_wr_bytes : 14;
        volatile unsigned int unuse1 : 2;
    } SPH_INFO;

    typedef struct {
    volatile unsigned int sph_rd_adr : 32;
    } SPH_RD_ADR;

    typedef struct {
    volatile unsigned int sph_wr_adr : 32;
    } SPH_WR_ADR;

    typedef struct {
    float sph_m00;
    float sph_m01;
    float sph_m02;
    float sph_m03;

    float sph_m10;
    float sph_m11;
    float sph_m12;
    float sph_m13;

    float sph_m20;
    float sph_m21;
    float sph_m22;
    float sph_m23;
} SPH_MATRIX;

typedef struct {
    SPH_CON      sph_con;
    SPH_INFO     sph_info;
    SPH_RD_ADR   sph_rd_adr;
    SPH_WR_ADR   sph_wr_adr;
    SPH_MATRIX   sph_matrix;
} JLGPU_SPH_MATRIX_TypeDef;

//top
typedef struct {
    volatile unsigned int top_kick : 1;
        volatile unsigned int vbus_clean : 1;
        volatile unsigned int set_pause : 1;
        volatile unsigned int top_status : 3;
        volatile unsigned int vbus_busy : 1;
        volatile unsigned int unuse : 25;
    } TOP_CON;

    typedef struct {
    volatile unsigned int top_w : 12;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int top_h : 12;
        volatile unsigned int unuse1 : 4;
    } TOP_INFO;

    typedef struct {
    volatile unsigned int top_x_ofs : 12;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int top_y_ofs : 12;
        volatile unsigned int unuse1 : 4;
    } TOP_OFS;

    typedef struct {
    volatile unsigned int buf_w : 12;
        volatile unsigned int unuse0 : 4;
        volatile unsigned int buf_h : 12;
        volatile unsigned int unuse1 : 4;
    } BUF_INFO;

    typedef struct {
    volatile unsigned int buf_base0 : 32;
    } BUF_BASE0;

    typedef struct {
    volatile unsigned int buf_base1 : 32;
    } BUF_BASE1;

    typedef struct {
    TOP_CON      top_con;
    TOP_INFO     top_info;
    TOP_OFS      top_ofs;
    BUF_INFO     buf_info;
    BUF_BASE0    buf_base0;
    BUF_BASE1    buf_base1;
} JLGPU_TOP_INFO_TypeDef;

///////////////////////////////////////
//cmd
typedef struct {
    //0
    union {
        struct {
            volatile unsigned int inst_mode : 1;
                volatile unsigned int layer_en : 1;
                volatile unsigned int breakpoint_en : 1;
                volatile unsigned int fg_mode : 3;
                volatile unsigned int blend_mode : 4;
                volatile unsigned int mask_mode : 2;
                volatile unsigned int pnt_trans_mode : 2;
                volatile unsigned int tex_trans_mode : 2;
                volatile unsigned int vtx_trans_mode : 2;
                volatile unsigned int dither_en : 1;
                volatile unsigned int gb_trans_dis : 1;
                volatile unsigned int rgb_limit_en : 1;
                volatile unsigned int msaa_sel : 2;
                volatile unsigned int unused0 : 1;
                volatile unsigned int global_alpha : 8;
            };
            struct {
            volatile unsigned int inst_mode_ : 1;
                volatile unsigned int layer_en_ : 1;
                volatile unsigned int breakpoint_en_ : 1;
                volatile unsigned int command : 3;
                volatile unsigned int reserved : 26;
            };
        };

        //1
        volatile unsigned int next_inst_adr : 32;

        //2
        struct {
        volatile unsigned int bbox_x_min : 12;
            volatile unsigned int unuse0 : 4;
            volatile unsigned int bbox_x_max : 12;
            volatile unsigned int unuse1 : 4;
        };

        //3
        struct {
        volatile unsigned int bbox_y_min : 12;
            volatile unsigned int unuse2 : 4;
            volatile unsigned int bbox_y_max : 12;
            volatile unsigned int unuse3 : 4;
        };

        //4
        struct {
        volatile unsigned int seg_cmd_len : 16;
            volatile unsigned int unuse4 : 16;
        };

        //5
        volatile unsigned int seg_cmd_adr : 32;
    } JLGPU_CMD_TypeDef;



//paint
    typedef struct {
    //0
    struct {
        volatile unsigned int	paint_type : 3;
            volatile unsigned int	spread_mode : 3;
            volatile unsigned int	reserved : 1;
            volatile unsigned int	premult : 1;
            volatile unsigned int	mmu_clut_en : 1;
            volatile unsigned int	mmu_page_size : 2;
            volatile unsigned int	clut_level : 4;
            volatile unsigned int	clut_format : 3;
            volatile unsigned int	sample_mode : 1;
            volatile unsigned int   unuse0 : 13;
        };

        //1
        union {
                struct {
                    volatile unsigned int clut_adr : 32;
                };

                struct {
        volatile unsigned int fill_color_b : 8;
            volatile unsigned int fill_color_g : 8;
            volatile unsigned int fill_color_r : 8;
            volatile unsigned int fill_color_a : 8;
        };
        };

        //2
        volatile unsigned int	    mmu_tb_base : 32;

        //3
        union {
                struct {
                    volatile unsigned int exter_radius : 16;
                    volatile unsigned int inside_radius : 16;
                };

                struct {
        volatile unsigned int theta_offs : 14;
            volatile unsigned int unuse1 : 18;
        };
        };

        //4
        union {
                struct {
                    volatile unsigned int inside_width : 16;
                    volatile unsigned int inside_high : 16;
                };

                struct {
        volatile unsigned int theta_gain : 19;
            volatile unsigned int unuse2 : 13;
        };
        };

        //5
        struct {
        volatile int clip_x_min : 13;
            volatile unsigned int unuse3 : 3;
            volatile int clip_x_max : 13;
            volatile unsigned int unuse4 : 3;
        };

        //6
        struct {
        volatile int clip_y_min : 13;
            volatile unsigned int unuse5 : 3;
            volatile int clip_y_max : 13;
            volatile unsigned int unuse6 : 3;
        };

        //7
        struct {
        volatile unsigned int smp_prm0 : 14;
            volatile unsigned int unuse7 : 2;
            volatile unsigned int smp_prm1 : 14;
            volatile unsigned int unuse8 : 2;
        };

    } JLGPU_FragPaint_Basic;

    typedef struct {
    float paint_m00;
    float paint_m01;
    float paint_m02;

    float paint_m10;
    float paint_m11;
    float paint_m12;
} JLGPU_FragPaint_Trans1;

typedef struct {
    float paint_m20;
    float paint_m21;
    float paint_m22;
} JLGPU_FragPaint_Trans2;

//typedef struct
//{
//    JLGPU_FragPaint_Basic  fragfaint_basic;
//    JLGPU_FragPaint_Trans1 fragpaint_trans1;
//    JLGPU_FragPaint_Trans2 fragpaint_trans2;
//}JLGPU_FragPaint_TypeDef;

//texture
typedef struct {
    //0
    struct {
        volatile unsigned int color_fmt : 5;
            volatile unsigned int compress_mode : 2;
            volatile unsigned int adr_mode : 1;
            volatile unsigned int big_end : 1;
            volatile unsigned int alpha_end : 1;
            volatile unsigned int rb_swap : 1;
            volatile unsigned int color_ext : 1;
            volatile unsigned int premult : 1;
            volatile unsigned int boundary_ext : 1;
            volatile unsigned int colorkey_en : 1;
            volatile unsigned int clut_format : 3;
            volatile unsigned int mmu_tex_en : 1;
            volatile unsigned int mmu_clut_en : 1;
            volatile unsigned int mmu_page_size : 2;
            volatile unsigned int unused0 : 10;
        };

        //1
        volatile unsigned int mmu_tb_base : 32;

        //2
        struct {
        volatile unsigned int tex_stride : 18;
            volatile unsigned int unused1 : 14;
        };

        //3
        volatile unsigned int tex_base_adr : 32;

        //4
        struct {
        volatile unsigned int rle_limit_adr_l : 16;//tex_w
            volatile unsigned int rle_limit_adr_m : 16;//tex_h
        };

        //5
        struct {
        volatile unsigned int clip_x_min : 12;
            volatile unsigned int unuse2 : 4;
            volatile unsigned int clip_x_max : 12;
            volatile unsigned int unuse3 : 4;
        };

        //6
        struct {
        volatile unsigned int clip_y_min : 12;
            volatile unsigned int unuse4 : 4;
            volatile unsigned int clip_y_max : 12;
            volatile unsigned int unuse5 : 4;
        };

        //7
        //volatile unsigned int	    clut_adr : 32;//ax_color
        union {
                struct {
                    volatile unsigned int clut_adr : 32;
                };

                struct {
        volatile unsigned int ax_color_b : 8;
            volatile unsigned int ax_color_g : 8;
            volatile unsigned int ax_color_r : 8;
            volatile unsigned int ax_color_a : 8;
        };
        };

    } JLGPU_FragImage_Basic;


    typedef struct {
    //0
    struct {
        volatile unsigned int smp_mode : 3;
            volatile unsigned int unuse0 : 1;
            volatile unsigned int smp_shift : 4;
            volatile unsigned int smp_prm0 : 6;
            volatile unsigned int unuse1 : 2;
            volatile unsigned int smp_prm1 : 6;
            volatile unsigned int unuse2 : 2;
            volatile unsigned int smp_prm2 : 6;
            volatile unsigned int unuse3 : 2;
        };

        //1
        struct {
        volatile unsigned int smp_prm3 : 6;
            volatile unsigned int unuse4 : 2;
            volatile unsigned int smp_prm4 : 6;
            volatile unsigned int unuse5 : 2;
            volatile unsigned int smp_prm5 : 6;
            volatile unsigned int unuse6 : 10;
        };

        float tex_m00;
        float tex_m01;
        float tex_m02;

        float tex_m10;
        float tex_m11;
        float tex_m12;

    } JLGPU_FragImage_Trans1;

    typedef struct {
    float tex_m20;
    float tex_m21;
    float tex_m22;
} JLGPU_FragImage_Trans2;

//seg

typedef struct {
    volatile unsigned int fill_rule : 1;
        volatile unsigned int unuse0 : 1;
        volatile unsigned int msaa_lvl : 2;
        volatile unsigned int max_bez_div : 4;
        volatile unsigned int unuse1 : 24;
    } JLGPU_Segment_Basic;

    typedef struct {
    float seg_m00;
    float seg_m01;
    float seg_m02;

    float seg_m10;
    float seg_m11;
    float seg_m12;
} JLGPU_Segment_Trans1;

typedef struct {
    float seg_m20;
    float seg_m21;
    float seg_m22;
} JLGPU_Segment_Trans2;

//typedef struct
//{
//    volatile unsigned int seg_byte0 : 8;
//    volatile unsigned int seg_byte1 : 8;
//    volatile unsigned int seg_byte2 : 8;
//    volatile unsigned int seg_byte3 : 8;
//
//}JLGPU_Segment_Path;


//mask
typedef struct {
    //0
    struct {
        volatile unsigned int mask_inv : 1;
            volatile unsigned int mask_format : 3;
            volatile unsigned int mask_big_end : 1;
            volatile unsigned int unuse0 : 11;
            volatile unsigned int mask_stride : 12;
            volatile unsigned int unuse1 : 4;
        };

        //1
        volatile unsigned int mask_adr : 32;

        //2
        struct {
        volatile unsigned int mask_x_min : 12;
            volatile unsigned int unuse2 : 4;
            volatile unsigned int mask_x_max : 12;
            volatile unsigned int unuse3 : 4;
        };

        //3
        struct {
        volatile unsigned int mask_y_min : 12;
            volatile unsigned int unuse4 : 4;
            volatile unsigned int mask_y_max : 12;
            volatile unsigned int unuse5 : 4;
        };
    } JLGPU_Mask_Basic;

#endif
