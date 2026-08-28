#ifndef AI_AUDIO_PLAYER_H
#define AI_AUDIO_PLAYER_H

#include "ai_rx_player.h"

int ai_audio_player_open(u8 *bt_addr,
                         u8 source,
                         struct ai_rx_player_param *param,
                         enum stream_node_state(*rx_func)(void *file, struct stream_frame **pframe),
                         void (*event_cb)(u8 source, int event, void *priv),
                         void *event_priv);
void ai_audio_player_close(u8 source);
int ai_audio_player_get_delay(u8 source);
bool ai_audio_player_running(u8 source);

#endif
