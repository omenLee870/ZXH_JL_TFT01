#ifndef _AUDIO_EQ_LIMITER_COMBINED_H_
#define _AUDIO_EQ_LIMITER_COMBINED_H_

#include "system/includes.h"
#include "media/includes.h"
#include "effects/audio_eq.h"
#include "effects/audio_multiband_limiter.h"

struct eq_limiter_param_tool {
    struct multiband_limiter_param_tool_set mblimiter_param;
    u16 eq_len;
    u8 reserve[2];
    u8 eq_tool[0];
};

struct eq_limiter_info {
    u32 sample_rate;
    u8 in_bit_width;
    u8 out_bit_width;
    u8 qval;
    u8 in_ch_num;
    u8 out_ch_num;
    u8 close_flag;
    u16 param_len;
};

struct eq_limiter_param {
    struct eq_limiter_info info;
    struct eq_limiter_param_tool cfg;
};

struct eq_limiter_combined {
    struct audio_eq *eq;
    struct audio_limiter *limiter;  //limiter与mblimiter二选一打开
    struct multiband_limiter_hdl *mb_limiter;
    struct eq_limiter_param *param;
};

unsigned int audio_eq_limiter_combined_need_buf(struct eq_limiter_param *param);
int audio_eq_limiter_combined_open(struct eq_limiter_combined *com, struct eq_limiter_param *param, void *tmpbuf);
int audio_eq_limiter_combined_run(struct eq_limiter_combined *com, void *datain, void *dataout, int PointsPerChannel);
int audio_eq_limiter_combined_update(struct eq_limiter_combined *com, struct eq_limiter_param *param);

void eq_limiter_combined_printf(struct eq_limiter_param *params);
extern struct effects_func_api eq_limiter_ops;

#endif

