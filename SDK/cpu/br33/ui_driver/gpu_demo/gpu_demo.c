/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

#include "gpu_demo.h"


#if (defined GPU_DEMO_ENABLE && GPU_DEMO_ENABLE)

/* @partA =================================<< DEMO API >>================================= */
extern pJLGPUChainHead_t gpu_demo_create_fill_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_fill_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_fill_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_fill_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_fill_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_fill_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_fill_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_texture_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_texture_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_texture_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_texture_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_texture_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_texture_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_texture_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_linegrad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linegrad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linegrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linegrad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linegrad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linegrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linegrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_radialgrad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radialgrad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radialgrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radialgrad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radialgrad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radialgrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radialgrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_roundgrad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_roundgrad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_roundgrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_roundgrad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_roundgrad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_roundgrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_roundgrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_ringgrad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_ringgrad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_ringgrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_ringgrad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_ringgrad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_ringgrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_ringgrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_anglegrad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_anglegrad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_anglegrad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_anglegrad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_anglegrad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_anglegrad_path_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_anglegrad_path_perspective_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_path_aff_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_linear_radial_grad_path_per_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_affine_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_perspective_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_path_aff_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_radial_angle_grad_path_per_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_img_fill_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_fill_aff_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_fill_per_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_fill_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_fill_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_fill_path_aff_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_fill_path_per_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_img_grad_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_grad_aff_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_grad_per_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_grad_mask_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_grad_path_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_grad_path_aff_task(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_create_img_grad_path_per_task(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_draw_ring_radial_angle_path(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_draw_ring_path(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_draw_cubic_bezier(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_draw_line_chart(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_draw_bar(pJLGPUChainHead_t head, struct rect *screen);
extern pJLGPUChainHead_t gpu_demo_draw_slider(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_demo_create_jlui_res(pJLGPUChainHead_t head, struct rect *screen);

void gpu_demo_breakpoint_init(void);
void gpu_demo_breakpoint_free(void);
extern pJLGPUChainHead_t gpu_demo_create_breakpoint_task(pJLGPUChainHead_t head, struct rect *screen);

extern void gpu_anim_cube_init();
extern void gpu_anim_cube_free();
extern pJLGPUChainHead_t gpu_anim_cube_draw(pJLGPUChainHead_t head, struct rect *screen);

extern void gpu_anim_flip_init();
extern void gpu_anim_flip_free();
extern pJLGPUChainHead_t gpu_anim_flip_draw(pJLGPUChainHead_t head, struct rect *screen);

extern pJLGPUChainHead_t gpu_image_auto_test_cb(pJLGPUChainHead_t head, struct rect *screen);


/* @partB =================================<< DEMO列表 >>================================= */
struct gpu_demo_list demo_list[] = {

#if (defined GPU_DEMO_FILL && GPU_DEMO_FILL)
    {
        .name = "GPU Fill",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_FILL_AFFINE && GPU_DEMO_FILL_AFFINE)
    {
        .name = "GPU Fill affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_FILL_PERSPECTIVE && GPU_DEMO_FILL_PERSPECTIVE)
    {
        .name = "GPU Fill perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_FILL_MASK && GPU_DEMO_FILL_MASK)
    {
        .name = "GPU Fill Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_FILL_PATH && GPU_DEMO_FILL_PATH)
    {
        .name = "GPU Fill Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_FILL_PATH_AFFINE && GPU_DEMO_FILL_PATH_AFFINE)
    {
        .name = "GPU Fill Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_FILL_PATH_PERSPECTIVE && GPU_DEMO_FILL_PATH_PERSPECTIVE)
    {
        .name = "GPU Fill Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_fill_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE && GPU_DEMO_TEXTURE)
    {
        .name = "GPU Image",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE_AFFINE && GPU_DEMO_TEXTURE_AFFINE)
    {
        .name = "GPU Image Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE_PERSPECTIVE && GPU_DEMO_TEXTURE_PERSPECTIVE)
    {
        .name = "GPU Image Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE_MASK && GPU_DEMO_TEXTURE_MASK)
    {
        .name = "GPU Image Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE_PATH && GPU_DEMO_TEXTURE_PATH)
    {
        .name = "GPU Image Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE_PATH_AFFINE && GPU_DEMO_TEXTURE_PATH_AFFINE)
    {
        .name = "GPU Image Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_TEXTURE_PATH_PERSPECTIVE && GPU_DEMO_TEXTURE_PATH_PERSPECTIVE)
    {
        .name = "GPU Image Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_texture_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD && GPU_DEMO_LINEGRAD)
    {
        .name = "GPU Line Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD_AFFINE && GPU_DEMO_LINEGRAD_AFFINE)
    {
        .name = "GPU Line Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD_PERSPECTIVE && GPU_DEMO_LINEGRAD_PERSPECTIVE)
    {
        .name = "GPU Line Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD_MASK && GPU_DEMO_LINEGRAD_MASK)
    {
        .name = "GPU Line Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD_PATH && GPU_DEMO_LINEGRAD_PATH)
    {
        .name = "GPU Line Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD_PATH_AFFINE && GPU_DEMO_LINEGRAD_PATH_AFFINE)
    {
        .name = "GPU Line Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEGRAD_PATH_PERSPECTIVE && GPU_DEMO_LINEGRAD_PATH_PERSPECTIVE)
    {
        .name = "GPU Line Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linegrad_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD && GPU_DEMO_RADIALGRAD)
    {
        .name = "GPU Radial Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD_AFFINE && GPU_DEMO_RADIALGRAD_AFFINE)
    {
        .name = "GPU Radial Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD_PERSPECTIVE && GPU_DEMO_RADIALGRAD_PERSPECTIVE)
    {
        .name = "GPU Radial Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD_MASK && GPU_DEMO_RADIALGRAD_MASK)
    {
        .name = "GPU Radial Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD_PATH && GPU_DEMO_RADIALGRAD_PATH)
    {
        .name = "GPU Radial Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD_PATH_AFFINE && GPU_DEMO_RADIALGRAD_PATH_AFFINE)
    {
        .name = "GPU Radial Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIALGRAD_PATH_PERSPECTIVE && GPU_DEMO_RADIALGRAD_PATH_PERSPECTIVE)
    {
        .name = "GPU Radial Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radialgrad_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD && GPU_DEMO_ROUNDGRAD)
    {
        .name = "GPU Round Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD_AFFINE && GPU_DEMO_ROUNDGRAD_AFFINE)
    {
        .name = "GPU Round Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD_PERSPECTIVE && GPU_DEMO_ROUNDGRAD_PERSPECTIVE)
    {
        .name = "GPU Round Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD_MASK && GPU_DEMO_ROUNDGRAD_MASK)
    {
        .name = "GPU Round Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD_PATH && GPU_DEMO_ROUNDGRAD_PATH)
    {
        .name = "GPU Round Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD_PATH_AFFINE && GPU_DEMO_ROUNDGRAD_PATH_AFFINE)
    {
        .name = "GPU Round Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ROUNDGRAD_PATH_PERSPECTIVE && GPU_DEMO_ROUNDGRAD_PATH_PERSPECTIVE)
    {
        .name = "GPU Round Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_roundgrad_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD && GPU_DEMO_RINGGRAD)
    {
        .name = "GPU Ring Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD_AFFINE && GPU_DEMO_RINGGRAD_AFFINE)
    {
        .name = "GPU Ring Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD_PERSPECTIVE && GPU_DEMO_RINGGRAD_PERSPECTIVE)
    {
        .name = "GPU Ring Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD_MASK && GPU_DEMO_RINGGRAD_MASK)
    {
        .name = "GPU Ring Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD_PATH && GPU_DEMO_RINGGRAD_PATH)
    {
        .name = "GPU Ring Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD_PATH_AFFINE && GPU_DEMO_RINGGRAD_PATH_AFFINE)
    {
        .name = "GPU Ring Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RINGGRAD_PATH_PERSPECTIVE && GPU_DEMO_RINGGRAD_PATH_PERSPECTIVE)
    {
        .name = "GPU Ring Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_ringgrad_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD && GPU_DEMO_ANGLEGRAD)
    {
        .name = "GPU Angle Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD_AFFINE && GPU_DEMO_ANGLEGRAD_AFFINE)
    {
        .name = "GPU Angle Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD_PERSPECTIVE && GPU_DEMO_ANGLEGRAD_PERSPECTIVE)
    {
        .name = "GPU Angle Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD_MASK && GPU_DEMO_ANGLEGRAD_MASK)
    {
        .name = "GPU Angle Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD_PATH && GPU_DEMO_ANGLEGRAD_PATH)
    {
        .name = "GPU Angle Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD_PATH_AFFINE && GPU_DEMO_ANGLEGRAD_PATH_AFFINE)
    {
        .name = "GPU Angle Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_path_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_ANGLEGRAD_PATH_PERSPECTIVE && GPU_DEMO_ANGLEGRAD_PATH_PERSPECTIVE)
    {
        .name = "GPU Angle Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_anglegrad_path_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD && GPU_DEMO_LINEAR_RADIAL_GRAD)
    {
        .name = "GPU Line Radial Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD_AFF && GPU_DEMO_LINEAR_RADIAL_GRAD_AFF)
    {
        .name = "GPU Line Radial Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD_PER && GPU_DEMO_LINEAR_RADIAL_GRAD_PER)
    {
        .name = "GPU Line Radial Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD_MASK && GPU_DEMO_LINEAR_RADIAL_GRAD_MASK)
    {
        .name = "GPU Line Radial Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD_PATH && GPU_DEMO_LINEAR_RADIAL_GRAD_PATH)
    {
        .name = "GPU Line Radial Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD_PATH_AFF && GPU_DEMO_LINEAR_RADIAL_GRAD_PATH_AFF)
    {
        .name = "GPU Line Radial Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_path_aff_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINEAR_RADIAL_GRAD_PATH_PER && GPU_DEMO_LINEAR_RADIAL_GRAD_PATH_PER)
    {
        .name = "GPU Line Radial Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_linear_radial_grad_path_per_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD && GPU_DEMO_RADIAL_ANGLE_GRAD)
    {
        .name = "GPU Radial Angle Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD_AFF && GPU_DEMO_RADIAL_ANGLE_GRAD_AFF)
    {
        .name = "GPU Radial Angle Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_affine_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD_PER && GPU_DEMO_RADIAL_ANGLE_GRAD_PER)
    {
        .name = "GPU Radial Angle Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_perspective_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD_MASK && GPU_DEMO_RADIAL_ANGLE_GRAD_MASK)
    {
        .name = "GPU Radial Angle Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD_PATH && GPU_DEMO_RADIAL_ANGLE_GRAD_PATH)
    {
        .name = "GPU Radial Angle Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD_PATH_AFF && GPU_DEMO_RADIAL_ANGLE_GRAD_PATH_AFF)
    {
        .name = "GPU Radial Angle Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_path_aff_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RADIAL_ANGLE_GRAD_PATH_PER && GPU_DEMO_RADIAL_ANGLE_GRAD_PATH_PER)
    {
        .name = "GPU Radial Angle Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_radial_angle_grad_path_per_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL && GPU_DEMO_IMG_FILL)
    {
        .name = "GPU Image Fill",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL_AFF && GPU_DEMO_IMG_FILL_AFF)
    {
        .name = "GPU Image Fill Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_aff_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL_PER && GPU_DEMO_IMG_FILL_PER)
    {
        .name = "GPU Image Fill Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_per_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL_MASK && GPU_DEMO_IMG_FILL_MASK)
    {
        .name = "GPU Image Fill Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL_PATH && GPU_DEMO_IMG_FILL_PATH)
    {
        .name = "GPU Image Fill Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL_PATH_AFF && GPU_DEMO_IMG_FILL_PATH_AFF)
    {
        .name = "GPU Image Fill Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_path_aff_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_FILL_PATH_PER && GPU_DEMO_IMG_FILL_PATH_PER)
    {
        .name = "GPU Image Fill Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_fill_path_per_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD && GPU_DEMO_IMG_GRAD)
    {
        .name = "GPU Image Grad",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD_AFF && GPU_DEMO_IMG_GRAD_AFF)
    {
        .name = "GPU Image Grad Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_aff_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD_PER && GPU_DEMO_IMG_GRAD_PER)
    {
        .name = "GPU Image Grad Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_per_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD_MASK && GPU_DEMO_IMG_GRAD_MASK)
    {
        .name = "GPU Image Grad Mask",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_mask_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD_PATH && GPU_DEMO_IMG_GRAD_PATH)
    {
        .name = "GPU Image Grad Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_path_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD_PATH_AFF && GPU_DEMO_IMG_GRAD_PATH_AFF)
    {
        .name = "GPU Image Grad Path Affine",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_path_aff_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_IMG_GRAD_PATH_PER && GPU_DEMO_IMG_GRAD_PATH_PER)
    {
        .name = "GPU Image Grad Path Perspective",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_img_grad_path_per_task,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RING_RADIAL_ANGLE_PATH && GPU_DEMO_RING_RADIAL_ANGLE_PATH)
    {
        .name = "GPU Draw Ring With radial angle and Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_draw_ring_radial_angle_path,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_RING_PATH && GPU_DEMO_RING_PATH)
    {
        .name = "GPU Draw Ring With Path",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_draw_ring_path,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_CUBIC_BEZIER && GPU_DEMO_CUBIC_BEZIER)
    {
        .name = "GPU Draw Cubic Bezier",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_draw_cubic_bezier,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_LINE_CHART && GPU_DEMO_LINE_CHART)
    {
        .name = "GPU Draw Line Chart",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_draw_line_chart,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_BAR && GPU_DEMO_BAR)
    {
        .name = "GPU Draw Bar",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_draw_bar,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_SLIDER && GPU_DEMO_SLIDER)
    {
        .name = "GPU Draw Slider",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_draw_slider,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_JLUI_RES && GPU_DEMO_JLUI_RES)
    {
        .name = "GPU JLUI RES",
        .time = GPU_DEMO_SHOW_MS,
        .init = NULL,
        .draw = gpu_demo_create_jlui_res,
        .free = NULL,
    },
#endif

#if (defined GPU_DEMO_BREAKPOINT && GPU_DEMO_BREAKPOINT)
    {
        .name = "GPU BREAKPOINT",
        .time = GPU_DEMO_SHOW_MS,
        .init = gpu_demo_breakpoint_init,
        .draw = gpu_demo_create_breakpoint_task,
        .free = gpu_demo_breakpoint_free,
    },
#endif

#if (defined GPU_DEMO_ANIM_CUBE && GPU_DEMO_ANIM_CUBE)
    {
        .name = "Anim Cube",
        .time = GPU_DEMO_SHOW_MS * 2,
        .init = gpu_anim_cube_init,
        .draw = gpu_anim_cube_draw,
        .free = gpu_anim_cube_free,
    },
#endif

#if (defined GPU_DEMO_ANIM_FLIP && GPU_DEMO_ANIM_FLIP)
    {
        .name = "Anim Flip",
        .time = GPU_DEMO_SHOW_MS * 10,
        .init = gpu_anim_flip_init,
        .draw = gpu_anim_flip_draw,
        .free = gpu_anim_flip_free,
    },
#endif



#if (defined GPU_IMAGE_AUTO_TEST && GPU_IMAGE_AUTO_TEST)
    {
        .name = "Image auto",
        .time = GPU_DEMO_SHOW_MS * 10,
        .init = NULL,
        .draw = gpu_image_auto_test_cb,
        .free = NULL,
    },
#endif

    /* 必须以NULL结尾 */
    {.name = "", .time = 0, .init = NULL, .draw = NULL, .free = NULL}
};


#endif

