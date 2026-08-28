#include "includes.h"

#pragma const_seg(".msg.const.cache.L1")
#pragma code_seg(".msg.text.cache.L1")
#pragma str_literal_override(".msg.const.cache.L1")


/*
	BR33新增q32f消息处理

1. MPC低功耗调度由q32s完成，查询q32f状态，挂起q32f
2. q32f不能直接唤醒主系统，起软中断由q32f完成

 */


/*
1.共享buf互斥操作,注意P11在等待主系统BUF锁时候需要响应主系统的操作，
防止主系统等待导致死锁,处理来自主系统的M2P中断（通过硬件lock不需要这一点）

2.接口互斥，不能同时操作
P11在等待主系统ack消息时，也要防止死锁(P11和主系统同时发送ACK消息时会死锁)，
有以下处理：

主系统
	1.主系统不能在关中断/中断中发送ACK类型的消息
	2.发送ACK类型的消息需要互斥，即不允许同时发多个ACK类型消息
	3.不允许在收到ACK类型消息处理函数中发送ACK类型消息的嵌套，收到ACK类型消息
调用post_ack_flag=0，发送函数判断post_ack_flag=0且当前任务为pmu_task则断言

P11系统：
	1.P11系统不能在关中断/中断中发送ACK类型的消息
	2.不允许在收到ACK类型消息后，又发送ACK类型消息，由于不会在中断中发送ACK类型消息，
在收到ACK类型的消息后调用post_ack_flag=0，发送函数内部检查post_ack_flag=0则断言
	3.不允许重复发送ACK类型的消息，由于一点的保证
	4.主系统和P11同时发ACK类型的消息，P11需要响应主系统
 */

#define MSG_ENTER_CRITICAL()	lp_lock()
#define MSG_EXIT_CRITICAL()		lp_unlock()

#define Q32F_WKUP_MSYS(msg)		PMC_SYSTEM->P2M_INT_SET = BIT(P2M_APP_INDEX)
#define MSYS_WKUP_Q32S()		PMC_SYSTEM->M2P_INT_SET = BIT(M2P_APP_INDEX)
#define MSYS_WKUP_Q32F(msg)		PMC_SYSTEM->M2P_INT_SET = BIT(M2P_F_APP_INDEX)
static bool post_ack_flag = 1;

#ifdef P11_RAM_BASE
//br33主系统和PMC偏移地址是一样的
#undef P11_RAM_BASE
#define P11_RAM_BASE 0
#endif

/*
function：  从缓存buf读取指定长度数据
return val: -2,没有数据 -1,数据不足：0,读取到数据
param len： 读取数据长度
 */
static int lp_buf_read(LP_Q *q, u8 *buf, u32 len)
{
    if (q->count == 0) {
        return LP_BUF_READ_NO_DATA;
    }

    if (q->count < len) {
        return LP_BUF_READ_NOT_ENOUGH_DATA;
    }

    u32 end = (q->out + len) % q->size;
    if (end > q->out) {
        memcpy(buf, (void *)(P11_RAM_BASE + q->start + q->out), len);
    } else {
        memcpy(buf, (void *)(P11_RAM_BASE + q->start + q->out), q->size - q->out);
        memcpy(buf + (q->size - q->out), (void *)(P11_RAM_BASE + q->start), end);
    }

    q->out = end;
    q->count -= len;

    return LP_BUF_NO_ERR;
}

static int lp_buf_write_enable(LP_Q *q, u32 len)
{
    if ((q->count + len) > q->size) {
        return LP_BUF_WRITE_OVER;
    } else {
        return LP_BUF_NO_ERR;
    }
}

/*
function：  从缓存buf写入指定长度数据
return val: 0,写入成功
param len： 写入的数据长度
 */
