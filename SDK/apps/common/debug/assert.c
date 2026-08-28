#include <assert.h>
#include "printf.h"
void __assert_func(const char *file, int line, const char *func, const char *_e)
{
    printf("\n\n ******************* ASSERT -> %s, %d, %s, %s \n", file, line, func, _e);
    while (1);
}

void __assert_func_cmpt(const char *file, int line, const char *func, const char *_e)
{
    printf("\n\n ******************* ASSERT -> %s, %d, %s, %s \n", file, line, func, _e);
}


void __assert_fail(const char *__assertion, const char *__file,
                   unsigned int __line, const char *__function)
{
    printf("\n\n ******************* ASSERT -> %s, %s, %u, %s \n", __assertion, __file, __line, __function);

}
/* void __assert(const char *__assertion, const char *__file, unsigned int __line, const char *__function) */
void __assert(const char *__assertion,   int __line, const char *__function)
{
    printf("\n\n ******************* ASSERT -> %s, %d, %s \n", __assertion,  __line, __function);
    while (1);

}
