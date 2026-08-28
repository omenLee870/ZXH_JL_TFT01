
#include "includes.h"
/* #include "string.h" */
/* #include "typedef.h" */
/* #include "tomcrypt.h" */
/* #include "os/os_cpu.h" */

#define aes_printf printf
#define aes_printf_hexdump put_buf

#define y_printf printf
#define r_printf printf

int aes128_start_enc(unsigned char key[16], unsigned char plaintext[16], unsigned char encrypt[16]);
void ccm_memory_v3(const unsigned char *key, const unsigned char *nonce,
                   u16 pkt_header,
                   const unsigned char *header, unsigned long headerlen,
                   unsigned char *pt,  unsigned char ptlen,
                   unsigned char *ct,	unsigned char *tag, unsigned char taglen, unsigned char direction);

// general swap/endianess utils
void btctler_reverse_bytes(const uint8_t *src, uint8_t *dst, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        dst[len - 1 - i] = src[i];
    }
}
void btctler_reverse_128(const uint8_t *src, uint8_t *dst)
{
    btctler_reverse_bytes(src, dst, 16);
}


#define AES_CCM_V3_TEST

void aes128_start(u8 key[16], u8 plaintext[16], u8 encrypt[16])
{
    local_irq_disable();
    aes128_start_enc(key, plaintext, encrypt);
    local_irq_enable();
}

