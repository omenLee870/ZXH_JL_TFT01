#ifndef _AEC_DAC_CH_REF_DATA_H__
#define _AEC_DAC_CH_REF_DATA_H__

#include "generic/typedef.h"
#include "audio_dac.h"
#include "Resample_api.h"

struct aec_ref_open_param {
    u32 hw_dma_len; //硬件模块(DAC/IIS)DMA_BUF长度
    u8 bit_width; //数据位宽 1:24bit(qval 23), 0:16bit
};

int aec_ref_dac_ch_data_read_init(struct aec_ref_open_param *param);
int aec_ref_dac_ch_data_buf_len_set(u32 buf_len);
int aec_ref_dac_ch_data_read_exit();
void aec_ref_dac_ch_data_src_init(RS_PARA_STRUCT *rs_para_obj);
void aec_ref_dac_ch_data_src_close();
int aec_ref_dac_ch_data_fill(s16 *data, int len);
int aec_ref_dac_ch_data_read(u8 is_stereo_to_mono, s16 *data, int len);

void set_aec_ref_dac_ch_write_flag(u8 flag);
u8 get_aec_ref_dac_ch_write_flag();

int aec_ref_dac_ch_data_align(int len);
int aec_ref_dac_ch_data_read_goback(int len);
int aec_ref_dac_ch_data_read_updata(int len);
int aec_ref_dac_ch_data_write_updata(int len);
void aec_ref_dac_ch_data_clear(void);
int aec_ref_dac_ch_data_cbuf_size();
int aec_ref_dac_ch_data_enable();

void set_aec_ref_iis_ch_name(char name[16]);
void set_aec_ref_dac_ch_name(char name[16]);
u8 is_aec_ref_dac_ch(struct audio_dac_channel *dac_ch);
u32 aec_ref_dac_ch_data_size_get(void);
u32 aec_ref_dac_ch_data_total_size_get(void);
int aec_ref_dac_ch_data_align_same_time(int cur_dac_len, int cur_aec_ref_len);

#endif
