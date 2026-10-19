#define YOUYIBAO_UI_TEST 1
#define APP_VEHICLE_TEST 1
#define CONFIG_UCOS_ENABLE 1
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "youyibao_sif.h"
#include "shenlong_protocol.h"
#include "vehicle_test_control.h"

typedef struct { unsigned short nesting; unsigned available; const char *owner; } OS_MUTEX,OS_SEM;
static uint32_t mock_now;
static const char *mock_task = "app_core";
static int mock_mutex_error, mock_timer_error, mock_lock_depth, mock_reset_error, mock_accept_error;
enum { Q_CALLBACK = 7, MOCK_QUEUE_CAPACITY = 4 };
static int mock_queue[MOCK_QUEUE_CAPACITY][4];
static unsigned mock_queue_head, mock_queue_count, mock_reset_count, mock_post_count;
static struct yyb_view mock_view;
static unsigned mock_deleted_timers, mock_shutdown_count;
static int mock_shutdown_error;
/* 忠实复现当前 SDK 的差异：Pend 建立 nesting=1，Accept 没有设置 nesting，Post 无条件减一。
 * 保留旧接口模拟，保证回退到旧实现时，首次 APP 查询后的锁泄漏会被回归检查发现。 */
static int os_mutex_create(OS_MUTEX *m) { memset(m,0,sizeof(*m));m->available=1;return mock_mutex_error; }
static int os_mutex_pend(OS_MUTEX *m, int timeout)
{
    (void)timeout;
    if(!m->available){assert(m->owner==mock_task);++m->nesting;return 0;}
    m->available=0;m->owner=mock_task;m->nesting=1;mock_lock_depth=1;return 0;
}
static int os_mutex_post(OS_MUTEX *m)
{
    if(--m->nesting)return 0;
    m->available=1;m->owner=NULL;mock_lock_depth=0;return 0;
}
struct os_event;
unsigned char OSMutexAccept(struct os_event *event)
{
    OS_MUTEX *m=(OS_MUTEX *)event;
    if (mock_accept_error || !m->available) return 0;
    m->available=0;m->owner=mock_task;mock_lock_depth=1;
    return 1;
}
static int os_sem_create(OS_SEM *s,int count)
{ assert(count==1);memset(s,0,sizeof(*s));s->available=1;return mock_mutex_error; }
static int os_sem_pend(OS_SEM *s,int timeout)
{
    if(timeout==-1&&(mock_accept_error||!s->available))return 11;
    assert(s->available&&!mock_lock_depth);s->available=0;mock_lock_depth=1;return 0;
}
static int os_sem_post(OS_SEM *s)
{ assert(!s->available&&mock_lock_depth);s->available=1;mock_lock_depth=0;return 0; }
static uint32_t sys_timer_get_ms(void) { return mock_now; }
static const char *os_current_task(void) { return mock_task; }
static unsigned short sys_timer_add(void *p, void (*cb)(void *), uint32_t ms)
{ (void)p; (void)cb; assert(ms == 50); return mock_timer_error ? 0 : 1; }
static void sys_timer_del(unsigned short id)
{ assert(id == 1 && !mock_lock_depth); ++mock_deleted_timers; }
void youyibao_sif_snapshot(struct yyb_view *v) { *v = mock_view; }
int youyibao_sif_shutdown(void)
{
    assert(!strcmp(mock_task, "app_core") && !mock_lock_depth);
    ++mock_shutdown_count;
    mock_view.online = 0; mock_view.speed_dkph = -1;
    mock_now += 250; /* Flash 耗时不能算进骑行时间。 */
    return mock_shutdown_error;
}
static int os_taskq_post_type(const char *task, int type, int argc, int *argv)
{
    assert(!strcmp(task, "app_core") && type == Q_CALLBACK && argc == 4);
    assert(!mock_lock_depth); /* 提交操作不能持有车辆互斥锁。 */
    ++mock_post_count;
    if (mock_queue_count == MOCK_QUEUE_CAPACITY) return -1;
    memcpy(mock_queue[(mock_queue_head + mock_queue_count) % MOCK_QUEUE_CAPACITY], argv, 4 * sizeof(int));
    ++mock_queue_count;
    return 0;
}
int youyibao_sif_reset_trip(void)
{
    assert(!strcmp(mock_task, "app_core"));
    assert(!mock_lock_depth); /* 协议操作不能嵌套在车辆互斥锁内。 */
    ++mock_reset_count;
    if (mock_reset_error) return mock_reset_error;
    mock_view.trip_mm = 0;
    return 0;
}
static int mock_start_error, mock_source_starts;
int youyibao_sif_start(void) { ++mock_source_starts;mock_view.online=0;mock_view.soc_valid=0;mock_view.speed_dkph=-1;return mock_start_error; }
#include "../../SDK/apps/watch/common/app_vehicle.c"
#include "app_meter_control.h"
static struct app_meter_state mock_meter={1,0,1,0};
static int mock_backlight_failure, mock_backlight_on=1;
void app_meter_control_stop(void) {}
void app_meter_control_ui_poll(void) {}
int app_meter_control_media_allowed(void) { return mock_meter.power_on; }
int app_meter_control_snapshot(struct app_meter_state *state) { *state=mock_meter;return ZXH_OK; }
int meter_ui_test_backlight(int power_on) { if(mock_backlight_failure)return -1;mock_backlight_on=power_on;return 0; }
#include "../../SDK/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/youyibao_sif_ui.c"
#include "../../SDK/apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1/custom/chongwuyibiao_donghua.c"

