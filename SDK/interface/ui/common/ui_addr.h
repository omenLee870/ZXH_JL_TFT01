/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_addr.h
 *
 * @brief 地址判断与转换
 * 1. 判断地址在哪个存储介质，如FLASH、SRAM、PSRAM；
 * 2. FLASH地址CACHE、NOCACHE互转；
 * 3. PSRAM地址CACHE、NOCACHE互转；
 * 4. 判断资源地址是否是临时的；
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-04-09
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_ADDR_H__
#define __UI_ADDR_H__

#include "system/malloc.h"
#include "asm/psram_api.h"

#include "asm/eva_rocache.h"


//================================================//
//              GPU 纹理地址配置                  //
//  0: 忽略地址段指定，应用层传入什么，GPU用什么  //
//  1: 强制使用cache地址，底层强制转换            //
//  2: 强制使用eva cache地址，底层强制转换        //
//  3: 强制使用no cache地址，底层强制转换         //
//================================================//
extern const unsigned int GPU_FLASH_ADDR_RANGE; /*!< FLASH地址段定义 */
extern const unsigned int GPU_PSRAM_ADDR_RANGE; /*!< PSRAM地址段定义 */


/* ================================== 地址段基址定义 ================================== */

/* FLASH 三段地址 */
#define FLASH_BASE_ADDR         0x08000000U
#define FLASH_END_ADDR          0x14000000U

#define FLASH_EVA_BASE_ADDR     0x18000000U
#define FLASH_EVA_END_ADDR      0x24000000U

#define FLASH_NOCACHE_BASE_ADDR 0x28000000U
#define FLASH_NOCACHE_END_ADDR  0x34000000U

/* PSRAM 三段地址 */
#define PSRAM_BASE_ADDR         0x14000000U
#define PSRAM_END_ADDR          0x18000000U

#define PSRAM_EVA_BASE_ADDR     0x24000000U
#define PSRAM_EVA_END_ADDR      0x28000000U

#define PSRAM_NOCACHE_BASE_ADDR 0x34000000U
#define PSRAM_NOCACHE_END_ADDR  0x38000000U


/* ================================= FLASH 地址段判断 ================================= */

/**
 * @brief 判断地址是否在 FLASH cache 范围内（BASE包含，END不包含）
 */
#define UI_ADDR_IN_FLASH_CACHE(addr) \
	(((unsigned int)(addr) >= FLASH_BASE_ADDR) && ((unsigned int)(addr) < FLASH_END_ADDR))

/**
 * @brief 判断地址是否在 FLASH eva 范围内（BASE包含，END不包含）
 */
#define UI_ADDR_IN_FLASH_EVA(addr) \
	(((unsigned int)(addr) >= FLASH_EVA_BASE_ADDR) && ((unsigned int)(addr) < FLASH_EVA_END_ADDR))

/**
 * @brief 判断地址是否在 FLASH nocache 范围内（BASE包含，END不包含）
 */
#define UI_ADDR_IN_FLASH_NOCACHE(addr) \
	(((unsigned int)(addr) >= FLASH_NOCACHE_BASE_ADDR) && ((unsigned int)(addr) < FLASH_NOCACHE_END_ADDR))

/**
 * @brief 判断地址是否为 FLASH 地址（任意段）
 */
#define UI_ADDR_IN_FLASH(addr) \
	(UI_ADDR_IN_FLASH_CACHE(addr) || UI_ADDR_IN_FLASH_EVA(addr) || UI_ADDR_IN_FLASH_NOCACHE(addr))


/* ================================= PSRAM 地址段判断 ================================= */

/**
 * @brief 判断地址是否在 PSRAM cache 范围内（BASE包含，END不包含）
 */
#define UI_ADDR_IN_PSRAM_CACHE(addr) \
	(((unsigned int)(addr) >= PSRAM_BASE_ADDR) && ((unsigned int)(addr) < PSRAM_END_ADDR))

/**
 * @brief 判断地址是否在 PSRAM eva 范围内（BASE包含，END不包含）
 */
#define UI_ADDR_IN_PSRAM_EVA(addr) \
	(((unsigned int)(addr) >= PSRAM_EVA_BASE_ADDR) && ((unsigned int)(addr) < PSRAM_EVA_END_ADDR))

/**
 * @brief 判断地址是否在 PSRAM nocache 范围内（BASE包含，END不包含）
 */
#define UI_ADDR_IN_PSRAM_NOCACHE(addr) \
	(((unsigned int)(addr) >= PSRAM_NOCACHE_BASE_ADDR) && ((unsigned int)(addr) < PSRAM_NOCACHE_END_ADDR))

/**
 * @brief 判断地址是否为 PSRAM 地址（任意段）
 */
#define UI_ADDR_IN_PSRAM(addr) \
	(UI_ADDR_IN_PSRAM_CACHE(addr) || UI_ADDR_IN_PSRAM_EVA(addr) || UI_ADDR_IN_PSRAM_NOCACHE(addr))


