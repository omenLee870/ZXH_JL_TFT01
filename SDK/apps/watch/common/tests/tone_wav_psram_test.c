#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *tone_wav_psram_malloc(size_t size);
void *tone_wav_psram_zalloc(size_t size);
void tone_wav_psram_free(void *ptr);

static void *live[3];
static unsigned allocations, releases;
static int fail;

void *malloc_psram(size_t size)
{
    if (fail) return NULL;
    void *ptr = malloc(size);
    assert(ptr);
    memset(ptr, 0xa5, size);
    for (unsigned i = 0; i < 3; ++i) {
        if (!live[i]) {
            live[i] = ptr;
            ++allocations;
            return ptr;
        }
    }
    assert(!"allocation not released between rounds");
    return NULL;
}

void free_psram(void *ptr)
{
    /* The SDK PSRAM free function need not accept NULL. */
    assert(ptr);
    for (unsigned i = 0; i < 3; ++i) {
        if (live[i] == ptr) {
            live[i] = NULL;
            ++releases;
            free(ptr);
            return;
        }
    }
    assert(!"double free or wrong allocator");
}

int main(void)
{
    for (unsigned round = 0; round < 20; ++round) {
        void *handle = tone_wav_psram_malloc(84);
        void *decoder = tone_wav_psram_malloc(1024);
        unsigned char *work = tone_wav_psram_zalloc(256);
        for (unsigned i = 0; i < 256; ++i) assert(work[i] == 0);
        tone_wav_psram_free(decoder);
        tone_wav_psram_free(work);
        tone_wav_psram_free(handle);
        assert(allocations == releases);
    }
    fail = 1;
    assert(!tone_wav_psram_malloc(84));
    assert(!tone_wav_psram_zalloc(256));
    tone_wav_psram_free(NULL);
    assert(allocations == 60 && releases == 60);
    puts("WAV PSRAM allocation/release tests passed");
    return 0;
}
