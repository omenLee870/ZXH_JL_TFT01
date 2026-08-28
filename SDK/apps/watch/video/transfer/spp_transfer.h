#ifndef SPP_TRANSFER_H
#define SPP_TRANSFER_H

#include "btstack/btstack_typedef.h"

#define SPP_TX_MIN_RAM_LIMIT    (1 * 1024)
#define SPP_TX_WAIT_TIMEOUT_TICKS (10)  /* TX 缓冲不足时有界等待排空的超时(OS tick, 1 tick=10ms → ~100ms) */
#define SPP_TX_THROTTLE_LIMIT   (4 * 1024)  /* TX 缓冲低于此值时启用分片限速(10ms 间隔), 平滑突发避免打到有界等待 */

void spp_protocol_init(void);
void spp_protocol_deinit(void);
int spp_send(u8 *data, u32 len);
int user_spp_data_packet_total_get(void);
int user_spp_data_packet_count_get(bd_addr_t addr);
#endif


