#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".usb.data.bss")
#pragma data_seg(".usb.data")
#pragma code_seg(".usb.text")
#pragma const_seg(".usb.text.const")
#pragma str_literal_override(".usb.text.const")
#endif

#include "system/includes.h"
#include "app_config.h"
#include "device_drive.h"
/* #include "os/os_compat.h" */
#include "system/event.h"
#include "clock_manager/clock_manager.h"
#if TCFG_USB_HOST_ENABLE
#include "usb_config.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "usb_ctrl_transfer.h"
#include "usb/host/usb_test_host.h"
#include "usb/usb_task.h"
#include "utils/resource_multiplex.h"
#include "usb/host/hub.h"
#include "device_event.h"

#if TCFG_HOST_UVC_ENABLE
#include "host_uvc.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_HOST]"
#define LOG_ERROR_ENABLE
#define LOG_WARN_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

extern const usb_host_class_item_t __usb_host_class_start[];
extern const usb_host_class_item_t __usb_host_class_end[];

static struct usb_host_device host_devices[USB_MAX_HW_NUM];// SEC(.usb_h_bss);

#define     device_to_usbdev(device)	((struct usb_host_device *)((device)->private_data))


int host_dev_status(const struct usb_host_device *host_dev)
{
    return ((host_dev)->private_data.status);
}

u32 host_device2id(const struct usb_host_device *host_dev)
{
#if USB_MAX_HW_NUM > 1
    return ((host_dev)->private_data.usb_id);
#else
    return 0;
#endif
}
const struct usb_host_device *host_id2device(const usb_dev id)
{
#if USB_MAX_HW_NUM > 1
    return &host_devices[id];
#else
    return &host_devices[0];
#endif
}
int usb_sem_init(struct usb_host_device  *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

#if USB_HUB
    if (host_dev->parent == NULL) {
        usb_host_config(usb_id);
    }
#else
    usb_host_config(usb_id);
#endif

    OS_SEM *sem = zalloc(sizeof(OS_SEM));
    ASSERT(sem, "usb alloc sem error");
    host_dev->sem = sem;
    log_debug("usb_sem_init %x %x ", host_dev, sem);
    os_sem_create(host_dev->sem, 0);
    return 0;
}
int usb_sem_pend(struct usb_host_device  *host_dev, u32 timeout)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    usb_dev id = host_device2id(host_dev);
    int ret = 0;

    while (timeout) {
        u32 ot_unit = timeout > 20 ? 20 : timeout;
        timeout -= ot_unit;
        if (usb_otg_online(id) != OTG_STATE_HOST_MODE) {
            log_warn("otg disconnect wait ctrl xfer");
            return -DEV_ERR_OFFLINE;
        }

        if (resource_multiplex_is_registered(IO_PORT_DM) && resource_multiplex_has_requester_waiting(IO_PORT_DM, (u32)host_dev)) {
            log_warn("otg disconnect %d, IO be occupyed", __LINE__);
            return -DEV_ERR_OFFLINE;
        }

        ret = os_sem_pend(host_dev->sem, ot_unit);
        if (ret == OS_TIMEOUT) {
            continue;
        } else {
            break;
        }
    }

    if (ret) {
        r_printf("usb_sem_pend %d ", ret);
    }
    return ret;
}
int usb_sem_post(struct usb_host_device  *host_dev)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    int ret = os_sem_post(host_dev->sem);
    if (ret) {
        r_printf("usb_sem_post %d ", ret);
    }
    return 0;
}
int usb_sem_del(struct usb_host_device *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

    r_printf("1");
    if (host_dev == NULL || host_dev->sem == NULL) {
        return 0;
    }
    r_printf("2");
    r_printf("3");
#if USB_HUB
    if (host_dev->parent == NULL) {
        os_sem_del(host_dev->sem, 0);
    }
#else
    os_sem_del(host_dev->sem, 0);
#endif
    r_printf("4");
    log_debug("usb_sem_del %x %x ", host_dev, host_dev->sem);
    free(host_dev->sem);
    r_printf("5");
    host_dev->sem = NULL;
    r_printf("6");
#if USB_HUB
    if (host_dev->parent == NULL) {
        usb_host_free(usb_id);
    }
#else
    usb_host_free(usb_id);
#endif
    r_printf("7");
    return 0;
}
int usb_mutex_init(struct usb_host_device *host_dev)
{
    OS_MUTEX *mutex = zalloc(sizeof(OS_MUTEX));
    ASSERT(mutex, "usb alloc mutex error");
    host_dev->mutex = mutex;
    os_mutex_create(host_dev->mutex);
    return 0;
}
int usb_mutex_pend(struct usb_host_device *host_dev, u32 timeout)
{
    if (host_dev->mutex == NULL) {
        return 1;
    }
    int ret = os_mutex_pend(host_dev->mutex, timeout);
    if (ret) {
        r_printf("usb_mutex_pend %d ", ret);
    }
    return ret;
}
int usb_mutex_post(struct usb_host_device *host_dev)
{
    if (host_dev->mutex == NULL) {
        return 1;
    }
    int ret = os_mutex_post(host_dev->mutex);
    if (ret) {
        r_printf("usb_mutex_post %d ", ret);
    }
    return 0;
}
int usb_mutex_del(struct usb_host_device *host_dev)
{
    if (host_dev->mutex == NULL) {
        return 0;
    }
    os_mutex_del(host_dev->mutex, 1);
    free(host_dev->mutex);
    host_dev->mutex = NULL;
    return 0;
}

