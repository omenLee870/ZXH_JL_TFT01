#ifndef __JLGPU_FRAMEWORK_H__
#define __JLGPU_FRAMEWORK_H__

#include "jlgpu_math.h"
#include "jlgpu_hal.h"

typedef void jlgpu_t;

typedef struct {
    int(*gpu_init_hal)();//init
    int(*gpu_uninit_hal)();//uninit
    void(*gpu_run_hal)(void *task_base);//run
} jlgpu_ops_t;


typedef struct {
    uint8_t clut_en;//lut表的mmu
    uint8_t tex_en;//tex base的mmu
    uint8_t page_size;//0:4K 1:16K 2:32K 3:64K
    uint8_t *tb_base;//table基地址,word对齐
} jlgpu_mmu_t;

typedef struct {
    int16_t x_min;//区域x最小值
    int16_t x_max;//区域x最大值
    int16_t y_min;//区域y最小值
    int16_t y_max;//区域y最大值
} jlgpu_area_t;

typedef struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
} jlgpu_color_t;

typedef struct {
    jlgpu_point2_t point_start;//起始点
    jlgpu_point2_t point_end;//结束点
    jlgpu_spread_mode_t spread_mode;//渐变延申方式
} jlgpu_lineargrad_param_t; //线性渐变,从起始点到结束点的渐变

typedef struct {
    jlgpu_point2_t point_center;//中心点
    int out_radius;//外径
    int in_radius;//内径,外径大于内径
    jlgpu_spread_mode_t spread_mode;
} jlgpu_radialgrad_param_t; //径向渐变,从内径到外径的渐变

typedef struct {
    jlgpu_point2_t point_center;//中心点
    int out_radius;//外径
    int in_radius;//内径,外径大于内径
    int in_width;//圆角矩形内部宽度
    int in_height;//圆角矩形内部高度
} jlgpu_roundgrad_param_t; //圆角矩形,从内径到外径的渐变

typedef struct {
    jlgpu_point2_t point_center;////中心点
    int out_radius;//外径
    int in_radius;//内径,外径大于内径
} jlgpu_ringgrad_param_t; //圆环,从内径到外径的渐变

typedef struct {
    jlgpu_point2_t point_center;//中心点
    int start_angle;//起始角度 0-360
    int end_angle;//结束角度 0-360,需大于起始角度
} jlgpu_anglegrad_param_t; //角度渐变,从起始角度到结束角度的渐变


typedef struct {
    jlgpu_premult_mode_t premult;//预乘开关
    jlgpu_area_t pt_clip;//有效区域
} jlgpu_paint_info_t;

typedef struct {
    jlgpu_pt_samplemode_t inter_mode;//fill,grad插值方式，一般取JLGPU_SAMPLE_MODE_X1
    uint16_t coeff0;//插值方式为JLGPU_SAMPLE_MODE_X4时候的系数
    uint16_t coeff1;//插值方式为JLGPU_SAMPLE_MODE_X4时候的系数
} jlgpu_paint_inter_params_t; //jlgpu_render_param_t中的render_quality为0时才有效

typedef struct {
    jlgpu_tex_samplemode_t inter_mode;//tex插值方式
    uint8_t right_shift;//右移参数
    uint8_t coeff0;//滤波系数
    uint8_t coeff1; //3x3, 5x5时候需要配置系数和right_shift
    uint8_t coeff2;
    uint8_t coeff3;
    uint8_t coeff4;
    uint8_t coeff5;
} jlgpu_tex_inter_params_t; //jlgpu_render_param_t中的render_quality为0时才有效

typedef struct {
    jlgpu_tex_format_t color_format;//tex颜色格式
    jlgpu_adr_mode_t adr_mode;//line or tile
    jlgpu_alpha_end_t alpha_end;//ALPHA位置
    jlgpu_rbs_t rb_swap;//R,B交换
    jlgpu_endian_t big_end;//大小端
    uint8_t *data;//tex数据
    uint32_t width;//图像宽
    uint32_t height;//图像高
    jlgpu_premult_mode_t premult;//预乘开关
    uint32_t compress_size;//rle压缩时需要配置
    jlgpu_compress_mode_t compress_mode;//压缩方式
    jlgpu_area_t tex_clip;//tex有效区域
} jlgpu_img_info_t;

