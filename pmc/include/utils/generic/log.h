#ifndef  __LOG_H__
#define  __LOG_H__




#include "generic/printf.h"

#define LOG_ASSERT_ENABLE

extern const char libs_debug;



#define LOG_VERBOSE     v
#define LOG_INFO        i
#define LOG_DEBUG       d
#define LOG_WARN        w
#define LOG_ERROR       e
#define LOG_CHAR        c

#define _STR(x)     #x
#define STR(x)     "["_STR(x)"]"


#define _LOG_TAG_CONST_DECLARE(level, name)         extern const char log_tag_const_##level##_##name
#define LOG_TAG_CONST_DECLARE(level, name)          _LOG_TAG_CONST_DECLARE(level, name)

#define ___LOG_IS_ENABLE(level, name)   (log_tag_const_##level##_##name)
#define __LOG_IS_ENABLE(level, name)    ___LOG_IS_ENABLE(level, name)
#define _LOG_IS_ENABLE(level)           __LOG_IS_ENABLE(level, LOG_TAG_CONST)

#ifdef LOG_TAG_CONST
LOG_TAG_CONST_DECLARE(LOG_VERBOSE,  LOG_TAG_CONST);
LOG_TAG_CONST_DECLARE(LOG_INFO,     LOG_TAG_CONST);
LOG_TAG_CONST_DECLARE(LOG_DEBUG,    LOG_TAG_CONST);
LOG_TAG_CONST_DECLARE(LOG_WARN,     LOG_TAG_CONST);
LOG_TAG_CONST_DECLARE(LOG_ERROR,    LOG_TAG_CONST);
LOG_TAG_CONST_DECLARE(LOG_CHAR,     LOG_TAG_CONST);
// #define _LOG_TAG                        STR(LOG_TAG_CONST)
#define _LOG_TAG            LOG_TAG
#define LOG_IS_ENABLE(level)            _LOG_IS_ENABLE(level)

#else
#define _LOG_TAG            ""
#define LOG_IS_ENABLE(x)    libs_debug
#endif

// #ifdef __DEBUG
#if 1

#define log_print printf

#define log_info(format, ...)       \
    if (LOG_IS_ENABLE(LOG_INFO)) \
        log_print("[Info] " _LOG_TAG format "\r\n", ## __VA_ARGS__)

#define log_info_hexdump(x, y)     \
    if (LOG_IS_ENABLE(LOG_INFO)) \
        printf_buf(x, y)

#define log_d(format, ...)       \
    if (LOG_IS_ENABLE(LOG_DEBUG)) \
        log_print(format , ## __VA_ARGS__)


#define log_debug(format, ...)       \
    if (LOG_IS_ENABLE(LOG_DEBUG)) \
        log_print("[Debug] " _LOG_TAG format "\r\n", ## __VA_ARGS__)

#define log_debug_hexdump(x, y)     \
    if (LOG_IS_ENABLE(LOG_DEBUG)) \
        printf_buf(x, y)

#define log_error(format, ...)       \
    if (LOG_IS_ENABLE(LOG_ERROR)) \
        log_print("<Error> " _LOG_TAG format "\r\n", ## __VA_ARGS__)

#define log_error_hexdump(x, y)     \
    if (LOG_IS_ENABLE(LOG_ERROR)) \
        printf_buf(x, y)

#define log_char(x)       \
    if (LOG_IS_ENABLE(LOG_CHAR)) \
		putchar(x)

#define r_printf(x, ...)  log_print("\e[31m\e[1m" x "\e[0m", ## __VA_ARGS__)
#define g_printf(x, ...)  log_print("\e[32m\e[1m" x "\e[0m", ## __VA_ARGS__)
#define y_printf(x, ...)  log_print("\e[33m\e[1m" x "\e[0m", ## __VA_ARGS__)
#define b_printf(x, ...)  log_print("\e[34m\e[1m" x "\e[0m", ## __VA_ARGS__)
#define r_f_printf(x, ...)  log_print("\e[31m\e[5m\e[1m" x "\e[0m", ## __VA_ARGS__)
#define g_f_printf(x, ...)  log_print("\e[32m\e[5m\e[1m" x "\e[0m", ## __VA_ARGS__)
#define y_f_printf(x, ...)  log_print("\e[33m\e[5m\e[1m" x "\e[0m", ## __VA_ARGS__)

#define log_v(...)  do {} while (0)
// #define log_d(...)  do {} while (0)
#define log_i(...)  do {} while (0)
#define log_w(...)  do {} while (0)
#define log_e(...)  do {} while (0)
#define log_c(...)  do {} while (0)

#else

#define log_info(format, ...)
#define log_info_hexdump(x, y)
#define log_debug(format, ...)
#define log_debug_hexdump(x, y)
#define log_error(format, ...)
#define log_error_hexdump(x, y)
#define log_char(x)
#define log_printf(format, ...)

#define log_v(...)  do {} while (0)
#define log_d(...)  do {} while (0)
#define log_i(...)  do {} while (0)
#define log_w(...)  do {} while (0)
#define log_e(...)  do {} while (0)
#define log_c(...)  do {} while (0)

#endif

#define traceSUPER_MODE() \


#endif  /*LOG_H*/


