#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "cpu/includes.h"
#include "app_config.h"
#include "system/timer.h"
#include "device/ioctl_cmds.h"
#include "device_drive.h"
#if TCFG_HID_HOST_ENABLE
#include "usb/host/usb_host.h"
#include "usb_ctrl_transfer.h"
#include "usb_bulk_transfer.h"
#include "hid.h"
#include "usb_config.h"
#include "usb_hid_keys.h"

#define MAIN_ITEM    0
#define GLOBAL_ITEM  1
#define LOCAL_ITEM   2

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[HID]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


struct hid_device_t hid_device[USB_MAX_HW_NUM][MAX_HOST_INTERFACE];

static int set_power(struct usb_host_device *host_dev, u32 value)
{
    const usb_dev usb_id = host_device2id(host_dev);
    for (int i = 0; i < MAX_HOST_INTERFACE; i++) {
        /* only clear report parsing state, keep bNumEndpoints + ep_pair + interval from usb_hid_parser */
        u8 saved_bNumEndpoints = hid_device[usb_id][i].bNumEndpoints;
        u8 saved_ep_pair[4];
        u8 saved_interval[16];
        memcpy(saved_ep_pair, hid_device[usb_id][i].ep_pair, 4);
        memcpy(saved_interval, hid_device[usb_id][i].interval, 16);
        memset(&hid_device[usb_id][i], 0, sizeof(struct hid_device_t));
        hid_device[usb_id][i].bNumEndpoints = saved_bNumEndpoints;
        memcpy(hid_device[usb_id][i].ep_pair, saved_ep_pair, 4);
        memcpy(hid_device[usb_id][i].interval, saved_interval, 16);
    }
    return DEV_ERR_NONE;
}

static int get_power(struct usb_host_device *host_dev, u32 value)
{
    return DEV_ERR_NONE;
}

static const struct interface_ctrl hid_ctrl = {
    .interface_class = USB_CLASS_HID,
    .set_power = set_power,
    .get_power = get_power,
    .ioctl = NULL,
};

static const struct usb_interface_info _usb_if[USB_MAX_HW_NUM][MAX_HOST_INTERFACE] = {
    {
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[0][0],
        },
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[0][1],
        },
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[0][2],
        },
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[0][3],
        },
    },
#if USB_MAX_HW_NUM > 1
    {
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[1][0],
        },
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[1][1],
        },
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[1][2],
        },
        {
            .ctrl = (struct interface_ctrl *) &hid_ctrl,
            .dev.hid = &hid_device[1][3],
        },
    },
#endif
};


int usb_hid_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
    pBuf += sizeof(struct usb_interface_descriptor);
    const usb_dev usb_id = host_device2id(host_dev);

    struct usb_endpoint_descriptor *endpoint;

    pBuf += 9;//hid desc;

    const struct usb_interface_info *usb_if = &_usb_if[usb_id][interface_num];

    memset(usb_if->dev.p, 0, sizeof(struct hid_device_t));

    host_dev->interface_info[interface_num] = usb_if;
    usb_if->dev.hid->parent = host_dev;

    log_info("hid eps %d  %d %x %x", interface->bNumEndpoints, interface_num, usb_if, usb_if->dev.p);

    log_info("parent %x hid @ interface %d usb_if %x hid %x",
             host_dev, interface_num, usb_if, usb_if->dev.hid);

    if ((interface->bInterfaceProtocol == 0x02) ||
        (interface->bInterfaceProtocol == 0x01) ||
        (interface->bInterfaceProtocol == 0x00)) { //keyboard (0x01), mouse (0x02), or report protocol (0x00)

        usb_if->dev.hid->bNumEndpoints = interface->bNumEndpoints;
        usb_if->dev.hid->report_list[0].usage = interface->bInterfaceProtocol;
        for (int i = 0 ; i < interface->bNumEndpoints; i++) {
            endpoint = (struct usb_endpoint_descriptor *)pBuf;
            if (USB_DIR_IN & endpoint->bEndpointAddress) {
                const u8 ep = endpoint->bEndpointAddress & 0x0f;
                usb_if->dev.hid->ep_pair[i] = ep;
                hid_device[usb_id][0].interval[ep] = endpoint->bInterval;
                log_info("interfacenum = %d,endpoint = %x interval = %x",
                         interface->bInterfaceNumber, endpoint->bEndpointAddress, endpoint->bInterval);
            }
            pBuf += endpoint->bLength;
        }
    } else {
        log_info("vendor");
        host_dev->interface_info[interface_num] = NULL; //???
        for (int i = 0 ; i < interface->bNumEndpoints; i++) {
            endpoint = (struct usb_endpoint_descriptor *)pBuf;
            if (USB_DIR_IN & endpoint->bEndpointAddress) {
                /* interface_endpoint[interface->bInterfaceNumber] = endpoint->bEndpointAddress & 0x0f; */
                log_info("interfacenum = %d,endpoint = %x interval = %x",
                         interface->bInterfaceNumber, endpoint->bEndpointAddress, endpoint->bInterval);
            }
            pBuf += endpoint->bLength;
        }
        return sizeof(struct usb_interface_descriptor);
    }

    return pBuf - (u8 *)interface;
}

