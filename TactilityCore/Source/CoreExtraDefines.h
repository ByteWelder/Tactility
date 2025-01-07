#pragma once

/** Find the largest value
 * @param[in] a first value to compare
 * @param[in] b second value to compare
 * @return the largest value of a and b
 */
#define TT_MAX(a, b)            \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

/** Find the smallest value
 * @param[in] a first value to compare
 * @param[in] b second value to compare
 * @return the smallest value of a and b
 */
#define TT_MIN(a, b)            \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

/** @return the absolute value of the input */
#define TT_ABS(a) ({ (a) < 0 ? -(a) : (a); })

/** Clamp a value between a min and a max.
 * @param[in] x value to clamp
 * @param[in] upper upper bounds for x
 * @param[in] lower lower bounds for x
 */
#define TT_CLAMP(x, upper, lower) (TT_MIN(upper, TT_MAX(x, lower)))
