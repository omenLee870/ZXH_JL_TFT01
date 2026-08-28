#ifndef __POWER_WAKEUP_H__
#define __POWER_WAKEUP_H__

typedef enum {
    RISING_EDGE = 1,
    FALLING_EDGE,
    BOTH_EDGE,
} P33_IO_WKUP_EDGE;

void power_wakeup_init();

void p33_io_wakeup_set_callback(u32(*callback)(u32 imap, P33_IO_WKUP_EDGE edge));

#endif
