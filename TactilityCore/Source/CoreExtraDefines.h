#pragma once

#define TT_MAX(a, b)            \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

#define TT_MIN(a, b)            \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

#define TT_ABS(a) ({ (a) < 0 ? -(a) : (a); })

#define TT_ROUND_UP_TO(a, b)    \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a / _b + !!(_a % _b);  \
    })

#define TT_CLAMP(x, upper, lower) (TT_MIN(upper, TT_MAX(x, lower)))

#define TT_COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

#define TT_SWAP(x, y)       \
    do {                    \
        typeof(x) SWAP = x; \
        x = y;              \
        y = SWAP;           \
    } while (0)

#define TT_STRINGIFY(x) #x

#define TT_TOSTRING(x) TT_STRINGIFY(x)

#define TT_CONCATENATE(a, b) CONCATENATE_(a, b)
#define TT_CONCATENATE_(a, b) a##b

#define TT_REVERSE_BYTES_U32(x)                                                           \
    ((((x) & 0x000000FF) << 24) | (((x) & 0x0000FF00) << 8) | (((x) & 0x00FF0000) >> 8) | \
     (((x) & 0xFF000000) >> 24))

#define TT_BIT(x, n) (((x) >> (n)) & 1)

#define TT_BIT_SET(x, n)        \
    ({                          \
        __typeof__(x) _x = (1); \
        (x) |= (_x << (n));     \
    })

#define TT_BIT_CLEAR(x, n)      \
    ({                          \
        __typeof__(x) _x = (1); \
        (x) &= ~(_x << (n));    \
    })

#define TT_SW_MEMBARRIER() asm volatile("" : : : "memory")
