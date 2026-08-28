#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".lib_net_config.data.bss")
#pragma data_seg(".lib_net_config.data")
#pragma const_seg(".lib_net_config.text.const")
#pragma code_seg(".lib_net_config.text")
#endif


#include "app_config.h"


//============================================================//
//          WIFI驱动 iperf wifi_dev内存是否使用psram
//============================================================//
#if TCFG_PSRAM_DEV_ENABLE
const int config_wifi_malloc_use_psram = 1;
#else
const int config_wifi_malloc_use_psram = 0;
#endif

//================================================//
//          lwip net 支持
//================================================//

/*
 * @brief 以下接口为各网络模块动态内存申请
 		  以帮助划分内存消耗和相关内存策略
 		  在各对应库内均有相关弱函数实现
 * @note
 		  set 1-----use psram mem
 		  set 0-----use ram mem
*/

/*注意：libmpeg、net_audio、net_down的分配介质需保持一致*/
#define JL_LWIP_USE_PSRAM				0			//lwip使用psram
#define JL_HTTP_USE_PSRAM				0			//http使用psram
#define JL_HTTPS_USE_PSRAM				0			//https使用psram
#define JL_NET_AUDIO_USE_PSRAM			0			//net_audio使用psram
#define JL_MBEDTLS_USE_PSRAM           	0           //mbedtls库使用psram
#define JL_WEBSOCKET_USE_PSRAM         	0           //websocket使用psram
#define JL_MPEG_USE_PSRAM				0			//libmpeg使用psram
#define JL_NET_DOWN_USE_PSRAM			0			//net_download使用psram


#if JL_LWIP_USE_PSRAM
#define LWIP_ALLOC(size) 			malloc_psram(size)
#define LWIP_FREE(ptr) 				free_psram(ptr)
#else
#define LWIP_ALLOC(size) 			malloc(size)
#define LWIP_FREE(ptr) 				free(ptr)
#endif

void *lwip_malloc(size_t size)
{
    return LWIP_ALLOC(size);
}

void *lwip_calloc(size_t count, size_t size)
{
    size_t total_size = count * size;
    void *p = LWIP_ALLOC(total_size);
    if (p) {
        memset(p, 0, total_size);
    }
    return p;
}

void lwip_free(void *pv)
{
    if (pv != NULL) {
        LWIP_FREE(pv);
    }
}

#if USER_SUPPORT_PROFILE_PAN || TCFG_EXT_WIFI_ENABLE || TCFG_LTE_PHY_ENABLE
const int CONFIG_LWIP_NET_ENABLE = 1;

#if JL_HTTP_USE_PSRAM
#define HTTP_MALLOC(size) 			malloc_psram(size)
#define HTTP_REALLOC(ptr,size) 		realloc_psram(ptr,size)
#define HTTP_FREE(ptr) 				free_psram(ptr)
#else
#define HTTP_MALLOC(size) 			malloc(size)
#define HTTP_REALLOC(ptr,size) 		realloc(ptr,size)
#define HTTP_FREE(ptr) 				free(ptr)
#endif

void *jl_http_malloc(size_t size)
{
    return HTTP_MALLOC(size);
}

void *jl_http_realloc(void *ptr, size_t size)
{
    return HTTP_REALLOC(ptr, size);
}

void jl_http_free(void *pv)
{
    if (pv != NULL) {
        HTTP_FREE(pv);
    }
}


#if JL_HTTPS_USE_PSRAM
#define HTTPS_ALLOC(size) 				malloc_psram(size)
#define HTTPS_FREE(ptr) 				free_psram(ptr)
#else
#define HTTPS_ALLOC(size) 				malloc(size)
#define HTTPS_FREE(ptr) 				free(ptr)
#endif


void *jl_https_malloc(size_t size)
{
    return HTTPS_ALLOC(size);
}

void *jl_https_calloc(unsigned long count, unsigned long size)
{
    size_t total_size = count * size;
    void *p = HTTPS_ALLOC(total_size);
    if (p) {
        memset(p, 0, count * size);
    }
    return p;
}


void jl_https_free(void *pv)
{
    if (pv != NULL) {
        HTTPS_FREE(pv);
    }
}




#if JL_NET_AUDIO_USE_PSRAM
#define NET_AUDIO_ALLOC(size) 				malloc_psram(size)
#define NET_AUDIO_FREE(ptr) 				free_psram(ptr)
#else
#define NET_AUDIO_ALLOC(size) 				malloc(size)
#define NET_AUDIO_FREE(ptr) 				free(ptr)
#endif


void *jl_net_audio_malloc(size_t size)
{
    printf(">>>zwz info: %s %d %s\n", __FUNCTION__, __LINE__, __FILE__);
    void *p = NET_AUDIO_ALLOC(size);
    return p;
}

