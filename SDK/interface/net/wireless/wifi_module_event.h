
#ifndef _EXT_WIFI_MODULE_EVENT_H_
#define _EXT_WIFI_MODULE_EVENT_H_

//#include "wireless/wifi_connect.h"

enum WIFI_MODULE {
    RTL8733BS,
    AIC8800D40,
    NONE_MODULE = -1,
};

//enum WIFI_MODE {
//    STA_MODE,
//    AP_MODE,
//    SMP_CFG_MODE,
//    MP_TEST_MODE,
//    P2P_MODE,
//    NONE_MODE,
//};


//enum P2P_ROLE {
//    P2P_GC_MODE,
//    P2P_GO_MODE,
//};


//enum SMP_CFG_TYPE {
//    PRIV_SMP_CFG1 = 1,
//    PRIV_SMP_CFG2,
//    AIRKISS_SMP_CFG,
//};

enum WIFI_MODULE_EVENT {
    SMP_CFG_START = 0x0,
    SMP_CFG_STOP = 0x1,
    SMP_CFG_TIMEOUT = 0x2,
    SMP_CFG_COMPLETED = 0x3,
    AP_ON_DISCONNECTED = 0x4,
    AP_ON_ASSOC = 0x5,
    PM_SUSPEND = 0x6,
    PM_RESUME = 0x7,
    P2P_GO_STA_CONNECTED = 0x8,
    P2P_GO_STA_DISCONNECTED = 0x9,
    P2P_WSC_OPERATION = 0xA,
};

struct netdev_ssid_info {
    char ssid[32];
    unsigned int ssid_len;
    unsigned char mac_addr[6];
    long rssi;
    unsigned int channel_number;
    unsigned char	SignalStrength;//(in percentage)
    unsigned char	SignalQuality;//(in percentage)
    unsigned char   SupportedRates[16];
    unsigned char   security;   //加密方式, 取值见 AIC_WIFI_SECURITY_TYPE(KEY_NONE/WEP/WPA/WPA2/WPA3...)
};


/*爱科微扫描结果ssid结构体*/
#define MAX_AP_COUNT 32
typedef struct wifi_ap_info {
    unsigned char   ssid[32];
    unsigned int    ssid_len;
    unsigned char   bssid[6];
    unsigned int    channel;
    unsigned int    rssi;
    unsigned char   security;   //加密方式, 取值见 AIC_WIFI_SECURITY_TYPE(KEY_NONE/WEP/WPA/WPA2/WPA3...)
} wifi_ap_info_t;

typedef struct _wifi_ap_list {
    unsigned short  ap_count;
    wifi_ap_info_t ap_info[MAX_AP_COUNT];
} wifi_ap_list_t;

//struct smp_cfg_result {
//    enum SMP_CFG_TYPE type;
//    unsigned int ssid_crc;
//    unsigned int random_val;
//    char ssid[32];
//    char passphrase[64];
//};

#endif  //_WIFI_MODULE_EVENT_H_

