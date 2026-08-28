

#ifndef FIR_CORE_API_H
#define FIR_CORE_API_H

#include "generic/typedef.h"
#include "system/spinlock.h"
#include "generic/list.h"
#include "os/os_api.h"
#include "effects/AudioEffect_DataType.h"

enum FIR_PCMDataType {
    FIR_DATA_INT_16BIT = 0,
    FIR_DATA_INT_24BIT,
    FIR_DATA_INT_32BIT
};

/* enum { */
// af_DATABIT_NOTSUPPORT = 0x404, //如果是错误的类型，或者sdk不支持的数据类型，返回该值，由初始化调用，int型返回
/* }; */
struct fir_core {
    struct list_head head;            //链表头
    OS_MUTEX mutex;
    u32 cur_ch;
    spinlock_t lock;
    u32 max_frame_length;//每个声道的最大点数
};


struct fir_DataType {
    unsigned char IndataBit;//0:16bit 1:32bit
    unsigned char OutdataBit;//0:16bit 1:24bit 2:32bit
    char IndataInc;// 1 2 3..
    char OutdataInc;
    char Qval;
};


struct fir_param {
    char fir_ch;//通道数 0~15 4bit
    char shift;//移位数 8~31 shift大小从8开始，表示信号和系数的放大Q值最少是8开始，最多shift是31（5bit）
    char dsf;//dsf数	 0~15
    char close_flag;
    short fir_tap;//tap数
    char  reserve[2];//
    struct fir_DataType ft;
    short **fir_cbuf;
};

int fir_getbuf(struct fir_param *fp);
int fir_init(void *ptr, struct fir_param *fp, void *tmp);//tmp 是配通用固件，接口内无使用
int fir_update(void *ptr, struct fir_param *fp);
void fir_run(void *ptr, void *indata, void *outdata, int PointsPerChannel);
void fir_uninit(void *ptr);

struct effects_func_api *fir_core_ops();

void fir_hardware_init(int max_framelength);

/* 注意事项： */
// 1.buflen 单通道fir缓存点数，应为帧长N+tap数L（fir缓存是循环buf，固定帧长，帧长10，tap数10，buflen=20）,
// 在交织的存储中，该值换算为字节不能超过8192字节，比如ABAB类型FIR缓存N点，32bit，需满足N*4*通道数<8192；
// 在块状的存储中，该值换算为字节不能超过8192字节，比如AABB类型FIR缓存N点，32bit，需满足N*4<8192；(这里不乘通道数)
// 2.硬件流程要求shift移位不能小于8，输入的系数和信号的总移位要不小于8
// 3.dsf表示下采样，从第一个点开始抽取，dsf+1倍抽，比如ABAB类型原输出为01 23 45 67 89 ，dsf=1输出为23 67
// AABB类型原输出为01234 56789 ，dsf=1输出为1368
// 3.sdf=0，输入len个，输出len个数据；
/* 4.支持输入输出同地址 */
#endif // !FIR_CORE_API_H
