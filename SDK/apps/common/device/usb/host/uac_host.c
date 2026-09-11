#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "generic/circular_buf.h"
#include "device/device.h"
#include "usb/host/usb_host.h"
#include "usb_ctrl_transfer.h"
#include "usb/device/uac_audio.h"
#include "uac_host.h"
#include "device_drive.h"
#include "usb_config.h"
#include "app_config.h"

#if TCFG_HOST_AUDIO_ENABLE

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#define UAC_HOST_TEST_MODE


#define UAC_SPK_BUFFER_SIZE     (4 * 1024)
#define UAC_MIC_BUFFER_SIZE     (2 * 1024)

typedef struct {
    u8 in_terminal_id;
    u8 feature_src_id;
    u8 feature_id;
    u8 select_src_id;
    u8 select_id;
    u8 out_terminal_src_id;
    u8 out_terminal_id;
    u8 channels;
} UAC_ID_INFO;


// Centralized context struct for all UAC host state
static struct {
    UAC_ID_INFO *spk_id;
    UAC_ID_INFO *mic_id;
    UAC_INFO *spk_info;
    UAC_INFO *mic_info;
    cbuffer_t spk_cbuf;
    u8 spk_buf[UAC_SPK_BUFFER_SIZE];
    cbuffer_t mic_cbuf;
    u8 mic_buf[UAC_MIC_BUFFER_SIZE];
    u32 fill_pkt_len;
    u32 fill_pkt_cnt;
    u8 *ep_in_dma_buf;
    u8 *ep_out_dma_buf;
} uac = {0};

