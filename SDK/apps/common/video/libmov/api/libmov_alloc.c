#include "libmov_alloc.h"
#include "system/includes.h"
#include "app_config.h"

__attribute__((weak)) void *libmov_malloc(size_t size)
{
#if defined(TCFG_PSRAM_DEV_ENABLE) && (TCFG_PSRAM_DEV_ENABLE == 1)
    return malloc_psram(size);
#else
    return malloc(size);
#endif
}

__attribute__((weak)) void *libmov_calloc(size_t n, size_t size)
{
    size_t total_size = n * size;
    void *ptr = NULL;

#if defined(TCFG_PSRAM_DEV_ENABLE) && (TCFG_PSRAM_DEV_ENABLE == 1)
    ptr = malloc_psram(size);
#else
    ptr = malloc(size);
#endif

    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

__attribute__((weak)) void *libmov_realloc(void *ptr, size_t size)
{
#if defined(TCFG_PSRAM_DEV_ENABLE) && (TCFG_PSRAM_DEV_ENABLE == 1)
    return realloc_psram(ptr, size);
#else
    return realloc(ptr, size);
#endif
}

__attribute__((weak)) void libmov_free(void *ptr)
{
#if defined(TCFG_PSRAM_DEV_ENABLE) && (TCFG_PSRAM_DEV_ENABLE == 1)
    free_psram(ptr);
#else
    free(ptr);
#endif
}