typedef struct {
    uint8_t layer_en;//图层开关
    uint8_t breakpoint_en;//断点开关
    jlgpu_blendmode_t blend_mode;//混合方式，支持11种
    uint8_t dither_en;//dither开关
    uint8_t gb_trans_en;//图层全局变换开关
    uint8_t rgb_limit_en;//RGB是否截断到A
    jlgpu_render_quality_t render_quality;//tex,fill和grad的渲染质量,在有变换时用
    uint8_t global_alpha;//全局ALPHA
    jlgpu_area_t render_area;//渲染区域
} jlgpu_render_param_t;


typedef struct {
    uint8_t                 layer_en;//图层开关
    uint8_t                 breakpoint_en;//断点开关
    jlgpu_extend_command_t  command;//扩展命令,支持6种扩展
    union {
        void *command_surface;//指向jlgpu_surface_t
        void *command_global_matrix;//指向jlgpu_global_matrix_t
        void *command_colorkey;//指向jlgpu_colorkey_t
        void *command_uv_adr;//指向jlgpu_uv_adr_t
        void *command_gxdec;//指向jlgpu_gxdec_t
    };
} jlgpu_extend_param_t;


typedef struct {
    jlgpu_paint_info_t paint_info;//fill预乘，有效区域
    jlgpu_color_t color;//颜色
    jlgpu_trans_mode_t fill_trans_mode;//fill的变换方式
    //fill的插值参数，存在变换时，需要配置(render_quality存在时，可以不配)
    jlgpu_paint_inter_params_t *fill_inter_params;
    jlgpu_matrix_t *surface2paint;//fill的变换矩阵，存在变换时，必须给矩阵
} jlgpu_draw_fill_t;

typedef struct {
    jlgpu_paint_info_t paint_info;//查找表预乘,grad有效区域
    jlgpu_grad_mode_t grad_mode;//渐变方式，支持5种渐变
    uint8_t clut_level; //查找表颜色数量，2^clut_level + 1,最大值为8;颜色越多，精度越高
    jlgpu_clut_format_t clut_format;//查找表格式
    uint8_t *clut_adr;//颜色查找表地址
    jlgpu_mmu_t pt_mmu;//grad的mmu配置
    union {
        void *lineargrad_ptr;//指向jlgpu_lineargrad_param_t
        void *radialgrad_ptr;//指向jlgpu_radialgrad_param_t
        void *roundgrad_ptr;//指向jlgpu_roundgrad_param_t
        void *ringgrad_ptr;//指向jlgpu_ringgrad_param_t
        void *anglegrad_ptr;//指向jlgpu_anglegrad_param_t
    };
    jlgpu_trans_mode_t grad_trans_mode;//grad的变换方式，不能为0
    //grad的插值参数，存在变换时,需要配置(render_quality存在时，可以不配)
    jlgpu_paint_inter_params_t *grad_inter_params;
    jlgpu_matrix_t *surface2paint;//grad的变换矩阵，存在变换时，必须给矩阵
} jlgpu_draw_grad_t;


typedef struct {
    jlgpu_img_info_t img_info;//图像的宽，高，格式，压缩方式，预乘排列方式，区域等信息
    jlgpu_color_extend_t color_ext;//低比特颜色向高比特扩展时的低位扩展方式;0:高位填充 1:0填充
    jlgpu_boundary_extend_t boundary_ext;//边界扩展方式;0:0像素填充 1:临近像素填充
    uint8_t colorkey_en;//colorkey使能
    jlgpu_clut_format_t clut_format;//颜色查找表格式
    jlgpu_mmu_t tex_mmu;//tex的mmu配置
    uint8_t *clut_adr;//颜色查找表地址
    jlgpu_color_t ax_color;//A格式的颜色
    jlgpu_trans_mode_t tex_trans_mode;//tex的变换方式
    //tex的插值参数，存在变换时，需要配置(render_quality存在时，可以不配)
    jlgpu_tex_inter_params_t *tex_inter_params;
    jlgpu_matrix_t *surface2image;//tex的变换矩阵，存在变换时，必须给矩阵
} jlgpu_draw_img_t;

