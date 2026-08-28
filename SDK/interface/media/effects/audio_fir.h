#ifndef  AUDIO_FIR_CORE_H
#define  AUDIO_FIR_CORE_H

#if defined(EXPORT_PLATFORM_AUDIO_FIR_ENABLE) && EXPORT_PLATFORM_AUDIO_FIR_ENABLE
#include "asm/fir_core.h"
#include "asm/fir_core_api.h"
#endif
struct fir_param_tool_set {
    int is_bypass;
    u8 shift;
    u8 dsf;
    u16 filter_len;
    short filter[0];
} __attribute__((packed));

#endif
