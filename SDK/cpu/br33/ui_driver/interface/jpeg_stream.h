#ifndef __jpeg_stream__
#define __jpeg_stream__

#include "app_config.h"
int jljpeg_stream_init();
int jljpeg_stream_deinit();

void jljpeg_stream_src_data_lock();
void jljpeg_stream_src_data_unlock();
int jljpeg_stream_src_data_lock_check();

int jljpeg_stream_src_data_copy(u8 *buf, int size, int copy);
int jljpeg_stream_src_data_len_get();
u8 *jljpeg_stream_src_data_get();

#endif// __jpeg_stream__