typedef struct {
    jlgpu_draw_fill_t draw_fill;//纯色填充
    jlgpu_draw_img_t draw_img;//纹理填充
    jlgpu_paint_mode_t paint_mode;//叠加方式
} jlgpu_draw_img_fill_t;

typedef struct {
    jlgpu_draw_grad_t draw_grad;//渐变色填充
    jlgpu_draw_img_t draw_img;//纹理填充
    jlgpu_paint_mode_t paint_mode;//叠加方式
} jlgpu_draw_img_grad_t;

typedef struct {
    jlgpu_draw_grad_t draw_lineargrad;//grad_mode必须为LINEAR_GRADIENT
    jlgpu_draw_grad_t draw_radialgrad;//grad_mode必须为RADIAL_GRADIENT/ROUND_GRADIENT/RING_GRADIENT
} jlgpu_draw_lineargrad_radialgrad_t; //draw_lineargrad和draw_radialgrad的trans_mode要一致

typedef struct {
    jlgpu_draw_grad_t draw_radialgrad;//grad_mode必须为RADIAL_GRADIENT/ROUND_GRADIENT/RING_GRADIENT
    jlgpu_draw_grad_t draw_anggrad;//grad_mode必须为ANGLE_GRADIENT
} jlgpu_draw_radialgrad_anggrad_t; //draw_radialgrad和draw_anggrad的trans_mode要一致

typedef struct {
    uint8_t                     mask_inv;//mask取反
    jlgpu_mask_format_t         mask_format;//mask格式
    jlgpu_endian_t              mask_big_end;//mask大小端
    uint32_t                    width;//mask宽度
    jlgpu_area_t                mask_clip;//mask区域
    uint8_t                    *mask_data;//mask地址
} jlgpu_mask_t;

typedef struct {
    jlgpu_fillrule_t                    fill_rule;//填充规则
    jlgpu_render_quality_t              msaa_lvl;//path的抗锯齿等级
    uint8_t max_bez_div;  //曲线分段数,最大为7;分段数为1<<(max_bez_div+1)
    uint8_t *seg_data;      //seg数据
    uint16_t seg_len;       //seg长度
    jlgpu_trans_mode_t path_trans_mode;//path变换方式
    jlgpu_matrix_t *user2surface;//path变换矩阵，变换方式存在时，矩阵必须存在
    //uint8_t div_en;//计算最优分段数使能
    //float div_scale;//分段数缩放因子
} jlgpu_path_t;

typedef struct {
    jlgpu_mask_mode_t mask_mode;//蒙版方式，0:没有mask,1:path,2:mask
    union {
        void *mask_ptr;//指向jlgpu_mask_t,mask_mode = 2
        void *path_ptr;//指向jlgpu_path_t,mask_mode = 1
    };
} jlgpu_mask_param_t;

//sfr/ extend
typedef struct {
    uint8_t *data;//bg数据
    uint32_t width;//bg宽度
    jlgpu_out_format_t format;//颜色格式
    jlgpu_rbs_t rb_swap;//RB交换
    jlgpu_alpha_end_t alpha_end;//ALPHA位置
    jlgpu_endian_t big_end;//大小端,RGB565,A4,A2,A1格式支持
    jlgpu_area_t window;//绘制区域
} jlgpu_surface_t;

typedef struct {
    uint8_t trans_en;//全局变换开关
    jlgpu_matrix_t fw_mat;//正向矩阵
    jlgpu_matrix_t bw_mat;//逆向矩阵
} jlgpu_global_matrix_t;

