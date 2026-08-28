/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */

/* ------------------------------------------------------------------------------------*/
/**
 * @file box_convert.h
 *
 * @brief 矩形区域结构体互转方法
 * @author JIELI TECHNOLOGY ZHUHAI
 * @version V1.0.0
 *
 * @date 2025-09-25
 */
/* ------------------------------------------------------------------------------------*/
#ifndef __UI_BOX_CONVERT_H__
#define __UI_BOX_CONVERT_H__


/* rect 结构体各成员 */
#ifndef rect_left
#define rect_left(rect)		(rect)->left
#endif

#ifndef rect_top
#define rect_top(rect)		(rect)->top
#endif

#ifndef rect_width
#define rect_width(rect)	(rect)->width
#endif

#ifndef rect_height
#define rect_height(rect)	(rect)->height
#endif

static inline void rect_move_to(struct rect *rect, rect_coord_t x, rect_coord_t y)
{
    rect_left(rect) = x;
    rect_top(rect)  = y;
}

static inline void rect_move_offset(struct rect *rect, rect_coord_t x, rect_coord_t y)
{
    rect_left(rect) += x;
    rect_top(rect)  += y;
}

#ifndef rect_set_val
#define rect_set_val(rect, l, t, w, h) \
	do {                               \
		rect_left(rect)   = l;         \
		rect_top(rect)    = t;         \
		rect_width(rect)  = w;         \
		rect_height(rect) = h;         \
	} while (0)
#endif


/* area 结构体各成员 */
#ifndef area_x1
#define area_x1(area)		(area)->x1
#endif
#ifndef area_x2
#define area_x2(area)		(area)->x2
#endif
#ifndef area_y1
#define area_y1(area)		(area)->y1
#endif
#ifndef area_y2
#define area_y2(area)		(area)->y2
#endif


/* bbox 结构体各成员 */
#ifndef bbox_minx
#define bbox_minx(bbox)		bbox_min_x(bbox)
#endif
#ifndef bbox_maxx
#define bbox_maxx(bbox)		bbox_max_x(bbox)
#endif
#ifndef bbox_miny
#define bbox_miny(bbox)		bbox_min_y(bbox)
#endif
#ifndef bbox_maxy
#define bbox_maxy(bbox)		bbox_max_y(bbox)
#endif

#ifndef bbox_width
#define bbox_width(bbox)	(bbox_maxx(bbox) - bbox_minx(bbox))
#endif
#ifndef bbox_height
#define bbox_height(bbox)	(bbox_maxy(bbox) - bbox_miny(bbox))
#endif

static inline void bbox_move_to(jlgpu_boundbox_t *bbox, int x, int y)
{
    bbox_maxx(bbox) = bbox_width(bbox) + x;
    bbox_maxy(bbox) = bbox_height(bbox) + y;
    bbox_minx(bbox) = x;
    bbox_miny(bbox) = y;
}

static inline void bbox_move_offset(jlgpu_boundbox_t *bbox, int x, int y)
{
    bbox_minx(bbox) += x;
    bbox_maxx(bbox) += x;
    bbox_miny(bbox) += y;
    bbox_maxy(bbox) += y;
}

/* 设置bbox区域 */
#define bbox_set_val(bbox, xmin, xmax, ymin, ymax) \
	do {                                           \
		bbox_minx(bbox) = xmin;                    \
		bbox_maxx(bbox) = xmax;                    \
		bbox_miny(bbox) = ymin;                    \
		bbox_maxy(bbox) = ymax;                    \
	} while (0)


/* rect转area结构体 */
#define RECT_TO_AREA(rect, area)                                \
	do {                                                        \
		area_x1(area) = rect_left(rect);                        \
		area_x2(area) = rect_left(rect) + rect_width(rect) - 1; \
		area_y1(area) = rect_top(rect);                         \
		area_y2(area) = rect_top(rect) + rect_height(rect) - 1; \
	} while (0)

/* rect转bbox结构体 */
#define RECT_TO_BBOX(rect, bbox)                              \
	do {                                                      \
		bbox_minx(bbox) = rect_left(rect);                    \
		bbox_maxx(bbox) = rect_left(rect) + rect_width(rect); \
		bbox_miny(bbox) = rect_top(rect);                     \
		bbox_maxy(bbox) = rect_top(rect) + rect_height(rect); \
	} while (0)

/* area转rect结构体 */
#define AREA_TO_RECT(area, rect)                               \
	do {                                                       \
		rect_left(rect)   = area_x1(area);                     \
		rect_top(rect)    = area_y1(area);                     \
		rect_width(rect)  = area_x2(area) - area_x1(area) + 1; \
		rect_height(rect) = area_y2(area) - area_y1(area) + 1; \
	} while (0)

/* area转bbox结构体 */
#define AREA_TO_BBOX(area, bbox)         \
	do {                                 \
		bbox_minx(bbox) = area_x1(area); \
		bbox_maxx(bbox) = area_x2(area); \
		bbox_miny(bbox) = area_y1(area); \
		bbox_maxy(bbox) = area_y2(area); \
	} while (0)

/* bbox转rect结构体 */
#define BBOX_TO_RECT(bbox, rect)                               \
	do {                                                       \
		rect_left(rect)   = bbox_minx(bbox);                   \
		rect_width(rect)  = bbox_maxx(bbox) - bbox_minx(bbox); \
		rect_top(rect)    = bbox_miny(bbox);                   \
		rect_height(rect) = bbox_maxy(bbox) - bbox_miny(bbox); \
	} while (0)

/* bbox转area结构体 */
#define BBOX_TO_AREA(bbox, area)         \
	do {                                 \
		area_x1(area) = bbox_minx(bbox); \
		area_x2(area) = bbox_maxx(bbox); \
		area_y1(area) = bbox_miny(bbox); \
		area_y2(area) = bbox_maxy(bbox); \
	} while (0)


#define __DUMP_RECT(rect) \
	printf("[DUMP] --> %s() %d, %s [%3d, %3d, %3d, %3d]\n", __func__, __LINE__, #rect, \
			rect_left(rect), rect_top(rect), rect_width(rect), rect_height(rect))

#define __DUMP_AREA(area) \
	printf("[DUMP] --> %s() %d, %s [%3d, %3d, %3d, %3d]\n", __func__, __LINE__, #area, \
			area_x1(area), area_x2(area), area_y1(area), area_y2(area))

#define __DUMP_BBOX(bbox) \
	printf("[DUMP] --> %s() %d, %s [%4d, %4d, %4d, %4d]\n", __func__, __LINE__, #bbox, \
			bbox_minx(bbox), bbox_miny(bbox), bbox_width(bbox), bbox_height(bbox))


#endif // !__UI_BOX_CONVERT_H__


