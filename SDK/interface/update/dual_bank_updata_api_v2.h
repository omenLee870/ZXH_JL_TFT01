#ifndef _DUAL_BANK_API_V2_H_
#define _DUAL_BANK_API_V2_H_

#include "typedef.h"

#define SUPPORT_DUAL_BANK_PASSIVE_UPDATE_V2 (1)

/**
 * @brief 被动更新错误码定义
 */
enum {
    // 基础返回码
    PUPDATE_RET_NONE = 0,
    PUPDATE_RET_PACKAGE_NO_CHECK = PUPDATE_RET_NONE,
    PUPDATE_RET_PACKAGE_MATCH_CHECK,
    PUPDATE_RET_PACKAGE_PID_NOMATCH,
    PUPDATE_RET_PACKAGE_VID_NOMATCH,
    PUPDATE_RET_PACKAGE_CRC_NOMATCH,

    // 核心错误码 (基础值50)
    PUPDATE_RET_CORE_BASE = PUPDATE_RET_NONE + 50,
    PUPDATE_RET_INVALID_PARM,                   ///< 无效参数
    PUPDATE_RET_MALLOC_FAILED,                  ///< 内存分配失败
    PUPDATE_RET_UNEXPECTED_ERROR,               ///< 未预期的错误
    PUPDATE_RET_NOT_SUPPORT_FUNC,               ///< 不支持的功能
    PUPDATE_RET_NOT_INIT,                       ///< 未初始化
    PUPDATE_RET_DATA_BUSY,                      ///< 数据忙
    PUPDATE_RET_PACKAGE_HEAD_ERROR,             ///< 包头错误
    PUPDATE_RET_WBUFF_OVERFLOW,                 ///< 写缓冲区溢出
    PUPDATE_RET_CBUFF_OVERFLOW,                 ///< 环形缓冲区溢出
    PUPDATE_RET_NOT_VALID_TARGET,               ///< 无效目标
    PUPDATE_RET_TARGET_INIT_ERR,                ///< 目标初始化错误
    PUPDATE_RET_TARGET_WRITE_ERR,               ///< 目标写入错误
    PUPDATE_RET_TARGET_VERIFY_ERR,              ///< 目标验证错误
    PUPDATE_RET_TARGET_EXIT_ERR,                ///< 目标退出错误
    PUPDATE_RET_TARGET_INFO_ERR,                ///< 目标信息错误
    PUPDATE_RET_WBUFF_PULL_ERR,                 ///< 写缓冲区拉取错误
    PUPDATE_RET_TARGET_PREPARE_ERR,             ///< 目标准备错误
    PUPDATE_RET_MSG_QUEUE_PUSH_ERR,             ///< 消息队列推送错误
    PUPDATE_RET_TARGET_READ_ERR,                ///< 目标读取错误
    PUPDATE_RET_TARGET_READ_OVERFLOW,           ///< 目标读取溢出
    PUPDATE_RET_TARGET_READ_NOT_ENOUGH,         ///< 目标读取不足
    PUPDATE_RET_TARGET_READ_TIMEOUT,            ///< 读取超时
    PUPDATE_RET_WRITE_NOT_32B_ALIGNED,          ///< 写入未按32字节对齐
    PUPDATE_RET_RE_INIT,                        ///< 重复初始化
    PUPDATE_RET_SAVE_BREAKPOINT_ERR,            ///< 保存断点错误
    PUPDATE_RET_SAVE_BREAKPOINT_PRIV_ERR,       ///< 保存断点私有数据错误
    PUPDATE_RET_BREAKPOINT_STEP_TOO_SMALL,      ///< 断点步进太小
    PUPDATE_RET_RESUME_BREAKPOINT_ERR,          ///< 恢复断点错误
    PUPDATE_RET_RESUME_BREAKPOINT_PRIV_CRC_ERR, ///< 恢复断点私有数据CRC错误
    PUPDATE_RET_RESUME_NOT_MATCH,               ///< 恢复不匹配
    PUPDATE_RET_TARGET_RESUME_ERR,              ///< 目标恢复错误
    PUPDATE_RET_TARGET_RESUME_NOT_SUPPORT,      ///< 目标不支持恢复
    PUPDATE_RET_RESUME_CHECK_OVERFLOW,          ///< 恢复检查溢出
    PUPDATE_RET_RESUME_CHECK_ERR,               ///< 恢复检查错误
    PUPDATE_RET_TARGET_SAVE_POINT_ERR,          ///< 目标保存点错误
    PUPDATE_RET_EXIT_TIMEOUT_ERR,               ///< 退出超时错误
    PUPDATE_RET_FLASH_READ_FAIL,                ///< Flash读取失败
    PUPDATE_RET_FLASH_WRITE_FAIL,               ///< Flash写入失败
    PUPDATE_RET_FLASH_WRITE_OVERFLOW,           ///< Flash写入溢出
    PUPDATE_RET_TARGET_WRITE_TIMEOUT,           ///< 写入超时错误
    PUPDATE_RET_BOOT_INFO_STASH_LEN_ERR,        ///< 启动信息暂存长度错误
    PUPDATE_RET_BOOT_INFO_STASH_NOT_MATCH,      ///< 启动信息暂存不匹配
    PUPDATE_RET_PACKAGE_HEAD_SIZE_ERROR,        ///< 包头大小错误
    PUPDATE_RET_PACKAGE_HEAD_MAGIC_ERROR,       ///< 包头魔数错误
    PUPDATE_RET_PACKAGE_HEAD_CRC_ERROR,         ///< 包头CRC错误
    PUPDATE_RET_PACKAGE_HEAD_FIRST_NOT_DIR,     ///< 包头首个不是目录
    PUPDATE_RET_CBUF_PUSH_OVERFLOW,             ///< 环形缓冲区推送溢出
    PUPDATE_RET_CBUF_PUSH_FAIL,                 ///< 环形缓冲区推送失败
    PUPDATE_RET_CBUF_POP_FAIL,                  ///< 环形缓冲区弹出失败
    PUPDATE_RET_PRASE_HEAD_LAST,                ///< 解析完包头后仍有数据
    PUPDATE_RET_PACKAGE_ALIGN_COFF_ERR,         ///< 包对齐系数错误
    PUPDATE_RET_TARGET_DEC_ALIGN_COFF_ERR,      ///< 目标解码对齐系数错误

