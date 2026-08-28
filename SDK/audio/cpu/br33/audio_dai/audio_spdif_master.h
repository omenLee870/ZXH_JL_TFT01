#include "generic/typedef.h"
#include "generic/list.h"
#include "audio_cfifo.h"

#define SPDIF_TX_CLK		  192 * 1000000UL//  480 * 1000000UL
#define SM_RESET()		do {JL_SPDIF->SM_CLK = 0; JL_SPDIF->SM_CON = 0; JL_SPDIF->SM_ABUFLEN = 0; JL_SPDIF->SM_AHPTR = 0; JL_SPDIF->SM_ASPTR = 0; JL_SPDIF->SM_ASHN; JL_SPDIF->SM_APNS = 0; JL_SPDIF->SM_IBUFLEN = 0; JL_SPDIF->SM_IHPTR = 0;} while(0)

#define SM_EN(x)         SFR(JL_SPDIF->SM_CLK, 0, 1, x)

#define SM_CLK_DIV_MODE(x)         SFR(JL_SPDIF->SM_CLK, 3, 2, x)	//0b00
#define SM_CLK_DIV_NSEL(x)         SFR(JL_SPDIF->SM_CLK, 5, 2, x)	//0b00

#define SM_CLK_INT_DIV(x)          SFR(JL_SPDIF->SM_CLK, 7, 9, x)
#define SM_CLK_FRAC_DIV(x)         SFR(JL_SPDIF->SM_CLK, 16, 16, x)


#define SM_OUT_EN(x)         SFR(JL_SPDIF->SM_CON, 0, 1, x)

#define SM_DAT_MODE(x)         SFR(JL_SPDIF->SM_CON, 1, 1, x)

#define SM_AUX_SEL(x)         SFR(JL_SPDIF->SM_CON, 2, 1, x)

#define SM_VAL_FLAG_L(x)         SFR(JL_SPDIF->SM_CON, 3, 1, x)
#define SM_VAL_FLAG_R(x)         SFR(JL_SPDIF->SM_CON, 4, 1, x)

#define SM_AUX_DAT_L(x)         SFR(JL_SPDIF->SM_CON, 5, 4, x)
#define SM_AUX_DAT_R(x)         SFR(JL_SPDIF->SM_CON, 9, 4, x)

#define SM_DATA_IE(x)         SFR(JL_SPDIF->SM_CON, 13, 1, x)
#define SM_INFO_IE(x)         SFR(JL_SPDIF->SM_CON, 14, 1, x)

#define SW_VAL_USER(x)         SFR(JL_SPDIF->SM_CON, 15, 1, x)

#define INF_IS_PND()        (JL_SPDIF->SM_CON & BIT(26))
#define DAT_IS_PND()        (JL_SPDIF->SM_CON & BIT(27))

#define INF_PND_CLR()    (JL_SPDIF->SM_CON |= BIT(30))
#define DAT_PND_CLR()    (JL_SPDIF->SM_CON |= BIT(31))

#define SM_DAT_LEN(x)           SFR(JL_SPDIF->SM_ABUFLEN, 0, 16, x)
#define SM_INF_LEN(x)           SFR(JL_SPDIF->SM_IBUFLEN, 0, 16, x)

#define GET_DATABUF_REMAIN_BYTE()       (JL_SPDIF->SM_ASHN * 4)
#define SET_DATABUF_WRITE_BYTE(x)       (JL_SPDIF->SM_ASHN = x / 4)

#define SM_SET_APNS(x)           SFR(JL_SPDIF->SM_APNS, 0, 31, x)

typedef enum {
    SPDIF_M_DAT_24BIT	= 0u,
    SPDIF_M_DAT_16BIT,
} SPDIF_M_DAT_WIDEN_MD;

typedef enum {
    AUX_FOR_CH_LR_REG	= 0u,
    AUX_FOR_AUDIO_LOW_BIT,
} SPDIF_M_AUX_DAT_SEL;

typedef enum {
    BUF_USER_REG_VAL	= 0u,
    BUF_VAL_REG_USER,
} SPDIF_M_USER_BUF_SW;


