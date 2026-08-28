#ifndef __CPU_DAC_H__
#define __CPU_DAC_H__


#include "generic/typedef.h"
#include "generic/atomic.h"
#include "os/os_api.h"
#include "audio_src.h"
#include "system/spinlock.h"
#include "audio_def.h"
#include "audio_general.h"

/************************************************************************************************************
  										Audio DAC Features
Notes:以下为芯片规格定义，不可修改，仅供引用
************************************************************************************************************/
#define AUDIO_DAC_CHANNEL_NUM				3	//DAC通道数
#define AUDIO_DAC_IO_ENABLE		1

//DAC Filter Type Definitions
#define DAC_FILTER_HB						(1UL << 0)
#define DAC_FILTER_MP						(1UL << 1)
#define DAC_FILTER_BALANCE					(1UL << 2)
#define DAC_FILTER_BASS						(1UL << 3)
#define DAC_FILTER_SLOW						(1UL << 4)

#define DAC_SRC_TYPE0                       (1UL << 0)
#define DAC_SRC_TYPE1                       (1UL << 1)

/************************************
             dac 供电模式
************************************/
// TCFG_DAC_POWER_LEVEL
#define	AUDIO_VCM_CAP_LEVEL1	           (0)	// VCM-cap, VCM = 1.2v
#define	AUDIO_VCM_CAP_LEVEL2	           (1)  // VCM-cap, VCM = 1.3v
#define	AUDIO_VCM_CAP_LEVEL3	           (2)	// VCM-cap, VCM = 1.4v
#define	AUDIO_VCM_CAP_LEVEL4	           (3)	// VCM-cap, VCM = 1.5v
#define	AUDIO_VCM_CAP_LEVEL5	           (4)	// VCM-cap, VCM = 1.6v
#define	AUDIO_VCM_CAPLESS_LEVEL1           (5)	// VCM-capless, DACLDO = 2.4v
#define	AUDIO_VCM_CAPLESS_LEVEL2           (6)	// VCM-capless, DACLDO = 2.6v
#define	AUDIO_VCM_CAPLESS_LEVEL3           (7)	// VCM-capless, DACLDO = 2.8v
#define	AUDIO_VCM_CAPLESS_LEVEL4           (8)	// VCM-capless, DACLDO = 3.0v


#define AUDIO_DAC_SYNC_IDLE             0
#define AUDIO_DAC_SYNC_PROBE            1
#define AUDIO_DAC_SYNC_START            2
#define AUDIO_DAC_SYNC_NO_DATA          3
#define AUDIO_DAC_ALIGN_NO_DATA         4
#define AUDIO_DAC_SYNC_ALIGN_COMPLETE   5
#define AUDIO_DAC_SYNC_KEEP_RATE_DONE   6

#define AUDIO_SRC_SYNC_ENABLE   1
#define SYNC_LOCATION_FLOAT      1
#if SYNC_LOCATION_FLOAT
#define PCM_PHASE_BIT           0
#else
#define PCM_PHASE_BIT           8
#endif

#define DA_SYNC_INPUT_BITS              20
#define DA_SYNC_MAX_NUM                 (1 << DA_SYNC_INPUT_BITS)

#define AUDIO_DAC_CLK_600M              (0)     //dac_clk: 6.00M, clk_src: std48M
#define AUDIO_DAC_CLK_666M              (1)     //dac_clk: 6.66M, clk_src: pll240M
#define AUDIO_DAC_CLK_685M              (2)     //dac_clk: 6.85M, clk_src: pll192M

#define TYPE_DAC_AGAIN      (0x01)
#define TYPE_DAC_DGAIN      (0x02)

#define DAC_TRIM_SEL_RDAC_FL            0
#define DAC_TRIM_SEL_RDAC_FR            1
#define DAC_TRIM_SEL_RDAC_RL            2
#define DAC_TRIM_SEL_PA_FL              3
#define DAC_TRIM_SEL_PA_FR              4
#define DAC_TRIM_SEL_PA_RL              5
#define DAC_TRIM_SEL_VCM                6

#define DAC_TRIM_CH_L                  0
#define DAC_TRIM_CH_R                  1

struct da2alink_param {
    u8 module_idx;
    u8 bit_width;
    u8 clk_close;
    u8 clk_mode;          //更新与采样边沿
    u8 alink_work_mode;   //alink模式配置
    u8 ch_num;
    u16 irq_points;
    u32 sr;
    u32 dma_time_ms;
};

//void audio_dac_power_state(u8 state)
//在应用层重定义 audio_dac_power_state 函数可以获取dac模拟开关的状态
struct audio_dac_hdl;
struct dac_platform_data {
    /*
         * dac_alink_dma_shared_en：
         * 使能：Alink固定输出所有来自DAC的数据
         * 不使能：需要根据audio_alink_demo.c中的例子自行调用接口开关该功能
         *
         * 使能后注意事项：
         * 1、需要在工具端打开：ALINK0配置->通道1配置使能->DAC Alink DMA Shared
         * 2、为了兼容通话16k采样率:
         * (1) 默认配置clk_close = 1，DAC退出时会关闭Alink模块，关闭时钟
         * (2) 需要确保Alink仅使能了ch1，若使能两个以上的ch，
         *     则退出时不会关模块，时钟维持，此时打开通话跑16k会出现采样率错误异常
         */
    u8 dac_alink_dma_shared_en;
    u8 performance_mode;    	// low_power/high_performance
    u8 dcc_level;				// DAC去直流滤波器档位, 0~7:关闭    8~15：开启(档位越大，高通截止点越小)
    u8 bit_width;             	// DAC输出位宽
    u8 power_mode;
    u8 clock_mode;
    u8 l_ana_gain;
    u8 r_ana_gain;
    u8 rl_ana_gain;
    u8 pa_isel0;                // 电流档位:2~6
    u8 pa_isel1;                // 电流档位:2~6
    u8 mute_delay_isel;
    u8 mute_delay_time;
    u8 dacldo_vsel;
    u8 dacldo_vcmsel;
    u8 fifo_clks;
    u8 dsm_clks;
    u8 fade_en;
    u8 fade_points;
    u8 fade_volume;
    u8 filter_type;
    u8 src_type;
    u8 fast_on;
    u16 dma_buf_time_ms;    	// DAC dma buf 大小
    float fixed_pns;        	// 固定pns,单位ms
    u32 max_sample_rate;    	//支持的最大采样率
    struct da2alink_param dac_alink[2];     //dac alink dma shared param

};

