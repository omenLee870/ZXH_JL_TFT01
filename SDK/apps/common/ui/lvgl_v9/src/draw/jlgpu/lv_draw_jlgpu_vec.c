#include "lv_draw_jlgpu_vec.h"

#include "app_config.h"

#if CONFIG_LVGL_UI_ENABLE



#if LV_USE_DRAW_JLGPU

// 默认大小和增长步进，由应用层客户配置配置
const int JLGPU_PATH_DEFAULT_SIZE = 512;	// path 默认大小
const int JLGPU_PATH_GROWTH_STEPS = 32;		// path 增长步进
const int JLGPU_PATH_DEFAULT_BEZ_DIV = JLGPU_GLOBAL_MAX_BEZ_DIV;   // path 默认细分

#define PATH_KAPPA 0.552284f

/* Magic number from https://spencermortensen.com/articles/bezier-circle/ */
#define PATH_ARC_MAGIC 0.55191502449351f
/*#define MATH_HALF_PI 1.57079632679489661923f*/
#define PATH_MEM_SIZE_MIN 128

#define SIGN(x) (math_zero(x) ? 0 : ((x) > 0 ? 1 : -1))

#define LV_MIN(a, b) ((a) < (b) ? (a) : (b))


#define PROJECTION_DEEP 500         // 投影深度 500~1000,值越小立体感越强

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void jlgpu_path_reserve_space(jlgpu_vec_t *path, int size);


//test
#if 1

// 变换二维点 (x, y)，返回变换后坐标 (out_x, out_y)
void jlgpu_matrix_transform_point(const jlgpu_matrix_t *matrix, float x, float y, float *out_x, float *out_y)
{
    float tx = matrix->m[0][0] * x + matrix->m[0][1] * y + matrix->m[0][2];
    float ty = matrix->m[1][0] * x + matrix->m[1][1] * y + matrix->m[1][2];
    float tw = matrix->m[2][0] * x + matrix->m[2][1] * y + matrix->m[2][2];

    // 齐次坐标除法，防止除以0
    if (tw != 0.0f) {
        *out_x = tx / tw;
        *out_y = ty / tw;
    } else {
        *out_x = tx;
        *out_y = ty;
    }
}


// 设置投影参数，修改矩阵第三行元素
void jlgpu_matrix_set_projection(jlgpu_matrix_t *matrix, float g, float h, float i)
{
    matrix->m[2][0] = g;
    matrix->m[2][1] = h;
    matrix->m[2][2] = i;
}



void jlgpu_get_flipx_matrix(jlgpu_matrix_t *matrix, float rot)
{
    matrix->m[0][0] = 1.0f * 1.0f;
    matrix->m[0][1] = 0.0f;
    matrix->m[0][2] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[1][1] = 1.0f * jlgpu_cosf(rot);
    matrix->m[1][2] = 0.0f;
    matrix->m[2][0] = 0.0f;
    matrix->m[2][1] = -jlgpu_sinf(rot) / PROJECTION_DEEP;
    matrix->m[2][2] = 1;
}

void jlgpu_get_flipy_matrix(jlgpu_matrix_t *matrix, float rot)
{
    matrix->m[0][0] = 1.0f * jlgpu_cosf(rot);
    matrix->m[0][1] = 0.0f;
    matrix->m[0][2] = 0.0f;
    matrix->m[1][0] = 0.0f;
    matrix->m[1][1] = 1.0f * 1.0f;
    matrix->m[1][2] = 0.0f;
    matrix->m[2][0] = -jlgpu_sinf(rot) / PROJECTION_DEEP;
    matrix->m[2][1] = 0.0f;
    matrix->m[2][2] = 1;
}
#endif


