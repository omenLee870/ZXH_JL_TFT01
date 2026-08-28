#ifndef __DSI_H__
#define __DSI_H__


//  enum
//**************************************
typedef enum { // clock mode
    DSI_CK_HS_ONLY
    , DSI_CK_HS_LPDT
    , DSI_CK_CONTINUE
} DSI_CLK_MODE;

typedef enum { // color code（颜色格式码）
    DSI_CC_RGB565
    , DSI_CC_RGB666
    , DSI_CC_RGB666_LOOSELY
    , DSI_CC_RGB888
} DSI_COLOR_CODE;

typedef enum { // color sort（颜色数据排列）
    DSI_CS_RGB565_DCS
    , DSI_CS_RGB565_VDM
    , DSI_CS_RGB666
    , DSI_CS_RGB666_LOOSELY
    , DSI_CS_RGB888
} DSI_COLOR_SORT;

typedef enum { // sync mode
    DSI_SYNC_PULSE
    , DSI_SYNC_EVENT
    , DSI_BURST_MODE
} DSI_SYNC_MODE;

typedef enum { // lane mapping
    DSI_MAP_DISABLE
    , DSI_MAP_CK_LANE
    , DSI_MAP_D0_LANE
    , DSI_MAP_D1_LANE
    , DSI_MAP_D2_LANE
    , DSI_MAP_D3_LANE
} DSI_MAP_TYPE;

/**
 * DSI 接口操作模式
 * - COMMAND 模式：主机通过指令控制外设，通常用于驱动控制器。
 * - VIDEO 模式：主机持续发送视频流数据，通常用于显示面板。
 */
typedef enum {
    DSI_COMMAND_MODE,
    DSI_VIDEO_MODE,
} DSI_DISPLAY_MODE;

/*
 * 包类型
 */
typedef enum {
    PACKET_DCS_WITH_BTA,		/* DCS包带响应 */
    PACKET_DCS_WITHOUT_BTA,		/* DCS包不带响应 */
    PACKET_GENERIC_BTA,  	    /* Generic包带响应 */
    PACKET_GENERIC_WITHOUT_BTA, /* Generic包不带响应 */
} PACKET_TYPE;

/*
 * 包分类
 */
typedef enum {
    BOTH_DCS_PACKET_AND_GENERIC_PACKET_AUTO,    /* DCS包和Generic包都有, 自动判断 */
    BOTH_DCS_PACKET_AND_GENERIC_PACKET_MANUAL,  /* DCS包和Generic包都有, 手动判断, 需在每条指令前面增加1字节的包类型标识 */
    ALL_DCS_PACKET_WITH_BTA,                    /* 仅有DCS包带响应类型(默认) */
    ALL_DCS_PACKET_WITHOUT_BTA,                 /* 仅有DCS包不带响应类型 */
    ALL_GENERIC_PACKET_WITH_BTA,                /* 仅有Generic包带响应类型 */
    ALL_GENERIC_PACKET_WITHOUT_BTA,             /* 仅有Generic包不带响应类型 */
} DSI_PACKET_CATEGORY;

typedef enum {
    BTA_ACK_RX_ACK = 0x0001,
    BTA_ACK_RX_TE_TRIGGER = 0x0002,
    BTA_ACK_RX_EOT = 0x0004,
    BTA_ACK_COMMAND_ERROR = 0x0008,
    BTA_ACK_SOT_ERROR = 0x0010, // SoT错误
    BTA_ACK_SOT_SYNC_ERROR = 0x0020, // SoT同步错误
    BTA_ACK_EOT_SYNC_ERROR = 0x0040, // EoT同步错误
    BTA_ACK_ESCAPE_MODE_ENTRY_COMMAND_ERROR = 0x0080, // 转义模式进入命令错误
    BTA_ACK_LOW_POWER_TRANSMIT_SYNC_ERROR = 0x0100, // 低功耗传输同步错误
    BTA_ACK_ANY_PROTOCOL_TIMER_TIMEOUT = 0x0200, // 任意协议定时器超时
    BTA_ACK_FALSE_CONTROL_ERROR = 0x0400, // 错误控件序列
    BTA_ACK_CONTENTION_IS_DETECTED_ON_THE_DISPLAY_MODULE = 0x0800, // 显示模组检测到总线占用
    BTA_ACK_ECC_ERROR_FOR_SINGLE_BIT = 0x1000, // ECC单比特错误(已检测,并纠正)
    BTA_ACK_ECC_ERROR_FOR_MULTI_BIT = 0x2000, // ECC多比特错误(已检测,未纠正)
    BTA_ACK_CHECKSUM_ERROR = 0x4000, // 校验和错误
    BTA_ACK_DSI_DATA_TYPE_NOT_RECOGNIZED = 0x8000, // DSI数据类型(DI)无法识别
    BTA_ACK_DSI_VIRTUAL_CHANNEL_INVALID = 0x10000, // DSI虚拟通道(VC)ID无效
    BTA_ACK_INVALID_TRANSMISSION_LENGTH = 0x20000, // 无效传输长度
    BTA_ACK_RESERVED = 0x40000,// 保留位,内部固定为0
    BTA_ACK_DSI_PROTOCOL_VIOLATION = 0x80000, // DSI协议违规
} BTA_ACK_ERROR_REPORT;

#endif
