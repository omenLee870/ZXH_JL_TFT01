#ifndef __GIF_H__
#define __GIF_H__

#include "typedef.h"
#include "gpu_port.h"
#include "res/resfile.h"
#include "generic/list.h"

enum {
    COMPRESS_LOW,       //低压缩，省SRAM，NorFlash不需要PSRAM
    COMPRESS_MIDDLE,    //中压缩，是否需要PSRAM视GIF图片而定
    COMPRESS_HIGH,      //高压缩，省Flash, 需要PSRAM
};

struct gif_file_info {
    u16 width;
    u16 height;
    u16 frame_num;
    u16 delay;
    u16 version;
};

struct gif_frame_info {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
    u32 addr;
    u32 size;
    int type;
    int compress;
    u16 delay;
    u8 *buf;
    u8 *lut;
};

struct gif_file {
    void *fp;                           //资源文件句柄
    int offset;                         //数据在资源文件里的偏移
    int flash_addr;                     //文件在flash中的起始地址
    int len;                            //数据长度
    void *file_info;                    //norflash地址映射表信息
    int extern_file_info;               //外部file_info
    int (*read)(void *, int, u8 *, int);//自定义读接口
};

typedef void *(*GIF_CACHE_CB)(void *, pJLGPUTaskParam_t, void *, int);
typedef void (*GIF_TIMER_CB)(void *);

struct gif_info {
    u16 gif_timer_id;
    u16 gif_frame_num;
    u16 gif_frame_index;
    u16 gif_frame_last_index;
    u16 gif_frame_main_index;
    u16 gif_delay;
    u8 *gif_palette1;
    u8 *gif_dec_buf;
    u8 *gif_palette2;
    struct gif_file gif_file;
    u32 gif_addr;
    u32 gif_size;

    u8 *gif_keyframe_mmu_tab_base;
    u8 *gif_keyframe_addr;
    u32 gif_keyframe_size;
    int priority;

    //gpu
    pJLGPUTaskHead_t head;
    gpu_matrix_t *matrix;
    struct rect draw; // gif绘制区域
    struct rect rect;
    u32 element_id;
    u32 task_id;

    u8 gif_type;
    u8 gif_compress;
    u16 gif_version;

    u8 *gif_buf;
    u8 task_prior;
    u8 play_single;

    //nandflash
    void *fp;
    int index;
    GIF_CACHE_CB cache_data_cb;
};

struct gif_timer {
    struct list_head head;
    pJLGPUTaskUnit_t task;
    int page;
    u16 timer_id;
    u32 element_id;
    u32 task_id;
};


struct gif_param {
    struct gif_file *file;
    struct gif_file_info *file_info;
    struct gif_frame_info *frame_info;
    pJLGPUTaskUnit_t p;
    GIF_TIMER_CB gif_timer_cb;
    pJLGPUTaskHead_t head;
    pJLGPUTaskParam_t gpu_param;
    struct rect *rect;
    struct rect *draw;
    void *fp;
    int index;
    u32 keyframe_addr;
    GIF_CACHE_CB cache_data_cb;
};

int gif_read_head_info(struct gif_file *file, struct gif_file_info *file_info, struct gif_frame_info *frame_info);
struct gif_timer *gif_decode_init(struct gif_param *gif_param);
int gif_decode(int element_id, int task_id);
int gif_decode_uninit(int element_id, int task_id, const char *task_name, int (*gif_msg_type)());
struct gif_info *gif_info_get(int element_id, int task_id);
int gif_info_del(int element_id, int task_id);
void gif_task_delete(pJLGPUTaskHead_t head, int element_id, int task_id);

#endif





