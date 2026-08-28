#ifndef __VOICE_MIC_AEC_H_
#define __VOICE_MIC_AEC_H_

#include "lib_h/AptFiltLib.h"
#include "lib_h/EchoSuppressLib.h"
#include "includes.h"
#include "audio_cvp.h"

struct voice_mic_aec_open_param {
    int enablebit;
    //AEC
    int M;
    int B;
    float ConverThr;
    float RefEngThr;
    float DT_AggressiveFactor;
    //NLP
    float AggressFactor;
    float minSuppress;
    float OverSuppressThr;
    float OverSuppress;
};

void voice_mic_aec_open(struct voice_mic_aec_open_param *param);
void voice_mic_aec_close();
void voice_mic_aec_run(void *in, void *out, void *ref, int len);

#endif
