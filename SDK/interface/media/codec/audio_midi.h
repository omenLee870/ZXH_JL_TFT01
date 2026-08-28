#ifndef _AUDIO_MIDI_H
#define _AUDIO_MIDI_H
#include "codec/midi_lib_api.h"
#include "codec/MIDI_DEC_API.h"
#include "codec/MIDI_CTRL_API.h"


int midi_get_cfg_addr(u8 *timbre_file_path, void **addr);


int midi_w2s_get_cfg_addr(void **addr);
void midi_init(struct midi_init_param  *midi_param);
u32 get_sr_tab_num(u32 sr);


int midi_ctrl_param_init(midi_ctrl_open_parm *parm);
void midi_ctrl_ioctrl(void *stream,  u32 cmd, void *priv);
extern void wdt_clear();
extern void *midi_flash_sdfile_mnt;


#define  MIDI_DEC_SR   44100  //midi采样率

#endif

