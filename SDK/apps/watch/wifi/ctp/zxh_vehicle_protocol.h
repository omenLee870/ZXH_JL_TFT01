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
    ZXH_UNCONFIRMED = 2005,
    ZXH_POWER_OFF = 2006 /**< 仪表业务已关闭，需先完成开机。 */
};

/** @brief 动作编号沿用现有 APP 协议；INSTRUMENT 范围的 LOCK/UNLOCK 仅指仪表待机/唤醒。 */
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
    int gear;        /**< 控制器档位；不代替 mode 驾驶模式。 */
    int link;        /**< 通信状态：0 等待、1 在线、2 掉线，未知为 -1。 */
    int trip_m;      /**< 仪表本地小计里程，单位 m。 */
    int ride_time_s; /**< 本次骑行累计时长，单位 s。 */
    int status;      /**< vehicle_status 位图，未知为 -1。 */
    int known_status; /**< 数据源支持的状态位；未支持的位不能当作关闭。 */
    int faults;      /**< vehicle_fault 位图，未知为 -1；不是厂家 E 码。 */
    int power_on;    /**< 仪表业务完成开机为 1，不表示整车电门。 */
    int power_state; /**< 0 运行、1 关机中、2 已关机、3 开机中、4 故障。 */
    int finding;     /**< 仪表寻车提示音状态。 */
};
struct zxh_bms {
    int soc, voltage_mv, current_ma, temperature_dc, charging, cycles;
};

/**
 * @brief 查询车辆后端已经接通的 APP 控制操作。
 * @return 按 zxh_action 编号组成的位图；默认返回 0，表示尚未接通控制操作。
 * @note 在协议互斥锁内调用，必须立即返回；能力声明不能代替鉴权或实际执行反馈。
 *       新增控制通道后由后端覆盖本接口，只声明已经实现的操作。
 */
uint32_t zxh_vehicle_supported_actions(void);
/** @brief 控制对象：INSTRUMENT 表示仪表本机，NONE 表示未接入控制。 */
const char *zxh_vehicle_control_scope(void);

/* Override these weak hooks when implementing the physical control chain.
 * Hooks run with the protocol mutex held: never block, send CTP, or complete inline.
 * submit returns ZXH_PENDING for deferred work, otherwise a terminal result.
 * Only set confirmed after execution feedback; transport ACK is not confirmation.
 * Instrument actions require an active BLE session. Physical vehicle actions, if
 * added separately, must enforce ownership/authentication and motion interlocks. */
int zxh_vehicle_authorize(void *client, const struct zxh_request *request);
int zxh_vehicle_authorize_read(void *client, const char *topic);
int zxh_vehicle_submit(void *client, uint32_t token, const struct zxh_request *request, int *confirmed);
int zxh_vehicle_read_state(struct zxh_state *state);
int zxh_vehicle_read_bms(struct zxh_bms *bms);
/* Task context only. Late/duplicate completions are ignored. Token is not an ID. */
void zxh_vehicle_complete(uint32_t token, int code, int confirmed);
void zxh_vehicle_disconnect(void *client);
int zxh_vehicle_receive(void *client, const char *topic, char *json);

#endif