static uint32_t transform(jlgpu_point_t *result, float x, float y, jlgpu_matrix_t *matrix)
{
    float pt_x;
    float pt_y;
    float pt_w;

    /* Test for identity matrix. */
    if (matrix == NULL) {
        result->x = (int)x;
        result->y = (int)y;

        /* Success. */
        return 1;
    }

    /* Transform x, y, and w. */
    pt_x = (x * matrix->m[0][0]) + (y * matrix->m[0][1]) + matrix->m[0][2];
    pt_y = (x * matrix->m[1][0]) + (y * matrix->m[1][1]) + matrix->m[1][2];
    pt_w = (x * matrix->m[2][0]) + (y * matrix->m[2][1]) + matrix->m[2][2];

    if (pt_w <= 0.0f) {
        return 0;
    }

    /* Compute projected x and y. */
    if (pt_x < 0) {
        result->x = (int)((pt_x / pt_w) - 0.5f);
    } else {
        result->x = (int)((pt_x / pt_w) + 0.5f);
    }
    if (pt_y < 0) {
        result->y = (int)((pt_y / pt_w) - 0.5f);
    } else {
        result->y = (int)((pt_y / pt_w) + 0.5f);
    }

    /* Success. */
    return 1;
}

jlgpu_area_t jlgpu_compute_bbox(jlgpu_matrix_t *matrix, jlvec_bbox_t *bounding_box, int width, int height)
{

    /*path->bounding_box[0] = path_bbox.x_min;*/
    /*path->bounding_box[1] = path_bbox.y_min;*/
    /*path->bounding_box[2] = path_bbox.x_max;*/
    /*path->bounding_box[3] = path_bbox.y_max;*/

    jlgpu_area_t bbox;
    if (!matrix) {
        bbox.x_min = bounding_box->x_min;
        bbox.x_max = bounding_box->x_max;
        bbox.y_min = bounding_box->y_min;
        bbox.y_max = bounding_box->y_max;
        return bbox;
    }
    jlgpu_point_t point_min = { 0 }, point_max = { 0 }, temp = { 0 };
    transform(&temp, (float)bounding_box->x_min, (float)bounding_box->y_min, matrix);
    point_min = point_max = temp;

    transform(&temp, (float)bounding_box->x_max, (float)bounding_box->y_min, matrix);
    if (temp.x < point_min.x) {
        point_min.x = temp.x;
    }
    if (temp.y < point_min.y) {
        point_min.y = temp.y;
    }
    if (temp.x > point_max.x) {
        point_max.x = temp.x;
    }
    if (temp.y > point_max.y) {
        point_max.y = temp.y;
    }

    transform(&temp, (float)bounding_box->x_max, (float)bounding_box->y_max, matrix);
    if (temp.x < point_min.x) {
        point_min.x = temp.x;
    }
    if (temp.y < point_min.y) {
        point_min.y = temp.y;
    }
    if (temp.x > point_max.x) {
        point_max.x = temp.x;
    }
    if (temp.y > point_max.y) {
        point_max.y = temp.y;
    }

    transform(&temp, (float)bounding_box->x_min, (float)bounding_box->y_max, matrix);
    if (temp.x < point_min.x) {
        point_min.x = temp.x;
    }
    if (temp.y < point_min.y) {
        point_min.y = temp.y;
    }
    if (temp.x > point_max.x) {
        point_max.x = temp.x;
    }
    if (temp.y > point_max.y) {
        point_max.y = temp.y;
    }

    if (point_min.x < 0) {
        point_min.x = 0;
    }
    if (point_min.y < 0) {
        point_min.y = 0;
    }
    if (point_max.x > width) {
        point_max.x = width;
    }
    if (point_max.y > height) {
        point_max.y = height;
    }

    bbox.x_min = point_min.x;
    bbox.x_max = point_max.x;
    bbox.y_min = point_min.y;
    bbox.y_max = point_max.y;

    return bbox;
}