static int _uac_control_parser(const u8 *pBuf)
{
    log_debug("func:_uac_control_parser, line:%d\n", __LINE__);
    uac.spk_id = (UAC_ID_INFO *)malloc(sizeof(UAC_ID_INFO));
    uac.mic_id = (UAC_ID_INFO *)malloc(sizeof(UAC_ID_INFO));
    struct usb_interface_descriptor *intr_desc = (struct usb_interface_descriptor *)pBuf;
    u8 interface_num = intr_desc->bInterfaceNumber;
    u32 total_len = 0, len = 0;
    pBuf += pBuf[0]; //HEADER 解析开始
    struct uac1_ac_header_descriptor *head_desc = (struct uac1_ac_header_descriptor *)pBuf;
    if (head_desc->bDescriptorSubtype != UAC_HEADER) {
        log_warn("uac_header->bDescriptorSubtype = 0x%x error!\n", head_desc->bDescriptorSubtype);
        return -1;
    } else {
        total_len = head_desc->wTotalLength;
        log_info("UAC_HEADER\n");
        log_debug_hexdump((u8 *)pBuf, pBuf[0]);
    }
    len += pBuf[0]; //解析结束

    while (len < total_len) {
        pBuf += pBuf[0]; //解析开始
        if (pBuf[1] != USB_DT_CS_INTERFACE) {
            log_warn("uac->bDescriptorType = 0x%x error!\n", pBuf[1]);
            break;
        }
        switch (pBuf[2]) {
        case UAC_INPUT_TERMINAL:
            log_info("UAC_INPUT_TERMINAL\n");
            log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            struct uac_input_terminal_descriptor *in_terminal_desc = (struct uac_input_terminal_descriptor *)pBuf;
            if (in_terminal_desc->wTerminalType == UAC_TERMINAL_STREAMING) {
                uac.spk_id->in_terminal_id = in_terminal_desc->bTerminalID;
                uac.spk_id->channels = in_terminal_desc->bNrChannels;
                uac.spk_info->ctrl_intr_num = interface_num;
            } else {
                uac.mic_id->in_terminal_id = in_terminal_desc->bTerminalID;
                uac.mic_id->channels = in_terminal_desc->bNrChannels;
                uac.mic_info->ctrl_intr_num = interface_num;
            }
            break;
        case UAC_FEATURE_UNIT:
            log_info("UAC_FEATURE_UNIT\n");
            log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            struct uac_feature_unit_descriptor *feature_desc = (struct uac_feature_unit_descriptor *)pBuf;
            if (feature_desc->bSourceID == uac.spk_id->in_terminal_id) {
                uac.spk_id->feature_id = feature_desc->bUnitID;
                uac.spk_id->feature_src_id = feature_desc->bSourceID;
                uac.spk_info->feature_id = feature_desc->bUnitID;
                u8 control_size = feature_desc->bControlSize;
                for (u8 i = 0; i < uac.spk_id->channels + 1; i++) {
                    uac.spk_info->controls[i] = feature_desc->bmaControls[i * control_size];
                }
            } else {
                uac.mic_id->feature_id = feature_desc->bUnitID;
                uac.mic_id->feature_src_id = feature_desc->bSourceID;
                uac.mic_info->feature_id = feature_desc->bUnitID;
                u8 control_size = feature_desc->bControlSize;
                for (u8 i = 0; i < uac.mic_id->channels + 1; i++) {
                    uac.mic_info->controls[i] = feature_desc->bmaControls[i * control_size];
                }
            }
            break;
        case UAC_SELECTOR_UNIT:
            log_info("UAC_SELECTOR_UNIT\n");
            log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            struct uac_selector_unit_descriptor *select_desc = (struct uac_selector_unit_descriptor *)pBuf;
            if (select_desc->baSourceID[0] == uac.spk_id->feature_id) {
                uac.spk_id->select_id = select_desc->bUnitID;
                uac.spk_id->select_src_id = select_desc->baSourceID[0];
            } else {
                uac.mic_id->select_id = select_desc->bUnitID;
                uac.mic_id->select_src_id = select_desc->baSourceID[0];
            }
            break;
        case UAC_OUTPUT_TERMINAL:
            log_info("UAC_OUTPUT_TERMINAL\n");
            log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            struct uac1_output_terminal_descriptor *out_terminal_desc = (struct uac1_output_terminal_descriptor *)pBuf;
            if (out_terminal_desc->wTerminalType == UAC_TERMINAL_STREAMING) {
                uac.mic_id->out_terminal_id = out_terminal_desc->bTerminalID;
                uac.mic_id->out_terminal_src_id = out_terminal_desc->bSourceID;
            } else {
                uac.spk_id->out_terminal_id = out_terminal_desc->bTerminalID;
                uac.spk_id->out_terminal_src_id = out_terminal_desc->bSourceID;
            }
            break;
        default:
            log_warn("uac->bDescriptorSubtype = 0x%x error!\n", head_desc->bDescriptorSubtype);
            break;
        }
        len += pBuf[0]; //解析结束
    }


    /* if (len == total_len) { */
    /*     len += USB_DT_INTERFACE_SIZE; //加上audio_control的interface描述符长度 */
    /*     log_debug("uac_control_desc parse success.\n"); */
    /* } else { */
    /*     log_error("uac_control_desc parse fail!!! len:%d != tolen:%d\n", len, total_len); */
    /*     len = -1; */
    /* } */
    len += USB_DT_INTERFACE_SIZE; //加上audio_control的interface描述符长度

    log_debug_hexdump((u8 *)uac.spk_id, sizeof(UAC_ID_INFO));
    log_debug_hexdump((u8 *)uac.mic_id, sizeof(UAC_ID_INFO));
    return (int)len;
}
static int _uac_streaming_parser(const u8 *pBuf)
{
    log_debug("func:_uac_streaming_parser, line:%d\n", __LINE__);
    struct usb_interface_descriptor *intr_desc = (struct usb_interface_descriptor *)pBuf;
    u32 len = 0;
    if ((intr_desc->bAlternateSetting == 0) && (intr_desc->bNumEndpoints == 0)) {
        log_info("interface: %d, AlternateSetting: %d, NumEndpoints: %d\n",
                 intr_desc->bInterfaceNumber, intr_desc->bAlternateSetting, intr_desc->bNumEndpoints);
        len = pBuf[0];
        pBuf += pBuf[0];
    }

    UAC_INFO *uac_info = NULL;
    intr_desc = (struct usb_interface_descriptor *)pBuf;
    u8 interface_num = intr_desc->bInterfaceNumber;
    u8 altersetting = intr_desc->bAlternateSetting;
    u8 parse_end = 1;
    while (parse_end) {
        len += pBuf[0];
        pBuf += pBuf[0];
        switch (pBuf[1]) {
        case USB_DT_CS_INTERFACE:
            if (pBuf[2] == UAC_AS_GENERAL) {
                log_info("UAC_AS_GENERAL\n");
                log_debug_hexdump((u8 *)pBuf, pBuf[0]);
                struct uac1_as_header_descriptor *as_general_desc = (struct uac1_as_header_descriptor *)pBuf;
                if (as_general_desc->bTerminalLink == uac.spk_id->in_terminal_id) {
                    uac_info = uac.spk_info;
                } else if (as_general_desc->bTerminalLink == uac.mic_id->out_terminal_id) {
                    uac_info = uac.mic_info;
                }
                uac_info->intr_num = interface_num;
                uac_info->as_cfg[altersetting].alterset = altersetting;
            } else if (pBuf[2] == UAC_FORMAT_TYPE) {
                log_info("UAC_FORMAT_TYPE\n");
                log_debug_hexdump((u8 *)pBuf, pBuf[0]);
                struct uac_format_type_i_discrete_descriptor *format_type_desc = (struct uac_format_type_i_discrete_descriptor *)pBuf;
                uac_info->as_cfg[altersetting].ch = format_type_desc->bNrChannels;
                uac_info->as_cfg[altersetting].bit_res = format_type_desc->bBitResolution;
                for (u8 i = 0; i < format_type_desc->bSamFreqType; i++) {
                    uac_info->as_cfg[altersetting].sample_freq[i] =
                        (u32)(format_type_desc->tSamFreq[i][0] |
                              format_type_desc->tSamFreq[i][1] << 8 |
                              format_type_desc->tSamFreq[i][2] << 16);
                }
            } else {
                log_warn("uac->bDescriptorSubtype = 0x%x error!\n", pBuf[2]);
                log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            }
            break;
        case USB_DT_ENDPOINT:
            log_info("USB_DT_ENDPOINT\n");
            log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            struct usb_endpoint_descriptor *endpoint_desc = (struct usb_endpoint_descriptor *)pBuf;
            /* UAC_ENDPOINT_DESC *endpoint_desc = (UAC_ENDPOINT_DESC *)pBuf; */
            uac_info->as_cfg[altersetting].target_ep = endpoint_desc->bEndpointAddress;
            uac_info->as_cfg[altersetting].max_packet_size = (u32)endpoint_desc->wMaxPacketSize & 0x3fff;
            uac_info->as_cfg[altersetting].interval = endpoint_desc->bInterval;
            break;
        case USB_DT_CS_ENDPOINT:
            log_info("USB_DT_CS_ENDPOINT\n");
            log_debug_hexdump((u8 *)pBuf, pBuf[0]);
            uac_info->as_cfg[altersetting].ep_attr = pBuf[3];
            break;
        case USB_DT_INTERFACE:
            intr_desc = (struct usb_interface_descriptor *)pBuf;
            if (intr_desc->bInterfaceNumber == interface_num) {
                parse_end = 1;
                altersetting = intr_desc->bAlternateSetting;
            } else {
                parse_end = 0;
            }
            break;
        default:
            //如果audio streaming描述符后面没有其他描述符（例如HID），整个
            //usb描述符已经结束，需要退出while()循环
            parse_end = 0;
            break;
        }
    }
    return len;
}
static int set_power(struct usb_host_device *host_dev, u32 value)
{
    return DEV_ERR_NONE;
}
static int get_power(struct usb_host_device *host_dev, u32 value)
{
    return DEV_ERR_NONE;
}
static const struct interface_ctrl uac_ctrl = {
    .interface_class = USB_CLASS_AUDIO,
    .set_power = set_power,
    .get_power = get_power,
    .ioctl = NULL,
};
static const struct usb_interface_info usb_info = {
    .ctrl = (struct interface_ctrl *) &uac_ctrl,
    .dev = NULL,
};
int usb_audio_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_debug("func:usb_audio_parser, line:%d\n", __LINE__);
    struct usb_interface_descriptor *intr_desc = (struct usb_interface_descriptor *)pBuf;
    int desc_len = -1;
    switch (intr_desc->bInterfaceSubClass) {
    case USB_SUBCLASS_AUDIOCONTROL:
        desc_len = _uac_control_parser(pBuf);
        break;
    case USB_SUBCLASS_AUDIOSTREAMING:
        desc_len = _uac_streaming_parser(pBuf);
        break;
    case USB_SUBCLASS_MIDISTREAMING:
        break;
    default:
        log_warn("uac_interface->bInterfaceSubClass = 0x%x error!\n", intr_desc->bInterfaceSubClass);
        break;
    }
    host_dev->interface_info[interface_num] = &usb_info;
    return desc_len;
}