// ---------- Report descriptor parser: type-specific helpers ----------

/// Track mouse Input item: buttons, X/Y, wheel position
static void _hid_input_mouse(struct report_info_t *rpt, u8 cur_usage,
                             u8 *total_bits, u8 section_bits)
{
    if (cur_usage == 1) {                       // buttons
        if (rpt->btn_start_bit == 0) {
            rpt->btn_start_bit = *total_bits;
        }
        rpt->btn_width += section_bits;
    } else if ((cur_usage == 0x30) || (cur_usage == 0x31)) { // X / Y
        if (rpt->xy_start_bit == 0) {
            rpt->xy_start_bit = *total_bits;
        }
        rpt->xy_width = section_bits;
    } else if (cur_usage == 0x38) {             // wheel
        if (rpt->wheel_start_bit == 0) {
            rpt->wheel_start_bit = *total_bits;
        }
        if (rpt->xy_width || section_bits < 24) {
            rpt->wheel_width = section_bits;
        } else {
            rpt->wheel_width = rpt->xy_width = section_bits / 3;
            rpt->xy_start_bit = *total_bits;
            rpt->wheel_start_bit = rpt->xy_start_bit + rpt->xy_width * 2;
        }
    } else if (cur_usage == 0xb8) {             // wheel (extended)
        rpt->wheel_width = rpt->xy_width = section_bits / 4;
        rpt->xy_start_bit = *total_bits;
        rpt->wheel_start_bit = rpt->xy_start_bit + rpt->xy_width * 2;
    }
}

/// Track consumer Input item: bit offset / count for button bits
static void _hid_input_consumer(struct report_info_t *rpt, int itemVal,
                                u8 *total_bits, u8 section_bits)
{
    // (itemVal & 1) == 0 → Data (not Constant)
    if ((itemVal & 1) == 0) {
        if (rpt->consumer_bit_count == 0) {
            rpt->consumer_bit_offset = *total_bits;
        }
        rpt->consumer_bit_count += section_bits;
    }
}

/// Handle Usage Page (bTag 0x00): detect device type from page value
/// Returns true when a report was saved (Consumer page switch)
static bool _hid_handle_usage_page(struct report_info_t *rpt,
                                   struct hid_device_t *hid, int itemVal)
{
    if (rpt->usage == 0x06 && itemVal == 0x07) {
        rpt->usage = 0x07;
        log_debug("usage page: keyboard(0x07)");
        return false;
    }
    if (itemVal == 0x02) {
        rpt->usage = 0x02;
        log_debug("usage page: mouse(0x02)");
        return false;
    }
    if (itemVal == 0x0C) {
        rpt->usage = 0x0C;
        rpt->consumer_usage_count = 0;
        rpt->consumer_bit_offset = 0;
        rpt->consumer_bit_count = 0;
        log_debug("usage page: consumer(0x0C)");
        return false;
    }
    // Usage Page switched away from Consumer — save what we collected
    if (rpt->usage == 0x0C && rpt->consumer_bit_count > 0) {
        if (hid->report_count < MAX_REPORT_COUNT) {
            memcpy(&hid->report_list[hid->report_count], rpt, sizeof(*rpt));
            hid->report_count++;
            log_debug("consumer saved (Usage Page -> %x)", itemVal);
        } else {
            log_warn("report_list full(%d), drop consumer report", MAX_REPORT_COUNT);
        }
        memset(rpt, 0, sizeof(*rpt));
        return true;
    }
    return false;
}

