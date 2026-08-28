#ifndef __AUDIO_SPLIT_GAIN_FADE_H_
#define __AUDIO_SPLIT_GAIN_FADE_H_

#include "generic/typedef.h"

#define GAIN_MAX_CH             8
#define SPLIT_GAIN_DVOL_MAX     16384.0f

struct aud_fade_gain_base {
    float update_gain;     //目标增益
    float cur_gain;        //当前生效增益
    int vol_fade;
    int fade_step;
};

struct aud_fade_gain {
    struct aud_fade_gain_base fade[GAIN_MAX_CH];
    int update_fade_time;
    int cur_fade_time;
    int sample_rate;
    u8 update;
};

struct aud_fade_gain_param {
    float gain[GAIN_MAX_CH];
    int fade_time;
    int sample_rate;
};

struct aud_fade_gain *split_gain_fade_open(struct aud_fade_gain_param *param);
void split_gain_fade_close(struct aud_fade_gain *fade_hdl);
void split_gain_fade_update(struct aud_fade_gain *fade_hdl, struct aud_fade_gain_param *param);
void split_gain_fade_process(struct aud_fade_gain *fade_hdl, s16 *in, s16 *out,
                             int channel, int IndataInc, int OutdataInc, int per_channel_npoint);
void split_gain_fade_process_32bit(struct aud_fade_gain *fade_hdl, int *in, int *out,
                                   int channel, int IndataInc, int OutdataInc, int per_channel_npoint);

#endif