static void update_ui(void) { vehicle_poll(NULL); refresh(NULL); }

/**
 * @brief 验证控制器字段换算、报文时效和未提供字段的有效位。
 * @retval 无。
 * @note 直接读取接入接口，检查输出不依赖界面刷新，也不把读取时间当作接收时间。
 */
static void vehicle_controller_test(void)
{
    struct yyb_view saved = mock_view;
    struct vehicle_input input;
    assert(vehicle_controller_capabilities() ==
        (VEHICLE_CONTROLLER_TELEMETRY | VEHICLE_CONTROLLER_RESET_TRIP));
    mock_view.online = 1;
    mock_view.last_frame_ms = 123;
    mock_view.timeout_ms = 4500;
    mock_view.speed_dkph = 360;
    mock_view.gear = 7;
    mock_view.current_a = -12;
    mock_view.soc = 83; mock_view.soc_valid = 1;
    mock_view.current_percent = 70; mock_view.current_percent_valid = 1;
    mock_view.flags = YYB_BRAKE | YYB_REVERSE | YYB_REGEN;
    mock_view.faults = YYB_HALL_FAULT | YYB_STALL;
    mock_view.odometer_mm = 123456789; mock_view.trip_mm = 12345;
    memset(&input, 0xff, sizeof(input));
    vehicle_controller_read(&input);
    assert(input.source == VEHICLE_SOURCE_YYB && input.online);
    assert(input.received_at_ms == 123 && input.timeout_ms == 4500);
    assert(input.values.speed_dkph == 360 && input.values.current_ma == -12000);
    assert(input.values.gear == 7 && input.values.soc == 83 && input.values.load_percent == 70);
    assert(input.values.status == (VEHICLE_BRAKE | VEHICLE_REVERSE | VEHICLE_REGEN));
    assert(input.values.faults == (VEHICLE_HALL_FAULT | VEHICLE_STALL));
    assert(input.values.odometer_mm == 123456789 && input.values.trip_mm == 12345);
    assert(!(input.values.valid & VEHICLE_VOLTAGE_VALID));
    mock_view.soc_valid = mock_view.current_percent_valid = 0;
    vehicle_controller_read(&input);
    assert(!(input.values.valid & (VEHICLE_SOC_VALID | VEHICLE_LOAD_VALID)));
    vehicle_controller_read(NULL);
    mock_view = saved;
    puts("Vehicle controller: capabilities, units, field validity, frame age, mileage PASS");
}

/** @brief 回归首次 APP 查询后 UI 停刷、后续查询永久忙的问题；交替模拟 APP、UI 和业务更新。 */
static void vehicle_snapshot_gate_test(void)
{
    struct vehicle_state state;
    unsigned i;
    assert(!os_sem_pend(&vehicle_gate,0));
    memset(&state,0x5a,sizeof(state));
    assert(app_vehicle_try_snapshot(&state)==APP_VEHICLE_BUSY);
    assert(state.values.speed_dkph==0x5a5a5a5a&&!vehicle_gate.available&&mock_lock_depth==1);
    os_sem_post(&vehicle_gate);
    for(i=0;i<2000;++i){
        mock_task="app_core";
        assert(!app_vehicle_try_snapshot(&state)&&!mock_lock_depth&&vehicle_gate.available==1);
        mock_task="ui";
        app_vehicle_snapshot(&state);
        assert(!mock_lock_depth&&vehicle_gate.available==1&&state.values.soc==83);
        mock_task="app_core";
        vehicle_poll(NULL);
        assert(!mock_lock_depth&&vehicle_gate.available==1);
    }
    puts("Vehicle snapshot: APP/UI/poll 2000 cycles, busy ownership, balanced release PASS");
}

/**
 * @brief 模拟 SDK 从队列复制消息后，在 app_core 执行回调。
 * @retval 无。
 * @note 发送方与执行方分开推进，用于检查异步边界及队列满时的行为。
 */
static void dispatch_vehicle_command(void)
{
    int message[4];
    const char *sender = mock_task;
    assert(mock_queue_count);
    memcpy(message, mock_queue[mock_queue_head], sizeof(message));
    mock_queue_head = (mock_queue_head + 1) % MOCK_QUEUE_CAPACITY;
    --mock_queue_count;
    assert(message[1] == 2);
    mock_task = "app_core";
    ((void (*)(int,unsigned))message[0])(message[2],message[3]);
    mock_task = sender;
}

/**
 * @brief 验证跨任务操作的异步执行、线程归属、队列满及清零后的状态一致性。
 * @retval 无。
 */