/// Handle Usage (bTag 0x00): detect type at top level, collect consumer usage IDs
static void _hid_handle_usage(struct report_info_t *rpt,
                              u8 itemVal, u8 *cur_usage, u8 collection_deep)
{
    if (rpt->usage == 0) {
        rpt->usage = itemVal;
        log_debug("set type %x", rpt->usage);
    }
    // collect consumer usage IDs inside consumer collections
    if (rpt->usage == 0x0C && collection_deep > 0) {
        if (rpt->consumer_usage_count < MAX_CONSUMER_USAGES) {
            rpt->consumer_usages[rpt->consumer_usage_count++] = (u32)itemVal;
            log_debug("consumer usage[%d] = %x",
                      rpt->consumer_usage_count - 1, itemVal);
        }
    }
    *cur_usage = itemVal;
    // detect keyboard  / mouse type from top-level usage
    if (!collection_deep) {
        if (itemVal == 0x06 || itemVal == 0x07) {
            rpt->usage = itemVal;
            log_debug("set type keyboard %x", rpt->usage);
        } else if (itemVal == 0x02) {
            rpt->usage = itemVal;
            log_debug("set type mouse %x", rpt->usage);
        }
    }
}

/// Check whether the current report should be saved at End Collection
static bool _hid_is_saveable_report(u8 usage)
{
    return (usage == 0x02 || usage == 0x06 || usage == 0x07 || usage == 0x0C);
}


// ---------- Main report descriptor parser loop ----------

