#ifndef __PUBLIC_JLUI_RESOURCE_H__
#define __PUBLIC_JLUI_RESOURCE_H__

#include "gpu_format.h"

int jlui_res_get_image_info(u32 prj_id, u32 img_id, UI_RESFILE *specfile, struct image_file *image, struct flash_file_info *info);
int jlui_res_get_strpic_info(u32 prj_id, u8 mode, u16 id, struct image_file *image, struct flash_file_info *info);

int ui_resfile_check();

int ui_resfile_info_init(
    void *(*malloc)(int size, u32 ram_type, u32 module_type),
    void (*free)(void *p, u32 ram_type, u32 module_type));
int ui_resfile_info_release();

#endif
