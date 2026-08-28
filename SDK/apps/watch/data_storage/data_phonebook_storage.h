#ifndef _WATCH_DATA_PHONEBOOK_STORAGE_
#define _WATCH_DATA_PHONEBOOK_STORAGE_


/*********************
 *      DEFINES
 *********************/
#define PHONEBOOK_NAME_LEN                          (20)
#define PHONEBOOK_NUMBER_LEN                        (20)
#define SMALL_FILE_PHONEBOOK_SIZE                   (sizeof(small_file_phonebook_t))

/**********************
 *      TYPEDEFS
 **********************/
typedef struct small_file_phonebook {
    char name[PHONEBOOK_NAME_LEN];
    char number[PHONEBOOK_NUMBER_LEN];
} small_file_phonebook_t;

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 获取存储的联系人数量
 */
/* ------------------------------------------------------------------------------------*/
int ui_small_file_phonebook_get_count(void);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过index获取对应的联系人信息
 *
 * @Params phonebook 存储读取到的联系人信息
 * @Params index 序号
 *
 * @Return true or false
 */
/* ------------------------------------------------------------------------------------*/
int ui_small_file_phonebook_read_by_index(small_file_phonebook_t *phonebook, int index);

int small_file_phonebook_get_name_by_number(char *name, char *number);





#endif /*_WATCH_DATA_PHONEBOOK_STORAGE_*/