static u32 _hid_report_parse(struct hid_device_t *hid, const u8 *report, u32 len)
{
    hid->report_count = 0;
    struct report_info_t null_rpt;
    struct report_info_t *const rpt = &null_rpt;

    memset(rpt, 0, sizeof(*rpt));

    int index = 0;
    u8 report_size = 0;
    u8 report_count = 0;
    u8 section_bits = 0;
    u8 total_bits = 0;
    u8 cur_usage = 0;
    u8 collection_deep = 0;
    u32 undef_type = 0;
    u8 undef_usage = 0;

    while (index < len) {
        u8 ops = report[index++];
        u8 bSize = ops & 0x03;
        if (bSize == 3) {
            bSize = 4;
        }
        u8 bType = (ops >> 2) & 0x03;
        u8 bTag = (ops >> 4) & 0x0F;

        int itemVal = 0;
        for (u8 j = 0; j < bSize; j++) {
            if (index + j < len) {
                itemVal += report[index + j] << (8 * j);
            }
        }

        // skip vendor-defined long items
        if (undef_type) {
            undef_type++;
            if (bTag == 0x0A) {
                undef_usage++;
            } else if (bTag == 0x0C) {
                undef_usage--;
            }
            if (undef_usage == 0 && undef_type > 2) {
                undef_type = 0;
            }
            index += bSize;
            continue;
        }
        if (itemVal == 0xffb5) {
            undef_type = 1;
            index += bSize;
            continue;
        }

        switch (bType) {
        case MAIN_ITEM:
            // Global Report Size/Count persist — accumulate section bits
            if (report_size && report_count) {
                section_bits = report_size * report_count;
            }

            switch (bTag) {
            case 0x08: // Input
                if (rpt->usage == 0x02) {
                    _hid_input_mouse(rpt, cur_usage, &total_bits, section_bits);
                } else if (rpt->usage == 0x0C) {
                    _hid_input_consumer(rpt, itemVal, &total_bits, section_bits);
                }
                total_bits += section_bits;
                break;

            case 0x09: // Output
            case 0x0B: // Feature
            case 0x0D:
            case 0x0E:
            case 0x0F: // Reserved
                break;

            case 0x0A: // Collection
                collection_deep++;
                log_debug("Collection %d %x", collection_deep, rpt->usage);
                break;

            case 0x0C: // End Collection
                collection_deep--;
                log_debug("End Collection %d %x", collection_deep, rpt->usage);
                if (collection_deep == 0 && _hid_is_saveable_report(rpt->usage)) {
                    if (hid->report_count < MAX_REPORT_COUNT) {
                        memcpy(&hid->report_list[hid->report_count], rpt, sizeof(*rpt));
                        hid->report_count++;
                    } else {
                        log_warn("report_list full(%d), drop report", MAX_REPORT_COUNT);
                    }
                    memset(rpt, 0, sizeof(*rpt));
                }
                break;

            default:
                // bTag == 0 is usually padding after End Collection
                if (bTag != 0) {
                    log_debug("MAIN_ITEM Unknown btag :%x", bTag);
                }
                break;
            }
            break;

        case GLOBAL_ITEM:
            switch (bTag) {
            case 0x00: // Usage Page
                _hid_handle_usage_page(rpt, hid, itemVal);
                break;

            case 0x07: // Report Size
                report_size = itemVal;
                break;

            case 0x08: // Report ID
                rpt->report_id = itemVal;
                log_debug("Report ID %x usage %x", itemVal, rpt->usage);
                break;

            case 0x09: // Report Count
                report_count = itemVal;
                break;

            case 0x01:
            case 0x02: // Logical Min/Max
            case 0x03:
            case 0x04: // Physical Min/Max
            case 0x05:
            case 0x06: // Unit Exponent, Unit
            case 0x0A:
            case 0x0B: // Push, Pop
                // not needed for keyboard/mouse/consumer
                break;

            default:
                log_debug("GLOBAL_ITEM Unknown btag :%x", bTag);
                return 2;
            }
            break;

        case LOCAL_ITEM:
            if (bTag == 0x00) { // Usage
                _hid_handle_usage(rpt, (u8)itemVal, &cur_usage, collection_deep);
            }
            // other LOCAL items (Usage Min/Max, Designator, String, Delimiter)
            // — not needed for keyboard/mouse/consumer
            break;

        default: // Reserved type
            log_debug("OTHER Unknown btype :%x", bType);
            return 4;
        }

        index += bSize;
    }
    return 0;
}
static u32 hid_report_parse(struct hid_device_t *hid, const u8 *report, u32 len)
{
    _hid_report_parse(hid, report, len);
    for (int i = 0; i < hid->report_count; i++) {
        struct report_info_t *rpt = &hid->report_list[i];
        if (rpt->report_id == 0) {
            rpt->report_id = 0xff;
        }
        if (rpt->usage == 0x02) {
            rpt->btn_start_bit /= 8;
            rpt->btn_width /= 8;
            rpt->xy_start_bit /= 8;
            rpt->xy_width /= 8;
            rpt->wheel_start_bit /= 8;
            rpt->wheel_width /= 8;

            log_info("mouse report_id %d",
                     rpt->report_id);

            log_info("btn_width %d-%d",
                     rpt->btn_start_bit, rpt->btn_width);
            log_info("xy_width %d-%d",
                     rpt->xy_start_bit, rpt->xy_width);
            log_info("wheel_width %d-%d",
                     rpt->wheel_start_bit, rpt->wheel_width);

            if (rpt->btn_width != 2) {
                rpt->btn_width = 1;
            }

            log_info("btn_width %d-%d",
                     rpt->btn_start_bit, rpt->btn_width);

        } else if (rpt->usage == 6 || rpt->usage == 7) {
            log_info("keyboard report_id %d", rpt->report_id);
        } else if (rpt->usage == 0x0C) {
            log_info("consumer report_id %d bit_offset=%d bit_count=%d usage_count=%d",
                     rpt->report_id, rpt->consumer_bit_offset,
                     rpt->consumer_bit_count, rpt->consumer_usage_count);
        } else {
            log_info("unknown usage %d", rpt->usage);
        }
    }
    return 0;
}

