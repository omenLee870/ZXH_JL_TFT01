/* #include "sfr.h" */
/* #include "interrupt.h" */
#include "includes.h"
/* #include "clock.h" */
/* #include "asm/power/pmc/pmc_sfr.h" */

#define aes_printf(x, ...) 			//printf(x, ...)
#define aes_printf_hexdump(x,y) 	//put_buf(x,y)


///<--使用pmc q32f 运行aes
/* #define AES_CCM_FROM_PMC */

#if 1

#define READ_BT32_BB(p, i) 	  ((p[i]<<24)|(p[i+1]<<16)|(p[i+2]<<8)|(p[i+3]))
#define WRITE_BT32_BB(p, i, v) \
	do { \
		p[i] = v>>24; \
		p[i+1] = v>>16; \
		p[i+2] = v>>8; \
		p[i+3] = v>>0; \
	} while(0)

#define READ_BT32_LL(p, i) 	  ((p[i+3]<<24)|(p[i+2]<<16)|(p[i+1]<<8)|(p[i+0]))
#define WRITE_BT32_LL(p, i, v) \
	do { \
		p[i] = v>>0; \
		p[i+1] = v>>8; \
		p[i+2] = v>>16; \
		p[i+3] = v>>24; \
	} while(0)

static void reverse_bytes(const uint8_t *src, uint8_t *dst, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        dst[len - 1 - i] = src[i];
    }
}


#ifdef  AES_CCM_FROM_PMC
int aes128_start_enc(unsigned char key[16], unsigned char plaintext[16], unsigned char encrypt[16])
{
    local_irq_disable();
    struct aes128_param_t aes128;
    struct pmc_aes_msg_t  msg;

    aes128.key          = key;
    aes128.plaintext    = plaintext;
    aes128.encrypt      = encrypt;

    msg.msg          = (0 << 8) | MSG_AES128;
    msg.param        = (u8 *)&aes128;


    P2P_AES128 = 0;
    m2p_post_msg_q32f(MSG_BT_MAIN, 1, &msg, sizeof(msg));
    while (P2P_AES128 == 0) {
    }
    local_irq_enable();

    return 0;
}

void ccm_memory_v3(const unsigned char *key, const unsigned char *nonce,
                   u16 pkt_header,
                   const unsigned char *header, unsigned long headerlen,
                   unsigned char *pt,  unsigned char ptlen,
                   unsigned char *ct,	unsigned char *tag, unsigned char taglen, unsigned char direction)
{

    local_irq_disable();
    struct ccm_v3_param_t ccm;
    struct pmc_aes_msg_t  msg;

    ccm.key         = (u8*)key;
    ccm.nonce       = (u8*)nonce;
    ccm.pkt_header  = pkt_header;
    ccm.header      = (u8*)header;
    ccm.headerlen   = headerlen;
    ccm.pt          = (u8*)pt;
    ccm.ptlen       = ptlen;
    ccm.ct          = (u8*)ct;
    ccm.tag         = (u8*)tag;
    ccm.taglen      = taglen;
    ccm.direction   = direction;

    msg.msg          = (0 << 8) | MSG_CCM_V3;
    msg.param        = (u8 *)&ccm;

    P2P_CCM_V3 = 0;
    m2p_post_msg_q32f(MSG_BT_MAIN, 1, &msg, sizeof(msg));
    while (P2P_CCM_V3 == 0) {
    }
    local_irq_enable();
}

#else
int aes128_start_enc(unsigned char key[16], unsigned char plaintext[16], unsigned char encrypt[16])
{
    unsigned int *ctptr;
    local_irq_disable();
    /* return 0; */

    aes_printf("key");
    aes_printf_hexdump(key, 16);

    u32 word;
    word = READ_BT32_BB(key, 12);//*((u32 *)&key[12]);//READ_BT32_BB(key, 0);
    PMC_AES->KEY = word;
    aes_printf("Key 0 : %x ", word);
    word = READ_BT32_BB(key, 8);//*((u32 *)&key[8]);//READ_BT32_BB(key, 4);
    PMC_AES->KEY = word;
    aes_printf("Key 1 : %x ", word);
    word = READ_BT32_BB(key, 4);//*((u32 *)&key[4]);//READ_BT32_BB(key, 8);
    PMC_AES->KEY = word;
    aes_printf("Key 2 : %x ", word);
    word = READ_BT32_BB(key, 0);//*((u32 *)&key[0]);//READ_BT32_BB(key, 12);
    PMC_AES->KEY = word;
    aes_printf("Key 3 : %x ", word);

    PMC_AES->CON = (1 << 4);          // set key
    while ((PMC_AES->CON & (1 << 5))) {
    }; // wait key set ok

    aes_printf("plaintext");
    aes_printf_hexdump(plaintext, 16);

    word = READ_BT32_BB(plaintext, 12);//*((u32 *)&plaintext[12]);//READ_BT32_BB(plaintext, 0);
    PMC_AES->DATIN = word;
    aes_printf("pt 0 : %x ", word);
    word = READ_BT32_BB(plaintext, 8);//*((u32 *)&plaintext[8]);//READ_BT32_BB(plaintext, 4);
    PMC_AES->DATIN = word;
    aes_printf("pt 1 : %x ", word);
    word = READ_BT32_BB(plaintext, 4);//*((u32 *)&plaintext[4]);//READ_BT32_BB(plaintext, 8);
    PMC_AES->DATIN = word;
    aes_printf("pt 2 : %x ", word);
    word = READ_BT32_BB(plaintext, 0);//*((u32 *)&plaintext[0]);//READ_BT32_BB(plaintext, 12);
    PMC_AES->DATIN = word;
    aes_printf("pt 3 : %x ", word);

    PMC_AES->CON  = (1 << 0);         //START ENC
    while (PMC_AES->CON & (1 << 2));  //wait enc

    WRITE_BT32_LL(encrypt, 0, PMC_AES->ENCRES0);
    WRITE_BT32_LL(encrypt, 4, PMC_AES->ENCRES1);
    WRITE_BT32_LL(encrypt, 8, PMC_AES->ENCRES2);
    WRITE_BT32_LL(encrypt, 12, PMC_AES->ENCRES3);

    aes_printf("encrypt");
    aes_printf_hexdump(encrypt, 16);
    local_irq_enable();

    return 0;
}

