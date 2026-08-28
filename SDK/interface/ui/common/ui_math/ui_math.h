#ifndef __UI_MATH_H__
#define __UI_MATH_H__



#ifndef UI_PI
#define UI_PI					(3.14159265f)              /*! 定义 PI    */
#endif


#ifndef UI_ANGLE_TO_RADIAN
#define UI_ANGLE_TO_RADIAN(a)	(((a) * (UI_PI)) / 180.0f) /*! 角度转弧度 */
#endif


#ifndef UI_RADIAN_TO_ANGLE
#define UI_RADIAN_TO_ANGLE(r)	(((r) * 180.0f) / (UI_PI)) /*! 弧度转角度 */
#endif


#ifndef LERP
#define LERP(v1, v2, w)         ((v1) * (w) + (v2) * (1.0f - (w)))
#endif


#ifndef CLAMP
#define CLAMP(x, min, max)      (((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x))
#endif



#ifndef ALIGN_UP /*! 向上对齐 */
#define ALIGN_UP(val, align) (((val) + (align) - 1) / (align) * (align))
#endif



#ifndef ALIGN_DOWN /*! 向下对齐 */
#define ALIGN_DOWN(val, align) ((val) / (align) * (align))
#endif



#endif



