#ifndef PET_DASHBOARD_BIP_TEST_SUPPORT_H
#define PET_DASHBOARD_BIP_TEST_SUPPORT_H
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned OS_MUTEX;
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define USER_CTRL_BIP_GET_IMAGE 0xfc
int os_mutex_create(OS_MUTEX *mutex);
int os_mutex_pend(OS_MUTEX *mutex, int timeout);
int os_mutex_post(OS_MUTEX *mutex);
int bt_cmd_prepare(int command, int length, void *data);
void bt_music_post_msg_to_lvgl_ui(const char *message);
#if TCFG_PSRAM_DEV_ENABLE
void *malloc_psram(size_t size);
void *realloc_psram(void *ptr, size_t size);
void free_psram(void *ptr);
#endif
#endif
