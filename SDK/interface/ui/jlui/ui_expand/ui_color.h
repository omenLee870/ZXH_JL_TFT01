
#ifndef __UI_COLOR_H__
#define __UI_COLOR_H__


// RGB555
#define TO_RGB555(R,G,B)		(((((u8)(R))>>3)<<10) | ((((u8)(G))>>3)<<5) | (((u8)(B))>>3))
#define TO_ARGB8555(A,R,G,B)	((((u8)(A))<<16) | TO_RGB555(R,G,B))

#define RGB555_R(C)			((u8)((((C)>>10)&0x1f)<<3)|0x7)
#define RGB555_G(C)			((u8)((((C)>>5)&0x1f)<<3)|0x7)
#define RGB555_B(C)			((u8)(((C)&0x1f)<<3)|0x7)

#define ARGB8555_A(C)		((u8)((C)>>16))
#define ARGB8555_R(C)		RGB555_R(C)
#define ARGB8555_G(C)		RGB555_G(C)
#define ARGB8555_B(C)		RGB555_B(C)


// RGB565
#define TO_RGB565(R,G,B)		(((((u8)(R))>>3)<<11) | ((((u8)(G))>>2)<<5) | (((u8)(B))>>3))
#define TO_ARGB8565(A,R,G,B)	((((u8)(A))<<16) | TO_RGB565(R,G,B))

#define RGB565_R(C)			((u8)(((((C)>>11)&0x1f)<<3)|0x7))
#define RGB565_G(C)			((u8)(((((C)>>5)&0x3f)<<2)|0x3))
#define RGB565_B(C)			((u8)((((C)&0x1f)<<3)|0x7))

#define ARGB8565_A(C)		((u8)((C)>>16))
#define ARGB8565_R(C)		RGB565_R(C)
#define ARGB8565_G(C)		RGB565_G(C)
#define ARGB8565_B(C)		RGB565_B(C)


// RGB888
#define TO_RGB888(R,G,B)		((((u8)(R))<<16) | (((u8)(G))<<8) | (((u8)(B))))
#define TO_ARGB8888(A,R,G,B)	((((u8)(A))<<24) | TO_RGB888(R,G,B))

#define RGB888_R(C)			((u8)((C)>>16))
#define RGB888_G(C)			((u8)((C)>>8))
#define RGB888_B(C)			((u8)((C)&0xff))

#define ARGB8888_A(C)		((u8)((C)>>24))
#define ARGB8888_R(C)		RGB888_R(C)
#define ARGB8888_G(C)		RGB888_G(C)
#define ARGB8888_B(C)		RGB888_B(C)


// 555转565、888
#define RGB555_TO_RGB565(C)		TO_RGB565(RGB555_R(C),RGB555_G(C),RGB555_B(C))
#define RGB555_TO_RGB888(C)		TO_RGB888(RGB555_R(C),RGB555_G(C),RGB555_B(C))
#define ARGB8555_TO_ARGB8565(C)	TO_ARGB8565(ARGB8555_A(C),ARGB8555_R(C),ARGB8555_G(C),ARGB8555_B(C))
#define ARGB8555_TO_ARGB8888(C)	TO_ARGB8888(ARGB8555_A(C),ARGB8555_R(C),ARGB8555_G(C),ARGB8555_B(C))

// 565转555、888
#define RGB565_TO_RGB555(C)		TO_RGB555(RGB565_R(C),RGB565_G(C),RGB565_B(C))
#define RGB565_TO_RGB888(C)		TO_RGB888(RGB565_R(C),RGB565_G(C),RGB565_B(C))
#define ARGB8565_TO_ARGB8555(C)	TO_ARGB8555(ARGB8565_A(C),ARGB8565_R(C),ARGB8565_G(C),ARGB8565_B(C))
#define ARGB8565_TO_ARGB8888(C)	TO_ARGB8888(ARGB8565_A(C),ARGB8565_R(C),ARGB8565_G(C),ARGB8565_B(C))

// 888转555、565
#define RGB888_TO_RGB555(C)		TO_RGB555(RGB888_R(C),RGB888_G(C),RGB888_B(C))
#define RGB888_TO_RGB565(C)		TO_RGB565(RGB888_R(C),RGB888_G(C),RGB888_B(C))
#define ARGB8888_TO_ARGB8555(C)	TO_ARGB8555(ARGB8888_A(C),ARGB8888_R(C),ARGB8888_G(C),ARGB8888_B(C))
#define ARGB8888_TO_ARGB8565(C)	TO_ARGB8565(ARGB8888_A(C),ARGB8888_R(C),ARGB8888_G(C),ARGB8888_B(C))





#endif


