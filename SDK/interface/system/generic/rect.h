#ifndef RECT_H
#define RECT_H

#include "typedef.h"

#define RECT_LARGE_SIZE_ENABLE		1

#if (defined RECT_LARGE_SIZE_ENABLE && RECT_LARGE_SIZE_ENABLE)
typedef int rect_coord_t;
#else
typedef short rect_coord_t;
#endif

#ifndef AT_UI_RAM
#define AT_UI_RAM             //AT(.ui_ram)
#endif

enum hori_align {
    HORI_ALIGN_LEFT = 0,
    HORI_ALIGN_CENTER,
    HORI_ALIGN_RIGHT,
};

enum vert_align {
    VERT_ALIGN_TOP = 0,
    VERT_ALIGN_CENTER,
    VERT_ALIGN_BOTTOM,
};

struct position {
    rect_coord_t x;
    rect_coord_t y;
};

struct rect {
    rect_coord_t left;
    rect_coord_t top;
    rect_coord_t width;
    rect_coord_t height;
};

#define rect_left(r)        ((r)->left)
#define rect_top(r)         ((r)->top)
#define rect_right(r)       ((r)->left + (r)->width)
#define rect_bottom(r)      ((r)->top + (r)->height)
#define rect_width(r)		((r)->width)
#define rect_height(r)		((r)->height)

#define rect_x_min(r)		rect_left(r)
#define rect_x_max(r)		rect_right(r)
#define rect_y_min(r)		rect_top(r)
#define rect_y_max(r)		rect_bottom(r)

//#define rect_height(v)  ((v)->bottom - (v)->top)
//#define rect_width(v)  ((v)->right - (v)->left)


AT_UI_RAM
static inline bool in_rect(const struct rect *rect, struct position *pos)
{
    if (rect->left <= pos->x && rect_right(rect) > pos->x) {
        if (rect->top <= pos->y && rect_bottom(rect) > pos->y) {
            return true;
        }
    }
    return false;
}

AT_UI_RAM
static inline bool get_rect_cover(const struct rect *a, const struct rect *b, struct rect *c)
{
    int right, bottom;

    c->top = MAX(a->top, b->top);
    c->left = MAX(a->left, b->left);
    right = MIN(rect_right(a), rect_right(b));
    bottom = MIN(rect_bottom(a), rect_bottom(b));
    c->width = right - c->left;
    c->height = bottom - c->top;

    if ((c->top < bottom) && (c->left < right)) {
        return true;
    }
    return false;
}

AT_UI_RAM
static inline bool get_rect_nocover_l(const struct rect *a, const struct rect *b, struct rect *c)
{
    int right, bottom;

    c->left = MIN(rect_left(a), rect_left(b));
    c->top = MIN(rect_top(a), rect_top(b));
    right = MAX(rect_left(a), rect_left(b));
    bottom = MAX(rect_bottom(a), rect_bottom(b));

    if ((c->top < bottom) && (c->left < right)) {
        c->width = right - c->left;
        c->height = bottom - c->top;
        return true;
    }
    return false;
}

AT_UI_RAM
static inline bool get_rect_nocover_r(const struct rect *a, const struct rect *b, struct rect *c)
{
    int right, bottom;

    c->left = MIN(rect_right(a), rect_right(b));
    c->top = MIN(rect_top(a), rect_top(b));
    right = MAX(rect_right(a), rect_right(b));
    bottom = MAX(rect_bottom(a), rect_bottom(b));

    if ((c->top < bottom) && (c->left < right)) {
        c->width = right - c->left;
        c->height = bottom - c->top;
        return true;
    }
    return false;
}

AT_UI_RAM
static inline bool get_rect_nocover_t(const struct rect *a, const struct rect *b, struct rect *c)
{
    int right, bottom;

    c->left = MIN(rect_left(a), rect_left(b));
    c->top = MIN(rect_top(a), rect_top(b));
    right = MAX(rect_right(a), rect_right(b));
    bottom = MAX(rect_top(a), rect_top(b));

    if ((c->top < bottom) && (c->left < right)) {
        c->width = right - c->left;
        c->height = bottom - c->top;
        return true;
    }
    return false;
}

AT_UI_RAM
static inline bool get_rect_nocover_b(const struct rect *a, const struct rect *b, struct rect *c)
{
    int right, bottom;

    c->left = MIN(rect_left(a), rect_left(b));
    c->top = MIN(rect_bottom(a), rect_bottom(b));
    right = MAX(rect_right(a), rect_right(b));
    bottom = MAX(rect_bottom(a), rect_bottom(b));

    if ((c->top < bottom) && (c->left < right)) {
        c->width = right - c->left;
        c->height = bottom - c->top;
        return true;
    }
    return false;
}

AT_UI_RAM
static inline bool has_crop_area(struct rect *rect, struct rect *lcd, struct rect *draw)
{
    struct rect r;
    if (get_rect_cover(rect, lcd, &r) && memcmp(&r, draw, sizeof(struct rect))) {
        return true;
    }
    return false;
}

AT_UI_RAM
static inline bool get_rect_align(struct rect *rect, struct rect *img, int hori_align, int vert_align)
{
    if ((img->width <= 0) || (img->height <= 0)) {
        return false;
    }

    if (hori_align == HORI_ALIGN_CENTER) {
        img->left = rect->left + (rect->width - img->width) / 2;
    } else if (hori_align == HORI_ALIGN_RIGHT) {
        img->left = rect->left + (rect->width - img->width);
    } else {
        img->left = rect->left;
    }

    if (vert_align == VERT_ALIGN_CENTER) {
        img->top = rect->top + (rect->height - img->height) / 2;
    } else if (vert_align == VERT_ALIGN_BOTTOM) {
        img->top = rect->top + (rect->height - img->height);
    } else {
        img->top = rect->top;
    }
    return true;
}

AT_UI_RAM
static inline bool get_rect_crop(struct rect *draw, struct rect *img, struct rect *crop)
{
    // 如果图片在绘制区域内，不需要裁剪
    if ((rect_left(img) >= rect_left(draw))
        && (rect_top(img) >= rect_top(draw))
        && (rect_right(img) <= rect_right(draw))
        && (rect_bottom(img) <= rect_bottom(draw))) {
        return false;
    }

    // 如果图片和绘制区域没有交集，不需要裁剪
    struct rect temp;
    if (!get_rect_cover(draw, img, &temp)) {
        return false;
    }

    crop->left	 = temp.left - img->left;
    crop->top	 = temp.top - img->top;
    crop->width  = temp.width;
    crop->height = temp.height;
    return true;

#if 0
    struct rect rect;
    if (!(get_rect_cover(draw, img, &rect) && memcmp(&rect, img, sizeof(struct rect)))) {
        return false;
    }
    crop->left	 = rect.left - img->left;
    crop->top	 = rect.top - img->top;
    crop->width  = rect.width;
    crop->height = rect.height;
    return true;
#endif
}



#endif