/**
 * @brief usb_descriptor_parser — iterate interfaces and dispatch to registered class drivers
 *
 * Traverses the config descriptor. For each interface, iterates the
 * .usb.host.class linker section calling match()->parser(). The
 * first matching class driver claims the interface and parses its
 * endpoint descriptors.
 *
 * @param host_dev    the host device being enumerated
 * @param pBuf        config descriptor buffer
 * @param total_len   total length of config descriptor
 * @param device_desc device descriptor
 *
 * @return 0 on success (at least one class claimed), non-zero on failure
 */
static int usb_descriptor_parser(struct usb_host_device *host_dev, const u8 *pBuf, u32 total_len, struct usb_device_descriptor *device_desc)
{
    int len = 0;
    u8 interface_num = 0;

    struct usb_config_descriptor *cfg_desc = (struct usb_config_descriptor *)pBuf;

    if (cfg_desc->bDescriptorType != USB_DT_CONFIG ||
        cfg_desc->bLength < USB_DT_CONFIG_SIZE) {
        log_warn("invalid descriptor for config bDescriptorType = %d bLength= %d",
                 cfg_desc->bDescriptorType, cfg_desc->bLength);
        return -USB_DT_CONFIG;
    }

    log_info("idVendor %x idProduct %x", device_desc->idVendor, device_desc->idProduct);

    len += USB_DT_CONFIG_SIZE;
    pBuf += USB_DT_CONFIG_SIZE;

    int i;
    u32 have_find_valid_class = 0;
    while (len < total_len) {
        if (interface_num >= MAX_HOST_INTERFACE) {
            log_warn("interface_num too much");
            break;
        }

        struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
        if (interface->bDescriptorType == USB_DT_INTERFACE) {

            log_info("inf class %x subclass %x ep %d",
                     interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bNumEndpoints);

            bool matched = false;
#if TCFG_HOST_UVC_ENABLE
            /* UVC not in linker section, handle here */
            if (interface->bInterfaceClass == USB_CLASS_VIDEO) {
                i = usb_uvc_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
                matched = true;
            }
#endif
            for (const usb_host_class_item_t *p = __usb_host_class_start;
                 p < __usb_host_class_end; p++) {
                if (p->match(device_desc, interface,
                             (const u8 *)cfg_desc, cfg_desc->wTotalLength)) {
                    i = p->parser(host_dev, interface_num, pBuf);
                    if (i < 0) {
                        log_warn("---usb_descriptor_parser %d---", __LINE__);
                        len = total_len;
                    } else {
                        interface_num++;
                        len += i;
                        pBuf += i;
                        have_find_valid_class = true;
                    }
                    matched = true;
                    break;
                }
            }

            if (!matched) {
                log_warn("find unsupport [class %x subClass %x] @ interface %d",
                         interface->bInterfaceClass,
                         interface->bInterfaceSubClass,
                         interface_num);

                len += USB_DT_INTERFACE_SIZE;
                pBuf += USB_DT_INTERFACE_SIZE;
            }
        } else {
            /* log_error("unknown section %d %d", len, pBuf[0]); */
            if (pBuf[0]) {
                len += pBuf[0];
                pBuf += pBuf[0];
            } else {
                len = total_len;
            }
        }
    }


    log_debug("len %d total_len %d", len, total_len);
    return !have_find_valid_class;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_suspend
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
void usb_host_suspend(const usb_dev usb_id)
{
    usb_h_entry_suspend(usb_id);
}

void usb_host_resume(const usb_dev usb_id)
{
    usb_h_resume(usb_id);
}

static bool _str_equals(const char *a, const char *b)
{
    if (a[0] == '\0' || b[0] == '\0') {
        return false;
    }
    for (int i = 0; i < 8; i++) {
        if (a[i] != b[i]) {
            return false;
        }
        if (a[i] == '\0') {
            return true;
        }
    }
    return true;
}

static int usb_event_notify(const struct usb_host_device *host_dev, u32 ev)
{
    const usb_dev id = host_device2id(host_dev);
    u32 event;
    typedef struct {
        char subdev[MAX_HOST_INTERFACE][16];
    } subdev_t;
    static subdev_t itf_set[USB_MAX_HW_NUM];
    struct device_event event_ = {0};
    u32 have_post_event = 0;

    if (ev == 0) {
        event = DEVICE_EVENT_IN;
        event_.event = DEVICE_EVENT_IN;
    } else if (ev == 1) {
        event = DEVICE_EVENT_CHANGE;
        event_.event = DEVICE_EVENT_CHANGE;
    } else {
        event = DEVICE_EVENT_OUT;
        event_.event = DEVICE_EVENT_OUT;
        goto __usb_event_out;
    }
    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        memset(itf_set[id].subdev[i], 0, sizeof(itf_set[id].subdev[i]));
        if (host_dev->interface_info[i]) {
            u8 if_class = host_dev->interface_info[i]->ctrl->interface_class;

#if TCFG_HOST_UVC_ENABLE
            if (if_class == USB_CLASS_VIDEO) {
                sprintf(itf_set[id].subdev[i], "uvc%d", id);
            } else
#endif
                /* Look up class item by interface_class */
                for (const usb_host_class_item_t *p = __usb_host_class_start;
                     p < __usb_host_class_end; p++) {
                    if (p->interface_class == if_class) {
                        sprintf(itf_set[id].subdev[i], p->event_name, id);
                        break;
                    }
                }

            /* Dedup: skip if same event_name already sent */
            if (itf_set[id].subdev[i][0]) {
                bool dup = false;
                for (u8 j = 0; j < i; j++) {
                    if (_str_equals(itf_set[id].subdev[i], itf_set[id].subdev[j])) {
                        dup = true;
                        memset(itf_set[id].subdev[i], 0, sizeof(itf_set[id].subdev[i]));
                        break;
                    }
                }
                if (!dup) {
                    log_info("event %x interface %x class %x %s",
                             event, i, if_class, itf_set[id].subdev[i]);
                    usb_driver_event_from_host(DEVICE_EVENT_FROM_USB_HOST, event, itf_set[id].subdev[i]);
                    have_post_event = 1;

                    event_.arg = (void *)"usb_host0";
                    event_.value = (int)itf_set[id].subdev[i];
                    device_event_notify(DEVICE_EVENT_FROM_USB_HOST, &event_);
                }
            }
        }
    }

__usb_event_out:
    if (event == DEVICE_EVENT_OUT) {
        for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
            if (!itf_set[id].subdev[i][0]) {
                continue;
            }
            bool dup = false;
            for (u8 j = 0; j < i; j++) {
                if (_str_equals(itf_set[id].subdev[i], itf_set[id].subdev[j])) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                have_post_event = 1;
                usb_driver_event_from_host(DEVICE_EVENT_FROM_USB_HOST, event, itf_set[id].subdev[i]);

                event_.arg = (void *)"usb_host0";
                event_.value = (int)itf_set[id].subdev[i];
                device_event_notify(DEVICE_EVENT_FROM_USB_HOST, &event_);
            }
        }
    }

    if (have_post_event) {
        return DEV_ERR_NONE;
    } else {
        return DEV_ERR_UNKNOW_CLASS;
    }

}
typedef enum {
    USB_ENUM_INIT = 0,
    USB_ENUM_GET_DEV_DESC1,
    USB_ENUM_SET_ADDR,
    USB_ENUM_GET_DEV_DESC2,
    USB_ENUM_GET_CFG_DESC_HDR,
    USB_ENUM_GET_CFG_DESC_FULL,
    USB_ENUM_SET_CONFIG,
    USB_ENUM_PARSE_DESC,
    USB_ENUM_DONE,
} usb_enum_state_t;



