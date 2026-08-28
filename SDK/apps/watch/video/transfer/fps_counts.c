/**
 * @file fps_counts.c
 * @brief Standalone FPS (frames per second) counter for vstream transfer module.
 *        Provides independent FPS counters that do NOT depend on video_rec's __this pointer.
 *        Used by vstream_receiver.c, vstream_sender.c, and spp_transfer.c.
 */

#include "system/includes.h"

#define LOG_TAG               "[FPS_COUNT]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"

/* --------------------------------------------------------------------------*/
/* Independent FPS counter variables                                         */
/* --------------------------------------------------------------------------*/

struct fps_count {
    u8 capacity;           	// 缓冲区最大容量（需 ≥ 最大可能帧率）
    u8 head;               	// 下一个写入位置
    u8 count;              	// 当前缓冲区中的有效时间戳数量
    u8 fps;					// 最近1s平均帧率
    u8 en;					// 使能
    u32 *timestamps;   		// 环形缓冲区，存储每个帧的时间戳（毫秒）
    u8 fps_audio;           //音频帧率
};

struct fps_count_handle {
    struct fps_count fps_win;	    //最近1s窗口帧率 tx
    struct fps_count fps_win_rx;	//最近1s窗口帧率 rx
};
static struct fps_count_handle fps_hd;
#define __this (&fps_hd)
DEFINE_SPINLOCK(fps_test_lock);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_counter_init 初始化帧率统计
 *
 * @param capacity	目标帧率（15）
 */
/* ------------------------------------------------------------------------------------*/
void fps_win_counter_init(int capacity)
{
    if (!__this && !__this->fps_win.en && capacity) {
        return ;
    }
    __this->fps_win.capacity = capacity;
    __this->fps_win.head = 0;
    __this->fps_win.count = 0;
    __this->fps_win.timestamps = malloc(sizeof(int) * capacity);
    if (__this->fps_win.timestamps) {
        __this->fps_win.en = 1;
    }

    if (!__this && !__this->fps_win_rx.en && capacity) {
        return ;
    }
    __this->fps_win_rx.capacity = capacity;
    __this->fps_win_rx.head = 0;
    __this->fps_win_rx.count = 0;
    __this->fps_win_rx.timestamps = malloc(sizeof(int) * capacity);
    if (__this->fps_win_rx.timestamps) {
        __this->fps_win_rx.en = 1;
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_counter_deinit 关闭帧率统计
 *
 */
/* ------------------------------------------------------------------------------------*/
void fps_win_counter_deinit()
{
    __this->fps_win.en = 0;
    if (__this->fps_win.timestamps) {
        free(__this->fps_win.timestamps);
        __this->fps_win.timestamps = NULL;
    }

    __this->fps_win_rx.en = 0;
    if (__this->fps_win_rx.timestamps) {
        free(__this->fps_win_rx.timestamps);
        __this->fps_win_rx.timestamps = NULL;
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_counter_update 更新帧率 video tx
 *
 * @param now_ms 时间戳
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int fps_win_counter_update(u32 now_ms)
{

    spin_lock(&fps_test_lock);
    if (!__this || !__this->fps_win.en) {
        spin_unlock(&fps_test_lock);
        return -1;
    }
    struct fps_count *fps = &__this->fps_win;
    // 将当前时间戳存入环形缓冲区
    fps->timestamps[fps->head] = now_ms;
    fps->head = (fps->head + 1) % fps->capacity;
    if (fps->count < fps->capacity) {
        fps->count++;
    }

    int threshold  = 1000; // 1秒前的阈值

    // 从最新时间戳向旧时间戳遍历，统计仍在窗口内的帧数
    int valid = 0;
    // 由于时间戳递增存储，一旦遇到早于阈值的，后面必然更早，可提前终止
    for (int i = 0; i < fps->count; i++) {
        // 从最新（head-1）向最旧遍历
        int idx = (fps->head - 1 - i + fps->capacity) % fps->capacity;
        if (jiffies_msec2offset(fps->timestamps[idx], now_ms) <= threshold) {
            valid++;
        } else {
            break; // 后续时间戳更小，无需继续
        }
    }
    fps->fps = valid;
    static u32 rec_dump_time;
    if (jiffies_msec2offset(rec_dump_time, now_ms) >= 1000) {
        printf("%s fps_win:%d count:%d", __func__, fps->fps, fps->count);
        rec_dump_time = now_ms;
    }
    spin_unlock(&fps_test_lock);
    return valid;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_rx_counter_update 更新帧率 video rx
 *
 * @param now_ms 时间戳
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int fps_win_rx_counter_update(u32 now_ms)
{
    if (!__this || !__this->fps_win_rx.en) {
        return -1;
    }
    struct fps_count *fps = &__this->fps_win_rx;

    spin_lock(&fps_test_lock);
    fps->fps++;
    spin_unlock(&fps_test_lock);
    return fps->fps;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_audio_win_rx_counter_update 更新帧率 audio rx
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int fps_audio_win_rx_counter_update(void)
{
    if (!__this || !__this->fps_win_rx.en) {
        return -1;
    }
    struct fps_count *fps = &__this->fps_win_rx;
    spin_lock(&fps_test_lock);
    fps->fps_audio++;
    spin_unlock(&fps_test_lock);
    return fps->fps_audio;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_counter_get_fps 获取最近1s平均帧率 video tx
 *
 * @note 目前统一打印相关信息, 在打印的地方用spin lock了
         不额外在这个函数使用spin lock, 增加消耗了
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int fps_win_counter_get_fps()
{
    if (!__this || !__this->fps_win.en) {
        return -1;
    }
    if (__this->fps_win.count < __this->fps_win.capacity) {
        return -1;
    }
    return __this->fps_win.fps;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_counter_get_fps_rx 获取最近1s平均帧率 video rx
 *
 * @note 目前统一打印相关信息, 在打印的地方用spin lock了
         不额外在这个函数使用spin lock, 增加消耗了
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int fps_win_counter_get_fps_rx()
{
    if (!__this || !__this->fps_win_rx.en) {
        return -1;
    }
    int fps = __this->fps_win_rx.fps;
    __this->fps_win_rx.fps = 0;
    return fps;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief fps_win_counter_get_fps_audio_rx 获取最近1s平均帧率 audio rx
 *
 * @note 目前统一打印相关信息, 在打印的地方用spin lock了
         不额外在这个函数使用spin lock, 增加消耗了
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int fps_win_counter_get_fps_audio_rx()
{
    if (!__this || !__this->fps_win_rx.en) {
        return -1;
    }
    int fps = __this->fps_win_rx.fps_audio;
    __this->fps_win_rx.fps_audio = 0;
    return fps;
}