typedef struct _SPDIF_MASTER_PARM {
    u8 bit_width;             // 输出位宽
    // SPDIF_M_DAT_WIDEN_MD data_mode;
    u8 tx_io;
    u32 sr;
    u32 data_dma_len;
    u32 inf_dma_len;
    u32 *data_buf;					//dma buf地址
    u32 *test_buf;					//dma buf地址
    u32 *inf_buf;					//dma buf地址
    void (*data_isr_cb)(void *buf, u32 len);
    void (*inf_isr_cb)(void *buf, u32 len);
} SPDIF_MASTER_PARM;

struct audio_spdif_hdl;

int spdif_master_init(struct audio_spdif_hdl *hd_spdif);
int spdif_master_start(struct audio_spdif_hdl *hd_spdif);
int spdif_master_stop(struct audio_spdif_hdl *spdif);
void spdif_master_uninit();
int spdif_master_out_init(struct audio_spdif_hdl *spdif_handler, SPDIF_MASTER_PARM *spdif_master_cfg);
u8 audio_spdif_is_working(struct audio_spdif_hdl *hd_spdif);
int audio_spdif_set_sample_rate(struct audio_spdif_hdl *spdif, int sample_rate);

struct audio_spdif_sync_node {
    u8 triggered;
    u8 network;
    u32 timestamp;
    void *hdl;
    struct list_head entry;
    void *ch;
};

struct audio_spdif_channel_attr {
    u16 delay_time;         /*spdif通道延时*/
    u16 protect_time;       /*spdif延时保护时间*/
    u16 write_mode;         /*SPDIF写入模式*/
    u8  dev_properties;         /*关联蓝牙同步的主设备使能，只能有一个节点是主设备*/
} __attribute__((packed));

struct audio_spdif_channel {
    u8  state;              /*spdif状态*/
    struct audio_spdif_channel_attr attr;     /*SPDIF通道属性*/
    struct audio_spdif_hdl *hd_spdif;              /* spdif设备*/
    struct audio_cfifo_channel fifo;        /*DAC cfifo通道管理*/
};

struct audio_spdif_hdl {
    OS_SEM sem;
    u8 tx_io;
    volatile u8 mute;
    volatile  u8 state;
    // volatile u8 light_state;
    // volatile u8 agree_on;
    u8 channel;
    u32 sample_rate;
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让SPDIF真正跑之前写入的PCM点数
    u16 irq_points;
    s16 protect_time;
    s16 protect_pns;
    s16 fadein_frames;
    // s16 fade_vol;
    // u8 protect_fadein;
    // u8 vol_set_en;
    // u8 volume_enhancement;
    // u8 sound_state;
    // unsigned long sound_resume_time;
    s16 *output_buf;
    u16 output_buf_len;
    u8 *output_inf_buf;
    u16 output_inf_buf_len;

    u8 fifo_state;
    u16 unread_samples;             /*未读样点个数*/
    struct audio_cfifo fifo;        /*SPDIF cfifo结构管理*/
    struct audio_spdif_channel main_ch;

    struct list_head sync_list;
    SPDIF_M_DAT_WIDEN_MD data_mode;
    spinlock_t lock;
    OS_MUTEX mutex;
};

void audio_spdif_channel_start(void *private_data);
int audio_spdif_channel_write(void *private_data, struct audio_spdif_hdl *spdif, void *buf, int len);
void audio_spdif_channel_close(void *private_data);
int audio_spdif_new_channel(struct audio_spdif_hdl *hd_spdif, struct audio_spdif_channel *ch);
int audio_spdif_channel_set_attr(struct audio_spdif_channel *ch, struct audio_spdif_channel_attr *attr);

void audio_spdif_remove_syncts_handle(struct audio_spdif_channel *ch, void *syncts);
void audio_spdif_add_syncts_with_timestamp(struct audio_spdif_channel *ch, void *syncts, u32 timestamp);
int audio_spdif_get_sample_rate(struct audio_spdif_hdl *spdif);
int audio_spdif_fill_slience_frames(void *private_data,  int frames);
int audio_spdif_data_len(void *private_data);
void audio_spdif_syncts_trigger_with_timestamp(void *private_data, u32 timestamp);
void audio_spdif_force_use_syncts_frames(void *private_data, int frames, u32 timestamp);
void audio_spdif_add_syncts_with_timestamp(struct audio_spdif_channel *ch, void *syncts, u32 timestamp);
void audio_spdif_remove_syncts_handle(struct audio_spdif_channel *ch, void *syncts);

