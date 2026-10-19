/**
 * @file vehicle_core.h
 * @brief 车辆业务状态、协议适配输入及业务核心接口。
 * @note 本模块不依赖 LVGL、RTOS 或具体控制器协议。所有时间使用同一毫秒时基，
 *       调用方负责串行访问业务核心；字段是否可用以 valid 和 known_status 为准。
 */
#ifndef VEHICLE_CORE_H
#define VEHICLE_CORE_H

#include <stdint.h>

/** @brief 当前接入的车辆数据来源，不表示该来源具备车辆控制能力。 */
enum vehicle_source {
    VEHICLE_SOURCE_NONE, /**< 尚未接入车辆数据源。 */
    VEHICLE_SOURCE_YYB,  /**< 优宜宝一线通，只接收控制器上报数据。 */
    VEHICLE_SOURCE_SHENLONG /**< 神龙锂电2号双向 UART。 */
};

/** @brief 控制器通信状态，区分首次等待与曾经在线后的掉线。 */
enum vehicle_link {
    VEHICLE_LINK_WAITING, /**< 尚未收到过在线数据。 */
    VEHICLE_LINK_ONLINE,  /**< 最近数据仍在有效时间内。 */
    VEHICLE_LINK_LOST     /**< 曾经收到在线数据，当前已掉线或超时。 */
};

/** @brief 依据有效车速作出的运动判定，不能用未知状态代替静止。 */
enum vehicle_motion {
    VEHICLE_MOTION_UNKNOWN, /**< 车速无效或通信掉线，无法判断运动状态。 */
    VEHICLE_STOPPED,        /**< 有效车速低于业务阈值 1.0 km/h，不代表严格零速。 */
    VEHICLE_MOVING          /**< 有效车速达到或超过 1.0 km/h。 */
};

/** @brief 数据有效位；数值为 0 与数据未知必须分别处理。 */
enum vehicle_valid {
    VEHICLE_SPEED_VALID = 1u << 0,    /**< 车速有效。 */
    VEHICLE_SOC_VALID = 1u << 1,      /**< 电量百分比有效。 */
    VEHICLE_CURRENT_VALID = 1u << 2,  /**< 电流有效，允许负电流。 */
    VEHICLE_VOLTAGE_VALID = 1u << 3,  /**< 电池电压有效。 */
    VEHICLE_GEAR_VALID = 1u << 4,     /**< 档位有效。 */
    VEHICLE_LOAD_VALID = 1u << 5,     /**< 控制器负载百分比有效。 */
    VEHICLE_DISTANCE_VALID = 1u << 6, /**< 总里程和小计里程有效。 */
    VEHICLE_STATUS_VALID = 1u << 7,   /**< 状态位有效，具体支持项见 known_status。 */
    VEHICLE_FAULTS_VALID = 1u << 8    /**< 故障位有效，包含本地产生的通信故障。 */
};

/** @brief 统一车辆状态位，由协议适配层转换，界面不直接解释协议原始位。 */
enum vehicle_status {
    VEHICLE_PARK = 1u << 0,         /**< 驻车状态。 */
    VEHICLE_SIDE_STAND = 1u << 1,   /**< 侧撑状态。 */
    VEHICLE_CRUISE = 1u << 2,       /**< 巡航已激活。 */
    VEHICLE_ASSIST = 1u << 3,       /**< 助力状态。 */
    VEHICLE_RUNNING = 1u << 4,      /**< 控制器报告运行，不替代实际车速判定。 */
    VEHICLE_BRAKE = 1u << 5,        /**< 刹车输入有效。 */
    VEHICLE_REGEN = 1u << 6,        /**< 滑行回充状态。 */
    VEHICLE_ONE_KEY = 1u << 7,      /**< 协议中的一键通状态标志。 */
    VEHICLE_CURRENT_70 = 1u << 8,   /**< 协议中的 70% 电流标志。 */
    VEHICLE_EKK = 1u << 9,          /**< EKK 状态标志，保留控制器协议含义。 */
    VEHICLE_REVERSE = 1u << 10,     /**< 倒车状态。 */
    VEHICLE_EBRAKE = 1u << 11,      /**< 电子刹车状态。 */
    VEHICLE_SPEED_LIMIT = 1u << 12, /**< 控制器报告限速状态。 */
    VEHICLE_HEADLIGHT = 1u << 13,   /**< 大灯状态，数据来源由适配层提供。 */
    VEHICLE_POSITION_LIGHT = 1u << 14, /**< 示廓灯状态。 */
    VEHICLE_LEFT_TURN = 1u << 15,   /**< 左转向开关，UI负责闪烁。 */
    VEHICLE_RIGHT_TURN = 1u << 16,  /**< 右转向开关，UI负责闪烁。 */
    VEHICLE_CHARGING = 1u << 17,    /**< 外部充电状态，独立于滑行回充。 */
    VEHICLE_MODE_ECO = 1u << 18,
    VEHICLE_MODE_CITY = 1u << 19,
    VEHICLE_MODE_SPORT = 1u << 20
};

