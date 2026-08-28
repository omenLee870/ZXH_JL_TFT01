#ifndef __GPADC_DEF_H__
#define __GPADC_DEF_H__

#include "typedef.h"

// ===============================
// 通道编码格式（32bit）
// ===============================
//
// [31:24] main 通道 (ADC_MUX_SEL)
// [23:16] sub  通道 (ADC_ASEL)
// [ 7:0 ] next 通道： [7:3] main, [2:0] sub
//

#define ADC_CHANNEL_CODE_EX(main, sub, next_main, next_sub) \
    ((((uint32_t)(main) & 0xFF) << 24) | \
     (((uint32_t)(sub)  & 0xFF) << 16) | \
     ((((uint32_t)(next_main) & 0x1F) << 3) | ((uint32_t)(next_sub) & 0x07)))

#define ADC_CHANNEL_CODE(main, sub, next) \
    ((((uint32_t)(main) & 0xFF) << 24) | \
     (((uint32_t)(sub)  & 0xFF) << 16) | \
     ((uint32_t)(next) & 0xFF))

#define ADC_GET_MAIN(code)      (((code) >> 24) & 0xFF)
#define ADC_GET_SUB(code)       (((code) >> 16) & 0xFF)
#define ADC_GET_NEXT(code)      (((code) >>  0) & 0xFF)
#define ADC_GET_NEXT_MAIN(code)  ((((code) >> 3) & 0x1F))
#define ADC_GET_NEXT_SUB(code)   ((code) & 0x07)

//
// ===============================
// 智能宏封装
// ===============================

// 普通IO通道：main = DMUX, sub = 0, next = io_index
#define ADC_IO_CHANNEL_CODE(io_index) \
    ADC_CHANNEL_CODE(ADC_MAIN_DMUX, ADC_ASEL_IO, (io_index))

// 模拟通道：main = AMUX
#define ADC_ANA_CHANNEL_CODE(ana_type, ana_sub) \
    ADC_CHANNEL_CODE(ADC_MAIN_AMUX, ana_type, (ana_sub))

// BT 测试通道：main = AMUX, sub = BT_TEST, 无PMU
#define ADC_BT_CHANNEL_CODE(bt_sub) \
    ADC_CHANNEL_CODE(ADC_MAIN_AMUX, ADC_ASEL_BT_TEST_LV, (bt_sub))

// AUDIO 测试通道：main = AMUX, sub = AUDIO_TEST, 无PMU
#define ADC_AUDIO_CHANNEL_CODE(bt_sub) \
    ADC_CHANNEL_CODE(ADC_MAIN_AMUX, ADC_ASEL_AUDIO_TEST, (bt_sub))

// PMU 内部通道：main = AMUX, sub = PMU_TEST
#define ADC_PMU_CHANNEL_CODE(pmu_main, pmu_sub) \
    ADC_CHANNEL_CODE_EX(ADC_MAIN_AMUX, ADC_ASEL_PMU_TEST, (pmu_main), (pmu_sub))

// 测试通道 (TMUX)
#define ADC_TEST_CHANNEL_CODE(sub, pmu_main, pmu_sub) \
    ADC_CHANNEL_CODE_EX(ADC_MAIN_TMUX, (sub), (pmu_main), (pmu_sub))


#endif

