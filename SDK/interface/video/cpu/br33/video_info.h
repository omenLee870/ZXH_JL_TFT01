#ifndef VIDEO_INFO_H
#define VIDEO_INFO_H

#include "video_ioctl.h"
#include "system/spinlock.h"
#include "device/device.h"
#include "generic/list.h"
#include "os/os_type.h"

//------->video_ioctl.h

struct video_source_crop {
    int x_offset;
    int y_offset;
    int width;
    int height;
};


struct video_capability {
    char name[16];
    u32 capabilities;
};

struct video_mirror {
    u32 enc_dis;
    u32 mirror;
};

struct video_jaggy {
    u32 enc_dis;
    u32 jaggy;
};

struct video_window {
    u16 left;
    u16 top;
    u16 width;
    u16 height;
    u16 border_left;
    u16 border_top;
    u16 border_right;
    u16 border_bottom;
    u16 mirror;
    u8 win_type;
    u8 combine;
};


struct video_format {
    enum video_buf_type type;
    u8 camera_type;
    u8 online;//0:是手动编码，1是联动
    u8 enable_dri;
    u8 three_way_type;
    u8 disp_yuv_type;
    u8 disp_fb_buf_num;
    u8 uvc_id;
    u8 disp_id;
    u8 fps;
    u8 osd_en;
    u16 enc_rotate;
    u16 dis_rotate;
    u8 stop_wait;
    u8 three_in_one_open;
    u16 src_width;
    u16 src_height;
    int blk_width;
    int blk_height;
    u32 pixelformat;
    u32 gap_fps;
    struct video_window win;
    void *private_data;
    u8 double_raw;
};

struct jpg_thumbnail {
    u8 enable;
    u8 quality;
    u16 width;
    u16 height;
    u8 *buf;
    int len;
};

struct image_sticker {
    u8 enable;
    u8 format;//YUV420/YUV422
    u8 threshold;//y分量阈值，小于该阈值为贴纸透明
    u16 width;
    u16 height;
    u8 *addr;
    u32 size;
};

struct video_image_capture {
    u16 width;
    u16 height;
    u8  *baddr;
    u32 size;
    int format;
    u32 mirror;
    struct jpg_thumbnail *thumbnail;
    struct image_sticker *sticker;
    u8 linear_scale;	// 使用软件线性插值标志
};

struct video_reqbufs {
    void *buf;
    int size;
    struct buffer_dev {
        void *fd;
        u32 head_size;
        u32 align_size;
        u32 addr;
        u32 size;
        int (*read)(void *fd, void *buf, u32 addr, int len);
        int (*write)(void *fd, void *buf, u32 addr, int len);
    } dev;

};

struct videobuf_buffer {
    u32 len;
    u32 magic;
    u32 msec;
    u8 data[0];
};

struct videobuf_state {
    u32 available_len;
    u32 max_continue_len;
};

// struct videobuf_queue {
// u8 ref;
// u8 streaming;
// u16 align;
// const char *name;
// OS_SEM sem;
// void *buf;
// struct lbuff_head *lbuf;
// void *private_data;
// };

struct video_buffer {
    u8  index;
    u8 	noblock;
    u16 timeout;
    u32 time_msec;
    u32 len;
    u32 baddr;
    void *priv;
    u32 test_num;
};





//------->video.h

struct videobuf_queue {
    u8 ref;
    u8 streaming;
    u16 align;
    const char *name;
    OS_SEM sem;
    void *buf;
    struct lbuff_head *lbuf;
    void *private_data;
};


struct yuv_frame_data {
    //编码器内部使用
    u16 width;          //对齐后宽度
    u16 height;         //对齐后高度
    u16 real_width;     //实际宽度(对齐前)
    u16 real_height;    //实际高度(对齐前)
    u16 data_width;     //当前传入的宽度
    u16 data_stride;    //像素步进, 不配置默认为data_width
    u16 data_height;    //当前传入的高度
    u8  *y;             //输入Y地址
    u8  *uv;            //输入UV地址
    float factor;

    //用户自定义使用
    u32 stat_width;     //统计已传入的宽
    u32 stat_height;    //统计已传入的高
    u32 data_col;       //记录标准传入宽
    u32 data_row;       //记录标准传入高
    u8 need_uv;         //是否需要uv地址
};

struct video_frame_yuv {
    u16 width;
    u16 height;
    u8 *y;
    u8 *u;
    u8 *v;
};

struct video_yuv_buffer {
    u8 *y;
    u8 *u;
    u8 *v;
};

struct image_scale_data {
    u8  zstate;
    u32 src_reg_data;
    u16 out_width;
    u16 out_height;
    u8 *output_buf;
    struct yuv_frame_data input_frame;
};

struct video_image_enc {
    u8  mkstart;
    u16 img_width;
    u16 img_height;
    u16 blk_width;
    u16 blk_height;
    struct video_yuv_buffer *blk_buf;
};

struct video_image_dec {
    u8  fkstart;
    u16 width;
    u16 height;
    u8  *buf;
    u32 size;
    struct video_yuv_buffer *blk_buf;
};