__attribute__((weak)) void mouse_route(const struct mouse_data_t *p)
{
    log_debug("btn: %x x-y %d %d wheel %d",
              p->btn, p->x, p->y, p->wheel);
}
static void hid_convert_mouse(const struct report_info_t *mouse, const u8 *buffer)
{
    struct mouse_data_t mouse_data;
    memset(&mouse_data, 0, sizeof(mouse_data));
    if (mouse->report_id != 0xff) {
        if (mouse->report_id != buffer[0]) {
            log_warn("report_id = %x buffer[0] = %x", mouse->report_id, buffer[0]);
            return;
        }
        buffer++;
    }
    const u8 *ptr;
    ptr = &buffer[mouse->btn_start_bit];
    if (mouse->btn_width == 2) {
        mouse_data.btn = ptr[0] | (ptr[1] << 8);
    } else {
        mouse_data.btn = ptr[0] ;
    }
    s16 tmp;
    ptr = &buffer[mouse->xy_start_bit];
    if (mouse->xy_width == 1 || mouse->xy_width == 2) {
        mouse_data.x = (char)ptr[0];
        mouse_data.y = (char)ptr[1];
    } else if (mouse->xy_width == 4) {
        mouse_data.x = ptr[0] | (ptr[1] << 8);
        ptr += 2;
        mouse_data.y = ptr[0] | (ptr[1] << 8);
    } else if (mouse->xy_width == 3) {
        tmp = (ptr[1] & 0xf) << 12 | ptr[0] << 4;
        tmp = tmp >> 4;
        mouse_data.x = tmp;

        tmp = (ptr[2] << 8) | ((ptr[1] >> 4) << 4);
        tmp = tmp >> 4;
        mouse_data.y = tmp;
    } else {
        log_warn("error mouse xy_width %d", mouse->xy_width);
    }

    ptr = &buffer[mouse->wheel_start_bit];

    if (mouse->wheel_width == 1) {
        mouse_data.wheel = (char)ptr[0];
    } else {
        mouse_data.wheel = ptr[0] | (ptr[1] << 8);
    }

    mouse_route(&mouse_data);
}
__attribute__((weak)) void keyboard_route(const u8  *p)
{
    log_debug("keyboard_buffer:");
    printf_buf((u8 *)p, 12);
}
__attribute__((weak)) void consumer_route(u32 consumer_key_mask)
{
    log_debug("consumer_key mask: %x", consumer_key_mask);
}
void hid_convert_krbd(const struct report_info_t *kbd, const u8 *buffer)
{
#if 0
    u8 keyboard_buffer[12];
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));
    if (kbd->report_id != 0xff) {
        if (kbd->report_id != buffer[0]) {
            log_error("report_id = %x buffer[0] = %x", kbd->report_id, buffer[0]);
            return;
        }
        buffer++;
    }

    u8 idx = 0;
    u8 index = 0;
    int i = 0;
    for (i = 0; i < 8; i++) {
        if (buffer[0] & BIT(i)) {
            keyboard_buffer[idx++] = 0xe0 + i;
        }
    }

    if (buffer[1] == 0) {
        buffer += 2;
    }
    index = idx;
    for (; idx < 12; idx++) {
        if (*buffer) {
            keyboard_buffer[index] = *buffer;
            index++;
        }
        buffer ++;
    }
    keyboard_route(keyboard_buffer);
#else
    if (kbd->report_id != 0xff) {
        if (kbd->report_id != buffer[0]) {
            log_error("report_id = %x buffer[0] = %x", kbd->report_id, buffer[0]);
            return;
        }
        buffer++;
    }
    u8 keyboard_buffer[8];
    memset(keyboard_buffer, 0, sizeof(keyboard_buffer));

    keyboard_buffer[0] = *buffer;
    buffer ++;

    if (*buffer == 0) {
        buffer ++;
    }
    keyboard_buffer[1] = 0;

    /* memcpy(&keyboard_buffer[2], buffer, 6); */
    u8 pos = 2;
    for (int i = pos; i < 8; i++) {
        if (*buffer) {
            keyboard_buffer[pos] = *buffer;
            pos++;
        }
        buffer++;
    }
    keyboard_route(keyboard_buffer);
