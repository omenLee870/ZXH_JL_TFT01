#ifndef PET_REVERSE_NOTICE_H
#define PET_REVERSE_NOTICE_H

#ifdef PET_REVERSE_NOTICE_TEST
#include <stdbool.h>
#else
#include "generic/typedef.h"
#endif
#include <stdint.h>

/* UI 投递请求；实际蓝牙和播放器操作均在 app_core 执行。 */
int pet_reverse_notice_request(bool enter);
void pet_reverse_notice_tick(uint32_t now_ms);
bool pet_reverse_notice_preserve_music_view(void);

#ifdef PET_REVERSE_NOTICE_TEST
void pet_reverse_notice_handle(bool enter, uint32_t now_ms);
void pet_reverse_notice_done(uint32_t generation);
bool pet_reverse_notice_media_state(uint8_t address[6], bool *playing);
int pet_reverse_notice_media_command(bool play);
bool pet_reverse_notice_resource_ready(void);
int pet_reverse_notice_play(uint32_t generation);
void pet_reverse_notice_stop(uint32_t generation);
#endif
#endif
