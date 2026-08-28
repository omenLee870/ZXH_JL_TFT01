#ifndef CVP_NODE_H
#define CVP_NODE_H

#include "audio_config.h"
#include "audio_cvp.h"
#include "effects/effects_adj.h"
#include "effects/convert_data.h"
#include "adc_file.h"

struct cvp_param_fmt {
    u8  mic_num;
    u16 source_uuid;
};

/*read node param*/
int cvp_sms_node_param_cfg_read(void *priv, u8 ignore_subid, u16 algo_uuid);	// single node
int cvp_sms_node_param_fill_attr(struct aec_s_attr *attr, u8 ignore_subid, u16 algo_uuid);
int cvp_sms_vf_node_param_cfg_read(void *priv, u8 ignore_subid, u16 algo_uuid); // single sms_vf node
int cvp_sms_vf_node_param_fill_attr(struct aec_s_attr *attr, u8 ignore_subid, u16 algo_uuid);
int cvp_dms_node_param_cfg_read(void *priv, u8 ignore_subid, u16 algo_uuid);	// dms node
int cvp_tms_node_param_cfg_read(void *priv, u8 ignore_subid, u16 algo_uuid);	// tms node
int cvp_v3_node_param_cfg_read(void *priv, u8 ignore_subid, u16 algo_uuid);		// cvp_v3 node
int cvp_dev_node_param_cfg_read(void *priv, u8 ignore_subid, u16 node_uuid);	// third node
int cvp_node_param_cfg_read(void *priv, u8 ignore_subid, u16 node_uuid);
int cvp_sms_param_cfg_read(void);
int cvp_dms_param_cfg_read(void);
int cvp_tms_param_cfg_read(void);
int cvp_v3_param_cfg_read(void);
int cvp_dev_param_cfg_read(void);
int cvp_sms_vf_param_cfg_read(void);
int cvp_param_cfg_read(void);
/*cvp node output_handle*/
int cvp_sms_node_output_handle(s16 *data, u16 len);
int cvp_sms_vf_node_output_handle(s16 *data, u16 len);
int cvp_dms_node_output_handle(s16 *data, u16 len);
int cvp_tms_node_output_handle(s16 *data, u16 len);
int cvp_v3_node_output_handle(s16 *data, u16 len);
int cvp_dev_node_output_handle(s16 *data, u16 len);
/*get mic_ch*/
u8 cvp_get_talk_mic_ch(void);
u8 cvp_v3_get_talk_mic_ch(void);
u8 cvp_dev_get_talk_mic_ch(void);
u8 cvp_node_get_talk_mic_ch(void);
u8 cvp_get_talk_ref_mic_ch(void);
u8 cvp_sms_vf_get_talk_mic_ch(void);
u8 cvp_v3_get_talk_ref_mic_ch(void);
u8 cvp_dev_get_talk_ref_mic_ch(void);
u8 cvp_node_get_talk_ref_mic_ch(void);
u8 cvp_get_talk_fb_mic_ch(void);
u8 cvp_v3_get_talk_fb_mic_ch(void);
u8 cvp_dev_get_talk_fb_mic_ch(void);
u8 cvp_node_get_talk_fb_mic_ch(void);
u8 cvp_dev_get_vpu_mic_ch(void);
u8 get_cvp_context_status(void);
int cvp_get_algo_type(void);

#define CVP_NODE_SUPPORT_32BIT  0  //cvp通话s算法节点支持位宽处理
#endif/*CVP_NODE_H*/

