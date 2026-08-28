#include <stdint.h>
#include <string.h>
#include "usb/device/usb_stack.h"
#include "usb_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define HISTORY_SIZE 4

/* 记录 USB 请求历史 */
struct request_history_t {
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

static struct request_history_t request_history[HISTORY_SIZE];
static uint8_t request_history_count = 0; /* number of entries stored, 0..HISTORY_SIZE */
static uint8_t host_registered = false;
static uint8_t host_type = HOST_TYPE_UNKNOWN;

/*-----------------------------------------------
 * 可配置的检测规则表
 *----------------------------------------------*/
#define RULE_ANY8  0xFFu
#define RULE_ANY16 0xFFFFu

typedef struct {
    uint8_t  bRequest; /* RULE_ANY8 to match any */
    uint16_t wValue;   /* RULE_ANY16 to match any */
    uint16_t wIndex;   /* RULE_ANY16 to match any */
    uint16_t wLength;  /* RULE_ANY16 to match any */
} req_pattern_t;

typedef struct {
    const char *name;
    uint8_t host_type; /* HOST_TYPE_* */
    uint8_t pattern_count; /* number of sequence entries (1..N) */
    req_pattern_t patterns[HISTORY_SIZE];
} host_rule_t;


static const host_rule_t built_in_rules[] = {
    /* Switch */
    {
        .name = "Switch",
        .host_type = HOST_TYPE_SWITCH,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 8 },
            { .bRequest = USB_REQ_SET_ADDRESS,  .wValue = RULE_ANY16,                       .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_CONFIG << 8), .wIndex = RULE_ANY16, .wLength = 9 },
        }
    },

    /* Windows_v1 */
    {
        .name = "Windows_v1",
        .host_type = HOST_TYPE_WINDOWS,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_SET_ADDRESS, .wValue = RULE_ANY16, .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 0x12 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_CONFIG << 8), .wIndex = RULE_ANY16, .wLength = 0x00ff },
        }
    },

    /* Windows_v2 */
    {
        .name = "Windows_v2",
        .host_type = HOST_TYPE_WINDOWS,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 0x40 },
            { .bRequest = USB_REQ_SET_ADDRESS, .wValue = RULE_ANY16, .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 0x12 },
        }
    },
#if 0
    /* Windows_bios */
    {
        .name = "Windows_bios",
        .host_type = HOST_TYPE_WINDOWS,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_SET_ADDRESS, .wValue = RULE_ANY16, .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 0x8 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_CONFIG << 8), .wIndex = RULE_ANY16, .wLength = 0x8 },
        }
    },
#endif
    /* Android */
    {
        .name = "Android",
        .host_type = HOST_TYPE_ANDROID,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_SET_ADDRESS, .wValue = RULE_ANY16, .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 0x12 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_CONFIG << 8), .wIndex = RULE_ANY16, .wLength = 9 },
        }
    },

    /* PS4 */
    {
        .name = "PS4",
        .host_type = HOST_TYPE_PS4,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_SET_ADDRESS, .wValue = RULE_ANY16, .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 8 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_STRING << 8), .wIndex = RULE_ANY16, .wLength = 2 },
        }
    },

    /* iOS */
    {
        .name = "iOS",
        .host_type = HOST_TYPE_IOS,
        .pattern_count = 3,
        .patterns = {
            { .bRequest = USB_REQ_SET_ADDRESS, .wValue = RULE_ANY16, .wIndex = RULE_ANY16, .wLength = RULE_ANY16 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = ((uint16_t)USB_DT_DEVICE << 8), .wIndex = RULE_ANY16, .wLength = 8 },
            { .bRequest = USB_REQ_GET_DESCRIPTOR, .wValue = (((uint16_t)USB_DT_STRING << 8) | 0x02), .wIndex = 0x0409, .wLength = 2 },
        }
    },
    {
        .name = "USBCV",
        .host_type = HOST_TYPE_USBCV,
        .pattern_count = 4,
        .patterns = {
            { USB_REQ_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), RULE_ANY16, 8 },
            { USB_REQ_SET_ADDRESS, RULE_ANY16, RULE_ANY16, RULE_ANY16 },
            { USB_REQ_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), RULE_ANY16, 8 },
            { USB_REQ_GET_DESCRIPTOR, (USB_DT_DEVICE << 8), RULE_ANY16, 0x12 },
        }
    },
};

/*-----------------------------------------------
 * 工具函数
 *----------------------------------------------*/
