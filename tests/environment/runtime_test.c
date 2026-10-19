/* Real environment state machine; replace only RTC, Flash, RTOS and BLE transport. */
#define METER_ENVIRONMENT_TEST 1
#include "../zxh_vehicle/zxh_vehicle_test_platform.h"
#include "zxh_vehicle_protocol.h"
#include "user_cfg_id.h"
typedef int OS_SEM;
struct sys_time { unsigned year, month, day, hour, min, sec; };
static struct sys_time rtc;
static unsigned char clock_blob[32], weather_blob[32];
static int clock_size,weather_size,writes,fail_write,updating;
static int os_sem_create(OS_SEM *s,int n){*s=n;return 0;}
static int os_sem_pend(OS_SEM *s,int timeout){(void)timeout;assert(*s==1);*s=0;return 0;}
static int os_sem_post(OS_SEM *s){assert(!*s);*s=1;return 0;}
static const char *os_current_task(void){return "app_core";}
static void rtc_read_time(struct sys_time *t){*t=rtc;}
static void rtc_write_time(struct sys_time *t){rtc=*t;}
static int syscfg_read(int id,void *p,int n){int size=id==CFG_METER_CLOCK?clock_size:weather_size;
    if(size!=n)return -1;
    memcpy(p,id==CFG_METER_CLOCK?clock_blob:weather_blob,n);return n;}
static int syscfg_write(int id,const void *p,int n){++writes;if(fail_write)return -1;
    assert(n<=32);memcpy(id==CFG_METER_CLOCK?clock_blob:weather_blob,p,n);
    if(id==CFG_METER_CLOCK)clock_size=n;else weather_size=n;return n;}
#define VM_ERR_NONE 0
#define VM_RAM_FLUSH_NONE 1
#define Q_CALLBACK 7
static int get_vm_ram_storage_enable(void){return 1;}
static int get_vm_ram_storage_in_irq_enable(void){return 0;}
static int vm_flush2flash(int ignored){(void)ignored;return 0;}
static void set_local_time_init_flag(unsigned char value){assert(!value);}
static int os_taskq_post_type(const char *task,int type,int count,int *argv){
    (void)task;(void)type;(void)count;(void)argv;return 0;}
int ui_update_page_is_active(void){return updating;}
int zxh_ble_client_active(void *client){return client==(void *)1;}
int zxh_ble_send(void *client,const char *json){assert(client==(void *)1&&json);return 0;}
#include "../../SDK/apps/watch/common/meter_environment.c"
static void boot(void){ready=0;weather_saved_this_boot=0;weather_saved_ms=0;reply_next=0;
    memset(replies,0,sizeof(replies));assert(!meter_environment_init());}
static void request_weather(unsigned id,int temperature,unsigned condition){
    char value[32],key[33];struct json_object *p=json_object_new_object();
    incoming=json_object_new_object();field(incoming,"op","PUT");json_object_object_add(incoming,"param",p);
    snprintf(key,sizeof(key),"%032x",id);field(p,"id",key);field(p,"v","1");field(p,"action","WEATHER");
    snprintf(value,sizeof(value),"%u",state.utc);field(p,"observed_utc_s",value);
    snprintf(value,sizeof(value),"%d",temperature);field(p,"temp_dc",value);
    snprintf(value,sizeof(value),"%u",condition);field(p,"weather",value);
    assert(meter_environment_receive((void *)1,"ZXH_ENV","{}"));incoming=NULL;
}
int main(void){
    struct meter_environment p;
    meter_environment_snapshot(&p);assert(!p.clock_valid&&strcmp(p.clock,"--:--")&&strcmp(p.date,"-- --"));
    boot();meter_environment_snapshot(&p);assert(!p.clock_valid&&!p.weather_cached&&p.temperature_dc==250);
    uint32_t before=p.utc;now=60000;tick(NULL);meter_environment_snapshot(&p);assert(p.utc==before+60);
    rtc=(struct sys_time){2026,10,19,14,36,0};boot();meter_environment_snapshot(&p);
    assert(p.clock_valid&&!strcmp(p.clock,"14:36")&&!strcmp(p.date,"10-19 周一"));
    assert(set_clock(p.utc,480)==ZXH_OK);assert(clock_size==sizeof(saved));
    request_weather(1,183,METER_WEATHER_RAIN);assert(last_code==ZXH_OK);
    meter_environment_snapshot(&p);assert(p.weather_valid&&p.weather_cached&&p.temperature_dc==183&&weather_size==sizeof(saved_weather));
    int previous_writes=writes;
    request_weather(2,192,METER_WEATHER_CLOUD);assert(writes==previous_writes&&state.temperature_dc==192);
    now+=WEATHER_TTL*1000u;tick(NULL);meter_environment_snapshot(&p);
    assert(!p.weather_valid&&p.weather_cached&&p.temperature_dc==192&&p.weather==METER_WEATHER_CLOUD);
    request_weather(3,-55,METER_WEATHER_SNOW);assert(state.weather_valid&&state.temperature_dc==-55&&writes==previous_writes+1);
    boot();meter_environment_snapshot(&p);assert(!p.weather_valid&&p.weather_cached&&p.temperature_dc==-55&&p.weather==METER_WEATHER_SNOW);
    request_weather(4,300,METER_WEATHER_UNKNOWN);assert(state.temperature_dc==-55&&state.weather_cached&&!state.weather_valid);
    updating=1;previous_writes=writes;request_weather(5,201,METER_WEATHER_CLEAR);assert(writes==previous_writes&&state.weather_valid);updating=0;
    fail_write=1;request_weather(6,205,METER_WEATHER_FOG);assert(state.temperature_dc==205&&state.weather_valid);
    fail_write=0;boot();assert(state.weather_cached&&state.temperature_dc==-55&&state.weather==METER_WEATHER_SNOW);
    weather_blob[4]^=1;boot();assert(!state.weather_cached&&state.temperature_dc==250);
    memset(&rtc,0,sizeof(rtc));boot();assert(!state.clock_valid&&state.utc==saved.utc&&strcmp(state.clock,"--:--"));
    now=0xfffffff0u;tick_ms=now;before=state.utc;now+=2000;tick(NULL);assert(state.utc==before+2);
    puts("Environment: RTC without marker, default clock ticks, cached weather expiry/reboot, CRC, rate limit, OTA and wrap PASS");
    return 0;
}
