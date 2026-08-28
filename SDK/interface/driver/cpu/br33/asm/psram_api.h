#ifndef __PSRAM_API_H__
#define __PSRAM_API_H__

//------------------------------------------------------//
// xpll config
//------------------------------------------------------//

#define PSRAM_MRR_DUM 4
#define PSRAM_MRW_DUM 0

typedef enum {
    RESERVE,
    XPLL_REFCLK_PAT,
    XPLL_REFCLK_RC16M,
    XPLL_REFCLK_EXT,
    XPLL_REFCLK_XOSC24M,
    XPLL_REFCLK_FLL24M,
} xpll_refclk_typedef;

typedef struct {
    union {
        struct {
            unsigned int PLL_EN: 1; //Bit 0
            unsigned int PLL_RN: 1; //Bit 1
            unsigned int PLL_MODE: 1; //Bit 2
            unsigned int reserved0: 1; //Bit 3
            unsigned int PLL_PFDS: 3; // Bits 4-6
            unsigned int PLL_ICPS: 2; // Bits 7-8
            unsigned int reserved1: 1; //Bit 9
            unsigned int PLL_LPFR2S: 3; // Bits 10-12
            unsigned int PLL_IVCOS: 3; // Bits 13-15
            unsigned int PLL_LDO11AS: 4; // Bits 16-19
            unsigned int PLL_LDO_BYPASS: 1; //Bit 20
            unsigned int PLL_KVCO_S: 2; // Bits 21-22
            unsigned int PLL_VCO_CAP_S: 5; // Bits 23-27
            unsigned int PLL_VC_OE: 1; //Bit 28
            unsigned int PLL_VC_DRVEN: 1; //Bit 29
            unsigned int PLL_VC_DRVS: 1; //Bit 30
            unsigned int reserved3: 1; //Bit 31
        };
        unsigned int value;
    } con0;
    union {
        struct {
            unsigned int PLL_REFDS: 7; // Bits 0-6
            unsigned int reserved0: 1; //Bit 7
            unsigned int PLL_REFCLKS: 3; // Bits 8-10
            unsigned int reserved1: 2; // Bits 11-12
            unsigned int PLL_REFDSEN: 1; //Bit 13
            unsigned int reserved2: 2; // Bits 14-15
            unsigned int PLL_CP_IOCSEL: 1; //Bit 16
            unsigned int PLL_CP_IOC_EN: 1; //Bit 17
            unsigned int PLL_CP_IOC_S: 2; // Bits 18-19
            unsigned int PLL_TEST_EN: 1; //Bit 20
            unsigned int PLL_TESTS: 2; // Bits 21-22
            unsigned int reserved3: 1; //Bit 23
            unsigned int PLL_BIAS_EN: 1; // Bits 24
            unsigned int PLL_RESER: 1; // Bits 25
            unsigned int PLL_IQGEN_DY: 2; // Bits 26-27
            unsigned int PLL_OPENLOOP_EN: 1; //Bit 28
            unsigned int reserved4: 3; // Bits 29-31
        };
        unsigned int value;
    } con1;
    union {
        struct {
            unsigned int PLL_D1P0_OE: 1; //Bit 0
            unsigned int PLL_D1P5_OE: 1; //Bit 1
            unsigned int PLL_D2P0_OE: 1; //Bit 2
            unsigned int PLL_D2P5_OE: 1; //Bit 3
            unsigned int PLL_D3P5_OE: 1; //Bit 4
            unsigned int PLL_CKSYN_CORE_OE: 1; //Bit 5
            unsigned int PLL_CKDSM_OE: 1; //Bit 6
            unsigned int PLL_CKEN: 1; //Bit 7
            unsigned int PLL_DSM_CKEN: 1; //Bit 8
            unsigned int PLL_NR_CKEN: 1; //Bit 9
            unsigned int reserved0: 22; // Bits 10-31
        };
        unsigned int value;
    } con2;
    union {
        struct {
            unsigned int DMAX: 24; // Bits 0-23
        };
        unsigned int value;
    } dmax;
    union {
        struct {
            unsigned int DMIN: 24; // Bits 0-23
        };
        unsigned int value;
    } dmin;
    union {
        struct {
            unsigned int PLL_DSM_RST: 1; //Bit 0
            unsigned int PLL_DSM_S: 2; // Bits 1-2
            unsigned int PLL_MSEL: 2; // Bits 3-4
            unsigned int PLL_RSEL: 2; // Bits 5-6
            unsigned int PLL_TSEL: 4; // Bits 7-10
            unsigned int reserved0: 21; // Bits 11-31
        };
        unsigned int value;
    } dsm;
    union {
        struct {
            unsigned int DSTP: 24; // Bits 0-23
        };
        unsigned int value;
    } dstp;
    union {
        struct {
            unsigned int INTF0: 32; // Bits 0-31
        };
        unsigned int value;
    } intf0;
    union {
        struct {
            unsigned int INTF1: 4; // Bits 0-3
        };
        unsigned int value;
    } intf1;
    union {
        struct {
            unsigned int PLL_NR: 12; // Bits 0-11
        };
        unsigned int value;
    } nr;
    union {
        struct {
            unsigned int SSC_EN: 1; //Bit 0
            unsigned int INTF_SEL: 1; //Bit 1
            unsigned int DSM_OFFSET_BT: 1; //Bit 2
            unsigned int DSM_OFFSET_FM: 1; //Bit 3
            unsigned int DSM_OFFSET_UT: 1; //Bit 4
            unsigned int reserved0: 27; // Bits 5-31
        };
        unsigned int value;
    } ssc;
    union {
        struct {
            unsigned int CLKSEL: 1; //Bit 0
            unsigned int RSTN: 1; //Bit 1
            unsigned int reserved0: 30; // Bits 2-31
        };
        unsigned int value;
    } xspi_con;
} xpll_cfg_t;