static inline bool pattern_match(const req_pattern_t *p, const struct request_history_t *r)
{
    if (p->bRequest != RULE_ANY8 && p->bRequest != r->bRequest) {
        return false;
    }
    if (p->wValue != RULE_ANY16 && p->wValue != r->wValue) {
        return false;
    }
    if (p->wIndex != RULE_ANY16 && p->wIndex != r->wIndex) {
        return false;
    }
    if (p->wLength != RULE_ANY16 && p->wLength != r->wLength) {
        return false;
    }
    return true;
}

static inline bool request_equal(const struct request_history_t *a,
                                 const struct request_history_t *b)
{
    return (a->bRequest == b->bRequest &&
            a->wValue   == b->wValue &&
            a->wIndex   == b->wIndex);
}

static void add_request_to_history(const struct usb_ctrlrequest *req)
{
    if (request_history_count >= HISTORY_SIZE) {
        /* 已满：不再记录新的请求（保持最先到达的历史） */
        return;
    }

    struct request_history_t newreq = {
        .bRequest = req->bRequest,
        .wValue   = req->wValue,
        .wIndex   = req->wIndex,
        .wLength  = req->wLength,
    };

    /* 特殊修正: SET_ADDRESS 总是 wValue/wIndex/wLength = 0 */
    if (newreq.bRequest == USB_REQ_SET_ADDRESS) {
        newreq.wValue = 0;
        newreq.wIndex = 0;
        newreq.wLength = 0;
    }

    /* 去重：如果和上一次完全相同，则跳过 */
    if (request_history_count > 0) {
        if (request_equal(&newreq, &request_history[request_history_count - 1])) {
            return;
        }
    }

    request_history[request_history_count++] = newreq;
}

void usb_reset_host_type(const usb_dev usb_id)
{
    host_registered = false;
    request_history_count = 0;
    memset(request_history, 0, sizeof(request_history));
}

uint32_t usb_get_host_type(const usb_dev usb_id)
{
#if 0 //TCFG_USB_APPLE_DOCK_EN
    return HOST_TYPE_IOS;
#else
    return host_type;
#endif
}

/*-----------------------------------------------
 * 主机识别逻辑
 *----------------------------------------------*/
static void try_detect_host(void)
{
    if (request_history_count < 3 || host_type != HOST_TYPE_UNKNOWN) {
        return;
    }


    for (int ri = 0; ri < ARRAY_SIZE(built_in_rules); ri++) {
        const host_rule_t *rule = &built_in_rules[ri];
        /* 历史记录不够匹配此规则的最小长度，跳过整个规则 */
        if (request_history_count < rule->pattern_count) {
            continue;
        }
        int matched = true;
        for (int pi = 0; pi < rule->pattern_count; pi++) {
            const req_pattern_t *p = &rule->patterns[pi];
            const struct request_history_t *r = &request_history[pi];
            if (!pattern_match(p, r)) {
                matched = false;
                break;
            }
        }
        if (matched) {
            host_type = rule->host_type;
            log_info("host_detect: matched rule '%s', host_type=%d", rule->name, host_type);
            return;
        }
    }
}

/*-----------------------------------------------
 * 主流程接口
 *----------------------------------------------*/
void detect_host_type(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    if (host_registered) {
        return;
    }

    add_request_to_history(req);
    try_detect_host();

    log_info("request_history_count: %d, host_type: %d", (int)request_history_count, (int)host_type);

    if (request_history_count < ARRAY_SIZE(request_history) && host_type == HOST_TYPE_UNKNOWN) {
        return;
    }

    host_registered = true;

    for (int i = 0; i < request_history_count; i++) {
        const struct request_history_t *r = &request_history[i];
        log_info("requeset@%d: %02x %04x %04x %04x", i, (unsigned)r->bRequest, (unsigned)r->wValue, (unsigned)r->wIndex, (unsigned)r->wLength);
    }
    const usb_dev usb_id = usb_device2id(usb_device);

    switch (host_type) {
    case HOST_TYPE_WINDOWS:
        log_info("-----------------HOST_TYPE_WINDOWS----------------------");
        break;

    case HOST_TYPE_ANDROID:
        log_info("-----------------HOST_TYPE_ANDROID----------------------");
        break;

    case HOST_TYPE_IOS:
        log_info("-----------------HOST_TYPE_IOS--------------------------");
        break;

    case HOST_TYPE_PS4:
        log_info("-----------------HOST_TYPE_PS4--------------------------");
        break;

    case HOST_TYPE_SWITCH:
        log_info("-----------------HOST_TYPE_SWITCH-----------------------");
        break;
    case HOST_TYPE_USBCV:
        log_info("-----------------HOST_TYPE_USBCV-----------------------");
        break;
    default:
        log_info("-----------------HOST_TYPE_UNKNOWN----------------------");
        break;
    }
}


