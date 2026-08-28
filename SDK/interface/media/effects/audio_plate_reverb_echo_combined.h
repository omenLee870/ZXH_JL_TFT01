#ifndef __AUDIO_PLATE_REVERB_ECHO_COMBINED_H
#define __AUDIO_PLATE_REVERB_ECHO_COMBINED_H

#include "system/includes.h"
#include "media/includes.h"
#include "effects/audio_plate_reverb.h"
#include "effects/audio_reverb_lite.h"
#include "effects/audio_reverb.h"
#include "effects/audio_echo.h"
#include "effects/audio_gain_process.h"
#include "effects/channel_adapter.h"
#include "effects/audio_eq.h"

#define BAND_MERGE3_COMBO  3

struct band_merge3_param_tool {
    int is_bypass;//打开时，只输出 gain0通道的数据
    float gain[BAND_MERGE3_COMBO];
} ;


#define PLATE_REVERB_COMBO_ENABLE         (1UL << (0))
#define PLATE_REVERB_ADVANCE_COMBO_ENABLE (1UL << (1))
#define PLATE_REVERB_LITE_COMBO_ENABLE    (1UL << (2))
#define REVERB_COMBO_ENABLE               (1UL << (3))

union reverb_par {
    plate_reverb_param_tool_set preverb_param;
    struct reverb_lite_param_tool_set preverb_lite_param;
    reverb_param_tool_set reverb_param;
};
struct preverb_echo_param_tool {
    int is_bypass;//打开时,模块直通
    u32 reverb_type;// 1:plate reverb 2:plate reverb advance 4:plate reverb lite 8:reverb
    union reverb_par reverb_params;
    echo_param_tool_set echo_param;
    struct band_merge3_param_tool gain;
    u16 eq_len;
    u8 reserve[2];
    u8 eq_tool[0];
};

struct preverb_echo_info {
    u32 sample_rate;
    u8 in_bit_width;
    u8 out_bit_width;
    u8 qval;
    u8 in_ch_num;
    u8 out_ch_num;
    u8 close_flag;
    u16 param_len;
};
struct preverb_echo_param {
    struct preverb_echo_info info;
    struct preverb_echo_param_tool cfg;
};

struct preverb_echo_combined {
    void  *preverb;
    struct audio_echo          *echo;
    struct audio_eq            *eq;
    MixParam                   mix[BAND_MERGE3_COMBO];
    struct preverb_echo_param  *param;
};

unsigned int audio_preverb_echo_combined_need_buf(struct preverb_echo_param *param);
int audio_preverb_echo_combined_open(struct preverb_echo_combined *com, struct preverb_echo_param *param, void *tmpbuf);
int audio_preverb_echo_combined_run(struct preverb_echo_combined *com, void *datain, void *dataout, int PointsPerChannel);
int audio_preverb_echo_combined_update(struct preverb_echo_combined *com, struct preverb_echo_param *param);

void preverb_echo_combined_printf(struct preverb_echo_param *params);
extern struct effects_func_api preverb_echo_ops;

#endif