/** @brief 统一故障位；位值不是厂家故障码，不能直接作为 E 码显示。 */
enum vehicle_fault {
    VEHICLE_HALL_FAULT = 1u << 0,         /**< 霍尔故障。 */
    VEHICLE_THROTTLE_FAULT = 1u << 1,     /**< 转把故障。 */
    VEHICLE_CONTROLLER_FAULT = 1u << 2,   /**< 控制器故障。 */
    VEHICLE_UNDERVOLTAGE = 1u << 3,       /**< 欠压保护。 */
    VEHICLE_PHASE_FAULT = 1u << 4,        /**< 电机缺相故障。 */
    VEHICLE_PROTECTION = 1u << 5,         /**< 控制器其他保护状态。 */
    VEHICLE_ANTI_RUNAWAY = 1u << 6,       /**< 防飞车保护。 */
    VEHICLE_OVERCURRENT = 1u << 7,        /**< 过流保护。 */
    VEHICLE_STALL = 1u << 8,              /**< 电机堵转。 */
    VEHICLE_COMMUNICATION_FAULT = 1u << 9 /**< 业务层判定的本地通信掉线故障。 */
};

/** @brief 车辆数值快照；使用字段前先检查对应的数据有效位。 */
struct vehicle_values {
    uint32_t valid,        /**< 数据有效位，取值为 vehicle_valid 的组合。 */
             status,       /**< 当前置位的车辆状态，取值为 vehicle_status 的组合。 */
             known_status, /**< 数据源实际支持的状态位，未支持的状态不能当作关闭。 */
             faults;       /**< 当前故障位，取值为 vehicle_fault 的组合。 */
    int32_t speed_dkph;     /**< 真实车速，单位 0.1 km/h；未知为 -1，不受限速设置截断。 */
    int32_t voltage_mv;     /**< 电池电压，单位 mV；未知为 -1。 */
    int32_t current_ma;     /**< 电流，单位 mA；回充允许为负数，未知为 INT32_MIN。 */
    int16_t soc,          /**< 电量百分比，范围 0~100；未知为 -1。 */
            gear,          /**< 协议适配后的档位；未知为 -1。 */
            load_percent;  /**< 负载百分比，范围 0~100；未知为 -1。 */
    uint64_t odometer_mm, /**< 累计总里程，单位 mm，来自已有里程累计模块。 */
             trip_mm;      /**< 本次小计里程，单位 mm；本业务核心不重复积分。 */
};

/** @brief 协议适配层提交的一次数据输入，接收时间必须保留原始报文时间。 */
struct vehicle_input {
    struct vehicle_values values; /**< 当前报文对应的数值和有效位。 */
    enum vehicle_source source;   /**< 当前数据来源。 */
    uint32_t received_at_ms,    /**< 最近有效报文的接收时间，单位 ms。 */
             timeout_ms;          /**< 报文有效期，单位 ms；0 表示不能作为在线数据使用。 */
    uint8_t online;               /**< 协议模块的在线标志，非 0 表示在线。 */
};

/** @brief 对外发布的车辆业务状态；显示、按键和 APP 后续共用此状态定义。 */
struct vehicle_state {
    struct vehicle_values values; /**< 数值、状态及故障快照。 */
    enum vehicle_source source;   /**< 当前接入的数据来源。 */
    enum vehicle_link link;       /**< 等待、在线或掉线。 */
    enum vehicle_motion motion;   /**< 运动判定，掉线时必须为未知。 */
    uint32_t received_at_ms;       /**< 最近一次接受的在线报文接收时间，单位 ms。 */
    uint64_t ride_time_ms;         /**< 本次核心初始化后累计的骑行时间，单位 ms，不写 Flash。 */
    uint32_t auxiliary_status, physical_status; /**< 当前独立输入有效状态位；不代表控制器在线。 */
    unsigned auxiliary_gear, physical_gear; /**< 档位来自独立输入/物理输入。 */
};