enum PSRAM_PORT_SEL_TABLE {
    PSRAM_PORT_SEL_PORTA = 0,
};

enum PSRAM_MODE_SEL_TABLE {
    PSRAM_MODE_1_WIRE = 0,
    PSRAM_MODE_4_WIRE_CMD1_ADR4_DAT4,
    PSRAM_MODE_4_WIRE_CMD4_ADR4_DAT4,
};


enum psram_power_status {
    PSRAM_STATE_POWER_OFF,
    PSRAM_STATE_POWER_ON,
    PSRAM_STATE_POWER_STANDBY,
};

struct psram_platform_data {
    u8 power_port;
    enum PSRAM_PORT_SEL_TABLE port;
    enum PSRAM_MODE_SEL_TABLE mode;
    u32 init_clk;
};

#define PSRAM_PLATFORM_DATA_BEGIN(data) \
		static const struct psram_platform_data data = {

#define PSRAM_PLATFORM_DATA_END()  \
};

/* ---------------------------------------------------------------------------- */
/**
 * @brief 获取psram在使用的buf数量
 *
 * @return psram在使用的buf数量
 */
/* ---------------------------------------------------------------------------- */
extern u32 psram_get_used_block(void);

/* ---------------------------------------------------------------------------- */
/**
 * @brief psram heap reset
 */
/* ---------------------------------------------------------------------------- */
extern void psram_heap_reset(void);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief psram_flush_cache
 *
 * @param begin
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
void psram_flush_cache(void *begin, u32 len);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief psram_flush_invaild_cache
 *
 * @param begin
 * @param len
 */
/* ------------------------------------------------------------------------------------*/
void psram_flush_invaild_cache(void *begin, u32 len);
/* ------------------------------------------------------------------------------------*/
/**
 * @brief psram_init psram初始化
 *
 * @param config
 */
/* ------------------------------------------------------------------------------------*/
void psram_init(const struct psram_platform_data *config);

/* --------------------------------------------------------------------------*/
/**
 * @brief :检查mem是否psram cache地址
 *
 * @param mem :检查对象
 *
 * @return :true - psram地址，false - 非psram地址
 */
/* ----------------------------------------------------------------------------*/
int psram_cache_check(void *pv);
/* --------------------------------------------------------------------------*/
/**
 * @brief :检查mem是否psram no cache地址
 *
 * @param mem :检查对象
 *
 * @return :true - psram地址，false - 非psram地址
 */
/* ----------------------------------------------------------------------------*/
int psram_no_cache_check(void *pv);

/* --------------------------------------------------------------------------*/
/**
 * @brief :psram no cache地址转换为cache地址
 *
 * @param mem :检查对象
 *
 * @return :非0- psram cahce地址，NULL - 非psram地址
 */
/* ----------------------------------------------------------------------------*/
void *psram_no_cache_2_cache(void *pv);
/* --------------------------------------------------------------------------*/
/**
 * @brief :psram cache地址转换为no cache地址
 *
 * @param mem :检查对象
 *
 * @return :非0- psram no cahce地址，NULL - 非psram地址
 */
/* ----------------------------------------------------------------------------*/
void *psram_cache_2_no_cache(void *pv);

#endif