#endif
}

static void hid_convert_consumer(const struct report_info_t *consumer, const u8 *buffer)
{
    u32 key_mask = 0;
    static u32 prev_key_mask;

    if (consumer->report_id != 0xff) {
        if (consumer->report_id != buffer[0]) {
            log_error("consumer report_id mismatch: expected %x got %x",
                      consumer->report_id, buffer[0]);
            return;
        }
        buffer++;
    }

    log_info("consumer data: %02x %02x", buffer[0], buffer[1]);

    for (u8 i = 0; i < consumer->consumer_usage_count; i++) {
        u32 usage_id = consumer->consumer_usages[i];

        u16 abs_bit = consumer->consumer_bit_offset + i;
        u8  byte_pos = abs_bit / 8;
        u8  bit_pos  = abs_bit % 8;

        if (!(buffer[byte_pos] & BIT(bit_pos))) {
            continue;
        }

        switch (usage_id) {
        case 0xE9:  /* VOLUME_INC */
            key_mask |= CONSUMER_KEY_VOLUP;
            log_info("volup");
            break;
        case 0xEA:  /* VOLUME_DEC */
            key_mask |= CONSUMER_KEY_VOLDOWN;
            log_info("voldown");
            break;
        case 0xE2:  /* MUTE */
            key_mask |= CONSUMER_KEY_MUTE;
            log_info("mute");
            break;
        case 0xCD:  /* PLAY_PAUSE */
            key_mask |= CONSUMER_KEY_PLAYPAUSE;
            log_info("play/pause");
            break;
        case 0xB5:  /* SCAN_NEXT_TRACK */
            key_mask |= CONSUMER_KEY_NEXTTRACK;
            log_info("next track");
            break;
        case 0xB6:  /* SCAN_PREV_TRACK */
            key_mask |= CONSUMER_KEY_PREVTRACK;
            log_info("prev track");
            break;
        case 0xB7:  /* STOP */
            key_mask |= CONSUMER_KEY_STOP;
            log_info("stop");
            break;
        case 0xCC:  /* STOP_EJECT */
            key_mask |= CONSUMER_KEY_STOPEJECT;
            log_info("stop/eject");
            break;
        case 0xB0:  /* PLAY */
        case 0xB1:  /* PAUSE */
            key_mask |= CONSUMER_KEY_PLAYPAUSE;
            log_info("play/pause");
            break;
        default:
            log_info("unmapped consumer usage %x", usage_id);
            break;
        }
    }

    // detect key release: all keys released
    if (prev_key_mask && !key_mask) {
        log_info("consumer key release");
    }
    prev_key_mask = key_mask;

    consumer_route(key_mask);
}

