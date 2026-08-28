/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/* ------------------------------------------------------------------------------------*/
/**
 * @file debug_macro.h
 *
 * @brief 快捷调试打印宏定义
 * 使用方法：
 * 1. c 文件中定义相关调试打印使能宏；
 * 2. 使能宏之后包含debug_macro.h；
 * 3. 在需要添加调试打印的地方调用对应的快捷宏
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 * @version V1.0.0
 *
 * @date 2025-09-20
 */
/* ------------------------------------------------------------------------------------*/



/* 使能相同类型的debug打印 */
// #define DBG_PRINT_POS	// 使能所有 TREAP_UP，FUNC_POS，FILE_POS，REAL_POS 打印
// #define DBG_PRINT_AREA	// 使能所有 RECT，AREA 打印
// #define DBG_PRINT_FUNC	// 使能所有 FUNC_ENTER，FUNC_LEAVE，FUNC_FAILD打印


/* 支持的宏功能，每个宏对应一个debug打印 */
// #define JL_TREAP_UP_ENABLE	// JL_TREAP_UP();
// #define JL_FUNC_POS_ENABLE	// JL_FUNC_POS();
// #define JL_FILE_POS_ENABLE	// JL_FILE_POS();
// #define JL_REAL_POS_ENABLE	// JL_REAL_POS();
// #define JL_DUMP_RECT_ENABLE	// JL_DUMP_RECT();
// #define JL_DUMP_AREA_ENABLE	// JL_DUMP_AREA();
// #define JL_DUMP_BBOX_ENABLE	// JL_DUMP_BBOX();
// #define JL_FUNC_ENTER_ENABLE	// FUNC_ENTER();
// #define JL_FUNC_LEAVE_ENABLE	// FUNC_LEAVE();
// #define JL_FUNC_FAILD_ENABLE	// FUNC_FAILD(faild info);

/* 使能标签或字符串打印，使能函数流程打印时会自动使能 */
// #define JL_FUNC_LABEL_ENABEL	// FUNC_LABEL(label)
// #define JL_FUNC_PRINT_ENABLE	// FUNC_PRINT(fmt, args)

/*
   DBG_PRINT 用于定义调试打印函数，允许重定义，
   在#include "debug_macro.h"之前定义这个宏，
   可以指定使用自定义打印函数，否则默认使用printf
 */
// #define DBG_PRINT printf // 定义打印函数



#ifndef __UI_DEBUG_MACRO_H__
#define __UI_DEBUG_MACRO_H__


#ifndef DBG_PRINT
#define DBG_PRINT printf /*! 默认打印函数 */
#endif


#ifdef DBG_PRINT_POS
#ifndef JL_TREAP_UP_ENABLE
#define JL_TREAP_UP_ENABLE /*! 使能 JL_TREAP_UP() */
#endif

#ifndef JL_FUNC_POS_ENABLE
#define JL_FUNC_POS_ENABLE /*! 使能 JL_FUNC_POS() */
#endif

#ifndef JL_FILE_POS_ENABLE
#define JL_FILE_POS_ENABLE /*! 使能 JL_FILE_POS() */
#endif

#ifndef JL_REAL_POSE
#define JL_REAL_POS_ENABLE /*! 使能 JL_REAL_POS() */
#endif
#endif


#ifdef DBG_PRINT_AREA
#ifndef JL_DUMP_RECT_ENABLE
#define JL_DUMP_RECT_ENABLE /*! 使能 JL_DUMP_RECT() */
#endif

#ifndef JL_DUMP_AREA_ENABLE
#define JL_DUMP_AREA_ENABLE /*! 使能 JL_DUMP_AREA() */
#endif

#ifndef JL_DUMP_BBOX_ENABLE
#define JL_DUMP_BBOX_ENABLE /*! 使能 JL_DUMP_BBOX() */
#endif
#endif


#ifdef DBG_PRINT_FUNC
#ifndef JL_FUNC_ENTER_ENABLE
#define JL_FUNC_ENTER_ENABLE /*! 使能 FUNC_ENTER() */
#endif

#ifndef JL_FUNC_LEAVE_ENABLE
#define JL_FUNC_LEAVE_ENABLE /*! 使能 FUNC_LEAVE() */
#endif

#ifndef JL_FUNC_FAILD_ENABLE
#define JL_FUNC_FAILD_ENABLE /*! 使能 FUNC_FAILD() */
#endif

#ifndef JL_FUNC_LABEL_ENABEL
#define JL_FUNC_LABEL_ENABEL /*! 使能 FUNC_LABEL() */
#endif
#endif

#if (defined JL_FUNC_FAILD_ENABLE || defined JL_FUNC_ENTER_ENABLE || defined JL_FUNC_LEAVE_ENABLE)
#ifndef JL_FUNC_LABEL_ENABEL
#define JL_FUNC_LABEL_ENABEL /*! 使能 FUNC_LABEL() */
#endif
#endif

#ifdef JL_FUNC_FAILD_ENABLE
#ifndef JL_FUNC_PRINT_ENABLE
#define JL_FUNC_PRINT_ENABLE   /*! 使能 FUNC_PRINT()   */
#endif
#endif


