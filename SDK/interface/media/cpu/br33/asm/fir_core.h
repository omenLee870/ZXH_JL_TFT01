

#ifndef  FIR_CORE_H
#define  FIR_CORE_H
#include "includes.h"
#include "fir_core_api.h"

#define FIR_MAX_FRAME_LEN  (128)//每个声道的最大点数
/* #define max_framelength  FIR_MAX_FRAME_LEN */

typedef struct fir_driv_num_ {
    int count_save;
    short Q_signal;//需要信号放大的位数，系数放大8位超出部分给到信号放大
    JL_FIRBUF_TypeDef fir_buf;
} fir_driv_num;

typedef struct fir_config_ { //
    struct fir_param fp;
    fir_driv_num *fd;
    JL_FIRPARA_TypeDef *fir_para;
    int count_dsf;
    int res;
    int remain;//用于判断当前剩余的数据
    short fir_buflen;//单通道tap数缓存+最大帧长；init后对于AABB类型意思不变，ABAB类型表示各通道相加的总长度用于，标定硬件通道的计算长度
    void *fir_ibuf_;//循环buf
    void *fir_obuf_;
    int PerChannel_point;
    volatile OS_SEM sem;
    struct list_head entry;
    void *fp_update;// audio_fir.c update接口使用
    u8 active;
    u8 update;
    u8 init;
    u8 error;
    short pool[0];
} fir_config;







#endif // ! FIR_CORE_H