static u32 _usb_host_mount(const usb_dev usb_id, u32 port,
                           u32 retry, u32 reset_delay, u32 mount_timeout)
{
    u32 ret = DEV_ERR_NONE;
    struct usb_host_device *host_dev;
    struct usb_private_data *private_data;

    u32 speed = USB_IS_HS_MODE ? USB_SPEED_HIGH : USB_SPEED_FULL;

    host_dev = (port == 0) ?
               &host_devices[usb_id] :
               host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];

    private_data = &host_dev->private_data;

    usb_host_clock_lock();

    u8 *desc_buf = NULL;


    for (int i = 0; i < retry; i++) {

        usb_enum_state_t state = USB_ENUM_INIT;

        struct usb_device_descriptor device_desc;
        struct usb_config_descriptor cfg_desc;

        memset(&device_desc, 0, sizeof(device_desc));
        memset(&cfg_desc, 0, sizeof(cfg_desc));

        private_data->status = 0;
        private_data->devnum = 0;
        private_data->ep0_max_packet_size = 8;

        while (state != USB_ENUM_DONE) {

            switch (state) {

            case USB_ENUM_INIT:
                if (port == 0) {
                    usb_h_sie_init(usb_id);

                    ret = usb_host_init(usb_id, reset_delay, mount_timeout, speed, 1);
                    if (ret) {
                        log_error("usb_h_chirp_and_reset error 0x%x", ret);
                        break;
                    }

                    //ep0 的buf是全局buf，不需要释放
                    void *ep0_dma = usb_h_alloc_ep_buffer(usb_id, 0, 64);
                    usb_set_dma_taddr(usb_id, 0, ep0_dma);

                    usb_sie_enable(usb_id);
                    usb_mdelay(reset_delay);
                }
                state = USB_ENUM_GET_DEV_DESC1;
                break;

            case USB_ENUM_GET_DEV_DESC1:
                ret = usb_get_device_descriptor(host_dev, &device_desc);
                state = USB_ENUM_SET_ADDR;
                break;

            case USB_ENUM_SET_ADDR:
                usb_mdelay(20);

                u8 devnum = (port == 0) ? 0xF : port;
                ret = set_address(host_dev, devnum);
                if (ret) {
                    log_error("set_address error 0x%x", ret);
                    break;
                }

                private_data->devnum = devnum;
                state = USB_ENUM_GET_DEV_DESC2;
                break;


            case USB_ENUM_GET_DEV_DESC2:
                usb_mdelay(20);
                ret = usb_get_device_descriptor(host_dev, &device_desc);
                if (ret) {
                    log_error("usb_get_device_descriptor error 0x%x", ret);
                    break;
                }

                private_data->ep0_max_packet_size = device_desc.bMaxPacketSize0;
                state = USB_ENUM_GET_CFG_DESC_HDR;
                break;

            case USB_ENUM_GET_CFG_DESC_HDR:
                /* Step 1: read 9-byte config descriptor header */
                ret = get_config_descriptor(host_dev, &cfg_desc, USB_DT_CONFIG_SIZE);
                if (ret) {
                    usb_mdelay(50); /* camera settle, retry once */
                    ret = get_config_descriptor(host_dev, &cfg_desc, USB_DT_CONFIG_SIZE);
                }
                if (ret) {
                    log_error("get_config_descriptor error 0x%x", ret);
                    break;
                }
                if (cfg_desc.wTotalLength > 4096) {
                    log_error("cfg_desc too large wTotalLength=%d (max 4096)", cfg_desc.wTotalLength);
                    ret = -DEV_ERR_INVALID_DATA;
                    break;
                }
                desc_buf = zalloc(cfg_desc.wTotalLength + 16);
                state = USB_ENUM_GET_CFG_DESC_FULL;
                break;

            case USB_ENUM_GET_CFG_DESC_FULL:
                /* Step 2: read full wTotalLength bytes */
                ret = get_config_descriptor(host_dev, desc_buf, cfg_desc.wTotalLength);
                if (ret) {
                    log_error("get_config_descriptor full error 0x%x", ret);
                    break;
                }
                state = USB_ENUM_SET_CONFIG;
                break;

            case USB_ENUM_SET_CONFIG:
                ret = set_configuration(host_dev);
                if (ret) {
                    log_warn("set_configuration error 0x%x", ret);
                    usb_mdelay(20);
                    break;
                }

                state = USB_ENUM_PARSE_DESC;
                break;

            case USB_ENUM_PARSE_DESC:
                put_buf(desc_buf, cfg_desc.wTotalLength);
                ret = usb_descriptor_parser(
                          host_dev,
                          desc_buf,
                          cfg_desc.wTotalLength,
                          &device_desc);

                if (ret) {
                    log_warn("usb_descriptor_parser error 0x%x", ret);
                    break;
                }

                state = USB_ENUM_DONE;
                break;

            default:
                ret = -DEV_ERR_UNKNOW;
                break;
            }

            if (ret) {
                if (ret == -DEV_ERR_OFFLINE) {
                    goto __exit_fail;
                }
                break;
            }
        }

        //每一轮都需要释放内存
        if (desc_buf) {
            free(desc_buf);
            desc_buf = NULL;
        }

        if (state == USB_ENUM_DONE) {
            private_data->status = 1;

            for (int itf = 0; itf < MAX_HOST_INTERFACE; itf++) {
                if (host_dev->interface_info[itf] &&
                    host_dev->interface_info[itf]->ctrl->set_power) {
                    host_dev->interface_info[itf]->ctrl->set_power(host_dev, 1);
                }
            }

            usb_host_clock_unlock(NULL);
            return DEV_ERR_NONE;
        }

        reset_delay += 10;
    }

