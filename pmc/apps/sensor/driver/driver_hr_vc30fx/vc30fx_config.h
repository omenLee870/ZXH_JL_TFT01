
#pragma once

// VC30FX配置文件

// 采样频率配置
#define VC30FX_DEFAULT_ODR          25      // 默认采样频率(Hz)
#define VC30FX_PPG_BASE_FREQ        20000   // PPG基础频率(Hz)

// 电流和电阻配置
#define VC30FX_DEFAULT_CURRENT      0x40    // 默认电流
#define VC30FX_DEFAULT_RESISTANCE   9       // 默认电阻等级(4.48M)

// 佩戴检测配置
#define VC30FX_WEAR_PS_THRESHOLD    46080   // PS阈值(0xB400)
#define VC30FX_WEAR_BIO_DELTA       20      // BIO变化阈值

// 算法配置
#define VC30FX_HR_GAPTIME           40      // 心率采样间隔(ms)
#define VC30FX_SPORT_MODE           0       // 运动模式(日常)

// 调试配置
#define VC30FX_DEBUG_ENABLE         1       // 调试使能
#define VC30FX_LOG_ENABLE           1       // 日志使能

// 缓冲区配置
#define VC30FX_PPG_BUFFER_SIZE      64      // PPG缓冲区大小
#define VC30FX_PS_BUFFER_SIZE       32      // PS缓冲区大小