void aes128_test()
{
    printf("<<<<<<<<<<<<<<  aes128_test >>>>>>>>>>>>>> ");

    u8 key[] = {0x4C, 0x68, 0x38, 0x41, 0x39, 0xF5, 0x74, 0xD8,
                0x36, 0xBC, 0xF3, 0x4E, 0x9D, 0xFB, 0x01, 0xBF
               };
    u8 plaintext[] = {0x02, 0x13, 0x24, 0x35, 0x46, 0x57, 0x68,
                      0x79, 0xac, 0xbd, 0xce, 0xdf, 0xe0, 0xf1, 0x02, 0x13
                     };
    u8 enc[] = {0x99, 0xad, 0x1b, 0x52, 0x26, 0xa3, 0x7e, 0x3e,
                0x05, 0x8e, 0x3b, 0x8e, 0x27, 0xc2, 0xc6, 0x66
               };
    u8 encrypt[16];
    u8 _encrypt[16];
    u8 _key[16];
    u8 _plaintext[16];

    aes_printf("aes128_test\r\n");

    btctler_reverse_128(key, _key);
    btctler_reverse_128(plaintext, _plaintext);

    aes128_start(_key, _plaintext, encrypt);

    btctler_reverse_128(encrypt, _key);
    put_buf(_key, 16);
    if (!memcmp(enc, _key, 16)) {
        printf("xxxxxxxxxxx AES128 Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx AES128 Dencrypt  err  xxxxxxxxxxxxx ");
        ASSERT(0);
    }

    /* ASSERT(!memcmp(enc, _key, 16), "%s\r\n", "AES128 encode err!"); */


    /* aes_printf_hexdump(encrypt, 16);
    aes_printf_hexdump(_encrypt, 16); */
    /* aes_printf_hexdump(encrypt, 16); */

    aes_printf("ah test\r\n");

    //MSB->LSB
    u8 irk[] = {0xec, 0x02, 0x34, 0xa3, 0x57, 0xc8, 0xad, 0x05, 0x34, 0x10, 0x10, 0xa6, 0x0a, 0x39, 0x7d, 0x9b};

    u8 prand[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x81, 0x94};

    u8 ah[] = {0x15, 0x9d, 0x5f, 0xb7, 0x2e, 0xbe, 0x23, 0x11, 0xa4, 0x8c, 0x1b, 0xdc, 0xc4, 0x0d, 0xfb, 0xaa};

    btctler_reverse_128(irk, _key);
    btctler_reverse_128(prand, _plaintext);

    /* aes128_start(irk, prand, encrypt); */
    aes128_start(_key, _plaintext, encrypt);

    btctler_reverse_128(encrypt, _key);
    /* ASSERT(!memcmp(ah, _key, 16), "%s\r\n", "ah encode err!"); */
    if (!memcmp(ah, _key, 16)) {
        printf("xxxxxxxxxxx AES128 Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx AES128 Dencrypt  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }

    /* aes_printf_hexdump(encrypt, 16); */
    /* while (1); */
    printf("<<<<<<<<<<<<<<  aes128_test over >>>>>>>>>>>>>> ");
    printf("\n\n -----------------------------------------------  \n\n  ");
}

void aes_ccm_esco_test(void)
{
    printf("<<<<<<<<<<<<<<  aes_ccm_esco_test >>>>>>>>>>>>>> ");

    u8 yctrs[48];
    u8 pt[251] = {0};
    u8 nonce[13] = {0};
    u8 edr_ccm_block[32] = {0};
    u8 tag[16] = {0};

    u8 lt_addr 		= 1 ;
    u8 packet_type 	= 7 ;
    u8 llid 		= 0 ;
    u16 payload_len = 0x1e;
    u8 nonce_type = 1;

    u32 clk = 0x030b5a8f;
    u32 daycounter = 0x061a;
    u8 iv[] = {
        0x0f, 0x63, 0xe9, 0xdf, 0xbd, 0x90, 0xb6, 0x70,
    };
    u8 skd[] = {
        0x97, 0x2b, 0x7d, 0xcd, 0xbe, 0x89, 0x42, 0xb7,
        0xd8, 0xfd, 0xc3, 0x56, 0xa5, 0x59, 0x0a, 0xca,
    };
    u8 payload[] = {
        0x46, 0x25, 0xf7, 0x78, 0x57, 0x82, 0x90, 0xf9,
        0x0e, 0xe8, 0x35, 0x76, 0xb3, 0xf8, 0xa8, 0x98,
        0xfa, 0xff, 0x3f, 0xe4, 0x5d, 0xb8, 0x12, 0xb2,
        0xe1, 0x89, 0x29, 0x7a, 0xfd, 0x89,
    };

    u8 enc_payload[] = {
        0x76, 0x63, 0x79, 0x37, 0x69, 0xb2, 0xba, 0x4c,
        0x41, 0x70, 0x41, 0x00, 0xd6, 0xc4, 0x60, 0x2a,
        0xdb, 0xae, 0x2a, 0x87, 0xf7, 0x7a, 0x9f, 0xdb,
        0xa0, 0xc5, 0x6d, 0x2c, 0xb5, 0x32,
    };

    nonce[0] = clk  >> 0 & 0xff; //clk[8:1]
    nonce[1] = (clk >> 8) & 0xff; //clk[16:9]
    nonce[2] = (clk >> 16) & 0xff; //clk[24:17]
    nonce[3] = ((clk >> 24) & 0x07) |
               ((daycounter & 0x1f) << 3); //clk[27:25]
    nonce[4] = ((daycounter >> 5) & 0x3f) |
               ((nonce_type & 0x03) << 6);


    memcpy(nonce + 5, iv, 8);

    edr_ccm_block[0] = 0x49; //ccm
    memcpy(edr_ccm_block + 1, nonce, 13); //nonce

    edr_ccm_block[14] = (payload_len >> 8) & 0xff; //payload H
    edr_ccm_block[15] = payload_len & 0xff; //payload L

    edr_ccm_block[16] = 0;
    edr_ccm_block[17] = ((lt_addr & 0x7) << 0) | ((packet_type & 0xf) << 3);
    edr_ccm_block[18] = ((llid & 0x3) << 0);



    printf("---------Encrypt -------------- ");

    printf("---------key -------------- ");
    put_buf(skd, 16);

    printf("---------nonce -------------- ");
    put_buf(nonce, 13);

    printf("---------bx -------------- ");
    put_buf(edr_ccm_block, 32);

    memset(tag, 0, sizeof(tag));
    memset(pt, 0, payload_len);
    ///< enc

    u8 phead_h = 0;
    u8 phead_l = ((lt_addr & 0x3) << 0) | ((packet_type & 0xf) << 3);

    u16 phead = (phead_h << 8) | phead_l;
    u8 head  = ((llid & 0x3) << 0);

#ifdef AES_CCM_V3_TEST
    printf(" +++++++++++++++  AES_CCM_V3_TEST +++++++++++++ ");
    ccm_memory_v3(skd, nonce, phead, &head, 1, payload, payload_len, pt, tag, 4, 0);
#else

    ccm_core_with_keyBx(skd, edr_ccm_block, sizeof(edr_ccm_block), yctrs,
                        payload, payload_len, pt, tag, 0);
#endif


    printf("---------yctrs -------------- ");
    put_buf(yctrs, 48);

    printf("---------payload sample data-------------- ");
    put_buf(payload, payload_len);
    printf("---------en payload sample data -------------- ");
    put_buf(enc_payload, payload_len);

    printf("---------ccm en payload -------------- ");
    put_buf(pt, payload_len);

    if (!memcmp(pt, enc_payload, payload_len)) {
        printf("xxxxxxxxxxx  Encrypt Data Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt Data err xxxxxxxxxxxxx ");
        ASSERT(0);
    }


    printf("\n ---------Decrypt -------------- \n\n");
    memset(tag, 0, sizeof(tag));
    memset(pt, 0, payload_len);
#ifdef AES_CCM_V3_TEST
    printf(" +++++++++++++++  AES_CCM_V3_TEST +++++++++++++ ");
    ccm_memory_v3(skd, nonce, phead, &head, 1, pt, payload_len, enc_payload, tag, 4, 1);
#else
    ///decrypt
    ccm_core_with_keyBx(skd, edr_ccm_block, sizeof(edr_ccm_block), yctrs,
                        pt, payload_len, enc_payload, tag, 1);
#endif

    put_buf(pt, payload_len);

    if (!memcmp(pt, payload, payload_len)) {
        printf("xxxxxxxxxxx  Dncrypt Data Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dncrypt Data err xxxxxxxxxxxxx ");
        ASSERT(0);
    }

    printf("\n\n /////////// aes_ccm_esco_test over //////////// \n\n  ");
    printf("\n\n -----------------------------------------------  \n\n  ");
}

void aes_ccm_test_case1()
{
    printf("<<<<<<<<<<<<<<   aes_ccm_test_case1 >>>>>>>>>>>>>> ");
#if 1//for edr

    u8 tag[16] = {0};
    u8 nonce[13] = {0};
    u8 pt[251] = {0};

    /* u8 skd[16] = {0x23,0x45,0x23,0x45,0x23,0x45,0x23,0x45,0x67,0x89,0x67,0x89,0x67,0x89,0x67,0x89}; */
    u8 skd[16] = {0x89, 0x67, 0x89, 0x67, 0x89, 0x67, 0x89, 0x67, 0x45, 0x23, 0x45, 0x23, 0x45, 0x23, 0x45, 0x23};
    u8 iv[8] = {0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88, 0x77, 0x66};
    /* u8 payload[8] = {0x6f,0x6e,0x6d,0x6c,0x6b,0x6a,0x69,0x68}; */
    u8 payload[8] = {0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f};
    u16 payload_len = 8;
    /* u8 MIC_sample[4] = {0x11, 0xa0, 0xd8, 0xb1}; */
    u8 MIC_sample[4] = {0xb1, 0xd8, 0xa0, 0x11};
    u8 enc_date[8] = {0xb0, 0xae, 0x89, 0x8a, 0x6e, 0x68, 0x64, 0xd4};


#define NONCE_CLOCK(x) ((x&0x7)<<0)
#define NONCE_DAYCOUNTER1(x) ((x&0x1f)<<3)
#define NONCE_DAYCOUNTER2(x) ((x&0x1f)<<3)

    u32 clock = 0xbc614e;
    u16 day_counter = 0;
    nonce[0] = clock & 0xff; //clk[8:1]
    nonce[1] = (clock >> 8) & 0xff; //clk[16:9]
    nonce[2] = (clock >> 16) & 0xff; //clk[24:17]
    nonce[3] = NONCE_CLOCK(clock >> 24) | NONCE_DAYCOUNTER1(day_counter & 0x1f); //clk[27:25]
    nonce[4] = NONCE_DAYCOUNTER2(day_counter >> 5);


    u8 acl_u_continuation = 0; //0:otherwise 1:zero-length acl-u continuation packet
    u8 send_dirction = 0; //0:master to slave  1:slave to master
    u8 nonce_type = 0; //0:acl  1:esco

    nonce[4] |= ((acl_u_continuation & 0x1) << 4) | ((send_dirction & 0x1) << 5) | ((nonce_type & 0x3) << 6);
    memcpy(nonce + 5, iv, 8);


    u8 lt_addr = 1, packet_type = 3, llid = 2;
    /* ccm_with_key(skd, nonce,13, (u8*)&head,  3, pt, 8, payload, tag, 4, 0); */
    u8 edr_ccm_block[32] = {0};
    edr_ccm_block[0] = 0x49; //ccm
    memcpy(edr_ccm_block + 1, nonce, 13); //nonce

    edr_ccm_block[14] = (payload_len >> 8) & 0xff; //payload H
    edr_ccm_block[15] = payload_len & 0xff; //payload L

    edr_ccm_block[16] = 0;
    edr_ccm_block[17] = ((lt_addr & 0x3) << 0) | ((packet_type & 0xf) << 3);
    edr_ccm_block[18] = ((llid & 0x3) << 0);
    u8 yctrs[48];

    puts("key:");
    put_buf(skd, 16);

    puts("nonce:");
    put_buf(nonce, 13);

    puts("Bx:");
    put_buf(edr_ccm_block, sizeof(edr_ccm_block));

    puts("payload:");
    put_buf(payload, payload_len);


    u8 phead_h = 0;
    u8 phead_l = ((lt_addr & 0x3) << 0) | ((packet_type & 0xf) << 3);

    u16 phead = (phead_h << 8) | phead_l;
    u8 head  = ((llid & 0x3) << 0);
#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd, nonce, phead, &head, 1, payload, payload_len, pt, tag, 4, 0);
#else
    ccm_core_with_keyBx(skd, edr_ccm_block, sizeof(edr_ccm_block), yctrs,
                        payload, payload_len, pt, tag, 0);
#endif
    puts("yctrs");
    put_buf(yctrs, 48);

    puts("MIC");
    put_buf(tag, 4);
    put_buf(pt, payload_len);
    if (!memcmp(pt, enc_date, payload_len)) {
        printf("xxxxxxxxxxx  Encrypt Data  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt Data err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    if (!memcmp(tag, MIC_sample, 4)) {
        printf("xxxxxxxxxxx  Encrypt MIC   Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt MIC err xxxxxxxxxxxxx ");
        ASSERT(0);
    }


    memset(tag, 0, sizeof(tag));
    memset(pt, 0, payload_len);

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd, nonce, phead, &head, 1, pt, payload_len, enc_date, tag, 4, 1);
#else
    ccm_core_with_keyBx(skd, edr_ccm_block, sizeof(edr_ccm_block), yctrs,
                        pt, payload_len, enc_date, tag, 1);
#endif

    puts("payload_dec:");
    put_buf(pt, payload_len);

    puts("mic_dec:");
    put_buf(tag, 4);

    if (!memcmp(pt, payload, payload_len)) {
        printf("xxxxxxxxxxx  Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    if (!memcmp(tag, MIC_sample, 4)) {
        printf("xxxxxxxxxxx  Dencrypt MIC  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt MIC  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }

#endif
    printf("\n\n -----------------------------------------------  \n\n  ");

}

void aes_ccm_test_case2()
{

    printf("<<<<<<<<<<<<<<   aes_ccm_test_case2 >>>>>>>>>>>>>> ");
    u8 tag[16] = {0};
    u8 nonce[13] = {0};
    u8 pt[251] = {0};
    //Core V5.0 | Vol 6, Part C
#if 1

    u8 head = 3;
    u16 phead = 0x0001;

    u8 skd[16] = {0x66, 0xC6, 0xC2, 0x27, 0x8E, 0x3B, 0x8E, 0x05, 0x3E, 0x7E, 0xA3, 0x26, 0x52, 0x1B, 0xAD, 0x99};
    u8 iv[8] = {0x24, 0xab, 0xdc, 0xba, 0xbe, 0xba, 0xaf, 0xde};

    u8 skd_flip[16];
    u8 iv_flip[16];

    //Slave -> Master
    u32 rx_counter_l = 0;
    u32 rx_counter_h = 0;

    nonce[0] = rx_counter_l;
    nonce[1] = rx_counter_l >> 8;
    nonce[2] = rx_counter_l >> 16;
    nonce[3] = rx_counter_l >> 24;
    nonce[4] = rx_counter_h;

    //slave
    nonce[4] |= BIT(7);

    memcpy(nonce + 5, iv, 8);

    //Encrypt Sample
    u8 ll_start_enc_rsp[] = {0x9f};
    u8 MIC_sample[] = {0xcd, 0xa7, 0xf4, 0x48};

    btctler_reverse_128(skd, skd_flip);

    /* rijndael_setup(skd_flip); */
    /* ccm_memory(nonce, &head, 1, pt, 1, ll_start_enc_rsp, tag, 1); */

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd_flip, nonce, phead, &head, 1, pt, 1, ll_start_enc_rsp, tag, 4, 1);
#else
    ccm_memory_v2(skd_flip, nonce, &head, 1, pt, 1, ll_start_enc_rsp, tag, 4, 1);
#endif

    aes_printf("MIC");
    aes_printf_hexdump(tag, 4);
    if (!memcmp(tag, MIC_sample, 4)) {
        printf("xxxxxxxxxxx  Dencrypt MIC  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt MIC  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }

    aes_printf("PT");
    aes_printf_hexdump(pt, 1);
    if (pt[0] == 0x06) {
        printf("xxxxxxxxxxx  Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    /*------------------------------------------------*/

    //Master -> Slave
    rx_counter_l = 0;
    rx_counter_h = 0;

    nonce[0] = rx_counter_l;
    nonce[1] = rx_counter_l >> 8;
    nonce[2] = rx_counter_l >> 16;
    nonce[3] = rx_counter_l >> 24;
    nonce[4] = rx_counter_h;

    memcpy(nonce + 5, iv, 8);

    u8 ll_start_enc_rsp2[] = {0xa3};
    u8 MIC_sample2[] = {0x4c, 0x13, 0xa4, 0x15};

    btctler_reverse_128(skd, skd_flip);

    /* rijndael_setup(skd_flip); */
    /* ccm_memory(nonce, &head, 1, pt, 1, ll_start_enc_rsp2, tag, 1); */

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd_flip, nonce, phead, &head,  1, pt, 1, ll_start_enc_rsp2, tag, 4, 1);
#else
    ccm_memory_v2(skd_flip, nonce, &head,  1, pt, 1, ll_start_enc_rsp2, tag, 4, 1);
#endif

    aes_printf("MIC");
    aes_printf_hexdump(tag, 4);
    if (!memcmp(tag, MIC_sample2, 4)) {
        printf("xxxxxxxxxxx  Dencrypt MIC  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt MIC  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }

    aes_printf("PT");
    aes_printf_hexdump(pt, 1);
    if (pt[0] == 0x06) {
        printf("xxxxxxxxxxx  Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    /*------------------------------------------------*/

#endif

    printf("\n\n -----------------------------------------------  \n\n  ");
}

void aes_ccm_test_case3()
{
    u8 tag[16] = {0};
    u8 nonce[13] = {0};
    u8 pt[251] = {0};
#if 1
    printf("<<<<<<<<<<<<<<   aes_ccm_test_case3 >>>>>>>>>>>>>> ");
    u8 head = 1;
    u16 phead = 0x0001;

    u8 skd[16] = {0x74, 0xFA, 0xE2, 0x87, 0x9D, 0xCA, 0xC3, 0x36, 0xB7, 0x76, 0x27, 0x21, 0x5E, 0x3D, 0x49, 0xBD};

    u8 nonce_[13] = {0x7C, 0x05, 0x00, 0x00, 0x00, 0x06, 0xBB, 0xC1, 0x35, 0xC6, 0x3B, 0x76, 0x5B};

    u8 data[27] = {0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54, 0x54};

    u8 enc[27] = {0x75, 0xC4, 0x04, 0x93, 0xA9, 0xAB, 0x7F, 0x0E, 0x3B, 0xB0, 0xCF, 0x21, 0xEA, 0xE0, 0x84, 0x76, 0x49, 0x47, 0x5D, 0x35, 0xF1, 0x3D, 0xC8, 0xCF, 0x10, 0x27, 0xFE};

    u8 mic[4] = {0x59, 0x81, 0x00, 0x8a};

    /* rijndael_setup(skd); */
    /* ccm_memory(nonce_, &head, 1, data, 27, pt, tag, 0); */

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd, nonce_, phead, &head, 1, data, 27, pt, tag, 4, 0);
#else
    ccm_memory_v2(skd, nonce_, &head, 1, data, 27, pt, tag, 4, 0);
#endif

    aes_printf("MIC");
    aes_printf_hexdump(tag, 4);
    aes_printf("PT");
    aes_printf_hexdump(pt, 27);

    if (!memcmp(pt, enc, 27)) {
        printf("xxxxxxxxxxx  Encrypt Data  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt Data err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    if (!memcmp(tag, mic, 4)) {
        printf("xxxxxxxxxxx  Encrypt MIC   Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt MIC err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    /*------------------------------------------------*/

    /* rijndael_setup(skd); */
    /* ccm_memory(nonce_, &head, 1, pt, 27, enc, tag, 1); */

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd, nonce_, phead, &head, 1, pt, 27, enc, tag, 4, 1);
#else
    ccm_memory_v2(skd, nonce_, &head, 1, pt, 27, enc, tag, 4, 1);
#endif

    aes_printf("MIC");
    aes_printf_hexdump(tag, 4);
    aes_printf("PT");
    aes_printf_hexdump(pt, 27);

    if (!memcmp(pt, data, 27)) {
        printf("xxxxxxxxxxx  Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    if (!memcmp(tag, mic, 4)) {
        printf("xxxxxxxxxxx  Dencrypt MIC  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt MIC  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    /*------------------------------------------------*/

#endif
    printf("\n\n -----------------------------------------------  \n\n  ");

}

void aes_ccm_test_case4()
{
    u8 tag[16] = {0};
    u8 nonce[13] = {0};
    u8 pt[251] = {0};

    printf("<<<<<<<<<<<<<<   aes_ccm_test_case4 >>>>>>>>>>>>>> ");
#if 1
    //48M enc-94us   dec-69us

    u8 head = 2;
    u8 phead = 1;

    u8 skd[16] = {0x0D, 0x5F, 0x0A, 0x82, 0xDA, 0xAC, 0x57, 0xFE, 0x43, 0x77, 0xE0, 0xCD, 0x12, 0xE5, 0x41, 0x1B};

    u8 nonce_[13] = {0x42, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x4C, 0x2F, 0x65, 0xF1, 0xAA, 0x54, 0xA8};

    u8 data[251] = {
        0xF7, 0x00, 0x04, 0x00, 0x1B, 0x09, 0x00, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45,
        0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45, 0x45
    };

    u8 enc[251] = {
        0xA7, 0x94, 0x6D, 0xB2, 0x93, 0x3B, 0x28, 0x28, 0x43, 0xA3, 0xE1, 0x9C, 0xF8, 0xB8, 0xB4, 0xB4,
        0x60, 0xBC, 0xAA, 0x99, 0x27, 0x47, 0x2B, 0x2C, 0xF6, 0xC4, 0x43, 0xC4, 0x47, 0x00, 0xBF, 0xB2,
        0xAA, 0x1C, 0xD1, 0x34, 0x79, 0x21, 0x9B, 0xD1, 0x3E, 0x8E, 0x8B, 0x67, 0x93, 0x75, 0xA0, 0x6D,
        0x92, 0x3C, 0xCC, 0xE5, 0xD7, 0x07, 0xE5, 0x75, 0x23, 0x6C, 0xF0, 0x74, 0xE1, 0x90, 0xD9, 0x76,
        0x99, 0xEC, 0xA9, 0x6F, 0xD7, 0x49, 0x76, 0x2F, 0x7B, 0x19, 0xB0, 0xDE, 0xA2, 0x51, 0x3D, 0xE6,
        0xEC, 0x0D, 0xFB, 0x21, 0x73, 0x6C, 0xFA, 0x78, 0x0B, 0x7B, 0xB9, 0xF9, 0x12, 0x10, 0x57, 0x04,
        0x95, 0x03, 0x5A, 0x6B, 0xC3, 0xB4, 0x43, 0x7C, 0x4F, 0x6C, 0x19, 0x7B, 0xCB, 0x40, 0x47, 0x8E,
        0xC0, 0x32, 0x6B, 0x5C, 0xD7, 0x6D, 0x24, 0xB2, 0x88, 0x27, 0x28, 0xBF, 0x32, 0xF3, 0x62, 0x22,
        0xE3, 0x53, 0xA2, 0xB5, 0x94, 0xF8, 0x5D, 0x78, 0x28, 0x00, 0xAA, 0x1B, 0x0D, 0x46, 0xA2, 0xCF,
        0x4F, 0x25, 0x20, 0x36, 0xE8, 0xDB, 0x6B, 0x14, 0x1E, 0x97, 0x80, 0xD1, 0xEC, 0x6A, 0xA5, 0x29,
        0x2F, 0x11, 0xF1, 0xA7, 0x85, 0x8B, 0x04, 0xFA, 0xAB, 0x9F, 0xC0, 0x55, 0xF0, 0x9A, 0xF0, 0x93,
        0xC7, 0x6C, 0x22, 0x59, 0x69, 0xC8, 0x12, 0xEC, 0x1A, 0xA6, 0x58, 0xF1, 0x9A, 0x31, 0x44, 0xD2,
        0xA4, 0x8D, 0xAF, 0x69, 0xB1, 0x71, 0xB3, 0x04, 0x9F, 0xA2, 0xDE, 0xAA, 0x95, 0xE8, 0x1D, 0x27,
        0xD9, 0xFD, 0x12, 0xAA, 0xA8, 0xE5, 0xE5, 0x5B, 0xD2, 0x5C, 0x35, 0x57, 0x47, 0xDC, 0x10, 0x14,
        0x04, 0x77, 0xA7, 0x36, 0x89, 0x83, 0x7C, 0xAC, 0x80, 0xC9, 0x3B, 0x55, 0x14, 0x57, 0x5F, 0xD8,
        0x9B, 0xAC, 0xD3, 0x3A, 0x0E, 0x6A, 0x36, 0x32, 0x96, 0x18, 0x52
    };

    u8 mic[4] = {0x23, 0xF6, 0x79, 0x74};

    /* rijndael_setup(skd); */
    /* ccm_memory(nonce_, &head, 1, data, 27, pt, tag, 0); */

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd, nonce_, phead, &head, 1, data, 251, pt, tag, 4, 0);
#else
    ccm_memory_v2(skd, nonce_, &head, 1, data, 251, pt, tag, 4, 0);
#endif

    aes_printf("MIC");
    aes_printf_hexdump(tag, 4);
    aes_printf("PT");
    aes_printf_hexdump(pt, 251);

    if (!memcmp(pt, enc, 251)) {
        printf("xxxxxxxxxxx  Encrypt Data  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt Data err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    if (!memcmp(tag, mic, 4)) {
        printf("xxxxxxxxxxx  Encrypt MIC   Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Encrypt MIC err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    /*------------------------------------------------*/

    /* rijndael_setup(skd); */
    /* ccm_memory(nonce_, &head, 1, pt, 27, enc, tag, 1); */

#ifdef AES_CCM_V3_TEST
    ccm_memory_v3(skd, nonce_, phead, &head, 1, pt, 251, enc, tag, 4, 1);
#else
    ccm_memory_v2(skd, nonce_, &head, 1, pt, 251, enc, tag, 4, 1);
#endif

    aes_printf("MIC");
    aes_printf_hexdump(tag, 4);
    aes_printf("PT");
    aes_printf_hexdump(pt, 251);

    if (!memcmp(pt, data, 251)) {
        printf("xxxxxxxxxxx  Dencrypt  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    if (!memcmp(tag, mic, 4)) {
        printf("xxxxxxxxxxx  Dencrypt MIC  Correct xxxxxxxxxxxxx ");
    } else {
        printf("xxxxxxxxxxx  Dencrypt MIC  err xxxxxxxxxxxxx ");
        ASSERT(0);
    }
    /*------------------------------------------------*/

#endif
    printf("\n\n -----------------------------------------------  \n\n  ");

}

void aes_ccm_test(void)
{
    printf("------  aes_ccm_test --------- ");

    aes_ccm_esco_test();
    aes_ccm_test_case1();
    aes_ccm_test_case2();
    aes_ccm_test_case3();
    aes_ccm_test_case4();


    printf("------  aes_ccm_test over --------- ");

}

