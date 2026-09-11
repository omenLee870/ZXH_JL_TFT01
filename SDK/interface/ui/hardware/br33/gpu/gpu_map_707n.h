/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file gpu_map_707n.h
 *
 * @brief GPU驱动API映射，兼容707n(BR35) SDK
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2025-12-10
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __PUBLIC_GPU_MAP_707N_H__
#define __PUBLIC_GPU_MAP_707N_H__


/* GPU API */
#include "gpu.h"

#define PRINT	//printf /**< 调试打印 */

/* GPU_GET_IMAGE_STRIDE 映射到 BR33 的图片 stride 计算函数 */
#if CONFIG_LVGL_UI_ENABLE
#define GPU_GET_IMAGE_STRIDE(fmt, img, adr, cmp, cmp_size) \
	lvgl_res_get_image_stride(fmt, img, adr, cmp, cmp_size)
#else
#define GPU_GET_IMAGE_STRIDE(fmt, img, adr, cmp, cmp_size) \
	res_get_image_stride(fmt, img, adr, cmp, cmp_size)
#endif


/* alpha值从(0~100)映射到硬件设计的范围内 */
#define ALPHA_MAP(alpha)				((u8)((((u32)alpha) & 0xff) * 0xff / 100))


/* -------------------- 结构体、API 映射 -------------------- */
#define JLGPUTaskUnit_t					JLGPUChainTaskUnit_t  /**< 任务单元类型映射 */
#define pJLGPUTaskUnit_t				pJLGPUChainTaskUnit_t  /**< 任务单元指针类型映射 */
#define JLGPUTaskHead_t					JLGPUChainHead_t  /**< 任务链表头类型映射 */
#define pJLGPUTaskHead_t				pJLGPUChainHead_t  /**< 任务链表头指针类型映射 */


/* --------------------- GPU任务 API 映射 -------------------- */
#define jlgpu_set_matrix(task, m)		jlgpu_task_set_matrix(jlgpu_chain_task_get_addr(task), m, NULL, NULL)  /**< 设置任务变换矩阵 */
#define jlgpu_get_matrix(task, m)		jlgpu_task_get_matrix(jlgpu_chain_task_get_addr(task), m, NULL, NULL)  /**< 获取任务变换矩阵 */
#define jlgpu_driver_set_bbox			jlgpu_task_set_bbox  /**< 设置任务包围盒 */
#define gpu_task_add_to_next			jlgpu_task_set_next  /**< 将任务追加到下一个 */
#define jlgpu_get_next_task(task)		jlgpu_task_get_next(jlgpu_chain_task_get_addr(task))  /**< 获取下一个任务 */
#define jlgpu_set_next_task				jlgpu_task_set_next  /**< 设置下一个任务 */
#define jlgpu_find_next_task(task)		jlgpu_task_get_next(jlgpu_chain_task_get_addr(task))  /**< 查找下一个任务 */

