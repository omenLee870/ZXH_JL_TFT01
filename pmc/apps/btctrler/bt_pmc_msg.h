#ifndef __BT_PMC_MSG_H__
#define __BT_PMC_MSG_H__


//bt main 消息类型
enum bt_main_msg {
    MSG_CMAC_INIT   = 0,
    MSG_BT_INIT    	,
    MSG_BT_TEST    	,
    MSG_BREDR_IRQ,
    MSG_CMAC_IRQ,
    MSG_CMAC_LBUF,
    MSG_AES128,
    MSG_CCM_V3,
    MSG_SET_RXTX_GPIO,
};

struct pmc_aes_msg_t {
    u32 msg;
    u8 *param;
};

//rf RX/TX 状态指示 GPIO 设置参数
struct rxtx_gpio_param_t {
    u32 rx_io;   //RX 状态指示 GPIO
    u32 tx_io;   //TX 状态指示 GPIO
};

struct aes128_param_t {
    u8 *key;
    u8 *plaintext;
    u8 *encrypt;
};

struct ccm_v3_param_t {
    u8 *key;
    u8 *nonce;
    u16 pkt_header;
    u8 *header;
    u8 headerlen;
    u8 *pt;
    u8 ptlen;
    u8 *ct;
    u8 *tag;
    u8 taglen;
    u8 direction;
};


#endif