void jlgpu_path_init(jlgpu_vec_t *path, uint8_t *base, uint8_t bez_div)
{
    path->base = base;
    path->cur_coord = (int16_t *)(path->base + 2);
    path->cur_seg = base;
    path->nsegments = 0;
    path->total_len = 0;
    path->edge_num = 0;
    path->bez_div = bez_div;

    path->path_bbox.x_min = JLGPU_FLOAT32_MAX;
    path->path_bbox.y_min = JLGPU_FLOAT32_MAX;
    path->path_bbox.x_max = JLGPU_FLOAT32_MIN;
    path->path_bbox.y_max = JLGPU_FLOAT32_MIN;
}

void jlgpu_path_deinit(jlgpu_vec_t *path)
{

}

int jlgpu_compute_div(jlgpu_area_t bbox)
{
    int length = (bbox.x_max - bbox.x_min) * (bbox.y_max - bbox.y_min);
    int div = 0;
    if (length <= 2) {
        div = 0;
    } else if (length <= 4) {
        div = 1;
    } else if (length <= 8) {
        div = 2;
    } else if (length <= 16) {
        div = 3;
    } else if (length <= 32) {
        div = 4;
    } else if (length <= 64) {
        div = 5;
    } else if (length <= 128) {
        div = 6;
    } else {
        div = 7;
    }
    return div;
}

static inline void jlgpu_vec_update(jlgpu_vec_t *path)
{
    if (path->nsegments % 2) {
        path->cur_seg++;
    } else {
        path->cur_seg = (uint8_t *)path->cur_coord;
        path->cur_coord = path->cur_coord + 1;
    }
}

static inline void jlgpu_vec_append_op(jlgpu_vec_t *path, uint32_t op)
{
    *path->cur_seg = op;
    path->nsegments++;
}

static inline void jlgpu_vec_append_point(jlgpu_vec_t *path, float x, float y)
{
    int16_t x_fix, y_fix;
    x_fix = (int16_t)(x * 8);
    y_fix = (int16_t)(y * 8);

    *path->cur_coord++ = x_fix;
    *path->cur_coord++ = y_fix;

}

void jlgpu_path_move_to(jlgpu_vec_t *path, float x, float y)
{

    jlgpu_path_reserve_space(path, 5);

    jlgpu_vec_append_op(path, JLGPU_MOVE_TO);
    jlgpu_vec_append_point(path, x, y);

    jlgpu_vec_update(path);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, x);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, y);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, x);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, y);

    path->total_len += 5;// 2 * sizeof(int16_t) + 1;
}

void jlgpu_path_line_to(jlgpu_vec_t *path, float x, float y)
{
    jlgpu_path_reserve_space(path, 5);

    jlgpu_vec_append_op(path, JLGPU_LINE_TO);
    jlgpu_vec_append_point(path, x, y);

    jlgpu_vec_update(path);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, x);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, y);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, x);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, y);

    path->total_len += 5;// 2 * sizeof(int16_t) + 1;
    path->edge_num++;
}

void jlgpu_path_quad_to(jlgpu_vec_t *path,
                        float cx, float cy,
                        float x, float y)
{

    jlgpu_path_reserve_space(path, 9);

    jlgpu_vec_append_op(path, JLGPU_QUAD_TO | (7 << 4));
    jlgpu_vec_append_point(path, cx, cy);
    jlgpu_vec_append_point(path, x, y);

    jlgpu_vec_update(path);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, x);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, y);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, x);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, y);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, cx);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, cy);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, cx);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, cy);

    path->total_len += 9;// 4 * sizeof(int16_t) + 1;
    path->edge_num = path->edge_num + (2 << path->bez_div);

}

