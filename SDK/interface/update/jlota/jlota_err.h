#ifndef JLOTA_ERROR_H
#define JLOTA_ERROR_H

#include "generic/typedef.h"
#include "stdarg.h"

/**
 * JL-OTA 错误码枚举定义
 */
typedef enum {
    JLOTA_SUCCESS                            = 0,

    JLOTA_ERROR_NONE                         = 0,
    JLOTA_ERR_NO_START,                      // 升级流程未开始就结束了
    JLOTA_ERR_NO_MEM,                        // 内存不足
    JLOTA_ERR_INVAL_ARG,                     // 无效参数
    JLOTA_ERR_SOURCE_TYPE_NOT_SUPPORT,       // 不支持的源类型
    JLOTA_ERR_PID_NOT_MATCH,                 // PID 不匹配
    JLOTA_ERR_VID_NOT_MATCH,                 // VID 不匹配
    JLOTA_ERR_VM_SIZE,                       // VM 大小错误
    JLOTA_ERR_READ_END,                      // 读取结束
    JLOTA_ERR_CRC_FAIL,                      // CRC 校验失败
    JLOTA_ERR_READ_FAIL,                     // 读取失败
    JLOTA_ERR_WRITE_FAIL,                   // 写入失败
    JLOTA_ERR_ADDR_NOT_MATCH,               // 地址不匹配
    JLOTA_ERR_FILE_SIZE_NOT_MATCH,          // 文件大小不匹配
    JLOTA_ERR_CANCEL,                       // 操作取消
    JLOTA_ERR_DECODE_LEN,                   // 解密长度错误
    JLOTA_ERR_ADDR_NOT_ALIGNED,             // 地址未对齐
    JLOTA_ERR_FILE_NOT_EXIST,               // 文件不存在
    JLOTA_ERR_OTA_REPEAT_INIT,              // OTA 重复初始化
    JLOTA_ERR_OTA_UNKONW_UPGRADE_TYPE,
    JLOTA_ERR_OTA_NOMATCH_UPGRADE_TYPE,     //不匹配的升级方式，检查是双备份升级还是单备份
    JLOTA_ERR_OTA_NOT_STOP,
    JLOTA_ERR_UFW_OPEN_FAIL,// UFW 相关错误码
    JLOTA_ERR_UFW_READ_FILE,                // UFW 读取文件失败
    JLOTA_ERR_UFW_SYD_HEAD_V1,              // UFW SYD 头版本错误
    JLOTA_ERR_UFW_FW_HEAD_V1,               // UFW 固件头版本错误
    JLOTA_ERR_UFW_NO_INIT,                  // UFW 未初始化
    JLOTA_ERR_UFW_FW_FILE_NUM,              // UFW 固件文件数量错误
    JLOTA_ERR_UFW_FILE_CRC,                 // UFW 文件 CRC 错误
    JLOTA_ERR_UFW_NO_FLASH_BIN,             // UFW 无 Flash 二进制文件
    JLOTA_ERR_UFW_KEY_ERR,                  // UFW 密钥错误
    JLOTA_ERR_UFW_NO_VAIL_FLASHBIN,
    JLOTA_ERR_UFW_FLASHBIN_HEAD_ERR,
    JLOTA_ERR_UFW_NOT_MATCH_OTA_BIN,        //UFW内找不到匹配的loader
    JLOTA_ERR_VM_SIZE_NOT_ENOUGHT,// VM 相关错误码 = 34         // VM 空间不足
    JLOTA_ERR_VM_PRASE_FAIL,              // VM 区域解析失败
    JLOTA_ERR_VM_DEFRAG_FAIL,              // VM 碎片整理失败
    JLOTA_ERR_VM_READ_FAIL,              // VM 读取失败
    JLOTA_ERR_VM_BACKUP_FAIL,              // VM 备份失败
    JLOTA_ERR_VM_BAK_EARLY_UPARAM,            // VM 备份时早期写入updata_param失败
    JLOTA_ERR_VM_NOT_FIT_UPDATE,            // VM 可用空间不足放入loader和升级参数以及VM备份
    JLOTA_ERR_VM_LOCAL_OVERWRITE,            // VM 本地VM在推送loader中可能会出现写越界，禁止写入
    JLOTA_ERR_VM_NEW_OVERWRITE,            // VM 新固件VM在在还原时可能会出现写越界，禁止写入
    JLOTA_ERR_VM_AREA_TOTAL_DIFF,            // VM 新旧固件的VM区域完全没有重叠的空间
    JLOTA_ERR_VM_NEW_AREA_TOO_SMALL,            // VM 新固件的VM区域一半，放不下当前的所有VM数据
    JLOTA_ERR_LOADER_CRC_FAIL,             // 加载器 CRC 失败
    JLOTA_ERR_LOADER_WRITE_HEAD_FAIL,       // loader写入头部失败/或vm备份时先写入update_param失败
    JLOTA_ERR_LOADER_WRITE_FAIL,          // loader写入失败
    JLOTA_ERR_LOADER_READ_FAIL,          // loader读取失败
    JLOTA_ERR_LOADER_CHECK_FAIL,          // loader推送完毕后检验失败
    JLOTA_ERR_UPDATA_PARAM_CHECK_FAIL,          // loader推送完毕后检验失败
    JLOTA_ERR_PART_NOT_INIT, //jlota_part 相关错误码
    JLOTA_ERR_PART_RE_INIT,
    JLOTA_ERR_PART_HAVE_REG,
    JLOTA_ERR_PART_READ_OVERFLOW,
    JLOTA_ERR_PART_READ_LEN_ERR,
    JLOTA_ERR_PART_WRITE_OVERFLOW,
    JLOTA_ERR_PART_WRITE_LEN_ERR,
    JLOTA_ERR_PART_ERASE_OVERFLOW,
    JLOTA_ERR_PART_ERASE_LEN_ERR,
    JLOTA_ERR_PART_NOT_READABLE,
    JLOTA_ERR_PART_NOT_WRITEABLE,
    JLOTA_ERR_PART_OPEN_FAIL,
    JLOTA_ERR_PART_NOT_OPEN_LCFLASH,
    JLOTA_ERR_PART_SCAN_NODE_ERR,
    JLOTA_ERR_PART_CREATE_ERR,
    JLOTA_ERR_PART_NOT_OPS_ERR,
    JLOTA_ERR_PART_NOT_OPEN_UFW,
    JLOTA_ERR_PART_NAME_TOO_LONG,
    JLOTA_ERR_PART_FIND_NOT_DESC,
    JLOTA_ERR_PART_ROOT_NO_DESC,
    JLOTA_ERR_PART_COPY_CHECK_ERR,
    JLOTA_ERR_LCFLASH_OPEN_FAIL,// 本地flash操作 相关错误码
    JLOTA_ERR_LCFLASH_APP_OPEN_FAIL,
    JLOTA_ERR_LCFLASH_APP_PATH_ERR,
    JLOTA_ERR_LCFLASH_APP_SINGLE,
    JLOTA_ERR_LCFLASH_HEAD_V2_ERR,
    JLOTA_ERR_LCFLASH_READ_FILE,
    JLOTA_ERR_IDU_SMFLAHS_HEAD_ERR,//jlota idu安全升级错误码, smflash的头部错误
    JLOTA_ERR_IDU_PRASE_DEC_KEY_ERR,//解析对称加密key失败
    JLOTA_ERR_IDU_PHY_TO_LOGIC_ERR,//物理块转换为逻辑块失败
    JLOTA_ERR_IDU_FHEAD_CHECK_FAIL,//压缩块头部校验失败
    JLOTA_ERR_IDU_RE_BOOTINFO_CHECK_FAIL,//smflash的appcore bootinfo检查失败
    JLOTA_ERR_IDU_RE_APPCORE_SIZE_ERR,//smflash的appcore长度太长
    JLOTA_ERR_IDU_RE_UINFO_NO_VMINFO,//远端的uinfo内没找到VM信息
    JLOTA_ERR_OTA_FS_FILE_NO_DIR,// jlot_afs 相关错误码
    JLOTA_ERR_OTA_FS_OPEN_ROOT,// 打开根节点失败
    JLOTA_ERR_OTA_FS_NO_EXIT_PATH,// 不存在的文件路径名
    JLOTA_ERR_OTA_FS_INV_FILE_TYPE,// 不识别的文件类型
    JLOTA_ERR_DB_UPDATE_UFW_KEY_ERR,//双备份升级 相关错误码
    JLOTA_ERR_DB_UPDATE_UFW_BOOTINFO_ERR,
    JLOTA_ERR_DB_UPDATE_COPY_ERR,
    JLOTA_ERR_DB_UPDATE_APPCORE_AREA_NOT_MATCH,//远端与本地的app区域不一致，分界线发生改变
    JLOTA_ERR_USER_INIT_ERR, //主动升级用户配置 相关错误码
    JLOTA_ERR_USER_INPUT_TYPE_UNKNOW,//用户配置了未知的升级类型
    JLOTA_ERR_USER_INPUT_NO_OTA_NAME, //用户在单备份升级模式下，没有配置ota文件名称
    JLOTA_ERR_USER_CHIP_NO_INIT,             // user_chip 模块未初始化
    JLOTA_ERR_USER_CHIP_ALREADY_INIT,        // user_chip 模块重复初始化
    JLOTA_ERR_USER_CHIP_UPDATE_FAIL,         // user_chip 升级执行失败
    JLOTA_ERR_LOADER_UPDATE_FINAL_CHECK_ERR, //一下为loader阶段错误码
    JLOTA_ERR_RESERVE_AREA_CHANGE_ERR, //预留区域升级错误码//预留区域位置发生改变
    JLOTA_RET_STATE, //返回状态，非错误
    JLOTA_RET_JUMP_TO_LOADER,//可跳入loader




} jlota_err_t;

#define ERR_LEVEL0  0
#define ERR_LEVEL1  1
#define ERR_LEVEL2  2
#define ERR_LEVEL3  3

struct err_code_storage {
    union {
        u32 raw;
        struct {
            u8 level0;
            u8 level1;
            u8 level2;
            u8 level3;
        } bytes;
    };
};



extern void jlota_save_err_code(int err, int lv);
extern void jlota_print_err_code(int err, const char *file, int line, const char *fmt, ...);

#define JLOTA_RETURN(err) \
    do { \
        jlota_print_err_code(err, __FILE__, __LINE__, NULL); \
        return -err; \
    } while (0)

#define JLOTA_RETURN_FORMAT(err, format, ...) \
    do { \
        jlota_print_err_code(err, __FILE__, __LINE__, format, __VA_ARGS__); \
        return -err; \
    } while (0)

#define JLOTA_ERR(err) \
    jlota_print_err_code(err, __FILE__, __LINE__, NULL);

#define JLOTA_ERR_FORMAT(err, format, ...) \
    jlota_print_err_code(err, __FILE__, __LINE__, format, __VA_ARGS__);

#define JLOTA_ERR_SAVE(err, lv) \
    do { \
        jlota_save_err_code(err, lv);\
    } while (0)



#endif
