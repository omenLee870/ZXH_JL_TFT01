#include "ifly_common.h"
#include "app_config.h"


void str_remove_quote(char *str, int len)
{
    int i, j;
    for (i = 0, j = 0; i < len; i++) {
        if (str[i] != '\"') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}


void *net_iflytek_malloc(size_t size)
{
#if TCFG_IFLYTEK_USE_PSRAM && TCFG_PSRAM_DEV_ENABLE
    void *p = malloc_psram(size);
    if (p) {
        memset(p, 0, size);
    }
    return p;
#else
    return zalloc(size);
#endif
}

void net_iflytek_free(void *pv)
{
#if TCFG_IFLYTEK_USE_PSRAM && TCFG_PSRAM_DEV_ENABLE
    return free_psram(pv);
#else
    return free(pv);
#endif
}
