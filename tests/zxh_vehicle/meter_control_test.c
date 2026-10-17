/* 使用真实协议、仪表后端和状态适配，只替代 RTOS、LCD 和音频硬件接口。 */
#define ZXH_PROTOCOL_TEST
#define ZXH_METER_BACKEND_TEST
#define APP_METER_CONTROL_TEST
#define TCFG_NETWORK_ENABLE 1
#define TCFG_NET_SCR_ENABLE 1
#include "zxh_vehicle_test_platform.h"
typedef int OS_SEM;
static int os_sem_create(OS_SEM *s,int count){*s=count;return 0;}
static int os_sem_pend(OS_SEM *s,int timeout){if(timeout==-1&&!*s)return 11;assert(*s==1);*s=0;return 0;}
static int os_sem_post(OS_SEM *s){assert(!*s);*s=1;return 0;}
static const char *os_current_task(void){return "app_core";}
static void sys_timer_del(unsigned short id){assert(id==1);}
enum stream_event { STREAM_EVENT_INIT,STREAM_EVENT_START,STREAM_EVENT_STOP };
static struct {const char *normal;} tones={"tone_zh/normal.*"};
static typeof(tones) *get_tone_files(void){return &tones;}
static int audio_busy,audio_failure,audio_calls,audio_stops;
static int a2dp_running,esco_running,all_tone_stops,a2dp_closes,esco_closes,mirror_stops,net_stops,volume_saves,net_stop_error;
static void tone_player_stop(void){++all_tone_stops;}
static int a2dp_player_runing(void){return a2dp_running;}
static int a2dp_player_get_btaddr(unsigned char *address){memset(address,0x38,6);return a2dp_running;}
static void a2dp_player_close(unsigned char *address){assert(address[0]==0x38);++a2dp_closes;a2dp_running=0;}
static int esco_player_runing(void){return esco_running;}
static void esco_player_close(void){++esco_closes;esco_running=0;}
static void screen_mirror_bt_h264_deinit(void){++mirror_stops;}
static int net_scr_shutdown(void){++net_stops;return net_stop_error;}
static void app_audio_volume_save_now(void){++volume_saves;}
static void *audio_priv;
static int (*audio_callback)(void *,enum stream_event);
static int tone_player_runing(void){return audio_busy;}
static int play_tone_file_alone_callback_psram(const char *file,void *priv,int (*cb)(void *,enum stream_event))
{assert(!strcmp(file,tones.normal));++audio_calls;audio_priv=priv;audio_callback=cb;return audio_failure;}
static void tone_player_stop_by_callback(void *priv,int (*cb)(void *,enum stream_event))
{if(priv==audio_priv&&cb==audio_callback){++audio_stops;cb(priv,STREAM_EVENT_STOP);}}
static int ui_calls,ui_result,ble_active=1,update_active;
static int service_off_calls,service_on_calls,service_off_error,service_on_error,services_on=1,live_vehicle;
static char power_trace[64];static unsigned trace_count;
static void trace(char c){if(trace_count<sizeof(power_trace)-1){power_trace[trace_count++]=c;power_trace[trace_count]=0;}}
int app_vehicle_suspend(void){trace('S');++service_off_calls;services_on=0;return service_off_error;}
int app_vehicle_resume(void){trace('R');++service_on_calls;services_on=!service_on_error;return service_on_error;}
int ui_update_page_is_active(void){return update_active;}
static int ui_power=1,ui_light,ui_mode=1;
int youyibao_sif_ui_meter_apply(int power_on,int light_on,int mode)
{trace(power_on?'L':'D');++ui_calls;if(ui_result)return ui_result;ui_power=power_on;ui_light=light_on;ui_mode=mode;return 0;}
void youyibao_sif_ui_update_wake(void){ui_power=1;}
#include "../../SDK/apps/watch/wifi/ctp/zxh_vehicle_protocol.c"
#include "../../SDK/apps/watch/common/app_meter_control.c"
#include "../../SDK/apps/watch/common/app_vehicle_zxh.c"
int zxh_ble_is_client(void *client){return client==(void *)0xb1000001u;}
int zxh_ble_client_active(void *client){return ble_active&&zxh_ble_is_client(client);}
int app_vehicle_try_snapshot(struct vehicle_state *state)
{memset(state,0,sizeof(*state));state->link=live_vehicle&&services_on?VEHICLE_LINK_ONLINE:VEHICLE_LINK_WAITING;
 state->values.valid=VEHICLE_DISTANCE_VALID;state->values.odometer_mm=12345000;
 if(live_vehicle&&services_on){state->values.valid|=VEHICLE_SOC_VALID|VEHICLE_SPEED_VALID;state->values.soc=83;state->values.speed_dkph=360;}return 0;}
