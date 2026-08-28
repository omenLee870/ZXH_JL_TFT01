#ifndef __VAD_ALGO_H__
#define __VAD_ALGO_H__

#define DVAD_VOICE_CHECK    1 //算法刚启动，检测中
#define DVAD_VOICE_NONE     2 //检测不到人声
#define DVAD_VOICE_DET      3 //检测到人声

struct dvad_config {
    int smooth;
    int confidence;
    int exit_cnt_limit;
};

void *jl_dvad_open(void *cfg);
int jl_dvad_detect(void *kws, short *data, int samples);

int jl_kws_model_get_heap_size(int model, int *model_size, int *private_heap_size, int *share_heap_size, int smooth);
void *jl_kws_model_init(int model, char *private_heap_buffer, int private_heap_size, char *share_heap_buffer, int share_heap_size, int model_size, int confidence, int online, int smooth);
int jl_kws_model_reset(void *m);
int jl_kws_model_process(void *m, int model, char *pcm, int size, int *result);

#endif /* #ifndef __VAD_ALGO_H__ */
