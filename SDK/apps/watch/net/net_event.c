#include "typedef.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE

#include "lwip.h"
#include "event.h"



/* extern const int CONFIG_LWIP_NET_ENABLE; */
static volatile u8 bt_dhcp_flag = 0;

int bt_lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event)
{
    switch (event) {
    case LWIP_BT_DHCP_BOUND_TIMEOUT:
        puts("LWIP_BT_DHCP_BOUND_TIMEOUT\n");
        break;
    case LWIP_BT_DHCP_BOUND_RELEASE:
        puts("LWIP_BT_DHCP_BOUND_RELEASE\n");
        bt_dhcp_flag = 0;
        break;
    case LWIP_BT_DHCP_BOUND_SUCC:
        puts("LWIP_BT_DHCP_BOUND_SUCC\n");
        bt_dhcp_flag = 1;
        break;
    default:
        break;
    }
    return 0;
}

u8 net_get_dhcp_flag()
{
    return bt_dhcp_flag;
}

#endif /* 网络功能宏 */

