
#ifndef __ANIMATION_H__
#define __ANIMATION_H__


typedef struct {
    unsigned short gh: 3;
    unsigned short r: 5;
    unsigned short b: 5;
    unsigned short gl: 3;
} rgb565_t;

// static char line_buf0[454*4];
// static char line_buf1[454*4];
// static char line_buf2[454*4];

typedef struct {
    int degree;
    float theta;
    float R;
    int focal;
    int alpha_factor;
    int width;
    int height;
    int transpose;

    int *ypos;
    int *xoff;
    int *xskip;
    int *xstep;
    // int *alpha;

    char *line_buf0;
    char *line_buf1;
    char *line_buf2;
} ring_effect_t;


typedef struct {

    int width;
    int height;
    int start_alpha;
    float start_scale;
    int frozen_x;
    unsigned char *img_cur;
    unsigned char *img_next;

    int cur_alpha;
    float cur_scale;
    int cur_xstep;
    int cur_ystep;

    char *line_buf0;
    char *line_buf1;
    char *line_buf2;

    int no_fill_zero;
} flipanim_scalealpha_t;


void ring_effect_init(ring_effect_t *re, int degree, int width, int height, int focal, int alpha_factor, int transpose);
void ring_effect_free(ring_effect_t *re);
void ring_effect_transform(ring_effect_t *re, rgb565_t *out, rgb565_t *in, int ystart, int yend);

void flipanim_scalealpha_init(flipanim_scalealpha_t *fa, int width, int height, unsigned char *img_cur, unsigned char *img_next, int start_alpha, float start_scale, int frozen_x);
void flipanim_scalealpha_free(flipanim_scalealpha_t *fa);
void flipanim_scalealpha_run(flipanim_scalealpha_t *fa, rgb565_t *out, int x, int ystart, int yend, int flip_all);
void scalealpha_run(flipanim_scalealpha_t *fa, rgb565_t *out, int x, int ystart, int yend, int alpha, float scale);

int animation_init(int width);
int animation_uninit();

#endif