static void vehicle_command_test(void)
{
    struct vehicle_state before, after;
    unsigned posts, i;
    app_vehicle_snapshot(&before);
    mock_task = "ui";
    assert(app_vehicle_post((enum app_vehicle_command)0) == APP_VEHICLE_UNSUPPORTED);
    assert(mock_queue_count == 0);
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_QUEUED);
    app_vehicle_snapshot(&after);
    assert(after.values.trip_mm == before.values.trip_mm && mock_reset_count == 0);
    /* 即使直接误调处理函数，其他线程也不能执行车辆操作或更新业务状态。 */
    vehicle_command_callback(APP_VEHICLE_CMD_RESET_TRIP,vehicle_generation);
    mock_view.gear = 7;
    vehicle_poll(NULL);
    app_vehicle_snapshot(&after);
    assert(after.values.gear == before.values.gear && mock_reset_count == 0);
    mock_view.gear = before.values.gear;
    dispatch_vehicle_command();
    app_vehicle_snapshot(&after);
    assert(mock_reset_count == 1 && after.values.trip_mm == 0);
    assert(after.values.odometer_mm == before.values.odometer_mm);
    assert(after.ride_time_ms == before.ride_time_ms);
    assert(after.link == before.link && after.motion == before.motion);
    mock_task = "btstack";
    for (i = 0; i < MOCK_QUEUE_CAPACITY; ++i)
        assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_QUEUED);
    posts = mock_post_count;
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_BUSY);
    assert(mock_post_count == posts + 1 && mock_reset_count == 1);
    while (mock_queue_count) dispatch_vehicle_command();
    assert(mock_reset_count == 1 + MOCK_QUEUE_CAPACITY);
    /* app_core 自己提交也必须排队，避免因调用线程不同而出现两种执行顺序。 */
    mock_task = "app_core";
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_QUEUED);
    assert(mock_reset_count == 1 + MOCK_QUEUE_CAPACITY);
    dispatch_vehicle_command();
    /* 底层操作失败时，统一状态不得伪造小计清零成功。 */
    mock_view.trip_mm = 9876;
    vehicle_poll(NULL);
    mock_reset_error = -1;
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_QUEUED);
    dispatch_vehicle_command();
    app_vehicle_snapshot(&after);
    assert(after.values.trip_mm == 9876 && mock_view.trip_mm == 9876);
    mock_reset_error = 0;
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_QUEUED);
    dispatch_vehicle_command();
    app_vehicle_snapshot(&after);
    assert(after.values.trip_mm == 0 && after.values.odometer_mm == before.values.odometer_mm);
    puts("Vehicle RTOS: deferred commands, task ownership, queue full, trip/ODO consistency PASS");
}

/** @brief 使用真实业务模块验证资源反复申请释放，以及上一周期消息和关机时长隔离。 */
static void vehicle_power_cycle_test(void)
{
    struct vehicle_state before, after;unsigned resets=mock_reset_count;int i;
    mock_view.online=1;mock_view.speed_dkph=360;mock_view.last_frame_ms=mock_now;
    vehicle_poll(NULL);app_vehicle_snapshot(&before);
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP)==APP_VEHICLE_QUEUED);
    for(i=0;i<20;++i){
        assert(!app_vehicle_suspend()&&!vehicle_timer);
        assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP)==APP_VEHICLE_NOT_READY);
        mock_now+=10000;
        assert(!app_vehicle_resume()&&vehicle_timer);
        app_vehicle_snapshot(&after);
        assert(after.values.odometer_mm==before.values.odometer_mm&&after.ride_time_ms==before.ride_time_ms);
        assert(after.values.speed_dkph==-1&&!(after.values.valid&VEHICLE_SOC_VALID));
    }
    dispatch_vehicle_command();assert(mock_reset_count==resets);
    assert(!app_vehicle_suspend());mock_start_error=1;
    assert(app_vehicle_resume()==-1&&!vehicle_timer);mock_start_error=0;
    mock_timer_error=1;assert(app_vehicle_resume()==-1&&!vehicle_timer);mock_timer_error=0;
    assert(!app_vehicle_resume());
    mock_deleted_timers=mock_shutdown_count=0;
    puts("Vehicle lifecycle: 20 capture/timer restart cycles, fresh telemetry, preserved ODO/time, stale message, allocation rollback PASS");
}

/** @brief 验证关机冻结、排队消息拦截、失败重试和重复调用。 */
static void vehicle_shutdown_test(void)
{
    struct vehicle_state before, after;
    unsigned resets = mock_reset_count;
    mock_view.online = 1; mock_view.speed_dkph = 360;
    mock_view.last_frame_ms = mock_now;
    vehicle_poll(NULL);
    app_vehicle_snapshot(&before);
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_QUEUED);
    mock_task = "ui";
    assert(app_vehicle_prepare_shutdown() == -1 && !mock_deleted_timers && !mock_shutdown_count);
    mock_task = "app_core";
    mock_shutdown_error = -1;
    assert(app_vehicle_prepare_shutdown() == -1);
    assert(mock_deleted_timers == 1 && mock_shutdown_count == 1);
    dispatch_vehicle_command();
    assert(mock_reset_count == resets);
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_NOT_READY);
    assert(app_vehicle_try_snapshot(&after) == APP_VEHICLE_NOT_READY && app_vehicle_start() == -1);
    app_vehicle_snapshot(&after);
    assert(after.motion == VEHICLE_MOTION_UNKNOWN && after.link == VEHICLE_LINK_LOST);
    assert(after.values.odometer_mm == before.values.odometer_mm);
    assert(after.values.trip_mm == before.values.trip_mm && after.ride_time_ms == before.ride_time_ms);
    mock_shutdown_error = 0;
    assert(!app_vehicle_prepare_shutdown() && mock_shutdown_count == 2 && mock_deleted_timers == 1);
    mock_task = "tone_play";
    assert(!app_vehicle_prepare_shutdown() && mock_shutdown_count == 2);
    mock_task = "app_core";
    puts("Vehicle shutdown: task ownership, timer stop, queued callback guard, retry, idempotence PASS");
}

lv_ui guider_ui;
lv_font_t lv_font_Lato_Regular_24;
static unsigned notice_active;
static unsigned ui_switch_count;
static bool ota_active;
static lv_ui_chongwuyibiao pet;
static unsigned char pixels[1024 * 600 * 2] __attribute__((aligned(32)));

