/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_color.h
 *
 * @brief RGB颜色分量操作
 * 1. 将各个分量合成RGB颜色值；
 * 2. 将RGB颜色值分解成各个分量；
 * 3. RGB颜色值互转；
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-03-23
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_COLOR_H__
#define __UI_COLOR_H__


/* ------------------------------------------------------------------------------------*/
#ifndef RGB555 /*! 合成RGB555颜色值 */
#define RGB555(R, G, B) \
    (0x8000 | ((((unsigned short)(R) & 0xf8) << 7) | \
			   (((unsigned short)(G) & 0xf8) << 2) | \
			   (((unsigned short)(B) & 0xf8) >> 3)))
#endif

#ifndef ARGB8555 /*! 合成ARGB8555颜色值 */
#define ARGB8555(A,R,G,B) ((((unsigned char)(A)) << 16) | RGB555(R,G,B))
#endif

/*! 从RGB555颜色值中提取红色分量（5位，0-31） */
#define RGB555_R(C) ((unsigned char)(((((C) >> 10) & 0x1f) << 3) | 0x7))
/*! 从RGB555颜色值中提取绿色分量（5位，0-31） */
#define RGB555_G(C) ((unsigned char)(((((C) >> 5) & 0x1f) << 3) | 0x7))
/*! 从RGB555颜色值中提取蓝色分量（5位，0-31） */
#define RGB555_B(C) ((unsigned char)((((C) & 0x1f) << 3) | 0x7))

#define ARGB8555_A(C) ((unsigned char)((C) >> 16))
#define ARGB8555_R(C) RGB555_R(C)
#define ARGB8555_G(C) RGB555_G(C)
#define ARGB8555_B(C) RGB555_B(C)
/* ------------------------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------*/
#ifndef RGB565 /*! 合成RGB565颜色值 */
#define RGB565(R, G, B) \
	((((unsigned short)((R) >> 3)) << 11) | \
	 (((unsigned short)((G) >> 2)) << 5) | \
	  ((unsigned short)((B) >> 3)))
#endif

#ifndef ARGB8565 /*! 合成ARGB8565颜色值 */
#define ARGB8565(A,R,G,B) ((((unsigned char)(A))<<16) | RGB565(R,G,B))
#endif

/*! 从RGB565颜色值中提取红色分量（5位，0-31）*/
#define RGB565_R(C) ((unsigned char)(((((C) >> 11) & 0x1f) << 3) | 0x7))
/*! 从RGB565颜色值中提取绿色分量（6位，0-31）*/
#define RGB565_G(C) ((unsigned char)(((((C) >> 5) & 0x3f) << 2) | 0x3))
/*! 从RGB565颜色值中提取蓝色分量（5位，0-31）*/
#define RGB565_B(C) ((unsigned char)((((C) & 0x1f) << 3) | 0x7))

#define ARGB8565_A(C) ((unsigned char)((C)>>16))
#define ARGB8565_R(C) RGB565_R(C)
#define ARGB8565_G(C) RGB565_G(C)
#define ARGB8565_B(C) RGB565_B(C)
/* ------------------------------------------------------------------------------------*/


/* ------------------------------------------------------------------------------------*/
#ifndef RGB888 /*! 合成RGB888颜色值 */
#define RGB888(R,G,B) \
	((((unsigned char)(R)) << 16) | \
	 (((unsigned char)(G)) << 8) | \
	 (((unsigned char)(B))))
#endif

#ifndef ARGB8888 /*! 合成ARGB8888颜色值 */
#define ARGB8888(A,R,G,B) ((((unsigned char)(A)) << 24) | RGB888(R,G,B))
#endif

#define RGB888_R(C) ((unsigned char)((C) >> 16))
#define RGB888_G(C) ((unsigned char)((C) >> 8))
#define RGB888_B(C) ((unsigned char)((C) & 0xff))

#define ARGB8888_A(C) ((unsigned char)((C) >> 24))
#define ARGB8888_R(C) RGB888_R(C)
#define ARGB8888_G(C) RGB888_G(C)
#define ARGB8888_B(C) RGB888_B(C)
/* ------------------------------------------------------------------------------------*/


// 555转565、888
#define RGB555_TO_RGB565(C) RGB565(RGB555_R(C),RGB555_G(C),RGB555_B(C))
#define RGB555_TO_RGB888(C) RGB888(RGB555_R(C),RGB555_G(C),RGB555_B(C))
#define ARGB8555_TO_ARGB8565(C)	ARGB8565(ARGB8555_A(C),ARGB8555_R(C),ARGB8555_G(C),ARGB8555_B(C))
#define ARGB8555_TO_ARGB8888(C)	ARGB8888(ARGB8555_A(C),ARGB8555_R(C),ARGB8555_G(C),ARGB8555_B(C))

// 565转555、888
#define RGB565_TO_RGB555(C)	RGB555(RGB565_R(C),RGB565_G(C),RGB565_B(C))
#define RGB565_TO_RGB888(C)	RGB888(RGB565_R(C),RGB565_G(C),RGB565_B(C))
#define ARGB8565_TO_ARGB8555(C)	ARGB8555(ARGB8565_A(C),ARGB8565_R(C),ARGB8565_G(C),ARGB8565_B(C))
#define ARGB8565_TO_ARGB8888(C)	ARGB8888(ARGB8565_A(C),ARGB8565_R(C),ARGB8565_G(C),ARGB8565_B(C))

// 888转555、565
#define RGB888_TO_RGB555(C)	RGB555(RGB888_R(C),RGB888_G(C),RGB888_B(C))
#define RGB888_TO_RGB565(C)	RGB565(RGB888_R(C),RGB888_G(C),RGB888_B(C))
#define ARGB8888_TO_ARGB8555(C)	ARGB8555(ARGB8888_A(C),ARGB8888_R(C),ARGB8888_G(C),ARGB8888_B(C))
#define ARGB8888_TO_ARGB8565(C)	ARGB8565(ARGB8888_A(C),ARGB8888_R(C),ARGB8888_G(C),ARGB8888_B(C))





#endif


