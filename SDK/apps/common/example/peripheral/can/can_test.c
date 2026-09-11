#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/can.h"
/* #include "asm/crc16.h" */
#include "irq.h"

#if	USE_CAN_TEST_DEMO || TCFG_DASHBOARD_HARDWARE_TEST_ENABLE

/* #define CAN_REINIT_MODIFY_PARAMETERS    ///< 演示demo-代码运行后修改can的参数 */
/* #define CAN_RECV_BLOCK_ENABLE           ///< 演示can阻塞式接收 */

#define CAN_RX_CNT	5

static u32 *data_priv = NULL;
static int *rxcnt_priv = NULL;  //当前can已接收数据计数，内部指针，实时更新
static void *can_hdl = NULL;

extern const u16 config_can_dma_ram_malloc_size;

//非阻塞式中断接收，中断回调仅传递底层存储数据的ram-buffer地址，数据处理由应用层can_rx_task进行
static int can_irq_cb(void *priv, void *hdl, can_event_isr_t event)
{
    switch (event) {
    case CAN_EVENT_RECEIVE_INTERRUPT:
        can_addr_t *adr_hdl = (can_addr_t *)hdl;
        if (!adr_hdl) {
            //hdl 为NULL则说明底层收数已经回滚，注意此时应用层是否有及时取数，否则会出现回滚覆盖的问题
            return 0;
        }
        if (!rxcnt_priv && !data_priv) {
            rxcnt_priv = adr_hdl->rxcnt_addr;
            data_priv = adr_hdl->rxdata_addr;
        }
        break;
    default:
        printf("%s: 0x%x", __func__, event);
        break;
    }
    return 0;
}

/* #ifndef  CAN_RECV_BLOCK_ENABLE */
void __can_receive_data_operation(u32 *p_rx_packet, can_data_t *can_rxdata);
static void can_rx_task(void *arg)
{
    can_data_t can_rxbuf = {0};
    int res = 0;
    int new_rxcnt = 0;      //当前can已接收数据计数，实时更新
    int old_rxcnt = 0;     //应用层已处理的can数据计数，应用层管理。
    int diff_rxcnt = 0;      //cur与lost的差值，应用层管理。
    u16 i = 0;
    while (1) {
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            rxcnt_priv = NULL;
            data_priv = NULL;
            os_task_del_res(OS_TASK_SELF);
            return;
        }
        udelay(100);
        if (rxcnt_priv && data_priv) {
            new_rxcnt = *rxcnt_priv;
            if (new_rxcnt > old_rxcnt) {
                diff_rxcnt = new_rxcnt - old_rxcnt;
                for (i = 0; i < diff_rxcnt; i++) {
                    __can_receive_data_operation(&data_priv[4 * (old_rxcnt + i)], &can_rxbuf);
                    /* g_printf("%d: new_rxcnt = %d; old_rxcnt = %d; i = %d", __LINE__, new_rxcnt, old_rxcnt, i); */
                    printf("id is -- %d  0x%x", can_rxbuf.id, can_rxbuf.id);
                    /* printf("dlc is -- %d", can_rxbuf.dlc); */
                    /* printf("rx_data: "); */
                    /* put_buf(can_rxbuf.data, can_rxbuf.dlc); */
                }
                old_rxcnt = new_rxcnt;
            } else if (old_rxcnt > new_rxcnt) {    //上一次接收帧数大于当前帧数，帧数回滚
                diff_rxcnt = config_can_dma_ram_malloc_size - old_rxcnt;       //dma l2级缓存固定帧数
                for (i = 0; i < diff_rxcnt; i++) {
                    __can_receive_data_operation(&data_priv[4 * (old_rxcnt + i)], &can_rxbuf);
                    /* g_printf("%d: new_rxcnt = %d; old_rxcnt = %d; i = %d", __LINE__, new_rxcnt, old_rxcnt, i); */
                    printf("id is -- %d  0x%x", can_rxbuf.id, can_rxbuf.id);
                    /* printf("dlc is -- %d", can_rxbuf.dlc); */
                    /* printf("rx_data: "); */
                    /* put_buf(can_rxbuf.data, can_rxbuf.dlc); */
                }
                old_rxcnt = 0;
                diff_rxcnt = new_rxcnt;
                for (i = 0; i < diff_rxcnt; i++) {
                    __can_receive_data_operation(&data_priv[4 * (old_rxcnt + i)], &can_rxbuf);
                    /* g_printf("%d: new_rxcnt = %d; old_rxcnt = %d; i = %d", __LINE__, new_rxcnt, old_rxcnt, i); */
                    printf("id is -- %d  0x%x", can_rxbuf.id, can_rxbuf.id);
                    /* printf("dlc is -- %d", can_rxbuf.dlc); */
                    /* printf("rx_data: "); */
                    /* put_buf(can_rxbuf.data, can_rxbuf.dlc); */
                }
                old_rxcnt = new_rxcnt;
            } else {
                //二者相等则说明此时dma无数据，不处理
                mdelay(10);
            }
        } else {
            mdelay(10);
        }
    }
}
/* #endif */