/* -------------------- GPU链表 API 映射 -------------------- */
#define jlgpu_unit_to_instruction		jlgpu_chain_task_get_addr  /**< 任务单元转指令地址 */
#define jlgpu_task_enable_by_id			jlgpu_chain_enable_task_by_id  /**< 根据ID使能任务 */
#define jlgpu_task_delete_by_id			jlgpu_chain_delete_task_by_id  /**< 根据ID删除任务 */
#define jlgpu_task_set_prior_by_id		jlgpu_chain_set_task_prior_by_id  /**< 根据ID设置任务优先级 */
#define jlgpu_create_task_list_head		jlgpu_chain_create_head  /**< 创建任务链表头 */
#define jlgpu_set_task_list_out_format	jlgpu_chain_head_set_out_format  /**< 设置链表输出格式 */
#define jlgpu_get_task_list_out_format	jlgpu_chain_head_out_format  /**< 获取链表输出格式 */
#define jlgpu_set_task_list_out_buf		jlgpu_chain_head_set_out_buffer  /**< 设置链表输出缓冲区 */
#define jlgpu_find_task_by_id			jlgpu_chain_search_task_by_id  /**< 根据ID查找任务 */
#define jlgpu_get_task_by_id			jlgpu_chain_search_task_by_id  /**< 根据ID获取任务 */
#define jlgpu_update_task_by_id			jlgpu_chain_update_task_by_id  /**< 根据ID更新任务 */
#define jlgpu_task_find_first_task		jlgpu_chain_base_task_unit  /**< 查找第一个任务 */
#define gpu_free_all_tasks()			;  /**< 释放所有任务（空实现） */
#define jlgpu_task_clean_up_by_id		jlgpu_chain_clean_up_task_by_id  /**< 根据ID清理任务 */
#define jlgpu_delete_task_list_head		jlgpu_chain_delete_head  /**< 删除任务链表头 */
#define jlgpu_create_task				jlgpu_chain_append_task  /**< 创建并追加任务 */
#define jlgpu_task_layer_enable			jlgpu_chain_set_all_task_enable  /**< 使能/禁用所有图层任务 */
#define jlgpu_task_list_reorder			jlgpu_chain_reorder_all_task  /**< 链表任务重排序 */
#define jlgpu_task_list_run				jlgpu_chain_render_head  /**< 运行任务链表 */
#define jlgpu_update_task				jlgpu_chain_update_task  /**< 更新任务 */

/**
 * @brief 获取任务包围盒并转为矩形区域
 * @param taskp 任务单元指针
 * @param rect 输出的矩形区域指针
 */
static inline void jlgpu_get_task_rect(pJLGPUChainTaskUnit_t taskp, struct rect *rect)
{
    jlgpu_boundbox_t bbox;
    void *task_addr = jlgpu_chain_task_get_addr(taskp);
    jlgpu_task_get_bbox(task_addr, &bbox);
    BBOX_TO_RECT(&bbox, rect);
}

/**
 * @brief 获取纹理MMU配置和纹理地址信息
 * @param task 任务指令buf
 * @param mmu_en MMU使能输出
 * @param mmu_tab MMU表输出
 * @param tex_addr 纹理地址输出
 * @param rle_adr RLE地址输出
 * @param tex_stride 纹理步长输出
 */
static inline void jlgpu_driver_get_texture_addr(void *task, u32 *mmu_en, u32 *mmu_tab, u32 *tex_addr, u32 *rle_adr, u32 *tex_stride)
{
    jlgpu_task_get_texture_mmu(task, mmu_en, mmu_tab);
    jlgpu_task_get_texture_addr(task, tex_addr, rle_adr, tex_stride);
}

/* -------------------- GPU多链 API 映射 -------------------- */
#define jlgpu_mult_task_head_idle_by_index		jlgpu_mult_chain_set_head_idle  /**< 根据索引获取空闲任务链头 */
#define jlgpu_mult_task_list_get_head_num		jlgpu_mult_chain_get_head_number  /**< 获取任务链数量 */
#define jlgpu_mult_task_head_by_index			jlgpu_mult_chain_get_head_by_id  /**< 根据索引获取任务链头 */
#define jlgpu_mult_task_head_modify_by_index	jlgpu_mult_chain_set_head_by_id  /**< 根据索引修改任务链头 */
#define jlgpu_mult_task_list_add				jlgpu_mult_chain_append_head  /**< 添加任务链 */
#define jlgpu_mult_task_list_del_by_index		jlgpu_mult_chain_del_head_by_id  /**< 根据索引删除任务链 */
#define jlgpu_mult_task_list_del				jlgpu_mult_chain_delete_head  /**< 删除任务链 */
#define jlgpu_mult_task_list_clr				jlgpu_mult_chain_clear_head  /**< 清空任务链 */
#define jlgpu_mult_task_list_get				jlgpu_mult_chain_get_head_table  /**< 获取任务链表表 */

#define jlgpu_mult_task_head_get_rect_by_index	jlgpu_mult_chain_get_head_rect_by_id  /**< 根据索引获取任务链区域 */
#define jlgpu_mult_task_head_set_rect_by_index	jlgpu_mult_chain_set_head_rect_by_id  /**< 根据索引设置任务链区域 */