int pet_reverse_notice_request(bool on) { notice_active = on; return 0; }
int ui_update_page_is_active(void) { return ota_active; }
void neicun_zhuizong_jieduan(const char *stage) { (void)stage; }
/** @brief 状态联动测试使用真实动画状态机，省略图片文件加载。 */
char *gui_get_res_path(int32_t id) { (void)id; return NULL; }
void *ui_get_scr_ptr(lv_ui *ui, int32_t id) { (void)ui; return id == GUI_SCREEN_CHONGWUYIBIAO ? &pet : NULL; }
gui_scr_t *ui_get_setup_scr(int32_t id) { return gui_scr_get(id); }
void ui_load_scr_anim(lv_ui *ui, gui_scr_t *s, lv_screen_load_anim_t anim,
                      uint32_t time, uint32_t delay, bool clean, bool del, bool push)
{
    ++ui_switch_count;
    gui_scr_set_act_anim(s, ui, anim, time, delay, clean, del);
    (void)push;
}
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *p)
{ (void)a; (void)p; lv_display_flush_ready(d); }
static lv_obj_t *label(lv_obj_t *root, int x, int y)
{
    lv_obj_t *o = lv_label_create(root);
    lv_obj_set_pos(o,x,y);
    lv_label_set_text(o,"sample");
    return o;
}

/**
 * @brief 创建显示联动测试使用的宠物页控件。
 * @retval 无。
 * @note 包含未接入数据的预览图标，用于验证首次显示和页面重建后的隐藏规则。
 */
static void create_pet_widgets(void)
{
    memset(&pet, 0, sizeof(pet));
    pet.chongwuyibiao=lv_obj_create(NULL);
    pet.chongwuyibiao_xiaogou=lv_image_create(pet.chongwuyibiao);
    pet.chongwuyibiao_yun1=lv_image_create(pet.chongwuyibiao);
    pet.chongwuyibiao_yun2=lv_image_create(pet.chongwuyibiao);
    pet.chongwuyibiao_yun3=lv_image_create(pet.chongwuyibiao);
    pet.chongwuyibiao_yun4=lv_image_create(pet.chongwuyibiao);
    pet.chongwuyibiao_chesushuzi=label(pet.chongwuyibiao,150,206);
    pet.chongwuyibiao_chesudanwei=label(pet.chongwuyibiao,308,253);
    pet.chongwuyibiao_jiashimoshi=label(pet.chongwuyibiao,715,466);
    pet.chongwuyibiao_dianliangshuzi=label(pet.chongwuyibiao,160,120);
    pet.chongwuyibiao_dianliangtiao=lv_bar_create(pet.chongwuyibiao);
    pet.chongwuyibiao_dianchirongqi=lv_obj_create(pet.chongwuyibiao);
    lv_obj_set_pos(pet.chongwuyibiao_dianchirongqi,350,116);
    lv_obj_set_size(pet.chongwuyibiao_dianchirongqi,50,24);
    pet.chongwuyibiao_guzhangxinxi=label(pet.chongwuyibiao,390,466);
    pet.chongwuyibiao_shachetubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_xunhangtubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_lichengbiaoti=label(pet.chongwuyibiao,155,466);
    pet.chongwuyibiao_lichengshuzi=label(pet.chongwuyibiao,223,466);
    pet.chongwuyibiao_lichengdanwei=label(pet.chongwuyibiao,326,466);
    pet.chongwuyibiao_wendu=label(pet.chongwuyibiao,650,253);
    pet.chongwuyibiao_shijian=label(pet.chongwuyibiao,715,213);
    pet.chongwuyibiao_riqixingqi=label(pet.chongwuyibiao,715,253);
    pet.chongwuyibiao_tianqitubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_zuozhuanxiang=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_youzhuanxiang=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_jinguangdengtubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_yuanguangdengtubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_shikuodengtubiao=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_xinhaozhu_1=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_xinhaozhu_2=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_xinhaozhu_3=lv_obj_create(pet.chongwuyibiao);
    pet.chongwuyibiao_xinhaozhu_4=lv_obj_create(pet.chongwuyibiao);
}

/**
 * @brief 验证宠物页的数据有效性、显示切换、状态图标与故障恢复。
 * @retval 无。
 */