u16 CRC16(const void *ptr, u32 len);
static void can_test_task(void *arg)
{
#ifdef CAN_REINIT_MODIFY_PARAMETERS
    can_mode_t		can_mode;
    can_baudrate_t  reset_rate;
    can_rx_filter_t	reset_filter;
#endif
    can_data_t can_rx_data[CAN_RX_CNT];
    can_data_t can_tx_data[4];
    memset(&can_tx_data, 0x0, sizeof(can_tx_data));
    memset(&can_rx_data, 0x0, sizeof(can_rx_data));
    can_hdl = dev_open("can", NULL);
    if (!can_hdl) {
        r_printf("can open err");
        goto exit;
    }
#ifdef CAN_RECV_BLOCK_ENABLE
    /* dev_ioctl(can_hdl, IOCTL_CAN_SET_RECV_WAIT_WHILE, 0); */
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RECV_WAIT_SEM, 0);
#else
    can_cb_t cb;
    cb.cb_func = can_irq_cb;
    cb.cb_priv = NULL;
    dev_ioctl(can_hdl, IOCTL_CAN_SET_IRQ_CB, (u32)&cb);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RECV_NON_BLOCK_ENABLE, 0);
#endif
    //初始化完成，复位CAN，确保原子性
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RESTART, 0);

#if (TCFG_CAN_MODE == 0) //basic can
    can_tx_data[0].id   = 0x233;
    can_tx_data[0].dlc  = 8;
    can_tx_data[0].rtr  = CAN_REQUEST_DATA_TYPE;//数据帧
    // 发送数据
    can_tx_data[0].data[0]  = 0x11;
    can_tx_data[0].data[1]  = 0x11;
    can_tx_data[0].data[2]  = 0x33;
    can_tx_data[0].data[3]  = 0x33;
    can_tx_data[0].data[4]  = 0x55;
    can_tx_data[0].data[5]  = 0x55;
    can_tx_data[0].data[6]  = 0x77;
    can_tx_data[0].data[7]  = 0x77;

    // 数据帧发送
    can_tx_data[1].id  = 0x159;
    can_tx_data[1].dlc  = 8;
    can_tx_data[1].rtr  = CAN_REQUEST_DATA_TYPE;//数据帧
    // 发送数据
    can_tx_data[1].data[0]  = 0x21;
    can_tx_data[1].data[1]  = 0x31;
    can_tx_data[1].data[2]  = 0x43;
    can_tx_data[1].data[3]  = 0x53;
    can_tx_data[1].data[4]  = 0x65;
    can_tx_data[1].data[5]  = 0x75;
    can_tx_data[1].data[6]  = 0x87;
    can_tx_data[1].data[7]  = 0x97;

    // 远程帧发送
    can_tx_data[2].id  = 0x159;
    can_tx_data[2].dlc  = 8;
    can_tx_data[2].rtr  = CAN_REQUEST_REMOTE_TYPE;//远程帧

    dev_write(can_hdl, &can_tx_data, 3);