typedef struct {
    uint8_t colorkey_b;
    uint8_t colorkey_g;
    uint8_t colorkey_r;
} jlgpu_colorkey_t;

typedef struct {
    uint8_t    *uv_data;//tex 格式为YUV420 line_base时,uv地址
} jlgpu_uv_adr_t;

typedef struct {
    uint8_t *img_base;//文件首地址
    uint8_t mmu_en;//mmu开关
    jlgpu_page_size_t page_size;//0:4K 1:16K 2:32K 3:64K
    uint8_t *tb_base;//table基地址
} jlgpu_gxdec_t;

typedef struct {
    jlgpu_render_quality_t msaa_lvl;//全局path最大抗锯齿等级
    uint8_t max_bez_div;//全局曲线分段数,最大为7;分段数为1<<(max_bez_div+1)
    jlgpu_cbuf_split_t cbuf_split;//分行处理，0:32(32*256) 1:16行(16*256) 2:16行(16*512)
    uint8_t full_frag;
    uint16_t cbuf_stride;//cbuf步长,cbuf宽度/2
    uint16_t max_edge_cfg;//最大edge数
    uint8_t *e_data;//edge内存,一个edge占2个word
    uint8_t *c_data;//cbuf内存,最小内存为cbuf_stride*h(分行后的行数)
    uint8_t clear_edge_en;//清空硬件记录的edge
} jlgpu_global_path_t;


typedef struct {
    jlgpu_sph_mode_t sph_mode;
    int16_t sph_wr_bytes;
    int16_t sph_rd_bytes;
    float *sph_rd_adr;
    float *sph_wr_adr;
    float m[3][4];
} jlgpu_sph_matrix_params_t; //sph

typedef struct {
    uint8_t *buf0;
    uint8_t *buf1;
    uint32_t buf_width;//buf宽度
    uint32_t buf_height;//buf宽度
    jlgpu_area_t window;//绘制区域
} jlgpu_block_render_params_t;

void jlgpu_computeLinearParameters(uint8_t lut_lvl, jlgpu_lineargrad_param_t *linegrad_params, jlgpu_matrix_t *grad_matrix);
void jlgpu_computeRadialParameters(uint8_t lut_lvl, jlgpu_radialgrad_param_t *radgrad_params, jlgpu_matrix_t *grad_matrix);
void jlgpu_computeRoundParameters(uint8_t lut_lvl, jlgpu_roundgrad_param_t *roundgrad_params, jlgpu_matrix_t *grad_matrix);
void jlgpu_computeRingParameters(uint8_t lut_lvl, jlgpu_ringgrad_param_t *ringgrad_params, jlgpu_matrix_t *grad_matrix);
void jlgpu_computeAngularParameters(uint8_t lut_lvl, jlgpu_anglegrad_param_t *anglegrad_params, jlgpu_matrix_t *grad_matrix);

