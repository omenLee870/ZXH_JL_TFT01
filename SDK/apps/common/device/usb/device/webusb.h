
#ifndef  __WEBUSB_H__
#define  __WEBUSB_H__


u32 webusb_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num);
u32 webusb_set_wakeup_handle(void (*handle)(struct usb_device_t *usb_device));
u32 webusb_register(const usb_dev usb_id);
u32 webusb_release(const usb_dev usb_id);


#endif  /*WEBUSB_H*/




