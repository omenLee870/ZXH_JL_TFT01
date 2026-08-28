#include "system/includes.h"
#include "app_config.h"
#include "jljpeg_decode.h"
#include "res/resfile.h"
#include "ui_core.h"
#include "jlgpu_math.h"		// matrix
#include "jlgpu_driver.h"	// gpu 驱动
#include "gpu_port.h"	// GPU 接口，依赖于 jlgpu_driver.h
#include "gpu_task.h"
#include "gpu_draw.h"
#include <math.h>
#include "ui/lcd/lcd_drive.h"	// lcd 驱动
#include "asm/psram_api.h"

#define LOG_TAG_CONST      	JPEG
#define LOG_TAG     		"[JPEG]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CHAR_ENABLE
#include "debug.h"

struct jpeg_hd_manager {
    struct list_head entry;
    jdec_opj *opj;
    u32 check;
    u32 vaild;
    u32 task_id;
    u32 elm_id;
};
struct jpeg_priv {
    u32 task_id;
    u32 elm_id;
};
struct jpeg_hd_manager jpg_hd_manager_info;
#define __this (&jpg_hd_manager_info)


static spinlock_t lock;
#define ENTER_CRITICAL() 	spin_lock(&lock)
#define	EXIT_CRITICAL()		spin_unlock(&lock)

#define     UI_IO_DEBUG_0(i,x)  //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     UI_IO_DEBUG_1(i,x)  //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

int jlgpu_scheduler_async_free_jpeg_res(void *p);
int jpeg_module_opj_release_async(void *jpg);
u32 jlui_jpeg_infunc_bits_stream(struct _jdec_opj *opj, u8 *buf, u32 len);
extern const u8 norflash_ext_sfc_en;//给gpu判断互斥
extern void norflash_espi_mutex_enter();
extern void norflash_espi_mutex_exit();
extern u32 ps_ram_size[];
extern int jpg_ui_dec_hw_init(void);
extern int jpg_ui_dec_hw_deinit(void);
static volatile const u32 psram_size = (u32)ps_ram_size;

#define IS_SFC1_ADDR(addr) 		(!psram_size && (memory_in_flash(addr)))
#define DUMP_RECT(func, line, name, rect) \
	printf("[RECT] %s() %d, %s [%d, %d, %d, %d]\n", func, line, name, (rect)->left, (rect)->top, (rect)->width, (rect)->height)
