#ifndef ZXH_VEHICLE_PROTOCOL_H
#define ZXH_VEHICLE_PROTOCOL_H

#include <stdint.h>

#define ZXH_PROTOCOL_VERSION 1
#define ZXH_ID_SIZE 33
#define ZXH_EXECUTION_MS 8000
#define ZXH_PENDING (-1)

enum zxh_result {
    ZXH_OK = 0,
    ZXH_VERSION_ERROR = 1001,
    ZXH_PARAMETER_ERROR = 1002,
    ZXH_ACTION_ERROR = 1003,
    ZXH_BUSY = 1004,
    ZXH_ID_CONFLICT = 1005,
    ZXH_EXECUTION_FAILED = 2001,
    ZXH_EXECUTION_TIMEOUT = 2002,
    ZXH_SAFETY_BLOCKED = 2003,
    ZXH_AUTH_FAILED = 2004,
    ZXH_UNCONFIRMED = 2005
};

enum zxh_action {
    ZXH_LOCK, ZXH_UNLOCK, ZXH_OPEN_SEAT, ZXH_CLOSE_SEAT,
    ZXH_LIGHT_SET, ZXH_FIND, ZXH_MODE_SET,
    ZXH_CHARGE_TARGET_SET, ZXH_PROXIMITY_SET, ZXH_ACTION_COUNT
};

struct zxh_request {
    char id[ZXH_ID_SIZE];
    enum zxh_action action;
    int value;
};

/* Unknown fields are -1; temperature is -32768 and current is INT32_MIN. */
struct zxh_state {
    int locked, seat_open, light_on, mode, proximity, charge_target;
    int speed_dkph, odometer_m, range_m;
};
struct zxh_bms {
    int soc, voltage_mv, current_ma, temperature_dc, charging, cycles;
};

/* Override these weak hooks when implementing the physical control chain.
 * Hooks run with the protocol mutex held: never block, send CTP, or complete inline.
 * submit returns ZXH_PENDING for deferred work, otherwise a terminal result.
 * Only set confirmed after physical feedback; transport ACK is not confirmation.
 * authorize must enforce ownership/authentication and motion interlocks. */
int zxh_vehicle_authorize(void *client, const struct zxh_request *request);
int zxh_vehicle_authorize_read(void *client, const char *topic);
int zxh_vehicle_submit(uint32_t token, const struct zxh_request *request, int *confirmed);
int zxh_vehicle_read_state(struct zxh_state *state);
int zxh_vehicle_read_bms(struct zxh_bms *bms);
/* Task context only. Late/duplicate completions are ignored. Token is not an ID. */
void zxh_vehicle_complete(uint32_t token, int code, int confirmed);
void zxh_vehicle_disconnect(void *client);
int zxh_vehicle_receive(void *client, const char *topic, char *json);

#endif