struct video_decode_frame {
    u8 type;
    unsigned int reset : 1;
    unsigned int mode : 7; //0 -- display, 1 -- encode
    u16 width;
    int line;
    u8 *buf;
    u8 *circle_y;
    u8 *circle_u;
    u8 *circle_v;
    int cnt;
};

struct video_encode_frame {
    u8 type;
    u8 reset;
    u8 *buf;
    int cnt;
    int lines;
};

struct video_cap_buffer {
    u8 num;
    u8 size;
    u8 *buf;
    u32 line;
    u32 total_size;
};


struct video_subdevice;
struct video_var_param_info;

struct video_subdevice_data {
    int tag;
    void *data;
};

struct video_platform_data {
    int num;
    const struct video_subdevice_data *data;
};

struct roi_cfg {
    u32 roio_xy;
    u32 roi1_xy;
    u32 roi2_xy;
    u32 roi3_xy;
    u32 roio_ratio;
    u32 roio_ratio1;
    u32 roio_ratio2;
    u32 roio_ratio3;
    u32 roio_config;
};

typedef struct jpg_q_table qtfilehead_t;
struct jpg_q_table {
    u16  YQT_DCT[0x40] ;
    u16  UVQT_DCT[0x40];
    u8   DQT[138]; //file header
};

struct h264_user_data {
    u8 *data;
    u32 len;
};

struct h264_s_attr {
    u8 quality;
    u8 IP_interval;
    u8 std_head;
    u16 width;
    u16 height;
    /*u32 i_bitrate;*/
    u32 gap_fps;
};

struct h264_qp {
    u8 static_min;
    u8 static_max;
    u8 dynamic_min;
    u8 dynamic_max;
    u8 enable_change_qp;
};

struct h264_d_attr {
    u32 abr_kbps;
    struct roi_cfg roi;
    struct h264_qp qp;

};

struct icap_auxiliary_mem {
    u8 *addr;
    u32 size;
};

struct image_s_attr {
    u8   quality;
    u16  width;
    u16  height;
    enum image_pix_format format;
    int    aligned_width;
    struct jpg_q_table *jpg_qt;
    struct icap_auxiliary_mem *aux_mem;
    struct jpg_thumbnail *thumb;

    u8 *exif;
    int exif_size;
};

struct jpg_dec_mem {
    u8 *addr;
    u32 size;
};

struct jpg_dec_s_attr {
    u16 max_o_width;
    u16 max_o_height;
    u16 src_width;
    u16 src_height;
    u16 dec_width;
    u16 dec_height;
    struct jpg_dec_mem dec_mem;
};

struct video_enc_attr {
    enum video_pix_format format;
    void *attr;
};

struct video_dec_attr {
    enum video_pix_format format;
    void *attr;
};

struct yuv_image {
    u8 format;
    u16 width;
    u16 height;
    u8 *addr;
    u32 size;
};

struct video_dev_fps {
    u8 camera_fps;
    u8 real_fps; //imc real fps !!!
    u8 target_fps;
    int tlp_time;
};

struct video_osd {

    u16 x ;//起始地址
    u16 y ;//结束地址
    // u32 osd_yuv;//osd颜色
    u32     color[3]; //2bit图像yuv颜色配置

//注意：下面的字符串地址必须是全局的,然后年是yyyy，月是nn，日是dd，时是hh，分是mm，秒是ss,其他字符是英文字母&&符号&&汉字
    char *osd_str; //用户自定义格式，例如 "yyyy-nn-dd\hh:mm:ss" 或者 "hh:mm:ss"
    char *osd_matrix_str; //用户自定义字模字符串,例如“abcd....0123..”
    u8 *osd_matrix_base; //用户自定义字模的起始地址
    u32 osd_matrix_len;//用户自定义字模数组的长度,no str len!!!
    u8 osd_w;//用户自定义字体大小,8的倍数
    u8 osd_h;//8的倍数
};

struct image_capture_info {
    u8 multi_scale;
    struct video_image_capture *icap;
};

struct video_text_osd {
    u16     x;//起始地址
    u16     y;//结束地址
    // u32     osd_yuv;//osd颜色
    u32     color[3]; //2bit图像yuv颜色配置
//注意：下面的字符串地址必须是全局的,然后年是yyyy，月是nn，日是dd，时是hh，分是mm，秒是ss,其他字符是英文字母&&符号&&汉字
    char    *text_format; //用户自定义格式，例如 "yyyy-nn-dd\hh:mm:ss" 或者 "hh:mm:ss"
    char    *font_matrix_table; //用户自定义字模字符串,例如“abcd....0123..”
    u8      *font_matrix_base; //用户自定义字模的起始地址
    u32     font_matrix_len;//用户自定义字模数组的长度,no str len!!!
    u8      font_w;//用户自定义字体大小,8的倍数
    u8      font_h;//8的倍数
    u8      direction; //0 -- 顺向, 1 -- 逆向
    u8      bit_mode; //2 - 2bit 1 - 1bit
};

struct video_graph_osd {
    u8      bit_mode; //2 - 2bit, 16 - 16bit
    u16     x;
    u16     y;
    u32     color[3]; //2bit图像yuv颜色配置
    u16     width;
    u16     height;
    u8      *icon; //图形内容buffer
    int     icon_size;
};

