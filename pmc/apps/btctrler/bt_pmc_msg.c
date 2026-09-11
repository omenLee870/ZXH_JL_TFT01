#include "includes.h"
#include "bt_pmc_msg.h"
#include "gpio.h"


#define LOG_TAG_CONST       BT
#define LOG_TAG             "[bt pmc]"
#include "generic/log.h"

int aes128_start_enc(unsigned char key[16], unsigned char plaintext[16], unsigned char encrypt[16]);

void ccm_memory_v3(const unsigned char *key, const unsigned char *nonce,
                   u16 pkt_header,
                   const unsigned char *header, unsigned long headerlen,
                   unsigned char *pt,  unsigned char ptlen,
                   unsigned char *ct,	unsigned char *tag, unsigned char taglen, unsigned char direction);

u8 cmac_lbuf_pool[8 * 1024] ;

void p11_init_app_cmac_lbuf()
{
    u32 ptr = (u32)&cmac_lbuf_pool[0];
    u32 size = sizeof(cmac_lbuf_pool);
    u8 msg[9];

    msg[0] = MSG_CMAC_LBUF;
    memcpy(msg + 1, (u8 *)&ptr, 4);
    memcpy(msg + 5, (u8 *)&size, 4);

    log_info("cmac_lbuf %x %d", ptr, size);

    p2m_post_msg(MSG_BT_MAIN, 0, (u8 *)msg, sizeof(msg));
}


static void bt_aes128(u8 *__msg, u32 len)
{
    struct pmc_aes_msg_t *msg = (struct pmc_aes_msg_t *)__msg;
    u8 way  = (msg->msg >> 8) & 0xff;

    struct aes128_param_t *aes128 = (struct aes128_param_t *)msg->param;

    aes128_start_enc(aes128->key, aes128->plaintext, aes128->encrypt);

    if (way) {
        log_info("F_M");
        M2P_AES128  = 1;
    } else {
        log_info("F_S");
        P2P_AES128  = 1;
    }
}

static void bt_ccm_v3(u8 *__msg, u32 len)
{
    struct pmc_aes_msg_t *msg = (struct pmc_aes_msg_t *)__msg;
    u8 way  = (msg->msg >> 8) & 0xff;

    struct ccm_v3_param_t *ccm = (struct ccm_v3_param_t *)msg->param;

    ccm_memory_v3(ccm->key, ccm->nonce, ccm->pkt_header, ccm->header, ccm->headerlen,
                  ccm->pt, ccm->ptlen, ccm->ct, ccm->tag, ccm->taglen, ccm->direction);

    if (way) {
        log_info("F_M");
        M2P_CCM_V3  = 1;
    } else {
        log_info("F_S");
        P2P_CCM_V3  = 1;
    }
}

static void bt_rxtx_gpio(u8 *__msg, u32 len)
{
    struct pmc_aes_msg_t *msg = (struct pmc_aes_msg_t *)__msg;
    struct rxtx_gpio_param_t *rxtx = (struct rxtx_gpio_param_t *)msg->param;

    printf("MSG_SET_RXTX_GPIO rx:%x tx:%x", rxtx->rx_io, rxtx->tx_io);

    /* 先配置成普通输出,再切换到 OCH0/OCH1 功能输出(RF RX/TX 状态) */
    gpio_set_direction(rxtx->rx_io, 0);
    gpio_set_output_value(rxtx->rx_io, 1);
    gpio_set_direction(rxtx->tx_io, 0);
    gpio_set_output_value(rxtx->tx_io, 1);

    /* OCH0/OCH1 信号源选择: 16/17 为 WL 的 RX/TX 状态输出 */
    SFR(PMC_IOMC->OCH_IOMC0, 0, 8, 16);
    SFR(PMC_IOMC->OCH_IOMC0, 8, 8, 17);

    gpio_set_fun_output_port(rxtx->rx_io, PMC_FO_GP_OCH1, 1, 1); //RX
    gpio_set_fun_output_port(rxtx->tx_io, PMC_FO_GP_OCH0, 1, 1); //TX
}

static void bt_main_msg_handler(void *priv, u8 *msg, u32 len)
{
    switch (msg[0]) {
    case MSG_CMAC_LBUF :
        log_info("cmac_lbuf init ");
        p11_init_app_cmac_lbuf();
        log_info("cmac_lbuf init ok ");
        break;
    case MSG_AES128 :
        log_info("MSG_AES128  ");
        bt_aes128(msg, len);
        break;
    case MSG_CCM_V3 :
        log_info("MSG_CCM_V3 ");
        bt_ccm_v3(msg, len);
        break;
    case MSG_SET_RXTX_GPIO :
        log_info("MSG_SET_RXTX_GPIO ");
        bt_rxtx_gpio(msg, len);
        break;
    }
}

REGISTER_M2P_MSG_HANDLER(0, MSG_BT_MAIN, bt_main_msg_handler);

#ifdef BREDR_PMC_CODE
void bredr_link_set(void *priv, u8 *msg, u32 len);
static void bt_link_msg_handler(void *priv, u8 *msg, u32 len)
{
    bredr_link_set(priv, msg, len);
}

REGISTER_M2P_MSG_HANDLER(0, MSG_BT_LINK, bt_link_msg_handler);
#endif
