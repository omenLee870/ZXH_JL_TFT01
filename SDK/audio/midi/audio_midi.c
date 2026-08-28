
#include "fs/fs.h"
#include "codec/audio_midi.h"
#include "audio_general_config.h"
#include "audio_config_def.h"

#if defined(MIDI_CTRL_DEC_ENABLE) && MIDI_CTRL_DEC_ENABLE || defined(MIDI_FILE_DEC_ENABLE) && MIDI_FILE_DEC_ENABLE
#pragma   code_seg(".audio_midi.text")
#pragma   data_seg(".audio_midi.data")
#pragma  const_seg(".audio_midi.text.const")

u32 get_sr_tab_num(u32 sr)
{
    int i = 0;
    for (i = 0; i < ARRAY_SIZE(smpl_rate_tab); i++) {
        if (sr == smpl_rate_tab[i]) {
            return i;
        }
    }
    return 4;
}
void *midi_flash_sdfile_mnt = NULL;
/*
 *通过路径获取flash文件路径簇地址
 * */
int midi_get_cfg_addr(u8 *timbre_file_path, void **addr)
{
    if (!midi_flash_sdfile_mnt) {
        midi_flash_sdfile_mnt = mount(NULL, "mnt/sdfile", "sdfile", 0, NULL);
        if (!midi_flash_sdfile_mnt) {
            log_e("=======mnt sdfile error=====\n");
        }
    }

    FILE *file = fopen((const char *)timbre_file_path, "r");
    if (!file) {
        log_e("=========MIDI.bin open err %s==========\n", timbre_file_path);
        return -1;
    }

    struct  vfs_attr attr = {0};
    fget_attrs(file, &attr);
    *addr = (void *)attr.sclust;
    fclose(file);
    return 0;
}
#endif