    // 推送LOADER升级错误码 (基础值200)
    PUPDATE_RET_PL_BASE = PUPDATE_RET_NONE + 200,
    PUPDATE_RET_PL_REINIT_ERR,                  ///< 重复初始化错误
    PUPDATE_RET_PL_TARGET_AREA,                 ///< 目标区域错误
    PUPDATE_RET_PL_UNEXPECT_TYPE,               ///< 未预期类型
    PUPDATE_RET_PL_COM_HEAD_ERR,                ///< 通用头错误
    PUPDATE_RET_PL_OTA_OVERFLOW,                ///< OTA溢出
    PUPDATE_RET_PL_FLASH_WRITEBACK,             ///< Flash回写
    PUPDATE_RET_PL_WRITE_FAIL,                  ///< 写入失败
    PUPDATE_RET_PL_WRITE_OVERFLOW,              ///< 写入溢出
    PUPDATE_RET_PL_NO_MATCH_FILE,               ///< 无匹配文件
    PUPDATE_RET_PL_READ_FAIL,                   ///< 读取失败
    PUPDATE_RET_PL_BOOT_INFO_FAIL,              ///< 启动信息失败
    PUPDATE_RET_PL_NO_LV1_OTA,                  ///< 无LV1 OTA
    PUPDATE_RET_PL_READ_LC_CRC,                 ///< 读取LC CRC
    PUPDATE_RET_PL_APP0_HEAD_CRC,               ///< APP0头CRC
    PUPDATE_RET_PL_READ_DEV_FAIL,               ///< 读取设备失败
    PUPDATE_RET_PL_UPDIFF_OVERFLOW,             ///< 更新差分溢出
    PUPDATE_RET_PL_AREA_ILLEGAL,                ///< 区域非法
    PUPDATE_RET_PL_SAVEPOINT_FAIL,              ///< 保存点失败
    PUPDATE_RET_PL_RESUMEPOINT_OFF_ERR,         ///< 恢复点偏移错误
    PUPDATE_RET_PL_RESUMEPOINT_LEN_ERR,         ///< 恢复点长度错误
    PUPDATE_RET_PL_RESUMEPOINT_CHECK_ERR,       ///< 恢复点检查错误
    PUPDATE_RET_PL_RESUMEPOINT_VM,              ///< 恢复点VM错误
    PUPDATE_RET_PL_RESUMEPOINT_NOT_MATCH,       ///< 恢复点不匹配
    PUPDATE_RET_PL_RESUMEPOINT_TIMEOUT,         ///< 恢复点超市
    PUPDATE_RET_PL_ERASE_PROCESS_BREAK,         ///< 擦除过程中断
    PUPDATE_RET_PL_ERASE_AREA_ERR,              ///< 擦除区域错误
    PUPDATE_RET_PL_ERASE_AREA_OVERFLOW,         ///< 擦除区域溢出