void uac_host_info_dump()
{
    log_info("func:uac_host_info_dump, line:%d\n", __LINE__);

    log_info("uac.spk_info\n");
    log_info("|   intr_num:%d\n", uac.spk_info->intr_num);
    log_info("|   feature_id:%d\n", uac.spk_info->feature_id);
    log_info("|   controls[0]:0x%x, controls[1]:0x%x, controls[2]:0x%x\n", uac.spk_info->controls[0], uac.spk_info->controls[1], uac.spk_info->controls[2]);
    log_info("|   host_ep:0x%x\n", uac.spk_info->host_ep);
    for (u8 i = 1; i < UAC_MAX_CFG; i++) {
        if (uac.spk_info->as_cfg[i].ch == 0) {
            continue;
        }
        log_info("|       alterseting %d\n", i);
        log_info("|   ch:%d\n", uac.spk_info->as_cfg[i].ch);
        log_info("|   bit_res:%d\n", uac.spk_info->as_cfg[i].bit_res);
        log_info("|   target_ep:0x%x\n", uac.spk_info->as_cfg[i].target_ep);
        log_info("|   max_packet_size:%d\n", uac.spk_info->as_cfg[i].max_packet_size);
        for (u8 j = 0; j < 6; j++) {
            if (uac.spk_info->as_cfg[i].sample_freq[j]) {
                log_info("|   sample_freq[%d]:%d\n", j, uac.spk_info->as_cfg[i].sample_freq[j]);
            }
        }
    }
    log_info("uac.mic_info\n");
    log_info("|   intr_num:%d\n", uac.mic_info->intr_num);
    log_info("|   feature_id:%d\n", uac.mic_info->feature_id);
    log_info("|   controls[0]:0x%x, controls[1]:0x%x, controls[2]:0x%x\n", uac.mic_info->controls[0], uac.mic_info->controls[1], uac.mic_info->controls[2]);
    log_info("|   host_ep:0x%x\n", uac.mic_info->host_ep);
    for (u8 i = 1; i < UAC_MAX_CFG; i++) {
        if (uac.mic_info->as_cfg[i].ch == 0) {
            continue;
        }
        log_info("|       alterseting %d\n", i);
        log_info("|   ch:%d\n", uac.mic_info->as_cfg[i].ch);
        log_info("|   bit_res:%d\n", uac.mic_info->as_cfg[i].bit_res);
        log_info("|   target_ep:0x%x\n", uac.mic_info->as_cfg[i].target_ep);
        log_info("|   max_packet_size:%d\n", uac.mic_info->as_cfg[i].max_packet_size);
        for (u8 j = 0; j < 6; j++) {
            if (uac.mic_info->as_cfg[i].sample_freq[j]) {
                log_info("|   sample_freq[%d]:%d\n", j, uac.mic_info->as_cfg[i].sample_freq[j]);
            }
        }
    }
}

#ifdef UAC_HOST_TEST_MODE
static const s16 sin_48k[] = {
    0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
    14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
    14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
    0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
    -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
    -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
};
s16 test_buf[96];
static void uac_host_test_func()
{
    memcpy(&test_buf[0], sin_48k, sizeof(sin_48k));
    memcpy(&test_buf[48], sin_48k, sizeof(sin_48k));
    cbuf_write(&uac.spk_cbuf, test_buf, sizeof(test_buf));
    cbuf_write(&uac.spk_cbuf, test_buf, sizeof(test_buf));
}
static void spk_isr_hanlder(void *priv)
{
    //putchar('s');
}
static void mic_isr_hanlder(void *priv)
{
    //putchar('m');
}
#endif

