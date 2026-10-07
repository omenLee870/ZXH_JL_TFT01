/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#include "lvgl.h"
#include "events_init.h"
#include "gui_guider.h"
#include "custom.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_display_t * hal_init(int32_t w, int32_t h);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void demo()
{
  lv_obj_t * btn = lv_btn_create(lv_screen_active());
  lv_obj_t * label = lv_label_create(btn);
  lv_label_set_text(label, "Hello World");
  lv_obj_center(btn);
  lv_obj_center(label);
}

void imagemask_demo()
{
  // lv_obj_t * img = lv_image_create(lv_screen_active());
  lv_obj_t * img = lv_imagemask_create(lv_screen_active());
  // lv_image_set_src(img, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\a8test1.png");
  lv_imagemask_set_src(img, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\qrcode.jpg");
  lv_imagemask_set_mask(img, "A:\\ProjectFile\\NewUITool\\H.ToolRelease\\workspace\\lvgl9_demo\\images\\a8test1.png");
  // lv_obj_center(img);
  // lv_obj_set_pos(img, 100, 100);
  lv_obj_set_size(img, 300, 300);
  lv_obj_set_style_border_width(img, 2, 0);
  lv_obj_set_style_border_color(img, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_opa(img, LV_OPA_100, 0);
  lv_obj_set_style_shadow_width(img, 10, 0);
  lv_obj_set_style_shadow_color(img, lv_color_hex(0x000000), 0);
  lv_obj_set_style_shadow_opa(img, LV_OPA_100, 0);
  // lv_obj_set_style_bg_opa(img, LV_OPA_100, 0);
  // lv_obj_set_style_bg_color(img, lv_color_hex(0x000000), 0);

  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);
}

lv_ui guider_ui;

int main(int argc, char **argv)
{
  (void)argc; /*Unused*/
  (void)argv; /*Unused*/

  /*Initialize LVGL*/
  lv_init();

  /*Initialize the display, and the input devices*/
  hal_init(SDL_HOR_RES, SDL_VER_RES);

  setup_ui(&guider_ui);
  events_init(&guider_ui);
  custom_init(&guider_ui);
  LV_LOG_WARN("main end");

  // demo();
  // imagemask_demo();

  /*Open a demo or an example*/
  // if (argc == 0) {
  //   lv_demo_widgets();
  //   //  lv_example_chart_1();
  // } else {
  //   if (!lv_demos_create(&argv[1], argc - 1)) {
  //     lv_demos_show_help();
  //     goto demo_end;
  //   }
  // }

  // demo();

  // canvas_demo();

  /*To hide the memory and performance indicators in the corners
   *disable `LV_USE_MEM_MONITOR` and `LV_USE_PERF_MONITOR` in `lv_conf.h`*/

  while(1) {
      /* Periodically call the lv_task handler.
       * It could be done in a timer interrupt or an OS task too.*/
      lv_timer_handler();
      usleep(10* 1000);
  }

demo_end:
  lv_deinit();
  return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static lv_display_t * hal_init(int32_t w, int32_t h)
{
  lv_group_set_default(lv_group_create());

  lv_display_t * disp = lv_sdl_window_create(w, h);

  lv_indev_t * mouse = lv_sdl_mouse_create();
  lv_indev_set_group(mouse, lv_group_get_default());
  lv_indev_set_display(mouse, disp);
  lv_display_set_default(disp);

  LV_IMAGE_DECLARE(mouse_cursor_icon); /*Declare the image file.*/
  lv_obj_t * cursor_obj;
  cursor_obj = lv_image_create(lv_screen_active()); /*Create an image object for the cursor */
  lv_image_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
  lv_indev_set_cursor(mouse, cursor_obj);             /*Connect the image  object to the driver*/

  lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
  lv_indev_set_display(mousewheel, disp);

  lv_indev_t * keyboard = lv_sdl_keyboard_create();
  lv_indev_set_display(keyboard, disp);
  lv_indev_set_group(keyboard, lv_group_get_default());

  return disp;
}