__exit_fail:

    log_error("usb probe error 0x%x", ret);
    if (desc_buf) {
        free(desc_buf);
        desc_buf = NULL;
    }
    private_data->status = 0;
    usb_sie_close(usb_id);
    usb_host_clock_unlock(NULL);
    return ret;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_mount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
u32 usb_host_mount(const usb_dev id, u32 port, u32 retry, u32 reset_delay, u32 mount_timeout)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif

    u32 ret;
    struct usb_host_device *host_dev;
    if (port == 0) {
        host_dev = &host_devices[usb_id];
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
    }

    struct usb_private_data *private_data = &host_dev->private_data;

    /* 先检查是否已挂载，避免memset销毁未释放的资源句柄 */
    if (private_data->status == 1) {
        log_warn("repeat mount, usb%d port%d already mounted", usb_id, port);
    }

    /* 安全释放旧资源后重新初始化 */
    if (port == 0) {
        usb_mutex_del(host_dev);
        usb_sem_del(host_dev);
        memset(host_dev, 0, sizeof(*host_dev));
    }

    host_dev->private_data.usb_id = id;

    usb_sem_init(host_dev);
    usb_mutex_init(host_dev);
    usb_h_isr_reg(usb_id, 1, 0);


    ret = _usb_host_mount(usb_id, port, retry, reset_delay, mount_timeout);
    usb_otg_resume(usb_id);
    if (ret) {
        goto __exit_fail;
    }
