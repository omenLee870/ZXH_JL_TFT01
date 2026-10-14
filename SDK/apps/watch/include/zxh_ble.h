#ifndef ZXH_BLE_H
#define ZXH_BLE_H
#include <stdint.h>
#define ZXH_BLE_SERVICE_HANDLE 0x0020
#define ZXH_BLE_RX_HANDLE 0x0022
#define ZXH_BLE_TX_HANDLE 0x0024
#define ZXH_BLE_CCC_HANDLE 0x0025
void zxh_ble_profile_ready(void);
int zxh_ble_write(void *hdl, uint16_t connection, uint16_t handle, uint16_t mode,
                  uint16_t offset, const uint8_t *data, uint16_t size);
uint16_t zxh_ble_read(uint16_t connection, uint16_t offset, uint8_t *data, uint16_t size);
void zxh_ble_disconnected(void *hdl);
/* Client is an opaque tagged session token, NOT a dereferenceable CTP pointer. */
int zxh_ble_is_client(void *client);
int zxh_ble_client_active(void *client);
uint16_t zxh_ble_client_connection(void *client);
int zxh_ble_send(void *client, const char *json);
#endif
