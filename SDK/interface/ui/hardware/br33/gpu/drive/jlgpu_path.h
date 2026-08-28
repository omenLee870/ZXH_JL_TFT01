#ifndef __JLGPU_PATH_H__
#define __JLGPU_PATH_H__


#include "jlgpu_framework.h"


#define JLGPU_PATH_DATATYPE_F32  (0)
#define JLGPU_PATH_DATATYPE_S32  (1)

typedef struct {
    uint32_t nsegments;
    uint32_t ndata;
    uint8_t *segments;
    void *data;
    uint8_t data_type;
} jlgpu_path_param_t;

int jlgpu_set_path(jlgpu_path_param_t *path, uint8_t data_type, uint32_t nsegments, uint8_t *segments, uint32_t ndata, void *data);
int jlgpu_path_update(jlgpu_path_param_t *path, jlgpu_path_t *jlgpu_path, uint8_t global_max_bez_div);


#endif