/* -------------------- GPU任务配置映射 -------------------- */
/* 注意：仅在707n SDK配置时，在创建GPU任务前调用 */

#define jlgpu_task_fill_param_map_707n()    /**< 填充参数兼容 707n */ \
do { \
	PRINT("\njlgpu_task_fill_param_map_707n(), Start! %s() %d\n", __func__, __LINE__); \
	if ((task_param.premult & 0xff) != GPU_MAGIC) { \
		PRINT("jlgpu_task_fill_param_map_707n(), map premult: %d\n", task_param.premult); \
		jlgpu_task_fill_set_premult(task_param.premult); \
	} \
	if (((task_param.alpha & 0xff) != GPU_MAGIC) && \
		((task_param.red & 0xff)   != GPU_MAGIC) && \
		((task_param.green & 0xff) != GPU_MAGIC) && \
		((task_param.blue & 0xff)  != GPU_MAGIC)) { \
		PRINT("jlgpu_task_fill_param_map_707n(), map color: 0x%x, 0x%x, 0x%x, 0x%x\n", \
				ALPHA_MAP(task_param.alpha), task_param.red, task_param.green, task_param.blue); \
		jlgpu_task_fill_set_color(ALPHA_MAP(task_param.alpha), task_param.red, task_param.green, task_param.blue); \
	} else { \
		task_param.fill.color.a = ALPHA_MAP(task_param.fill.color.a); \
	} \
	/* jlgpu_task_fill_set_color(0x7f, 0x00, 0xff, 0x00); */   \
	/* printf("task_id: 0x%x, elm_id: 0x%x\n", task_param.task_id, task_param.element_id); */ \
	/* __DUMP_RECT(&task_param.draw); */  \
	if (task_param.perspective_en) { \
		jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION); \
		jlgpu_task_fill_set_trans_matrix(task_param.matrix); \
	} \
	int tranx = task_param.draw.left; \
	int trany = task_param.draw.top; \
	if (task_param.scale_en) { \
		jlgpu_task_fill_set_affine_scale(task_param.fill.tran.ratio_w, task_param.fill.tran.ratio_h); \
		int scale_w = task_param.draw.width * task_param.fill.tran.ratio_w; \
		int scale_h = task_param.draw.height * task_param.fill.tran.ratio_h; \
		tranx += ((task_param.draw.width - scale_w) / 2); \
		trany += ((task_param.draw.height - scale_h) / 2); \
		PRINT("jlgpu_task_fill_param_map_707n(), map scale. scale_w: %d, scale_h: %d, tranx: %d, trany: %d\n", \
				scale_w, scale_h, tranx, trany); \
	} \
	if (task_param.rotate_en) { \
		int cx = task_param.fill.tran.rotate_cx; \
		int cy = task_param.draw.height - task_param.fill.tran.rotate_cy; \
		if (task_param.scale_en) { \
			cx *= task_param.fill.tran.ratio_w; \
			cy *= task_param.fill.tran.ratio_h; \
		} \
		jlgpu_task_fill_set_affine_rotate(cx, cy, task_param.fill.tran.rotate_angle); \
	} \
	if (task_param.scale_en || task_param.rotate_en) { \
		if (!task_param.matrix) { \
			struct rect win; \
			jlgpu_get_win_rect(&win); \
			tranx += rect_left(&win); \
			trany += rect_top(&win); \
		} \
		jlgpu_task_fill_set_trans_mode(JLGPU_TRANS_MODE_AFFINE); \
		jlgpu_task_fill_set_affine_translate(tranx, trany); \
	} \
	if (!task_param.draw_en) { \
		jlgpu_shift_rect(&task_param.draw); \
	} \
	PRINT("jlgpu_task_fill_param_map_707n(), End!\n\n"); \
} while (0)


#define jlgpu_task_texture_big_endian_enable	jlgpu_task_img_set_endian  /**< 纹理大端使能 */

