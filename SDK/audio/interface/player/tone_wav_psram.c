/* Only the WAV adapter's malloc/zalloc/free references are redirected here.
 * Stream frames and DAC/DMA buffers keep the SDK's original allocators. */
#ifdef TONE_WAV_PSRAM_TEST
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern void *malloc_psram(size_t size);
extern void free_psram(void *ptr);
#else
#include "app_config.h"
#include "system/malloc.h"
#include <string.h>
#endif

void *tone_wav_psram_malloc(size_t size)
{
#if TCFG_PSRAM_DEV_ENABLE
    void *ptr = malloc_psram(size);
    printf("[WAV_PSRAM] alloc=%p bytes=%u\n", ptr, (unsigned int)size);
    return ptr;
#else
    return malloc(size);
#endif
}

void *tone_wav_psram_zalloc(size_t size)
{
    void *ptr = tone_wav_psram_malloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}

void tone_wav_psram_free(void *ptr)
{
    if (!ptr) return;
#if TCFG_PSRAM_DEV_ENABLE
    printf("[WAV_PSRAM] free=%p\n", ptr);
    free_psram(ptr);
#else
    free(ptr);
#endif
}
