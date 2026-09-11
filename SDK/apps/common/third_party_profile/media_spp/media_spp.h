#ifndef MEDIA_SPP_H
#define MEDIA_SPP_H

/* 流媒体独立 SPP 通道。本模块只提供透传通道, 不做协议解析。 */

/* 总开关: 1=开启(默认), 0=关闭(空实现, SDP 不注册) */
#ifndef MEDIA_SPP_CHANNEL_EN
#define MEDIA_SPP_CHANNEL_EN    1
#endif

/* 0x1101 命令通道上是否继续旁路解析流媒体数据(兼容旧 App)。
 * 独立通道启用后默认关闭, 避免新旧两条通道重复收包。 */
#ifndef MEDIA_SPP_LEGACY_CH1_ROUTE
#if MEDIA_SPP_CHANNEL_EN
#define MEDIA_SPP_LEGACY_CH1_ROUTE  0
#else
#define MEDIA_SPP_LEGACY_CH1_ROUTE  1
#endif
#endif

typedef void (*media_spp_recv_handler_t)(void *hdl, void *remote_addr, u8 *buf, u16 len);
typedef void (*media_spp_state_handler_t)(void *hdl, void *remote_addr, u8 state);

int media_spp_init(void);
int media_spp_exit(void);
int media_spp_send(u8 *buf, u16 len);
u8  media_spp_is_connected(void);
void *media_spp_get_hdl(void);
int media_spp_recv_handler_register(media_spp_recv_handler_t handler);
int media_spp_state_handler_register(media_spp_state_handler_t handler);

#endif /* MEDIA_SPP_H */