    // 推送APPCORE升级错误码 (基础值300)
    PUPDATE_RET_APP_BASE = PUPDATE_RET_NONE + 300,
    PUPDATE_RET_APP_REINIT_ERR,                 ///< 应用重复初始化错误
    PUPDATE_RET_APP_LC_BOOTINFO_ERR,            ///< 应用LC启动信息错误
    PUPDATE_RET_APP_GET_LC_HEAD_ERR,            ///< 应用获取LC头错误
    PUPDATE_RET_APP_FILE_CHECK_ERR,             ///< ota包的app固件校验失败

    // 推送预留区域升级错误码 (基础值400)
    PUPDATE_RET_REV_BASE = PUPDATE_RET_NONE + 400,
    PUPDATE_RET_REV_OPEN_FAIL,                  ///< 预留区域打开失败

    // 推送SM_FLASH升级错误码 (基础值500)
    PUPDATE_RET_SM_BASE = PUPDATE_RET_NONE + 500,
    PUPDATE_RET_SM_REINIT_ERR,
    PUPDATE_RET_SM_OTA_OVERFLOW,
    PUPDATE_RET_SM_NO_LV1_OTA,
    PUPDATE_RET_SM_AREA_ILLEGAL,
    PUPDATE_RET_SM_NO_MATCH_FILE,
    PUPDATE_RET_SM_FLASH_WRITEBACK,
    PUPDATE_RET_SM_WRITE_FAIL,
    PUPDATE_RET_SM_WRITE_OVERFLOW,
    PUPDATE_RET_SM_READ_DEV_FAIL,
    PUPDATE_RET_SM_READ_FAIL,
    PUPDATE_RET_SM_SAVEPOINT_FAIL,
    PUPDATE_RET_SM_RESUMEPOINT_OFF_ERR,
    PUPDATE_RET_SM_RESUMEPOINT_LEN_ERR,
    PUPDATE_RET_SM_RESUMEPOINT_CHECK_ERR,
    PUPDATE_RET_SM_RESUMEPOINT_NOT_MATCH,
    PUPDATE_RET_SM_UNEXPECT_TYPE,

    // DBSM_FLASH升级APP错误码 (基础值600)
    PUPDATE_RET_SMDB_BASE = PUPDATE_RET_NONE + 500,
    PUPDATE_RET_SMDB_FIRST_PACKAGE_LEN_ERR,
    PUPDATE_RET_SMDB_HEAD_MAGIC_ERR,
    PUPDATE_RET_SMDB_DECOM_ERR,
    PUPDATE_RET_SMDB_DECOM_UNKNW_TAG,
    PUPDATE_RET_SMDB_PACK_BOOTINFO_ERR,
    PUPDATE_RET_SMDB_VERIFY_ERR,
    PUPDATE_RET_SMDB_CBUFF_FULL,

};

/**
 * @brief 被动升级初始化拓展参数
 * @param upper_buf 上层传递给底层用的计算缓存，该缓存大小必须>=(max_pkt_len+(96B))
 * @param ubuf_len 上层传递给底层用的计算缓存大小必须>=(max_pkt_len+(96B))
 * @param task_en 升级库内是否创建线程
 * @node   进一步优化内存使用，在passive_update.h开启PASSIVE_UPDATE_V2_MALLOC_SBUF宏，
 *         ubuf_len则需要根据实际情况调试,调试方法是先传递一个足够大的缓存，
 *         成功升级一次看最后一次打印:“pupdate sbuf alloc max: xxxxB”，
 *         然后ubuf_len最小可以配置成(xxxxB+32B)
 */
