#include "app_config.h"
#include "jlgpu_driver.h"
#include "res/resfile.h"
#include "res_config.h"
#include "ui_resource.h"
#include "control.h"
#include "font/font_all.h"
#include "font/language_list.h"
#include "res/mem_var.h"
#include "jlui_app/ui_style.h"


#if (defined(CONFIG_UI_STYLE_JL_PUBLIC_MODLS_ENABLE) || defined(CONFIG_UI_STYLE_JL_CSC_PUBLIC_MODLS_ENABLE))


//只读文件系统配置
#if     TCFG_SDFILE_INSERT_FLASH_ENABLE
#define UI_MODE_PHY_BASE    TCFG_SDFILE_FLASH_DEV_PATY_BASE	  			//分区地址
#define UI_MODE_PHY_FLASH   PHY_JL_INSERT_FLASH				//分区物理区间
#elif 	TCFG_SDFILE_EXTERN_FLASH_ENABLE
#define UI_MODE_PHY_BASE    TCFG_SDFILE_FLASH_DEV_PATY_BASE	  		  	//分区地址
#define UI_MODE_PHY_FLASH   PHY_JL_EXTERN_FLASH				//分区物理区间
#else
#define UI_MODE_PHY_BASE 	0
#define UI_MODE_PHY_FLASH   PHY_JL_INSERT_FLASH
#endif



//vir fat 文件系统
#if     TCFG_VIRFAT_INSERT_FLASH_ENABLE
//内置flash
#define UI_FAT_PHY_FLASH  PHY_JL_INSERT_FLASH
#define UI_FAT_PHY_WATCH_BASE	TCFG_VIRFAT_FLASH_DEV_PATY_BASE
#else
//外挂flash
#define UI_FAT_PHY_FLASH  PHY_JL_EXTERN_FLASH
#define UI_FAT_PHY_WATCH_BASE  TCFG_VIRFAT_FLASH_DEV_PATY_BASE
#endif



extern u32 sfc1_flash_addr2cpu_addr(u32 offset);

int ui_res_get_image_flash_info(struct flash_file_info *image_info, struct flash_file_info *prj_info, struct image_file *img_file);
int ui_prj_info_table_release();
int ui_watch_resfile_check(void);

//字库设计放内置
struct ui_load_info ui_load_info_table[] = {
#if TCFG_NANDFLASH_DEV_ENABLE
    {0,  0, UI_MODE_PHY_FLASH, UI_MODE_PHY_BASE,  RES_PATH"JL/JL.sty", NULL},
#else
    {0,  0, UI_MODE_PHY_FLASH, UI_MODE_PHY_BASE, JL_PATH"JL.sty", NULL},
#endif
#if TCFG_NANDFLASH_DEV_ENABLE
    {1,  0, UI_FAT_PHY_FLASH, UI_FAT_PHY_WATCH_BASE, RES_PATH"watch/watch.sty", NULL},
#else
    {1,  1, UI_FAT_PHY_FLASH, UI_FAT_PHY_WATCH_BASE, RES_PATH"watch/watch.sty", NULL},
#endif
    {2,  0, UI_MODE_PHY_FLASH, UI_MODE_PHY_BASE, SIDEBAR_PATH"sidebar.sty", NULL},
    {3,  0, UI_MODE_PHY_FLASH, UI_MODE_PHY_BASE, NULL, NULL},
    {4,  0, PHY_JL_INSERT_FLASH, 0, UPGRADE_PATH"upgrade.sty", NULL},
    {-1, 0, PHY_JL_EXTERN_FLASH, 0, NULL, NULL},
};

#define UI_PRJ_RES_JL		BIT(0)
#define UI_PRJ_RES_WATCH	BIT(1)
#define UI_PRJ_RES_SIDEBAR	BIT(2)
#define UI_PRJ_RES_UPDATE	BIT(3)