enum vehicle_aux_source { VEHICLE_AUX_TEST, VEHICLE_AUX_PHYSICAL, VEHICLE_AUX_COUNT };
#define VEHICLE_RIDE_MODE_MASK (VEHICLE_MODE_ECO | VEHICLE_MODE_CITY | VEHICLE_MODE_SPORT)
#define VEHICLE_AUX_STATUS_MASK (VEHICLE_HEADLIGHT | VEHICLE_POSITION_LIGHT | VEHICLE_LEFT_TURN | VEHICLE_RIGHT_TURN | VEHICLE_REVERSE | VEHICLE_CHARGING | VEHICLE_RIDE_MODE_MASK)
/** @brief 已知且单选的骑行模式；未知返回-1。物理适配一次发布完整模式掩码。 */
static inline int vehicle_values_ride_mode(const struct vehicle_values *v)
{
    if (!(v->valid & VEHICLE_STATUS_VALID) ||
        (v->known_status & VEHICLE_RIDE_MODE_MASK) != VEHICLE_RIDE_MODE_MASK) return -1;
    switch (v->status & VEHICLE_RIDE_MODE_MASK) {
    case VEHICLE_MODE_ECO: return 0;
    case VEHICLE_MODE_CITY: return 1;
    case VEHICLE_MODE_SPORT: return 2;
    default: return -1;
    }
}
struct vehicle_aux_input {
    uint32_t known, status, received_at_ms, timeout_ms;
    int gear; /**< -1表示该来源不提供档位；否则0~15。 */
};

/** @brief 业务核心内部上下文，由所属执行线程维护。 */
struct vehicle_core {
    struct vehicle_state state; /**< 当前业务状态。 */
    uint32_t timeout_ms,       /**< 当前报文有效期，单位 ms。 */
             clock_ms;          /**< 上次计时处理位置，单位 ms，避免重复累计时间。 */
    uint8_t received_once;       /**< 是否曾收到在线输入，用于区分首次等待与后续掉线。 */
    struct vehicle_aux_input auxiliary[VEHICLE_AUX_COUNT];
};

/** @brief 统一发布测试或物理状态，串行调用；known包含明确关闭的位。物理输入逐字段优先。 */
int vehicle_core_aux_publish(struct vehicle_core *core, enum vehicle_aux_source source,
    uint32_t known, uint32_t status, int gear, uint32_t now_ms, uint32_t timeout_ms);
/** @brief 将有效独立输入合并到输出副本，不修改原协议快照、通信在线、运动判定或里程。 */
void vehicle_core_aux_apply(const struct vehicle_core *core, struct vehicle_state *out, uint32_t now_ms);

/**
 * @brief 初始化车辆业务核心。
 * @param[out] core 待初始化的核心上下文，不能为空。
 * @param[in] now_ms 当前毫秒时基。
 * @retval 无。
 * @note 清空本次骑行时间，通信状态设为首次等待，实时数值设为未知。
 */
void vehicle_core_init(struct vehicle_core *core, uint32_t now_ms);

/**
 * @brief 接收协议适配层数据并更新车辆业务状态。
 * @param[in,out] core 核心上下文，不能为空，调用方负责串行访问。
 * @param[in] input 本次输入；为空时直接返回。
 * @param[in] now_ms 当前毫秒时基，必须与接收时间使用同一时钟。
 * @retval 无。
 * @note 先结算已有状态的骑行时间，再检查输入有效期；重复读取旧报文不能延长在线。
 *       里程只接收已有累计值，本函数不再次积分，也不执行 Flash 保存。
 */
void vehicle_core_update(struct vehicle_core *core, const struct vehicle_input *input, uint32_t now_ms);

/**
 * @brief 推进骑行计时并检查通信超时。
 * @param[in,out] core 核心上下文，不能为空，调用方负责串行访问。
 * @param[in] now_ms 当前毫秒时基。
 * @retval 无。
 * @note 只累计有效车速大于 0 的时间，最多计到报文有效期结束；掉线保留里程，
 *       清除过期实时数据。无符号时间差可处理一次 32 位时基回绕。
 */
void vehicle_core_poll(struct vehicle_core *core, uint32_t now_ms);
#endif
