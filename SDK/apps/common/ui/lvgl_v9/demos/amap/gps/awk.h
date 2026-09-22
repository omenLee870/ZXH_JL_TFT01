#ifndef _AWK_H
#define _AWK_H

#include "awk_adapter.h"
#include "awk_version.h"
#include "map/awk_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTOS SDK版本（保持向后兼容）
 * @deprecated 请使用 awk_version.h 中的版本定义
 */
#define AWK_VERSION AWK_VERSION_STRING

/**
 * @brief 语言枚举，默认简体中文
 */
typedef enum _awk_language_t {
    AWK_LANGUAGE_ZH_HANS = 0,
    AWK_LANGUAGE_EN = 1,
    AWK_LANGUAGE_ES = 2,
    AWK_LANGUAGE_PT = 3,
    AWK_LANGUAGE_FR = 4,
    AWK_LANGUAGE_DE = 5,
    AWK_LANGUAGE_TH = 6,
    AWK_LANGUAGE_JA = 7,
    AWK_LANGUAGE_KO = 8,
    AWK_LANGUAGE_AR = 9,
    AWK_LANGUAGE_TR = 10,
    AWK_LANGUAGE_HE = 11,
    AWK_LANGUAGE_IT = 12,
    AWK_LANGUAGE_RU = 13,
    AWK_LANGUAGE_MS = 14,
    AWK_LANGUAGE_ID = 15,
    AWK_LANGUAGE_VI = 16,
    AWK_LANGUAGE_PL = 17,
    AWK_LANGUAGE_CS = 18,
    AWK_LANGUAGE_UK = 19,
    AWK_LANGUAGE_AZ = 20,
    AWK_LANGUAGE_ZH_HANT = 21,
} awk_language_t;

/**
 * @brief POI/RoadName 数据源枚举，默认国内数据
 */
typedef enum _awk_poi_data_source_t {
    AWK_POI_DATA_SOURCE_INVALID = 0,
    AWK_POI_DATA_SOURCE_MAINLAND = 1,
    AWK_POI_DATA_SOURCE_OVERSEAS = 2,
} awk_poi_data_source_t;

/**
 * @brief 上下文
 */
typedef struct _awk_context_t {
    char *device_id;                           //!< 设备id
    char *key;                                 //!< 开放平台key,需要使用高德开放平台申请的智能硬件key
    char *root_dir;                            //!< SDK内部文件夹根路径
    char *offline_map_dir;                     //!< 下载好的离线地图路径
    char *ext_res_path;                        //!<外挂的离线map style 和图版路径

    bool tile_zip;                             //!< 瓦片是否压缩
    bool tile_clip_load;                       //!< 瓦片在加载时是否进行裁剪，只有当tile_mem_cache_max_size为0时才有效
    bool tile_background_custom_draw;          //!< 瓦片背景（背景色&网格线）是否自行绘制
    bool tile_buff_mem_outer_free;             //!< 瓦片绘制内存是否由外部释放，true表示由外部释放，false表示由SDK内部释放，只有tile_clip_load为false时生效
    bool tile_cache_decoded_bitmap;            //!< 缓存瓦片时是否缓存解码后的位图
    bool poi_labels_hidden;                    //!< 设置图面是否显示poi,不建议变更此配置，变更后已缓存数据不会刷新
    bool road_labels_hidden;                   //!< 设置图面是否显示路名,不建议变更此配置，变更后已缓存数据不会刷新
    int32_t tile_load_mode;                    //!< 地图瓦片加载类型，离线，在线，在线离线混合，具体枚举值见：awk_map_tile_load_mode_t
    awk_pixel_mode_t tile_pixel_mode;          //!< 地图瓦片像素格式
    bool disable_network_gzip_header;           //!< 是否关闭网络gzip头
    uint32_t tile_disk_cache_max_size;         //!< tile磁盘缓存最大空间，单位: MB
    uint32_t tile_mem_cache_max_size;          //!< tile内存缓存最大空间，单位：KB
    uint32_t poi_tile_disk_cache_max_size;     //!< poi磁盘缓存最大空间，单位: MB
    uint32_t poi_tile_mem_cache_max_size;      //!< poi瓦片内存缓存最大空间，单位：KB
    awk_map_tile_style_t tile_style;           //!< 瓦片样式类型
    awk_map_tile_svg_data_t svg_tile_data_type; //!< svg瓦片数据类型 默认xml，支持fbs二进制
    uint32_t poi_tile_density;                 //!< 地图瓦片精细度，数值越高精细度越好，共三档（0：最差，1：适中，2：最高）
    uint32_t max_file_count_in_dir;            //!< 一个文件目录最大文件数，0表示不限定文件数
    uint32_t max_one_file_size;                //!< 一个文件最大大小，0表示不限定文件大小，单位：MB
    bool need_force_render;                    //!< 是否需要强制刷新
    awk_render_adapter_t render_adapter;       //!< 绘制相关代理
    awk_file_adapter_t file_adapter;           //!< 文件io相关代理
    awk_memory_adapter_t memory_adapter;       //!< 内存相关代理
    awk_fast_memory_adapter_t fast_memory_adapter;  //!< 内存相关代理(针对大内存进行优化)
    awk_network_adapter_t network_adapter;     //!< 网络相关代理
    awk_thread_adapter_t thread_adapter;       //!< 线程相关代理
    awk_system_adapter_t system_adapter;       //!< 其他系统相关代理
    awk_map_tile_file_adapter_t tile_file_adapter; //!< 瓦片文件相关代理
    awk_map_custom_adapter_t custom_adapter;     //!< 其他相关代理(仅针对部分特殊客户)
    bool pre_decode_enable;                      //!< 是否启用瓦片预解码（下载时即解码），默认false
    awk_pre_decode_adapter_t pre_decoder;         //!< 预解码适配器（pre_decode_enable=true时必填）
    float limit_min_zoom;                         //!< 限制最小缩放级别 最小支持3级
    float limit_max_zoom;                         //!< 限制最大缩放级别 最大支持20级
    awk_language_t language_type;                 //!< 语言枚举，默认AWK_LANGUAGE_ZH_HANS
    awk_poi_data_source_t poi_data_source;        //!< POI/RoadName数据源，默认AWK_POI_DATA_SOURCE_MAINLAND
} awk_context_t;