#define UI_PRJ_RES_CHECK 	(UI_PRJ_RES_JL|UI_PRJ_RES_WATCH)

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_res_flash_info_get 获取文件信息
 *
 * @param file_info	输出，文件信息
 * @param path		文件路径
 * @param str		不重要，给"res"
 * @param mmu_tab	1,返回mmu_tab
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int ui_res_flash_info_get(struct flash_file_info *file_info, char *path, char *str, int mmu_tab)
{
#if TCFG_NANDFLASH_DEV_ENABLE
    return 0;
#else
    return __ui_res_flash_info_get(file_info, path, str,  UI_FAT_PHY_FLASH, UI_FAT_PHY_WATCH_BASE, mmu_tab);
#endif
}
int jlui_res_get_image_info(u32 prj_id, u32 img_id, UI_RESFILE *specfile, struct image_file *image, struct flash_file_info *info)
{
    int ret = 0;
    u32 image_id = img_id;
    u32 page_num = img_id >> 16;

    /* ret = open_image_by_id(prj_id, specfile, image, (image_id & 0xffff), (image_id >> 16)); */
    if (specfile) {
        if (open_image_by_id(prj_id, specfile, image, (image_id & 0xffff), (image_id >> 16))) {
            printf("open image by id err!\n");
            return -EFAULT;
        }

    } else {
        struct mem_var *list;
        if ((list = mem_var_search(0, 0, (image_id & 0xffff), page_num, prj_id)) != NULL) {
            mem_var_get(list, (u8 *)image, sizeof(struct image_file));
        } else {
            if (open_image_by_id(prj_id, specfile, image, (image_id & 0xffff), (image_id >> 16))) {
                printf("open image by id err!\n");
                return -EFAULT;
            }
            mem_var_add(0, 0, (image_id & 0xffff), page_num, prj_id, (u8 *)image, sizeof(struct image_file));
        }
    }

    /* if (ret) { */
    /* printf("open image by id err!\n"); */
    /* return ret; */
    /* } */
    /* printf("open image by id succ!\n"); */


    return ret;
}
int jlui_res_get_strpic_info(u32 prj_id, u8 mode, u16 id, struct image_file *image, struct flash_file_info *info)
{
    int ret;
    u32 str_id = id;
    if (mode == 0) {
        ret = open_string_pic(prj_id, image, str_id);
    } else if (mode == 1) {
        ret = open_string_pic2(prj_id, image, str_id);
    } else {
        ret = -1;
    }

    if (ret) {
        printf("open string err!\n");
        return ret;
    }
    /* printf("open string succ!\n"); */

    ui_res_get_image_flash_info(info, &ui_load_info_table[prj_id].str_file_info, image);

    return ret;
}
char *ui_get_res_path_by_pj_id(int prj)
{
    return (char *)ui_load_info_table[prj].path;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_resfile_check 资源校验
 *
 * @return 0 成功 other 失败
 *
 * @note UI_PRJ_RES_CHECK 中配置需要检查的项
 */
/* ------------------------------------------------------------------------------------*/
int ui_resfile_check()
{
#if (UI_PRJ_RES_CHECK & UI_PRJ_RES_JL)
    UI_RESFILE *fp_jl = res_fopen(ui_load_info_table[0].path, "r");
    if (!fp_jl) {
        printf("[%s] ui prj[jl] check fail", __func__);
        return -1;
    }
    res_fclose(fp_jl);
#endif
#if (UI_PRJ_RES_CHECK & UI_PRJ_RES_WATCH)
    int ret = ui_watch_resfile_check();
    if (ret == false) {
        printf("[%s] ui prj[watch] check fail", __func__);
        return -1;
    }
#endif
#if (UI_PRJ_RES_CHECK & UI_PRJ_RES_SIDEBAR)
    UI_RESFILE *fp_sidebar = res_fopen(ui_load_info_table[2].path, "r");
    if (!fp_sidebar) {
        printf("[%s] ui prj[sidebar] check fail", __func__);
        return -1;
    }
    res_fclose(fp_sidebar);
#endif
#if (UI_PRJ_RES_CHECK & UI_PRJ_RES_UPDATE)
    UI_RESFILE *fp_update = res_fopen(ui_load_info_table[4].path, "r");
    if (!fp_update) {
        printf("[%s] ui prj[update] check fail", __func__);
        return -1;
    }
    res_fclose(fp_update);
#endif
    void  rcsp_update_flag_clear();
#if TCFG_UPDATE_ENABLE
    rcsp_update_flag_clear();
#endif
    printf("[%s] ui prj check succ ", __func__);
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_resfile_info_init ui资源信息初始化
 *
 * @return 0 succ other fail
 */
/* ------------------------------------------------------------------------------------*/
int ui_resfile_info_init(
    void *(*malloc)(int size, u32 ram_type, u32 module_type),
    void (*free)(void *p, u32 ram_type, u32 module_type))
{
    //资源校验
    if (ui_resfile_check()) {
        return -1;
    }
    //注册tab到库
    ui_prj_info_table_init(ui_load_info_table, malloc, free);
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief ui_resfile_info_release 注销ui资源
 *
 * @return 0
 */
/* ------------------------------------------------------------------------------------*/
int ui_resfile_info_release()
{
    extern int ui_prj_info_table_release();
    ui_prj_info_table_release();
    return 0;
}





//**************************************************************************************//
#if  0
void ui_resfile_test_code()
{
    struct flash_file_info file_info = {0};
    struct flash_file_info image_info = {0};
    struct image_file image_f = {0};
    ui_resfile_info_init();
    int ret = open_image_by_id(0, NULL, &image_f, 1, 1);
    if (ret) {
        printf("open image file");
        return ;
    }
    ui_res_get_image_flash_info(&image_info, &ui_load_info_table[0].image_file_info, &image_f);


}
#endif

char *font_get_file_addr(char *file_name)
{
    struct vfs_attr attr;
    FILE *fp = fopen(file_name, "r");
    if (!fp) {
        printf("Failed to get font file addr!\n");
        return NULL;
    }
    fget_attrs(fp, &attr);
    fclose(fp);
#if  TCFG_SDFILE_EXTERN_FLASH_ENABLE
    return (char *) sfc1_flash_addr2cpu_addr(attr.sclust + UI_MODE_PHY_BASE);
#else
    return (char *)sdfile_flash_addr2cpu_addr(attr.sclust + UI_MODE_PHY_BASE);
#endif
}

__attribute__((always_inline))
void __font_pix_copy(struct font_info *info, u8 *pix, s16 x, s16 y, u16 height, u16 width)
{
    int i, j, h;
    u16 xpos;
    u16 ypos;
    if (info->flags & FONT_SHOW_SCROLL) {
        int font_lang_id = font_lang_get();
        if (font_lang_id == Arabic || font_lang_id == Hebrew || font_lang_id == UnicodeMixLeftword) { //显示方向从右到左
            if (x > info->text_width - info->xpos_offset) {
                return;
            }
        } else {
            if (x + width < info->xpos_offset) { //x + width防止字符被中断
                return;  //当解析字符串累计buf宽度未到指定位置时不填充字库buf
            } else {
                x -= info->xpos_offset; //偏移x的值，使其对应到字库buf起始位置
            }
        }
    }

    for (j = 0; j < (height + 7) / 8; j++) { /* 纵向8像素为1字节 */
        for (i = 0; i < width; i++) {
            u8 pixel = pix[j * width + i];
            int hh = height - (j * 8);
            if (hh > 8) {
                hh = 8;
            }

            if (x + i < 0) {
                continue; //x的值在文本框边界的时候可能为负，将负的部分舍弃
            }
            xpos = x + i;
            for (h = 0; h < hh; h++) {
                u16 clr = (pixel & BIT(h)) ? 1 : 0;
                if (clr) {
                    if (info->text_image_buf) {
                        u8 *pdisp = info->text_image_buf;
                        if (y + j * 8 + h < 0) {
                            continue;
                        }
                        ypos = y + j * 8 + h;
                        if ((ypos < info->text_image_height) && (xpos < info->text_image_width)) {
                            pdisp[ypos * info->text_image_stride + xpos / 8] |= BIT(7 - xpos % 8);
                        } else {
                            break;
                        }
                    }
                }
            } /* endof for h */
        }/* endof for i */
    }/* endof for j */
}

__attribute__((always_inline)) //new font tool, for 1 2 4 8 bit
void __new_cb_font_pix_copy(struct font_info *info, u8 *pix, s16 x, s16 y, u16 height, u16 width)
{
    if (info->flags & FONT_SHOW_SCROLL) {
        int font_lang_id = font_lang_get();
        if (font_lang_id == Arabic || font_lang_id == Hebrew || \
            font_lang_id == UnicodeMixLeftword) {
            if (x > info->text_width - info->xpos_offset) {
                return;
            }
        } else {
            if (x + width < info->xpos_offset) { //x + width防止字符被中断
                return;  //当解析字符串累计buf宽度未到指定位置时不填充字库buf
            } else {
                x -= info->xpos_offset; //偏移x的值，使其对应到字库buf起始位置
            }
        }
    }
    u8 *pdisp = (u8 *)info->text_image_buf;
    u8 *pixbuf = (u8 *)pix;
    int i, j;

    struct rect font_r;
    struct rect image_r;
    struct rect r;
    font_r.left = x;
    font_r.top = y;
    font_r.width = width;
    font_r.height = height;
    image_r.left = 0;
    image_r.top = 0;
    image_r.width = info->text_image_width;
    image_r.height = info->text_image_height;

    u8 pixel, bpp, bpp_mode;
    u32 font_bit, image_bit;
    u16 font_byte_idx, font_bit_idx;
    u16 image_byte_idx, image_bit_idx;
    if (get_rect_cover(&font_r, &image_r, &r)) {
        bpp_mode = Font_GetBitDepth();
        u16 line_pixel = 0;
        if (bpp_mode == BIT_DEPTH_1BPP) {
            bpp = 1;
            line_pixel = info->text_image_stride * 8;
        } else if (bpp_mode == BIT_DEPTH_2BPP) {
            line_pixel = info->text_image_stride * 4;
            bpp = 2;
        } else if (bpp_mode == BIT_DEPTH_4BPP) {
            line_pixel = info->text_image_stride * 2;
            bpp = 4;
        } else {
            line_pixel = info->text_image_stride;
            bpp = 8;
        }
        u8 pix_num_in_byte = 8 / bpp;
        u8 reserved_bit = 0xff >> (8 - bpp);
        int font_bit_row_start, img_bit_row_start;

        if (bpp == 1) {
            for (j = 0; j < r.height; j++) {
                font_bit_row_start = (r.top - font_r.top + j) * width + r.left - font_r.left;
                img_bit_row_start = (r.top - image_r.top + j) * line_pixel + r.left - image_r.left;
                for (i = 0; i < r.width; i++) {
                    font_bit = font_bit_row_start + i;
                    font_byte_idx = font_bit >> 3;
                    font_bit_idx = font_bit & 7;
                    image_bit = img_bit_row_start + i;
                    image_byte_idx = image_bit >> 3;
                    image_bit_idx = image_bit & 7;
                    pixel = (pixbuf[font_byte_idx] >> ((7 - font_bit_idx))) & reserved_bit;
                    pdisp[image_byte_idx] |= (pixel << ((7 - image_bit_idx)));
                }
            }
        } else {
            for (j = 0; j < r.height; j++) {
                font_bit_row_start = (r.top - font_r.top + j) * width + r.left - font_r.left;
                img_bit_row_start = (r.top - image_r.top + j) * line_pixel + r.left - image_r.left;
                for (i = 0; i < r.width; i++) {
                    font_bit = font_bit_row_start + i;
                    font_byte_idx = font_bit / pix_num_in_byte;
                    font_bit_idx = font_bit % pix_num_in_byte;
                    image_bit = img_bit_row_start + i;
                    image_byte_idx = image_bit / pix_num_in_byte;
                    image_bit_idx = image_bit % pix_num_in_byte;
                    pixel = (pixbuf[font_byte_idx] >> ((pix_num_in_byte - 1 - font_bit_idx) * bpp)) & reserved_bit;
                    pdisp[image_byte_idx] |= (pixel << ((pix_num_in_byte - 1 - image_bit_idx) * bpp));
                }
            }
        }
    }
}

void platform_putchar(struct font_info *info, u8 *pixel, u16 width, u16 height, u16 x, u16 y)
{
    if (info->each_line_width_info) {
        u8 total_line = (u8)(info->each_line_width_info[0]);
        u8 curr_line = (u8)((info->each_line_width_info[0] >> 8) + 1);
        u16 curr_line_width = info->each_line_width_info[curr_line];
        if (total_line > 1 || ((info->flags & FONT_VERTICAL_SCROLL) && info->top_extra_fill < 0)) {
            u16 x_offset = (info->text_width - curr_line_width) / 2;
            int font_lang_id = font_lang_get();
            if (font_lang_id == Arabic || font_lang_id == Hebrew || font_lang_id == UnicodeMixLeftword) {
                x -= x_offset;
            } else {
                x += x_offset;
            }
        }
    }

    if ((info->tool_version == 0) || (info->tool_version == 0x0100)) {
        __font_pix_copy(info,
                        pixel,
                        (s16)x,
                        (s16)y,
                        height,
                        width);
    } else {
        __new_cb_font_pix_copy(info,
                               pixel,
                               (s16)x,
                               (s16)y,
                               height,
                               width);
    }
}


const struct font_info font_info_table[] = {
    //gb2312
    {
        .language_id = Chinese_Simplified,   //简体中文
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)FONT_PATH"F_GB2312.PIX",
        .ascpixel.file.name = (char *)FONT_PATH"F_ASCII.PIX",
        .tabfile.name = (char *)FONT_PATH"F_GB2312.TAB",
        .isgb2312 = true,
        .bigendian = false,
        .putchar = platform_putchar,
    },

    {
        .language_id = Unicode,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)FONT_PATH"F_UNIC.PIX",
        .bigendian = false,
        .putchar = platform_putchar,
    },
    {
        .language_id = UnicodeMixRightword,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)FONT_PATH"F_UNIC.PIX",
        .bigendian = false,
        .putchar = platform_putchar,
        .ft_filename = (char *)FONT_PATH"test.ttf",
        .fontsize = 36,
    },
    {
        .language_id = UnicodeMixLeftword,
        .flags = FONT_SHOW_PIXEL | FONT_SHOW_MULTI_LINE,
        .pixel.file.name = (char *)FONT_PATH"F_UNIC.PIX",
        .bigendian = false,
        .putchar = platform_putchar,
        .ft_filename = (char *)FONT_PATH"test.ttf",
        .fontsize = 48,
    },

    {
        .language_id = 0,//不能删
    },
};


#endif