struct db_param_ext {
    void *upper_buf;
    u32 ubuf_len;
    u8 task_en;
};



/**
 * @brief 初始化更新任务，并设置新固件的CRC值和文件大小
 * @param fw_crc 新固件文件的CRC值
 * @param fw_size 新固件文件的总大小
 * @param max_pkt_len 支持的最大编程长度，决定每次编程的最大大小
 * @param ex_param 拓展被动升级参数，见上
 * @return 初始化结果，0表示成功，其他值表示失败
 */
int dual_bank_passive_update_init_v2(u32 fw_crc, u32 fw_size, u32 max_pkt_len, struct db_param_ext *ex_param);


/**
 * @brief 退出更新任务
 * @param err 0表示无错误，其他值表示失败
 * @return 退出结果，0表示成功，其他值表示失败
 */
int dual_bank_passive_update_exit_v2(int err);

/**
 * @brief 将数据复制到临时缓冲区并通知任务写入非易失性存储器
 * @param data 指向下载数据的指针
 * @param len 下载数据的长度
 * @param write_complete_cb 编程完成回调，无错误发生时返回0
 * @return 写入结果，0表示成功，其他值表示失败
 */
int dual_bank_update_write_v2(void *data, u32 len, int (*write_complete_cb)(void *priv));

/**
 * @brief 检查OTA包是否有效
 * @return 检查结果，0表示有效，其他值表示无效
 */
int dual_bank_update_package_valid_v2(void);

/**
 * @brief 写入boot_info并立即生效，重启后会立刻进行升级包安装，异步接口
 * @param misc_buff 用户需要传递给底层的信息，可以为NULL
 * @param misc_len  用户需要传递给底层的信息长度，可以为0，目前最大256字节
 * @param burn_boot_info_result_hdl 应用层回调函数，底层返回保存boot_info结果，
 *                                  err为0代表成功，其他值代表失败
 * @return 0代表成功，其他值代表失败
 */
int dual_bank_update_burn_boot_info_v2(u8 *misc_buff, u32 misc_len, int (*burn_boot_info_result_hdl)(int err));

/**
 * @brief 该函数功能一般用于静默升级，写入boot_info，但是暂存起来，传递给应用层暂存，不会立刻写入
 * @param misc_buff 用户需要传递给底层的信息，可以为NULL
 * @param misc_len  用户需要传递给底层的信息长度，可以为0，目前最大256字节
 * @param boot_info_stash_hdl 应用层回调暂存boot_info回调函数，应用层实现，
 *                            其中err代表底层暂存boot_info结果，stash_buff
 *                            即为需要应用层暂存的内容（长度最大256），例如
 *                            可以写在VM里
 *                            回调返回0成功，其他值代表失败
 * @return 0代表成功，其他值代表失败
 */
int dual_bank_update_burn_boot_info_stash_v2(u8 *misc_buff, u32 misc_len, int (*boot_info_stash_hdl)(int err, u8 *stash_buff, u8 len));

/**
 * @brief 该函数功能一般用于静默升级，将此前暂存起来的boot_info，真正写入
 * @param stash_buff 此前stash回调中暂存的boot_info数据
 * @param len stash_buff的长度，注意长度最大256
 * @return 0代表成功，其他值代表失败
 */
int dual_bank_update_burn_boot_info_apply_v2(u8 *stash_buff, u8 len);

/**
 * @brief 此API供用户读取flash数据以计算CRC
 * @param offset 相对于更新区域的偏移量
 * @param read_buf 用户数据缓冲区
 * @param read_len 读取长度
 * @return 实际读取长度
 */
int dual_bank_update_read_data_v2(u32 offset, u8 *read_buf, u32 read_len);

/**
 * @brief 验证更新结果
 * @param verify_result_hdl 验证结果回调函数,ret返回0是成功，其他是失败
 * @return 验证结果，0表示成功，其他值表示失败
 */