/**
 * @brief 地图环境初始化，方法的调用需要在主流程线程中
 * @param {awk_context} *context 环境参数
 * @return {成功返回0, 失败返回错误码: -1表示已经初始化，不能重复初始化, -100表示参数context为空, -101表示context.key为空，-102表示context.device_id为空，-103表示context.root_dir为空，-104表示pre_decode_enable为true但pre_decoder.decode为空，-200开头错误码表示参数context.render_adapter的函数指针为空，-300开头错误码表示参数context.file_adapter的函数指针为空，-400开头错误码表示参数context.memory_adapter的函数指针为空，-500开头错误码表示参数context.system_adapter的函数指针为空, -600开头错误码表示参数context.network_adapter的函数指针为空 -700开头错误码表示参数context.thread_adapter的函数指针为空，-800开头错误码表示参数context.tile_file_adapter的函数指针为空，-900表示POI功能被禁用但tile_style需要POI支持，-901表示SVG功能被禁用但tile_style需要SVG支持，-902表示TILE功能被禁用但tile_style需要TILE支持}
 */
int32_t awk_init(awk_context_t *context);

/**
 * @brief 地图环境反初始化，方法的调用需要在主流程线程中
 * @return {成功返回0, 失败返回错误码：-1表示没有初始化 -3初始化的线程和当前调用不一致}
 */
int32_t awk_uninit(void);

// 设备激活相关参数
typedef struct _awk_device_activate_param {
    char *area;                 //!< 区域（mainland - 国内, overseas - 海外）
    char *country;              //!< 地图主张 cn
    int type;                   //!< 恢复出厂设置（0 -- 新激活， 1 -- 恢复出厂设置， 2 -- 续约，其他会报错）无法区分是否恢厂状态则一律传0
    char *data_type;            //!< 数据类型 raster栅格地图，vector矢量地图
} awk_device_activate_param_t;

typedef struct _awk_device_active_callback {
    /**
     * @brief 设备激活成功回调, 方法的调用需要在主流程线程中
     * @param {char*} device_id 成功返回device_id, 后续使用需要拷贝
     * @param {license_id} result 成功返回license_id, 后续使用需要拷贝
     * @return {*}
     */
    void (*awk_device_active_on_success) (const char *device_id, const char* license_id);

    /**
     * @brief 设备激活失败回调, 方法的调用需要在主流程线程中
     * @param {int} code 错误码 {30046 license数量超过限制} {30047 license已存在} {30048 license已被禁用} {30049 license超过有效期}
     * @param {char*} msg 错误信息
     */
    void (*awk_device_active_on_fail) (int code, const char* msg);
} awk_device_active_callback;

/**
 * @brief 激活设备，需要在设备初次使用时调用，需要保证网络可用，未激活会导致SDK不可用，方法的调用需要在主流程线程中
 * @param {awk_device_active_callback} callback 回调结果
 */
void awk_activate_device(awk_device_activate_param_t *activate_param, awk_device_active_callback callback);

/**
 * @brief 判断设备是否已激活，未激活会导致SDK不可用，方法的调用需要在主流程线程中
 * @param {awk_context_t} 初始化参数,必填项：
 *            .root_dir
 *            .device_id
 *            .file_adapter:file_open/file_read/file_close
 *            .memory_adapter:mem_malloc/mem_free
 *            .system_adapter:get_system_time
 */
bool awk_check_device_activated(awk_device_activate_param_t *activate_param, awk_context_t *context);

/**
 * @brief 清除磁盘缓存, 方法的调用需要在主流程线程中
 * @return {成功返回0, 失败返回错误码：-1表示没有初始化，-3初始化的线程和当前调用不一致}
 */
int32_t awk_clear_disk_cache(void);

typedef enum {
    AWK_MEMORY_CACHE_TYPE_TILE = 1 << 0, // 瓦片
    AWK_MEMORY_CACHE_TYPE_POI = 1 << 1, // poi
} awk_memory_cache_type_t;

/**
 * @brief 清除内存缓存, 方法的调用需要在主流程线程中
 * @param {awk_memory_cache_type_t} type 内存缓存类型
 * @return {成功返回0, 失败返回错误码：-1表示没有初始化，-3初始化的线程和当前调用不一致}
 */
int32_t awk_clear_memory_cache(awk_memory_cache_type_t type);

typedef enum {
    AWK_TILE_STYLE_NONE = 0, // 无
    AWK_TILE_STYLE_GRID = 1 << 0, // 栅格图
    AWK_TILE_STYLE_VECTOR = 1 << 1, // 矢量图
} awk_tile_style_t;

/**
 * @brief 获取某路径下瓦片类型
 * @param {char *} dir_path 路径非空
 * @return awk_tile_style_t
 */
awk_tile_style_t awk_tile_style_with_path(char *dir_path);
#ifdef __cplusplus
}
#endif

#endif