/* =================================  SRAM 地址处理 ================================= */

/* 判断地址是否在物理 RAM 范围内 */
#define UI_ADDR_IN_PHY_RAM(addr)	(memory_in_phy((const void *)(addr)))

/* 判断地址是否在虚拟 RAM 范围内 */
#define UI_ADDR_IN_VLT_RAM(addr)	(memory_in_vtl((const void *)(addr)))

/* 判断地址是否为 RAM 地址 */
#define UI_ADDR_IN_RAM(addr)		(UI_ADDR_IN_PHY_RAM(addr) || UI_ADDR_IN_VLT_RAM(addr))


/* ================================= FLASH 地址转换 ================================= */

/**
 * @brief FLASH cache 地址转 eva 地址
 * @note  偏移量 = 目标段BASE - 源段BASE，保持段内偏移不变
 */
#define UI_FLASH_CACHE_TO_EVA(addr) \
	(UI_ADDR_IN_FLASH_CACHE(addr) ? \
		(void *)((unsigned int)(addr) - FLASH_BASE_ADDR + FLASH_EVA_BASE_ADDR) : (void *)(addr))

/**
 * @brief FLASH eva 地址转 cache 地址
 */
#define UI_FLASH_EVA_TO_CACHE(addr) \
	(UI_ADDR_IN_FLASH_EVA(addr) ? \
		(void *)((unsigned int)(addr) - FLASH_EVA_BASE_ADDR + FLASH_BASE_ADDR) : (void *)(addr))

/**
 * @brief FLASH cache 地址转 nocache 地址
 */
#define UI_FLASH_CACHE_TO_NOCACHE(addr) \
	(UI_ADDR_IN_FLASH_CACHE(addr) ? \
		(void *)((unsigned int)(addr) - FLASH_BASE_ADDR + FLASH_NOCACHE_BASE_ADDR) : (void *)(addr))

/**
 * @brief FLASH nocache 地址转 cache 地址
 */
#define UI_FLASH_NOCACHE_TO_CACHE(addr) \
	(UI_ADDR_IN_FLASH_NOCACHE(addr) ? \
		(void *)((unsigned int)(addr) - FLASH_NOCACHE_BASE_ADDR + FLASH_BASE_ADDR) : (void *)(addr))

/**
 * @brief FLASH eva 地址转 nocache 地址
 */
#define UI_FLASH_EVA_TO_NOCACHE(addr) \
	(UI_ADDR_IN_FLASH_EVA(addr) ? \
		(void *)((unsigned int)(addr) - FLASH_EVA_BASE_ADDR + FLASH_NOCACHE_BASE_ADDR) : (void *)(addr))

/**
 * @brief FLASH nocache 地址转 eva 地址
 */
#define UI_FLASH_NOCACHE_TO_EVA(addr) \
	(UI_ADDR_IN_FLASH_NOCACHE(addr) ? \
		(void *)((unsigned int)(addr) - FLASH_NOCACHE_BASE_ADDR + FLASH_EVA_BASE_ADDR) : (void *)(addr))


/* ================================= PSRAM 地址转换 ================================= */

/**
 * @brief PSRAM cache 地址转 eva 地址
 * @note  偏移量 = 目标段BASE - 源段BASE，保持段内偏移不变
 */
#define UI_PSRAM_CACHE_TO_EVA(addr) \
	(UI_ADDR_IN_PSRAM_CACHE(addr) ? \
		(void *)((unsigned int)(addr) - PSRAM_BASE_ADDR + PSRAM_EVA_BASE_ADDR) : (void *)(addr))

/**
 * @brief PSRAM eva 地址转 cache 地址
 */
#define UI_PSRAM_EVA_TO_CACHE(addr) \
	(UI_ADDR_IN_PSRAM_EVA(addr) ? \
		(void *)((unsigned int)(addr) - PSRAM_EVA_BASE_ADDR + PSRAM_BASE_ADDR) : (void *)(addr))

/**
 * @brief PSRAM cache 地址转 nocache 地址
 */
#define UI_PSRAM_CACHE_TO_NOCACHE(addr) \
	(UI_ADDR_IN_PSRAM_CACHE(addr) ? \
		(void *)((unsigned int)(addr) - PSRAM_BASE_ADDR + PSRAM_NOCACHE_BASE_ADDR) : (void *)(addr))

/**
 * @brief PSRAM nocache 地址转 cache 地址
 */
#define UI_PSRAM_NOCACHE_TO_CACHE(addr) \
	(UI_ADDR_IN_PSRAM_NOCACHE(addr) ? \
		(void *)((unsigned int)(addr) - PSRAM_NOCACHE_BASE_ADDR + PSRAM_BASE_ADDR) : (void *)(addr))

/**
 * @brief PSRAM eva 地址转 nocache 地址
 */
