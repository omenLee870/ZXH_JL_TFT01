#ifndef CONFIG_VIDEO_REC_H
#define CONFIG_VIDEO_REC_H

#include "system/includes.h"

/*==============================================================================
 * 录像模式入口/退出
 *==============================================================================
 */

/* video_rec模式入口函数 */
struct app_mode *app_enter_video_rec_mode(int arg);

/* video_rec模块初始化 */
int video_rec_init(int param);

/* video_rec模块退出 */
void video_rec_exit(void);

/*==============================================================================
 * 录像状态枚举
 *==============================================================================
 */
enum video_rec_state {
    VIDEO_STATE_IDLE = 0,      // 空闲状态
    VIDEO_STATE_RECORDING,     // 录像中
    VIDEO_STATE_PAUSED,        // 暂停
    VIDEO_STATE_SAVING,        // 保存中
    VIDEO_STATE_ERROR,         // 异常(SD满/拔出/编码失败)
};

/*==============================================================================
 * 循环录像时长选项
 *==============================================================================
 */
/*==============================================================================
 * 低电量阈值 — 电量低于此值录像自动停止 (PRD P0)
 *============================================================================*/
#define VIDEO_REC_LOW_BAT_PERCENT    10

enum video_rec_loop_time {
    LOOP_TIME_30_SEC = 30,    // 30秒
    LOOP_TIME_60_SEC = 60,    // 60秒（默认）
    LOOP_TIME_90_SEC = 90,    // 90秒
};
/* 兼容旧名: 默认档 = 60秒 */
#define LOOP_TIME_DEFAULT  LOOP_TIME_60_SEC

/*==============================================================================
 * 录像配置宏 - PRD规范
 *==============================================================================
 */

/* 分辨率和帧率 */
#define VIDEO_REC_WIDTH           640     // 录像宽度
#define VIDEO_REC_HEIGHT          480     // 录像高度
#define VIDEO_REC_FPS             20      // 录像目标帧率: 须与 VI 实际输出一致(GC0308 只能出20fps, VI output fps=20; 原30帧是错误基准)

/* 文件路径和命名 */
#define VIDEO_REC_FILE_PREFIX     "VID_"
#define VIDEO_REC_FILE_EXT        ".AVI"
/* 录像文件命名模式 (t_338c85b4: 双模式命名, t_caa69f68: Mode1 清理 file_index)
 *   0 = 时间戳模式 (默认): VID_YYYYMMDD_HHMMSS_NNN.AVI
 *       段文件由 CYC_FILE 回调走 frename 重命名为带时间戳的最终名,
 *       每段一个独立文件, 适合保留历史.
 *       file_index 在 Mode 0 下作为 _NNN 同秒内递增序号, 由 CYC_FILE
 *       事件中赋值 = cyc_count, 启动时在 worker 中 +1 让首段从 001 起.
 *   1 = 固定模式: VID_****.AVI, 字面量占位, 由 JLFS 在 fopen(O_CREAT)
 *       时若发现同名文件自动追加数字避免覆盖, 段之间不重命名.
 *       file_index 在 Mode 1 下不写入路径, 也不推进, 完全由文件系统
 *       自动命名. 适合简化文件列表、不希望保留历史段的用户. */
#define VIDEO_REC_NAMING_MODE             1

/* 固定模式命名模板 - 4 位数字, 由 JLFS 自增填充
 *   实际写入路径 = CONFIG_REC_PATH + sprintf("VID_%04d.AVI", file_index) */
#define VIDEO_REC_FIXED_NAME_FMT          "VID_****.AVI"
#define VIDEO_REC_FIXED_INDEX_MAX         9999

/* 存储管理配置 */
#define STORAGE_PROTECTED_RATIO   30      // 加锁文件占TF卡百分比阈值
#define MIN_FREE_SPACE_MB         500      // 保留最小空间(MB)
#define VIDEO_DELETE_MAX_FILES    10       // 单次空间/数量检查最多删除的旧录像文件数
#define VIDEO_REC_MAX_FILES       9999     // 录像目录最大AVI文件数 (超出后循环覆盖最旧文件)

/*==============================================================================
 * MPI通道定义
 *==============================================================================
 */
