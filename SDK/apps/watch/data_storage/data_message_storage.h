#ifndef _WATCH_DATA_MESSAGE_STORAGE_
#define _WATCH_DATA_MESSAGE_STORAGE_



/**********************
 *      TYPEDEFS
 **********************/
#define MESSAGE_TIMESTAMP_TYPE                      (0)
#define MESSAGE_PACKAGENAME_TYPE                    (1)
#define MESSAGE_APP_IDENTIFIER_TYPE                 (2)
#define MESSAGE_TITLE_TYPE                          (3)
#define MESSAGE_CONTENT_TYPE                        (4)


#define MESSAGE_TIMESTAMP_LEN                       (4)
#define MESSAGE_PACKAGENAME_LEN                     (31+1)
#define MESSAGE_APP_IDENTIFIER_LEN                  (1)
#define MESSAGE_TITLE_LEN                           (36+1)
#define MESSAGE_CONTENT_LEN                         (439+1)

#define SMALL_FILE_MESSAGE_SIZE                     (sizeof(small_file_message_t))
#define SMALL_FILE_MESSAGE_PARTITION_NAME           "message"


#define PACKAGE_NAME_SYS_MESSAGE_RECEIVE        "MobileSMS_RECEIVE"
#define PACKAGE_NAME_SYS_MESSAGE_SEND           "MobileSMS_SEND"

#define IOS_PACKAGE_NAME_SYS_MESSAGE    		"com.apple.MobileSMS"
#define IOS_PACKAGE_NAME_WECHAT         		"com.tencent.xin"
#define IOS_PACKAGE_NAME_QQ             		"com.tencent.mqq"
#define IOS_PACKAGE_NAME_DING_DING      		"com.laiwang.DingTalk"

#pragma pack(1)
typedef struct message {
    u32 timestamp;
    char packagename[MESSAGE_PACKAGENAME_LEN];
    u8 app_identifier;
    u8 title[MESSAGE_TITLE_LEN];
    u8 content[MESSAGE_CONTENT_LEN];
} small_file_message_t;
#pragma pack()



int ui_small_file_message_get_count(void);
int small_file_message_read_by_index(small_file_message_t *message, int index);
void message_set_info_from_ancs(void *info, void *name, void *data, u16 len);
void message_add_info_from_ancs(void *info);



#endif /*_WATCH_DATA_MESSAGE_STORAGE_*/



