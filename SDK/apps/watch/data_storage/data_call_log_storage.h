#ifndef _WATCH_DATA_CALL_LOG_STORAGE_
#define _WATCH_DATA_CALL_LOG_STORAGE_

/*********************
 *      DEFINES
 *********************/
#define CALL_LOG_NAME_LEN                       (20)
#define CALL_LOG_NUMBER_LEN                     (20)
#define CALL_LOG_DATE_LEN                       (20)
#define SMALL_FILE_CALL_LOG_SIZE                (sizeof(small_file_call_log_t))


/**********************
 *      TYPEDEFS
 **********************/
#pragma pack(1)
typedef struct call_log {
    u32 utc_time;
    char name[CALL_LOG_NAME_LEN];
    char number[CALL_LOG_NUMBER_LEN];
    u8  type;   /*通话类型*/
    u8  sel;    /*通话设备*/
    u32 mask;
} small_file_call_log_t;
#pragma pack()

enum CALL_SEL {
    CALL_SEL_AUTO = 0,
    CALL_SEL_BT,        /*蓝牙通话*/
    CALL_SEL_CAT1,      /*CAT1通话*/
    CALL_SEL_ERA_BLE,   /*耳机通话*/

    CALL_SEL_MAX,
};

enum CALL_TYPE {
    CALL_OUT = 0,       /*电话播出*/
    CALL_INCOME,        /*电话打入*/
    CALL_INCOME_REJECT, /*电话打入拒接*/
};


/**********************
 *  GLOBAL PROTOTYPES
 **********************/

/*通话记录接口*/
void small_file_call_log_set_name(char *name, int name_len);
void small_file_call_log_set_number(char *number, int number_len);
void small_file_call_log_set_date(void);
u32 small_file_call_log_get_date(void);
void small_file_call_log_set_type(enum CALL_TYPE type);
void small_file_call_log_set_sel(enum CALL_SEL call_sel);
int small_file_call_log_save(void);

int ui_small_file_call_log_read_by_index(small_file_call_log_t *call_log, int index);
int ui_small_file_call_log_get_count(void);


#endif /*_WATCH_DATA_CALL_LOG_STORAGE_*/