#define jlgpu_task_texture_param_map_707n() /**< 纹理参数兼容 707n */ \
do { \
	PRINT("\njlgpu_task_texture_param_map_707n(), Start! %s() %d\n", __func__, __LINE__); \
	/* __DUMP_RECT(&task_param.draw); */ \
	/* __DUMP_RECT(&task_param.area); */ \
	jlgpu_task_img_set_adr_mode(JLGPU_ADRMODE_LINE); /**< 默认LINE_BASE */ \
	jlgpu_task_img_set_quality(JLGPU_RENDERING_QUALITY_MSAA_4X); /**< 设置渲染质量 */ \
	if ((task_param.premult & 0xff) != GPU_MAGIC) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map premult: %d\n", task_param.premult); \
		jlgpu_task_img_set_premult(task_param.premult); \
	} \
	if (!task_param.texture.width && !task_param.texture.height) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map size: %d x %d\n", \
				task_param.image.width, task_param.image.height); \
		jlgpu_task_img_set_size(task_param.image.width, task_param.image.height); \
	} \
	if (((task_param.texture.format & 0xff) == GPU_UNUSED) || \
		(task_param.format && !task_param.texture.format)) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map format: %d\n", task_param.format); \
		jlgpu_task_img_set_format(task_param.format); \
	} \
	if (!task_param.crop_en) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map crop! w: %d, h: %d\n", \
				task_param.image.width, task_param.image.height); \
		jlgpu_task_img_set_clip(0, 0, task_param.image.width, task_param.image.height); \
	} else { \
		/* __DUMP_RECT(&task_param.texture.crop); */ \
	} \
	if (task_param.texture.not_compress) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map not compress!\n"); \
		jlgpu_task_img_set_compress(JLGPU_COMPRESS_MODE_NONE); \
		jlgpu_task_img_set_compress_size(task_param.image.height << 16 | task_param.image.width); \
	} else if (task_param.image.len && (task_param.task_type != GPU_TASK_JPEG)) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map compress and stride!\n"); \
		PRINT("jlgpu_task_texture_param_map_707n(), image format: %d, clut_fmt: %d, cmp: %d, has_clut: %d, strpic_mode: %d, w: %d, h: %d, len: %d\n", \
				task_param.image.format, task_param.image.clut_format, task_param.image.compress, \
				task_param.image.has_clut, task_param.image.strpic_mode, task_param.image.width, \
				task_param.image.height, task_param.image.len); \
		u8 __adr, __cmp; int __cmp_size; \
		int stride = GPU_GET_IMAGE_STRIDE(task_param.format, &task_param.image, &__adr, &__cmp, &__cmp_size); \
		PRINT("jlgpu_task_texture_param_map_707n(), adr: %d, cmp: %d, cmp_size: %d, stride: %d\n", \
				__adr, __cmp, __cmp_size, stride); \
		if (__cmp < 2) { \
			jlgpu_task_img_set_stride(stride); \
		} \
		jlgpu_task_img_set_adr_mode(__adr); \
		jlgpu_task_img_set_compress(__cmp); \
		jlgpu_task_img_set_compress_size(__cmp_size); \
	} else { \
		PRINT("jlgpu_task_texture_param_map_707n(), map compress size!\n"); \
		jlgpu_task_img_set_compress_size(task_param.image.height << 16 | task_param.image.width); \
	} \
	if (task_param.texture.mmu_tab_base) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map mmu table!\n"); \
		jlgpu_task_img_set_mmu(task_param.texture.mmu_page_size, task_param.texture.mmu_tab_base); \
	} \
	if (task_param.has_clut) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map clut table!\n"); \
		u8 *clut_addr = NULL; \
		uint16_t clut_size = get_clut_format_tabsize(gpu_format_to_res(task_param.format), task_param.clut_format); \
        /* PRINT("%s(), clut: 0x%x, fmt: %d, size: %d\n", __func__, (u32)task_param.clut_tab, task_param.clut_format, clut_size); */ \
		PRINT("jlgpu_task_texture_param_map_707n(), clut size: %d\n", clut_size); \
		if (task_param.texture.mmu_tab_base) { \
			/* 纹理有mmu时，颜色表也在mmu中 */ \
			/* jlgpu_task_img_set_mmu_clut(true); */ \
            clut_addr = (u8 *)jlgpu_malloc(clut_size); \
            u32 clut = ((u32*)task_param.texture.mmu_tab_base)[0] + (u32)task_param.texture.data - clut_size; \
            memcpy(clut_addr, (u8 *)clut, clut_size); \
            /* PRINT("clut addr: 0x%x, clut: 0x%x, size: %d\n", (u32)clut_addr, clut, clut_size); */ \
		} else { \
			u8* tab_addr = (u8 *)((u32)task_param.texture.data - clut_size); \
			if ((task_param.has_clut == CLUT_TAB_FROM_RAM) && task_param.clut_tab) { \
				clut_addr = task_param.clut_tab; \
			} else if (((u32)tab_addr % 4) == 0) { \
				/* 如果地址4字节对齐，可以直接使用 */ \
				clut_addr = tab_addr; \
			} else { \
				/* 如果地址非4字节对齐，拷贝到RAM里面使用 */ \
				PRINT("jlgpu_task_texture_param_map_707n(), realloc clut cache buf!"); \
				clut_addr = (u8 *)jlgpu_malloc(clut_size); \
				memcpy(clut_addr, tab_addr, clut_size); \
			} \
		} \
		PRINT("jlgpu_task_texture_param_map_707n(), clut_format: %d, clut_tab: 0x%x, clut_size: %d\n", \
				task_param.clut_format, (u32)clut_addr, clut_size); \
		if (GPU_IMG_NEED_COLOR(task_param.format)) { \
			PRINT("jlgpu_task_texture_param_map_707n(), color: 0x%02x, 0x%02x, 0x%02x\n", \
					clut_addr[2], clut_addr[1], clut_addr[0]); \
			jlgpu_task_img_set_color(clut_addr[2], clut_addr[1], clut_addr[0]);  \
			/* jlgpu_free(clut_addr); */ \
		} else { \
			jlgpu_task_img_set_clut_en(true); \
			jlgpu_task_img_set_clut_format(task_param.clut_format); \
			jlgpu_task_img_set_clut(clut_size, clut_addr); \
		} \
    } else { \
        if (GPU_IMG_NEED_COLOR(task_param.format)) { \
            PRINT("jlgpu_task_texture_param_map_707n(), map color: 0x%02x, 0x%02x, 0x%02x\n", \
                    task_param.red, task_param.green, task_param.blue); \
            jlgpu_task_img_set_color(task_param.red, task_param.green, task_param.blue);   \
        } \
    } \
	if (task_param.perspective_en) { \
		PRINT("jlgpu_task_texture_param_map_707n(), map perspective!\n"); \
		jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION); \
		jlgpu_task_img_set_trans_matrix(task_param.matrix); \
		jlgpu_task_img_set_external_matrix(NULL); \
	} else if (task_param.matrix) { \
		PRINT("jlgpu_task_texture_param_map_707n(), inherit parent transform!\n"); \
		if (jlgpu_matrix_is_affine(task_param.matrix)) { \
			jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE); \
		} else { \
			jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_PROJECTION); \
		} \
		/* struct rect win_rect; */ \
		/* jlgpu_get_win_rect(&win_rect); */ \
		/* __DUMP_MATRIX(task_param.matrix); */ \
		/* jlgpu_matrix_translate(task_param.matrix, rect_left(&win_rect), rect_top(&win_rect)); */  \
		/* jlgpu_matrix_translate(task_param.matrix, task_param.draw.left, task_param.draw.top); */  \
		/* jlgpu_task_img_set_trans_matrix(task_param.matrix); */ \
		/* __DUMP_MATRIX(task_param.matrix); */ \
	} \
	int tranx = task_param.draw.left; \
	int trany = task_param.draw.top; \
	if (task_param.scale_en) { \
		PRINT("jlgpu_task_texture_param_map_707n(), scale w: %f, h: %f\n", \
				task_param.texture.tran.ratio_w, task_param.texture.tran.ratio_h); \
		jlgpu_task_img_set_affine_scale(task_param.texture.tran.ratio_w, task_param.texture.tran.ratio_h); \
		/* 渲染区域不必在应用层缩放，底层会通过矩阵计算渲染区域 */ \
		int draw_w = task_param.draw.width / task_param.texture.tran.ratio_w;  \
		int draw_h = task_param.draw.height / task_param.texture.tran.ratio_h;  \
		/* tranx += ((draw_w - task_param.draw.width) / 2); */  \
		/* trany += ((draw_h - task_param.draw.height) / 2); */ \
		task_param.draw.width = draw_w;  \
		task_param.draw.height = draw_h;  \
	} \
	if (task_param.rotate_en) { \
		PRINT("jlgpu_task_texture_param_map_707n(), rotate cx: %d, cy: %d, anlge: %f\n", \
				task_param.texture.tran.rotate_cx, \
				task_param.texture.tran.rotate_cy, \
				task_param.texture.tran.rotate_angle); \
		/* 旋转中心从左下角转换到左上角 */ \
		/* int cx = task_param.image.width - task_param.texture.tran.rotate_cx; */ \
		/* int cy = task_param.image.height - task_param.texture.tran.rotate_cy; */ \
		int cx = task_param.texture.tran.rotate_cx;  \
		int cy = task_param.texture.tran.rotate_cy;   \
		if (!task_param.scale_en) { \
			tranx = task_param.texture.tran.rotate_dx - cx;  \
			trany = task_param.texture.tran.rotate_dy - cy;  \
		} \
		/* 如果有缩放，旋转中心要对应配置为缩放后的坐标 */ \
		if (task_param.scale_en ) { \
			cx *= task_param.texture.tran.ratio_w; \
			cy *= task_param.texture.tran.ratio_h; \
		} \
		jlgpu_task_img_set_affine_rotate(cx, cy, task_param.texture.tran.rotate_angle); \
	} \
	if (task_param.scale_en || task_param.rotate_en) { \
		PRINT("jlgpu_task_texture_param_map_707n(), translate x: %d, y: %d\n", tranx, trany); \
		struct rect win; \
		jlgpu_get_win_rect(&win); \
		tranx += rect_left(&win); \
		trany += rect_top(&win); \
		jlgpu_task_img_set_trans_mode(JLGPU_TRANS_MODE_AFFINE); /**< 使能仿射变换 */ \
		jlgpu_task_img_set_affine_translate(tranx, trany); /**< 设置仿射平移 */ \
		if (task_param.matrix) { \
			jlgpu_matrix_set_translate(task_param.matrix, 0.0f, 0.0f); \
		} \
	} \
	if (!task_param.draw_en) { \
		jlgpu_shift_rect(&task_param.draw); \
	} \
	PRINT("jlgpu_task_texture_param_map_707n(), End!\n\n"); \
} while (0)


