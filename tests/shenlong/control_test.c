#include "app_vehicle.h"
#include "vehicle_test_control.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
static struct vehicle_core core;
static uint32_t clock_ms;
int app_vehicle_aux_publish(enum vehicle_aux_source source,uint32_t known,uint32_t status,int gear,uint32_t timeout)
{ return vehicle_core_aux_publish(&core,source,known,status,gear,clock_ms,timeout); }
int app_vehicle_try_snapshot(struct vehicle_state *out)
{ *out=core.state;vehicle_core_aux_apply(&core,out,clock_ms);return 0; }
static void send(struct vehicle_test_control *t,struct sl_decoder *d,const char *line)
{ vehicle_test_feed(t,d,(const uint8_t *)line,strlen(line),clock_ms); }
static void check_state(unsigned flags,int gear,unsigned physical)
{
    struct vehicle_state s;app_vehicle_try_snapshot(&s);
    unsigned actual=((s.values.status>>13)&15)|((s.values.status&VEHICLE_REVERSE)?16:0)|((s.values.status&VEHICLE_CHARGING)?64:0);
    assert(actual==flags);assert(s.values.gear==gear);
    assert(s.physical_status==physical);
    assert(s.link==VEHICLE_LINK_WAITING&&s.motion==VEHICLE_MOTION_UNKNOWN&&!(s.values.valid&VEHICLE_SPEED_VALID));
}
int main(int argc,char **argv)
{
    struct vehicle_test_control t;struct sl_decoder d;
    const char *set="AT+ZXHSET=1,15,2\r\n";
    unsigned i;
    if(argc==2&&!strcmp(argv[1],"--server")) {
#ifdef _WIN32
        _setmode(_fileno(stdout),_O_BINARY); /* Preserve UART CRLF; avoid CRT CRCRLF translation. */
#endif
        char line[96];vehicle_core_init(&core,0);vehicle_test_init(&t);sl_decoder_init(&d,160);
        while(fgets(line,sizeof(line),stdin)) {
            clock_ms+=10;send(&t,&d,line);
            while(vehicle_test_reply(&t)){fputs(vehicle_test_reply(&t),stdout);vehicle_test_reply_sent(&t);}
            fflush(stdout);
        }
        return 0;
    }
    for(i=0;i<=strlen(set);i++) {
        vehicle_core_init(&core,0);vehicle_test_init(&t);sl_decoder_init(&d,160);
        vehicle_test_feed(&t,&d,(const uint8_t *)set,i,0);
        vehicle_test_feed(&t,&d,(const uint8_t *)set+i,strlen(set)-i,0);
        assert(!strcmp(vehicle_test_reply(&t),"+ZXH:1,OK,3,15,2,127,0,255\r\n"));
        assert(d.good==0&&d.errors==0);check_state(15,2,0);
    }
    vehicle_test_reply_sent(&t);
    clock_ms=1000;send(&t,&d,set);assert(strstr(vehicle_test_reply(&t),",OK,"));vehicle_test_reply_sent(&t);
    clock_ms=1500;check_state(0,-1,0); /* Retried ID must not renew the original lease. */
    send(&t,&d,"AT+ZXHSET=1,0,0\n");assert(strstr(vehicle_test_reply(&t),"ERROR,4"));vehicle_test_reply_sent(&t);
    send(&t,&d,"AT+ZXHSET=2,31,9\n");vehicle_test_reply_sent(&t);check_state(31,9,0);
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_LEFT_TURN|VEHICLE_HEADLIGHT,0,3,500));
    check_state(26,3,VEHICLE_LEFT_TURN|VEHICLE_HEADLIGHT); /* Physical OFF wins per field. */
    send(&t,&d,"AT+ZXHGET=3\n");assert(strstr(vehicle_test_reply(&t),",26,3,127,37"));vehicle_test_reply_sent(&t);
    clock_ms+=500;check_state(31,9,0);
    send(&t,&d,"AT+ZXHSET=4,32,2\n");assert(strstr(vehicle_test_reply(&t),"ERROR,2"));vehicle_test_reply_sent(&t);check_state(31,9,0);
    send(&t,&d,"AT+ZXHSET=4,1,2junk\n");assert(strstr(vehicle_test_reply(&t),"ERROR,1"));vehicle_test_reply_sent(&t);
    send(&t,&d,"AT+ZXHCLEAR=5\n");vehicle_test_reply_sent(&t);check_state(0,-1,0);
    /* Overlong/incomplete ASCII, corrupted binary then valid ASCII and OEM frame recover. */
    send(&t,&d,"AT+");clock_ms+=251;send(&t,&d,"AT+ZXHGET=6\n");assert(strstr(vehicle_test_reply(&t),",OK,"));vehicle_test_reply_sent(&t);
    char overflow[160];memset(overflow,'A',sizeof(overflow));overflow[159]='\n';
    vehicle_test_feed(&t,&d,(const uint8_t *)overflow,sizeof(overflow),clock_ms);assert(!vehicle_test_reply(&t));
    const uint8_t frame[]={2,14,1,0,0xa0,0x40,0x7b,61,0,153,80,0,45,0x4f};
    vehicle_test_feed(&t,&d,frame,sizeof(frame),clock_ms);assert(d.good==1);
    send(&t,&d,"AT+ZXHGET=7\n");assert(strstr(vehicle_test_reply(&t),",OK,"));vehicle_test_reply_sent(&t);
    uint8_t prefix[]={2,25,1};vehicle_test_feed(&t,&d,prefix,3,clock_ms);clock_ms+=31;
    send(&t,&d,"AT+ZXHGET=8\n");assert(strstr(vehicle_test_reply(&t),",OK,"));vehicle_test_reply_sent(&t);
    /* Full reply queue cannot apply a fifth unacknowledgeable command. */
    for(i=20;i<24;i++){char line[40];snprintf(line,sizeof(line),"AT+ZXHGET=%u\n",i);send(&t,&d,line);}
    send(&t,&d,"AT+ZXHSET=24,15,2\n");assert(t.count==4);check_state(0,-1,0);
    while(vehicle_test_reply(&t))vehicle_test_reply_sent(&t);
    clock_ms=0xfffffff0u;send(&t,&d,"AT+ZXHSET=25,1,255\n");vehicle_test_reply_sent(&t);
    clock_ms+=1000;check_state(1,-1,0);clock_ms+=500;check_state(0,-1,0);
    send(&t,&d,"AT+ZXHSET=30,64,255\n");vehicle_test_reply_sent(&t);check_state(64,-1,0);
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_CHARGING,0,-1,500));
    check_state(0,-1,VEHICLE_CHARGING);
    send(&t,&d,"AT+ZXHGET=31\n");assert(strstr(vehicle_test_reply(&t),",0,255,95,64"));vehicle_test_reply_sent(&t);
    clock_ms+=500;check_state(64,-1,0);
    send(&t,&d,"AT+ZXHSET=32,0,255\n");vehicle_test_reply_sent(&t);
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_CHARGING,VEHICLE_CHARGING,-1,50));
    check_state(64,-1,VEHICLE_CHARGING);clock_ms+=50;check_state(0,-1,0);
    clock_ms+=1500;check_state(0,-1,0);
    for (i=0;i<3;i++) {
        char line[64], expected[64];
        snprintf(line,sizeof(line),"AT+ZXHSET=%u,64,2,%u\n",100+i,i);
        snprintf(expected,sizeof(expected),",64,2,255,0,%u\r\n",i);
        send(&t,&d,line);assert(strstr(vehicle_test_reply(&t),expected));vehicle_test_reply_sent(&t);
        struct vehicle_state s;app_vehicle_try_snapshot(&s);assert(vehicle_values_ride_mode(&s.values)==(int)i);
    }
    assert(!app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_RIDE_MODE_MASK,VEHICLE_MODE_ECO,-1,500));
    send(&t,&d,"AT+ZXHGET=110\n");assert(strstr(vehicle_test_reply(&t),",255,128,0\r\n"));vehicle_test_reply_sent(&t);
    assert(app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_MODE_ECO,VEHICLE_MODE_ECO,-1,500)<0);
    assert(app_vehicle_aux_publish(VEHICLE_AUX_PHYSICAL,VEHICLE_RIDE_MODE_MASK,VEHICLE_MODE_ECO|VEHICLE_MODE_CITY,-1,500)<0);
    clock_ms+=500;
    send(&t,&d,"AT+ZXHSET=111,0,2,3\n");assert(strstr(vehicle_test_reply(&t),"ERROR,2"));vehicle_test_reply_sent(&t);
    struct vehicle_state s;app_vehicle_try_snapshot(&s);assert(vehicle_values_ride_mode(&s.values)==2);
    send(&t,&d,"AT+ZXHCLEAR=112\n");vehicle_test_reply_sent(&t);app_vehicle_try_snapshot(&s);
    assert(vehicle_values_ride_mode(&s.values)==-1);
    send(&t,&d,"AT+ZXHSET=113,0,2,0\n");vehicle_test_reply_sent(&t);clock_ms+=1500;app_vehicle_try_snapshot(&s);
    assert(vehicle_values_ride_mode(&s.values)==-1);
    puts("AT control: parsing/ACK/lease/physical priority; all riding modes, physical ECO override, invalid mode atomicity and expiry PASS");
    return 0;
}