#if TCFG_USB_TEST_ENABLE
    usb_test_post_start(host_dev);
#endif

    return usb_event_notify(host_dev, 0);

__exit_fail:
    usb_sie_disable(usb_id);
    usb_mutex_del(host_dev);
    usb_sem_del(host_dev);
    return ret;
}

static u32 _usb_host_unmount(const usb_dev usb_id, u32 port)
{
    struct usb_host_device *host_dev;
    if (port == 0) {
        host_dev = &host_devices[usb_id];
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
    }

    struct usb_private_data *private_data = &host_dev->private_data;
    if (private_data->status == 0) {
        log_warn("repeat unmount, usb%d port%d already unmounted", usb_id, port);
        return 0;
    }
    private_data->status = 0;

    usb_sem_post(host_dev);//拔掉设备时，让读写线程快速释放

    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i]) {
            if (host_dev->interface_info[i]->ctrl->set_power) {
                host_dev->interface_info[i]->ctrl->set_power(host_dev, 0);
            }
            if (host_dev->interface_info[i]->ctrl->release) {
                host_dev->interface_info[i]->ctrl->release(host_dev);
            }
            host_dev->interface_info[i] = NULL;
        }
    }

    if (port == 0) {
        usb_sie_close(usb_id);
    }
    return DEV_ERR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_unmount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
