/* Copyright(C) 2010- , JIELI TECHNOLOGY, Inc.
 * All right reserved.
 */
/* ------------------------------------------------------------------------------------*/
/**
 * @file ui_prime.h
 *
 * @brief 32bit哈希容量素数表
 * 1. 按约2倍递增列出哈希表扩容时可用的素数容量；
 * 2. 覆盖 2^1 ~ 2^31 区间，最大素数为 2^31-1 (0x7fffffff)；
 *
 * @author JIELI TECHNOLOGY ZHUHAI
 *
 * @version V1.0.0
 *
 * @date 2026-09-08
 */
/* ------------------------------------------------------------------------------------*/

#ifndef __UI_PRIME_H__
#define __UI_PRIME_H__


/* ------------------------------------------------------------------------------------*/
/*! 哈希素数宏族：UI_HASH_PRIME_<n> 对应"恰好小于 2^n 的最大素数"（n = 2~31），
 *  与 ui_prime_tab 各元素一一对应，可用作编译期常量（静态容量/乘子）。
 *  若所需素数不在表中（如希望完全按运行时容量取值），用 UI_PRIME_UPPER(need) 取最近素数 */
#ifndef UI_HASH_PRIME_2
#define UI_HASH_PRIME_2		3u		/* < 2^2 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_3
#define UI_HASH_PRIME_3		7u		/* < 2^3 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_4
#define UI_HASH_PRIME_4		13u		/* < 2^4 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_5
#define UI_HASH_PRIME_5		31u		/* < 2^5 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_6
#define UI_HASH_PRIME_6		61u		/* < 2^6 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_7
#define UI_HASH_PRIME_7		127u		/* < 2^7 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_8
#define UI_HASH_PRIME_8		251u		/* < 2^8 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_9
#define UI_HASH_PRIME_9		509u		/* < 2^9 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_10
#define UI_HASH_PRIME_10		1021u		/* < 2^10 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_11
#define UI_HASH_PRIME_11		2039u		/* < 2^11 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_12
#define UI_HASH_PRIME_12		4093u		/* < 2^12 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_13
#define UI_HASH_PRIME_13		8191u		/* < 2^13 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_14
#define UI_HASH_PRIME_14		16381u		/* < 2^14 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_15
#define UI_HASH_PRIME_15		32749u		/* < 2^15 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_16
#define UI_HASH_PRIME_16		65521u		/* < 2^16 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_17
#define UI_HASH_PRIME_17		131071u		/* < 2^17 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_18
#define UI_HASH_PRIME_18		262139u		/* < 2^18 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_19
#define UI_HASH_PRIME_19		524287u		/* < 2^19 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_20
#define UI_HASH_PRIME_20		1048573u		/* < 2^20 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_21
#define UI_HASH_PRIME_21		2097143u		/* < 2^21 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_22
#define UI_HASH_PRIME_22		4194301u		/* < 2^22 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_23
#define UI_HASH_PRIME_23		8388593u		/* < 2^23 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_24
#define UI_HASH_PRIME_24		16777213u		/* < 2^24 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_25
#define UI_HASH_PRIME_25		33554393u		/* < 2^25 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_26
#define UI_HASH_PRIME_26		67108859u		/* < 2^26 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_27
#define UI_HASH_PRIME_27		134217689u		/* < 2^27 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_28
#define UI_HASH_PRIME_28		268435399u		/* < 2^28 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_29
#define UI_HASH_PRIME_29		536870909u		/* < 2^29 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_30
#define UI_HASH_PRIME_30		1073741789u		/* < 2^30 的最大素数 */
#endif
#ifndef UI_HASH_PRIME_31
#define UI_HASH_PRIME_31		2147483647u		/* < 2^31 的最大素数 */
#endif
/* ------------------------------------------------------------------------------------*/


/*! 返回不小于期望容量 need 的最小素数（哈希扩容时用）；
 *  表中 30 个固定素数不够用时，通过本宏按运行期容量取值 */
#ifndef UI_PRIME_UPPER
#define UI_PRIME_UPPER(need)		ui_prime_upper((need))
#endif
/* ------------------------------------------------------------------------------------*/

unsigned int ui_prime_upper(unsigned int num);

/* ------------------------------------------------------------------------------------*/
/* 哈希计算 */
/* ------------------------------------------------------------------------------------*/

/* ------------------------------------------------------------------------------------*/
/**
 * @brief 计算 n 个 u32 值的哈希值
 *
 * 采用 FNV-1a 主循环 + fmix32 最终混合，速度快且碰撞率低，适合作为缓冲池节点的
 * 键值哈希（如按多个参数快速定位节点）。哈希初始值为 RANDOM_SEED（后续可由应用层
 * 替换为运行时随机种子，进一步降低针对性碰撞）。
 *
 * @Params value 指向 n 个 u32 值的数组
 * @Params count 数组元素个数
 *
 * @return 哈希值；value 为 NULL 或 count 为 0 时返回 RANDOM_SEED
 *
 * @note 哈希算法对输入顺序敏感：value 元素顺序不同，哈希值不同。
 */
/* ------------------------------------------------------------------------------------*/
unsigned int ui_prime_hash_int(const unsigned int *value, unsigned int count);

unsigned int ui_prime_hash_str(const char *str, int len);

#endif
