/**
 * @file prop.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "prop.h"
#include "res.h"

#if LV_USE_GUI_PARSE_DYN != 0

static const char * scaleclock_hour_ticks[] = {"12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", NULL};

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void control_deleted_event_cb(lv_event_t * e);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void prop_set(char * buf, int32_t size, lv_obj_t * obj, int32_t page_id)
{
    if(buf == NULL || size <= 0) {
        return;
    }

    JLDynControl * control = (JLDynControl *)buf;
    char * prop_buf = buf + control->prop_offset;
    int32_t offset = 0;
    int32_t props_size = control->style_offset - control->prop_offset;

    bool isHide = false;
    while(offset < props_size) {
        int32_t prop_id = *(int32_t *)(prop_buf + offset);
        offset += sizeof(int32_t);
        int32_t temp_offset = offset;
        switch(prop_id) {
            case LV_OBJ_SET_POS_ID:
                lv_obj_set_pos(obj, *(int32_t *)(prop_buf + offset), *(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_SIZE_ID:
                lv_obj_set_size(obj, *(int32_t *)(prop_buf + offset), *(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_OBJ_SET_X_ID:
                lv_obj_set_x(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_SET_Y_ID:
                lv_obj_set_y(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_SET_WIDTH_ID:
                lv_obj_set_width(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_SET_HEIGHT_ID:
                lv_obj_set_height(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_SET_USER_DATA_ID: {
                    int32_t len = *(int32_t *)(prop_buf + offset);
                    int32_t padding = (4 - len % 4) % 4; // 4字节对齐
                    offset += sizeof(int32_t);
                    char * data = lv_malloc(len + 1);
                    lv_memset(data, 0, len + 1);
                    lv_memcpy(data, prop_buf + offset, len);
                    lv_obj_set_user_data(obj, data);
                    offset += len + padding;
                    lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE, data); // 控件删除时释放用户数据
                }
                break;
            case LV_OBJ_ADD_STATE_ID:
                lv_obj_add_state(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_ADD_FLAG_ID:
                lv_obj_add_flag(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_CLEAR_FLAG_ID:
                lv_obj_remove_flag(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_OBJ_ALIGN_ID:
                lv_obj_align(obj, *(int32_t *)(prop_buf + offset), *(int32_t *)(prop_buf + offset + sizeof(int32_t)),
                             *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t)));
                offset += 3 * sizeof(int32_t);
                break;

            #if LV_USE_LABEL

            case LV_LABEL_SET_TEXT_ID: {
                    int32_t len = *(int32_t *)(prop_buf + offset);
                    int32_t padding = (4 - len % 4) % 4;
                    offset += sizeof(int32_t);
                    char * text = lv_malloc(len + 1);
                    lv_memset(text, 0, len + 1);
                    lv_memcpy(text, prop_buf + offset, len);
                    lv_label_set_text(obj, text);
                    offset += len + padding;
                    lv_free(text); // set_text会内部复制，这里释放
                }
                break;
            case LV_LABEL_SET_TEXT_FMT_ID: {
                    int32_t len = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    char * text = (char *)(prop_buf + offset);
                    lv_label_set_text_fmt(obj, text);
                }
                break;
            case LV_LABEL_SET_LONG_MODE_ID:
                lv_label_set_long_mode(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_LABEL_SET_RECOLOR_ID:
                lv_label_set_recolor(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;

            #endif

            #if LV_USE_IMAGE

            case LV_IMG_SET_SRC_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    lv_image_set_src(obj, res_get(page_id, img_id));
                }
                break;
            case LV_IMG_SET_PIVOT_ID:
                lv_image_set_pivot(obj, *(int32_t *)(prop_buf + offset), *(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                offset += 2 * sizeof(int32_t);
                break;
            case LV_IMG_SET_ANGLE_ID:
                lv_image_set_rotation(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_IMG_SET_ZOOM_ID:
                lv_image_set_scale(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_IMG_SET_OFFSET_X_ID:
                lv_image_set_offset_x(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_IMG_SET_OFFSET_Y_ID:
                lv_image_set_offset_y(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;

            #endif

            #if LV_USE_ANALOGCLOCK

            case LV_ANALOGCLOCK_SET_HIDE_DIGITS_ID:
                isHide = *(int32_t *)(prop_buf + offset) == 1 ? true : false;
              //  lv_analogclock_hide_digits(obj, isHide);
                offset += sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_MAJOR_TICKS_ID:
                //lv_analogclock_set_major_ticks(obj, *(int32_t *)(prop_buf + offset),
                  //                             *(int32_t *)(prop_buf + offset + sizeof(int32_t)),
                    //                           lv_color_hex(*(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t))),
                      //                         *(int32_t *)(prop_buf + offset + 3 * sizeof(int32_t)));
                offset += 4 * sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_TICKS_ID:
                //lv_analogclock_set_ticks(obj, *(int32_t *)(prop_buf + offset),
                  //                       *(int32_t *)(prop_buf + offset + sizeof(int32_t)),
                    //                     lv_color_hex(*(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t))));
                offset += 3 * sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_HOUR_NEEDLE_LINE_ID:
                //lv_analogclock_set_hour_needle_line(obj, *(int32_t *)(prop_buf + offset),
                  //                                  lv_color_hex(*(int32_t *)(prop_buf + offset + sizeof(int32_t))),
                    //                                *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t)));
                offset += 3 * sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_MIN_NEEDLE_LINE_ID:
                //lv_analogclock_set_min_needle_line(obj, *(int32_t *)(prop_buf + offset),
                  //                                 lv_color_hex(*(int32_t *)(prop_buf + offset + sizeof(int32_t))),
                    //                               *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t)));
                offset += 3 * sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_SEC_NEEDLE_LINE_ID:
                //lv_analogclock_set_sec_needle_line(obj, *(int32_t *)(prop_buf + offset),
                  //                                 lv_color_hex(*(int32_t *)(prop_buf + offset + sizeof(int32_t))),
                    //                               *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t)));
                offset += 3 * sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_TIME_ID:
                //lv_analogclock_set_time(obj, *(int32_t *)(prop_buf + offset),
                  //                      *(int32_t *)(prop_buf + offset + sizeof(int32_t)),
                    //                    *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t)));
                offset += 3 * sizeof(int32_t);
                break;
            case LV_ANALOGCLOCK_SET_DIAL_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    //lv_analogclock_set_dial_img(obj, res_get(page_id, img_id));
                }
                break;
            case LV_ANALOGCLOCK_SET_HOUR_NEEDLE_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    //lv_analogclock_set_hour_needle_img(obj, res_get(page_id, img_id), *(int32_t *)(prop_buf + offset),
                      //                                 *(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                    offset += 2 * sizeof(int32_t);// pivot_x, pivot_y
                }
                break;
            case LV_ANALOGCLOCK_SET_MIN_NEEDLE_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                   // lv_analogclock_set_min_needle_img(obj, res_get(page_id, img_id), *(int32_t *)(prop_buf + offset),
                     //                                 *(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                    offset += 2 * sizeof(int32_t);// pivot_x, pivot_y
                }
                break;
            case LV_ANALOGCLOCK_SET_SEC_NEEDLE_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    //lv_analogclock_set_sec_needle_img(obj, res_get(page_id, img_id), *(int32_t *)(prop_buf + offset),
                      //                                *(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                    offset += 2 * sizeof(int32_t);// pivot_x, pivot_y
                }
                break;

            #endif

            #if LV_USE_IMGLIST

            case LV_IMGLIST_SET_SRC_ID: {
                    int32_t size = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    void ** src = lv_malloc(size * sizeof(void *));
                    lv_memset(src, 0, size * sizeof(void *));
                    for(int i = 0; i < size; i++) {
                        int32_t img_id = *(int32_t *)(prop_buf + offset);
                        offset += sizeof(int32_t);
                        src[i] = res_get(page_id, img_id);
                    }
                    lv_imglist_set_src(obj, (const void **)src, size);
                    lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE,
                                        src); // 控件删除时释放图片列表指针数组
                }
                break;
            case LV_IMGLIST_SET_ACT_ID:
                lv_imglist_set_act(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
                
            #endif

            #if LV_USE_LINE

            case LV_LINE_SET_POINTS_ID: {
                    int32_t size = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    lv_point_precise_t * points = lv_malloc(size * sizeof(lv_point_precise_t));
                    lv_memset(points, 0, size * sizeof(lv_point_precise_t));
                    for(int i = 0; i < size; i++) {
                        points[i].x = *(int32_t *)(prop_buf + offset);
                        offset += sizeof(int32_t);
                        points[i].y = *(int32_t *)(prop_buf + offset);
                        offset += sizeof(int32_t);
                    }
                    lv_line_set_points(obj, points, size);
                    lv_obj_add_event_cb(obj, control_deleted_event_cb, LV_EVENT_DELETE, points); // 控件删除时释放点数组
                }
                break;

            #endif

            #if LV_USE_SCALE

            case LV_SCALECLOCK_SET_LABEL_ENABLE_ID:
                {
                    bool enable = *(int32_t *)(prop_buf + offset) == 1 ? true : false;
                    offset += sizeof(int32_t);
                    lv_scale_set_label_show(obj, enable);
                    if(enable) lv_scale_set_text_src(obj, scaleclock_hour_ticks);
                }
                break;
            case LV_SCALECLOCK_SET_TOTAL_TICK_ID:
                lv_scale_set_total_tick_count(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_SCALECLOCK_SET_MAJOR_TICK_EVERY_ID:
                lv_scale_set_major_tick_every(obj, *(int32_t *)(prop_buf + offset));
                offset += sizeof(int32_t);
                break;
            case LV_SCALECLOCK_SET_HOUR_NEEDLE_LINE_ID:
                {
                    /* width, color, length */
                    int32_t width = *(int32_t *)(prop_buf + offset);
                    lv_color_t color = lv_color_hex(*(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                    int32_t length = *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t));
                    offset += 3 * sizeof(int32_t);

                    lv_obj_t * line = lv_line_create(obj);
                    lv_obj_set_style_line_width(line, width, LV_PART_MAIN);
                    lv_obj_set_style_line_color(line, color, LV_PART_MAIN);
                    lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN);
                    lv_obj_update_layout(obj);
                    lv_scale_set_line_needle_value(obj, line, length, 0);
                    lv_obj_set_user_data(line, (void *)(intptr_t)length);
                }
                break;
            case LV_SCALECLOCK_SET_MIN_NEEDLE_LINE_ID:
                {
                    /* width, color, length */
                    int32_t width = *(int32_t *)(prop_buf + offset);
                    lv_color_t color = lv_color_hex(*(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                    int32_t length = *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t));
                    offset += 3 * sizeof(int32_t);

                    lv_obj_t * line = lv_line_create(obj);
                    lv_obj_set_style_line_width(line, width, LV_PART_MAIN);
                    lv_obj_set_style_line_color(line, color, LV_PART_MAIN);
                    lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN);
                    lv_obj_update_layout(obj);
                    lv_scale_set_line_needle_value(obj, line, length, 0);
                    lv_obj_set_user_data(line, (void *)(intptr_t)length);
                }
                break;
            case LV_SCALECLOCK_SET_SEC_NEEDLE_LINE_ID:
                {
                    /* width, color, length */
                    int32_t width = *(int32_t *)(prop_buf + offset);
                    lv_color_t color = lv_color_hex(*(int32_t *)(prop_buf + offset + sizeof(int32_t)));
                    int32_t length = *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t));
                    offset += 3 * sizeof(int32_t);

                    lv_obj_t * line = lv_line_create(obj);
                    lv_obj_set_style_line_width(line, width, LV_PART_MAIN);
                    lv_obj_set_style_line_color(line, color, LV_PART_MAIN);
                    lv_obj_set_style_line_rounded(line, true, LV_PART_MAIN);
                    lv_obj_update_layout(obj);
                    lv_scale_set_line_needle_value(obj, line, length, 0);
                    lv_obj_set_user_data(line, (void *)(intptr_t)length);
                }
                break;
            case LV_SCALECLOCK_SET_TIME_ID:
                {
                    int32_t hour = *(int32_t *)(prop_buf + offset);
                    int32_t min = *(int32_t *)(prop_buf + offset + sizeof(int32_t));
                    int32_t sec = *(int32_t *)(prop_buf + offset + 2 * sizeof(int32_t));
                    offset += 3 * sizeof(int32_t);

                    /* child[0]: hour, child[1]: min, child[2]: sec */
                    if(lv_obj_get_child_count(obj) >= 1) {
                        lv_obj_t * hour_nd = lv_obj_get_child(obj, 0);
                        int32_t hour_val = (hour % 12) * 60 + min;
                        if(lv_obj_check_type(hour_nd, &lv_image_class)) {
                            lv_scale_set_image_needle_value(obj, hour_nd, hour_val);
                        }
                        else {
                            lv_scale_set_line_needle_value(obj, hour_nd, (int32_t)(intptr_t)lv_obj_get_user_data(hour_nd), hour_val);
                        }
                    }
                    if(lv_obj_get_child_count(obj) >= 2) {
                        lv_obj_t * min_nd = lv_obj_get_child(obj, 1);
                        if(lv_obj_check_type(min_nd, &lv_image_class)) {
                            lv_scale_set_image_needle_value(obj, min_nd, min * 12 + sec / 5);
                        }
                        else {
                            lv_scale_set_line_needle_value(obj, min_nd, (int32_t)(intptr_t)lv_obj_get_user_data(min_nd), min * 12 + sec / 5);
                        }
                    }
                    if(lv_obj_get_child_count(obj) >= 3) {
                        lv_obj_t * sec_nd = lv_obj_get_child(obj, 2);
                        if(lv_obj_check_type(sec_nd, &lv_image_class)) {
                            lv_scale_set_image_needle_value(obj, sec_nd, sec * 12);
                        }
                        else {
                            lv_scale_set_line_needle_value(obj, sec_nd, (int32_t)(intptr_t)lv_obj_get_user_data(sec_nd), sec * 12);
                        }
                    }
                }
                break;
            case LV_SCALECLOCK_SET_DIAL_IMG_ID: {
                    /* Use background image on the scale's main part */
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    lv_obj_set_style_bg_image_src(obj, res_get(page_id, img_id), LV_PART_MAIN);
            }
                break;
            case LV_SCALECLOCK_SET_HOUR_NEEDLE_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    int32_t pivot_x = *(int32_t *)(prop_buf + offset);
                    int32_t pivot_y = *(int32_t *)(prop_buf + offset + sizeof(int32_t));
                    offset += 2 * sizeof(int32_t);

                    lv_obj_update_layout(obj);
                    lv_obj_t * img = lv_image_create(obj);
                    lv_image_set_src(img, res_get(page_id, img_id));
                    /* Align so the pivot is on the center of scale */
                    lv_obj_align(img, LV_ALIGN_CENTER,
                                 lv_image_get_src_width(img) / 2 - pivot_x,
                                 lv_image_get_src_height(img) / 2 - pivot_y);
                    lv_image_set_pivot(img, pivot_x, pivot_y);
                    lv_scale_set_image_needle_value(obj, img, 0);
            }
                break;
            case LV_SCALECLOCK_SET_MIN_NEEDLE_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    int32_t pivot_x = *(int32_t *)(prop_buf + offset);
                    int32_t pivot_y = *(int32_t *)(prop_buf + offset + sizeof(int32_t));
                    offset += 2 * sizeof(int32_t);

                    lv_obj_update_layout(obj);
                    lv_obj_t * img = lv_image_create(obj);
                    lv_image_set_src(img, res_get(page_id, img_id));
                    lv_obj_align(img, LV_ALIGN_CENTER,
                                 lv_image_get_src_width(img) / 2 - pivot_x,
                                 lv_image_get_src_height(img) / 2 - pivot_y);
                    lv_image_set_pivot(img, pivot_x, pivot_y);
                    lv_scale_set_image_needle_value(obj, img, 0);
            }
                break;
            case LV_SCALECLOCK_SET_SEC_NEEDLE_IMG_ID: {
                    int32_t img_id = *(int32_t *)(prop_buf + offset);
                    offset += sizeof(int32_t);
                    int32_t pivot_x = *(int32_t *)(prop_buf + offset);
                    int32_t pivot_y = *(int32_t *)(prop_buf + offset + sizeof(int32_t));
                    offset += 2 * sizeof(int32_t);

                    lv_obj_update_layout(obj);
                    lv_obj_t * img = lv_image_create(obj);
                    lv_image_set_src(img, res_get(page_id, img_id));
                    lv_obj_align(img, LV_ALIGN_CENTER,
                                 lv_image_get_src_width(img) / 2 - pivot_x,
                                 lv_image_get_src_height(img) / 2 - pivot_y);
                    lv_image_set_pivot(img, pivot_x, pivot_y);
                    lv_scale_set_image_needle_value(obj, img, 0);
            }
                break;
                
            #endif

            default:
                LV_LOG_WARN("prop_set failed %d", prop_id);
                break;
        }
        if(temp_offset == offset) {
            break;
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Control deleted event callback
 *
 * @param e
 */
static void control_deleted_event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    void * user_data = lv_event_get_user_data(e);
    if(user_data != NULL) {
        lv_free(user_data); // 释放用户数据
    }
}

#endif