#define VIDEO_REC_VI_DEV          0       // VI设备号
#define VIDEO_REC_VPSS_GRP        0       // VPSS组号
#define VIDEO_REC_DISPLAY_CHN     0       // 显示通道
#define VIDEO_REC_RECORD_CHN      1       // 录像通道
#define VIDEO_REC_VENC_DEV        0       // VENC设备号

/* 视频格式 */
#define VIDEO_REC_FORMAT          MPI_PIXEL_FMT_YUV422_PACKED
#define VIDEO_REC_YUV_ORDER      MPI_YUV_ORDER_YUYV

/* VENC配置 */
/* 4M 无 PSRAM 内存档: 录像后物理池余量 ~34KB -> ~80-120KB, VGA+OSD 不再 P_MEM */
#define VIDEO_REC_QUALITY        10      // 编码质量(低=省首帧输出缓冲)
#define VIDEO_REC_VENC_ABR_KBPS  3000    // 码率(kbps)
#define VIDEO_REC_STREAM_BUF_SIZE (128 * 1024)  // 码流缓冲(q10 首帧 ~77KB, 128KB 留余量)
/* 同拍照根因(jpeg_encoder.c:out_buf_cal_size), 640*480*1.0(最坏系数)=307200B,
 * 原200KB对首帧偏紧, 提到384KB留余量(仅影响每次录像的第1帧, 后续帧自适应sizing)。 */

/* 工作线程配置 */
#define VIDEO_REC_WORKER_PRIO     10      // 线程优先级
#define VIDEO_REC_WORKER_STACK    2048    // 线程栈大小
#define VIDEO_REC_GET_STREAM_TIMEOUT_MS  100   // 获取码流超时(ms)

/*==============================================================================
 * 录像文件信息结构体
 *==============================================================================
 */
typedef struct {
    char path[128];        // 文件路径
    u32 create_time;      // 创建时间（秒）
    u32 file_size;        // 文件大小（字节）
    u8 locked;            // 是否加锁(只读属性)
    u8 in_use;            // 是否正在使用
} video_file_info_t;

/*==============================================================================
 * 循环录像上下文结构体
 *==============================================================================
 */
typedef struct {
    /* 基础状态 */
    u8 hw_ready: 1;           // 硬件是否就绪
    u8 recording: 1;          // 是否正在录像
    u8 worker_stop: 1;        // 工作线程停止标志
    u8 worker_abort: 1;       // 工作线程异常标志
    u8 paused: 1;           // 是否暂停
    u8 low_bat_alert_shown: 1;  // 低电量弹窗已显示(防500ms重复)
    /* t_26bc2b20: worker_exited 字段已废弃, 不再使用.
     * 旧实现 (task_create + pend worker_exited + task_kill) 用该标志做
     * 跨线程同步, 但标志位时序和 task_kill 资源回收分离 → 0 地址崩溃.
     * 新实现 (thread_fork + thread_kill KILL_WAIT) 由 OS 原子完成
     * "等待线程退出 + 回收 TCB/栈", 不再需要该字段.
     * 保留其位槽 (注释位) 暂不释放, 避免破坏未来若要回滚的兼容性. */

    int worker_pid;            // 工作线程PID (thread_fork 写入, thread_kill 时使用)

    /* 文件信息 */
    char current_file_path[128];   // 当前录像文件路径
    u32 current_file_create_time;  // 当前文件创建时间
    u32 file_index;               // 文件索引

    /* 循环录像配置 */
    u8 loop_time_sec;         // 循环时长（秒）

    /* 录像统计 */
    u32 record_start_time;        // 录像开始时间
    u32 segment_start_time;       // 当前段开始时间
    u32 pause_start_ms;           // 本次暂停起点(ms)
    u32 pause_accum_ms;           // 累计暂停时长(ms): UI 计时扣除, 恢复后时间不跳变
    u32 total_frames;             // 总帧数
    u32 total_segments;           // 录像段数

    /* AVI封装 */
    void *avi_file;           // AVI文件句柄(avi_t*)
    u32 avi_frame_count;      // AVI帧计数
    u32 avi_next_pts;        // 下一个PTS
    u32 avi_frame_interval_ms;// 帧间隔(ms)

    /* 帧计数定时器 */
    u16 frame_timer;        // 帧计数定时器句柄

    /* 状态机单一权威字段 (COD1-P0)
     * 由 video_rec_transition_to() 维护，与 recording/paused 位保持同步。
     * 旧代码读 recording/paused 仍可工作；新代码 (UI 回调) 读 current_state 更清晰。 */
    u32 current_state;      // enum video_rec_state

} video_rec_ctx_t;