static int lp_buf_write(LP_Q *q, u8 *buf, u32 len)
{
    u32 end = (q->in + len) % q->size;
    if (end > q->in) {
        memcpy((void *)(P11_RAM_BASE + q->start + q->in), buf, len);
    } else {
        memcpy((void *)(P11_RAM_BASE + q->start + q->in), buf, q->size - q->in);
        memcpy((void *)(P11_RAM_BASE + q->start), buf + (q->size - q->in), end);
    }

    q->in = end;
    q->count += len;

    return LP_BUF_NO_ERR;
}

static int find_first_zero_bit_optimized(uint32_t num)
{
    // 对数字取反，然后查找第一个为1的位
    uint32_t inverted = ~num;

    // 如果取反后为0，说明原数所有位都是1
    if (inverted == 0) {
        return -1;
    }

    // 使用位运算技巧找到最低位的1
    // inverted & -inverted 会得到只包含最低位1的数
    int position = 0;
    while ((inverted & 1) == 0) {
        inverted >>= 1;
        position++;
    }

    return position;
}

/*
 * function：从消息池中获取主系统发送过来的消息
 * param head：消息的头部，包含消息类型、长度等信息
 * param len：读取消息内存的长度
 * param msg：读取到消息
 *
 * return val: MSG_NO_MSG
 *			   MSG_NO_ERROR
 * 			   MSG_BUF_ERROR
 * 			   MSG_BUF_READ_OVER
 *
 */
int q32f_m2p_get_msg(struct lp_msg_head *head, void *msg,  u32 len)
{
    LP_Q *msg_q_p = (LP_Q *)(P11_RAM_BASE + (Q32F_MSG_Q_ADDR0 | Q32F_MSG_Q_ADDR1 << 8 | Q32F_MSG_Q_ADDR2 << 16 | Q32F_MSG_Q_ADDR3 << 24));

    //长度至少是头+1byte数据
    if (len < sizeof(struct lp_msg_head) + 1) {
        return MSG_BUF_READ_OVER;
    }

    MSG_ENTER_CRITICAL();

    //先读取头部数据
    int ret = lp_buf_read(msg_q_p, (u8 *)head, sizeof(struct lp_msg_head));
    if (ret == LP_BUF_READ_NO_DATA) {

        //确保每次事件都能唤醒p11
        local_irq_disable();
        //若没有数据则进入低功耗
        MSG_EXIT_CRITICAL();

        //确保每次事件都能唤醒p11
        //get no msg,cpu enter lowpower
        local_irq_enable();

        return MSG_NO_MSG;

    } else if (ret == LP_BUF_READ_NOT_ENOUGH_DATA) {
        MSG_EXIT_CRITICAL();

        //读取头部的数据长度不够返回错误
        return MSG_BUF_ERROR;

    } else {

        //判断长度够不够
        if (len < head->len) {
            MSG_EXIT_CRITICAL();

            //读了头，数据读不了返回错误
            return MSG_BUF_READ_OVER;
        }

        ret = lp_buf_read(msg_q_p, msg, head->len);
        if (ret != LP_BUF_NO_ERR) {
            MSG_EXIT_CRITICAL();

            //数据长度为控不对返回格式错误
            return MSG_BUF_ERROR;
        }
    }

    MSG_EXIT_CRITICAL();

    return MSG_NO_ERROR;
}

/*
 * 接收到ack消息处理过程中调用
 */
void config_post_ack_flag(u32 enable)
{
    local_irq_disable();
    if (enable) {
        post_ack_flag = 1;
    } else {
        post_ack_flag = 0;
    }
    local_irq_enable();
}

static u32 get_ack_flag(LP_Q *msg_q_p, u32 index)
{
    MSG_ENTER_CRITICAL();
    u32 ret = msg_q_p->ack_r & BIT(index);
    MSG_EXIT_CRITICAL();

    return ret ? 1 : 0;
}

u32 q32f_pmc_ack_msg(u32 index)
{
    LP_Q *msg_q_p = (LP_Q *)(P11_RAM_BASE + (Q32F_MSG_Q_ADDR0 | Q32F_MSG_Q_ADDR1 << 8 | Q32F_MSG_Q_ADDR2 << 16 | Q32F_MSG_Q_ADDR3 << 24));
    MSG_ENTER_CRITICAL();
    msg_q_p->ack_r |= BIT(index);
    MSG_EXIT_CRITICAL();

    return 0;
}

