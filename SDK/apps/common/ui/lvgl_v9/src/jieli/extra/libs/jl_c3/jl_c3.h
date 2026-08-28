#ifndef JL_C3_H_
#define JL_C3_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
/*#include "../bin/lv_bin.h"*/


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct c3_head {
    uint32_t width_diff: 4;
    uint32_t height_diff: 4;
    uint32_t has_rgb_qtab: 1;
    uint32_t has_alpha_qtab: 1;
    uint32_t rgb_quality : 7;
    uint32_t alpha_quality : 7;
    uint32_t reserved: 8;
    uint32_t reorder_len;
} c3_head_t;

typedef struct {
    unsigned int img_width : 12;
    unsigned int img_height : 12;
    unsigned int img_format : 4;
    unsigned int yuv_mode : 1;
    unsigned int block_mode : 1;
    unsigned int reserved : 2;

    unsigned int bs_base_addr : 32;
} c3_info_t;



/**********************
 * GLOBAL PROTOTYPES
 **********************/

int lv_c3_init(uint8_t cache_num, uint8_t load_table_mode);
int lv_c3_set_global_qtable(char *src);
int lv_c3_load_qtable_type_uitool(uint8_t *src_buf, int data_size);
int lv_c3_load_qtable_type_app(uint8_t *src_buf, int data_size);
int lv_c3_load_global_qtable(void);
int lv_c3_uninit(void);
int lv_c3_hw_check_and_reinit(void);

/**********************
 *      MACROS
 **********************/

#endif

#ifdef __cplusplus
}
#endif




