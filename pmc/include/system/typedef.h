#ifndef __SYS_TYPEDEF_H__
#define __SYS_TYPEDEF_H__

#include "typedef/typedef.h"

#include "generic/errno-base.h"
#include "string.h"
#include "strings.h"
#include "generic/printf.h"
// #include "generic/log.h"
#include "typedef/assert.h"


#define likely(x) 	__builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)


#endif
