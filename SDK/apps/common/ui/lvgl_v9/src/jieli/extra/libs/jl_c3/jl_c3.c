/**
 * @file jl_c3.c
 *
 */


/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#if LV_USE_GPU_COMPRESS
#define BOOL_DEFINE_CONFLICT
#include "lv_port_fs.h"
#include "lvgl.h"
#include "fs.h"
#include "jlgpu_driver.h"
#include "jl_c3.h"

/*********************
 *      DEFINES
 *********************/
#define C3_IRQ_ENABLE 0

#define JL_JPG        ((EVA_JPG_typedef       *)(eva_map_adr(CID_JPG,0x00)))
#define JL_GXDEC       ((EVA_GXDEC_typedef     *)(eva_map_adr(CID_GXDEC,0x00)))
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int lv_c3_hw_open(void);
static int lv_c3_hw_close(void);
static int lv_c3_hw_set_qtable(s16 *qtab, s16 *a_tab, u16 *huff);
static void lv_c3_hw_state_save(void);
static int lv_c3_hw_state_check(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static uint8_t *gxdec_a_addr = NULL;
static uint8_t *gxdec_yuv_addr = NULL;
static uint8_t *g_yuv_qtable = NULL;
static uint8_t *g_alpha_qtable = NULL;
static uint8_t lv_c3_load_table_mode = 0;
static uint8_t lv_c3_load_table_init_flag = 0;
static uint8_t c3_rgb_quality = 0;
static uint8_t c3_alpha_quality = 0;

/* Save expected register state after hw init, used to detect
   if other modules have changed the JPG/GXDEC registers. */
static struct {
    uint32_t jpg_com_con;
    uint32_t jpg_con0;
    uint32_t jpg_yuv_con;
    uint32_t jpg_stride;
    uint32_t jpg_buf_sadr0;
    uint32_t jpg_buf_eadr0;
    uint32_t gxdec_a_base;
    uint32_t gxdec_yuv_base;
    uint32_t gxdec_cache_cnt;

    uint8_t  cache_num;
    uint8_t  load_table_mode;
    uint8_t  saved; /* 1 = state has been saved, 0 = not yet */
} lv_c3_hw_expected_state;


static const uint8_t g_zigzag[64] = {
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63,
};

static const uint16_t g_scalefactor[64] = {
    8192,   11363,  10703,  9633,   8192,   6436,   4433,   2260,
    11363,  15760,  14846,  13361,  11363,  8928,   6149,   3135,
    10703,  14846,  13985,  12586,  10703,  8410,   5793,   2953,
    9633,   13361,  12586,  11327,  9633,   7568,   5213,   2658,
    8192,   11363,  10703,  9633,   8192,   6436,   4433,   2260,
    6436,   8928,   8410,   7568,   6436,   5057,   3483,   1776,
    4433,   6149,   5793,   5213,   4433,   3483,   2399,   1223,
    2260,   3135,   2953,   2658,   2260,   1776,   1223,   624,
};


static uint8_t STD_HUFFMAN_TBL[548] __attribute__((aligned(32))) = {
    0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x0E, 0x00, 0x1E, 0x00, 0x3E, 0x00, 0x7E, 0x00, 0xFE, 0x00,
    0xFE, 0x01, 0xFE, 0x03, 0xFC, 0x07, 0xF8, 0x0F, 0xF0, 0x1F, 0xE0, 0x3F, 0xC0, 0x7F, 0x80, 0xFF,
    0x00, 0x00, 0x01, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x1A, 0x00, 0x3A, 0x00, 0x78, 0x00, 0xF8, 0x00,
    0xF6, 0x01, 0xF6, 0x03, 0xF6, 0x07, 0xF4, 0x0F, 0xF0, 0x1F, 0xE0, 0x3F, 0xC0, 0x7F, 0x82, 0xFF,
    0x00, 0x00, 0x02, 0x03, 0x06, 0x09, 0x0B, 0x0F, 0x12, 0x17, 0x1C, 0x20, 0x24, 0x24, 0x24, 0x25,
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0E, 0x00, 0x1E, 0x00, 0x3E, 0x00, 0x7E, 0x00,
    0xFE, 0x00, 0xFE, 0x01, 0xFE, 0x03, 0xFE, 0x07, 0xFE, 0x0F, 0xFC, 0x1F, 0xF8, 0x3F, 0xF0, 0x7F,
    0xE0, 0xFF, 0x00, 0x00, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0A, 0x00, 0x18, 0x00, 0x38, 0x00, 0x78, 0x00,
    0xF6, 0x00, 0xF4, 0x01, 0xF6, 0x03, 0xF6, 0x07, 0xF4, 0x0F, 0xF0, 0x1F, 0xE0, 0x3F, 0xC2, 0x7F,
    0x88, 0xFF, 0x00, 0x00, 0x02, 0x03, 0x05, 0x09, 0x0D, 0x10, 0x14, 0x1B, 0x20, 0x24, 0x28, 0x28,
    0x29, 0x2B, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07,
    0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33,
    0x52, 0xF0, 0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19,
    0x1A, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46,
    0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85,
    0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
    0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA,
    0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8,
    0xD9, 0xDA, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
    0xF7, 0xF8, 0xF9, 0xFA
};

static u16 QT_TBL[0x80] __attribute__((aligned(32))) = {
    0x0180, 0x0163, 0x014e, 0x01c3, 0x0280, 0x0324, 0x02b4, 0x01a7, 0x0163, 0x01ec, 0x02b7, 0x0343, 0x0377,
    0x0689, 0x0481, 0x021a, 0x01f5, 0x02b7, 0x028f, 0x03d7, 0x0539, 0x05a5, 0x04f3, 0x01fb, 0x01c3, 0x0272,
    0x0312, 0x0425, 0x05e1, 0x07da, 0x0517, 0x01f2, 0x0200, 0x02c6, 0x0492, 0x0677, 0x0700, 0x08a4, 0x05ae,
    0x0211, 0x01f6, 0x03d0, 0x05a5, 0x0601, 0x0649, 0x067b, 0x04e3, 0x01f3, 0x02b4, 0x04e1, 0x05a8, 0x0568,
    0x05ae, 0x051a, 0x0383, 0x017e, 0x01ee, 0x0371, 0x036c, 0x033e, 0x0308, 0x022a, 0x0191, 0x00c2, 0x0180,
    0x02c6, 0x0344, 0x054a, 0x0a00, 0x07db, 0x0569, 0x02c2, 0x02c6, 0x03d9, 0x0487, 0x0a99, 0x0dde, 0x0ae5,
    0x0781, 0x03d3, 0x0344, 0x0487, 0x0963, 0x0f5d, 0x0d10, 0x0a43, 0x0712, 0x039a, 0x054a, 0x0a99, 0x0f5d,
    0x0dd3, 0x0bc2, 0x093d, 0x065d, 0x033e, 0x0a00, 0x0dde, 0x0d10, 0x0bc2, 0x0a00, 0x07db, 0x0569, 0x02c2,
    0x07db, 0x0ae5, 0x0a43, 0x093d, 0x07db, 0x062c, 0x0440, 0x022a, 0x0569, 0x0781, 0x0712, 0x065d, 0x0569,
    0x0440, 0x02ed, 0x017e, 0x02c2, 0x03d3, 0x039a, 0x033e, 0x02c2, 0x022a, 0x017e, 0x00c2
};

static u16 QT_TBL_A[0x80] __attribute__((aligned(32))) = {
    0x0180, 0x0163, 0x014e, 0x01c3, 0x0280, 0x0324, 0x02b4, 0x01a7, 0x0163, 0x01ec, 0x02b7, 0x0343, 0x0377,
    0x0689, 0x0481, 0x021a, 0x01f5, 0x02b7, 0x028f, 0x03d7, 0x0539, 0x05a5, 0x04f3, 0x01fb, 0x01c3, 0x0272,
    0x0312, 0x0425, 0x05e1, 0x07da, 0x0517, 0x01f2, 0x0200, 0x02c6, 0x0492, 0x0677, 0x0700, 0x08a4, 0x05ae,
    0x0211, 0x01f6, 0x03d0, 0x05a5, 0x0601, 0x0649, 0x067b, 0x04e3, 0x01f3, 0x02b4, 0x04e1, 0x05a8, 0x0568,
    0x05ae, 0x051a, 0x0383, 0x017e, 0x01ee, 0x0371, 0x036c, 0x033e, 0x0308, 0x022a, 0x0191, 0x00c2, 0x0180,
    0x02c6, 0x0344, 0x054a, 0x0a00, 0x07db, 0x0569, 0x02c2, 0x02c6, 0x03d9, 0x0487, 0x0a99, 0x0dde, 0x0ae5,
    0x0781, 0x03d3, 0x0344, 0x0487, 0x0963, 0x0f5d, 0x0d10, 0x0a43, 0x0712, 0x039a, 0x054a, 0x0a99, 0x0f5d,
    0x0dd3, 0x0bc2, 0x093d, 0x065d, 0x033e, 0x0a00, 0x0dde, 0x0d10, 0x0bc2, 0x0a00, 0x07db, 0x0569, 0x02c2,
    0x07db, 0x0ae5, 0x0a43, 0x093d, 0x07db, 0x062c, 0x0440, 0x022a, 0x0569, 0x0781, 0x0712, 0x065d, 0x0569,
    0x0440, 0x02ed, 0x017e, 0x02c2, 0x03d3, 0x039a, 0x033e, 0x02c2, 0x022a, 0x017e, 0x00c2
};

#define QTAB_ADDR ((s16 *)(QT_TBL))
#define QTAB_A_ADDR ((s16 *)(QT_TBL_A))
#define HTAB_DC0_ADDR ((u16 *)((u8 *)STD_HUFFMAN_TBL))                            //  0x30

static uint16_t huffman_tab_len = sizeof(STD_HUFFMAN_TBL); // 0

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
struct jpeg_run_state_t {
    u32 mcu_cnt;
};
static struct jpeg_run_state_t __jpg_state = {0};

#if C3_IRQ_ENABLE
___interrupt
static void __jpg_isr(void)
{
    if (JL_JPG->PND_CON & BIT(0)) {
        putchar('0');
    }
    if (JL_JPG->PND_CON & BIT(1)) {
        putchar('1');
    }
    if (JL_JPG->PND_CON & BIT(2)) {
        putchar('2');
    }
    if (JL_JPG->PND_CON & BIT(3)) {
        putchar('3');
    }
    if (JL_JPG->PND_CON & BIT(4)) {
        putchar('4');
    }
    if (JL_JPG->PND_CON & BIT(5)) {
        putchar('5');
    }
    if (JL_JPG->PND_CON & BIT(6)) {
        putchar('6');
    }
    JL_JPG->PND_CLR = 0xff;
}
#endif

int lv_c3_init(uint8_t cache_num, uint8_t load_table_mode)
{
    u8 cache_nums_array[] = {2, 4, 8, 16};
    u8 i = 0;
    lv_c3_load_table_mode = load_table_mode;

    if (gxdec_a_addr == NULL && gxdec_yuv_addr == NULL) {
        // gxdec_init
        gxdec_a_addr = malloc(32 * 32 * cache_num);
        gxdec_yuv_addr = malloc(32 * 32 * 4 * cache_num);

        EVA_GXDEC->A_BASE = (uint32_t)gxdec_a_addr;
        EVA_GXDEC->YUV_BASE = (uint32_t)gxdec_yuv_addr;

        for (i = 0; i < 4; i++) {
            if (cache_nums_array[i] == cache_num) {
                break;
            }
        }
        EVA_GXDEC->CACHE_CNT = i;

        /*printf("\ngxdec init a_base:%x yuv_base:%x cache_num=%d cache_cnt=%d\n",*/
        /*gxdec_a_addr,*/
        /*gxdec_yuv_addr,*/
        /*cache_num,*/
        /*i);*/
        /*return 0;*/

        // jpg_hw_open
        lv_c3_hw_open();
        lv_c3_hw_expected_state.cache_num = cache_num;
        lv_c3_hw_expected_state.load_table_mode = load_table_mode;
        lv_c3_hw_state_save();
    }
    return 0;
}




/**
 * @brief 将JPEG标准量化表转换为硬件/算法所需的定点数格式
 *
 * @param out_qtab   [输出] 转换后的量化表缓冲区 (至少64个short)
 * @param in_qtab    [输入] 原始JPEG 8bit量化表 (Zigzag顺序, 64字节)
 * @return int       0: 成功; -EINVAL: 参数无效
 *
 * @note 该接口为纯计算函数，不依赖任何全局变量或硬件寄存器地址
 */
static int jpeg_convert_qtable(short *out_qtab, const u8 *in_qtab)
{
    int i;
    int j;

    if (!out_qtab || !in_qtab) {
        return -EINVAL;
    }

    for (i = 0; i < 64; i++) {
        j = g_zigzag[i];
        /*
         * 定点数运算: (scalefactor * q_value) >> 6
         * 等价于原代码中的 >> (13 - 7)，即右移6位(除以64)
         * 注意: g_scalefactor 需确保在链接时可用，或改为通过参数传入
         */
        out_qtab[j] = (g_scalefactor[j] * in_qtab[i]) >> 6;
    }

    return 0;
}




int lv_c3_set_global_qtable(char *src)
{
    lv_img_dsc_t src_dsc = {0};
    c3_info_t *c3_info;
    if (src == NULL) {
        return LV_RES_INV;
    }
    if (lv_get_img_dsc_bin(src, &src_dsc) != LV_RES_OK) {
        LV_LOG("can not set global qtable from %s!\n", src);
        return LV_RES_INV;
    }
    int recoder_len = 0;
    c3_info = (c3_info_t *)src_dsc.data;
    //JL header + lv_img_dsc_t + recoder + huffman_tab + a_tab + qtab
    if (c3_info->img_format >= 3) {
        //has alpha
        recoder_len = src_dsc.data_size - (huffman_tab_len + 256 + 256);
        g_alpha_qtable = (uint8_t *)(src_dsc.data + recoder_len + huffman_tab_len);
        g_yuv_qtable = (uint8_t *)(src_dsc.data + recoder_len + huffman_tab_len + 256);
    } else  {
        recoder_len = src_dsc.data_size - (huffman_tab_len + 256);
        g_yuv_qtable = (uint8_t *)(src_dsc.data + recoder_len + huffman_tab_len);
        g_alpha_qtable = g_yuv_qtable;
    }

    return LV_RES_OK;
}


int lv_c3_load_global_qtable(void)
{
    /* Check if other modules have changed the hardware registers.
     * If so, re-initialize before loading tables. */
    lv_c3_hw_check_and_reinit();

    if (lv_c3_load_table_init_flag && (lv_c3_load_table_mode == 0)) {
        return 0;
    }

    if (g_alpha_qtable == NULL || g_yuv_qtable == NULL) {
        LV_LOG("global qtable is null\n");
        return -1;
    }
    lv_c3_hw_set_qtable((s16 *)g_yuv_qtable, (s16 *)g_alpha_qtable, (u16 *)HTAB_DC0_ADDR);
    return 0;
}

int lv_c3_load_qtable_type_uitool(uint8_t *src_buf, int data_size)
{
    c3_head_t *c3_head = NULL;
    c3_info_t *c3_info = NULL;
    if (gxdec_a_addr == NULL || gxdec_yuv_addr == NULL) {
        LV_LOG_ERROR("gxdec_a_addr or gxdec_yuv_addr is NULL\n");
        return -1;
    }

    c3_head = (c3_head_t *)src_buf;
    c3_info = (c3_info_t *)(src_buf + sizeof(c3_head_t));

    int recoder_len = c3_head->reorder_len;
    int rgb_quality_diff = ((c3_head->has_rgb_qtab) && (c3_head->rgb_quality != c3_rgb_quality)) ? 1 : 0;
    int alpha_quality_diff = ((c3_head->has_alpha_qtab) && (c3_head->alpha_quality != c3_alpha_quality)) ? 1 : 0;

    if (rgb_quality_diff || alpha_quality_diff) {
        lv_c3_load_table_init_flag = 0; /* Reset so tables get re-loaded on next call */
    }

    /* Check if other modules have changed the hardware registers.
     * If so, re-initialize before loading tables. */
    if (lv_c3_hw_check_and_reinit()) {
        alpha_quality_diff = 1;
        rgb_quality_diff   = 1;
    }

    if (lv_c3_load_table_init_flag && (lv_c3_load_table_mode == 0)) {
        return 0;
    }

    u16 *Huff = NULL;
    s16 *Qtab_yuv = NULL;
    s16 *Qtab_a = NULL;
    s16 *Qtab_a_tmp = (s16 *)(src_buf + sizeof(c3_head_t) + c3_head->reorder_len + c3_head->has_rgb_qtab * 128);
    s16 *Qtab_yuv_tmp = (s16 *)(src_buf + sizeof(c3_head_t) + c3_head->reorder_len);

    Qtab_yuv = (s16 *)QTAB_ADDR;
    Qtab_a = (s16 *)QTAB_A_ADDR;

    if (c3_head->has_rgb_qtab && rgb_quality_diff) {
        jpeg_convert_qtable(QTAB_ADDR, (u8 *)Qtab_yuv_tmp);
        jpeg_convert_qtable((short *)((u8 *)QTAB_ADDR + 128), ((u8 *)Qtab_yuv_tmp));
        c3_rgb_quality = c3_head->rgb_quality;
    }

    if (c3_head->has_alpha_qtab && alpha_quality_diff) {
        jpeg_convert_qtable(QTAB_A_ADDR, (u8 *)Qtab_a_tmp);
        c3_alpha_quality = c3_head->alpha_quality;
    }

    Huff = (u16 *)HTAB_DC0_ADDR;//标准huff

    if (alpha_quality_diff || rgb_quality_diff) {
        lv_c3_hw_set_qtable(QTAB_ADDR, QTAB_A_ADDR, Huff);
    }

    return 0;
}

int lv_c3_load_qtable_type_app(uint8_t *src_buf, int data_size)
{
    c3_head_t *c3_head = NULL;
    c3_info_t *c3_info = NULL;

    if (gxdec_a_addr == NULL || gxdec_yuv_addr == NULL) {
        LV_LOG_ERROR("gxdec_a_addr or gxdec_yuv_addr is NULL\n");
        return -1;
    }

    c3_head = (c3_head_t *)src_buf;
    c3_info = (c3_info_t *)(src_buf + sizeof(c3_head_t));

    int recoder_len = c3_head->reorder_len;
    int rgb_quality_diff = ((c3_head->has_rgb_qtab) && (c3_head->rgb_quality != c3_rgb_quality)) ? 1 : 0;
    int alpha_quality_diff = ((c3_head->has_alpha_qtab) && (c3_head->alpha_quality != c3_alpha_quality)) ? 1 : 0;

    if (rgb_quality_diff || alpha_quality_diff) {
        lv_c3_load_table_init_flag = 0; /* Reset so tables get re-loaded on next call */
    }

    /* Check if other modules have changed the hardware registers.
     * If so, re-initialize before loading tables. */
    if (lv_c3_hw_check_and_reinit()) {
        alpha_quality_diff = 1;
        rgb_quality_diff   = 1;
    }

    if (lv_c3_load_table_init_flag && (lv_c3_load_table_mode == 0)) {
        return 0;
    }

    u16 *Huff = NULL;
    s16 *Qtab_yuv = NULL;
    s16 *Qtab_a = NULL;
    s16 *Qtab_a_tmp = (s16 *)(src_buf + sizeof(c3_head_t) + c3_head->reorder_len + c3_head->has_rgb_qtab * 128);
    s16 *Qtab_yuv_tmp = (s16 *)(src_buf + sizeof(c3_head_t) + c3_head->reorder_len);

    if (c3_head->has_rgb_qtab && rgb_quality_diff) {
        jpeg_convert_qtable(QTAB_ADDR, (u8 *)Qtab_yuv_tmp);
        jpeg_convert_qtable((short *)((u8 *)QTAB_ADDR + 128), ((u8 *)Qtab_yuv_tmp));
        c3_rgb_quality = c3_head->rgb_quality;
    }

    if (c3_head->has_alpha_qtab && alpha_quality_diff) {
        jpeg_convert_qtable(QTAB_A_ADDR, (u8 *)Qtab_a_tmp);
        c3_alpha_quality = c3_head->alpha_quality;
    }

    Huff = (u16 *)HTAB_DC0_ADDR;//标准huff

    if (alpha_quality_diff || rgb_quality_diff) {
        lv_c3_hw_set_qtable(QTAB_ADDR, QTAB_A_ADDR, Huff);
    }

    return 0;
}

int lv_c3_uninit(void)
{
    if (gxdec_yuv_addr) {
        free(gxdec_yuv_addr);
        gxdec_yuv_addr = NULL;
    }
    if (gxdec_a_addr) {
        free(gxdec_a_addr);
        gxdec_a_addr = NULL;
    }
    lv_c3_hw_close();

    return 0;
}


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void lv_c3_hw_state_save(void)
{
    lv_c3_hw_expected_state.jpg_com_con     = EVA_JPG0->COM_CON;
    lv_c3_hw_expected_state.jpg_con0        = EVA_JPG0->CON0;
    lv_c3_hw_expected_state.jpg_yuv_con     = EVA_JPG0->YUV_CON;
    lv_c3_hw_expected_state.jpg_stride      = EVA_JPG0->STRIDE;
    lv_c3_hw_expected_state.jpg_buf_sadr0   = EVA_JPG0->BUF_SADR0;
    lv_c3_hw_expected_state.jpg_buf_eadr0   = EVA_JPG0->BUF_EADR0;
    lv_c3_hw_expected_state.gxdec_a_base    = EVA_GXDEC->A_BASE;
    lv_c3_hw_expected_state.gxdec_yuv_base  = EVA_GXDEC->YUV_BASE;
    lv_c3_hw_expected_state.gxdec_cache_cnt = EVA_GXDEC->CACHE_CNT;
    lv_c3_hw_expected_state.saved           = 1;
}

/**
 * Compare current JPG/GXDEC register values against the saved state.
 * @return  0: registers unchanged (no re-init needed)
 *          1: registers changed by other modules (need re-init)
 *         -1: state not saved yet (no baseline to compare)
 */
static int lv_c3_hw_state_check(void)
{
    if (!lv_c3_hw_expected_state.saved) {
        return -1;
    }

    if (EVA_JPG0->COM_CON     != lv_c3_hw_expected_state.jpg_com_con   ||
        (EVA_JPG0->CON0 & (0x07))        != (lv_c3_hw_expected_state.jpg_con0 & (0x07))     ||
        EVA_JPG0->YUV_CON     != lv_c3_hw_expected_state.jpg_yuv_con   ||
        EVA_JPG0->STRIDE      != lv_c3_hw_expected_state.jpg_stride    ||
        EVA_JPG0->BUF_SADR0   != lv_c3_hw_expected_state.jpg_buf_sadr0 ||
        EVA_JPG0->BUF_EADR0   != lv_c3_hw_expected_state.jpg_buf_eadr0 ||
        EVA_GXDEC->A_BASE     != lv_c3_hw_expected_state.gxdec_a_base  ||
        EVA_GXDEC->YUV_BASE   != lv_c3_hw_expected_state.gxdec_yuv_base ||
        EVA_GXDEC->CACHE_CNT  != lv_c3_hw_expected_state.gxdec_cache_cnt) {
        LV_LOG_WARN("jl_c3: registers changed by other module, re-init needed");
        // printf("EVA_JPG0->COM_CON: 0x%02x 0x%02x\n", EVA_JPG0->COM_CON, lv_c3_hw_expected_state.jpg_com_con);
        // printf("EVA_JPG0->CON0: 0x%02x 0x%02x\n", EVA_JPG0->CON0, lv_c3_hw_expected_state.jpg_con0);
        // printf("EVA_JPG0->YUV_CON: 0x%02x 0x%02x\n", EVA_JPG0->YUV_CON, lv_c3_hw_expected_state.jpg_yuv_con);
        // printf("EVA_JPG0->STRIDE: 0x%02x 0x%02x\n", EVA_JPG0->STRIDE, lv_c3_hw_expected_state.jpg_stride);
        // printf("EVA_JPG0->BUF_SADR0: 0x%08x 0x%08x\n", EVA_JPG0->BUF_SADR0, lv_c3_hw_expected_state.jpg_buf_sadr0);
        // printf("EVA_JPG0->BUF_EADR0: 0x%08x 0x%08x\n", EVA_JPG0->BUF_EADR0, lv_c3_hw_expected_state.jpg_buf_eadr0);
        // printf("EVA_GXDEC->A_BASE: 0x%08x 0x%08x\n", EVA_GXDEC->A_BASE, lv_c3_hw_expected_state.gxdec_a_base);
        // printf("EVA_GXDEC->YUV_BASE: 0x%08x 0x%08x\n", EVA_GXDEC->YUV_BASE, lv_c3_hw_expected_state.gxdec_yuv_base);
        // printf("EVA_GXDEC->CACHE_CNT: 0x%02x 0x%02x\n", EVA_GXDEC->CACHE_CNT, lv_c3_hw_expected_state.gxdec_cache_cnt);
        return 1;
    }

    return 0;
}

/**
 * Check if JPG/GXDEC hardware registers have been changed by other modules,
 * and re-initialize if needed.
 * @return  0: no re-init needed
 *          1: re-init performed
 */
int lv_c3_hw_check_and_reinit(void)
{
    if (gxdec_a_addr == NULL || gxdec_yuv_addr == NULL) {
        return 0; /* Not initialized by lv_c3_init yet, skip */
    }

    int ret = lv_c3_hw_state_check();
    if (ret == 0) {
        return 0; /* Registers unchanged */
    }

    /* Registers changed (or state not saved) — re-initialize hardware */
    lv_c3_uninit();
    lv_c3_init(lv_c3_hw_expected_state.cache_num, lv_c3_hw_expected_state.load_table_mode);
    lv_c3_load_table_init_flag = 0; /* Reset so tables get re-loaded on next call */

    /* In global table mode, re-load the global qtable immediately */
    if (lv_c3_load_table_mode == 0 && g_yuv_qtable != NULL && g_alpha_qtable != NULL) {
        lv_c3_hw_set_qtable((s16 *)g_yuv_qtable, (s16 *)g_alpha_qtable, (u16 *)HTAB_DC0_ADDR);
    }

    LV_LOG_WARN("jl_c3: hardware re-initialized");
    return 1;
}

static int lv_c3_hw_set_qtable(s16 *qtab, s16 *a_tab, u16 *huff)
{

    if (!lv_c3_load_table_init_flag) {
        lv_c3_load_table_init_flag = 1;
    }

    int i = 0;

    EVA_JPG0->CFGRAMADDR = 0x0; // huffman table start addr
    __asm_csync();
    for (i = 0; i < 274; i++) {
        EVA_JPG0->CFGRAMVAL = huff[i];
    }

    EVA_JPG0->CFGRAMADDR = 0x200; // quant talphaable start addr
    __asm_csync();
    for (i = 0; i < 128; i++) {
        EVA_JPG0->CFGRAMVAL = qtab[i]; // YUV 数据的量化表
    }
    /* Qtab_a = Qtab_yuv; */
    /*puts("Qtab:\n"); */
    /*put_buf((u8 *)qtab, 256); */
    for (i = 0; i < 64; i++) {
        EVA_JPG0->CFGRAMVAL = a_tab[i]; // a数据的量化表
    }
    /*puts("Qtab_a:\n"); */
    /*put_buf((u8 *)a_tab, 256); */


    return 0;
}


static int jpg_clk_init(void)
{
    // clk initial
    /* SFR(JL_VDOCOM->JPG_CON, 0, 4, 3);  // clk sel */
    /* SFR(JL_VDOCOM->JPG_CON, 4, 4, 0);  // div */
    /* SFR(JL_VDOCOM->JPG_CON, 8, 1, 0);  // clk phase */
    /* SFR(JL_VDOCOM->JPG_CON, 9, 1, 1);  // clk en */
    /* SFR(JL_VDOCOM->JPG_CON, 10, 1, 1); // rst_ */

    /* SFR(JL_VDOCOM->EVA_CON, 0, 4, 1); */
    /* SFR(JL_VDOCOM->EVA_CON, 9, 1, 1); */
    /* SFR(JL_VDOCOM->EVA_CON, 10, 1, 1); */

    SFR(EVA_JPG0->COM_CON, 0, 1, 1); // clk en
    SFR(EVA_JPG0->COM_CON, 1, 1, 0); // rst_
    asm("nop");
    SFR(EVA_JPG0->COM_CON, 1, 1, 1); // rst_

    asm("nop");
    asm("nop");
    asm("nop");

    return 0;
}

static int lv_c3_hw_open(void)
{
    jpg_clk_init();

    // jpg initial
    EVA_JPG0->CON0 = 0;
    EVA_JPG0->CON1 = 0;
    SFR(EVA_JPG0->CON0, 1, 1, 1); // codec en
    memset(&__jpg_state, 0, sizeof(struct jpeg_run_state_t));

#if C3_IRQ_ENABLE
    request_irq(IRQ_JPG_IDX, 6, __jpg_isr, 0);
    EVA_JPG0->PND_IE = 0xff;
#else
    EVA_JPG0->PND_IE = 0;
#endif

    int i = 0;

    u16 *Huff;
    s16 *Qtab_yuv;
    s16 *Qtab_a;

    //huffman表配置
    Huff = (uint16_t *)STD_HUFFMAN_TBL;
    EVA_JPG0->CFGRAMADDR = 0x0; // huffman table start addr
    __asm_csync();
    for (i = 0; i < 274; i++) {
        /* for (i = 0; i < 258; i++) { */
        EVA_JPG0->CFGRAMVAL = Huff[i];
    }
    /* puts("Huff:\n"); */
    /* put_buf((u8 *)Huff,274*2); */

#if 0
    Qtab_yuv = QTAB_ADDR;
    EVA_JPG0->CFGRAMADDR = 0x200; // quant talphaable start addr
    __asm_csync();
    for (i = 0; i < 128; i++) {
        EVA_JPG0->CFGRAMVAL = Qtab_yuv[i]; // YUV 数据的量化表
    }
    /* Qtab_a = Qtab_yuv; */
    /* puts("Qtab:\n"); */
    /* put_buf((u8 *)Qtab_yuv, 256); */
    Qtab_a = QTAB_A_ADDR;
    for (i = 0; i < 64; i++) {
        EVA_JPG0->CFGRAMVAL = Qtab_a[i]; // a数据的量化表
    }
    /* puts("Qtab_a:\n"); */
    /* put_buf((u8 *)Qtab_a, 256); */
#endif

    SFR(EVA_JPG0->STRIDE, 0, 16, 32); // 联动模式固定stride:32

    // 联动模式下只是为了框住GXDEC地址范围，用处不大，这里先设置个最大范围
    EVA_JPG0->BUF_SADR0 = 0;          // 输出buf的起始地址
    EVA_JPG0->BUF_EADR0 = 0xffffffff; // 输出buf的结束地址

    /* SFR(EVA_JPG0->DMA_CON, 0, 1, 1); // fifo init */
    SFR(EVA_JPG0->YUV_CON, 4, 1, 0); // order mode
    SFR(EVA_JPG0->YUV_CON, 3, 1, 0); // 0:tile 1:line
    SFR(EVA_JPG0->YUV_CON, 2, 1, 0); // yuv-128

    SFR(EVA_JPG0->CON0, 9, 1, 1); // bs full , 联动模式下必须为1
    SFR(EVA_JPG0->CON0, 2, 1, 1); // code:0  / decode:1

    return 0;
}

static int lv_c3_hw_close(void)
{
    JL_JPG->CON0 = 0;
    JL_JPG->DMA_CON = 0;
    JL_JPG->BUF_SADR0 = 0;
    JL_JPG->BUF_EADR0 = 0;
    JL_JPG->STRIDE = 0;

#if C3_IRQ_ENABLE
    unrequest_irq(IRQ_JPG_IDX);
#endif
    return 0;
}

#endif