static void hid_route(const struct hid_device_t *hid, const u8 *buffer)
{
    for (int i = 0; i < hid->report_count; i++) {
        if ((hid->report_list[i].usage == 0x06) ||
            (hid->report_list[i].usage == 0x07)) {
            hid_convert_krbd(&hid->report_list[i], buffer);
        } else if (hid->report_list[i].usage == 0x02) {
            hid_convert_mouse(&hid->report_list[i], buffer);
        } else if (hid->report_list[i].usage == 0x0C) {
            hid_convert_consumer(&hid->report_list[i], buffer);
        } else {
            r_printf("usage %x", hid->report_list[i].usage);
        }
    }
}
static void hid_isr(struct usb_host_device *host_dev, u32 ep)
{
    u8 buffer[64] = {0};

    // host_dev is actually usb_if (passed via usb_h_set_ep_isr's p parameter)
    struct usb_interface_info *usb_if = (struct usb_interface_info *)host_dev;
    struct hid_device_t *hid = usb_if->dev.hid;

    u32 target_ep = hid->ep_pair[ep];
    if (!target_ep) {
        return;
    }

    usb_dev usb_id = host_device2id((struct usb_host_device *)hid->parent);
    u32 rx_len = usb_h_ep_read_async(usb_id, ep, target_ep, buffer, 64, USB_ENDPOINT_XFER_INT, 0);

    if (rx_len) {
        hid_route(hid, buffer);
    }

    usb_h_ep_read_async(usb_id, ep, target_ep, buffer, 8, USB_ENDPOINT_XFER_INT, 1);
}
void hid_process(u32 id)
{
    struct usb_host_device *host_dev = (struct usb_host_device *)host_id2device(id);
    u8 report[256 + 2];
    u8 ep_pair[5];

    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        struct usb_interface_info  *usb_if = (struct usb_interface_info *)host_dev->interface_info[i];
        log_info("parent %x hid @ interface %d usb_if %x hid %x",
                 host_dev, i, usb_if, usb_if ? usb_if->dev.hid : 0);
        if (usb_if &&
            (usb_if->ctrl->interface_class == USB_CLASS_HID)) {
            hid_set_idle(host_dev, i);
            memset(report, 0, sizeof(report));
            hid_get_report(host_dev, report, i, 0xff);
            printf_buf(report, 256);
            hid_report_parse(usb_if->dev.hid, report, 256);
            memcpy(ep_pair, usb_if->dev.hid->ep_pair, 4);

            log_info("bNumEndpoints=%d report_count=%d ep_pair=%02x %02x %02x %02x",
                     usb_if->dev.hid->bNumEndpoints, usb_if->dev.hid->report_count,
                     ep_pair[0], ep_pair[1], ep_pair[2], ep_pair[3]);

            if (usb_if->dev.hid->report_count == 0) {
                log_info("not found report, skip");
                continue;
            }

            if (usb_if->dev.hid->bNumEndpoints == 0) {
                log_info("bNumEndpoints=0, skip");
                continue;
            }

            for (int j = 0; j < usb_if->dev.hid->bNumEndpoints; j++) {
                u32 target_ep = ep_pair[j];
                if (target_ep == 0) {
                    log_info("target_ep=0, skip (index %d)", j);
                    continue;
                }

                u32 host_ep = usb_get_ep_num(id, USB_DIR_IN, USB_ENDPOINT_XFER_INT);

                if (host_ep == -1) {
                    log_error("no free host ep, target_ep=%x", target_ep);
                    continue;
                }

                usb_if->dev.hid->ep_pair[host_ep] = target_ep;

                log_info("D2H ep: %x --> %x interval %d",
                         target_ep, host_ep, hid_device[id][0].interval[target_ep]);

                usb_h_set_ep_isr(host_dev, host_ep | USB_DIR_IN, hid_isr, usb_if);

                if (!hid_device[id][0].intr_ep_in_buf) {
                    hid_device[id][0].intr_ep_in_buf = usb_h_alloc_ep_buffer(id, host_ep | USB_DIR_IN, 64);
                }
#ifdef USB_HW_20
#if USB_HUB
                usb_hub_rxreg_set(id, host_ep, target_ep, &(host_dev->private_data.hub_info));
#else
                usb_write_rxfuncaddr(id, host_ep, host_dev->private_data.devnum);
#endif
#endif
                usb_h_ep_config(id,  host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 1,
                                hid_device[id][0].interval[target_ep], hid_device[id][0].intr_ep_in_buf, 64);
                int r = usb_h_ep_read_async(id, host_ep, target_ep, NULL, 0, USB_ENDPOINT_XFER_INT, 1);
            }
        } else {
            if (usb_if) {
                log_info("skip non-hid class %x", usb_if->ctrl->interface_class);
            }
        }
    }

}

static bool _usb_hid_match(const struct usb_device_descriptor *device_desc,
                           const struct usb_interface_descriptor *interface_desc,
                           const u8 *cfg_buf, u32 cfg_len)
{
    return (interface_desc->bInterfaceClass == USB_CLASS_HID);
}

USB_HOST_CLASS_REGISTER(_usb_hid_match, usb_hid_parser,
                        USB_CLASS_HID, "hid%d");

#endif
