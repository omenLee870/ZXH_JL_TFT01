#include "includes.h"
#include "sdk_config.h"

#define LOG_TAG_CONST       MAIN
#define LOG_TAG             "[pmu msg]"
#include "generic/log.h"

#define MSG_PMU_GET_FREE_RAM		0x01
#define MSG_PMU_POST_FREE_RAM		0x02

#define READ_BIG_U16(a)   		((*((u8*)(a)) <<8)  + *((u8*)(a)+1))
#define READ_BIG_U32(a)   		((*((u8*)(a)) <<24) + (*((u8*)(a)+1)<<16) + (*((u8*)(a)+2)<<8) + *((u8*)(a)+3))

#define WRITE_BIG_U16(a,src)   	{*((u8*)(a)+0) = (u8)(src>>8); *((u8*)(a)+1) = (u8)(src&0xff); }
#define WRITE_BIG_U32(a,src)   	{*((u8*)(a)+0) = (u8)((src)>>24);  *((u8*)(a)+1) = (u8)(((src)>>16)&0xff);*((u8*)(a)+2) = (u8)(((src)>>8)&0xff);*((u8*)(a)+3) = (u8)((src)&0xff);}


extern int pmu_free_begin;
extern int pmu_free_size;

static void pmu_msg_handler(void *priv, u8 *msg, u32 len)
{
    log_info("pmu_msg_handler = %d\n", msg[0]);
    if (msg[0] == MSG_PMU_POST_FREE_RAM) {
        u32 free_start, free_end, used_start, used_end;
        free_start = READ_BIG_U32(&msg[1]);
        free_end   = READ_BIG_U32(&msg[1 + 4]);
        used_start = (u32)(&pmu_free_begin);
        used_end   = used_start + (u32)(&pmu_free_size);
        log_info("pmu free ram:0x%x, 0x%x \n", free_start, free_end);
        log_info("used pmu free ram:0x%x, 0x%x \n", used_start, used_end);
        if ((used_start < free_start) || (used_end > free_end)) {
            log_error("used pmu free ram error \n");
            local_irq_disable();
            while (1);
        }
    }
}
REGISTER_M2P_MSG_HANDLER(0, MSG_PMU_PMC, pmu_msg_handler);

void pmu_free_ram_init(void)
{
    u8 msg[1];
    msg[0] = MSG_PMU_GET_FREE_RAM;
    m2p_post_msg(MSG_PMU_PMC, 0, (u8 *)msg, sizeof(msg));
}

