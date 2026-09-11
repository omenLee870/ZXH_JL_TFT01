#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// 方案
#define SUB_CASE_TYPE_WATCH                 0                // 手表
#define SUB_CASE_TYPE_AI_GLASSES            1                // AI眼镜
#define SUB_CASE_TYPE_DASHBOARD             2                // 仪表盘
#define SUB_CASE_TYPE_KID_CAMERA            3                // 儿童相机

// 配置信息基本规则:
// 1、sdk_config.h   是由杰理可视化工具在线配置生成，原则上不建议客户直接修改sdk_config.h的文件，如果客户希望可视化工具不覆盖sdk_config.h，可以配置可视化工具脱机模式
// 2、board_config.h 是唯一包含有sdk_config.h和板卡配置的文件的，用户可以在自己的板卡对配置信息进行完善
// 3、app_config.h   包含有board_config.h
//
#include "sdk_config.h"
#include "audio_type.h"
#include "msys_pmc_app.h"

/*
 * 拍照内存受限方案开关:
 * 无 PSRAM 的方案内存受限, 大尺寸拍照走 "2K 阈值 + 整帧/分段 + 缩略" 三分支方案
 * (见 apps/common/.../app_camera_photo.c); 有 PSRAM 的方案内存充足, 保持原有拍照逻辑。
 * 按 TCFG_PSRAM_DEV_ENABLE 动态判定, 配置变更无需改代码。
 */
#define CONFIG_CAMERA_PHOTO_MEM_LIMIT  (!TCFG_PSRAM_DEV_ENABLE)

/*
 *  板级配置选择 (根据 sdk_config.h 的子项目宏 TCFG_SUB_CASE_TYPE 区分)
 */

#include "media/audio_def.h"

#if (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_DASHBOARD)
// 仪表盘 -> jl380n
#define CONFIG_BOARD_JL380N_DEMO
#include "board_jl380n_demo/board_jl380n_demo_cfg.h"

#elif (TCFG_SUB_CASE_TYPE == SUB_CASE_TYPE_KID_CAMERA)
// 儿童相机 -> ac561n
#define CONFIG_BOARD_AC561N_DEMO
#include "board_ac561n_demo/board_ac561n_demo_cfg.h"

#else // WATCH / AI_GLASSES -> jl681n
#define CONFIG_BOARD_JL681N_DEMO
#include "board_jl681n_demo/board_jl681n_demo_cfg.h"

#endif

#define  DUT_AUDIO_DAC_LDO_VOLT                 DACVDD_LDO_1_35V

#endif