static void uac_host_query_infomation(u32 usb_id)
{
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);
    typedef struct  {
        u16 cur;
        u16 min;
        u16 max;
        u16 res;
        u8  mute;
    } UAC_VOLUME;

    UAC_VOLUME spk_vol[3] = {0};
    UAC_VOLUME mic_vol[3] = {0};



    if (uac.spk_info && uac.spk_info->intr_num) {
        for (int i = 0; i < 3; i++) {
            if (uac.spk_info->controls[i] & BIT(0)) {
                usb_control_msg(host_dev,
                                UAC_GET_CUR,
                                0xa1,
                                (0x01 << 8) | i,
                                (uac.spk_info->feature_id << 8) | uac.spk_info->ctrl_intr_num,
                                &spk_vol[i].mute,
                                1);
            }

            if (uac.spk_info->controls[i] & BIT(1)) {
                usb_control_msg(host_dev,
                                UAC_GET_CUR,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.spk_info->feature_id << 8) | uac.spk_info->ctrl_intr_num,
                                &spk_vol[i].cur,
                                2);
                usb_control_msg(host_dev,
                                UAC_GET_MIN,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.spk_info->feature_id << 8) | uac.spk_info->ctrl_intr_num,
                                &spk_vol[i].min,
                                2);
                usb_control_msg(host_dev,
                                UAC_GET_MAX,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.spk_info->feature_id << 8) | uac.spk_info->ctrl_intr_num,
                                &spk_vol[i].max,
                                2);
                usb_control_msg(host_dev,
                                UAC_GET_RES,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.spk_info->feature_id << 8) | uac.spk_info->ctrl_intr_num,
                                &spk_vol[i].res,
                                2);

                log_info("spk vol[%d] cur %04x, min %04x, max %04x, res %04x",
                         i,
                         spk_vol[i].cur,
                         spk_vol[i].min,
                         spk_vol[i].max,
                         spk_vol[i].res);
            }
        }
        usb_set_interface(host_dev, uac.spk_info->intr_num, 0);
    }
    if (uac.mic_info && uac.mic_info->intr_num) {
        for (int i = 0; i < 3; i++) {
            if (uac.mic_info->controls[i] & BIT(0)) {
                usb_control_msg(host_dev,
                                UAC_GET_CUR,
                                0xa1,
                                (0x01 << 8) | i,
                                (uac.mic_info->feature_id << 8) | uac.mic_info->ctrl_intr_num,
                                &mic_vol[i].mute,
                                1);
            }

            if (uac.mic_info->controls[i] & BIT(1)) {
                usb_control_msg(host_dev,
                                UAC_GET_CUR,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.mic_info->feature_id << 8) | uac.mic_info->ctrl_intr_num,
                                &mic_vol[i].cur,
                                2);
                usb_control_msg(host_dev,
                                UAC_GET_MIN,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.mic_info->feature_id << 8) | uac.mic_info->ctrl_intr_num,
                                &mic_vol[i].min,
                                2);
                usb_control_msg(host_dev,
                                UAC_GET_MAX,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.mic_info->feature_id << 8) | uac.mic_info->ctrl_intr_num,
                                &mic_vol[i].max,
                                2);
                usb_control_msg(host_dev,
                                UAC_GET_RES,
                                0xa1,
                                (0x02 << 8) | i,
                                (uac.mic_info->feature_id << 8) | uac.mic_info->ctrl_intr_num,
                                &mic_vol[i].res,
                                2);
                log_info("mic vol[%d] cur %04x, min %04x, max %04x, res %04x",
                         i,
                         mic_vol[i].cur,
                         mic_vol[i].min,
                         mic_vol[i].max,
                         mic_vol[i].res);
            }
        }
        usb_set_interface(host_dev, uac.mic_info->intr_num, 0);
    }
}

void uac_host_mount(u32 usb_id)
{
    log_info("func:uac_host_mount, line:%d\n", __LINE__);
    free(uac.spk_id);//不需要用到这部分内存，释放掉
    free(uac.mic_id);//不需要用到这部分内存，释放掉
    uac_host_query_infomation(usb_id);
    uac_host_info_dump();
#ifdef UAC_HOST_TEST_MODE
    /* set handlers BEFORE open so isr_hanlder is valid when uac_tx_isr fires */
    uac_host_set_spk_hanlder(spk_isr_hanlder, NULL);
    uac_host_set_mic_hanlder(mic_isr_hanlder, NULL);

    u32 ret;
    ret = uac_host_open_spk(usb_id, 2, 48000, 16);
    if (ret != DEV_ERR_NONE) {
        uac_host_open_spk(usb_id, 1, 48000, 16);
    }
    ret = uac_host_open_mic(usb_id, 1, 48000, 16);
    if (ret != DEV_ERR_NONE) {
        uac_host_open_mic(usb_id, 2, 48000, 16);
    }
    /* usr_timer_add(NULL, uac_host_test_func, 1, 1); */
#endif
}

void uac_host_unmount(u32 usb_id)
{
    log_info("func:uac_host_unmount, line:%d\n", __LINE__);
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);
    if (host_dev->private_data.status == 0) {
        log_info("host already unmounted, skip uac cleanup");
        return;
    }
#ifdef UAC_HOST_TEST_MODE
    uac_host_close_spk(usb_id);
    uac_host_close_mic(usb_id);
#endif
}

//static void uac_host_event_handler(struct sys_event *event)
//{
//    log_info("func:uac_host_unmount, line:%d\n", __LINE__);
//    const char *audio = (const char *)event->u.dev.value;
//    switch (event->type) {
//    case SYS_DEVICE_EVENT:
//        if ((u32)event->arg == DEVICE_EVENT_FROM_USB_HOST) {
//            if (strncmp(audio, "audio", 5) == 0) {
//                if ((event->u.dev.event == DEVICE_EVENT_IN) ||
//                    (event->u.dev.event == DEVICE_EVENT_CHANGE)) {
//                    log_debug("uac_host IN %s", event->u.dev.value);
//                    uac_host_mount(audio[5] - '0');
//                    audio_usb_host_init(audio);
//                } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
//                    log_debug("uac_host OUT %s", event->u.dev.value);
//                    uac_host_unmount(audio[5] - '0');
//                    audio_usb_host_release();
//                }
//            }
//            break;
//        }
//    }
//}

