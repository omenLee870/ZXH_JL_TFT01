#ifndef _DEV_DESC_H_
#define _DEV_DESC_H_

#include "app_config.h"

#define DEV_DESC_HEAD       "{\"app_list\": {"\
                            "\"match_android_ver\": "\
                            "[\"1.0\",\"2.0\"],"\
                            "\"match_ios_ver\": "\
                            "[\"1.0\",\"2.0\"]},"
//AC521x前视只有720 VGA
#define FORW_SPT            DEV_DESC_HEAD\
                            "\"forward_support\": ["\
                                "\"0\","\
                            "],"

//AC521x后视只有720 VGA
#define BIHD_SPT             FORW_SPT\
                            "\"behind_support\": ["\
                                "\"0\""\
                            "],"

#define FORW_REC_SPT        BIHD_SPT\
                            "\"forward_record_support\": ["\
                                "\"0\","\
                                "\"1\""\
                            "],"

//AC521x后视录像只有720 VGA
#define BIHD_REC_SPT        FORW_REC_SPT\
                            "\"behind_record_support\": ["\
                                "\"0\""\
                            "],"

//AC521x RTSP前视
#define RTSP_FSPT           BIHD_REC_SPT\
                            "\"rtsp_forward_support\": ["\
                                "\"0\","\
                                "\"1\""\
                            "],"

//AC521x RTSP后视只有720 VGA
#define RTSP_HSPT           RTSP_FSPT\
                            "\"rtsp_behind_support\": ["\
                                "\"0\""\
                            "],"

#define   DEV_TYPE          RTSP_HSPT\
                            "\"device_type\": \"1\","

//实时流传输协议配置
#ifdef CONFIG_NET_UDP_ENABLE
#define     NET_TYPE        DEV_TYPE\
                            "\"net_type\": \"1\","
#else
#define     NET_TYPE        DEV_TYPE\
                            "\"net_type\": \"0\","
#endif

//实时流类型配置
#ifdef CONFIG_NET_JPEG
#define     PKG_TYPE        NET_TYPE\
                            "\"rts_type\": \"0\","
#else
#define     PKG_TYPE        NET_TYPE\
                            "\"rts_type\": \"1\","
#endif

//产品名称配置
#define   PROD_TYPE         PKG_TYPE\
                            "\"product_type\": \"AC792x_wifi_car_camera\","

//投屏
#ifdef CONFIG_NET_SCR
#define   NET_SCR           PROD_TYPE\
                            "\"support_projection\":\"1\","

#else
#define   NET_SCR           PROD_TYPE\
                            "\"support_projection\":\"0\","

#endif


#define   DEV_DESC_STRING 	NET_SCR\
                            "\"firmware_version\": \"1.0.1\","\
                            "\"match_app_type\": \"DVRunning 2\","\
                            "\"uuid\": \"xxxxxx\"}"


/***************************http_虚拟文件注册**********************************/
#define DEV_DESC_PATH		"mnt/spiflash/res/dev_desc.txt"
#define DEV_DESC_CONTENT 	DEV_DESC_STRING



#endif