#define UI_PSRAM_EVA_TO_NOCACHE(addr) \
	(UI_ADDR_IN_PSRAM_EVA(addr) ? \
		(void *)((unsigned int)(addr) - PSRAM_EVA_BASE_ADDR + PSRAM_NOCACHE_BASE_ADDR) : (void *)(addr))

/**
 * @brief PSRAM nocache 地址转 eva 地址
 */
#define UI_PSRAM_NOCACHE_TO_EVA(addr) \
	(UI_ADDR_IN_PSRAM_NOCACHE(addr) ? \
		(void *)((unsigned int)(addr) - PSRAM_NOCACHE_BASE_ADDR + PSRAM_EVA_BASE_ADDR) : (void *)(addr))


/* ================================= 地址强制转换 ================================= */

/**
 * @brief 根据 GPU_FLASH_ADDR_RANGE / GPU_PSRAM_ADDR_RANGE 配置，
 *        将传入的 FLASH/PSRAM 地址强制转换到指定地址段
 *
 * @param addr 输入地址
 * @return     转换后的地址；若 addr 非 FLASH/PSRAM 则原样返回
 */
static inline void *ui_addr_process(void *addr)
{
    if (UI_ADDR_IN_FLASH(addr)) {
        switch (GPU_FLASH_ADDR_RANGE) {
        case 0:	/* 不转换，直接返回 */
            return addr;
        case 1:	/* 强制转为 FLASH_BASE (cache) 地址段 */
            if (UI_ADDR_IN_FLASH_CACHE(addr)) {
                return addr;
            } else if (UI_ADDR_IN_FLASH_EVA(addr)) {
                return UI_FLASH_EVA_TO_CACHE(addr);
            } else {
                return UI_FLASH_NOCACHE_TO_CACHE(addr);
            }
        case 2:	/* 强制转为 FLASH_EVA 地址段 */
            if (UI_ADDR_IN_FLASH_EVA(addr)) {
                return addr;
            } else if (UI_ADDR_IN_FLASH_CACHE(addr)) {
                return UI_FLASH_CACHE_TO_EVA(addr);
            } else {
                return UI_FLASH_NOCACHE_TO_EVA(addr);
            }
        case 3:	/* 强制转为 FLASH_NOCACHE 地址段 */
            if (UI_ADDR_IN_FLASH_NOCACHE(addr)) {
                return addr;
            } else if (UI_ADDR_IN_FLASH_CACHE(addr)) {
                return UI_FLASH_CACHE_TO_NOCACHE(addr);
            } else {
                return UI_FLASH_EVA_TO_NOCACHE(addr);
            }
        default:
            return addr;
        }
    } else if (UI_ADDR_IN_PSRAM(addr)) {
        switch (GPU_PSRAM_ADDR_RANGE) {
        case 0:	/* 不转换，直接返回 */
            return addr;
        case 1:	/* 强制转为 PSRAM_BASE (cache) 地址段 */
            if (UI_ADDR_IN_PSRAM_CACHE(addr)) {
                return addr;
            } else if (UI_ADDR_IN_PSRAM_EVA(addr)) {
                return UI_PSRAM_EVA_TO_CACHE(addr);
            } else {
                return UI_PSRAM_NOCACHE_TO_CACHE(addr);
            }
        case 2:	/* 强制转为 PSRAM_EVA 地址段 */
            if (UI_ADDR_IN_PSRAM_EVA(addr)) {
                return addr;
            } else if (UI_ADDR_IN_PSRAM_CACHE(addr)) {
                return UI_PSRAM_CACHE_TO_EVA(addr);
            } else {
                return UI_PSRAM_NOCACHE_TO_EVA(addr);
            }
        case 3:	/* 强制转为 PSRAM_NOCACHE 地址段 */
            if (UI_ADDR_IN_PSRAM_NOCACHE(addr)) {
                return addr;
            } else if (UI_ADDR_IN_PSRAM_CACHE(addr)) {
                return UI_PSRAM_CACHE_TO_NOCACHE(addr);
            } else {
                return UI_PSRAM_EVA_TO_NOCACHE(addr);
            }
        default:
            return addr;
        }
    } else {
        return addr;
    }
}

/* ================================== 测试代码 ================================== */
/**
 * @brief ui_addr.h 模块自测代码
 * @note  定义 UI_ADDR_TEST_ENABLE 宏以启用测试，测试完成后取消宏定义即可屏蔽
 *
 * 测试覆盖：
 *  1. 6 个地址段判断宏的边界测试（BASE-1、BASE、BASE+1、中间值、END-1、END）
 *  2. UI_ADDR_IN_FLASH / UI_ADDR_IN_PSRAM 综合判断
 *  3. 12 个地址转换宏的正确性
 *  4. 往返转换（A->B->A 恒等）
 *  5. ui_addr_process 在各种 GPU_xxx_ADDR_RANGE 配置下的行为
 *  6. 段内偏移保持验证
 *  7. 各段首尾地址的边界转换
 *  8. 非 FLASH/PSRAM 地址直通验证
 */
#if 0//def UI_ADDR_TEST_ENABLE