struct video_osd_config {
    struct video_text_osd text_osd;
    struct video_graph_osd icon_osd;
};

struct video_osd_attr {
    u32 enable;
    u32 ability;
    enum video_osd_type type;
    struct video_text_osd *text_osd;
    struct video_graph_osd *graph_osd;
};

struct app_enc_info {
    u8 user_cfg_enable;

    int qp_offset_low;
    int qp_offset_high;
    float f_aq_strength;

    u8 dsw_size;
    int fs_x_points;
    int fs_y_points;

    int nr_reduction;

    int pre_pskip_limit_flag;
    int common_pskip_limit_flag;
    int pre_pskip_th;
    int common_pskip_th;

};

struct image_capability {
    u8  zoom;
    u16 width;
    u16 height;
    struct video_image_capture *icap;
};

struct video_subdevice_ops {
    int (*init)(const char *name, struct video_platform_data *);

    bool (*online)(const char *name);

    int (*get_fmt)(struct video_format *f);

    int (*set_fmt)(struct video_format *f);

    struct video_endpoint *(*open)(struct video_var_param_info *);

    int (*overlay)(struct video_endpoint *, int i);

    int (*streamon)(struct video_endpoint *);

    int (*streamoff)(struct video_endpoint *);

    int (*get_image_capability)(struct video_endpoint *, struct image_capability *);
    int (*image_capture)(struct video_endpoint *, struct image_capability *);

    int (*response)(struct video_endpoint *, int cmd, void *);

    int (*write)(struct video_endpoint *, void *buf, u32 len);

    int (*close)(struct video_endpoint *);

    int (*querycap)(struct video_endpoint *, struct video_capability *cap);

    int (*ioctrl)(struct video_endpoint *, u32 cmd, void *arg);
};

struct video_var_param_info {
    u32 fps;
    u16 width;
    u16 height;

    u8 ch;
    u8 channel;
    int source;
    struct video_format *f;
    void *priv;
};

struct video_subdevice {
    u8 subid;
    const char *name;
//    u32 input_pixelformat;
    u32 output_pixelformat;
    const struct video_subdevice_ops *ops;
    int (*request)(struct video_endpoint *, int cmd, void *);
};

struct video_endpoint {
    struct list_head entry;
    struct video_subdevice *dev;
    int inused;
    void *parent;
    void *private_data;
    void *imc_data;
};

struct video_crop_ctrl {
    u16 crop_sx;
    u16 crop_ex;
    u16 crop_sy;
    u16 crop_ey;
};

struct video_crop_sca {
    u16 src_w;
    u16 src_h;
    u16 crop_w;
    u16 crop_h;
};

struct video_crop_tri {
    int (*do_crop)(void *priv, void *parm);
    void *priv;
    void *parm;
};


extern struct video_subdevice video_subdev_begin[];
extern struct video_subdevice video_subdev_end[];

extern const struct device_operations video_dev_ops;
extern const struct device_operations usb_cam_dev_ops;

struct video_device;

struct video_device_ops {
    const char *name;
    bool (*online)(void *p);
    int (*init)(const char *name, const struct video_platform_data *data);
    int (*open)(struct video_device *);
    int (*set_fmt)(struct video_device *, struct video_format *);
    int (*overlay)(struct video_device *, int on);
    int (*streamon)(struct video_device *);
    int (*streamoff)(struct video_device *);
    int (*image_capture)(struct video_device *, struct video_image_capture *icap);
    int (*start_I_frame)(struct video_device *);
    int (*adjust_fps)(struct video_device *, void *);
    int (*ioctl)(struct video_device *, int cmd, u32 arg);
    int (*write)(struct video_device *, void *data, u32 len);
    int (*close)(struct video_device *);
};

extern struct video_device_ops video_dev_begin[];
extern struct video_device_ops video_dev_end[];

struct video_fill_frames {
    u8    reset_timer;
    u32    time_cnt;
    u32   o_frame_cnt;
    short interval;
    short fill_dly;
    char  fps_remain;
    int   lost_frames;
    int   timer;
    int   timer_one_frame;
    spinlock_t lock;
    int   msg_post;
};

struct video_device {
    struct device device;
    struct list_head endpoint;
    struct list_head entry;
    struct videobuf_queue video_q;
    struct video_var_param_info info;
    struct video_subdevice *subdev[4];
    struct video_image_capture *icap;
    struct video_device_ops *dev_ops;
    struct video_dev_fps fps;
    struct video_fill_frames ff;
    void   *private_data;
    OS_SEM sem;
    u8 ref;
    u8 time_base;
    u8 streamon;
    u8 subdev_num;
    u8 major, mijor;
    u8 mal_dbuf;
    u8 stop_now;
    u32 frame_cnt;
    u32 stop_frame_cnt;
    u32 stop_frame_interval;
    u32 pixelformat;
    u32 stop_time;
    u32 stop_skip_frame;
    u32 start_time;
    u32 insert_frame_cnt;
    u32 last_insert_time;
};

#endif

