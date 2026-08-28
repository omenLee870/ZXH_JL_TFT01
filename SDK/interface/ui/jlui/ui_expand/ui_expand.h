#ifndef __PUBLIC_UI_EXPAND_H__
#define __PUBLIC_UI_EXPAND_H__


#ifndef ABS
#define ABS(x)		(((x) > 0) ? (x) : (-(x)))		// 整形绝对值
#endif

#ifndef INV
#define INV(x)		(((x) > 0) ? (-(x)) : (x))		// 取反向值
#endif

#ifndef ASS
#define ASS(a, x)	(((a) > 0) ? ABS(x) : INV(x))	// 取a同号
#endif

#ifndef	OPE
#define OPE(x)		(((x) > 0) ? 1 : (-1))			// 取x符号
#endif

#ifndef FABS
#define FABS(x)		(((x) < 0.0f) ? (-(x)) : (x))	// 浮点绝对值
#endif

#ifndef MAX
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))		// 取较大值
#endif

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))		// 取较小值
#endif


#ifndef UI_PI
#define UI_PI	3.14159265f							// 定义PI
#endif


#ifndef ANGLE_TO_RADIAN
#define ANGLE_TO_RADIAN(a)	(((a) * (UI_PI)) / 180.0f)	// 角度转弧度
#endif


#ifndef RADIAN_TO_ANGLE
#define RADIAN_TO_ANGLE(r)	(((r) * 180.0f) / (UI_PI))	// 弧度转角度
#endif


#ifndef INT_MAX
#define INT_MAX		(0x7FFFFFFF)	// 2147483647
#endif


#ifndef INT_MIN
#define INT_MIN		(-INT_MAX - 1)
#endif


#ifndef U16_MAX
#define U16_MAX		(65535)			// u16 最大值
#endif


// 字符串HASH化，djb2
#ifndef HASH_STRING
#define HASH_STRING(str) ({ \
    unsigned int hash = 5381; \
    const char *s = str; \
    while (*s) { \
        hash = ((hash << 5) + hash) + (*s++); \
    } \
    hash; \
})
#endif


// 计算数组长度
#ifndef ARRAY_LEN
#define ARRAY_LEN(array)	(sizeof(array)/sizeof(array[0]))
#endif


#define DUMP_RECT(func, line, name, rect) \
	printf("[RECT] %s() %d, %s [%d, %d, %d, %d]\n", func, line, name, (rect)->left, (rect)->top, (rect)->width, (rect)->height)

#define JLUI_RECT_TO_CSS(x, X)	(x)//((x) * 10000 / (X))		// 绝对坐标转相对坐标
#define JLUI_CSS_TO_RECT(c, C)	((c) * (C) / 10000)		// 相对坐标转绝对坐标







#endif


