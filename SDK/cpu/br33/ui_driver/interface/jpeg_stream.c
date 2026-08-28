/* #include "app_task.h" */
#include "system/timer.h"
#include "device/device.h"
#include "key_event_deal.h"

#include "res/resfile.h"
#include "jlui/ui.h"
#include "jlui_app/ui_style.h"
#include "jlui_app/ui_api.h"
#include "jlui_app/res_config.h"
#include "jlui_app/ui_resource.h"
#include "jlui_app/ui_sys_param.h"
#include "ui_draw/ui_type.h"

#include "jlgpu_math.h"		// matrix
#include "jlgpu_driver.h"	// gpu 驱动

#include "gpu_port.h"	// GPU 接口，依赖于 jlgpu_driver.h
#include "gpu_task.h"

#include "jljpeg_decode.h"
#include "jpeg_stream.h"
/* #include "video/avi/avilib.h" */

#if CONFIG_JL_UI_ENABLE

#define LOG_TAG_CONST       JPEG
#define LOG_TAG     		"[JPEG_STREAM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_WARN_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_PSRAM_DEV_ENABLE
#define JPEG_STREAM_ALLOC  			malloc_psram
#define JPEG_STREAM_FREE			free_psram
#else
#define JPEG_STREAM_ALLOC  			malloc
#define JPEG_STREAM_FREE			free
#endif

#define JPEG_STREAM_SPIN_LOCK_EN	1

struct jpeg_stream_info {
    // jpeg原图
    u8 *src_buf;
    int src_len;
    volatile u8 data_lock;
    u8 copy;
    u16 idx;
#if JPEG_STREAM_SPIN_LOCK_EN
    spinlock_t lock;
#else
    OS_MUTEX lock;
#endif

} __jpeg_stream_info;
#define __this (&__jpeg_stream_info)

#if JPEG_STREAM_SPIN_LOCK_EN
#define JPEG_STREAM_LOCK_INIT spin_lock_init(&__this->lock);
#define JPEG_STREAM_LOCK	spin_lock(&__this->lock);
#define JPEG_STREAM_UNLOCK	spin_unlock(&__this->lock);
#else
#define JPEG_STREAM_LOCK_INIT  os_mutex_create(&__this->lock);
#define JPEG_STREAM_LOCK		os_mutex_pend(&__this->lock);
#define JPEG_STREAM_UNLOCK		os_mutex_post(&__this->lock);
#endif

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_init jpeg数据流初始化
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_stream_init()
{
    JPEG_STREAM_LOCK_INIT;
    __this->data_lock = 0;
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_deinit

 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_stream_deinit()
{
    JPEG_STREAM_LOCK;
    __this->data_lock = 1;
    __this->src_len = 0;
    if (__this->copy) {
        JPEG_STREAM_FREE(__this->src_buf);
    }
    __this->src_buf = NULL;
    JPEG_STREAM_UNLOCK;
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_src_data_lock 数据锁保护
 */
/* ------------------------------------------------------------------------------------*/
void jljpeg_stream_src_data_lock()
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("<%s> rets:%x\n", __func__, rets);
    JPEG_STREAM_LOCK;
    __this->data_lock = 1;
    JPEG_STREAM_UNLOCK;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_src_data_unlock 数据解锁保护
 */
/* ------------------------------------------------------------------------------------*/
void jljpeg_stream_src_data_unlock()
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("<%s> rets:%x\n", __func__, rets);
    JPEG_STREAM_LOCK;
    __this->data_lock = 0;
    JPEG_STREAM_UNLOCK;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_src_data_lock_check 查询锁状态
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_stream_src_data_lock_check()
{
    return __this->data_lock;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_src_data_copy 输入数据并拷贝到psram缓存
 *
 * @param buf
 * @param size
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_stream_src_data_copy(u8 *buf, int size, int copy)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("<%s> rets:%x size:%d\n", __func__, rets, size);
    if (jljpeg_stream_src_data_lock_check()) {
        return -1;
    }
    JPEG_STREAM_LOCK;
    __this->copy = copy;
    if (__this->copy) {
        if (__this->src_buf) {
            JPEG_STREAM_FREE(__this->src_buf);
        }
        log_debug("%s line:%d buf:%x", __func__, __LINE__, (u32)__this->src_buf);
        __this->src_buf = JPEG_STREAM_ALLOC(size);
        memcpy(__this->src_buf, buf, size);
    } else {
        __this->src_buf = buf;
    }
    __this->src_len = size;
    __this->idx++;
    log_debug("%s line:%d buf:%x", __func__, __LINE__, (u32)__this->src_buf);
    JPEG_STREAM_UNLOCK;
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_src_data_len_get 获取缓存数据长度
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jljpeg_stream_src_data_len_get()
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("<%s> rets:%x len:%d\n", __func__, rets, __this->src_len);
    return __this->src_len;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jljpeg_stream_src_data_get 获取缓存数据地址
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
u8 *jljpeg_stream_src_data_get()
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("<%s> rets:%x buf:0x%x\n", __func__, rets, (u32)__this->src_buf);
    return __this->src_buf;
}

#endif /*#if CONFIG_JL_UI_ENABLE*/




