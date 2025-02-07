#pragma once

#include "CoreExtraDefines.h"

#define TT_UNUSED __attribute__((unused))

// region Variable arguments support

// Adapted from https://stackoverflow.com/a/78848701/3848666
#define TT_ARG_IGNORE(X)
#define TT_ARG_CAT(X,Y) _TT_ARG_CAT(X,Y)
#define _TT_ARG_CAT(X,Y) X ## Y
#define TT_ARGCOUNT(...)     _TT_ARGCOUNT ## __VA_OPT__(1(__VA_ARGS__) TT_ARG_IGNORE) (0)
#define _TT_ARGCOUNT1(X,...) _TT_ARGCOUNT ## __VA_OPT__(2(__VA_ARGS__) TT_ARG_IGNORE) (1)
#define _TT_ARGCOUNT2(X,...) _TT_ARGCOUNT ## __VA_OPT__(3(__VA_ARGS__) TT_ARG_IGNORE) (2)
#define _TT_ARGCOUNT3(X,...) _TT_ARGCOUNT ## __VA_OPT__(4(__VA_ARGS__) TT_ARG_IGNORE) (3)
#define _TT_ARGCOUNT4(X,...) _TT_ARGCOUNT ## __VA_OPT__(5(__VA_ARGS__) TT_ARG_IGNORE) (4)
#define _TT_ARGCOUNT5(X,...) 5
#define _TT_ARGCOUNT(X) X
