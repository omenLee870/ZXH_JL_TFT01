/**
 * @file ui_animation_math.h
 *
 */

#ifndef _UI_ANIMATION_MATH_H
#define _UI_ANIMATION_MATH_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#define UI_TRIGO_SIN_MAX 32767
#define UI_TRIGO_SHIFT 15 /**<  >> UI_TRIGO_SHIFT to normalize*/

#define UI_BEZIER_VAL_MAX 1024 /**< Max time in Bezier functions (not [0..1] to use integers)*/
#define UI_BEZIER_VAL_SHIFT 10 /**< log2(UI_BEZIER_VAL_MAX): used to normalize up scaled values*/


#define UI_ATTRIBUTE_FAST_MEM



/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint16_t i;
    uint16_t f;
} ui_sqrt_res_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

//! @cond Doxygen_Suppress
/**
 * Return with sinus of an angle
 * @param angle
 * @return sinus of 'angle'. sin(-90) = -32767, sin(90) = 32767
 */
UI_ATTRIBUTE_FAST_MEM int16_t ui_trigo_sin(int16_t angle);

static inline UI_ATTRIBUTE_FAST_MEM int16_t ui_trigo_cos(int16_t angle)
{
    return ui_trigo_sin(angle + 90);
}

//! @endcond

/**
 * Calculate a value of a Cubic Bezier function.
 * @param t time in range of [0..UI_BEZIER_VAL_MAX]
 * @param u0 start values in range of [0..UI_BEZIER_VAL_MAX]
 * @param u1 control value 1 values in range of [0..UI_BEZIER_VAL_MAX]
 * @param u2 control value 2 in range of [0..UI_BEZIER_VAL_MAX]
 * @param u3 end values in range of [0..UI_BEZIER_VAL_MAX]
 * @return the value calculated from the given parameters in range of [0..UI_BEZIER_VAL_MAX]
 */
uint32_t ui_bezier3(uint32_t t, uint32_t u0, uint32_t u1, uint32_t u2, uint32_t u3);

/**
 * Calculate the atan2 of a vector.
 * @param x
 * @param y
 * @return the angle in degree calculated from the given parameters in range of [0..360]
 */
uint16_t ui_atan2(int x, int y);

//! @cond Doxygen_Suppress

/**
 * Get the square root of a number
 * @param x integer which square root should be calculated
 * @param q store the result here. q->i: integer part, q->f: fractional part in 1/256 unit
 * @param mask optional to skip some iterations if the magnitude of the root is known.
 * Set to 0x8000 by default.
 * If root < 16: mask = 0x80
 * If root < 256: mask = 0x800
 * Else: mask = 0x8000
 */
UI_ATTRIBUTE_FAST_MEM void ui_sqrt(uint32_t x, ui_sqrt_res_t *q, uint32_t mask);

//! @endcond

/**
 * Calculate the integer exponents.
 * @param base
 * @param power
 * @return base raised to the power exponent
 */
int64_t ui_pow(int64_t base, int8_t exp);

/**
 * Get the mapped of a number given an input and output range
 * @param x integer which mapped value should be calculated
 * @param min_in min input range
 * @param max_in max input range
 * @param min_out max output range
 * @param max_out max output range
 * @return the mapped number
 */
int32_t ui_map(int32_t x, int32_t min_in, int32_t max_in, int32_t min_out, int32_t max_out);

/**
 * Get a pseudo random number in the given range
 * @param min   the minimum value
 * @param max   the maximum value
 * @return return the random number. min <= return_value <= max
 */
uint32_t ui_rand(uint32_t min, uint32_t max);

/**********************
 *      MACROS
 **********************/
#define UI_MIN(a, b) ((a) < (b) ? (a) : (b))
#define UI_MIN3(a, b, c) (UI_MIN(UI_MIN(a,b), c))
#define UI_MIN4(a, b, c, d) (UI_MIN(UI_MIN(a,b), UI_MIN(c,d)))

#define UI_MAX(a, b) ((a) > (b) ? (a) : (b))
#define UI_MAX3(a, b, c) (UI_MAX(UI_MAX(a,b), c))
#define UI_MAX4(a, b, c, d) (UI_MAX(UI_MAX(a,b), UI_MAX(c,d)))

#define UI_CLAMP(min, val, max) (UI_MAX(min, (UI_MIN(val, max))))

#define UI_ABS(x) ((x) > 0 ? (x) : (-(x)))
#define UI_UDIV255(x) (((x) * 0x8081U) >> 0x17)

#define UI_IS_SIGNED(t) (((t)(-1)) < ((t)0))
#define UI_UMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0xFULL << ((sizeof(t) * 8ULL) - 4ULL)))
#define UI_SMAX_OF(t) (((0x1ULL << ((sizeof(t) * 8ULL) - 1ULL)) - 1ULL) | (0x7ULL << ((sizeof(t) * 8ULL) - 4ULL)))
#define UI_MAX_OF(t) ((unsigned long)(UI_IS_SIGNED(t) ? UI_SMAX_OF(t) : UI_UMAX_OF(t)))

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif

