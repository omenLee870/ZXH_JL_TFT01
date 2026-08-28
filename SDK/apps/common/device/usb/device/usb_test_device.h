#ifndef _USB_TEST_DEVICE_H_
#define _USB_TEST_DEVICE_H_

#include "system/includes.h"
#include "usb/test_usb.h"

#if TCFG_USB_TEST_ENABLE

u32 test_register(const usb_dev usb_id);
u32 test_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);
void test_release(const usb_dev usb_id);

#endif

#endif