/* u32 usb_host_unmount(const usb_dev usb_id, char *device_name) */
u32 usb_host_unmount(const usb_dev id, u32 port)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;
    struct usb_host_device *host_dev;
    if (port == 0) {
        host_dev = &host_devices[usb_id];
    } else {
        host_dev = host_devices[usb_id].interface_info[0]->dev.hub->child_dev[port];
    }

#if (TCFG_UDISK_ENABLE && UDISK_READ_512_ASYNC_ENABLE)
    _usb_stor_async_wait_sem(host_dev);
#endif
    ret = _usb_host_unmount(usb_id, port);
    if (ret) {
        goto __exit_fail;
    }
    usb_mutex_del(host_dev);
    usb_sem_del(host_dev);

    /* printf("usb_host_unmount notify >>>>>>>>>>>\n"); */
    usb_event_notify(host_dev, 2);
    return DEV_ERR_NONE;

__exit_fail:
    return ret;
}

u32 usb_host_remount(const usb_dev id, u32 port, u32 retry, u32 delay, u32 ot, u8 notify)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;

    ret = _usb_host_unmount(usb_id, port);
    if (ret) {
        goto __exit_fail;
    }

    struct usb_host_device *host_dev = &host_devices[usb_id];
    os_sem_set(host_dev->sem, 0);

    //设备掉线时,otg会把ctrl_ie 和 sof_ie都关掉
    usb_h_isr_reg(usb_id, 1, 0);
    ret = _usb_host_mount(usb_id, port, retry, delay, ot);
    if (ret) {
        goto __exit_fail;
    }

    if (notify) {
        usb_event_notify(host_dev, 1);
    }
    return DEV_ERR_NONE;

__exit_fail:
    return ret;
}

int usb_host_force_reset(const usb_dev usb_id)
{
    //TODO:预留以后加上hub的处理
    usb_h_force_reset(usb_id, 20);
    return 0;
}

static u16 clock_lock_timer;
void usb_host_clock_unlock(void *priv)
{
#ifdef USB_HW_20
    if (clock_lock_timer) {
        clock_unlock("usb_host");
        sys_timeout_del(clock_lock_timer);
        clock_lock_timer = 0;
    }
#endif
}

void usb_host_clock_lock(void)
{
#ifdef USB_HW_20

    if (clock_lock_timer) {
        sys_timer_re_run(clock_lock_timer);
        return;
    }
    clock_lock("usb_host", clk_get_max_frequency());
    clock_lock_timer = sys_timeout_add(NULL, usb_host_clock_unlock, 3000);
#endif
}
#endif
