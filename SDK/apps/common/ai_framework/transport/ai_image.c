#include "ai_internal.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE

#include <mbedtls/base64.h>

#include <stdlib.h>
#include <string.h>

/* 统一给 provider 复用的 base64 编码工具，输出字符串由调用方释放。 */
int ai_base64_encode(const unsigned char *data, unsigned int len, char **out)
{
    size_t olen = 0;
    size_t out_len;
    unsigned char *encoded;
    int ret;

    if (!out || (!data && len)) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    out_len = ((size_t)len + 2) / 3 * 4;
    encoded = (unsigned char *)ai_malloc_large(out_len + 1);
    if (!encoded) {
        return AI_ERR_NOMEM;
    }

    ret = mbedtls_base64_encode(encoded, out_len + 1, &olen, data, len);
    if (ret != 0) {
        ai_free_large(encoded);
        return AI_ERR_PARSE;
    }

    encoded[olen] = '\0';
    *out = (char *)encoded;

    return AI_OK;
}

/* 把 provider/transport 收到的 base64 文本还原成二进制，结果缓冲由调用方回收。 */
int ai_base64_decode(const char *text, unsigned char **out, unsigned int *out_len)
{
    size_t olen = 0;
    size_t len;
    unsigned char *decoded;
    int ret;

    if (!text || !out || !out_len) {
        return AI_ERR_PARAM;
    }

    *out = NULL;
    *out_len = 0;
    len = strlen(text);
    ret = mbedtls_base64_decode(NULL, 0, &olen, (const unsigned char *)text, len);
    if (ret != 0 && ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        return AI_ERR_PARSE;
    }

    decoded = (unsigned char *)ai_malloc_large(olen ? olen : 1);
    if (!decoded) {
        return AI_ERR_NOMEM;
    }

    ret = mbedtls_base64_decode(decoded, olen, &olen, (const unsigned char *)text, len);
    if (ret != 0) {
        ai_free_large(decoded);
        return AI_ERR_PARSE;
    }

    *out = decoded;
    *out_len = (unsigned int)olen;
    return AI_OK;
}

#endif