#endif

#if (TCFG_CAN_MODE == 1) //enhanced can
    // 测试发送标准数据帧
    can_tx_data[0].dlc 		 	= 8;
    can_tx_data[0].rtr 		 	= CAN_REQUEST_DATA_TYPE;
    can_tx_data[0].data_format  = CAN_FRAME_STANDARD_FORMAT;
    can_tx_data[0].id  		 	= 0x457;

    can_tx_data[0].data[0]  =  0x18;
    can_tx_data[0].data[1]  =  0x27;
    can_tx_data[0].data[2]  =  0x36;
    can_tx_data[0].data[3]  =  0x45;
    can_tx_data[0].data[4]  =  0x54;
    can_tx_data[0].data[5]  =  0x63;
    can_tx_data[0].data[6]  =  0x72;
    can_tx_data[0].data[7]  =  0x81;

    // 测试发送标准远程帧
    can_tx_data[1].dlc			 = 2;
    can_tx_data[1].rtr			 = CAN_REQUEST_REMOTE_TYPE;
    can_tx_data[1].data_format   = CAN_FRAME_STANDARD_FORMAT;
    can_tx_data[1].id			 = 0x456;

    // 测试发送扩展数据帧
    can_tx_data[2].dlc			 = 8;
    can_tx_data[2].rtr			 = CAN_REQUEST_DATA_TYPE;
    can_tx_data[2].data_format   = CAN_FRAME_EXTENDED_FORMAT;
    can_tx_data[2].id			 = 0x123456;

    can_tx_data[2].data[0]  =  0x33;
    can_tx_data[2].data[1]  =  0x33;
    can_tx_data[2].data[2]  =  0x33;
    can_tx_data[2].data[3]  =  0x33;
    can_tx_data[2].data[4]  =  0x33;
    can_tx_data[2].data[5]  =  0x33;
    can_tx_data[2].data[6]  =  0x33;
    can_tx_data[2].data[7]  =  0x33;
    can_tx_data[2].crc_en   =  1;
    can_tx_data[2].crc      =  CRC16(&can_tx_data[2], sizeof(can_data_t) - 2);

    // 测试发送扩展远程帧
    can_tx_data[3].dlc			 = 8;
    can_tx_data[3].rtr			 = CAN_REQUEST_REMOTE_TYPE;
    can_tx_data[3].rtr			 = CAN_REQUEST_DATA_TYPE;
    can_tx_data[3].data_format   = CAN_FRAME_EXTENDED_FORMAT;
    can_tx_data[3].id			 = 0x423457;

    dev_ioctl(can_hdl, IOCTL_CAN_GET_PUT_REGISTER_INFO, 0);
    dev_write(can_hdl, &can_tx_data, 4);
#endif

    while (1) {
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            os_task_del_res(OS_TASK_SELF);
            return;
        }
#ifdef CAN_RECV_BLOCK_ENABLE
        os_time_dly(1);
        ///接收数据
        printf("\n\n--------can run---------\n\n");
        dev_read(can_hdl, &can_rx_data, CAN_RX_CNT);

        for (int cnt = 0; cnt < CAN_RX_CNT; cnt++) {
            printf("The CAN RX data-[%d]", cnt);
            printf("data_format is [%d]-%d", cnt, can_rx_data[cnt].data_format);
            printf("rtr is [%d]-%d", cnt, can_rx_data[cnt].rtr);
            printf("id is [%d]-0x%x; RMC = %d", cnt, can_rx_data[cnt].id, JL_ECAN->RMC);
            printf("dlc is [%d]-%d; data: ", cnt, can_rx_data[cnt].dlc);
            put_buf(can_rx_data[cnt].data, can_rx_data[cnt].dlc);
        }
