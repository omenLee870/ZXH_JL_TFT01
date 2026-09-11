#ifndef __IFLY_COMMON__H
#define __IFLY_COMMON__H

#include "generic/includes.h"

// 引号去除函数
void str_remove_quote(char *str, int len);

void *net_iflytek_malloc(size_t size);
void net_iflytek_free(void *pv);
#endif