/*
 * function:该函数可实现向Q32F主循环推数据
 *
 * param type: 消息类型，收到消息后根据消息类型匹配回调函数
 * param ack：该函数不需要ack参数，也即发送的头index为0，预留给该函数使用
 * param len：发送消息的长度，单位为一个字节
 * param msg：发送的消息
 *
 */
int m2p_post_msg_q32f(u32 type, u32 ack, void *msg, u32 len)
{
    struct lp_msg_head head;
    LP_Q *msg_q_p = (LP_Q *)(P11_RAM_BASE + (Q32F_MSG_Q_ADDR0 | Q32F_MSG_Q_ADDR1 << 8 | Q32F_MSG_Q_ADDR2 << 16 | Q32F_MSG_Q_ADDR3 << 24));

    MSG_ENTER_CRITICAL();

    /*P11的m2p接口不需要ack与index*/
    head.type = type;
    head.ack = 0;
    head.len = len;
    head.index = 0;

    if (lp_buf_write_enable(msg_q_p, len + sizeof(struct lp_msg_head)) == LP_BUF_WRITE_OVER) {
        MSG_EXIT_CRITICAL();
        return MSG_BUF_WRITE_OVER;
    }

    //写头
    lp_buf_write(msg_q_p, (u8 *)&head, sizeof(struct lp_msg_head));

    //写数据
    lp_buf_write(msg_q_p, msg, len);

    MSYS_WKUP_Q32F();

    MSG_EXIT_CRITICAL();

    return MSG_NO_ERROR;
}

/*
 * function:向主系统发送消息
 *
 * param type: 消息类型，收到消息后根据消息类型匹配回调函数
 * param ack：收到消息执行完成后是否发送应答消息，此时系统死等消息回应
 * 并处理发过来的所有消息
 * param len：发送消息的长度，单位为一个字节
 * param msg：发送的消息
 *
 * 注：ack=1时，因为是单核且不允许中断调用，所以等待ack时不需要进入临界
 */
int p2m_post_msg(u32 type, u32 ack, void *msg, u32 len)
{
    struct lp_msg_head head;
    LP_Q *msg_q_p = (LP_Q *)(P11_RAM_BASE + (MSYS_MSG_Q_ADDR0 | MSYS_MSG_Q_ADDR1 << 8 | MSYS_MSG_Q_ADDR2 << 16 | MSYS_MSG_Q_ADDR3 << 24));

    /*不允许在中断函数/关中断发阻塞操作，因为阻塞的同时要处理
     主系统发的消息，不建议在中断函数处理/关闭中断处理*/
    if (ack == 1) {
        //ASSERT(!(cpu_in_irq() || cpu_irq_disabled()), "p2m_post_msg ack");
        ASSERT(0, "dont support ack msg to msys!!!");
    }

    MSG_ENTER_CRITICAL();

    u32 index = 0;
    int ack_index = 0;
    if (ack) {
        //ack序号，保证可以同事发多个ack消息
        ack_index = find_first_zero_bit_optimized(msg_q_p->ack_w);
        ASSERT(ack_index >= 0 && ack_index < 32);
        head.ack_index = ack_index;

        msg_q_p->ack_w |= BIT(ack_index);
        /*收到ACK消息处理函数，不允许发送ACK消息*/
        if ((post_ack_flag != 1) && (!cpu_in_irq())) {
            ASSERT(0, "post_ack_flag check fail!");
        }
    }

    head.type = type;
    head.ack = ack;
    head.len = len;
    head.index = index;

    if (lp_buf_write_enable(msg_q_p, len + sizeof(struct lp_msg_head)) == LP_BUF_WRITE_OVER) {
        MSG_EXIT_CRITICAL();
        return MSG_BUF_WRITE_OVER;
    }

    //写头
    lp_buf_write(msg_q_p, (u8 *)&head, sizeof(struct lp_msg_head));

    //写数据
    lp_buf_write(msg_q_p, msg, len);

    /* __power_recover(); */

    Q32F_WKUP_MSYS();

    MSG_EXIT_CRITICAL();

#if 0
    if (ack) {
        while (1) {
            if (get_ack_flag(msg_q_p, ack_index)) {
                MSG_ENTER_CRITICAL();
                msg_q_p->ack_w &= ~BIT(ack_index);
                msg_q_p->ack_r &= ~BIT(ack_index);
                MSG_EXIT_CRITICAL();
                break;
            }

            /*主系统可能在等待P11的消息，处理来自主系统的消息，防止死锁*/
            if (q32f_m2p_msg_hdl(head.index)) {
                break;
            }
        }
    }
#endif

    return MSG_NO_ERROR;
}

