#define ZXH_PROTOCOL_TEST
#include "../../SDK/apps/watch/wifi/ctp/zxh_vehicle_protocol.c"
#include "../../SDK/apps/watch/common/app_vehicle_zxh.c"
static int authorization=ZXH_EXECUTION_FAILED, submit_code=ZXH_EXECUTION_FAILED, feedback;
static uint32_t submitted_token;
static uint32_t supported_action_mask=(1u<<ZXH_ACTION_COUNT)-1;
uint32_t zxh_vehicle_supported_actions(void) { return supported_action_mask; }
const char *zxh_vehicle_control_scope(void) { return "NONE"; }
int app_meter_control_snapshot(struct app_meter_state *state) { return ZXH_EXECUTION_FAILED; }
int zxh_vehicle_authorize(void *client,const struct zxh_request *r) { return authorization; }
static struct vehicle_state read_fixture;
static int read_result=APP_VEHICLE_NOT_READY;
int app_vehicle_try_snapshot(struct vehicle_state *state) { if(read_result)return read_result;*state=read_fixture;return 0; }
int zxh_ble_is_client(void *client) { return ((uintptr_t)client&0xff000000u)==0xb1000000u; }
int zxh_vehicle_submit(void *client,uint32_t token,const struct zxh_request *r,int *confirmed) {
    submissions++;submitted_token=token;*confirmed=feedback;return submit_code;
}
static void check(int value) { assertions++;assert(value); }
static void request(const char *id,const char *action,const char *value,const char *topic,const char *version,void *client) {
    struct json_object *p=json_object_new_object();incoming=json_object_new_object();
    field(incoming,"op","PUT");json_object_object_add(incoming,"param",p);
    field(p,"v",version);field(p,"id",id);field(p,"action",action);field(p,"value",value);
    receive(client,"fixture",topic);incoming=NULL;
}
static void control(const char *id,const char *action,const char *value) { request(id,action,value,"ZXH_CONTROL","1",(void *)1); }
int main(void) {
    int before, v;
    check(zxh_protocol_init()==0);
    control("one","UNLOCK","0");check(sends==2&&last_code==2001&&!strcmp(last_phase,"result"));check(submissions==0);
    before=sends;control("one","UNLOCK","0");check(sends==before+1&&last_code==2001&&submissions==0);
    control("one","LOCK","0");check(last_code==1005);
    control("two","UNKNOWN","0");check(last_code==1003);
    control("two","LIGHT_SET","2");check(last_code==1002);
    control("two","LOCK","1");check(last_code==1002);
    control("two","FIND","999");check(last_code==1002);
    control("two","FIND","10001");check(last_code==1002);
    control("two","MODE_SET","3");check(last_code==1002);
    control("two","CHARGE_TARGET_SET","69");check(last_code==1002);
    control("two","PROXIMITY_SET","-1");check(last_code==1002);
    control("bad id","LOCK","0");check(last_code==1002&&!strcmp(last_id,""));
    request("two","LOCK","0","ZXH_CONTROL","2",(void *)1);check(last_code==1001);
    check(parse_value("1000",ZXH_FIND,&v));check(parse_value("10000",ZXH_FIND,&v));
    check(parse_value("70",ZXH_CHARGE_TARGET_SET,&v));check(parse_value("100",ZXH_CHARGE_TARGET_SET,&v));
    check(!parse_value("1e3",ZXH_FIND,&v));check(!parse_value("1.0",ZXH_LIGHT_SET,&v));
    authorization=0;submit_code=0;feedback=0;
    control("unconfirmed","LOCK","0");check(last_code==2005&&last_confirmed==0);
    feedback=1;control("confirmed","LOCK","0");check(last_code==0&&last_confirmed==1);
    before=submissions;control("confirmed","LOCK","0");check(submissions==before&&last_code==0);
    submit_code=ZXH_PENDING;feedback=0;
    control("async","UNLOCK","0");check(last_code==0&&!strcmp(last_phase,"ack"));
    before=submissions;control("async","UNLOCK","0");check(submissions==before&&!strcmp(last_phase,"ack"));
    control("busy","LOCK","0");check(last_code==1004);
    before=sends;zxh_vehicle_complete(submitted_token+1,0,1);check(sends==before);
    zxh_vehicle_complete(submitted_token,0,1);check(last_code==0&&last_confirmed==1&&!strcmp(last_phase,"result"));
    before=sends;zxh_vehicle_complete(submitted_token,2001,0);check(sends==before);
    control("timeout","LOCK","0");now=8000;protocol_poll(NULL);check(last_code==2002&&last_confirmed==0);
    control("blocked","UNLOCK","0");check(last_code==1004);
    before=sends;zxh_vehicle_complete(submitted_token,0,1);check(sends==before);
    control("timeout","LOCK","0");check(last_code==2002);
    control("disconnect","LIGHT_SET","1");zxh_vehicle_disconnect((void *)1);
    before=sends;zxh_vehicle_complete(submitted_token,2001,0);check(sends==before);
    request("disconnect","LIGHT_SET","1","ZXH_CONTROL","1",(void *)2);check(last_code==2001);
    request("query","","0","ZXH_STATE","1",(void *)2);check(last_code==ZXH_AUTH_FAILED);
    request("query","","0","ZXH_BMS","1",(void *)2);check(last_code==ZXH_AUTH_FAILED);
    request("query","","0","ZXH_CAPS","1",(void *)2);check(last_code==0);
    /* The eighth retained command fills the cache; no ID is evicted early. */
    submit_code=2001;control("seventh","LOCK","0");check(last_code==2001);
    control("eighth","LOCK","0");check(last_code==2001);
    control("ninth","LOCK","0");check(last_code==1004);
    now=70001;protocol_poll(NULL);control("ninth","LOCK","0");check(last_code==2001);
    /* Unsigned elapsed time remains correct across uptime wrap. */
    now=0xfffffff0u;submit_code=ZXH_PENDING;control("wrap","UNLOCK","0");
    now=0xfffffff0u+8000u;protocol_poll(NULL);check(last_code==2002);
    zxh_vehicle_complete(submitted_token,2001,0);
    /* 能力列表必须只列出已接通的操作，未知位不得生成虚假操作。 */
    supported_action_mask=(1u<<ZXH_LIGHT_SET)|(1u<<ZXH_MODE_SET);
    request("selected_caps","","0","ZXH_CAPS","1",(void *)2);
    check(last_code==0&&!strcmp(last_actions,"LIGHT_SET|MODE_SET"));
    supported_action_mask=0;
    request("receive_only_caps","","0","ZXH_CAPS","1",(void *)2);
    check(last_code==0&&!strcmp(last_actions,""));
    supported_action_mask=1u<<31;
    request("unknown_caps","","0","ZXH_CAPS","1",(void *)2);
    check(last_code==0&&!strcmp(last_actions,""));
    now+=70001u;protocol_poll(NULL);
    before=submissions;control("unsupported_backend","LOCK","0");
    check(last_code==ZXH_ACTION_ERROR&&last_confirmed==0&&submissions==before);
    /* 使用实际只读适配代码核对 APP 回复，未知字段不能被填成零或其他状态。 */
    read_result=0;
    read_fixture.link=VEHICLE_LINK_ONLINE;
    read_fixture.values.valid=VEHICLE_SPEED_VALID|VEHICLE_GEAR_VALID|VEHICLE_SOC_VALID|
        VEHICLE_CURRENT_VALID|VEHICLE_DISTANCE_VALID|VEHICLE_STATUS_VALID|VEHICLE_FAULTS_VALID;
    read_fixture.values.speed_dkph=360;read_fixture.values.gear=3;read_fixture.values.soc=83;
    read_fixture.values.current_ma=-12000;read_fixture.values.odometer_mm=12345678;
    read_fixture.values.trip_mm=2345600;read_fixture.ride_time_ms=3723000;
    read_fixture.values.status=VEHICLE_PARK|VEHICLE_BRAKE;
    read_fixture.values.known_status=VEHICLE_PARK|VEHICLE_BRAKE;
    read_fixture.values.faults=VEHICLE_HALL_FAULT;
    request("live_state","","0","ZXH_STATE","1",(void *)0xb1000001u);
    check(last_code==0&&!strcmp(reply_value("speed_dkph"),"360"));
    check(!strcmp(reply_value("gear"),"3")&&!strcmp(reply_value("link"),"1"));
    check(!strcmp(reply_value("odometer_m"),"12345")&&!strcmp(reply_value("trip_m"),"2345"));
    check(!strcmp(reply_value("ride_time_s"),"3723")&&!strcmp(reply_value("faults"),"1"));
    check(!strcmp(reply_value("locked"),"-1")&&!strcmp(reply_value("mode"),"-1"));
    check(!strcmp(reply_value("status"),"33")&&!strcmp(reply_value("known_status"),"33"));
    request("live_bms","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(last_code==0&&!strcmp(reply_value("soc"),"83")&&!strcmp(reply_value("current_ma"),"-12000"));
    check(!strcmp(reply_value("voltage_mv"),"-1")&&!strcmp(reply_value("temperature_dc"),"-32768"));
    check(!strcmp(reply_value("charging"),"-1")&&!strcmp(reply_value("cycles"),"-1"));
    read_fixture.values.speed_dkph=0;read_fixture.values.soc=0;
    request("zero_speed","","0","ZXH_STATE","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("speed_dkph"),"0"));
    request("zero_soc","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("soc"),"0"));
    read_fixture.link=VEHICLE_LINK_LOST;
    read_fixture.values.valid=VEHICLE_DISTANCE_VALID|VEHICLE_FAULTS_VALID;
    read_fixture.values.faults=VEHICLE_COMMUNICATION_FAULT;
    request("lost_state","","0","ZXH_STATE","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("link"),"2")&&!strcmp(reply_value("speed_dkph"),"-1"));
    check(!strcmp(reply_value("gear"),"-1")&&!strcmp(reply_value("status"),"-1"));
    check(!strcmp(reply_value("odometer_m"),"12345")&&!strcmp(reply_value("faults"),"512"));
    request("lost_bms","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("soc"),"-1")&&!strcmp(reply_value("current_ma"),"-2147483648"));
    read_fixture.values.valid|=VEHICLE_STATUS_VALID;
    read_fixture.values.known_status=read_fixture.values.status=VEHICLE_CHARGING;
    read_fixture.auxiliary_status=VEHICLE_CHARGING;
    request("aux_charge","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("charging"),"1")&&!strcmp(reply_value("soc"),"-1"));
    request("aux_charge_state","","0","ZXH_STATE","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("status"),"131072")&&!strcmp(reply_value("link"),"2"));
    read_fixture.values.status=0;
    request("aux_charge_off","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("charging"),"0"));
    read_fixture.auxiliary_status=0;
    request("aux_charge_expired","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("charging"),"-1"));
    read_fixture.values.odometer_mm=UINT64_MAX;read_fixture.ride_time_ms=UINT64_MAX;
    request("large_counts","","0","ZXH_STATE","1",(void *)0xb1000001u);
    check(!strcmp(reply_value("odometer_m"),"-1")&&!strcmp(reply_value("ride_time_s"),"-1"));
    read_fixture.values.known_status=VEHICLE_RIDE_MODE_MASK;
    read_fixture.auxiliary_status=VEHICLE_RIDE_MODE_MASK;
    for(unsigned mode=0;mode<3;++mode) {
        char id[24], expected[2]={(char)('0'+mode),0};
        read_fixture.values.status=VEHICLE_MODE_ECO<<mode;
        snprintf(id,sizeof(id),"aux_mode_%u",mode);
        request(id,"","0","ZXH_STATE","1",(void *)0xb1000001u);
        check(!strcmp(reply_value("mode"),expected));
    }
    read_result=APP_VEHICLE_BUSY;
    request("read_busy","","0","ZXH_STATE","1",(void *)0xb1000001u);
    check(last_code==ZXH_BUSY&&!strlen(reply_value("speed_dkph")));
    read_result=APP_VEHICLE_NOT_READY;
    request("read_not_ready","","0","ZXH_BMS","1",(void *)0xb1000001u);
    check(last_code==ZXH_EXECUTION_FAILED);
    check(zxh_vehicle_read_state(NULL)==ZXH_PARAMETER_ERROR&&zxh_vehicle_read_bms(NULL)==ZXH_PARAMETER_ERROR);
    printf("PASS: %d firmware protocol assertions\n",assertions);return 0;
}