/* #include <stdio.h> */

static int __ui_addr_test_failures;

#define UI_ADDR_TEST_ASSERT(cond, msg) \
	do { \
		if (!(cond)) { \
			printf("  [FAIL] %s\n", msg); \
			__ui_addr_test_failures++; \
		} else { \
			printf("  [PASS] %s\n", msg); \
		} \
	} while (0)

static void ui_addr_test(void)
{
    __ui_addr_test_failures = 0;
    printf("\n========== ui_addr.h Module Test ==========\n");

    /* ---- 1. FLASH Cache 段判断（边界测试） ---- */
    printf("\n--- UI_ADDR_IN_FLASH_CACHE ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_CACHE(0x07FFFFFFU),
                        "0x07FFFFFF (BASE-1) NOT in FLASH_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(0x08000000U),
                        "0x08000000 (BASE)   in FLASH_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(0x08000001U),
                        "0x08000001 (BASE+1) in FLASH_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(0x10000000U),
                        "0x10000000 (mid)    in FLASH_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(0x13FFFFFFU),
                        "0x13FFFFFF (END-1)  in FLASH_CACHE");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_CACHE(0x14000000U),
                        "0x14000000 (END)   NOT in FLASH_CACHE");

    /* ---- 2. FLASH EVA 段判断（边界测试） ---- */
    printf("\n--- UI_ADDR_IN_FLASH_EVA ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_EVA(0x17FFFFFFU),
                        "0x17FFFFFF (BASE-1) NOT in FLASH_EVA");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_EVA(0x18000000U),
                        "0x18000000 (BASE)   in FLASH_EVA");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_EVA(0x20000000U),
                        "0x20000000 (mid)    in FLASH_EVA");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_EVA(0x23FFFFFFU),
                        "0x23FFFFFF (END-1)  in FLASH_EVA");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_EVA(0x24000000U),
                        "0x24000000 (END)   NOT in FLASH_EVA");

    /* ---- 3. FLASH NOCACHE 段判断（边界测试） ---- */
    printf("\n--- UI_ADDR_IN_FLASH_NOCACHE ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_NOCACHE(0x27FFFFFFU),
                        "0x27FFFFFF (BASE-1) NOT in FLASH_NOCACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_NOCACHE(0x28000000U),
                        "0x28000000 (BASE)   in FLASH_NOCACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_NOCACHE(0x30000000U),
                        "0x30000000 (mid)    in FLASH_NOCACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_NOCACHE(0x33FFFFFFU),
                        "0x33FFFFFF (END-1)  in FLASH_NOCACHE");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_NOCACHE(0x34000000U),
                        "0x34000000 (END)   NOT in FLASH_NOCACHE");

    /* ---- 4. UI_ADDR_IN_FLASH 综合判断 ---- */
    printf("\n--- UI_ADDR_IN_FLASH ---\n");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH(0x08000000U),
                        "FLASH_CACHE addr   -> in FLASH");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH(0x18000000U),
                        "FLASH_EVA addr     -> in FLASH");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH(0x28000000U),
                        "FLASH_NOCACHE addr -> in FLASH");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH(0x14000000U),
                        "PSRAM_BASE (0x14000000) NOT in FLASH");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH(0x00000000U),
                        "NULL-ish addr NOT in FLASH");

    /* ---- 5. PSRAM Cache 段判断（边界测试） ---- */
    printf("\n--- UI_ADDR_IN_PSRAM_CACHE ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_CACHE(0x13FFFFFFU),
                        "0x13FFFFFF (BASE-1) NOT in PSRAM_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_CACHE(0x14000000U),
                        "0x14000000 (BASE)   in PSRAM_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_CACHE(0x16000000U),
                        "0x16000000 (mid)    in PSRAM_CACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_CACHE(0x17FFFFFFU),
                        "0x17FFFFFF (END-1)  in PSRAM_CACHE");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_CACHE(0x18000000U),
                        "0x18000000 (END)   NOT in PSRAM_CACHE");

    /* ---- 6. PSRAM EVA 段判断（边界测试） ---- */
    printf("\n--- UI_ADDR_IN_PSRAM_EVA ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_EVA(0x23FFFFFFU),
                        "0x23FFFFFF (BASE-1) NOT in PSRAM_EVA");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_EVA(0x24000000U),
                        "0x24000000 (BASE)   in PSRAM_EVA");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_EVA(0x26000000U),
                        "0x26000000 (mid)    in PSRAM_EVA");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_EVA(0x27FFFFFFU),
                        "0x27FFFFFF (END-1)  in PSRAM_EVA");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_EVA(0x28000000U),
                        "0x28000000 (END)   NOT in PSRAM_EVA");

    /* ---- 7. PSRAM NOCACHE 段判断（边界测试） ---- */
    printf("\n--- UI_ADDR_IN_PSRAM_NOCACHE ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_NOCACHE(0x33FFFFFFU),
                        "0x33FFFFFF (BASE-1) NOT in PSRAM_NOCACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_NOCACHE(0x34000000U),
                        "0x34000000 (BASE)   in PSRAM_NOCACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_NOCACHE(0x36000000U),
                        "0x36000000 (mid)    in PSRAM_NOCACHE");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_NOCACHE(0x37FFFFFFU),
                        "0x37FFFFFF (END-1)  in PSRAM_NOCACHE");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_NOCACHE(0x38000000U),
                        "0x38000000 (END)   NOT in PSRAM_NOCACHE");

    /* ---- 8. UI_ADDR_IN_PSRAM 综合判断 ---- */
    printf("\n--- UI_ADDR_IN_PSRAM ---\n");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM(0x14000000U),
                        "PSRAM_CACHE addr   -> in PSRAM");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM(0x24000000U),
                        "PSRAM_EVA addr     -> in PSRAM");
    UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM(0x34000000U),
                        "PSRAM_NOCACHE addr -> in PSRAM");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM(0x08000000U),
                        "FLASH_BASE (0x08000000) NOT in PSRAM");

    /* ---- 9. FLASH 地址转换宏 ---- */
    printf("\n--- FLASH Address Conversion ---\n");
    {
        void *p;

        /* CACHE -> EVA */
        p = UI_FLASH_CACHE_TO_EVA((void *)0x08000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x18000000U,
                            "CACHE->EVA: 0x08000000 -> 0x18000000");
        p = UI_FLASH_CACHE_TO_EVA((void *)0x08001000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x18001000U,
                            "CACHE->EVA: offset 0x1000 preserved");
        p = UI_FLASH_CACHE_TO_EVA((void *)0x13FFFFFFU);
        UI_ADDR_TEST_ASSERT(p == (void *)0x23FFFFFFU,
                            "CACHE->EVA: END-1 boundary");

        /* EVA -> CACHE */
        p = UI_FLASH_EVA_TO_CACHE((void *)0x18000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x08000000U,
                            "EVA->CACHE: 0x18000000 -> 0x08000000");
        p = UI_FLASH_EVA_TO_CACHE((void *)0x20000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x10000000U,
                            "EVA->CACHE: 0x20000000 -> 0x10000000");

        /* CACHE -> NOCACHE */
        p = UI_FLASH_CACHE_TO_NOCACHE((void *)0x08000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x28000000U,
                            "CACHE->NOCACHE: 0x08000000 -> 0x28000000");
        p = UI_FLASH_CACHE_TO_NOCACHE((void *)0x10000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x30000000U,
                            "CACHE->NOCACHE: 0x10000000 -> 0x30000000");

        /* NOCACHE -> CACHE */
        p = UI_FLASH_NOCACHE_TO_CACHE((void *)0x28000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x08000000U,
                            "NOCACHE->CACHE: 0x28000000 -> 0x08000000");
        p = UI_FLASH_NOCACHE_TO_CACHE((void *)0x30000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x10000000U,
                            "NOCACHE->CACHE: 0x30000000 -> 0x10000000");

        /* EVA -> NOCACHE */
        p = UI_FLASH_EVA_TO_NOCACHE((void *)0x18000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x28000000U,
                            "EVA->NOCACHE: 0x18000000 -> 0x28000000");
        p = UI_FLASH_EVA_TO_NOCACHE((void *)0x20000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x30000000U,
                            "EVA->NOCACHE: 0x20000000 -> 0x30000000");

        /* NOCACHE -> EVA */
        p = UI_FLASH_NOCACHE_TO_EVA((void *)0x28000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x18000000U,
                            "NOCACHE->EVA: 0x28000000 -> 0x18000000");
        p = UI_FLASH_NOCACHE_TO_EVA((void *)0x30000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x20000000U,
                            "NOCACHE->EVA: 0x30000000 -> 0x20000000");

        /* 非源段地址不转换 */
        p = UI_FLASH_CACHE_TO_EVA((void *)0x14000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x14000000U,
                            "CACHE->EVA: non-FLASH_CACHE addr unchanged");
    }

    /* ---- 10. PSRAM 地址转换宏 ---- */
    printf("\n--- PSRAM Address Conversion ---\n");
    {
        void *p;

        /* CACHE -> EVA */
        p = UI_PSRAM_CACHE_TO_EVA((void *)0x14000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x24000000U,
                            "CACHE->EVA: 0x14000000 -> 0x24000000");
        p = UI_PSRAM_CACHE_TO_EVA((void *)0x15000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x25000000U,
                            "CACHE->EVA: 0x15000000 -> 0x25000000");

        /* EVA -> CACHE */
        p = UI_PSRAM_EVA_TO_CACHE((void *)0x24000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x14000000U,
                            "EVA->CACHE: 0x24000000 -> 0x14000000");
        p = UI_PSRAM_EVA_TO_CACHE((void *)0x25000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x15000000U,
                            "EVA->CACHE: 0x25000000 -> 0x15000000");

        /* CACHE -> NOCACHE */
        p = UI_PSRAM_CACHE_TO_NOCACHE((void *)0x14000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x34000000U,
                            "CACHE->NOCACHE: 0x14000000 -> 0x34000000");
        p = UI_PSRAM_CACHE_TO_NOCACHE((void *)0x15000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x35000000U,
                            "CACHE->NOCACHE: 0x15000000 -> 0x35000000");

        /* NOCACHE -> CACHE */
        p = UI_PSRAM_NOCACHE_TO_CACHE((void *)0x34000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x14000000U,
                            "NOCACHE->CACHE: 0x34000000 -> 0x14000000");

        /* EVA -> NOCACHE */
        p = UI_PSRAM_EVA_TO_NOCACHE((void *)0x24000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x34000000U,
                            "EVA->NOCACHE: 0x24000000 -> 0x34000000");

        /* NOCACHE -> EVA */
        p = UI_PSRAM_NOCACHE_TO_EVA((void *)0x34000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x24000000U,
                            "NOCACHE->EVA: 0x34000000 -> 0x24000000");

        /* 非源段地址不转换 */
        p = UI_PSRAM_CACHE_TO_EVA((void *)0x08000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x08000000U,
                            "CACHE->EVA: non-PSRAM_CACHE addr unchanged");
    }

    /* ---- 11. 往返转换测试 (A -> B -> A 恒等) ---- */
    printf("\n--- Round-Trip Test ---\n");
    {
        void *orig, *converted, *back;
        unsigned int test_addrs[] = {
            0x08000000U, 0x08001000U, 0x10000000U, 0x13FFFFFFU,
            0x18000000U, 0x20000000U, 0x23FFFFFFU,
            0x28000000U, 0x30000000U, 0x33FFFFFFU,
            0x14000000U, 0x16000000U, 0x17FFFFFFU,
            0x24000000U, 0x26000000U, 0x27FFFFFFU,
            0x34000000U, 0x36000000U, 0x37FFFFFFU,
        };
        int i;
        for (i = 0; i < (int)(sizeof(test_addrs) / sizeof(test_addrs[0])); i++) {
            orig = (void *)test_addrs[i];
            if (UI_ADDR_IN_FLASH_CACHE(orig)) {
                converted = UI_FLASH_CACHE_TO_EVA(orig);
                back = UI_FLASH_EVA_TO_CACHE(converted);
                UI_ADDR_TEST_ASSERT(orig == back,
                                    "FLASH CACHE<->EVA round-trip OK");
                converted = UI_FLASH_CACHE_TO_NOCACHE(orig);
                back = UI_FLASH_NOCACHE_TO_CACHE(converted);
                UI_ADDR_TEST_ASSERT(orig == back,
                                    "FLASH CACHE<->NOCACHE round-trip OK");
            }
            if (UI_ADDR_IN_FLASH_EVA(orig)) {
                converted = UI_FLASH_EVA_TO_NOCACHE(orig);
                back = UI_FLASH_NOCACHE_TO_EVA(converted);
                UI_ADDR_TEST_ASSERT(orig == back,
                                    "FLASH EVA<->NOCACHE round-trip OK");
            }
            if (UI_ADDR_IN_PSRAM_CACHE(orig)) {
                converted = UI_PSRAM_CACHE_TO_EVA(orig);
                back = UI_PSRAM_EVA_TO_CACHE(converted);
                UI_ADDR_TEST_ASSERT(orig == back,
                                    "PSRAM CACHE<->EVA round-trip OK");
                converted = UI_PSRAM_CACHE_TO_NOCACHE(orig);
                back = UI_PSRAM_NOCACHE_TO_CACHE(converted);
                UI_ADDR_TEST_ASSERT(orig == back,
                                    "PSRAM CACHE<->NOCACHE round-trip OK");
            }
            if (UI_ADDR_IN_PSRAM_EVA(orig)) {
                converted = UI_PSRAM_EVA_TO_NOCACHE(orig);
                back = UI_PSRAM_NOCACHE_TO_EVA(converted);
                UI_ADDR_TEST_ASSERT(orig == back,
                                    "PSRAM EVA<->NOCACHE round-trip OK");
            }
        }
    }

    /* ---- 12. ui_addr_process 核心函数测试 ---- */
    printf("\n--- ui_addr_process Test ---\n");
    {
        void *result;
        unsigned int cfg_flash = GPU_FLASH_ADDR_RANGE;
        unsigned int cfg_psram = GPU_PSRAM_ADDR_RANGE;

        printf("  GPU_FLASH_ADDR_RANGE = %u\n", cfg_flash);
        printf("  GPU_PSRAM_ADDR_RANGE = %u\n", cfg_psram);

        /* 非 FLASH/PSRAM 地址应原样返回 */
        result = ui_addr_process((void *)0x00000000U);
        UI_ADDR_TEST_ASSERT(result == (void *)0x00000000U,
                            "NULL addr pass-through");
        result = ui_addr_process((void *)0x00001000U);
        UI_ADDR_TEST_ASSERT(result == (void *)0x00001000U,
                            "SRAM addr pass-through");
        result = ui_addr_process((void *)0xFFFFFFFFU);
        UI_ADDR_TEST_ASSERT(result == (void *)0xFFFFFFFFU,
                            "0xFFFFFFFF pass-through");

        /* ---- FLASH 地址测试 ---- */
        /* FLASH_CACHE addr -> 根据 GPU_FLASH_ADDR_RANGE 转换 */
        result = ui_addr_process((void *)0x08000000U);
        if (cfg_flash == 0) {
            UI_ADDR_TEST_ASSERT(result == (void *)0x08000000U,
                                "FLASH R0: CACHE addr unchanged");
        } else if (cfg_flash == 1) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(result),
                                "FLASH R1: CACHE addr -> CACHE");
        } else if (cfg_flash == 2) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_EVA(result),
                                "FLASH R2: CACHE addr -> EVA");
        } else if (cfg_flash == 3) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_NOCACHE(result),
                                "FLASH R3: CACHE addr -> NOCACHE");
        }

        /* FLASH_EVA addr -> 根据 GPU_FLASH_ADDR_RANGE 转换 */
        result = ui_addr_process((void *)0x18000000U);
        if (cfg_flash == 0) {
            UI_ADDR_TEST_ASSERT(result == (void *)0x18000000U,
                                "FLASH R0: EVA addr unchanged");
        } else if (cfg_flash == 1) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(result),
                                "FLASH R1: EVA addr -> CACHE");
        } else if (cfg_flash == 2) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_EVA(result),
                                "FLASH R2: EVA addr -> EVA (already)");
        } else if (cfg_flash == 3) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_NOCACHE(result),
                                "FLASH R3: EVA addr -> NOCACHE");
        }

        /* FLASH_NOCACHE addr -> 根据 GPU_FLASH_ADDR_RANGE 转换 */
        result = ui_addr_process((void *)0x28000000U);
        if (cfg_flash == 0) {
            UI_ADDR_TEST_ASSERT(result == (void *)0x28000000U,
                                "FLASH R0: NOCACHE addr unchanged");
        } else if (cfg_flash == 1) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_CACHE(result),
                                "FLASH R1: NOCACHE addr -> CACHE");
        } else if (cfg_flash == 2) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_EVA(result),
                                "FLASH R2: NOCACHE addr -> EVA");
        } else if (cfg_flash == 3) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_FLASH_NOCACHE(result),
                                "FLASH R3: NOCACHE addr -> NOCACHE (already)");
        }

        /* ---- PSRAM 地址测试 ---- */
        /* PSRAM_CACHE addr -> 根据 GPU_PSRAM_ADDR_RANGE 转换 */
        result = ui_addr_process((void *)0x14000000U);
        if (cfg_psram == 0) {
            UI_ADDR_TEST_ASSERT(result == (void *)0x14000000U,
                                "PSRAM R0: CACHE addr unchanged");
        } else if (cfg_psram == 1) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_CACHE(result),
                                "PSRAM R1: CACHE addr -> CACHE");
        } else if (cfg_psram == 2) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_EVA(result),
                                "PSRAM R2: CACHE addr -> EVA");
        } else if (cfg_psram == 3) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_NOCACHE(result),
                                "PSRAM R3: CACHE addr -> NOCACHE");
        }

        /* PSRAM_EVA addr -> 根据 GPU_PSRAM_ADDR_RANGE 转换 */
        result = ui_addr_process((void *)0x24000000U);
        if (cfg_psram == 0) {
            UI_ADDR_TEST_ASSERT(result == (void *)0x24000000U,
                                "PSRAM R0: EVA addr unchanged");
        } else if (cfg_psram == 1) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_CACHE(result),
                                "PSRAM R1: EVA addr -> CACHE");
        } else if (cfg_psram == 2) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_EVA(result),
                                "PSRAM R2: EVA addr -> EVA (already)");
        } else if (cfg_psram == 3) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_NOCACHE(result),
                                "PSRAM R3: EVA addr -> NOCACHE");
        }

        /* PSRAM_NOCACHE addr -> 根据 GPU_PSRAM_ADDR_RANGE 转换 */
        result = ui_addr_process((void *)0x34000000U);
        if (cfg_psram == 0) {
            UI_ADDR_TEST_ASSERT(result == (void *)0x34000000U,
                                "PSRAM R0: NOCACHE addr unchanged");
        } else if (cfg_psram == 1) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_CACHE(result),
                                "PSRAM R1: NOCACHE addr -> CACHE");
        } else if (cfg_psram == 2) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_EVA(result),
                                "PSRAM R2: NOCACHE addr -> EVA");
        } else if (cfg_psram == 3) {
            UI_ADDR_TEST_ASSERT(UI_ADDR_IN_PSRAM_NOCACHE(result),
                                "PSRAM R3: NOCACHE addr -> NOCACHE (already)");
        }
    }

    /* ---- 13. 段内偏移保持测试 ---- */
    printf("\n--- Intra-Segment Offset Preservation ---\n");
    {
        void *p;
        /* FLASH 段内偏移 28 位（0x0C000000 跨度） */
        p = UI_FLASH_CACHE_TO_EVA((void *)0x08000100U);
        UI_ADDR_TEST_ASSERT(((unsigned int)p & 0x0FFFFFFFU) == 0x00000100U,
                            "FLASH CACHE->EVA offset preserved");
        p = UI_FLASH_CACHE_TO_NOCACHE((void *)0x0800ABCDU);
        UI_ADDR_TEST_ASSERT(((unsigned int)p & 0x0FFFFFFFU) == 0x0000ABCDU,
                            "FLASH CACHE->NOCACHE offset preserved");
        p = UI_FLASH_EVA_TO_NOCACHE((void *)0x18001234U);
        UI_ADDR_TEST_ASSERT(((unsigned int)p & 0x0FFFFFFFU) == 0x00001234U,
                            "FLASH EVA->NOCACHE offset preserved");

        /* PSRAM 段内偏移 26 位（0x04000000 跨度） */
        p = UI_PSRAM_CACHE_TO_EVA((void *)0x14005678U);
        UI_ADDR_TEST_ASSERT(((unsigned int)p & 0x03FFFFFFU) == 0x00005678U,
                            "PSRAM CACHE->EVA offset preserved");
        p = UI_PSRAM_CACHE_TO_NOCACHE((void *)0x14009ABCU);
        UI_ADDR_TEST_ASSERT(((unsigned int)p & 0x03FFFFFFU) == 0x00009ABCU,
                            "PSRAM CACHE->NOCACHE offset preserved");
    }

    /* ---- 14. 各段首尾边界值转换 ---- */
    printf("\n--- Boundary Value Conversion ---\n");
    {
        void *p;
        /* FLASH_CACHE BASE */
        p = UI_FLASH_CACHE_TO_EVA((void *)0x08000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x18000000U,
                            "FLASH_CACHE BASE -> EVA BASE");
        /* FLASH_CACHE END-1 */
        p = UI_FLASH_CACHE_TO_EVA((void *)0x13FFFFFFU);
        UI_ADDR_TEST_ASSERT(p == (void *)0x23FFFFFFU,
                            "FLASH_CACHE END-1 -> EVA END-1");
        /* PSRAM_CACHE BASE */
        p = UI_PSRAM_CACHE_TO_NOCACHE((void *)0x14000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x34000000U,
                            "PSRAM_CACHE BASE -> NOCACHE BASE");
        /* PSRAM_CACHE END-1 */
        p = UI_PSRAM_CACHE_TO_NOCACHE((void *)0x17FFFFFFU);
        UI_ADDR_TEST_ASSERT(p == (void *)0x37FFFFFFU,
                            "PSRAM_CACHE END-1 -> NOCACHE END-1");
        /* FLASH_NOCACHE BASE */
        p = UI_FLASH_NOCACHE_TO_CACHE((void *)0x28000000U);
        UI_ADDR_TEST_ASSERT(p == (void *)0x08000000U,
                            "FLASH_NOCACHE BASE -> CACHE BASE");
        /* PSRAM_NOCACHE END-1 */
        p = UI_PSRAM_NOCACHE_TO_EVA((void *)0x37FFFFFFU);
        UI_ADDR_TEST_ASSERT(p == (void *)0x27FFFFFFU,
                            "PSRAM_NOCACHE END-1 -> EVA END-1");
    }

    /* ---- 15. 段交界处不混淆验证 ---- */
    printf("\n--- Segment Boundary Non-Overlap ---\n");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_CACHE(0x14000000U),
                        "0x14000000 (PSRAM_BASE) is NOT FLASH_CACHE");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_CACHE(0x18000000U),
                        "0x18000000 (FLASH_EVA_BASE) is NOT PSRAM_CACHE");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_EVA(0x24000000U),
                        "0x24000000 (PSRAM_EVA_BASE) is NOT FLASH_EVA");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_PSRAM_EVA(0x28000000U),
                        "0x28000000 (FLASH_NOCACHE_BASE) is NOT PSRAM_EVA");
    UI_ADDR_TEST_ASSERT(!UI_ADDR_IN_FLASH_NOCACHE(0x34000000U),
                        "0x34000000 (PSRAM_NOCACHE_BASE) is NOT FLASH_NOCACHE");

    /* ---- 结果汇总 ---- */
    printf("\n==========================================\n");
    if (__ui_addr_test_failures == 0) {
        printf("  Test Result: ALL PASSED!\n");
    } else {
        printf("  Test Result: %d FAILURE(S)!\n", __ui_addr_test_failures);
    }
    printf("==========================================\n\n");
}

#endif /* UI_ADDR_TEST_ENABLE */


#endif