static void pet_vehicle_display_test(void)
{
    struct yyb_view saved = mock_view;
    unsigned resets = mock_reset_count, posts = mock_post_count;
    char first_fault[32];
    mock_view.trip_mm=2345600;
    update_ui();
    youyibao_sif_ui_bind_pet(&pet); /* 重复绑定不能使一次点击切换两次。 */
    lv_obj_send_event(pet.chongwuyibiao_lichengbiaoti,LV_EVENT_CLICKED,NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengbiaoti),"TRIP"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"2.3"));
    lv_obj_send_event(pet.chongwuyibiao_lichengdanwei,LV_EVENT_CLICKED,NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengbiaoti),"TIME"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengdanwei),"H:M"));
    latest.ride_time_ms=3723000;pet_distance_refresh(&pet);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"01:02"));
    lv_obj_send_event(pet.chongwuyibiao_lichengshuzi,LV_EVENT_CLICKED,NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengbiaoti),"ODO"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"12.3"));
    assert(mock_reset_count==resets&&mock_post_count==posts);
    latest.values.odometer_mm=UINT64_MAX;pet_distance_refresh(&pet);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"18446744073709.5"));
    mock_view.flags=YYB_CRUISE|YYB_EBRAKE;
    update_ui();
    assert(!lv_obj_has_flag(pet.chongwuyibiao_xunhangtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_shachetubiao,LV_OBJ_FLAG_HIDDEN));
    /* 缺少有效位时不能把字段值或残留图标当作有效反馈。 */
    vehicle.state.values.valid &= ~(VEHICLE_GEAR_VALID|VEHICLE_STATUS_VALID);
    refresh(NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"CITY / --"));
    assert(lv_obj_has_flag(pet.chongwuyibiao_xunhangtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_shachetubiao,LV_OBJ_FLAG_HIDDEN));
    mock_view.flags=0;mock_view.speed_dkph=0;mock_view.soc=0;
    update_ui();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"0")&&zhuangtai.chesu==0&&!zhuangtai.paobu);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"0%"));
    mock_view.soc_valid=0;update_ui();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"--%"));
    mock_view.flags=YYB_PARK;update_ui();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"CITY / P"));
    mock_view.faults=YYB_HALL_FAULT|YYB_THROTTLE_FAULT;update_ui();
    strcpy(first_fault,lv_label_get_text(pet.chongwuyibiao_guzhangxinxi));
    lv_tick_inc(1500);update_ui();
    assert(strcmp(first_fault,lv_label_get_text(pet.chongwuyibiao_guzhangxinxi)));
    mock_view.faults=0;update_ui();
    assert(lv_obj_has_flag(pet.chongwuyibiao_guzhangxinxi,LV_OBJ_FLAG_HIDDEN));
    ota_active=true;
    lv_obj_send_event(pet.chongwuyibiao_lichengbiaoti,LV_EVENT_CLICKED,NULL);
    assert(pet_distance_mode==PET_DISTANCE_ODO);
    ota_active=false;
    mock_view=saved;update_ui();
    puts("Pet vehicle UI: ODO/TRIP/TIME, zero/unknown, cruise/brake, fault rotation/recovery PASS");
}

/** @brief 统计真实 LVGL 定时器，检查页面反复退出后没有遗留或重复创建。 */
static unsigned ui_timer_count(void)
{
    unsigned count=0;lv_timer_t *timer=NULL;
    while((timer=lv_timer_get_next(timer))!=NULL)++count;
    return count;
}

