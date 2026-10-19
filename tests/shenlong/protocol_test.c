#include "shenlong_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Independently worked vector: 153ms wheel period, 0x407b = 12.3A,
 * brake on, 80% SOC, 45km range; last byte XOR=4f. */
static const uint8_t sample[] = {2,14,1,0,0xa0,0x40,0x7b,61,0,153,80,0,45,0x4f};
static void fix(uint8_t *p, unsigned n) { p[n-1] = sl_xor(p,n-1); }
static void decode_cli(void)
{
    char line[256];
    while (fgets(line,sizeof(line),stdin)) {
        uint8_t data[25];unsigned count=0,value;char *p=line;int advance;
        struct sl_decoder d;
        while (count<25 && sscanf(p,"%x%n",&value,&advance)==1) { data[count++]=value;p+=advance; }
        sl_decoder_init(&d,160);
        if (sl_parse_feedback(&d,data,count,100)) { puts("ERROR");continue; }
        printf("%d %d %d %u %u\n",d.input.values.speed_dkph,d.input.values.current_ma,
            d.input.values.soc,d.input.values.status,d.input.values.faults);
    }
}
int main(int argc,char **argv)
{
    struct sl_decoder d;
    uint8_t f[25],out[25];unsigned i,j;
    struct sl_command c={.drive=2,.assist=10,.control=0xa0,.magnets=46,
        .diameter_tenth_inch=160,.sensitivity=12,.start_strength=1,.speed_limit=60,
        .current_limit=20,.undervoltage_tenth_v=420,.control2=8};
    if (argc>1 && !strcmp(argv[1],"--decode")) {decode_cli();return 0;}
    sl_decoder_init(&d,160);
    assert(!sl_parse_feedback(&d,sample,sizeof(sample),100));
    assert(d.input.values.speed_dkph==300);
    assert(d.input.values.current_ma==12300);
    assert(d.input.values.soc==80 && d.remaining_km==45);
    assert(d.input.values.status==VEHICLE_BRAKE);
    assert(!d.input.values.faults);
    memcpy(f,sample,14);f[4]|=8;fix(f,14);
    assert(!sl_parse_feedback(&d,f,14,100));
    assert((d.input.values.status & VEHICLE_CHARGING) && (d.input.values.known_status & VEHICLE_CHARGING));
    assert(!sl_parse_feedback(&d,sample,14,100));assert(!(d.input.values.status & VEHICLE_CHARGING));
    for(i=0;i<14;i++) for(j=0;j<8;j++) {
        memcpy(f,sample,14);f[i]^=1u<<j;
        assert(sl_parse_feedback(&d,f,14,300)<0);
        assert(d.input.received_at_ms==100);
    }
    for(i=0;i<=14;i++) {
        sl_decoder_init(&d,160);
        sl_feed(&d,sample,i,100);sl_feed(&d,sample+i,14-i,110);
        assert(d.good==1 && d.used==0);
    }
    for(i=14;i<=25;i++) {
        memcpy(f,sample,13);memset(f+13,0,i-13);f[1]=i;fix(f,i);
        sl_decoder_init(&d,160);sl_feed(&d,f,i,10);assert(d.good==1);
    }
    for(i=0;i<16;i++) {
        memcpy(f,sample,13);f[1]=18;f[13]='Z';f[14]='X';f[15]=1;f[16]=i;fix(f,18);
        sl_decoder_init(&d,160);sl_feed(&d,f,18,100);
        assert(d.good==1 && d.input.values.status==(VEHICLE_BRAKE | (i<<13)));
        assert((d.input.values.known_status & (15u<<13))==(15u<<13));
        assert(!sl_parse_feedback(&d,sample,14,110));
        assert(!(d.input.values.known_status & (15u<<13)));
        assert(!(d.input.values.status & (15u<<13)));
    }
    f[15]=2;fix(f,18);assert(!sl_parse_feedback(&d,f,18,120));
    assert(!(d.input.values.known_status & (15u<<13))); /* Unknown version is not lamp data. */
    sl_decoder_init(&d,160);
    memset(f,0xff,25);sl_feed(&d,f,25,10);
    sl_feed(&d,sample,5,11);sl_feed(&d,sample,14,100);assert(d.good==1);
    for(i=0;i<32;i++) {
        sl_decoder_init(&d,160);memcpy(f,sample,14);f[3]=1u<<(i%8);f[4]=0x80;fix(f,14);
        assert(!sl_parse_feedback(&d,f,14,10));
        if(i%8==6)assert(d.input.values.faults==VEHICLE_HALL_FAULT);
    }
    sl_decoder_init(&d,160);memcpy(f,sample,14);
    f[5]=0;f[6]=16;fix(f,14);assert(!sl_parse_feedback(&d,f,14,0));assert(d.input.values.current_ma==16000);
    f[5]=0x40;fix(f,14);assert(!sl_parse_feedback(&d,f,14,1));assert(d.input.values.current_ma==1600);
    f[5]=0x80;fix(f,14);assert(sl_parse_feedback(&d,f,14,2)<0);
    memcpy(f,sample,14);f[8]=f[9]=0;fix(f,14);assert(!sl_parse_feedback(&d,f,14,3));assert(!(d.input.values.valid & VEHICLE_SPEED_VALID));
    f[8]=15;f[9]=160;f[10]=0;fix(f,14);assert(!sl_parse_feedback(&d,f,14,4));
    assert(d.input.values.speed_dkph==0 && !(d.input.values.valid & VEHICLE_SOC_VALID));
    sl_decoder_init(&d,160);assert(!sl_parse_feedback(&d,sample,14,0xfffffff0u));
    sl_poll(&d,0x1e4);assert(d.input.online && d.input.values.odometer_mm==4166);
    sl_poll(&d,0x7c0);assert(!d.input.online && d.input.values.odometer_mm==8333);
    sl_poll(&d,0x900);assert(d.input.values.odometer_mm==8333);
    assert(sl_encode_command(&c,out,sizeof(out))==20 && out[1]==20 && !sl_xor(out,20));
    assert(out[7]==0 && out[8]==160 && out[14]==1 && out[15]==164);
    c.extended=1;c.assist_pwm=100;assert(sl_encode_command(&c,out,sizeof(out))==21 && out[19]==100 && !sl_xor(out,21));
    c.throttle_pwm=1001;assert(!sl_encode_command(&c,out,sizeof(out)));
    puts("Shenlong protocol: known vectors, every bit corruption, all splits, extension lengths, timeout, wrap and distance PASS");
    return 0;
}