#else
        os_time_dly(100);
        printf("waiting recv...");
        wdt_clear();
        dev_write(can_hdl, &can_tx_data[2], 2);
#endif

#ifdef CAN_REINIT_MODIFY_PARAMETERS
        break;
#endif
    }

#ifdef CAN_REINIT_MODIFY_PARAMETERS
    can_mode = CAN_MODE_BASICCAN;

    reset_rate.brp 			= 39;
    reset_rate.sjw 			= TCFG_CAN_BAUD_SJW;
    reset_rate.sam 			= TCFG_CAN_BAUD_SAM;
    reset_rate.tseg1 		= TCFG_CAN_BAUD_PTS_PBS1;
    reset_rate.tseg2 		= TCFG_CAN_BAUD_PBS2;

    reset_filter.accept_id0 = 0x123;
    reset_filter.mask_id0	= 0x00;
    // 演示修改can参数-波特率从1Mhz修改为100khz；模式改为基础can；滤波器修改
    dev_ioctl(can_hdl, IOCTL_CAN_SET_MODE, (u32)&can_mode);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_FILTER, (u32)&reset_filter);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_RATE, (u32)&reset_rate);
    while (1) {

#ifdef CAN_RECV_BLOCK_ENABLE
        os_time_dly(1);
        // 接收数据
        printf("\n\n--------modify_parameters_can run---------\n\n");
        dev_read(can_hdl, &can_rx_data, CAN_RX_CNT);
        for (int cnt = 0; cnt < CAN_RX_CNT; cnt++) {
            printf("The CAN RX data-[%d]", cnt);
            printf("data_format is [%d]-%d", cnt, can_rx_data[cnt].data_format);
            printf("rtr is [%d]-%d", cnt, can_rx_data[cnt].rtr);
            printf("id is [%d]-0x%x", cnt, can_rx_data[cnt].id);
            printf("dlc is [%d]-%d", cnt, can_rx_data[cnt].dlc);
            for (int i = 0; i < can_rx_data[cnt].dlc; i++) {
                printf("[%d]-0x%x", cnt, can_rx_data[cnt].data[i]);
            }
        }
#else
        os_time_dly(100);
        printf("waiting recv...");
        dev_write(can_hdl, &can_tx_data[1], 2);
#endif
    }
#endif
exit:
    if (can_hdl) {
        dev_close(can_hdl);
    }
    while (1) {
        os_time_dly(100);
    }
}

__attribute__((interrupt("")))
static void can_isr()
{
    u32 arg;
    r_printf(">>>%s:  can interrupt occure.", __func__);
    dev_ioctl(can_hdl, IOCTL_CAN_GET_INTERRUPT_SOURCE, (u32)&arg);
    dev_ioctl(can_hdl, IOCTL_CAN_SET_INTERRUPT_DISABLE, CAN_EVENT_CLEAR_INTERRUPT);
    /* dev_ioctl(can_hdl, IOCTL_CAN_SET_INTERRUPT_DISABLE, CAN_EVENT_ALL_INTERRUPT_DISABLE); */
    printf("arg = %x", arg);
}

int c_main_can(void)
{
    /* request_irq(IRQ_CAN_IDX, 5, can_isr, 1); */
    printf("\n\n-----------------CAN_TEST_DEMO run %s---------------- \n\n", __TIME__);
    os_task_create(can_rx_task, NULL, 18, 1024, 0, "can_rx_task");
    //CAN的主线程，即所有访问底层的操作必须和CAN的中断绑定在同一个核，具体可查看板级c文件中can初始化结构体的cpu_id赋值
    os_task_create_affinity_core(can_test_task, NULL, 20, 1024, 0, "can_test_task", 0);
    return 0;
}

/* 删除 demo 任务(任务在 del_req 处协作退出) */
int can_test_stop(void)
{
    os_task_del("can_rx_task");
    os_task_del("can_test_task");
    return 0;
}

#endif // USE_CAN_TEST_DEMO

