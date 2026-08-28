#ifndef _USB_TEST_HOST_H_
#define _USB_TEST_HOST_H_

#include "system/includes.h"
#include "usb/test_usb.h"

#if TCFG_USB_TEST_ENABLE

int _usb_test_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf);
void usb_test_process(struct usb_host_device *host_dev);

int usb_test_get_local_stats(struct usb_host_device *host_dev, u8 type, struct test_ep_stats *stats);
int usb_test_get_remote_stats(struct usb_host_device *host_dev, u8 type, struct test_ep_stats *stats);
int usb_test_get_remote_in_stats(struct usb_host_device *host_dev, u8 type, struct test_ep_stats *stats);
int usb_test_reset_stats(struct usb_host_device *host_dev);

void usb_test_post_start(struct usb_host_device *host_dev);

#endif

#endif
