#ifndef __LP_MSG_H__
#define __LP_MSG_H__

//=================================消息格式========================================

//消息buf大小
#define MAX_POOL		    512

//消息类型
enum {
    MSG_ACK    = 0,
    MSG_TEST   = 1,
    MSG_COMMOM = 2,
    MSG_CTMU   = 3,
    MSG_SENSOR = 4,
    MSG_VAD    = 5,
    MSG_RTC    = 6,
    MSG_APP    = 7,
    MSG_CLOCK  = 8,
    MSG_BT_LINK  = 9,
    MSG_BT_SCH 	= 10,
    MSG_BT_MAIN	= 11,
    MSG_CMAC    = 12,
    MSG_BT_IRQ  = 13,
    MSG_POWER  = 14,
    MSG_BREDR_LOW_PWR  = 15,
    MSG_BREDR_LOW_PWR_IRQ  = 16,
    MSG_PMU_PMC = 17,
    MSG_GPADC   = 18,
    MSG_BATTERY = 19,
};

//消息函数返回值
enum {
    MSG_NO_ERROR = 0,			//读取/发送消息成功
    MSG_NO_MSG = -1,			//未读取到消息
    MSG_BUF_ERROR = -2,			//读/写消息格式不对
    MSG_BUF_READ_OVER = -3,		//读消息溢出，传的参数长度不对
    MSG_BUF_WRITE_OVER = -4,	//写消息会溢出
};

//消息头格式
#define MSG_HEADER_BYTE_LEN     4
#define MSG_HEADER_BIT_LEN     (MSG_HEADER_BYTE_LEN*8)
#define MSG_HEADER_ALL_BIT     ((1L<<MSG_HEADER_BIT_LEN) - 1)

#define MSG_INDEX_BIT			10
#define MSG_ACK_INDEX_BIT		5
#define MSG_ACK_BIT  			1

#define MSG_TYPE_BIT_LEN        8
#define MSG_PARAM_BIT_LEN       (MSG_HEADER_BYTE_LEN*8-MSG_TYPE_BIT_LEN-MSG_INDEX_BIT-MSG_ACK_INDEX_BIT-MSG_ACK_BIT)

struct lp_msg_head {
u32 type  :
    MSG_TYPE_BIT_LEN;
u32 ack   :
    MSG_ACK_BIT;
u32 index :
    MSG_INDEX_BIT;
u32 ack_index :
    MSG_ACK_INDEX_BIT;
u32 len   :
    MSG_PARAM_BIT_LEN;
} __attribute__((packed));

//消息队列
typedef struct LP_Q {
    u16         in;		  //写位置
    u16         out;	  //读位置
    u16         count;	  //有效数据
    u16         size;	  //buf大小
    u32         start;	  //buf起址
    u32			ack_w;
    u32			ack_r;
} LP_Q;

enum {
    LP_BUF_NO_ERR = 0,
    LP_BUF_READ_NOT_ENOUGH_DATA = -1,		//buf里数据不够读
    LP_BUF_READ_NO_DATA = -2,				//buf里面没有数据
    LP_BUF_WRITE_OVER = -3,					//写数据超过了buf大小
};

//用户消息对应处理
struct lp_msg_handler {
    void (*handler)(void *, u8 *, u32);
    void *priv;
    u8 type;
} __attribute__((packed));

void msys_to_p11_sync_cmd(u8 cmd);

void lp_ipc_init();

void lp_lock();

void lp_unlock();

void config_post_ack_flag(u32 enable);

void lp_ipc_p11_to_suspend();//p11 to sleep

//================================M2P========================================================

#define REGISTER_M2P_MSG_HANDLER(pri, _type, fn) \
	const struct lp_msg_handler _##fn SEC_USED(.m2p_msg_handler)= { \
		.handler = fn, \
		.priv = pri, \
		.type = _type, \
	}

extern struct lp_msg_handler m2p_msg_handler_begin[];
extern struct lp_msg_handler m2p_msg_handler_end[];

#define list_for_each_m2p_msg_handler(p) \
	for (p = m2p_msg_handler_begin; p < m2p_msg_handler_end; p++)

int m2p_get_msg(struct lp_msg_head *head, void *msg, u32 len);
int q32f_m2p_get_msg(struct lp_msg_head *head, void *msg,  u32 len);

int m2p_post_msg(u32 type, u32 ack, void *msg, u32 len);
int m2p_post_msg_q32f(u32 type, u32 ack, void *msg, u32 len);

int m2p_msg_hdl();
int q32f_m2p_msg_hdl(u32 index);

u32 pmc_ack_msg(u32 index);
u32 q32f_pmc_ack_msg(u32 index);

/*
 * function:主系统向P11发送消息，往消息池丢数据
 *
 * param name: null
 * param argc：参数个数
 */

int task_post_msg2p11(char *name, int argc, ...);

//================================P2M========================================================

#define REGISTER_P2M_MSG_HANDLER(pri, _type, fn) \
	const struct lp_msg_handler _##fn SEC_USED(.p2m_msg_handler)= { \
		.handler = fn, \
		.priv = pri,\
		.type = _type, \
	}

extern struct lp_msg_handler p2m_msg_handler_begin[];
extern struct lp_msg_handler p2m_msg_handler_end[];

#define list_for_each_p2m_msg_handler(p) \
	for (p = p2m_msg_handler_begin; p < p2m_msg_handler_end; p++)

int p2m_get_msg(struct lp_msg_head *head, void *msg, u32 len);
int p2m_post_msg(u32 type, u32 ack, void *msg, u32 len);

int msys_msg_hdl();

u32 msys_ack_msg(u32 index);

#endif