#define		CCM_ENCRYPT		0
#define		CCM_DECRYPT		1
unsigned char ccm_status;
void ccm_memory_v3(const unsigned char *key, const unsigned char *nonce,
                   u16 pkt_header,
                   const unsigned char *header, unsigned long headerlen,
                   unsigned char *pt,  unsigned char ptlen,
                   unsigned char *ct,	unsigned char *tag, unsigned char taglen, unsigned char direction)
{
    int i;
    /* OS_ENTER_CRITICAL(); */
    local_irq_disable();

    aes_printf("key");
    aes_printf_hexdump((u8 *)key, 16);

    PMC_AES->CTCNT   = ptlen;
    aes_printf("CNT : %x ", ptlen);

    u32 word;
    word = READ_BT32_LL(key, 0);
    PMC_AES->KEY = word;
    aes_printf("Key 0 : %x ", word);
    word = READ_BT32_LL(key, 4);
    PMC_AES->KEY = word;
    aes_printf("Key 1 : %x ", word);
    word = READ_BT32_LL(key, 8);
    PMC_AES->KEY = word;
    aes_printf("Key 2 : %x ", word);
    word = READ_BT32_LL(key, 12);
    PMC_AES->KEY = word;
    aes_printf("Key 3 : %x ", word);
    PMC_AES->CON = BIT(4);       //set key
    if (headerlen) {
        ASSERT(headerlen == 1, "Header length exceed");
        PMC_AES->CON |= BIT(11);
    } else {
        PMC_AES->CON &= ~BIT(11);
    }
    while (PMC_AES->CON & 0x20) {

    };   //wait key set ok

    aes_printf("nonce");
    aes_printf_hexdump((u8 *)nonce, 13);
    /* PMC_AES->CON = 0; */
    /* PMC_AES->CON |= BIT(9);        //CCM_IE */
    PMC_AES->TAGLEN  = taglen;
    aes_printf("TAGLEN : %x ", taglen);

    word = READ_BT32_LL(nonce, 0);
    PMC_AES->NONCE = word;
    aes_printf("Nonce 0 : %x ", word);
    word = READ_BT32_LL(nonce, 4);
    PMC_AES->NONCE = word;
    aes_printf("Nonce 1 : %x ", word);
    word = READ_BT32_LL(nonce, 8);
    PMC_AES->NONCE = word;
    aes_printf("Nonce 2 : %x ", word);
    word = nonce[12];//READ_BT32_LL(nonce, 12);
    PMC_AES->NONCE = word;
    aes_printf("Nonce 3 : %x ", word);
    PMC_AES->PHEADER = pkt_header;

    PMC_AES->HEADER = *header;
    aes_printf("Header : %x ", *header);

    ccm_status = 0;

    if (direction == CCM_ENCRYPT) {
        aes_printf("CCM_ENCRYPT");
        aes_printf_hexdump(pt, ptlen);
        PMC_AES->SRCADR = (unsigned int)pt;
        aes_printf_hexdump(ct, ptlen);
        PMC_AES->DSTADR = (unsigned int)ct;
        PMC_AES->CON |= BIT(16);        //CCM_ENC_KST
    } else {
        aes_printf("CCM_DECRYPT");
        aes_printf_hexdump(ct, ptlen);
        PMC_AES->SRCADR = (unsigned int)ct;
        aes_printf_hexdump(pt, ptlen);
        PMC_AES->DSTADR = (unsigned int)pt;
        PMC_AES->CON |= BIT(18);        //CCM_DEC_KST
    }
    /* while(1) */
    /* { */
    /* if(ccm_status==1) */
    /* break; */
    /* asm volatile ("idle"); */
    /* }	 */

    while ((PMC_AES->CON & BIT(31)) == 0);  //CCM_PND
    PMC_AES->CON |= BIT(30);                //CLR_CCM_PND
    /* lp_waiting((int *)(&PMC_AES->CON), BIT(31), BIT(30), IRQ_AES_IDX); */

    WRITE_BT32_BB(tag, 0, PMC_AES->TAGRES0);    //这里读了16 bytes给tag//
    WRITE_BT32_BB(tag, 4, PMC_AES->TAGRES1);    //可根据实际应用读taglen//
    WRITE_BT32_BB(tag, 8, PMC_AES->TAGRES2);
    WRITE_BT32_BB(tag, 12, PMC_AES->TAGRES3);

    aes_printf("MIC");
    aes_printf_hexdump(tag, 16);

    /* OS_EXIT_CRITICAL(); */
    local_irq_enable();
}
#endif


#endif