/*==============================================================================
 * 录像控制接口
 *==============================================================================
 */

/**
 * 开始录像
 * @return 0:成功 -1:失败
 */
int video_rec_start(void);

/**
 * 停止录像
 * @return 0:成功 -1:失败
 */
int video_rec_stop(void);

/**
 * 暂停录像 (真暂停: 编码链路与 AVI 保持打开, 暂停期帧丢弃, 恢复后同一文件续录)
 * @return 0:成功 -1:失败
 */
int video_rec_pause(void);

/**
 * 恢复录像 (同一文件续录, 不新开文件)
 * @return 0:成功 -1:失败
 */
int video_rec_resume(void);

/*==============================================================================
 * 循环录像配置接口
 *==============================================================================
 */

/**
 * 设置循环录像时长
 * @param seconds 时长（LOOP_TIME_30/60/90_SEC）
 * @return 0:成功 -1:无效参数
 */
int video_rec_set_loop_time(u8 seconds);

/**
 * 获取当前录像文件路径
 * @return 当前正在录像的文件路径（未录像返回空字符串）
 */
const char *video_rec_get_current_file(void);

/*==============================================================================
 * 文件加锁接口 - 使用文件系统只读属性
 *==============================================================================
 */

/**
 * 加锁当前录像文件（设置只读属性）
 * @return 0:成功 -1:失败
 */
int video_rec_lock_current_file(void);

/**
 * 锁定指定文件
 * @param file_path 文件路径
 * @return 0:成功 -1:失败
 */
int video_rec_lock_file_by_path(const char *file_path);

/**
 * 解锁指定文件（清除只读属性）
 * @param file_path 文件路径
 * @return 0:成功 -1:失败
 */
int video_rec_unlock_file_by_path(const char *file_path);

/**
 * 检查文件是否加锁（检查只读属性）
 * @param file_path 文件路径
 * @return 1:加锁 0:未加锁
 */
int video_rec_is_file_locked_by_path(const char *file_path);

/*==============================================================================
 * 存储管理接口
 *==============================================================================
 */

/**
 * 获取存储空间信息
 * @param total_kb 总容量(KB)
 * @param free_kb 剩余容量(KB)
 * @return 0:成功 -1:失败
 */
int video_rec_get_storage_info(u32 *total_kb, u32 *free_kb);

/**
 * 检查并清理存储空间（删除最旧未加锁文件）
 * @return 0:空间足够 -1:空间不足
 */
int video_rec_check_storage(void);

/*==============================================================================
 * 状态查询接口
 *==============================================================================
 */

/**
 * 获取当前录像状态
 * @return 当前状态，见video_rec_state枚举
 */
enum video_rec_state video_rec_get_state(void);

/**
 * 获取录像总帧数
 * @return 总帧数
 */
u32 video_rec_get_total_frames(void);

/**
 * 获取录像段数
 * @return 录像段数
 */
u32 video_rec_get_total_segments(void);

int  video_rec_is_recording(void);      /* state==RECORDING快捷判断 */
u32  video_rec_get_elapsed_sec(void);   /* 当前录像已录制秒数 */

/*==============================================================================
 * SD 热插拔事件 (t_5c37af8d: 仅录像模式生效)
 *
 *   - app_default_msg_handler 在 DRIVER_EVENT_FROM_SD0 case 检测到当前为
 *     APP_MODE_VIDEO 时, 通过 app_send_message() 把 SD0 设备事件转发
 *     给当前模式的 video_app_msg_handler(), arg = DEVICE_EVENT_IN/OUT.
 *   - video_rec.c 在 video_app_msg_handler() 中处理:
 *       插卡 (DEVICE_EVENT_IN)  -> video_rec_start()  (空闲时)
 *       拔卡 (DEVICE_EVENT_OUT) -> video_rec_stop() + ERROR 等待态 + 弹窗
 *   - 取值 0xF000 避开 app_msg.h 主枚举空间 (避免与现有 APP_MSG_* 冲突).
 *==============================================================================
 */

/* 按键消息处理 — 由 video_rec_app_msg_handler.c 实现 */
extern int video_rec_app_msg_handler(int *msg);

#endif /* CONFIG_VIDEO_REC_H */
