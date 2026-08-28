#include "generic/typedef.h"
#include "app_config.h"

#if TCFG_EXT_WIFI_ENABLE || TCFG_UART_NET_ENABLE || TCFG_SPI_NET_ENABLE
#include "debug.h"

REGISTER_LOG_TAG(TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, CTP_CMD)
#endif
