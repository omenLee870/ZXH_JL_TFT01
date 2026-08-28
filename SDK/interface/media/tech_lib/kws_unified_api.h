#ifndef _KWS_UNIFIED_API_H_
#define _KWS_UNIFIED_API_H_

#define KWS_TYPE_PHONE_CALL    (0)  //  (jl_kws_model_phone_call)
#define KWS_TYPE_FAR_KEYWORD   (1)  //  (jl_kws_model_far_keyword)
#define KWS_TYPE_SMART_WATCH   (2)  //  (jl_kws_model_smart_watch)

typedef struct __KWS_FUNC_API_ {
    int (*get_heap_size)(int model,
                         int *model_size,
                         int *private_heap_size,
                         int *share_heap_size);

    void *(*init)(int model,
                  char *private_heap_buffer,
                  int private_heap_size,
                  char *share_heap_buffer,
                  int share_heap_size,
                  int model_size,
                  float *confidence,
                  int online);

    int (*reset)(void *m);
    int (*process)(void *m, int model, char *pcm, int size);
    int (*free)(void *m);
} GET_KWS_FUNC_API;

extern GET_KWS_FUNC_API *get_kws_func_api(int type_mask);

#endif // KWS_UNIFIED_API_H