void uac_host_init()
{
    log_info("func:uac_host_init, line:%d\n", __LINE__);
    uac.spk_info = (UAC_INFO *)malloc(sizeof(UAC_INFO));
    uac.mic_info = (UAC_INFO *)malloc(sizeof(UAC_INFO));
    memset(uac.spk_info, 0, sizeof(UAC_INFO));
    memset(uac.mic_info, 0, sizeof(UAC_INFO));
    cbuf_init(&uac.spk_cbuf, uac.spk_buf, UAC_SPK_BUFFER_SIZE);
    cbuf_init(&uac.mic_cbuf, uac.mic_buf, UAC_MIC_BUFFER_SIZE);
    //register_sys_event_handler(SYS_DEVICE_EVENT, DEVICE_EVENT_FROM_USB_HOST, 2,
    //                           uac_host_event_handler);
}

void uac_host_deinit()
{
    log_info("func:uac_host_deinit, line:%d\n", __LINE__);
    free(uac.spk_info);
    uac.spk_info = NULL;
    free(uac.mic_info);
    uac.mic_info = NULL;
    //unregister_sys_event_handler(uac_host_event_handler);
}

static u32 vol_convert(u16 v)
{
    const u16 vol_table[] = {
        0xe3a0, 0xe461, 0xe519, 0xe5c8, 0xe670, 0xe711, 0xe7ac, 0xe840, 0xe8cf, 0xe959,
        0xe9df, 0xea60, 0xeadc, 0xeb55, 0xebca, 0xec3c, 0xecab, 0xed16, 0xed7f, 0xede5,
        0xee48, 0xeea9, 0xef08, 0xef64, 0xefbe, 0xf016, 0xf06c, 0xf0c0, 0xf113, 0xf164,
        0xf1b3, 0xf200, 0xf24c, 0xf297, 0xf2e0, 0xf328, 0xf36e, 0xf3b4, 0xf3f8, 0xf43a,
        0xf47c, 0xf4bd, 0xf4fc, 0xf53b, 0xf579, 0xf5b5, 0xf5f1, 0xf62c, 0xf666, 0xf69f,
        0xf6d7, 0xf70e, 0xf745, 0xf77b, 0xf7b0, 0xf7e5, 0xf818, 0xf84b, 0xf87e, 0xf8b0,
        0xf8e1, 0xf911, 0xf941, 0xf970, 0xf99f, 0xf9cd, 0xf9fb, 0xfa28, 0xfa55, 0xfa81,
        0xfaad, 0xfad8, 0xfb03, 0xfb2d, 0xfb56, 0xfb80, 0xfba9, 0xfbd1, 0xfbf9, 0xfc21,
        0xfc48, 0xfc6f, 0xfc95, 0xfcbb, 0xfce1, 0xfd06, 0xfd2b, 0xfd50, 0xfd74, 0xfd98,
        0xfdbc, 0xfddf, 0xfe02, 0xfe25, 0xfe47, 0xfe69, 0xfe8b, 0xfead, 0xfece, 0xfeef,
        0xff0f,
    };

    if (v <= 100) {
        return vol_table[v];
    }

    for (int i = 0; i < sizeof(vol_table) / 2; i++) {
        if (v <= vol_table[i]) {
            return i;
        }
    }

    return 0;
}

static int uac_set_volume(struct usb_host_device *host_dev, u8 intr_num, u8 feature_id, u16 volume)
{
    UAC_INFO *uac_info = NULL;
    if (feature_id == 0) {
        log_warn("uac Volume adjustment is not supported\n");
        return -1;
    }
    if (feature_id == uac.spk_info->feature_id) {
        uac_info = uac.spk_info;
    } else if (feature_id == uac.mic_info->feature_id) {
        uac_info = uac.mic_info;
    } else {
        return -1;
    }
    if (uac_info->controls[0] & BIT(1)) {
        usb_audio_volume_control(host_dev, intr_num, feature_id, 0, volume);
    }
    if (uac_info->controls[1] & BIT(1)) {
        usb_audio_volume_control(host_dev, intr_num, feature_id, 1, volume);
    }
    if (uac_info->controls[2] & BIT(1)) {
        usb_audio_volume_control(host_dev, intr_num, feature_id, 2, volume);
    }
    return 0;
}



static void uac_tx_isr(struct usb_host_device *priv, u32 ep)
{
    struct audio_streaming_config *as_cfg = &uac.spk_info->as_cfg[uac.spk_info->cur_alterseting];
    u32 spk_frame_len = as_cfg->cur_sample_freq * (as_cfg->bit_res / 8) * as_cfg->ch / 1000;
    u8 buf[288];
    if (uac.fill_pkt_len) {
        uac.fill_pkt_cnt++;
        if (uac.fill_pkt_cnt == 10) {
            spk_frame_len += uac.fill_pkt_len;
            uac.fill_pkt_cnt = 0;
        }
    }
    if (spk_frame_len > sizeof(buf)) {
        spk_frame_len = sizeof(buf);
    }
    u32 rlen = cbuf_read(&uac.spk_cbuf, buf, spk_frame_len);

    static u8 need_fade_in_flag = 0;
    static u32 need_fade_in_cnt = 0;
    if (rlen != spk_frame_len) { //读不到足够的长度的数据，补0
        need_fade_in_cnt++;
        /* putchar('r'); */
        memset(buf, 0x00, spk_frame_len);
        rlen = spk_frame_len;
    } else {
        need_fade_in_flag = 0;
        need_fade_in_cnt = 0;
    }
    if (need_fade_in_cnt >= 10 && need_fade_in_flag == 0) {
        need_fade_in_flag = 1;
        //TODO
        /* audio_host_spk_fade_in(); */
    }
    usb_h_ep_write_async(0, ep, as_cfg->max_packet_size, as_cfg->target_ep, buf, rlen, USB_ENDPOINT_XFER_ISOC, !rlen);

#ifdef UAC_HOST_TEST_MODE
    if (uac.spk_info->isr_hanlder) {
        uac.spk_info->isr_hanlder(uac.spk_info->priv);
    }
#endif
}

