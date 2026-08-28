/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#include "../gpu_demo.h"

#if (defined GPU_DEMO_ANIM_CUBE && GPU_DEMO_ANIM_CUBE)

#include "../img_data/img_data.h"

#include "rect.h"


/* struct gpu_demo_anim_cube { */
/*     int task_id; */
/*     int elm_id; */
/* }; */
/* static struct gpu_demo_anim_cube anim_cube = {0}; */
/* #define __this (&anim_cube); */


static void create_page_chain(pJLGPUChainHead_t head, struct rect *screen)
{
    jlgpu_task_fill_param_init(1, 1, 0xff, 0xff, 0x00, 0x00);
    jlgpu_task_fill_set_draw(0, 0, 128, 128);
    jlgpu_task_fill_set_area(rect_left(screen), rect_top(screen), rect_width(screen), rect_height(screen));
    jlgpu_update_task_by_id(head, 1, 1, &task_param);
}


void gpu_anim_cube_init()
{
    printf("@@@@@@ %s() %d\n", __func__, __LINE__);
}

void gpu_anim_cube_free()
{
    printf("@@@@@@ %s() %d\n", __func__, __LINE__);
}

pJLGPUChainHead_t gpu_anim_cube_draw(pJLGPUChainHead_t head, struct rect *screen)
{
    printf("@@@@@@ %s() %d\n", __func__, __LINE__);

    create_page_chain(head, screen);

    jlgpu_matrix_t matrix;
    jlgpu_matrix_t *m = &matrix;

#if 1
    int x = 0;
    int y = 0;
    int w = 320;//128;
    int h = 386;//128;

    jlgpu_matrix_set_identity(m);

    float rot[9];
    jlgpu_matrix_3d_rotate(rot, 0, 0, 0);
    jlgpu_matrix_3d_scale(rot, 1, 1, 1);

    float camera[12];
    jlgpu_matrix_3d_camera(camera, 0, 0, 1, w, h, 90, 90);

    float M1[12];
    jlgpu_matrix_3d_34_mul_33(M1, camera, rot);

    jlgpu_3Dmodel_t *model = jlgpu_matrix_3d_parse_model(jlgpu_matrix_3d_get_cube_model());
    float *pts2d = malloc(sizeof(float) * 2 * model->npoint);
    jlgpu_matrix_3d_to_2d(M1, model->points, pts2d, model->npoint);

    int f = 0;
    uint16_t *idxs = &model->faces[model->npoint_per_face * f];
    float *face_matrix = &model->face_matrix[9 * f];

    float M2[12], M3[9];
    jlgpu_matrix_3d_34_mul_33(M2, M1, face_matrix);

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            M3[i * 3 + j] = M2[i * 4 + j];
        }
    }
    M3[2] += M2[3];
    M3[5] += M2[7];
    M3[8] += M2[11];

    jlgpu_matrix_3d_invert(M3, M2);

    float w_scale = 1.0f;
    float h_scale = 1.0f;
    for (int j = 0; j < 3; j++) {
        M2[0 * 3 + j] = M2[0 * 3 + j] * ((float)w / 2.0f) * w_scale
                        + M2[2 * 3 + j] * ((float)(w - 1) / 2.0f);

        M2[1 * 3 + j] = M2[1 * 3 + j] * ((float)h / 2.0f) * h_scale
                        + M2[2 * 3 + j] * ((float)(h - 1) / 2.0f);
    }

    m->m[0][0] = M2[0 * 3 + 0];
    m->m[0][1] = M2[0 * 3 + 1];
    m->m[0][2] = M2[0 * 3 + 2];
    m->m[1][0] = M2[1 * 3 + 0];
    m->m[1][1] = M2[1 * 3 + 1];
    m->m[1][2] = M2[1 * 3 + 2];
    m->m[2][0] = M2[2 * 3 + 0];
    m->m[2][1] = M2[2 * 3 + 1];
    m->m[2][2] = M2[2 * 3 + 2];

    jlgpu_matrix_t mt;
    jlgpu_matrix_set_identity(&mt);
    jlgpu_matrix_set_translate(&mt, -x, -y);

    jlgpu_matrix_t dst;
    jlgpu_matrix_multiply(&dst, m, &mt);
    memcpy(m, &dst, sizeof(jlgpu_matrix_t));

    free(pts2d);

#else

    struct rect win_rect;
    jlgpu_get_win_rect(&win_rect);

    jlgpu_matrix_set_identity(m);
    jlgpu_matrix_translate(m, -rect_left(&win_rect) - 50, -rect_top(&win_rect) - 50);
    jlgpu_matrix_scale(m, 2, 2);
    jlgpu_matrix_rotate(m, 45);

    __DUMP_MATRIX(m);
#endif

    gpu_matrix_t matrix_1 = {
        {{-0.872632, 1.013288, -158.429688}, {-0.228431, -2.191444, 2756.238770}, {0.000489, -0.001891, 2.147404}}
    };
    m = &matrix_1;

    pJLGPUChainHead_t new_head = jlgpu_task_list_copy_create(NULL, head, m, 1);

    jlgpu_chain_dump_all_task(new_head, __func__, __LINE__);


    jlgpu_chain_delete_head(head);

    return new_head;
}



#endif // !GPU_DEMO_ANIM_CUBE


