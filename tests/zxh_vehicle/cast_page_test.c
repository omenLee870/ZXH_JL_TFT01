#define ZXH_CAST_PAGE_TEST
#include "../../SDK/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/ui_cast_page.c"
static int checks;
#define CHECK(value) do {checks++;assert(value);} while(0)
static void run(void){void (*fn)(void *, ...)=queued;queued=NULL;if(fn)fn((void *)(uintptr_t)(uint32_t)queued_token);}
int main(void){
    current.id=GUI_SCREEN_CHONGWUYIBIAO;screen_map_request_display(1);
    CHECK(loads==0&&enters==0);run();CHECK(loads==1&&enters==1&&current.id==GUI_SCREEN_PAGE_MAP);
    screen_map_request_display(1);run();CHECK(loads==1&&enters==2);
    current.id=GUI_SCREEN_CHONGWUYIBIAO;screen_map_request_display(1);screen_map_request_display(0);run();CHECK(loads==1);
    for(int id=GUI_SCREEN_PAGE_VIDEO;id<=GUI_SCREEN_HARDWARE_TEST;id++){
        current.id=id;screen_map_request_display(1);run();CHECK(loads==1&&enters==2);
    }
    current.id=GUI_SCREEN_CHONGWUYIBIAO;update=1;screen_map_request_display(1);run();CHECK(loads==1);update=0;
    missing=1;screen_map_request_display(1);run();CHECK(loads==1);missing=2;
    screen_map_request_display(1);run();CHECK(loads==1);missing=0;
    refused=1;screen_map_request_display(1);run();CHECK(loads==2&&enters==2);refused=0;
    queue_failed=1;screen_map_request_display(1);CHECK(queued==NULL&&loads==2);queue_failed=0;
    cast_request=0xfffffffeu;screen_map_request_display(1);run();CHECK(current.id==GUI_SCREEN_PAGE_MAP&&enters==3);
    printf("PASS: %d cast UI dispatch assertions\n",checks);return 0;
}