//计算纯色填充TASK长度
uint8_t jlgpu_draw_fill_task_len(jlgpu_t *ctx, jlgpu_draw_fill_t *draw_fill, jlgpu_mask_param_t *mask_params);
//生成纯色填充的TASK
void jlgpu_draw_fill(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_fill_t *draw_fill, jlgpu_mask_param_t *mask_params);
//计算渐变色填充TASK长度
uint8_t jlgpu_draw_grad_task_len(jlgpu_t *ctx, jlgpu_draw_grad_t *draw_grad, jlgpu_mask_param_t *mask_params);
//生成渐变色填充的TASK
void jlgpu_draw_grad(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_grad_t *draw_grad, jlgpu_mask_param_t *mask_params);
//计算纹理填充TASK长度
uint8_t jlgpu_draw_img_task_len(jlgpu_t *ctx, jlgpu_draw_img_t *draw_img, jlgpu_mask_param_t *mask_params);
//生成纹理填充的TASK
void jlgpu_draw_img(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_img_t *draw_img, jlgpu_mask_param_t *mask_params);
//计算纯色填充叠加纹理填充的TASK长度
uint8_t jlgpu_draw_img_fill_task_len(jlgpu_t *ctx, jlgpu_draw_img_fill_t *draw_img_fill, jlgpu_mask_param_t *mask_params);
//生成纯色填充叠加纹理填充的TASK
void jlgpu_draw_img_fill(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_img_fill_t *draw_img_fill, jlgpu_mask_param_t *mask_params);
//计算渐变色填充叠加纹理填充的TASK长度
uint8_t jlgpu_draw_img_grad_task_len(jlgpu_t *ctx, jlgpu_draw_img_grad_t *draw_img_grad, jlgpu_mask_param_t *mask_params);
//生成渐变色填充叠加纹理填充的TASK
void jlgpu_draw_img_grad(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_img_grad_t *draw_img_grad, jlgpu_mask_param_t *mask_params);
//计算线性渐变叠加径向渐变填充的TASK长度
uint8_t jlgpu_draw_lineargrad_radialgrad_task_len(jlgpu_t *ctx, jlgpu_draw_lineargrad_radialgrad_t *jlgpu_lineargrad_radialgrad, jlgpu_mask_param_t *mask_params);
//生成线性渐变叠加径向渐变填充的TASK
void jlgpu_draw_lineargrad_radialgrad(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_lineargrad_radialgrad_t *jlgpu_lineargrad_radialgrad, jlgpu_mask_param_t *mask_params);
//计算径向渐变叠加角度渐变填充的TASK长度
uint8_t jlgpu_draw_radialgrad_anglegrad_task_len(jlgpu_t *ctx, jlgpu_draw_radialgrad_anggrad_t *jlgpu_radialgrad_anggrad, jlgpu_mask_param_t *mask_params);
//生成径向渐变叠加角度渐变填充的TASK
void jlgpu_draw_radialgrad_anglegrad(jlgpu_t *ctx, jlgpu_render_param_t *render_params, jlgpu_draw_radialgrad_anggrad_t *jlgpu_radialgrad_anggrad, jlgpu_mask_param_t *mask_params);

//计算扩展命令的TASK长度
uint8_t jlgpu_extend_command_task_len(jlgpu_t *ctx, jlgpu_extend_param_t *extend_params);
//生成扩展命令的TASK
void jlgpu_extend_command(jlgpu_t *ctx, jlgpu_extend_param_t *extend_params);


int get_jlgpu_context_size();
jlgpu_t *jlgpu_create_context(void *buf);
void jlgpu_destroy_context(jlgpu_t *ctx);
void jlgpu_set_task(jlgpu_t *ctx, void *task, uint8_t task_len);
jlgpu_err_t jlgpu_get_err(jlgpu_t *ctx);
void jlgpu_set_ops(jlgpu_t *ctx, jlgpu_ops_t *jlgpu_ops);

void jlgpu_init(jlgpu_t *ctx);
void jlgpu_uninit(jlgpu_t *ctx);
void jlgpu_run(jlgpu_t *ctx, void *task_base);

//配置SFR参数
void jlgpu_set_params(jlgpu_t *ctx, jlgpu_surface_t *surface, jlgpu_global_matrix_t *global_matrix,
                      jlgpu_colorkey_t *colorkey, jlgpu_global_path_t *global_path, jlgpu_uv_adr_t *uv_adr);
jlgpu_err_t jlgpu_set_win_param(jlgpu_surface_t *surface);
jlgpu_err_t jlgpu_set_global_trans_param(jlgpu_global_matrix_t *global_matrix);
jlgpu_err_t jlgpu_set_colorkey_param(jlgpu_colorkey_t *colorkey);
jlgpu_err_t jlgpu_set_global_path_param(jlgpu_global_path_t *global_path);
jlgpu_err_t jlgpu_set_uv_adr_param(jlgpu_uv_adr_t *uv_adr);

void jlgpu_set_block_render_param(jlgpu_t *ctx, jlgpu_block_render_params_t *block_render_param);
void jlgpu_sph_matrix_set_param(jlgpu_sph_matrix_params_t *sph_matrix_param);

#endif