#define jlgpu_task_mask_param_map_707n() /**< MASK参数兼容707n */ \
do { \
	PRINT("\njlgpu_task_mask_param_map_707n() %s() %d\n", __func__, __LINE__); \
	if (task_param.mask_en) { \
		PRINT("jlgpu_task_mask_param_map_707n(): map mask!\n"); \
		jlgpu_task_param_set_mask_type(MASK_TYPE(JLGPU_TRANS_MODE_NONE, JLGPU_MASK_MODE_EXT)); \
		jlgpu_task_mask_set_size(task_param.image.width, task_param.image.height); \
		jlgpu_task_mask_set_format(task_param.mask.mask_format); \
		jlgpu_task_mask_set_endian(task_param.mask.big_end); \
		jlgpu_task_mask_set_invert(task_param.mask.invert); \
	} \
	PRINT("jlgpu_task_mask_param_map_707n(), End!\n\n"); \
} while(0)


#define jlgpu_task_linegrad_param_init(task_id, elm_id) /**< 线性渐变参数初始化 */ \
	jlgpu_task_gradient_param_init(task_id, elm_id)


/* -------------------- GPU调试 API 映射 -------------------- */
#define jlgpu_dump_all_task		jlgpu_chain_dump_all_task  /**< 打印所有任务 */



#endif