//======================================================================//
//			jpeg链表管理
//	支持多jpeg显示，注意
//	1. 	多jpeg水平方向无交叉时，效率不影响（即垂直排列）
//  2.	需要全屏刷新
//	3. 	不带psram时不支持变换
//	4.	注意ram消耗
//======================================================================//
void *jpeg_hd_create_priv(u32 task_id, u32 elm_id)
{
    struct jpeg_priv *priv = (struct jpeg_priv *)jpeg_malloc(sizeof(struct jpeg_priv));
    priv->task_id = task_id;
    priv->elm_id = elm_id;
    return (void *)priv;
}
int jpeg_hd_get_priv_len()
{
    return sizeof(struct jpeg_priv);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_check 查询是否存在jpeg句柄
 *
 * @param check
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static void *jpeg_hd_manager_check(u32 check)
{
    struct jpeg_hd_manager *p;
    struct jpeg_hd_manager *rp = NULL;
    ENTER_CRITICAL();
    list_for_each_entry(p, &__this->entry, entry) {
        if (p->check == check) {
            rp = p;
            break;
        }
    }
    EXIT_CRITICAL();
    if (rp) {
        log_debug("%s hd:0x%x opj:0x%x check:0x%x", __func__, (u32)rp, (u32)rp->opj, check);
    } else {

        log_debug("%s check:0x%x", __func__, check);
    }
    return (void *)rp;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_find_by_opj 判断句柄是否存在
 *
 * @param opj
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static void *jpeg_hd_manager_find_by_opj(void *opj)
{
    struct jpeg_hd_manager *p;
    struct jpeg_hd_manager *rp = NULL;
    ENTER_CRITICAL();
    list_for_each_entry(p, &__this->entry, entry) {
        if (p->opj == opj) {
            rp = p;
            break;
        }
    }
    EXIT_CRITICAL();
    if (rp) {
        log_debug("%s hd:0x%x opj:0x%x check:0x%x", __func__, (u32)rp, (u32)rp->opj, (u32)opj);
    } else {

        log_debug("%s check:0x%x", __func__, (u32)opj);
    }
    return (void *)rp;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_find_by_id 通过id查找句柄
 *
 * @param task_id
 * @param elm_id
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static void *jpeg_hd_manager_find_by_id(u32 task_id, u32 elm_id)
{
    struct jpeg_hd_manager *p;
    struct jpeg_hd_manager *rp = NULL;
    ENTER_CRITICAL();
    list_for_each_entry(p, &__this->entry, entry) {
        if ((p->task_id == task_id)
            && (p->elm_id == elm_id)) {
            rp = p;
            break;
        }
    }
    EXIT_CRITICAL();
    if (rp) {
        log_debug("%s hd:0x%x opj:0x%x task_id:0x%x elm_id:0x%x \n", __func__, (u32)rp, (u32)rp->opj, task_id, elm_id);
    } else {

        log_debug("%s task_id:0x%x elm_id:0x%x \n", __func__, task_id, elm_id);
    }
    return (void *)rp;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_add 添加jpeg句柄记录
 *
 * @param opj
 * @param check
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static struct jpeg_hd_manager *jpeg_hd_manager_add(void *opj, u32 check)
{
    struct jpeg_hd_manager *p = jpeg_malloc(sizeof(struct jpeg_hd_manager));
    memset(p, 0, sizeof(struct jpeg_hd_manager));
    p->opj = (jdec_opj *)opj;
    p->check = check;
    p->vaild = 0;
    log_debug("%s hd:0x%x opj:0x%x check:0x%x", __func__, (u32)p, (u32)p->opj, check);
    ENTER_CRITICAL();
    list_add_tail(&p->entry, &__this->entry);
    EXIT_CRITICAL();
    return p;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_del 删除jpeg句柄记录
 *
 * @param opj
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int jpeg_hd_manager_del(void *opj)
{
    log_debug("%s opj:0x%x ", __func__, (u32)opj);
    struct jpeg_hd_manager *p, *n;
    ENTER_CRITICAL();
    list_for_each_entry_safe(p, n, &__this->entry, entry) {
        if (p->opj == opj) {
            list_del(&(p->entry));
            jpeg_module_opj_release_async((void *)p->opj);
            jpeg_free(p);
            break;
        }
    }
    EXIT_CRITICAL();
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_clr 清空所有jpeg句柄记录
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int jpeg_hd_manager_clr()
{
    log_debug("%s %d", __func__, __LINE__);
    struct jpeg_hd_manager *p, *n;
    ENTER_CRITICAL();
    list_for_each_entry_safe(p, n, &__this->entry, entry) {
        list_del(&(p->entry));
        jpeg_module_opj_release_async((void *)p->opj);

        jpeg_free(p);
    }
    EXIT_CRITICAL();
    log_debug("%s %d", __func__, __LINE__);
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_vaild_set 有效性设置
 *
 * @param opj
 * @param vaild
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int jpeg_hd_manager_vaild_set(void *opj, int vaild)
{
    log_debug("%s opj:0x%x", __func__, (u32)opj);
    struct jpeg_hd_manager *p, *n;
    ENTER_CRITICAL();
    list_for_each_entry_safe(p, n, &__this->entry, entry) {
        if (p->opj == opj) {
            p->vaild = vaild;
            log_debug("%s hd:0x%x opj:0x%x svaild:%d pvaild:0x%x", __func__, (u32)p, (u32)opj, vaild, p->vaild);
            break;
        }
    }
    EXIT_CRITICAL();
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_hd_manager_vaild_sub 有效性--
 *
 * @param opj
 * @param vaild
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
static int jpeg_hd_manager_vaild_sub(void *opj)
{
    log_debug("%s opj:0x%x", __func__, (u32)opj);
    struct jpeg_hd_manager *p, *n;
    ENTER_CRITICAL();
    list_for_each_entry_safe(p, n, &__this->entry, entry) {
        if (p->opj == opj) {
            if (p->vaild) {
                p->vaild --;
            }
            log_debug("%s hd:0x%x opj:0x%x pvaild:0x%x", __func__, (u32)p, (u32)opj,  p->vaild);
            break;
        }
    }
    EXIT_CRITICAL();
    return 0;
}

//======================================================================//
//			jpeg句柄管理
//======================================================================//
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_free_res_cb jpeg资源释放接口，gpu_task调用
 *
 * @param p
 */
/* ------------------------------------------------------------------------------------*/
void jpeg_module_free_res_cb(void *p)
{
    jdec_opj *opj = (jdec_opj *)p;
    if (opj) {
        /*释放图片资源*/
        jpeg_free(opj->device);
        /*释放底层*/
        jljpeg_decode_release(opj, 0);
        jpg_ui_dec_hw_deinit(); // 释放JPEG解码硬件引用
        jpeg_hd_manager_del(p);
        /*释放句柄*/
        jpeg_free(opj);
        /*如果全局句柄与当前一致，释放，如果不一致说明已经切换为新的句柄，就不需再清*/
        if (jljpeg_get_decode_hd() == opj) {
            jljpeg_get_decode_hd_clr();
        }
    }
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_opj_release_async 异步释放接口，post到gpu_task执行
 *
 * @param opj
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_module_opj_release_async(void *opj)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    log_debug("%s 0x%x opj:%x", __func__, rets, opj);

    if (opj) {
        /*同步到gpu线程释放，避免gpu在使用jpeg资源*/
        jlgpu_scheduler_async_free_jpeg_res(opj);
    }
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_opj_init jpeg句柄初始化（初始化硬件、解析文件头）
 *
 * @param priv
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_module_opj_init(void *priv)
{
    jdec_opj *opj = (jdec_opj *)priv;
    void *dev = opj->device;
    jpg_ui_dec_hw_init(); // 确保JPEG解码硬件已按需初始化
    int ret = jljpeg_decode_init(opj);
    if (!ret) {
        ret = jljpeg_img_info_get(opj, jlui_jpeg_infunc_bits_stream, dev);
        ASSERT(!ret, "jpeg file notsupport err:%x", ret);
    }
    log_debug("%s %d", __func__, ret);
    return ret;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_opj_create 创建jpeg句柄
 *
 * @param dev
 * @param dev_len
 * @param check
 * @param index
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void *jpeg_module_opj_create(void *dev, int dev_len, u32 check, u32 index, u32 task_id, u32 elm_id)
{
    int ret;
    struct jpeg_hd_manager *hd = jpeg_hd_manager_check(check);
    jdec_opj *opj = NULL;
    if (!hd) {
        opj = jpeg_malloc(sizeof(jdec_opj));
        hd  = jpeg_hd_manager_add(opj, check);
        opj->device = jpeg_malloc(dev_len);
        memcpy(opj->device, dev, dev_len);
        ret = jpeg_module_opj_init((void *)opj);
        if (ret) {
            log_debug("%s %d", __func__, __LINE__);
            jpeg_module_opj_release_async((void *)opj);
            opj = NULL;
        }
        hd->task_id = task_id;
        hd->elm_id = elm_id;
    } else {
        opj = hd->opj;
    }
    if (opj) {
        jpeg_hd_manager_vaild_set(opj, 1);
    }
    return (void *)opj;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_opj_set_invaild_by_index 将某个jpeg句柄设为无效
 *
 * @param index
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_module_opj_set_invaild_by_index(int index)
{

    log_debug("%s %d", __func__, __LINE__);
    struct jpeg_hd_manager *p, *n;
    ENTER_CRITICAL();
    list_for_each_entry_safe(p, n, &__this->entry, entry) {
        jpeg_hd_manager_vaild_sub(p->opj);
    }
    EXIT_CRITICAL();
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_opj_invaild_clr 将无效的句柄清除
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_module_opj_invaild_clr()
{

    log_debug("%s %d", __func__, __LINE__);
    struct jpeg_hd_manager *p, *n;
    ENTER_CRITICAL();

    list_for_each_entry_safe(p, n, &__this->entry, entry) {
        if (p->vaild) {
            continue;
        }
        list_del(&(p->entry));
        jpeg_module_opj_release_async(p->opj);
        jpeg_free(p);
    }
    EXIT_CRITICAL();
    return 0;
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_opj_clr_all 清除所有句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_module_opj_clr_all()
{

    log_debug("%s %d", __func__, __LINE__);
    return jpeg_hd_manager_clr();
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_module_init 模块初始化
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_module_init()
{
    INIT_LIST_HEAD(&__this->entry);
    return 0;
}
//======================================================================//
//			jpeg数据输出输出
//======================================================================//
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlui_jpeg_infunc_bits_stream jpeg数据流
 *
 * @param opj	jpeg句柄
 * @param buf	jpeg buf
 * @param len	jpeg读取长度
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
u32 jlui_jpeg_infunc_bits_stream(struct _jdec_opj *opj, u8 *buf, u32 len)
{
    u32 rets;
    __asm__ volatile("%0 = rets":"=r"(rets));
    /* log_debug("%s jpeg_info:%x %x buf:%x rets:0x%x\n", __func__, (int)opj, (int)opj->device, (int)buf, rets); */
    //dev句柄，可以是flash_info、ram地址、文件系统句柄等，只需要将
    //偏移为opj->curr_offset长度为len数据,copy到buf,并返回实际的copy长度即可
    struct flash_file_info *image_info = (struct flash_file_info *)opj->device;
    int start_addr, end_addr, curr_addr, read_len;
    if (image_info->tab) { //使用mmu_table
        int need_len = len;
        int poffset = opj->curr_offset;
        int curr_offset = 0;
        int mmu_idx = (poffset + image_info->offset) / 4096;	//计算起始mmu_tab
        /* printf("%s offset %d end:%d tbsize:%d", __func__, image_info->offset, image_info->last_tab_data_len, image_info->tab_size / 4); */
        do {
            //当前mmu块的最大长度
            int block_end = (mmu_idx == ((image_info->tab_size / 4) - 1)) ? image_info->last_tab_data_len : 4096;
            //当前mmu块的起始偏移
            int block_offset = (image_info->offset + poffset) % 4096;
            //当前mmu块的可读长度
            int block_len = block_end - block_offset;
            //拷贝长度
            read_len = (block_len < need_len) ? block_len : need_len;
            //拷贝地址
            curr_addr = image_info->tab[mmu_idx] + block_offset;
            /* printf("%s mmu_idx:%d block_end:%d block_offset:%d block_len:%d read_len:%d need_len:%d,poffset:%d curr_offset%d\n", */
            /* __func__, mmu_idx, block_end, block_offset, block_len, read_len, need_len, poffset, curr_offset); */
            //分块拷贝
            if (buf) {
                if (norflash_ext_sfc_en && IS_SFC1_ADDR(curr_addr)) {
                    norflash_espi_mutex_enter();
                }
                memcpy(buf + curr_offset, (const u8 *)curr_addr, read_len);
                if (norflash_ext_sfc_en && IS_SFC1_ADDR(curr_addr)) {
                    norflash_espi_mutex_exit();
                }
            }
            //计算下一块mmu
            poffset += read_len;
            curr_offset += read_len;
            need_len -= read_len;
            if (mmu_idx >= ((image_info->tab_size / 4) - 1)) {//读取完所有mmu
                break;
            } else {
                mmu_idx ++;
            }
            if (need_len <= 0) {//读取完数据
                break;
            }
        } while (1);
        //返回实际读取总长度
        read_len = curr_offset;
    } else {
        //资源起始地址
        start_addr = image_info->offset;
        //结束地址
        end_addr =  image_info->offset + image_info->last_tab_data_len;
        //拷贝地址
        curr_addr = start_addr + opj->curr_offset;
        //拷贝长度
        read_len  = (curr_addr + len > end_addr) ? end_addr - curr_addr : len;
        //拷贝
        if (buf) {
            if (norflash_ext_sfc_en && IS_SFC1_ADDR(curr_addr)) {
                norflash_espi_mutex_enter();
            }
            memcpy(buf, (const u8 *)curr_addr, read_len);
            if (norflash_ext_sfc_en && IS_SFC1_ADDR(curr_addr)) {
                norflash_espi_mutex_exit();
            }
        }
    }
    /* log_debug("%s start_addr:0x%x end_addr:0x%x file_len:%d curr_addr:0x%x len:%d read_len:%d", */
    /* __func__, start_addr, end_addr, end_addr - start_addr, curr_addr, len, read_len); */
    return read_len;
}


/* ------------------------------------------------------------------------------------*/
/**
 * @brief  jpeg绘图回调
 *
 * @param id				0
 * @param dst_buf			屏显buffer
 * @param dst_r				屏显buffer rect
 * @param src_r				图片/控件尺寸
 * @param bytes_per_pixel	格式
 * @param priv				jpeg句柄
 * @param matrix			变换矩阵
 */
/* ------------------------------------------------------------------------------------*/
static void jpeg_draw_cb(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv, void *matrix)
{
    jdec_opj *opj = (jdec_opj *)priv;				//jpeg解码句柄
    if (jljpeg_get_decode_hd() && (opj != jljpeg_get_decode_hd())) {
        jljpeg_decode_reset_curr();
    }
    opj->matrix = matrix;							//获取变换矩阵
    //计算缩放系数
    float ratio_w = 1.0;
    float ratio_h = 1.0;
    if (opj->matrix) {
        gpu_matrix_t *m = matrix;
        ratio_w = m->m[0][0];//
        ratio_h = m->m[1][1];//
        /* log_debug("ratio_w: %f, ratio_h: %f\n", ratio_w, ratio_h); */
    }

    /* 计算变换后图片的输出区域 */
    struct rect jpeg_draw_src;
    memcpy(&jpeg_draw_src, src_r, sizeof(struct rect));
    //放大时为中心放大
    int center_y = src_r->top + src_r->height / 2;
    jpeg_draw_src.height = opj->height / ratio_h;
    jpeg_draw_src.top = center_y - jpeg_draw_src.height / 2;
    int center_x = src_r->left + src_r->width / 2;
    jpeg_draw_src.width = opj->width / ratio_w;
    jpeg_draw_src.left = center_x - jpeg_draw_src.width / 2;
    /* DUMP_RECT(__func__, __LINE__, "dst_r", dst_r);						//推屏buf */
    /* DUMP_RECT(__func__, __LINE__, "src_r", src_r);						//jpeg显示区域 */
    /* DUMP_RECT(__func__, __LINE__, "jpeg_draw_src", &jpeg_draw_src);		//jpeg缩放后区域 */

    /* 计算图片输出和推屏Buf的重叠区域 */
    struct rect cover_r_t;
    if (!get_rect_cover(dst_r, src_r, &cover_r_t)) {
        return;
    }
    //计算缩放后的区域*以推屏区域、jpeg缩放区域、控件显示区域的交集作为最终输出，放大时受控件尺寸限制会变成中心放大
    struct rect cover_r;
    get_rect_cover(&cover_r_t, &jpeg_draw_src, &cover_r);
    /* DUMP_RECT(__func__, __LINE__, "cover_r", &cover_r);					//重叠区域 */
    //计算jpeg解码的区域
    int draw_top = cover_r.top - jpeg_draw_src.top;							//绘制的top
    int draw_btm = cover_r.height + draw_top;								//绘制的bottom
    int draw_h = cover_r.height;											//绘制高度
    /* int dec_top = draw_top * ratio_h;										//需要解码的top */
    /* int dec_btm = ceilf((float)draw_btm * ratio_h);							//需要解码的高度，这里需要向上取 */
    /* int dec_h	 = dec_btm - dec_top;										//解码高度 */
    int max_dec_height = (opj->msy << 3) / ratio_h;							//一个mcu分块缩放后可以有效绘制的行数
    int sub_top = 0;
    int sub_height = (draw_h > max_dec_height) ? max_dec_height : draw_h;
    //按mcu快缩放后的有效高度进行分行处理
    while (sub_top + draw_top < draw_btm) {
        int dec_sub_top	= (draw_top + sub_top) * ratio_h;
        int dec_sub_btm = ceilf((float)(draw_top + sub_top + sub_height) * ratio_h);
        int dec_sub_h  = dec_sub_btm - dec_sub_top;
        /* log_debug("%s draw(%d %d) dec(%d %d)%d maxdec%d sub(%d %d) dec_sub(%d %d)", */
        /* __func__, draw_top, draw_btm, dec_top, dec_btm, dec_h, max_dec_height, sub_top, sub_height, dec_sub_top, dec_sub_btm); */
        struct rect jpeg_dec_r;												/*jpeg解码区域*/
        /* jpeg_dec_r.left = (cover_r.left - src_r->left) * ratio_w; */
        jpeg_dec_r.left = (cover_r.left - jpeg_draw_src.left) * ratio_w;
        jpeg_dec_r.top = dec_sub_top;
        jpeg_dec_r.width = cover_r.width * ratio_w;
        jpeg_dec_r.height = dec_sub_h;
        struct rect block_r;												/*缩放后显示的区域*/
        block_r.top = cover_r.top + sub_top;
        block_r.height = sub_height;
        block_r.left = dst_r->left;
        block_r.width = dst_r->width;
        /* DUMP_RECT(__func__, __LINE__, "block_r", &block_r); */
        /* DUMP_RECT(__func__, __LINE__, "jpeg_dec_r", &jpeg_dec_r); */
        u8 *disp_buf = dst_buf + (block_r.top - dst_r->top) * 2 * dst_r->width;
        /*启动解码*/
        jljpeg_decode_start(opj, &jpeg_draw_src, &jpeg_dec_r, &cover_r, &block_r, disp_buf);
        sub_top += sub_height;
        sub_height = (sub_top + sub_height + draw_top < draw_btm) ? max_dec_height : draw_btm - draw_top - sub_top;
    }
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief jpeg_draw_cb_gpu gpu绘图回调
 *
 * @param id
 * @param dst_buf
 * @param dst_r
 * @param src_r
 * @param bytes_per_pixel
 * @param priv
 * @param matrix
 */
/* ------------------------------------------------------------------------------------*/
void jpeg_draw_cb_gpu(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv, void *matrix)
{
    //先用过id获取句柄，如果获取不到就return
    struct jpeg_priv *__jpeg_priv = (struct jpeg_priv *) priv;
    struct jpeg_hd_manager *hd = jpeg_hd_manager_find_by_id(__jpeg_priv->task_id, __jpeg_priv->elm_id);
    if (!hd) {
        log_warn("%s elmid:%d task_id:%d", __func__, __jpeg_priv->elm_id, __jpeg_priv->task_id);
        return;
    }
    jpeg_draw_cb(id, dst_buf, dst_r, src_r, bytes_per_pixel, hd->opj, matrix);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief jlui_draw_jpeg_cb_exit 任务销毁时调用，释放jpeg资源
 *
 * @param priv	jpeg句柄
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
int jpeg_draw_cb_exit(void *draw_info, void *priv)
{
    pJLGPUTaskDraw_t __draw_info = (pJLGPUTaskDraw_t)draw_info;
    /*使用双任务链推屏时，在息屏前或更新资源句柄前释放jpeg句柄，避免双链表冲突*/
    void *opj = __draw_info->priv;
    __draw_info->priv = NULL;//由应用层释放
    __draw_info->priv_len = 0;
    return 0;
}

/* ------------------------------------------------------------------------------------*/
/**
 * @brief cache_gpu_input_jpeg_data_cb 	将数据拷贝到psram
 *
 * @param priv	jpeg句柄
 * @param dst	解码后的输出buf
 * @param src	NULL
 * @param len	0
 */
/* ------------------------------------------------------------------------------------*/
static void cache_gpu_input_jpeg_data_cb(void *priv, void *dst, void *src, int len)
{
    jdec_opj *jpg_hd = (jdec_opj *)priv;
    //解码
    struct rect jpeg_dec = {
        .left = 0,
        .top = 0,
        .width = jpg_hd->width,
        .height = jpg_hd->height,
    };
    int bytes_per_pixel = 2;
    u32 jpeg_width_align = (jpeg_dec.width + 15) / 16 * 16;
    u32 jpeg_height_align = (jpg_hd->height + 15) / 16 * 16;
    u32 jpeg_stride = bytes_per_pixel * jpeg_width_align ;
    u8 *tmp_buf = malloc_psram(jpeg_stride * jpeg_height_align);
    jlgpu_scheduler_wait_sync();
    extern int ui_disp_out_stride_set(u16 stride);
    ui_disp_out_stride_set(jpeg_stride);
    struct rect dst_r = {
        .left = 0,
        .top = 0,
        .width = jpeg_width_align,
        .height = jpeg_height_align,
    };
    jpeg_draw_cb(0, tmp_buf, &dst_r, &jpeg_dec, bytes_per_pixel, jpg_hd, NULL);
    UI_IO_DEBUG_1(B, 6)
    texture_line_to_tile_base(dst, tmp_buf, jpeg_width_align, jpeg_height_align, GPU_FORMAT_RGB565);
    UI_IO_DEBUG_0(B, 6)
    free_psram(tmp_buf);
}
/* ------------------------------------------------------------------------------------*/
/**
 * @brief cache_gpu_input_jpeg_data 解码jpeg图片到psram
 *
 * @param head			预留
 * @param task_param	gpu任务参数
 * @param fp			jpeg文件指针
 * @param index			任务链索引,一般为0、1
 *
 * @return
 */
/* ------------------------------------------------------------------------------------*/
void *cache_gpu_input_jpeg_data(void *head, pJLGPUTaskParam_t task_param, void *fp, int index)
{
    /* 检查是否已经有缓存，如果需要缓存jpg没在cache里，释放原来的，缓存新的 */
    void *cache_addr = gpu_cache_check(task_param->image.data_crc);
    //jpeg原始数据，nand使用
    u8 *jpeg_src_data = NULL;
    int vaild_index = index;
    /* printf("@@@@@ cache_addr: 0x%x\n crc:%d", (u32)cache_addr,task_param->image.data_crc); */
    if (!cache_addr) {//解码jpeg到psram
        jdec_opj *jpg_hd = jpeg_malloc(sizeof(jdec_opj)); //在任务销毁时释放
        jpg_hd->device = jpeg_malloc(sizeof(struct flash_file_info));//设备句柄，这里可以是文件句柄等，搭配input_steam使用
#if TCFG_NANDFLASH_DEV_ENABLE
        if (fp) {
            //文件头解析的时候比较零散，这里直接将文件拷贝到psram，解完后free
            UI_IO_DEBUG_1(B, 2);
            jpeg_src_data = jpeg_malloc(task_param->image.len);
            res_fseek(fp, (u32)task_param->image.offset, SEEK_SET);
            res_fread(fp, jpeg_src_data, task_param->image.len);
            struct flash_file_info *p_info = (struct flash_file_info *)jpg_hd->device;
            p_info->offset = (u32)jpeg_src_data;
            p_info->tab = NULL;
            p_info->tab_size = 0;
            p_info->last_tab_data_len = task_param->image.len;
            UI_IO_DEBUG_0(B, 2);
        } else {
            ASSERT(0);
        }
#else
        memcpy(jpg_hd->device, &task_param->info, sizeof(struct flash_file_info));
#endif
        UI_IO_DEBUG_1(B, 4);
        if (jpeg_module_opj_init(jpg_hd)) {//初始化jpeg头
            //异常释放资源
            jpeg_free(jpg_hd->device);
            jljpeg_decode_release(jpg_hd, 0);
            jpg_ui_dec_hw_deinit(); // 释放JPEG解码硬件引用
            jpeg_free(jpg_hd);
            if (jpeg_src_data) {
                jpeg_free(jpeg_src_data);
            }
            return NULL;
        }
        //cache
        /* u32 jpeg_stride = 2 * (jpg_hd->width + 7) / 8 * 8; */
        /* u32 jpeg_height_align = (jpg_hd->height + 7) / 8 * 8; */
        u32 jpeg_width_align = (jpg_hd->width + 15) / 16  * 16;
        u32 jpeg_stride = 2 * jpeg_width_align;
        u32 jpeg_height_align = (jpg_hd->height + 15) / 16 * 16;
        u32 jpeg_img_len = jpeg_stride  * jpeg_height_align ;
        cache_addr = gpu_cache_add_with_callback(jpg_hd, jpg_hd, jpeg_img_len,
                     task_param->image.data_crc, NULL, 0,	cache_gpu_input_jpeg_data_cb);
        //从cache同步到物理介质，后续用nocache直接访问
        psram_flush_cache(cache_addr, jpeg_img_len);
        jpeg_free(jpg_hd->device);
        jljpeg_decode_release(jpg_hd, 1);
        jpg_ui_dec_hw_deinit(); // 释放JPEG解码硬件引用
        jpeg_free(jpg_hd);
        if (jpeg_src_data) {
            jpeg_free(jpeg_src_data);
        }
        UI_IO_DEBUG_0(B, 4);
    }
    //使用nocache地址访问，提高读效率
    task_param->texture.data = psram_cache_2_no_cache(cache_addr);
    /* task_param->texture.data =(cache_addr); */
    //更新任务配置
    task_param->texture.mmu_tab_base = NULL;
    task_param->format = GPU_FORMAT_RGB565;
    task_param->texture.not_compress = 1;
    task_param->texture.adr_mode = 0;
    int width = task_param->image.width;
    int height = task_param->image.height;

    task_param->image.width = (width + 15) / 16 * 16;
    task_param->image.height = (height + 15) / 16 * 16;

    if (cache_addr) {
        gpu_cache_valid_value_set((u32)cache_addr, 3);
    }
    return task_param->texture.data;
}


