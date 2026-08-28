/*****************************************************************
>file name : vad_develop.c
>create time : Thu 28 Apr 2022 07:11:24 PM CST
*****************************************************************/
#include "includes.h"
#include "vad_algo.h"
/*******************************************************
 * DVAD算法模块开发说明
 * 1、默认使用杰理api，命名为jl_dvad_xxx
 * 2、第三方算法开发，需将对应接口替换为自身算法模块
 *
 *******************************************************/
extern const int CONFIG_DVAD_DEVELOP_ENABLE;

void *dvad_algo_open(void *cfg)
{
    if (CONFIG_DVAD_DEVELOP_ENABLE == 0) {
        return jl_dvad_open(cfg);
    }

    /*其他算法开发*/
    if (CONFIG_DVAD_DEVELOP_ENABLE == 1) {
        return NULL;
    }

    return NULL;
}

/*
 * 参数说明 : handle    - 算法私有数据
 *            data      - PCM采样数据
 *            samples   - 音频采样点数
 * 返回值 : 2 - 检测不到语音
 *          3 - 检测到语音
 *
 */
int dvad_algo_detect(void *handle, short *data, int samples)
{
    if (CONFIG_DVAD_DEVELOP_ENABLE == 0) {
        return jl_dvad_detect(handle, data, samples);
    }

    if (CONFIG_DVAD_DEVELOP_ENABLE == 1) {
        return 0;
    }

    return 0;
}

void dvad_algo_close(void *handle)
{
    if (CONFIG_DVAD_DEVELOP_ENABLE == 1) {
    }
}
