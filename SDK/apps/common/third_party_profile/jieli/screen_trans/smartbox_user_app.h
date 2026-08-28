#ifndef SMARTBOX_USER_APP_H
#define SMARTBOX_USER_APP_H

#include "typedef.h"
#include "sys_time.h"

struct custom_music_info {
    u16 time;
    u8 type_artist_name;
    u8 name_len;
    u8 type_album_name;
    u8 album_len;
    u8 type_title;
    u8 title_len;
    u8 artist_name[256];
    u8 album_name[256];
    u8 title[256];
};

#define LYRICS_MAX_SIZE 256
#define LYRICS_MAX_NUM  3

#define STR_MAX_SIZE    LYRICS_MAX_SIZE
#define STR_MAX_NUM     LYRICS_MAX_NUM

extern struct custom_music_info local_music_info;
extern char lyrics_artist_name_array[STR_MAX_SIZE] ;



void sbox_state_handler(int chgbox_event);
void sbox_power_on_check(void);
void sbox_update_earphone_mac(void);

void custom_client_send_volume_up(void);
void custom_client_send_volume_down(void);
void custom_client_send_music_ctrl(u8 state);
void custom_client_send_music_play(void);
void custom_client_send_music_pause(void);
void custom_client_send_music_prev(void);
void custom_client_send_music_next(void);
void custom_client_send_find_earphone(u8 cmd);
void custom_client_send_anc_mode(u8 mode);
void custom_client_send_eq_off(void);
void custom_client_send_eq_mode(u8 val);
void custom_client_send_poweroff_earphone(void);
void custom_client_send_call(u8 data);
void custom_client_send_alarm_earphone(u8 cmd);
void custom_client_send_ble_setting(u8 data);
void custom_client_send_box_all_info(void);
void custom_client_send_ctrl_tiktop(u8 cmd);
void custom_client_send_ctrl_photo(u8 cmd);
void custom_client_send_ctrl_siri(u8 data);
void custom_client_send_ctrl_edr_info(void);
void custom_client_sleep_control(u8 on);
u8 get_sleep_control_status(void);
void set_sleep_control_status(u8 en);
void set_emitter_info(void);
void custom_client_send_ctrl_edr_conn(u8 cmd);
void custom_client_send_ctrl_siri(u8 data);
void custom_client_send_ctrl_phoneout(u8 *buf);
int ble_notify_recv_data_handler(uint8_t *buffer, uint16_t buffer_size);
int ble_recv_musicdata_handler(uint8_t *buffer, uint16_t buffer_size);
void ear_inbox_state_deal(u8 status);
void ble_connect_snyc_info(void *p);
void set_adv_close(void *p);
void user_check_scan_timeout(void *p);
u8 *sbox_key_info_get(void);
u8 *sbox_eq_gain_get(u8 *gain);
void sbox_key_info_set(u8 *mysetting);
void sbox_eq_gain_set(u8 *gain);
int ble_recv_musicdata_handler(uint8_t *buffer, uint16_t buffer_size);
void sbox_get_earphone_mac(u8 *addr);

#endif
