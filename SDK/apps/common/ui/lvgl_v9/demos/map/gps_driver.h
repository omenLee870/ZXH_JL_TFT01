#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include "typedef.h"

typedef enum {
    GPS_ANT_OK = 0,
    GPS_ANT_OPEN,
    GPS_ANT_SHORT,
    GPS_ANT_UNKNOWN,
} gps_antenna_t;

typedef struct {
    u8 valid;
    s32 latitude_e7;
    s32 longitude_e7;
    u8 fix_mode;
    u8 satellites_used;
    u8 antenna_status;
} gps_data_t;

void gps_init(void);
void gps_deinit(void);
void gps_poll(void);
void gps_get_info(gps_data_t *out);

const char *gps_antenna_str(int status);
void gps_get_debug_counters(int *poll_cnt, int *byte_cnt);

#endif