u32 uac_host_open_spk(usb_dev usb_id, u32 ch, u32 sample_freq, u32 bit_res)
{
    if (uac.spk_info == NULL) {
        log_warn("func:uac_host_open_spk, line:%d, uac.spk_info not init\n", __LINE__);
        return -1;
    }
    /* 复位当前配置, 避免多次调用(例如回退重试不同位宽)时复用上一次的残留状态 */
    spk_info->cur_alterseting = 0;
    for (u8 alt = 1; alt < UAC_MAX_CFG; alt++) {
        if ((uac.spk_info->as_cfg[alt].ch == ch) && (uac.spk_info->as_cfg[alt].bit_res == bit_res)) {
            for (u8 i = 0; i < 6; i++) {
                if (uac.spk_info->as_cfg[alt].sample_freq[i] == sample_freq) {
                    log_info("find config\n");
                    uac.spk_info->cur_alterseting = alt;
                    uac.spk_info->as_cfg[alt].cur_sample_freq = sample_freq;
                    break;
                }
            }
        }
        if (spk_info->cur_alterseting != 0) {
            break;
        }
    }
    if (uac.spk_info->cur_alterseting == 0) {
        log_warn("func:uac_host_open_spk, line:%d\n", __LINE__);
        log_warn("can not find Alternatesetting,please check bit_reso and sample_rate\n");
        return DEV_ERR_CONTROL;
    }
    u32 host_ep = usb_get_ep_num(usb_id, USB_DIR_OUT, USB_ENDPOINT_XFER_ISOC);
    ASSERT(host_ep != -1, "ep not enough");
    uac.spk_info->host_ep = (u8)host_ep;

    struct audio_streaming_config *as_cfg = &uac.spk_info->as_cfg[uac.spk_info->cur_alterseting];
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);

    u32 spk_frame_len;
    spk_frame_len = as_cfg->cur_sample_freq * (as_cfg->bit_res / 8) * as_cfg->ch / 1000;
    spk_frame_len += (as_cfg->cur_sample_freq % 1000) ? (as_cfg->bit_res / 8) * as_cfg->ch : 0;
    uac.fill_pkt_cnt = 0;
    uac.fill_pkt_len = (as_cfg->cur_sample_freq % 1000) ? (as_cfg->bit_res / 8) * as_cfg->ch : 0;

    if (uac.ep_out_dma_buf == NULL) {
        uac.ep_out_dma_buf = usb_h_alloc_ep_buffer(usb_id, host_ep, spk_frame_len);
    }

    usb_set_interface(host_dev, uac.spk_info->intr_num, uac.spk_info->cur_alterseting); //interface   Alternatesetting
    if (as_cfg->ep_attr & BIT(0)) {
        usb_audio_sampling_frequency_control(host_dev, as_cfg->target_ep, sample_freq);//设置采样率
    }
    //TODO
    //设置音量
    //uac_set_volume(host_dev, uac.spk_info->ctrl_intr_num, uac.spk_info->feature_id, vol_convert(100));

    usb_h_set_ep_isr(host_dev, host_ep, uac_tx_isr, NULL);
#ifdef USB_HW_20
#if USB_HUB
    usb_hub_txreg_set(usb_id, host_ep, as_cfg->target_ep, &(host_dev->private_data.hub_info));
#else
    usb_write_txfuncaddr(usb_id, host_ep, host_dev->private_data.devnum);
