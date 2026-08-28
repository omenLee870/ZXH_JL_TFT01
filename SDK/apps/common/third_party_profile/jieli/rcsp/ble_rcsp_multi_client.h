// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_RCSP_MULTI_CLIENT_H
#define _BLE_RCSP_MULTI_CLIENT_H

#include "ble_user.h"
#include "typedef.h"

void rcsp_bt_multi_client_init();
void rcsp_bt_multi_client_exit(void);
void ble_client_module_enable(u8 en);
void ble_client_set_remote_addr(u8 *addr);
void ble_multi_client_disconnect(void);
ble_state_e ble_client_get_cur_work_state(void);


#endif // _BLE_RCSP_MULTI_CLIENT_H