/** @brief 执行真实 LVGL 显示入口，验证大灯、模式、关闭页及重复开关机。 */
static void meter_display_test(void)
{
    unsigned children=lv_obj_get_child_count(pet.chongwuyibiao);
    assert(!youyibao_sif_ui_meter_apply(1,1,2));mock_meter.light_on=1;mock_meter.mode=2;
    update_ui();
    assert(!lv_obj_has_flag(pet.chongwuyibiao_jinguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(strstr(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"SPORT / ")==lv_label_get_text(pet.chongwuyibiao_jiashimoshi));
    assert(!youyibao_sif_ui_meter_apply(0,0,2));mock_meter.power_on=0;mock_meter.light_on=0;
    assert(!mock_backlight_on&&!lv_obj_has_flag(meter_standby_cover,LV_OBJ_FLAG_HIDDEN));
    assert(lv_screen_active()==meter_standby_cover&&!zhuangtai.yunxing&&!zhuangtai.yun_dingshiqi&&!zhuangtai.paobu_dingshiqi);
    if(refresh_timer)assert(lv_timer_get_paused(refresh_timer));
    mock_backlight_failure=1;
    assert(youyibao_sif_ui_meter_apply(1,0,2)==-1&&!meter_display.power_on);
    mock_backlight_failure=0;
    assert(!youyibao_sif_ui_meter_apply(1,0,0));mock_meter.power_on=1;mock_meter.mode=0;
    /* 关闭页是没有父对象的屏幕根节点，切出后也禁止设置 HIDDEN。
     * 本机 SDK 再移除 HIDDEN 会直接使空父对象布局失效，导致第二次关机异常。 */
    assert(mock_backlight_on&&!lv_obj_has_flag(meter_standby_cover,LV_OBJ_FLAG_HIDDEN));
    assert(lv_screen_active()==pet.chongwuyibiao&&zhuangtai.yunxing);
    assert(lv_obj_has_flag(pet.chongwuyibiao_jinguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(strstr(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"ECO / ")==lv_label_get_text(pet.chongwuyibiao_jiashimoshi));
    assert(lv_obj_get_child_count(pet.chongwuyibiao)==children&&lv_obj_get_y(pet.chongwuyibiao_jiashimoshi)==466);
    ota_active=true;assert(youyibao_sif_ui_meter_apply(0,1,1)==-2);ota_active=false;
    assert(!youyibao_sif_ui_meter_apply(1,0,1));mock_meter.mode=1;
    lv_obj_t *off_screen=meter_standby_cover;
    unsigned running_timers=ui_timer_count();
    for(unsigned cycle=0;cycle<100;++cycle){
        assert(!youyibao_sif_ui_meter_apply(0,0,1));mock_meter.power_on=0;
        assert(meter_standby_cover==off_screen&&lv_screen_active()==off_screen);
        assert(!lv_obj_get_parent(off_screen)&&!lv_obj_has_flag(off_screen,LV_OBJ_FLAG_HIDDEN));
        assert(!mock_backlight_on&&!zhuangtai.yunxing&&!zhuangtai.yun_dingshiqi&&!zhuangtai.paobu_dingshiqi);
        lv_tick_inc(100);lv_timer_handler();
        assert(!youyibao_sif_ui_meter_apply(1,0,1));mock_meter.power_on=1;
        assert(lv_screen_active()==pet.chongwuyibiao&&mock_backlight_on&&zhuangtai.yunxing);
        assert(!lv_obj_has_flag(off_screen,LV_OBJ_FLAG_HIDDEN)&&ui_timer_count()==running_timers);
        lv_tick_inc(100);lv_timer_handler();
    }
    puts("Meter LVGL: 100 OFF/ON/OFF cycles, root screen never hidden, stable timer ownership PASS");
    puts("Meter LVGL: headlight on/off, ECO/CITY/SPORT, standby/wake, backlight error, OTA guard PASS");
}
static void export_ppm(const char *name, lv_display_t *display)
{
    unsigned i;
    lv_refr_now(display);
    FILE *f = fopen(name,"wb"); assert(f);
    fprintf(f,"P6\n1024 600\n255\n");
    for (i=0;i<1024*600;++i) {
        uint16_t p = pixels[i*2] | ((uint16_t)pixels[i*2+1] << 8);
        unsigned char rgb[3] = {(p>>11)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};
        fwrite(rgb,1,3,f);
    }
    fclose(f);
}
int main(void)
{
    struct vehicle_state state;
    vehicle_controller_test();
    memset(&state,0x5a,sizeof(state));
    assert(app_vehicle_try_snapshot(&state)==APP_VEHICLE_NOT_READY);
    assert(state.values.speed_dkph==0x5a5a5a5a);
    app_vehicle_snapshot(&state);
    assert(state.link == VEHICLE_LINK_WAITING && state.values.speed_dkph == -1);
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_NOT_READY);
    mock_task = "ui";
    assert(app_vehicle_start() == -1);
    mock_task = "app_core"; mock_mutex_error = 1;
    assert(app_vehicle_start() == -1);
    mock_mutex_error = 0;
    lv_init(); gui_scr_init();
    lv_font_t *font = lv_freetype_font_create("C:/Windows/Fonts/arial.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,24,LV_FREETYPE_FONT_STYLE_NORMAL);
    assert(font); lv_font_Lato_Regular_24 = *font;
    lv_display_t *display=lv_display_create(1024,600);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,pixels,NULL,sizeof(pixels),LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display,flush);
    create_pet_widgets();
    lv_label_set_text(pet.chongwuyibiao_chesushuzi,"46");
    chongwuyibiao_donghua_bangding(&pet);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"--")&&zhuangtai.chesu==0);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_guzhangxinxi),"SIF WAIT"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"--"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_shijian),"--:--"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_riqixingqi),"-- --"));
    assert(lv_obj_has_flag(pet.chongwuyibiao_zuozhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_youzhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_jinguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_yuanguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_shikuodengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_tianqitubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_xinhaozhu_4,LV_OBJ_FLAG_HIDDEN));
    gui_scr_t *pet_scr=gui_scr_create(GUI_SCREEN_CHONGWUYIBIAO,"pet",pet.chongwuyibiao,NULL,NULL);
    gui_scr_t *video=gui_scr_create(GUI_SCREEN_PAGE_VIDEO,"camera",lv_obj_create(NULL),NULL,NULL);
    gui_scr_set_act_anim(pet_scr,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    mock_view.online=1; mock_view.speed_dkph=360; mock_view.soc=83; mock_view.soc_valid=1;
    mock_view.timeout_ms=1000;
    mock_view.gear=3; mock_view.flags=YYB_BRAKE; mock_view.odometer_mm=12345000;
    mock_view.trip_mm=45000;
    mock_timer_error=1;
    assert(app_vehicle_start() == -1);
    assert(app_vehicle_post(APP_VEHICLE_CMD_RESET_TRIP) == APP_VEHICLE_NOT_READY);
    mock_timer_error=0;
    assert(app_vehicle_start() == 0 && app_vehicle_start() == 0);
    mock_accept_error=1;
    assert(app_vehicle_try_snapshot(&state)==APP_VEHICLE_BUSY);
    mock_accept_error=0;
    assert(app_vehicle_try_snapshot(&state)==0&&state.values.speed_dkph==360);
    /* 首次 STATE 成功后必须已经释放状态锁，不能让后续 BMS 一直忙、UI 一直等待。 */
    assert(mock_lock_depth==0);
    vehicle_snapshot_gate_test();
    app_vehicle_snapshot(&state);
    assert(state.values.status == VEHICLE_BRAKE && state.values.voltage_mv == -1);
    state.values.speed_dkph=999;
    app_vehicle_snapshot(&state);
    assert(state.values.speed_dkph==360); /* 读取方修改副本不能影响内部状态。 */
    /* 即使应用线程没有继续更新，快照也必须在有效期结束后失效。 */
    mock_now=1000;
    assert(app_vehicle_try_snapshot(&state)==0&&state.link==VEHICLE_LINK_LOST);
    app_vehicle_snapshot(&state);
    assert(state.link==VEHICLE_LINK_LOST && state.motion==VEHICLE_MOTION_UNKNOWN);
    assert(state.ride_time_ms==1000 && state.values.odometer_mm==12345000);
    mock_now=2000;
    app_vehicle_snapshot(&state);
    assert(state.ride_time_ms==1000);
    vehicle_poll(NULL);
    app_vehicle_snapshot(&state);
    assert(state.link==VEHICLE_LINK_LOST); /* 重复发布过期数据不能使通信恢复在线。 */
    mock_view.last_frame_ms=mock_now;
    vehicle_poll(NULL);
    vehicle_command_test();
    youyibao_sif_ui_init();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"36"));
    assert(zhuangtai.chesu==36&&zhuangtai.paobu);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"83%"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"12.3"));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_shachetubiao,LV_OBJ_FLAG_HIDDEN));
    pet_vehicle_display_test();
    meter_display_test();
    mock_view.faults=YYB_HALL_FAULT; update_ui();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_guzhangxinxi),"HALL"));
    export_ppm("output/youyibao_dashboard.ppm",display);
    mock_view.flags=YYB_REVERSE; update_ui();
    assert(gui_scr_get_act()==video && notice_active);
    mock_view.online=0; update_ui();
    assert(gui_scr_get_act()==pet_scr && !notice_active);
    mock_view.online=1;mock_view.flags=0;update_ui();
    /* Manually leaving automatic reverse cancels ownership, without reopening. */
    mock_view.flags=YYB_REVERSE; update_ui();
    gui_scr_set_act_anim(pet_scr,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    update_ui(); assert(gui_scr_get_act()==pet_scr && !notice_active);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"CITY / R"));
    mock_view.online=0; mock_view.speed_dkph=-1; mock_view.soc_valid=0; update_ui();
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"--"));
    assert(zhuangtai.chesu==0&&!zhuangtai.paobu);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"--%"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_guzhangxinxi),"SIF OFFLINE"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengshuzi),"12.3"));
    ota_active=true; update_ui(); ota_active=false; update_ui();
    mock_meter.light_on=1;mock_meter.mode=2;update_ui();
    gui_scr_set_act_anim(video,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    lv_obj_delete(pet.chongwuyibiao);
    assert(pet_root==NULL);
    assert(pet_charge_badge==NULL);
    update_ui();
    create_pet_widgets();
    chongwuyibiao_donghua_bangding(&pet);
    pet_scr->scr=pet.chongwuyibiao;
    gui_scr_set_act_anim(pet_scr,&guider_ui,LV_SCR_LOAD_ANIM_NONE,0,0,false,false);
    lv_obj_send_event(pet.chongwuyibiao_lichengbiaoti,LV_EVENT_CLICKED,NULL);
    assert(pet_distance_mode==PET_DISTANCE_TRIP);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_lichengbiaoti),"TRIP"));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_chesushuzi),"--"));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_jinguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"SPORT / --"));
    /* Actual serial bytes -> decoder -> vehicle core -> real LVGL lamp widgets. */
    struct sl_decoder sl;
    uint8_t lamps_frame[]={2,18,1,0,0x80,0x40,0,0,15,160,80,0,45,'Z','X',1,15,0};
    lamps_frame[17]=sl_xor(lamps_frame,17);
    sl_decoder_init(&sl,160);assert(!sl_parse_feedback(&sl,lamps_frame,18,mock_now));
    vehicle_core_update(&vehicle,&sl.input,mock_now);
    lv_tick_inc((1000-lv_tick_get()%1000)%1000);refresh(NULL);
    assert(!lv_obj_has_flag(pet.chongwuyibiao_jinguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_shikuodengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_zuozhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_youzhuanxiang,LV_OBJ_FLAG_HIDDEN));
    lv_tick_inc(500);refresh(NULL);
    assert(lv_obj_has_flag(pet.chongwuyibiao_zuozhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_youzhuanxiang,LV_OBJ_FLAG_HIDDEN));
    lamps_frame[16]=0;lamps_frame[17]=sl_xor(lamps_frame,17);
    assert(!sl_parse_feedback(&sl,lamps_frame,18,mock_now));vehicle_core_update(&vehicle,&sl.input,mock_now);refresh(NULL);
    assert(lv_obj_has_flag(pet.chongwuyibiao_jinguangdengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(pet.chongwuyibiao_shikuodengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_xinhaozhu_4,LV_OBJ_FLAG_HIDDEN));
    mock_now+=1001;refresh(NULL);
    assert(lv_obj_has_flag(pet.chongwuyibiao_shikuodengtubiao,LV_OBJ_FLAG_HIDDEN));
    puts("Youyibao/serial UI: telemetry, reverse, offline, OTA, deletion, ZXH lamp decode/display, turn blink and signal bars PASS");
    /* Independent test commands and future physical inputs share the same UI path,
     * including when the controller telemetry itself is offline. */
    mock_view.online=0;update_ui();
    struct vehicle_test_control at;vehicle_test_init(&at);
    const char *request="AT+ZXHSET=101,15,3\r\n";
    vehicle_test_feed(&at,&sl,(const uint8_t *)request,strlen(request),mock_now);
    assert(strstr(vehicle_test_reply(&at),"+ZXH:101,OK,3,15,3,127,0"));vehicle_test_reply_sent(&at);
    lv_tick_inc((1000-lv_tick_get()%1000)%1000);refresh(NULL);
    assert(!lv_obj_has_flag(pet.chongwuyibiao_zuozhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_youzhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"SPORT / GEAR 3"));
    assert(latest.link==VEHICLE_LINK_LOST); /* AT must not fake UART/SIF online. */
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_LEFT_TURN,0,1,500));refresh(NULL);
    assert(lv_obj_has_flag(pet.chongwuyibiao_zuozhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(pet.chongwuyibiao_youzhuanxiang,LV_OBJ_FLAG_HIDDEN));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"SPORT / GEAR 1"));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,0,0,-1,0));
    request="AT+ZXHSET=102,16,2\r\n";
    vehicle_test_feed(&at,&sl,(const uint8_t *)request,strlen(request),mock_now);refresh(NULL);
    assert(gui_scr_get_act()==video&&notice_active);
    vehicle_test_reply_sent(&at);
    unsigned reverse_switches=ui_switch_count;
    for(unsigned cycle=0;cycle<3;cycle++) {
        for(unsigned pause=0;pause<9;pause++) {
            mock_now+=100;lv_tick_inc(100);refresh(NULL);
            assert(gui_scr_get_act()==video&&notice_active&&latest.values.gear==2);
            assert(ui_switch_count==reverse_switches);
        }
        char renewal[48];snprintf(renewal,sizeof(renewal),"AT+ZXHSET=%u,16,2\r\n",104+cycle);
        vehicle_test_feed(&at,&sl,(const uint8_t *)renewal,strlen(renewal),mock_now);
        vehicle_test_reply_sent(&at);
    }
    request="AT+ZXHCLEAR=103\r\n";
    vehicle_test_feed(&at,&sl,(const uint8_t *)request,strlen(request),mock_now);refresh(NULL);
    assert(gui_scr_get_act()==pet_scr&&!notice_active);
    assert(lv_obj_has_flag(pet.chongwuyibiao_shikuodengtubiao,LV_OBJ_FLAG_HIDDEN));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_TEST,VEHICLE_LEFT_TURN,VEHICLE_LEFT_TURN,2,1500));
    mock_now+=1500;refresh(NULL);
    assert(lv_obj_has_flag(pet.chongwuyibiao_zuozhuanxiang,LV_OBJ_FLAG_HIDDEN));
    puts("AT/physical -> common vehicle state -> actual LVGL: lamp OFF priority, gear, offline independence, reverse page, CLEAR and expiry PASS");
    /* Original charging feedback, independent AT and physical OFF/ON share one badge. */
    while(vehicle_test_reply(&at))vehicle_test_reply_sent(&at);
    assert(pet_charge_badge&&lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    request="AT+ZXHSET=200,64,2\r\n";
    vehicle_test_feed(&at,&sl,(const uint8_t *)request,strlen(request),mock_now);refresh(NULL);
    vehicle_test_reply_sent(&at);
    assert(!lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_CHARGING,0,-1,1000));refresh(NULL);
    assert(lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,0,0,-1,0));refresh(NULL);
    assert(!lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    request="AT+ZXHCLEAR=201\r\n";
    vehicle_test_feed(&at,&sl,(const uint8_t *)request,strlen(request),mock_now);refresh(NULL);vehicle_test_reply_sent(&at);
    assert(lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    lamps_frame[4]|=8;lamps_frame[17]=sl_xor(lamps_frame,17);
    assert(!sl_parse_feedback(&sl,lamps_frame,18,mock_now));vehicle_core_update(&vehicle,&sl.input,mock_now);refresh(NULL);
    assert(!lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_dianliangshuzi),"80%"));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_TEST,VEHICLE_CHARGING,0,2,1500));refresh(NULL);
    assert(lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_TEST,0,0,-1,0));refresh(NULL);
    assert(!lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    mock_now+=1001;refresh(NULL);assert(lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_CHARGING,VEHICLE_CHARGING,-1,1000));refresh(NULL);
    assert(!lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    mock_now+=1000;refresh(NULL);assert(lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    puts("Reverse continuity: three 900ms renewal gaps, stable GEAR/video without repeated switches PASS");
    puts("Charging LVGL: AT ON, physical OFF priority, CLEAR, OEM feedback, timeout, physical ON/expiry and unchanged SOC PASS");
    for(unsigned mode=0;mode<3;++mode) {
        static const char *names[]={"ECO / GEAR 2","CITY / GEAR 2","SPORT / GEAR 2"};
        char line[64];snprintf(line,sizeof(line),"AT+ZXHSET=%u,64,2,%u\r\n",300+mode,mode);
        vehicle_test_feed(&at,&sl,(const uint8_t *)line,strlen(line),mock_now);vehicle_test_reply_sent(&at);refresh(NULL);
        assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),names[mode]));
        static const uint32_t expected_colors[]={0x31c48d,0x36a9ff,0xff9f43};
        assert(lv_color_eq(lv_obj_get_style_text_color(pet.chongwuyibiao_jiashimoshi,0),lv_color_hex(expected_colors[mode])));
        assert(lv_obj_get_child_count(pet_charge_badge)==0);
        assert(lv_obj_get_style_bg_opa(pet_charge_badge,0)==LV_OPA_TRANSP);
        lv_obj_update_layout(pet.chongwuyibiao);
        assert(lv_obj_get_x(pet_charge_badge)==406&&lv_obj_get_y(pet_charge_badge)==115);
    }
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_RIDE_MODE_MASK,VEHICLE_MODE_ECO,-1,500));refresh(NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"ECO / GEAR 2"));
    mock_now+=500;refresh(NULL);assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"SPORT / GEAR 2"));
    export_ppm("output/vehicle-v36-mode-charge.ppm",display);
    request="AT+ZXHCLEAR=399\r\n";vehicle_test_feed(&at,&sl,(const uint8_t *)request,strlen(request),mock_now);
    vehicle_test_reply_sent(&at);refresh(NULL);
    assert(!strcmp(lv_label_get_text(pet.chongwuyibiao_jiashimoshi),"SPORT / --"));
    assert(lv_obj_has_flag(pet_charge_badge,LV_OBJ_FLAG_HIDDEN));
    puts("AT -> actual LVGL ECO/CITY/SPORT, physical mode priority, CLEAR fallback and battery-side lightning without text/background PASS");
    vehicle_power_cycle_test();
    vehicle_shutdown_test();
    return 0;
}