void jlgpu_path_cubic_to(jlgpu_vec_t *path,
                         float cx1, float cy1,
                         float cx2, float cy2,
                         float x, float y)
{
    jlgpu_path_reserve_space(path, 13);

    jlgpu_vec_append_op(path, JLGPU_CUBIC_TO | (7 << 4));
    jlgpu_vec_append_point(path, cx1, cy1);
    jlgpu_vec_append_point(path, cx2, cy2);
    jlgpu_vec_append_point(path, x, y);

    jlgpu_vec_update(path);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, cx1);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, cy1);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, cx1);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, cy1);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, cx2);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, cy2);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, cx2);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, cy2);

    path->path_bbox.x_min = JLVEC_MIN(path->path_bbox.x_min, x);
    path->path_bbox.y_min = JLVEC_MIN(path->path_bbox.y_min, y);
    path->path_bbox.x_max = JLVEC_MAX(path->path_bbox.x_max, x);
    path->path_bbox.y_max = JLVEC_MAX(path->path_bbox.y_max, y);

    path->total_len += 13;// 6 * sizeof(int16_t) + 1;
    path->edge_num = path->edge_num + (2 << path->bez_div);
}

void jlgpu_path_close(jlgpu_vec_t *path)
{
    //not support
}

void jlgpu_path_end(jlgpu_vec_t *path)
{
    //jlgpu_vec_append_op(path, 0xf);
    *path->cur_seg = 0xf;
    path->total_len++;
}

void jlgpu_path_append_rect(
    jlgpu_vec_t *path,
    float x, float y,
    float w, float h,
    float r)
{
    const float half_w = w / 2.0f;
    const float half_h = h / 2.0f;

    /*clamping cornerRadius by minimum size*/
    const float r_max = LV_MIN(half_w, half_h);
    if (r > r_max) {
        r = r_max;
    }

    /*rectangle*/
    if (r <= 0) {
        jlgpu_path_move_to(path, x, y);
        jlgpu_path_line_to(path, x + w, y);
        jlgpu_path_line_to(path, x + w, y + h);
        jlgpu_path_line_to(path, x, y + h);
        //jlgpu_path_close(path);
        jlgpu_path_line_to(path, x, y);
        return;
    }

    /*circle*/
    if (math_equal(r, half_w) && math_equal(r, half_h)) {
        jlgpu_path_append_circle(path, x + half_w, y + half_h, r, r);
        return;
    }

    /* Get the control point offset for rounded cases */
    const float offset = r * PATH_ARC_MAGIC;

    /* Rounded rectangle case */
    /* Starting point */
    jlgpu_path_move_to(path, x + r, y);

    /* Top side */
    jlgpu_path_line_to(path, x + w - r, y);

    /* Top-right corner */
    jlgpu_path_cubic_to(path, x + w - r + offset, y, x + w, y + r - offset, x + w, y + r);

    /* Right side */
    jlgpu_path_line_to(path, x + w, y + h - r);

    /* Bottom-right corner*/
    jlgpu_path_cubic_to(path, x + w, y + h - r + offset, x + w - r + offset, y + h, x + w - r, y + h);

    /* Bottom side */
    jlgpu_path_line_to(path, x + r, y + h);

    /* Bottom-left corner */
    jlgpu_path_cubic_to(path, x + r - offset, y + h, x, y + h - r + offset, x, y + h - r);

    /* Left side*/
    jlgpu_path_line_to(path, x, y + r);

    /* Top-left corner */
    jlgpu_path_cubic_to(path, x, y + r - offset, x + r - offset, y, x + r, y);

    /* Ending point */
    //jlgpu_path_close(path);
}


void jlgpu_path_append_circle(
    jlgpu_vec_t *path,
    float cx, float cy,
    float rx, float ry)
{

    /* https://learn.microsoft.com/zh-cn/xamarin/xamarin-forms/user-interface/graphics/skiasharp/curves/beziers */
    float rx_kappa = rx * PATH_KAPPA;
    float ry_kappa = ry * PATH_KAPPA;

    jlgpu_path_move_to(path, cx, cy - ry);
    jlgpu_path_cubic_to(path, cx + rx_kappa, cy - ry, cx + rx, cy - ry_kappa, cx + rx, cy);
    jlgpu_path_cubic_to(path, cx + rx, cy + ry_kappa, cx + rx_kappa, cy + ry, cx, cy + ry);
    jlgpu_path_cubic_to(path, cx - rx_kappa, cy + ry, cx - rx, cy + ry_kappa, cx - rx, cy);
    jlgpu_path_cubic_to(path, cx - rx, cy - ry_kappa, cx - rx_kappa, cy - ry, cx, cy - ry);
    //jlgpu_path_close(path);

}