#endif
#endif
    usb_h_ep_config(usb_id, host_ep, USB_ENDPOINT_XFER_ISOC, 1, as_cfg->interval, uac.ep_out_dma_buf, spk_frame_len);
    uac_tx_isr(NULL, host_ep);
    return DEV_ERR_NONE;
}
u32 uac_host_close_spk(usb_dev usb_id)
{
    if (uac.spk_info == NULL) {
        log_warn("func:uac_host_close_spk, line:%d, uac.spk_info not init\n", __LINE__);
        return -1;
    }
    if (uac.spk_info->cur_alterseting == 0) {
        log_warn("func:uac_host_close_spk, line:%d\n", __LINE__);
        return -1;
    }
    uac.spk_info->cur_alterseting = 0;
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);
    usb_h_set_ep_isr(host_dev, uac.spk_info->host_ep, NULL, NULL);
    usb_free_ep_num(usb_id, uac.spk_info->host_ep);
    usb_set_interface(host_dev, uac.spk_info->intr_num, uac.spk_info->cur_alterseting); //interface   Alternatesetting
    if (uac.ep_out_dma_buf) {
        usb_h_free_ep_buffer(usb_id, uac.ep_out_dma_buf);
        uac.ep_out_dma_buf = NULL;
    }
    return DEV_ERR_NONE;
}
int uac_host_set_spk_volume(usb_dev usb_id, u16 vol)
{
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);
    if (!host_dev) {
        return -1;
    }
    if (!(uac.spk_info && uac.spk_info->intr_num)) {
        return -1;
    }
    //vol的范围：0 ~ 100
    //设置音量，函数里面会根据uac耳机左右声道使能给每个声道发音量
    return uac_set_volume(host_dev, uac.spk_info->ctrl_intr_num, uac.spk_info->feature_id, vol_convert(vol));
}
u32 uac_host_write_spk_cbuf(u8 *buf, u32 len)
{
    if (uac.spk_info == NULL) {
        log_warn("func:uac_host_write_spk_cbuf, line:%d, uac.spk_info not init\n", __LINE__);
        return -1;
    }
    return cbuf_write(&uac.spk_cbuf, buf, len);
}
u32 uac_host_get_spk_cbuf_len() //获取spk_cbuf当前长度
{
    u32 cur_len = cbuf_get_data_size(&uac.spk_cbuf);
    return cur_len;
}
u32 uac_host_get_spk_cbuf_size() //获取spk_cbuf总长度
{
    return UAC_SPK_BUFFER_SIZE;
}
u32 uac_host_query_spk_info(u32 ch, u32 sample_freq, u32 bit_res) //查询spk是否支持该配置 0:支持, 1:不支持
{
    struct audio_streaming_config *as_cfg;
    for (u8 alt = 1; alt < UAC_MAX_CFG; alt++) {
        as_cfg = &uac.spk_info->as_cfg[alt];
        if (as_cfg->ch == ch && as_cfg->bit_res == bit_res) {
            for (u8 i = 0; i < 6; i++) {
                if (as_cfg->sample_freq[i] == sample_freq) {
                    return 0;
                } else {
                    continue;
                }
            }
        } else {
            continue;
        }
    }
    return 1;
}
u32 uac_host_set_spk_hanlder(void (*isr_hanlder)(void *priv), void *priv) //设置spk回调函数
{
    if (uac.spk_info == NULL) {
        log_warn("func:uac_host_write_spk_cbuf, line:%d, uac.spk_info not init\n", __LINE__);
        return -1;
    }
    uac.spk_info->isr_hanlder = isr_hanlder;
    uac.spk_info->priv = priv;
    return 0;
}

static void uac_rx_isr(struct usb_host_device *priv, u32 ep)
{
    struct audio_streaming_config *as_cfg = &uac.mic_info->as_cfg[uac.mic_info->cur_alterseting];
    u32 mic_frame_len;
    u8 buf[288];
    u32 rlen = 0;
    u32 wlen = 0;

    mic_frame_len = as_cfg->cur_sample_freq * (as_cfg->bit_res / 8) * as_cfg->ch / 1000;
    mic_frame_len += (as_cfg->cur_sample_freq % 1000) ? (as_cfg->bit_res / 8) * as_cfg->ch : 0;
    if (mic_frame_len > sizeof(buf)) {
        mic_frame_len = sizeof(buf);
    }
    rlen = usb_h_ep_read_async(0, ep, as_cfg->target_ep, buf, mic_frame_len, USB_ENDPOINT_XFER_ISOC, 0);
    if (rlen <= 0) {
#ifdef UAC_HOST_TEST_MODE
        /* memset(buf, 0x00, sizeof(buf)); */
        /* rlen = mic_frame_len; */
#endif
        goto __kstart;
    }
#ifdef UAC_HOST_TEST_MODE
    if (as_cfg->bit_res == 16) {
        if (as_cfg->ch == 1) {
            /* memcpy(buf, sin_48k, mic_frame_len); */
            /* wlen = cbuf_write(&uac.spk_cbuf, buf, mic_frame_len); */
            /* wlen = cbuf_write(&uac.spk_cbuf, buf, mic_frame_len); */
            s16 buf_2[384];
            for (u32 i = 0; i < rlen / 2; i++) {
                buf_2[i * 2] = ((s16 *)buf)[i];
                buf_2[i * 2 + 1] = ((s16 *)buf)[i];
            }
            rlen *= 2;
            wlen = cbuf_write(&uac.spk_cbuf, buf_2, rlen);
        } else if (as_cfg->ch == 2) {
            wlen = cbuf_write(&uac.spk_cbuf, buf, rlen);
        }
    }
#else
    wlen = cbuf_write(&uac.mic_cbuf, buf, rlen);
#endif
    if (wlen != rlen) {
        /* putchar('w'); */
    } else {
        if (uac.mic_info->isr_hanlder) {
            uac.mic_info->isr_hanlder(uac.mic_info->priv);
        }
    }

__kstart:
    usb_h_ep_read_async(0, ep, as_cfg->target_ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1); //触发下一个接收中断
}
u32 uac_host_open_mic(usb_dev usb_id, u32 ch, u32 sample_freq, u32 bit_res)
{
    if (uac.mic_info == NULL) {
        log_warn("func:uac_host_open_mic, line:%d, uac.mic_info not init\n", __LINE__);
        return -1;
    }
    for (u8 alt = 1; alt < UAC_MAX_CFG; alt++) {
        if ((uac.mic_info->as_cfg[alt].ch == ch) && (uac.mic_info->as_cfg[alt].bit_res == bit_res)) {
            for (u8 i = 0; i < 6; i++) {
                if (uac.mic_info->as_cfg[alt].sample_freq[i] == sample_freq) {
                    log_info("find config\n");
                    uac.mic_info->cur_alterseting = alt;
                    uac.mic_info->as_cfg[alt].cur_sample_freq = sample_freq;
                    break;
                }
            }
        }
    }
    if (uac.mic_info->cur_alterseting == 0) {
        log_warn("func:uac_host_open_mic, line:%d\n", __LINE__);
        log_warn("can not find Alternatesetting,please check bit_reso and sample_rate\n");
        return DEV_ERR_CONTROL;
    }
    u32 host_ep = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_ISOC);
    ASSERT(host_ep != -1, "ep not enough");
    uac.mic_info->host_ep = (u8)host_ep;

    struct audio_streaming_config *as_cfg = &uac.mic_info->as_cfg[uac.mic_info->cur_alterseting];
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);

    u32 mic_frame_len;
    mic_frame_len = as_cfg->cur_sample_freq * (as_cfg->bit_res / 8) * as_cfg->ch / 1000;
    mic_frame_len += (as_cfg->cur_sample_freq % 1000) ? (as_cfg->bit_res / 8) * as_cfg->ch : 0;
    if (uac.ep_in_dma_buf == NULL) {
        uac.ep_in_dma_buf = usb_h_alloc_ep_buffer(usb_id, host_ep | USB_DIR_IN, mic_frame_len);
    }

    usb_set_interface(host_dev, uac.mic_info->intr_num, uac.mic_info->cur_alterseting); //interface   Alternatesetting
    if (as_cfg->ep_attr & BIT(0)) {
        usb_audio_sampling_frequency_control(host_dev, as_cfg->target_ep, sample_freq);//设置采样率
    }
    //TODO
    //设置音量
    //uac_set_volume(host_dev, uac.mic_info->ctrl_intr_num, uac.mic_info->feature_id, vol_convert(100));
    usb_h_set_ep_isr(host_dev, host_ep | USB_DIR_IN, uac_rx_isr, NULL);