struct trim_init_param_t {
    u8 clock_mode;
    u8 power_level;
    s16 precision;
    float trim_speed;
    struct audio_dac_trim *dac_trim;
    struct audio_dac_hdl *dac;              /* DAC设备*/
};

struct audio_dac_trim {
    s16 left;
    s16 right;
};

// *INDENT-OFF*
struct audio_dac_sync {
    u32 channel : 3;
    u32 start : 1;
    u32 fast_align : 1;
    u32 connect_sync : 1;
    u32 release_by_bt : 1;
    u32 resevered : 1;
    u32 input_num : DA_SYNC_INPUT_BITS;
    int fast_points;
    int keep_points;
    int phase_sub;
    int in_rate;
    int out_rate;
    int bt_clkn;
    int bt_clkn_phase;
#if AUDIO_SRC_SYNC_ENABLE
    struct audio_src_sync_handle *src_sync;
    void *buf;
    int buf_len;
    void *filt_buf;
    int filt_len;
#else
    struct audio_src_base_handle *src_base;
#endif
#if SYNC_LOCATION_FLOAT
    float pcm_position;
#else
    u32 pcm_position;
#endif
    void *priv;
    int (*handler)(void *priv, u8 state);
    void *correct_priv;
    void (*correct_cabllback)(void *priv, int diff);
};

struct audio_dac_fade {
    u8 enable;
    volatile u8 ref_L_gain;
    volatile u8 ref_R_gain;
    int timer;
};

struct audio_dac_hdl {
    u8 analog_inited;
    u8 digital_inited;
    volatile u8 state;
    volatile u8 light_state;
    u8 gain;
    u8 channel;
    u8 protect_fadein;
    u8 vol_set_en;
    u8 anc_dac_open;
    u8 dac_read_flag;	//AEC可读取DAC参考数据的标志
	u8 frames_fade_out_disable; //设置是否关掉DAC淡出
    u8 mute_step;             	//DAC MUTE当前步骤
    u8 mute_state;            	//DAC MUTE当前状态
    u8 mute_update;           	//DAC MUTE最新状态
	u8 dvol_mute;             //DAC数字音量是否mute
    u8 clk_fre;     // 0:6M   1:6.6666M
	u8 power_on;
    u8 need_close;
    u16 mute_timer;           	//DAC PA Mute Timer
    u16 unread_samples;             /*未读样点个数*/
    u16 d_volume[4];
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让DAC真正跑之前写入的PCM点数
    u16 irq_points;
    s16 protect_time;
    s16 protect_pns;
    s16 fadein_frames;
    s16 fade_vol;
    s16 *output_buf;
    u32 sample_rate;
    u32 output_buf_len;
    OS_SEM *sem;
    OS_MUTEX mutex;
    spinlock_t lock;
    const struct dac_platform_data *pd;
	struct audio_dac_noisegate ng;
    void (*fade_handler)(u8 left_gain, u8 right_gain);
	u8 (*is_aec_ref_dac_ch)(void *dac_ch);
	void (*irq_handler_cb)(void);
};

struct audio_dac_io_param {
    /*
     *       state 通道初始状态
     * 使能单左/单右声道，初始状态为高电平：state[0] = 1
     * 使能双声道，左声道初始状态为高，右声道初始状态为低：state[0] = 1，state[1] = 0。
     */
    u8 state[3];
    /*
     *       irq_points 中断点数
     * 申请buf的大小为 buf_len = irq_points * channel_num * 4
     */
    u16 irq_points;
    /*
     * 由可视化 DAC配置->声道配置(TCFG_AUDIO_DAC_CONNECT_MODE)决定
     */
    u8 channel;
    /*
     *       digital_gain 增益
     * 影响输出电平幅值，-32768~32767可配
     */
    u16 digital_gain;
    /*
     *       ldo_volt 电压
     * 影响输出电平幅值，0~15可配
     */
    u8 ldo_volt;
};

enum DAC_MERGE_DATA {
    MERGE_DATA_NULL = 0,
    IIS_R_DATA,
    IIS_L_DATA,
    ADC_DMA_0,
    ADC_DMA_1,
};

struct audio_dac_merge_param {
    enum DAC_MERGE_DATA ch0_data0;
    enum DAC_MERGE_DATA ch0_data1;
    enum DAC_MERGE_DATA ch1_data0;
    enum DAC_MERGE_DATA ch1_data1;
    enum DAC_MERGE_DATA ch2_data0;
    enum DAC_MERGE_DATA ch2_data1;
};
void audio_dac_merge_data_open(struct audio_dac_merge_param *param);
void audio_dac_merge_data_close();
#endif