//仅保留base和bez_div
void jlgpu_path_reset(jlgpu_vec_t *path)
{
    if (path == NULL) {
        return;
    }

    memset(path->base, 0, path->total_len);

    /*path->base = base;*/
    path->cur_seg = path->base;
    path->cur_coord = (int16_t *)(path->base + 2);
    path->nsegments = 0;
    path->total_len = 0;
    path->edge_num = 0;
    /*path->bez_div = bez_div;*/
}

void jlgpu_path_append_arc_right_angle(jlgpu_vec_t *path,
                                       float start_x, float start_y,
                                       float center_x, float center_y,
                                       float end_x, float end_y)
{
    float dx1 = center_x - start_x;
    float dy1 = center_y - start_y;
    float dx2 = end_x - center_x;
    float dy2 = end_y - center_y;

    float c = SIGN(dx1 * dy2 - dx2 * dy1) * PATH_ARC_MAGIC;

    jlgpu_path_cubic_to(path,
                        start_x - c * dy1, start_y + c * dx1,
                        end_x - c * dy2, end_y + c * dx2,
                        end_x, end_y);
}


void jlgpu_path_append_arc(jlgpu_vec_t *path,
                           float cx, float cy,
                           float radius,
                           float start_angle,
                           float sweep,
                           uint8_t pie)
{
    /* just circle */
    if (sweep >= 360.0f || sweep <= -360.0f) {
        jlgpu_path_append_circle(path, cx, cy, radius, radius);
        return;
    }

    start_angle = MATH_RADIANS(start_angle);
    sweep = MATH_RADIANS(sweep);

    int n_curves = (int)ceil(fabs(sweep / MATH_HALF_PI));
    float sweep_sign = sweep < 0 ? -1.f : 1.f;
    float fract = fmodf(sweep, MATH_HALF_PI);
    fract = (math_zero(fract)) ? MATH_HALF_PI * sweep_sign : fract;

    /* Start from here */
    float start_x = radius * MATH_COSF(start_angle);
    float start_y = radius * MATH_SINF(start_angle);

    if (pie) {
        jlgpu_path_move_to(path, cx, cy);
        jlgpu_path_line_to(path, start_x + cx, start_y + cy);
    }

    for (int i = 0; i < n_curves; ++i) {
        float end_angle = start_angle + ((i != n_curves - 1) ? MATH_HALF_PI *sweep_sign : fract);
        float end_x = radius * MATH_COSF(end_angle);
        float end_y = radius * MATH_SINF(end_angle);

        /* variables needed to calculate bezier control points */

        /** get bezier control points using article:
         * (http://itc.ktu.lt/index.php/ITC/article/view/11812/6479)
         */
        float ax = start_x;
        float ay = start_y;
        float bx = end_x;
        float by = end_y;
        float q1 = ax * ax + ay * ay;
        float q2 = ax * bx + ay * by + q1;
        float k2 = (4.0f / 3.0f) * ((MATH_SQRTF(2 * q1 * q2) - q2) / (ax * by - ay * bx));

        /* Next start point is the current end point */
        start_x = end_x;
        start_y = end_y;

        end_x += cx;
        end_y += cy;

        float ctrl1_x = ax - k2 * ay + cx;
        float ctrl1_y = ay + k2 * ax + cy;
        float ctrl2_x = bx + k2 * by + cx;
        float ctrl2_y = by - k2 * bx + cy;

        jlgpu_path_cubic_to(path, ctrl1_x, ctrl1_y, ctrl2_x, ctrl2_y, end_x, end_y);
        start_angle = end_angle;
    }

    if (pie) {
        //lv_vg_lite_path_close(path);
        jlgpu_path_line_to(path, cx, cy);
    }

}

