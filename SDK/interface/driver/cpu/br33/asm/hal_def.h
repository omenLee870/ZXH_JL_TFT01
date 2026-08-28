#ifndef _hal_def_h_
#define _hal_def_h_

#define __IO        volatile
#define __O         volatile

/**
 * @addtogroup Exported_types
 */
typedef enum {
    STATE_RESET = 0,
    STATE_SET = !STATE_RESET
} FlagStatus, ITStatus;

typedef enum {
    STATE_DISABLE = 0,
    STATE_ENABLE = !STATE_DISABLE
} FunctionalState;
#define IS_FUNCTIONAL_STATE(STATE) (((STATE) == STATE_DISABLE) || ((STATE) == STATE_ENABLE))

typedef enum {
    STATE_SUCCESS = 0,
    STATE_ERROR = !STATE_SUCCESS
} ErrorStatus;

/**
 * @brief  HAL Status structures definition
 */
typedef enum {
    HAL_OK       = 0x00,
    HAL_ERROR    = 0x01,
    HAL_BUSY     = 0x02,
    HAL_TIMEOUT  = 0x03
} HAL_StatusTypeDef;


/**
 * @brief  HAL Lock structures definition
 */
typedef enum {
    HAL_UNLOCKED = 0x00,
    HAL_LOCKED   = 0x01
} HAL_LockTypeDef;

/**
 * @addtogroup Exported_macros
 */
#define SET_BIT(REG, BIT)       ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT)     ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT)      ((REG) & (BIT))
#define CLEAR_REG(REG)          ((REG) = (0x0))
#define WRITE_REG(REG, VAL)     ((REG) = (VAL))
#define READ_REG(REG)           ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))
#define UNUSED(x)               ((void)(x))

#define HAL_MAX_DELAY      0xFFFFFFFFU

#define HAL_IS_BIT_SET(REG, BIT)         (((REG) & (BIT)) == (BIT))
#define HAL_IS_BIT_CLR(REG, BIT)         (((REG) & (BIT)) == 0U)

#define __HAL_RESET_HANDLE_STATE(__HANDLE__) ((__HANDLE__)->State = 0)

#define __HAL_LOCK(__HANDLE__)                 \
    do{                                        \
        if((__HANDLE__)->Lock == HAL_LOCKED) { \
            return HAL_BUSY;                   \
        } else {                               \
            (__HANDLE__)->Lock = HAL_LOCKED;   \
        }                                      \
    }while (0)

#define __HAL_UNLOCK(__HANDLE__)               \
    do{                                        \
        (__HANDLE__)->Lock = HAL_UNLOCKED;     \
    }while (0)

#define assert_param(expr) ASSERT((expr))

#endif

