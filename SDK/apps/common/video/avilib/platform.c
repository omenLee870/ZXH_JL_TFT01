/*
 * platform_tc.c -- project platform wrapper for avilib.
 *
 * The Debian/transcode version used libtc/xio.  In this project avilib is
 * wired to the platform file API exposed through system/includes.h and fs/fs.h.
 */

#include "platform.h"

#include "system/includes.h"
#include "fs/fs.h"
#include <fcntl.h>

#if 0//TCFG_PSRAM_DEV_ENABLE
extern void *malloc_psram(size_t size);
extern void *realloc_psram(void *pv, size_t size);
extern void free_psram(void *pv);
#define AVI_MEM_MALLOC(sz)       malloc_psram(sz)
#define AVI_MEM_REALLOC(p, sz)   realloc_psram(p, sz)
#define AVI_MEM_FREE(p)          free_psram(p)
#else
#define AVI_MEM_MALLOC(sz)       malloc(sz)
#define AVI_MEM_REALLOC(p, sz)   realloc(p, sz)
#define AVI_MEM_FREE(p)          free(p)
#endif

/* #include <stdarg.h> */
/* #include <stdio.h> */
/* #include <stdlib.h> */
/* #include <string.h> */

static const char *plat_mode_from_flags(int flags)
{
    if (flags & O_APPEND) {
        return (flags & O_RDWR) ? "a+" : "a";
    }

    if (flags & O_TRUNC) {
        return (flags & O_RDWR) ? "w+" : "w";
    }

    if (flags & O_RDWR) {
        return "r+";
    }

    if (flags & O_WRONLY) {
        return "w";
    }

    return "r";
}

int plat_open(const char *pathname, int flags, int mode)
{
    FILE *fp;
    const char *f = plat_mode_from_flags(flags);

    (void)mode;

    fp = fopen(pathname, f);
    if (fp == NULL) {
        printf("plat_open fail path:%s  mode:%s \n", pathname, f);
        return -1;
    }

    return (int)fp;
}

int plat_close(int fd)
{
    if (fd < 0) {
        return -1;
    }

    return fclose((FILE *)fd);
}

ssize_t plat_read(int fd, void *buf, size_t count)
{
    size_t done = 0;

    if (fd < 0 || buf == NULL) {
        return -1;
    }

    while (done < count) {
        size_t n = fread((char *)buf + done, 1, count - done, (FILE *)fd);

        if (n == 0) {
            break;
        }

        done += n;
    }

    return (ssize_t)done;
}

ssize_t plat_write(int fd, const void *buf, size_t count)
{
    size_t done = 0;

    if (fd < 0 || buf == NULL) {
        return -1;
    }

    if (count == 0) {
        return 0;
    }
    while (done < count) {
        size_t n = fwrite((void *)((const char *)buf + done), 1, count - done, (FILE *)fd);

        if (n == 0) {
            return -1;
        }

        done += n;
    }

    return (ssize_t)done;
}

int32_t plat_seek(int fd, int32_t offset, int whence)
{
    if (fd < 0) {
        return -1;
    }

    if (fseek((FILE *)fd, offset, whence) != 0) {
        return -1;
    }

    return (int32_t)ftell((FILE *)fd);
}

int plat_ftruncate(int fd, int32_t length)
{
    if (fd < 0) {
        return -1;
    }

    /* printf("plat_ftruncate: fd=%d, length=%d\n", fd, length); */
    return ftruncate((FILE *)fd, length);
}

void *_plat_malloc(const char *file, int line, size_t size)
{
    (void)file;
    (void)line;

    return AVI_MEM_MALLOC(size);
}

void *_plat_zalloc(const char *file, int line, size_t size)
{
    void *ptr;

    (void)file;
    (void)line;

    ptr = AVI_MEM_MALLOC(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }

    return ptr;
}

void *_plat_realloc(const char *file, int line, void *ptr, size_t size)
{
    (void)file;
    (void)line;

    return ptr ? AVI_MEM_REALLOC(ptr, size) : AVI_MEM_MALLOC(size);
}

void plat_free(void *ptr)
{
    AVI_MEM_FREE(ptr);
}

int plat_log_open(void)
{
    return 0;
}

int plat_log_send(PlatLogLevel level, const char *tag, const char *fmt, ...)
{
    va_list ap;

    (void)level;

    if (tag != NULL) {
        printf("[%s] ", tag);
    }

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    printf("\n");

    return 0;
}

int plat_log_close(void)
{
    return 0;
}

