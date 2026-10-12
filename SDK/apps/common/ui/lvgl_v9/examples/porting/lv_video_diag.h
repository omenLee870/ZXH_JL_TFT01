#ifndef LV_VIDEO_DIAG_H
#define LV_VIDEO_DIAG_H

/* Temporary fb5 diagnostics. Set to 0 after reproducing the freeze. */
#ifndef LV_VIDEO_DIAG_ENABLE
#define LV_VIDEO_DIAG_ENABLE 1
#endif

enum lv_video_diag_lane {
    LV_VIDEO_DIAG_UI,
    LV_VIDEO_DIAG_PUSH,
    LV_VIDEO_DIAG_FRAME,
    LV_VIDEO_DIAG_DRAIN,
    LV_VIDEO_DIAG_LANES
};

#if LV_VIDEO_DIAG_ENABLE
void lv_video_diag_start(void);
void lv_video_diag_stop(void);
/* stage must be a string literal; no framebuffer/context pointers are retained. */
void lv_video_diag_mark(enum lv_video_diag_lane lane, const char *stage,
                        unsigned int detail, unsigned int waiting);
#else
static inline void lv_video_diag_start(void) {}
static inline void lv_video_diag_stop(void) {}
static inline void lv_video_diag_mark(enum lv_video_diag_lane lane, const char *stage,
                                     unsigned int detail, unsigned int waiting) {}
#endif

#endif
