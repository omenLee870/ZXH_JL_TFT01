/* Exercise actual runtime resource ownership and wire generation with mocked HW. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "shenlong_protocol.h"
#include "app_vehicle.h"

#define SHENLONG_RUNTIME_TEST 1
#define TCFG_SHENLONG_UART_ENABLE 1
#define TCFG_SHENLONG_TX_IO 4
#define TCFG_SHENLONG_RX_IO 5
#define TCFG_SHENLONG_UART_ID 1
#define TCFG_SHENLONG_DIAMETER_TENTH_INCH 160
#define CFG_YYB_ODOMETER 58
#define CFG_YYB_ODOMETER_BACKUP 57
#define ZXH_OK 0
#define VM_ERR_NONE 0
#define VM_RAM_FLUSH_NONE 1
#define UART_PARITY_all_1 2
#define UART_EVENT_RX_TIMEOUT 4
#define UART_EVENT_RX_DATA 1
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
struct uart_config {u32 baud_rate;u16 tx_pin,rx_pin;int parity;u8 tx_wait_mutex;};
struct uart_dma_config {u32 rx_timeout_thresh,frame_size,rx_cbuffer_size;void *rx_cbuffer;u8 irq_priority;int event_mask;};
struct app_meter_state {int power_on,light_on,mode;};
static u32 mock_now;
static unsigned live_allocations,allocation_attempt,allocation_fail,fail_uart,fail_dma,fail_timer;
static unsigned open_count,timer_count,sent_count,tx_busy,meter_fail;
static int mock_tx_state;
static struct app_meter_state mock_meter={1,1,2};
static u8 incoming[64],sent[96],stored[2][64];
static struct vehicle_core mock_vehicle;
int app_vehicle_aux_publish(enum vehicle_aux_source source,uint32_t known,uint32_t status,int gear,uint32_t timeout)
{ return vehicle_core_aux_publish(&mock_vehicle,source,known,status,gear,mock_now,timeout); }
int app_vehicle_try_snapshot(struct vehicle_state *state)
{ *state=mock_vehicle.state;vehicle_core_aux_apply(&mock_vehicle,state,mock_now);return 0; }
static unsigned incoming_length,sent_length,stored_length[2];
static u32 sys_timer_get_ms(void) {return mock_now;}
static u16 sys_timer_add(void *priv,void (*cb)(void *),u32 period)
{ (void)priv;(void)cb;assert(period==10);if(fail_timer)return 0;timer_count++;return 1; }
static void sys_timer_del(u16 id) {assert(id==1 && timer_count==1);timer_count--;}
static void *dma_malloc(size_t n)
{ if(++allocation_attempt==allocation_fail)return NULL;void *p=malloc(n);assert(p);live_allocations++;return p; }
static void dma_free(void *p) {assert(p && live_allocations);live_allocations--;free(p);}
static int uart_init(int id,const struct uart_config *c)
{
    assert(id==1 && c->baud_rate==9600 && c->parity==UART_PARITY_all_1);
    assert(c->tx_pin==4 && c->rx_pin==5);
    if(fail_uart)return -1;
    assert(!open_count);open_count++;mock_tx_state=2;return id;
}
static int uart_dma_init(int id,const struct uart_dma_config *d)
{ assert(id==1 && d->rx_cbuffer && d->rx_cbuffer_size==256);return fail_dma?-1:0; }
static int uart_deinit(int id) {assert(id==1 && open_count==1);open_count--;return 0;}
static int uart_is_send_complete(int id) {assert(id==1);if(!open_count)return -3;return tx_busy?0:mock_tx_state;}
static int uart_send_bytes(int id,const void *p,u32 n)
{ assert(id==1 && (n==20 ? !sl_xor(p,n) : !memcmp(p,"+ZXH:",5)));assert(mock_tx_state>0);memcpy(sent,p,n);sent_length=n;sent_count++;mock_tx_state=1;return n; }
static int uart_recv_bytes(int id,void *p,u32 n)
{ assert(id==1 && n>=incoming_length);memcpy(p,incoming,incoming_length);n=incoming_length;incoming_length=0;return n; }
static int app_meter_control_snapshot(struct app_meter_state *m) {*m=mock_meter;return meter_fail?-1:0;}
static int syscfg_read(int id,void *p,unsigned n)
{ unsigned i=id==57;if(!stored_length[i])return -1;if(n>stored_length[i])n=stored_length[i];memcpy(p,stored[i],n);return n; }
static int syscfg_write(int id,const void *p,unsigned n)
{ unsigned i=id==57;assert(n<=64);memcpy(stored[i],p,n);stored_length[i]=n;return n; }
static int get_vm_ram_storage_enable(void) {return 0;}
static int get_vm_ram_storage_in_irq_enable(void) {return 0;}
static int vm_flush2flash(int n) {(void)n;return 0;}
#include "../../SDK/apps/watch/common/shenlong_uart.c"

static void assert_closed(void) {assert(!open_count && !timer_count && !live_allocations);}
int main(void)
{
    struct vehicle_input in;
    const u8 frame[]={2,14,1,0,0xa0,0x40,0x7b,61,0,153,80,0,45,0x4f};
    allocation_fail=1;assert(shenlong_uart_start()<0);assert_closed();
    allocation_fail=allocation_attempt+2;assert(shenlong_uart_start()<0);assert_closed();
    allocation_fail=0;fail_uart=1;assert(shenlong_uart_start()<0);assert_closed();fail_uart=0;
    fail_dma=1;assert(shenlong_uart_start()<0);assert_closed();fail_dma=0;
    fail_timer=1;assert(shenlong_uart_start()<0);assert_closed();fail_timer=0;
    assert(!shenlong_uart_start());assert(live_allocations==2 && timer_count==1);
    assert(!shenlong_uart_start());assert(live_allocations==2 && timer_count==1);
    assert(uart_is_send_complete(1)==2); /* Actual BR33 state before first TX. */
    mock_now=120;transport_poll(NULL);assert(sent_count==1 && sent_length==20);
    assert(sent[4]==15 && sent[5]==0xb0); /* SPORT, light, work, RX fault */
    mock_now=240;tx_busy=1;transport_poll(NULL);assert(sent_count==1);tx_busy=0;
    mock_tx_state=-3;transport_poll(NULL);assert(sent_count==1);mock_tx_state=1;
    meter_fail=1;transport_poll(NULL);assert(sent_count==1);meter_fail=0;
    memcpy(incoming,frame,sizeof(frame));incoming_length=sizeof(frame);
    mock_now=360;transport_poll(NULL);assert(sent_count==2 && sent[5]==0xa0);
    shenlong_uart_read(&in);assert(in.online && in.values.current_ma==12300 && in.values.speed_dkph==300);
    mock_now=860;shenlong_uart_read(&in);assert(in.values.odometer_mm==4166);
    assert(!shenlong_uart_reset_trip());shenlong_uart_read(&in);assert(!in.values.trip_mm);
    assert(!shenlong_uart_shutdown());assert_closed();shenlong_uart_read(&in);assert(!in.online);
    assert(in.values.odometer_mm==4166);assert(!shenlong_uart_shutdown());assert_closed();
    mock_now=1000;assert(!shenlong_uart_start());shenlong_uart_read(&in);
    assert(!in.online && in.values.odometer_mm==4166 && !in.values.trip_mm);
    assert(!shenlong_uart_shutdown());assert_closed();
    loaded=0;memset(&decoder,0,sizeof(decoder));assert(!shenlong_uart_start());
    shenlong_uart_read(&in);assert(in.values.odometer_mm==4166 && !in.values.trip_mm);
    assert(!shenlong_uart_shutdown());assert_closed();
    assert(!shenlong_uart_start());
    assert(uart_is_send_complete(1)==2); /* Reinitialization also needs first-TX handling. */
    mock_now+=120;transport_poll(NULL);
    assert(uart_is_send_complete(1)==1);
    vehicle_core_init(&mock_vehicle,mock_now);
    const char *request="AT+ZXHSET=1,15,2\r\n";
    memcpy(incoming,request,strlen(request));incoming_length=strlen(request);
    tx_busy=1;mock_now+=10;transport_poll(NULL);
    assert(vehicle_test_reply(&test_control));
    tx_busy=0;mock_now+=10;transport_poll(NULL);
    assert(!vehicle_test_reply(&test_control)&&sent_length>20&&!memcmp(sent,"+ZXH:1,OK,3,15,2,127,0",21));
    for (unsigned mode=0;mode<3;++mode) {
        char line[64];snprintf(line,sizeof(line),"AT+ZXHSET=%u,0,2,%u\r\n",20+mode,mode);
        memcpy(incoming,line,strlen(line));incoming_length=strlen(line);
        mock_now+=10;transport_poll(NULL);
        mock_now+=120;transport_poll(NULL);
        assert(sent_length==20&&sent[4]==(mode==0?5:mode==1?10:15));
    }
    mock_now+=1500;transport_poll(NULL);assert(sent_length==20&&sent[4]==15);
    assert(!shenlong_uart_shutdown());assert_closed();
    puts("Shenlong runtime: UART ownership, command mapping/ODO; AT ECO/CITY/SPORT -> returned assist and expiry fallback PASS");
    return 0;
}
