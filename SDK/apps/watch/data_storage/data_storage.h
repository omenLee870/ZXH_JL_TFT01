#ifndef _WATCH_DATA_STORAGE_H
#define _WATCH_DATA_STORAGE_H

/*********************
 *      INCLUDES
 *********************/
#include "data_phonebook_storage.h"
#include "data_call_log_storage.h"
#include "data_message_storage.h"
#include "data_weather_storage.h"

/*********************
 *      DEFINES
 *********************/

/* 小文件类型 */
#define F_TYPE_BASE                 (1)

#define F_TYPE_PHONEBOOK            (F_TYPE_BASE)
#define F_TYPE_SPORTRECORD          (F_TYPE_BASE+1)
#define F_TYPE_HEART                (F_TYPE_BASE+2)
#define F_TYPE_BLOOD_OXYGEN         (F_TYPE_BASE+3)
#define F_TYPE_SLEEP                (F_TYPE_BASE+4)
#define F_TYPE_MESSAGE              (F_TYPE_BASE+5)
#define F_TYPE_WEATHER              (F_TYPE_BASE+6)
#define F_TYPE_CALL_LOG				(F_TYPE_BASE+7)
#define F_TYPE_STEP					(F_TYPE_BASE+8)
#define F_TYPE_HEART_SINGLE			(F_TYPE_BASE+9)
#define F_TYPE_BLOOD_OXYGEN_SINGLE  (F_TYPE_BASE+10)

#define F_TYPE_CHAT_REC				(F_TYPE_BASE+11)

#define F_TYPE_MAX                  (F_TYPE_CHAT_REC)
/* F_TYPE_COUNT 同时用作 hd->child[] 数组大小与 ASSERT 上界；
   数组按原始 type 值(1..F_TYPE_MAX)直接下标访问，故需 F_TYPE_MAX+1 个槽位 */
#define F_TYPE_COUNT                (F_TYPE_MAX + 1)

#define ID_TABLE_ITEM_SIZE  4
/*ID_TABLE 格式:每一项4个字节 低位两个字节file_id  高位两个字节file_size*/


/**********************
 *      TYPEDEFS
 **********************/


struct small_file {
    u8 type;
    u8 max_item;
    u8 storage_sel;
};


int data_small_file_init(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过id,读取对应小文件内容
 *
 * @Params small_file_type 小文件类型
 * @Params id 文件id
 * @Params buf 存储要读取文件数据的buffer
 * @Params len 要读取的数据长度
 *
 * @Return 读取到的字节数
 */
/* ------------------------------------------------------------------------------------*/
int small_file_read(u8 small_file_type, u32 id, u32 offset, void *buf, u32 len);


/* ------------------------------------------------------------------------------------*/
/**
 * @brief 写入小文件数据
 *
 * @Params small_file_type 小文件类型
 * @Params buf_offset 要写入数据的偏移
 * @Params buf 存储写入文件数据的buffer
 * @Params len 要写入数据的长度
 *
 * @Return 写入成功 返回u16位大小的id; 写入失败 返回id为0
 */
/* ------------------------------------------------------------------------------------*/
u32  small_file_write(u8 small_file_type, u32 *id, u32 buf_offset, void *buf, u32 len, u32 total_len);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过id,更新对应小文件内容(仅原来内容范围内)
 *
 * @Params small_file_type 小文件类型
 * @Params id 文件id
 * @Params buf_offset 要写入数据的偏移
 * @Params buf 存储写入文件数据的buffer
 * @Params len 要写入数据的长度
 *
 * @Return 实际写入长度
 */
/* ------------------------------------------------------------------------------------*/
int small_file_update_by_id(u8 small_file_type, u32 id, u32 buf_offset, void *buf, u32 len, u32 total_len);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过id,删除小文件
 *
 * @Params file_type 小文件类型
 * @Params id 文件id
 *
 * @Return false or true
 */
/* ------------------------------------------------------------------------------------*/
int small_file_delete_by_id(u8 small_file_type, u32 id);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 删除所有小文件
 *
 * @Return false or true
 */
/* ------------------------------------------------------------------------------------*/
int small_file_del_all(void);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 获取小文件的id_table的大小
 *
 * @Params small_file_type 小文件类型
 *
 * @Return length of the id_table
 */
/* ------------------------------------------------------------------------------------*/
int small_file_get_id_table_len(u8 small_file_type);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 获取小文件的id_table
 *
 * @Params small_file_type
 * @Params table_data Pointer to the buffer where the id_table data will be stored.
 * @Params data_len The length of the table_data buffer.
 *
 * @Return false or true
 */
/* ------------------------------------------------------------------------------------*/
int small_file_get_id_table(u8 small_file_type, u8 *table_data, u16 data_len);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 获取小文件的数量
 *
 * @Params small_file_type 小文件类型
 *
 */
/* ------------------------------------------------------------------------------------*/
int small_file_get_count(u8 small_file_type);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过index获取小文件的id
 *
 * @Params small_file_type 小文件类型
 * @Params index 文件存储顺序序号
 *
 * @Return 文件id
 */
/* ------------------------------------------------------------------------------------*/
u32 small_file_get_id_by_index(u8 small_file_type, int index);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过index获取小文件的大小
 *
 * @Params small_file_type 小文件类型
 * @Params index 文件存储顺序序号
 *
 * @Return 文件大小
 */
/* ------------------------------------------------------------------------------------*/
u32 small_file_get_size_by_index(u8 small_file_type, int index);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 通过id获取小文件的大小
 *
 * @Params small_file_type 小文件类型
 * @Params index 文件存储顺序序号
 *
 * @Return 文件大小
 */
/* ------------------------------------------------------------------------------------*/
u32 small_file_get_size_by_id(u8 small_file_type, int id);

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 删除某一小文件类型下的文件
 *
 * @Params small_file_type 小文件类型
 *
 * @Return false or true
 */
/* ------------------------------------------------------------------------------------*/
int small_file_del_by_file_type(u8 small_file_type);

#endif /*_WATCH_DATA_STORAGE_H*/

