#define ZXH_PROTOCOL_TEST
#include "../../SDK/apps/watch/wifi/ctp/zxh_vehicle_protocol.c"
static int authorization=ZXH_EXECUTION_FAILED, submit_code=ZXH_EXECUTION_FAILED, feedback;
static uint32_t submitted_token;
int zxh_vehicle_authorize(void *client,const struct zxh_request *r) { return authorization; }
int zxh_vehicle_authorize_read(void *client,const char *topic) { return ZXH_EXECUTION_FAILED; }
int zxh_vehicle_submit(uint32_t token,const struct zxh_request *r,int *confirmed) {
    submissions++;submitted_token=token;*confirmed=feedback;return submit_code;
}
int zxh_vehicle_read_state(struct zxh_state *s) { return ZXH_EXECUTION_FAILED; }
int zxh_vehicle_read_bms(struct zxh_bms *b) { return ZXH_EXECUTION_FAILED; }
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
    request("query","","0","ZXH_STATE","1",(void *)2);check(last_code==2001);
    request("query","","0","ZXH_BMS","1",(void *)2);check(last_code==2001);
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
    printf("PASS: %d firmware protocol assertions\n",assertions);return 0;
}
