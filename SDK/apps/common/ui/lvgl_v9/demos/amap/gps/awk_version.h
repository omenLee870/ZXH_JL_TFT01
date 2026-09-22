#ifndef _AWK_VERSION_H
#define _AWK_VERSION_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 版本号定义
 * 主版本号.次版本号.补丁版本号
 */
#define AWK_VERSION_MAJOR 3
#define AWK_VERSION_MINOR 3
#define AWK_VERSION_PATCH 0

/**
 * @brief 宏拼接辅助宏
 */
#define AWK_VERSION_STRINGIFY(x) #x
#define AWK_VERSION_TOSTRING(x) AWK_VERSION_STRINGIFY(x)

/**
 * @brief 版本号字符串（自动从主、次、补丁版本号拼接生成）
 */
#define AWK_VERSION_STRING \
    AWK_VERSION_TOSTRING(AWK_VERSION_MAJOR) "." \
    AWK_VERSION_TOSTRING(AWK_VERSION_MINOR) "." \
    AWK_VERSION_TOSTRING(AWK_VERSION_PATCH)

/**
 * @brief 版本号数值（用于版本比较）
 * 格式: (major * 10000 + minor * 100 + patch)
 */
#define AWK_VERSION_NUMBER ((AWK_VERSION_MAJOR * 10000) + (AWK_VERSION_MINOR * 100) + AWK_VERSION_PATCH)

/**
 * @brief 编译时间戳
 */
#define AWK_BUILD_DATE __DATE__
#define AWK_BUILD_TIME __TIME__

/**
 * @brief 版本信息结构体
 */
typedef struct _awk_version_info_t {
    int major;              //!< 主版本号
    int minor;              //!< 次版本号
    int patch;              //!< 补丁版本号
    const char *version;    //!< 版本字符串
    const char *build_date; //!< 编译日期
    const char *build_time; //!< 编译时间
} awk_version_info_t;

/**
 * @brief 获取头文件版本信息
 * @return 版本信息结构体（编译时确定）
 */
static inline awk_version_info_t awk_get_header_version(void)
{
    awk_version_info_t info = {
        .major = AWK_VERSION_MAJOR,
        .minor = AWK_VERSION_MINOR,
        .patch = AWK_VERSION_PATCH,
        .version = AWK_VERSION_STRING,
        .build_date = AWK_BUILD_DATE,
        .build_time = AWK_BUILD_TIME
    };
    return info;
}

/**
 * @brief 获取库文件版本信息（运行时获取）
 * @return 版本信息结构体指针
 */
const awk_version_info_t *awk_get_library_version(void);

/**
 * @brief 检查头文件和库文件版本是否一致
 * @return true表示一致，false表示不一致
 */
bool awk_check_version_compatibility(void);

/**
 * @brief 版本比较
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁版本号
 * @return 如果当前版本 >= 指定版本返回true，否则返回false
 */
static inline bool awk_version_at_least(int major, int minor, int patch)
{
    int version_number = (major * 10000) + (minor * 100) + patch;
    return AWK_VERSION_NUMBER >= version_number;
}

#ifdef __cplusplus
}
#endif

#endif /* _AWK_VERSION_H */