void jlgpu_path_append_polygon(jlgpu_vec_t *path,
                               float *coord, int nedges)
{
    if (nedges < 3) {
        return;
    }

    float sx, sy;
    sx = coord[0];
    sy = coord[1];
    jlgpu_path_move_to(path, sx, sy);
    for (int i = 1; i < nedges; i++) {
        jlgpu_path_line_to(path, coord[i * 2 + 0], coord[i * 2 + 1]);
    }
    jlgpu_path_line_to(path, sx, sy);//close
}


//todo
//创建path
jlgpu_vec_t *jlgpu_path_create(void)
{
    /*LV_PROFILER_DRAW_BEGIN;*/
    jlgpu_vec_t *path = lv_malloc_zeroed(sizeof(jlgpu_vec_t));
    LV_ASSERT_MALLOC(path);
    uint8_t *base = lv_malloc_zeroed(JLGPU_PATH_DEFAULT_SIZE);
    path->capacity = JLGPU_PATH_DEFAULT_SIZE;
    jlgpu_path_init(path, base, JLGPU_PATH_DEFAULT_BEZ_DIV);
    /*LV_PROFILER_DRAW_END;*/
    return path;
}

void jlgpu_path_destroy(jlgpu_vec_t *path)
{
    /*LV_PROFILER_DRAW_BEGIN;*/
    LV_ASSERT_NULL(path);
    if (path->base != NULL) {
        lv_free(path->base);
        path->base = NULL;
    }
    lv_free(path);
}

// 申请path预留空间
static void jlgpu_path_reserve_space(jlgpu_vec_t *path, int size)
{
    if (path->total_len + size > path->capacity) {
        /*printf("path == 0x%x\n",path);*/
        /*printf("path->total_len == %d\n",path->total_len);*/
        /*printf("path->capacity == %d\n",path->capacity);*/
        /*printf("size == %d\n",size);*/
        printf("fixme\n");
        // 所需内存大于当前path容量，需重新申请。合理配置 JLGPU_PATH_DEFAULT_SIZE 和 JLGPU_PATH_GROWTH_STEPS 可以减少进这个if
        void *old_path = path->base; // 原先的path先记录下来
        if (size < JLGPU_PATH_GROWTH_STEPS) {
            path->capacity += JLGPU_PATH_GROWTH_STEPS; // 如果要预留的size小于增长步进，则增长一个步进
        } else {
            // 计算需要增长的内存，以JLGPU_PATH_GROWTH_STEPS为步进，超过size即可
            int need_growth = (size + JLGPU_PATH_GROWTH_STEPS - 1) / JLGPU_PATH_GROWTH_STEPS * JLGPU_PATH_GROWTH_STEPS;
            path->capacity += need_growth; // 计算更新后的缓存大小
        }

        path->base = lv_malloc_zeroed(path->capacity); // 申请新的缓存buf

        memcpy(path->base, old_path, path->total_len); // 把旧的path拷贝到新的缓存buf
        lv_free(old_path); // 释放原先的path
    }

}

void lv_jlgpu_path_reset(jlgpu_vec_t *path)
{
    jlgpu_path_init(path, path->base, JLGPU_PATH_DEFAULT_BEZ_DIV);
}

//获取一个可以重复利用的global path
jlgpu_vec_t *lv_jlgpu_path_get(lv_draw_jlgpu_unit_t *unit)
{
    /*lv_jlgpu_path_reset(unit->global_path);*/
    jlgpu_path_reset(unit->global_path);
    return unit->global_path;
}

void lv_jlgpu_path_set_bez_div(jlgpu_vec_t *path, uint8_t bez_div)
{
    path->bez_div = bez_div;
}

#endif

#endif /* #if CONFIG_LVGL_UI_ENABLE */