#ifdef JL_TREAP_UP_ENABLE	/* 打印函数rets指针，用于追溯函数上一层位置 */
#define JL_TREAP_UP() \
{ \
    unsigned int __rets; \
    __asm__ volatile("%0 = rets":"=r"(__rets)); \
	DBG_PRINT("[RETS] --> %s() rets: 0x%x\n", __func__, __rets); \
}
#else
#define JL_TREAP_UP()
#endif


#ifdef JL_FUNC_POS_ENABLE	/* 打印当前在函数中的位置 */
#define JL_FUNC_POS()	DBG_PRINT("[FUNC] %s(), line: %d\n", __func__, __LINE__)
#else
#define JL_FUNC_POS()
#endif


#ifdef JL_FILE_POS_ENABLE	/* 打印当前在文件中的位置 */
#define JL_FILE_POS()	DBG_PRINT("[FILE] %s(), line: %d\n", __FILE__, __LINE__);
#else
#define JL_FILE_POS()
#endif


#ifdef JL_REAL_POS_ENABLE	/* 打印当前代码的具体位置 */
#define JL_REAL_POS()	DBG_PRINT("[REAL] file: %d, func: %s(), line: %d\n", __FILE__, __func__, __LINE__);
#else
#define JL_REAL_POS()
#endif


#ifdef JL_DUMP_RECT_ENABLE	/* 打印RECT结构体 */
#define JL_DUMP_RECT(rect) \
	DBG_PRINT("[RECT] --> %s() %d, %s [%d, %d, %d, %d]\n", __func__, __LINE__, #rect, \
			rect_left(rect), rect_top(rect), rect_width(rect), rect_height(rect))
#else
#define JL_DUMP_RECT(rect)
#endif


#ifdef JL_DUMP_AREA_ENABLE	/* 打印AREA结构体 */
#define JL_DUMP_AREA(area) \
	DBG_PRINT("[AREA] --> %s() %d, %s [%d, %d, %d, %d]\n", __func__, __LINE__, #area, \
			area_x1(area), area_x2(area), area_y1(area), area_y2(area))
#else
#define JL_DUMP_AREA(area)
#endif


#ifdef JL_DUMP_BBOX_ENABLE  /* 打印BBOX结构体 */
#define JL_DUMP_BBOX(bbox) \
	DBG_PRINT("[BBOX] --> %s() %d, %s [%d, %d, %d, %d]\n", __func__, __LINE__, #bbox, \
			bbox_minx(bbox), bbox_maxx(bbox), bbox_miny(bbox), bbox_maxy(bbox))
#else
#define JL_DUMP_BBOX(bbox)
#endif


/*
   打印函数位置并打印一个标签
   e.g.
   invoke: FUNC_LABEL(debug);
   output: [FUNC] main(), line: 20. <debug>
 */
#ifdef JL_FUNC_LABEL_ENABEL
#define FUNC_LABEL(label) DBG_PRINT("[FUNC] %s(), line: %d. <%s>\n", __func__, __LINE__, #label)
#else
#define FUNC_LABEL(label)
#endif

/*
   打印函数位置并打印一个字符串
   e.g.
   invoke: FUNC_PRINT("%s", "hello");
   output: [FUNC] main(), line: 20: hello
 */
#ifdef JL_FUNC_PRINT_ENABLE
#define FUNC_PRINT(fmt, ...) DBG_PRINT("[FUNC] %s(), line: %d: "fmt"\n", __func__, __LINE__, __VA_ARGS__)
#else
#define FUNC_PRINT(fmt, ...)
#endif


/*
   打印函数进入信息
   e.g.
   invoke: FUNC_ENTER();
   output: [FUNC] main(), line: 10. Enter
 */
#ifdef JL_FUNC_ENTER_ENABLE
#define FUNC_ENTER() FUNC_LABEL(Enter)
#else
#define FUNC_ENTER()
#endif

/*
   打印函数进入信息
   e.g.
   invoke: FUNC_LEAVE();
   output: [FUNC] main(), line: 50. Leave
 */
#ifdef JL_FUNC_LEAVE_ENABLE
#define FUNC_LEAVE() FUNC_LABEL(Leave)
#else
#define FUNC_LEAVE()
#endif

/*
   打印函数错误信息，一般错误退出的时候可以打印。
   e.g.
   invoke: FUNC_FAILD("%s", "Memory request faild.");
   output: [FUNC] test(), line: 10. <faild> Memory request faild.
 */
#ifdef JL_FUNC_FAILD_ENABLE
#define FUNC_FAILD(fmt, ...) \
	DBG_PRINT("[FUNC] %s(), line: %d. <Faild>: "fmt"\n", __func__, __LINE__, __VA_ARGS__)
#else
#define FUNC_FAILD(fmt, ...)
#endif

/*
   指针断言检查，如果指针为NULL，则触发断言。
   e.g.
   invoke: CHECK_POINTER(ptr);
   output: ASSERT-FAILD: ((ptr) != NULL) , func: xxx(), Pointer "ptr" is NULL!
 */
#define CHECK_POINTER(ptr)	ASSERT(((ptr) != NULL), ", func: %s(), Pointer \"%s\" is NULL!", __func__, #ptr)

/*
 UI_LIB_DEBUG_CODE 用于包住UI库调试代码，此宏仅UI库里使用。
 使用方法：
 	#if UI_LIB_DEBUG_CODE
 		TODO // 调试代码块
	#endif
*/
#define UI_LIB_DEBUG_CODE	((defined UI_DEBUG_ENABLE) && (UI_DEBUG_ENABLE))



#endif