static void check(int value){++assertions;assert(value);}
static void request(const char *id,const char *action,int value,const char *topic)
{
    struct json_object *p=json_object_new_object();char text[24];
    incoming=json_object_new_object();field(incoming,"op","PUT");json_object_object_add(incoming,"param",p);
    field(p,"v","1");field(p,"id",id);field(p,"action",action);snprintf(text,sizeof(text),"%d",value);field(p,"value",text);
    receive((void *)0xb1000001u,"fixture",topic);incoming=NULL;
}
static void clear_cache(void){now+=61001;protocol_poll(NULL);}
static void control(const char *id,const char *action,int value)
{clear_cache();request(id,action,value,"ZXH_CONTROL");}
static void apply_ui(void){meter_control_tick(NULL);app_meter_control_ui_poll();meter_control_tick(NULL);}
/** @brief 不清缓存、不等待一分钟，连续执行 100 次本机操作，复现第九次起报忙的问题。 */
static void repeated_controls_test(void)
{
    unsigned i;char id[24];int calls;
    clear_cache();
    for(i=0;i<100;++i){
        snprintf(id,sizeof(id),"rapid_%u",i);
        request(id,i%2?"MODE_SET":"LIGHT_SET",i%2?(i/2)%3:(i/2)%2,"ZXH_CONTROL");
        apply_ui();check(last_code==ZXH_OK&&last_confirmed);
        now+=100;
    }
    calls=ui_calls;request("rapid_99","MODE_SET",(99/2)%3,"ZXH_CONTROL");apply_ui();
    check(last_code==ZXH_OK&&last_confirmed&&ui_calls==calls);
    request("held","LIGHT_SET",1,"ZXH_CONTROL");
    request("overlap","MODE_SET",0,"ZXH_CONTROL");check(last_code==ZXH_BUSY);
    apply_ui();check(last_code==ZXH_OK&&ui_light==1);
    puts("Meter controls: 100 operations within 10 seconds, recent replay, pending ownership PASS");
}
/** @brief 验证收尾顺序、关闭数据屏蔽、失败回滚、迟到事件策略以及重复开关机。 */
static void power_lifecycle_test(void)
{
    struct app_meter_state state;int i;
    live_vehicle=1;
    a2dp_running=esco_running=1;
    request("live_bms","",0,"ZXH_BMS");check(!strcmp(reply_value("soc"),"83"));
    trace_count=0;power_trace[0]=0;
    control("power_off","LOCK",0);meter_control_tick(NULL);
    check(app_meter_control_snapshot(&state)==0&&state.power_state==APP_METER_STOPPING&&!state.power_on);
    request("closing_bms","",0,"ZXH_BMS");check(!strcmp(reply_value("soc"),"-1"));
    app_meter_control_ui_poll();check(!ui_power&&services_on);
    meter_control_tick(NULL);check(!strcmp(power_trace,"DS")&&!services_on&&last_confirmed);
    check(!a2dp_running&&!esco_running&&a2dp_closes&&esco_closes&&mirror_stops&&net_stops&&volume_saves&&all_tone_stops);
    check(!app_meter_control_media_allowed()&&!app_meter_control_tone_allowed(NULL,NULL));
    request("off_state","",0,"ZXH_STATE");check(!strcmp(reply_value("power_state"),"2")&&!strcmp(reply_value("speed_dkph"),"-1")&&!strcmp(reply_value("odometer_m"),"12345"));
    control("off_lamp","LIGHT_SET",1);check(last_code==ZXH_POWER_OFF&&!services_on&&!ui_power);
    control("off_mode","MODE_SET",0);check(last_code==ZXH_POWER_OFF&&!services_on);
    control("off_find","FIND",1000);meter_control_tick(NULL);audio_callback(audio_priv,STREAM_EVENT_START);meter_control_tick(NULL);
    check(last_code==0&&last_confirmed&&!services_on&&!app_meter_control_media_allowed());
    check(app_meter_control_tone_allowed(audio_priv,(void *)audio_callback)&&!app_meter_control_tone_allowed(NULL,NULL));
    now+=1000;meter_control_tick(NULL);check(!meter_audio_play&&!app_meter_control_tone_allowed(audio_priv,(void *)audio_callback));
    trace_count=0;power_trace[0]=0;
    control("power_on","UNLOCK",0);apply_ui();check(!strcmp(power_trace,"RL")&&services_on&&ui_power&&last_confirmed);
    check(app_meter_control_media_allowed());
    service_off_error=1;control("save_fail","LOCK",0);apply_ui();
    check(last_code==ZXH_EXECUTION_FAILED&&!last_confirmed&&!services_on&&app_meter_control_snapshot(&state)==0&&state.power_state==APP_METER_FAULT);
    service_off_error=0;control("save_retry","LOCK",0);apply_ui();check(last_code==0&&state.power_on==0);
    service_on_error=1;control("start_fail","UNLOCK",0);apply_ui();check(last_code==ZXH_EXECUTION_FAILED&&!services_on&&!ui_power);
    service_on_error=0;ui_result=-1;control("display_fail","UNLOCK",0);apply_ui();check(last_code==ZXH_EXECUTION_FAILED&&!services_on&&!ui_power);ui_result=0;
    control("display_timeout","UNLOCK",0);meter_control_tick(NULL);check(services_on);now+=7000;meter_control_tick(NULL);
    check(last_code==ZXH_EXECUTION_TIMEOUT&&!services_on&&!ui_power);
    for(i=0;i<20;++i){control("cycle_on","UNLOCK",0);apply_ui();check(last_code==0&&services_on&&ui_power);control("cycle_off","LOCK",0);apply_ui();check(last_code==0&&!services_on&&!ui_power);}
    control("restore_running","UNLOCK",0);apply_ui();check(last_code==0&&services_on&&ui_power);
    live_vehicle=0;
    puts("Meter lifecycle: UI exit -> save/release; acquire -> UI start; OFF telemetry; failure/timeout rollback; 20 cycles PASS");
}
int main(void)
{
    struct app_meter_state state;int before;
    check(!app_meter_control_actions());check(app_meter_control_start()==0);
    app_meter_control_ui_poll();check(zxh_protocol_init()==0);
    request("caps","",0,"ZXH_CAPS");
    check(!strcmp(last_actions,"LOCK|UNLOCK|LIGHT_SET|FIND|MODE_SET"));
    check(!strcmp(reply_value("control_scope"),"INSTRUMENT"));
    control("lamp_on","LIGHT_SET",1);check(!strcmp(last_phase,"ack")&&!last_confirmed);
    meter_control_tick(NULL);check(!ui_light&&!last_confirmed);app_meter_control_ui_poll();
    check(ui_light&&!last_confirmed);meter_control_tick(NULL);check(last_code==0&&last_confirmed);
    request("lamp_state","",0,"ZXH_STATE");check(!strcmp(reply_value("light_on"),"1")&&!strcmp(reply_value("locked"),"-1"));
    control("standby","LOCK",0);apply_ui();check(!ui_power&&!ui_light&&last_code==0&&last_confirmed);
    check(app_meter_control_snapshot(&state)==0&&!state.power_on&&!state.light_on);
    request("standby_state","",0,"ZXH_STATE");check(!strcmp(reply_value("power_on"),"0"));
    app_meter_control_ui_wake_for_update();check(ui_power&&app_meter_control_snapshot(&state)==0&&!state.power_on&&!services_on);
    control("wake","UNLOCK",0);apply_ui();check(ui_power&&last_code==0&&last_confirmed);
    before=ui_calls;request("wake","UNLOCK",0,"ZXH_CONTROL");apply_ui();check(ui_calls==before&&last_confirmed);
    control("sport","MODE_SET",2);apply_ui();check(ui_mode==2&&last_code==0);
    request("mode_state","",0,"ZXH_STATE");check(!strcmp(reply_value("mode"),"2")&&!strcmp(reply_value("power_on"),"1"));
    control("find","FIND",3000);meter_control_tick(NULL);check(audio_calls==1&&!last_confirmed);
    audio_callback(audio_priv,STREAM_EVENT_START);meter_control_tick(NULL);
    check(last_code==0&&last_confirmed&&app_meter_control_snapshot(&state)==0&&state.finding);
    now+=3000;meter_control_tick(NULL);check(audio_stops==1&&app_meter_control_snapshot(&state)==0&&!state.finding);
    audio_busy=1;control("find_busy","FIND",3000);meter_control_tick(NULL);check(last_code==ZXH_BUSY&&!last_confirmed);audio_busy=0;
    audio_failure=1;control("find_fail","FIND",3000);meter_control_tick(NULL);check(last_code==ZXH_EXECUTION_FAILED&&!meter_audio_play);audio_failure=0;
    ui_result=-2;control("ota","MODE_SET",0);apply_ui();check(last_code==ZXH_BUSY&&ui_mode==2);ui_result=0;
    update_active=1;control("ota_audio","FIND",3000);check(last_code==ZXH_BUSY);update_active=0;
    before=ui_calls;control("timeout","LOCK",0);meter_control_tick(NULL);now+=7000;meter_control_tick(NULL);
    app_meter_control_ui_poll();check(last_code==ZXH_EXECUTION_TIMEOUT&&ui_calls==before&&ui_power);
    control("cancel","LOCK",0);app_meter_control_disconnect((void *)0xb1000001u);meter_control_tick(NULL);app_meter_control_ui_poll();
    check(last_code==ZXH_AUTH_FAILED&&ui_calls==before&&ui_power);
    ble_active=0;control("inactive","UNLOCK",0);check(last_code==ZXH_AUTH_FAILED);ble_active=1;
    os_sem_pend(&meter_gate,0);check(app_meter_control_snapshot(&state)==ZXH_BUSY);os_sem_post(&meter_gate);
    repeated_controls_test();
    power_lifecycle_test();
    app_meter_control_stop();check(!app_meter_control_actions()&&app_meter_control_start()==-1);
    printf("PASS: %d real protocol/meter backend assertions\n",assertions);return 0;
}
