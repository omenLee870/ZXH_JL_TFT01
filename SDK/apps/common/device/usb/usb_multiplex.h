#ifndef __USB_MULTIPLEX__
#define __USB_MULTIPLEX__

#include "usb/usb.h"
#include "usb/ch9.h"
#include "usb/usb_phy.h"
#include "usb/host/usb_host.h"

struct pc_multiplex_callback {
    void *pc_mult_priv;
    void (*close_other_device)(void *pc_mult_priv);
    void (*open_other_device)(void *pc_mult_priv);
};

int usb_storage_multiplex_resume(const struct usb_host_device *host_dev);
int usb_storage_multiplex_suspend(const struct usb_host_device *host_dev);
void usb_storage_multiplex_host_released(const struct usb_host_device *host_dev);
int pc_multiplex_resume();
int pc_multiplex_suspend();
void pc_multiplex_register_callback(struct pc_multiplex_callback *callback);
void pc_multiplex_close_other_device();
void pc_multiplex_open_other_device();

#endif
