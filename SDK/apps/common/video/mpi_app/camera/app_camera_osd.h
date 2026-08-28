#ifndef APP_CAMERA_OSD_H
#define APP_CAMERA_OSD_H

#include "app_camera_mpi.h"
#include "mpi_vpss.h"
#include "sys_time.h"

#define APP_CAMERA_OSD_COLOR0           0x057d88u
#define APP_CAMERA_OSD_COLOR_TEXT       0xe20095u
#define APP_CAMERA_OSD_BUF_COUNT        2u

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint8_t *buf;
    uint8_t *buf_base;
    uint32_t buf_size;
} app_camera_osd_bitmap_t;

typedef struct {
    uint8_t enable;
    uint8_t has_time;
    uint8_t buf_index;
    uint16_t timer;
    app_camera_handle_t owner;
    mpi_vpss_chn_t vpss_chn;
    uint16_t x;
    uint16_t y;
    char text[256];
    app_camera_osd_bitmap_t bitmap[APP_CAMERA_OSD_BUF_COUNT];
} app_camera_text_osd_t;

uint8_t app_camera_osd_has_time(const char *text);
app_camera_ret_t app_camera_osd_text_format(app_camera_text_osd_t *text_osd,
        const char *text);
app_camera_ret_t app_camera_osd_set_text(app_camera_text_osd_t *text_osd,
        const app_camera_osd_attr_t *attr);
app_camera_ret_t app_camera_osd_enable(app_camera_handle_t ctx,
                                       app_camera_text_osd_t *text_osd,
                                       mpi_vpss_chn_t vpss_chn,
                                       uint8_t vpss_started);
app_camera_ret_t app_camera_osd_apply(app_camera_handle_t ctx,
                                      app_camera_text_osd_t *text_osd,
                                      mpi_vpss_chn_t vpss_chn);
app_camera_ret_t app_camera_osd_disable(app_camera_handle_t ctx,
                                        app_camera_text_osd_t *text_osd,
                                        mpi_vpss_chn_t vpss_chn);
void app_camera_osd_stop_timer(app_camera_text_osd_t *text_osd);
app_camera_ret_t app_camera_osd_create_bitmap(app_camera_text_osd_t *text_osd);
void app_camera_osd_free_bitmap(app_camera_text_osd_t *text_osd);
app_camera_ret_t app_camera_osd_update_bitmap(app_camera_text_osd_t *text_osd,
        app_camera_osd_bitmap_t **bitmap);
app_camera_ret_t app_camera_osd_text_parse(const char *text,
        app_camera_osd_bitmap_t *bitmap);
void app_camera_osd_bitmap_free(app_camera_osd_bitmap_t *bitmap);

#endif /* APP_CAMERA_OSD_H */