int dual_bank_update_verify_v2(int (*verify_result_hdl)(int ret));

/**
 * @brief 注册升级流程用户暂停回调函数
 * @param user_pause_hdl 应用层回调处理函数，底层在擦写flash过程中会调用该回调，应用
 *                       可以在该回调函数内进行例如线程休眠，或判断到某些状态直接杀死升级等动作
 *                       ret为0代表流程仍在进行，为1代表流程结束
 *                       回调返回0代表升级继续，返回其他值代表升级中断
 * @return 0代表成功，其他代表失败
 */
int dual_bank_update_pause_register_v2(int (*user_pause_hdl)(int ret));

/**
 * @brief 提前处理一些事务，例如在开机时调用提前擦除升级区域
 * @return 同步接口，0代表成功，其他代表失败
 */
int dual_bank_update_early_handle_v2(void);

/**
 * @brief 用户断点结构体定义
 */
struct user_breakpoint {
    /* 断点步进，每隔step个字节保存一次断点 */
    u32 step;

    /**
     * @brief 保存底层断点续传的相关私有数据回调函数
     * @param priv_data 底层断点续传的相关私有数据
     * @param priv_len 私有数据长度
     * @param info_len 每个断点的信息的长度
     * @return 0代表成功，其他代表失败
     */
    int (*save_priv_data)(void *priv_data, u32 priv_len, u32 point_len);

    /**
     * @brief 保存底层断点回调函数，用户层实现
     * @param offset 断点的偏移
     * @param point_info 该断点的信息，需要应用层保存, 长度为此前的point_len
     * @return 0代表成功，其他代表失败
     */
    int (*save_breakpoint)(u32 offset, void *point_info);
};

/**
 * @brief 断点续传初始化
 * @param ub 用户的断点功能结构体，结构体如上定义
 * @return 成功返回0，其他则失败
 */
int dual_bank_passive_update_init_breakpoint_v2(struct user_breakpoint *ub);

/**
 * @brief 断点恢复
 * @param offset 断点偏移
 * @param point_info 此前底层传递给应用的断点信息
 * @param info_len 断点信息长度
 * @param priv_data 此前底层传递给应用的私有数据
 * @param priv_len 私有数据长度
 * @return 成功返回0，其他则失败
 */
int dual_bank_passive_update_resume_breakpoint_v2(u32 offset, void *point_info, u32 info_len, void *priv_data, u32 priv_len);


#define DUAL_BANK_PASSIVE_V1_PORTING_V2
#ifdef DUAL_BANK_PASSIVE_V1_PORTING_V2

static inline u32 dual_bank_passive_update_init(u32 fw_crc, u32 fw_size, u16 max_pkt_len, void *priv)
{
    return dual_bank_passive_update_init_v2(fw_crc, fw_size, max_pkt_len, priv);
}
static inline u32 dual_bank_passive_update_exit(void *priv)
{
    return dual_bank_passive_update_exit_v2(0);
}
static inline u32 dual_bank_update_allow_check(u32 fw_size)
{
    return 0;
}
static inline u32 dual_bank_update_write(void *data, u16 len, int (*write_complete_cb)(void *priv))
{
    return dual_bank_update_write_v2(data, len, write_complete_cb);
}
static inline u8 dual_bank_update_verify_without_crc(void)
{
    return dual_bank_update_verify_v2(NULL);
}
static inline u32 dual_bank_update_verify(void (*crc_init_hdl)(void), u32(*crc_calc_hdl)(u32 init_crc, const void *data, u32 len), int (*verify_result_hdl)(int calc_crc))
{
    return dual_bank_update_verify_v2(verify_result_hdl);
}
static inline u8 dual_bank_update_verify_without_crc_new(int (*verify_result_hdl)(int calc_crc))
{
    return dual_bank_update_verify_v2(verify_result_hdl);
}
static inline u32 dual_bank_update_burn_boot_info(int (*burn_boot_info_result_hdl)(int err))
{
    return dual_bank_update_burn_boot_info_v2(NULL, 0, burn_boot_info_result_hdl);
}

#endif

#endif
