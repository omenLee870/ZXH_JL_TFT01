#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
enum {GUI_SCREEN_PAGE_MAP, GUI_SCREEN_PAGE_VIDEO, GUI_SCREEN_UPGRADE_PAGE,
      GUI_SCREEN_HARDWARE_TEST, GUI_SCREEN_CHONGWUYIBIAO};
#define LV_SCR_LOAD_ANIM_NONE 0
typedef struct {int id;} gui_scr_t;
static gui_scr_t current, pet={GUI_SCREEN_CHONGWUYIBIAO};
static int guider_ui, update, missing, loads, enters, refused, queue_failed;
static int animation_allowed=1;
static void chongwuyibiao_donghua_yunxu(bool allowed){animation_allowed=allowed;}
static void (*queued)(void *, ...);
static int queued_token;
static gui_scr_t *gui_scr_get_act(void){return missing==2?NULL:&current;}
static int ui_update_page_is_active(void){return update;}
static gui_scr_t *ui_get_scr(int id){assert(id==GUI_SCREEN_CHONGWUYIBIAO);return missing==1?NULL:&pet;}
static void ui_load_scr_anim(void *ui,gui_scr_t *scr,int anim,int time,int delay,bool clean,bool del,bool push){
    assert(ui==&guider_ui&&!anim&&!time&&!delay&&!clean&&!del&&!push);
    loads++;if(!refused)current=*scr;
}
static void map_video_enter(void){enters++;}
int lvgl_rpc_post_func(void (*fn)(void *, ...),int argc,...){
    va_list ap;assert(argc==1);va_start(ap,argc);queued_token=va_arg(ap,int);va_end(ap);
    if(queue_failed)return -1;queued=fn;return 0;
}
