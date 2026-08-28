
#ifndef __UI_SCROLL_VIEW_H__
#define __UI_SCROLL_VIEW_H__



typedef struct ui_scrollview_t {
    u16 timer_id; // 定时器ID
    u16 interval; // 定时器中断间隔
    void *priv; // 私有参数

    int pos;

    int record;
    int cursor;
    int target;

    int min_pos;
    int max_pos;
    int bounces;

    int velocity;
    int distance;

    int scroll_mode;
    int align_mode;

    int align_gap;
    int align_dir; // 对齐方向


    int (*cb)(struct ui_scrollview_t *s, void *priv, int pos);
} ui_scrollview_t, *pui_scrollview_t;


int ui_scrollview_init(ui_scrollview_t *s, void *priv, int pos, int (*cb)(ui_scrollview_t *s, void *priv, int pos));
int ui_scrollview_free(ui_scrollview_t *s);
int ui_scrollview_stop(ui_scrollview_t *s);

int ui_scrollview_set_bounces(ui_scrollview_t *s, int bounces);
int ui_scrollview_set_scroll_area(ui_scrollview_t *s, int min, int max);
int ui_scrollview_set_align_by_gap(ui_scrollview_t *s, int gap);
int ui_scrollview_set_align_by_tab(ui_scrollview_t *s, int *tab, int size);

int ui_scrollview_auto_align(ui_scrollview_t *s);
int ui_scrollview_move_offset(ui_scrollview_t *s, int offset);
int ui_scrollview_move_accrue(ui_scrollview_t *s, int offset);
int ui_scrollview_move_velocity(ui_scrollview_t *s, int v);
int ui_scrollview_move_distance(ui_scrollview_t *s, int dist);



#endif