#ifdef USB_HW_20
#if USB_HUB
    usb_hub_rxreg_set(id, host_ep, as_cfg->target_ep, &(host_dev->private_data.hub_info));
#else
    usb_write_rxfuncaddr(usb_id, host_ep, host_dev->private_data.devnum);
#endif
#endif
    usb_h_ep_config(usb_id, host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, as_cfg->interval, uac.ep_in_dma_buf, mic_frame_len);
    usb_h_ep_read_async(usb_id, host_ep, as_cfg->target_ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
    return DEV_ERR_NONE;
}
u32 uac_host_close_mic(usb_dev usb_id)
{
    if (uac.mic_info == NULL) {
        log_warn("func:uac_host_close_mic, line:%d, uac.mic_info not init\n", __LINE__);
        return -1;
    }
    if (uac.mic_info->cur_alterseting == 0) {
        log_warn("func:uac_host_close_mic, line:%d\n", __LINE__);
        return -1;
    }
    uac.mic_info->cur_alterseting = 0;
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);
    usb_h_set_ep_isr(host_dev, uac.mic_info->host_ep | USB_DIR_IN, NULL, NULL);
    usb_free_ep_num(usb_id, uac.mic_info->host_ep | USB_DIR_IN);
    usb_set_interface(host_dev, uac.mic_info->intr_num, uac.mic_info->cur_alterseting); //interface   Alternatesetting
    if (uac.ep_in_dma_buf) {
        usb_h_free_ep_buffer(usb_id, uac.ep_in_dma_buf);
        uac.ep_in_dma_buf = NULL;
    }
    return 0;
}
int uac_host_set_mic_volume(usb_dev usb_id, u16 vol)
{
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(usb_id);
    if (!host_dev) {
        return -1;
    }
    if (!(uac.mic_info && uac.mic_info->intr_num)) {
        return -1;
    }
    //vol的范围：0 ~ 100
    //设置音量，函数里面会根据uac耳机左右声道使能给每个声道发音量
    return uac_set_volume(host_dev, uac.mic_info->ctrl_intr_num, uac.mic_info->feature_id, vol_convert(vol));
}
u32 uac_host_read_mic_cbuf(u8 *buf, u32 len)
{
    if (uac.mic_info == NULL) {
        log_warn("func:uac_host_read_mic_cbuf, line:%d, uac.mic_info not init\n", __LINE__);
        return -1;
    }
    return cbuf_read(&uac.mic_cbuf, buf, len);
}
u32 uac_host_get_mic_cbuf_len()//获取mic_cbuf当前长度
{
    u32 cur_len = cbuf_get_data_size(&uac.mic_cbuf);
    return cur_len;
}
u32 uac_host_get_mic_cbuf_size() //获取mic_cbuf总长度
{
    return UAC_MIC_BUFFER_SIZE;
}
u32 uac_host_query_mic_info(u32 ch, u32 sample_freq, u32 bit_res) //查询mic是否支持该配置 0:支持, 1:不支持
{
    struct audio_streaming_config *as_cfg;
    for (u8 alt = 1; alt < UAC_MAX_CFG; alt++) {
        as_cfg = &uac.mic_info->as_cfg[alt];
        if (as_cfg->ch == ch && as_cfg->bit_res == bit_res) {
            for (u8 i = 0; i < 6; i++) {
                if (as_cfg->sample_freq[i] == sample_freq) {
                    return 0;
                } else {
                    continue;
                }
            }
        } else {
            continue;
        }
    }
    return 1;
}
u32 uac_host_set_mic_hanlder(void (*isr_hanlder)(void *priv), void *priv) //设置mic中断的回调函数
{
    if (uac.mic_info == NULL) {
        log_warn("func:uac_host_read_mic_cbuf, line:%d, uac.mic_info not init\n", __LINE__);
        return -1;
    }
    uac.mic_info->isr_hanlder = isr_hanlder;
    uac.mic_info->priv = priv;
    return 0;
}

static bool _usb_audio_match(const struct usb_device_descriptor *device_desc,
                             const struct usb_interface_descriptor *interface_desc,
                             const u8 *cfg_buf, u32 cfg_len)
{
    return (interface_desc->bInterfaceClass == USB_CLASS_AUDIO);
}

USB_HOST_CLASS_REGISTER(_usb_audio_match, usb_audio_parser,
                        USB_CLASS_AUDIO, "audio%d");

#endif