/*
 * function:向主PMC Q32F系统发送消息，往消息池丢数据
 *
 * param type: 消息类型，主系统收到消息后根据消息类型匹配回调函数
 * param ack：P11收到消息执行完成后是否发送应答消息给主系统，P11死等消息
 * 并处理主系统发送过来的所有消息
 * param len：发送消息的长度，单位为一个字节
 * param msg：发送的消息
 *
 */

int m2p_post_msg(u32 type, u32 ack, void *msg, u32 len)
{
    struct lp_msg_head head;
    LP_Q *msg_q_p = (LP_Q *)(P11_RAM_BASE + (Q32S_MSG_Q_ADDR0 | Q32S_MSG_Q_ADDR1 << 8 | Q32S_MSG_Q_ADDR2 << 16 | Q32S_MSG_Q_ADDR3 << 24));

    /*不允许在中断函数/关中断发阻塞操作，因为阻塞的同时要处理
     主系统发的消息，不建议在中断函数处理/关闭中断处理*/
    if (ack == 1) {
        //ASSERT(!(cpu_in_irq() || cpu_irq_disabled()), "p2m_post_msg ack");
    }

    MSG_ENTER_CRITICAL();

    u32 index = 0;
    int ack_index = 0;
    if (ack) {
        //ack序号，保证可以同事发多个ack消息
        ack_index = find_first_zero_bit_optimized(msg_q_p->ack_w);
        ASSERT(ack_index >= 0 && ack_index < 32);
        head.ack_index = ack_index;

        msg_q_p->ack_w |= BIT(ack_index);
        /*收到ACK消息处理函数，不允许发送ACK消息*/
        if ((post_ack_flag != 1) && (!cpu_in_irq())) {
            /* ASSERT(0, "post_ack_flag check fail!"); */ //x先注释掉,因为ack按照这样发送会断言:主系统->q32f->q32s
        }
    }

    head.type = type;
    head.ack = ack;
    head.len = len;
    head.index = index;

    if (lp_buf_write_enable(msg_q_p, len + sizeof(struct lp_msg_head)) == LP_BUF_WRITE_OVER) {
        MSG_EXIT_CRITICAL();
        return MSG_BUF_WRITE_OVER;
    }

    //写头
    lp_buf_write(msg_q_p, (u8 *)&head, sizeof(struct lp_msg_head));

    //写数据
    lp_buf_write(msg_q_p, msg, len);

    MSYS_WKUP_Q32S();

    MSG_EXIT_CRITICAL();

    if (ack) {
        while (1) {
            if (get_ack_flag(msg_q_p, ack_index)) {
                MSG_ENTER_CRITICAL();
                msg_q_p->ack_w &= ~BIT(ack_index);
                msg_q_p->ack_r &= ~BIT(ack_index);
                MSG_EXIT_CRITICAL();
                break;
            }

            /*主系统可能在等待P11的消息，处理来自主系统的消息，防止死锁*/
            //if (q32f_m2p_msg_hdl(head.index)) {
            //    break;
            //}
        }
    }

    return MSG_NO_ERROR;
}

