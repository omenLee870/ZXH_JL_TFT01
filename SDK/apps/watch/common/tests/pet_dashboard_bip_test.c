#include "pet_dashboard_bip_test_support.h"
#include "pet_dashboard_music.h"
#include <assert.h>

void bip_mem_init(void);
void bip_rx_data_handle(u8 *packet, u16 chunk, u32 total, u8 status);
u8 *bip_file_data_get(void);
u32 bip_file_data_size(void);
u8 bip_file_status_get(void);
static unsigned commands, notifications;
static bool reject_command;

/** @brief 检查真实接收代码的加锁配对。 */
int os_mutex_create(OS_MUTEX *mutex) { *mutex = 0; return 0; }
int os_mutex_pend(OS_MUTEX *mutex, int timeout)
{
    (void)timeout;
    assert(*mutex == 0);
    *mutex = 1;
    return 0;
}
int os_mutex_post(OS_MUTEX *mutex) { assert(*mutex == 1); *mutex = 0; return 0; }

/** @brief 只替换 SDK 命令和 UI 投递，不替换接收状态机。 */
int bt_cmd_prepare(int command, int length, void *data)
{
    assert(command == USER_CTRL_BIP_GET_IMAGE && length == 0 && data == NULL);
    commands++;
    if (reject_command) { reject_command = false; return -1; }
    return 0;
}
void bt_music_post_msg_to_lvgl_ui(const char *message)
{
    assert(!strcmp(message, "music_album_pic"));
    notifications++;
}

/** @brief 验证重复请求不中断数据、等待 START 时换歌、连续快切和断开。 */
int main(void)
{
    u8 data[] = {1, 2, 3, 4, 5, 6};
    bip_mem_init();
    pet_dashboard_music_album_select("0000052");
    assert(pet_dashboard_music_album_request() == 0 && commands == 1);
    bip_rx_data_handle(data, 2, 6, 1);
    u8 *buffer = bip_file_data_get();
    bip_rx_data_handle(NULL, 0, 0, 5);
    bip_rx_data_handle(NULL, 0, 0, 6);
    assert(bip_file_status_get() == 2 && bip_file_data_get() == buffer && bip_file_data_size() == 2);
    assert(pet_dashboard_music_album_request() == 0 && commands == 1);
    bip_rx_data_handle(data + 2, 2, 0, 2);
    bip_rx_data_handle(data + 4, 2, 0, 3);
    assert(bip_file_status_get() == 1 && bip_file_data_size() == 6);
    assert(!memcmp(bip_file_data_get(), data, 6));
    bip_rx_data_handle(NULL, 0, 0, 5);
    assert(bip_file_status_get() == 1 && bip_file_data_get() == buffer);
    puts("PASS: duplicate/pending request preserves stream bytes; complete JPEG survives duplicate status");

    pet_dashboard_music_album_select("0000053");
    assert(bip_file_status_get() == 0);
    assert(pet_dashboard_music_album_request() == 0 && commands == 2);
    /* 旧请求已发但尚未 START，连续切到两个新封面。 */
    pet_dashboard_music_album_select("0000054");
    assert(pet_dashboard_music_album_request() == 0 && commands == 2);
    bip_rx_data_handle(NULL, 0, 0, 6);
    pet_dashboard_music_album_select("0000055");
    assert(pet_dashboard_music_album_request() == 0 && commands == 2);
    bip_rx_data_handle(data, 2, 6, 1);
    bip_rx_data_handle(data + 2, 4, 0, 3);
    assert(bip_file_status_get() == 0); /* 0000053 不能标成 0000055。 */
    assert(pet_dashboard_music_album_request() == 0 && commands == 2);
    /* SDK 在旧 STOP 后自动续取最新句柄。 */
    bip_rx_data_handle(data, 2, 6, 1);
    bip_rx_data_handle(data + 2, 4, 0, 3);
    assert(bip_file_status_get() == 1 && !memcmp(bip_file_data_get(), data, 6));
    puts("PASS: rapid changes before START discard old cover; SDK continuation accepts latest handle without extra command");

    pet_dashboard_music_album_select("0000056");
    reject_command = true;
    assert(pet_dashboard_music_album_request() == -1);
    assert(pet_dashboard_music_album_request() == 0 && commands == 4);
    bip_rx_data_handle(data, 2, 6, 1);
    pet_dashboard_music_album_reset();
    bip_rx_data_handle(data + 2, 4, 0, 3);
    assert(bip_file_status_get() == 0);
    bip_rx_data_handle(NULL, 0, 0, 4);
    assert(!bip_file_data_get());
    assert(notifications == 4);
    puts("PASS: rejected commands retry; disconnect drops late STOP; real error frees buffer");
    return 0;
}