void jl_net_audio_free(void *pv)
{
    printf(">>>zwz info: %s %d %s\n", __FUNCTION__, __LINE__, __FILE__);
    if (pv != NULL) {
        NET_AUDIO_FREE(pv);
    }
}

void *jl_net_audio_calloc(unsigned long count, unsigned long size)
{
    printf(">>>zwz info: %s %d %s\n", __FUNCTION__, __LINE__, __FILE__);
    size_t total = count * size;
    void *p = NET_AUDIO_ALLOC(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}





#if JL_MBEDTLS_USE_PSRAM
#define MBEDTLS_MALLOC(size) 		    malloc_psram(size)
#define MBEDTLS_FREE(ptr) 				free_psram(ptr)
#else
#define MBEDTLS_MALLOC(size) 		    malloc(size)
#define MBEDTLS_FREE(ptr) 				free(ptr)
#endif


void jl_mbedtls_free(void *pv)
{
    if (pv != NULL) {
        MBEDTLS_FREE(pv);
    }
}

void *jl_mbedtls_calloc(unsigned long count, unsigned long size)
{
    size_t total = count * size;
    void *p = MBEDTLS_MALLOC(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}




#if JL_WEBSOCKET_USE_PSRAM
#define WEBSOCKET_API_MALLOC(size) 		    malloc_psram(size)
#define WEBSOCKET_API_REALLOC(ptr,size) 	realloc_psram(ptr,size)
#define WEBSOCKET_API_FREE(ptr) 			free_psram(ptr)
#else
#define WEBSOCKET_API_MALLOC(size) 		    malloc(size)
#define WEBSOCKET_API_REALLOC(ptr,size) 	realloc(ptr,size)
#define WEBSOCKET_API_FREE(ptr) 			free(ptr)
#endif


void jl_websocket_api_free(void *pv)
{
    if (pv != NULL) {
        WEBSOCKET_API_FREE(pv);
    }
}

void *jl_websocket_api_malloc(size_t size)
{
    void *p = WEBSOCKET_API_MALLOC(size);
    if (p) {
        memset(p, 0, size);
    }
    return p;
}

void *jl_websocket_api_zalloc(size_t size)
{
    void *ptr = WEBSOCKET_API_MALLOC(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *jl_websocket_api_realloc(void *ptr, size_t size)
{
    return WEBSOCKET_API_REALLOC(ptr, size);
}





#if JL_MPEG_USE_PSRAM
#define MPEG_API_MALLOC(size) 		    malloc_psram(size)
#define MPEG_API_REALLOC(ptr,size) 		realloc_psram(ptr,size)
#define MPEG_API_FREE(ptr) 				free_psram(ptr)
#else
#define MPEG_API_MALLOC(size) 		    malloc(size)
#define MPEG_API_REALLOC(ptr,size) 		realloc(ptr,size)
#define MPEG_API_FREE(ptr) 				free(ptr)
#endif

void jl_mpeg_free(void *pv)
{
    if (pv != NULL) {
        MPEG_API_FREE(pv);
    }
}


void *jl_mpeg_malloc(size_t size)
{
    return MPEG_API_MALLOC(size);
}


void *jl_mpeg_calloc(unsigned long count, unsigned long size)
{
    size_t total = count * size;
    void *p = MPEG_API_MALLOC(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}


void *jl_mpeg_realloc(void *ptr, size_t size)
{
    return MPEG_API_REALLOC(ptr, size);
}


void *jl_mpeg_zalloc(size_t size)
{
    void *ptr = MPEG_API_MALLOC(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}





#if JL_NET_DOWN_USE_PSRAM
#define NET_DOWN_MALLOC(size) 				malloc_psram(size)
#define NET_DOWN_REALLOC(ptr,size) 			realloc_psram(ptr,size)
#define NET_DOWN_FREE(ptr) 					free_psram(ptr)
#else
#define NET_DOWN_MALLOC(size) 				malloc(size)
#define NET_DOWN_REALLOC(ptr,size) 			realloc(ptr,size)
#define NET_DOWN_FREE(ptr) 					free(ptr)
#endif

void *jl_net_down_malloc(size_t size)
{
    return NET_DOWN_MALLOC(size);
}

void *jl_net_down_calloc(unsigned long count, unsigned long size)
{
    size_t total_size = count * size;
    void *p = NET_DOWN_MALLOC(total_size);
    if (p) {
        memset(p, 0, count * size);
    }
    return p;
}

void *jl_net_down_zalloc(size_t size)
{
    void *ptr = NET_DOWN_MALLOC(size);
    if (ptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *jl_net_down_realloc(void *ptr, size_t size)
{
    return NET_DOWN_REALLOC(ptr, size);
}

void jl_net_down_free(void *pv)
{
    if (pv != NULL) {
        NET_DOWN_FREE(pv);
    }
}

#else
const int CONFIG_LWIP_NET_ENABLE = 0;
#endif

