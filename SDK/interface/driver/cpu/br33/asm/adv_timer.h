#ifndef _ADV_TIMER_
#define _ADV_TIMER_

#include "typedef.h"
#include "asm/hal_def.h"

#define  USE_HAL_TIM_REGISTER_CALLBACKS     0 /* TIM register callback disabled */

/* Exported types ------------------------------------------------------------*/
/** @defgroup TIM_Exported_Types TIM Exported Types
  * @{
  */

/**
  * @brief  TIM Time base Configuration Structure definition
  */
typedef struct {
    u32 DBA;
    u32 OP;
    u32 DmaRoundEn;
    u32 DBL;
    u32 ADR;
    u32 DOL;
} TIM_DMA_InitTypeDef;

/**
  * @brief  TIM Time base Configuration Structure definition
  */
typedef struct {
    u32 Prescaler;         /*!< Specifies the prescaler value used to divide the TIM clock.
                                   This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF */

    u32 CounterMode;       /*!< Specifies the counter mode.
                                   This parameter can be a value of @ref TIM_Counter_Mode */

    u32 Period;            /*!< Specifies the period value to be loaded into the active
                                   Auto-Reload Register at the next update event.
                                   This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF.  */

    u32 ClockDivision;     /*!< Specifies the clock division.
                                   This parameter can be a value of @ref TIM_ClockDivision */

    u32 RepetitionCounter;  /*!< Specifies the repetition counter value. Each time the RCR downcounter
                                    reaches zero, an update event is generated and counting restarts
                                    from the RCR value (N).
                                    This means in PWM mode that (N+1) corresponds to:
                                        - the number of PWM periods in edge-aligned mode
                                        - the number of half PWM period in center-aligned mode
                                     GP timers: this parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF.
                                     Advanced timers: this parameter must be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF. */

    u32 AutoReloadPreload;  /*!< Specifies the auto-reload preload.
                                   This parameter can be a value of @ref TIM_AutoReloadPreload */
} TIM_Base_InitTypeDef;

/**
  * @brief  TIM Output Compare Configuration Structure definition
  */
typedef struct {
    u32 OCMode;        /*!< Specifies the TIM mode.
                               This parameter can be a value of @ref TIM_Output_Compare_and_PWM_modes */

    u32 Pulse;         /*!< Specifies the pulse value to be loaded into the Capture Compare Register.
                               This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF */

    u32 OCPolarity;    /*!< Specifies the output polarity.
                               This parameter can be a value of @ref TIM_Output_Compare_Polarity */

    u32 OCNPolarity;   /*!< Specifies the complementary output polarity.
                               This parameter can be a value of @ref TIM_Output_Compare_N_Polarity
                               @note This parameter is valid only for timer instances supporting break feature. */

    u32 OCFastMode;    /*!< Specifies the Fast mode state.
                               This parameter can be a value of @ref TIM_Output_Fast_State
                               @note This parameter is valid only in PWM1 and PWM2 mode. */


    u32 OCIdleState;   /*!< Specifies the TIM Output Compare pin state during Idle state.
                               This parameter can be a value of @ref TIM_Output_Compare_Idle_State
                               @note This parameter is valid only for timer instances supporting break feature. */

    u32 OCNIdleState;  /*!< Specifies the TIM Output Compare pin state during Idle state.
                               This parameter can be a value of @ref TIM_Output_Compare_N_Idle_State
                               @note This parameter is valid only for timer instances supporting break feature. */
} TIM_OC_InitTypeDef;

/**
  * @brief  TIM One Pulse Mode Configuration Structure definition
  */
typedef struct {
    u32 OCMode;        /*!< Specifies the TIM mode.
                               This parameter can be a value of @ref TIM_Output_Compare_and_PWM_modes */

    u32 Pulse;         /*!< Specifies the pulse value to be loaded into the Capture Compare Register.
                               This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF */

    u32 OCPolarity;    /*!< Specifies the output polarity.
                               This parameter can be a value of @ref TIM_Output_Compare_Polarity */

    u32 OCNPolarity;   /*!< Specifies the complementary output polarity.
                               This parameter can be a value of @ref TIM_Output_Compare_N_Polarity
                               @note This parameter is valid only for timer instances supporting break feature. */

    u32 OCIdleState;   /*!< Specifies the TIM Output Compare pin state during Idle state.
                               This parameter can be a value of @ref TIM_Output_Compare_Idle_State
                               @note This parameter is valid only for timer instances supporting break feature. */

    u32 OCNIdleState;  /*!< Specifies the TIM Output Compare pin state during Idle state.
                               This parameter can be a value of @ref TIM_Output_Compare_N_Idle_State
                               @note This parameter is valid only for timer instances supporting break feature. */

    u32 ICPolarity;    /*!< Specifies the active edge of the input signal.
                               This parameter can be a value of @ref TIM_Input_Capture_Polarity */

    u32 ICSelection;   /*!< Specifies the input.
                              This parameter can be a value of @ref TIM_Input_Capture_Selection */

    u32 ICFilter;      /*!< Specifies the input capture filter.
                              This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
} TIM_OnePulse_InitTypeDef;

/**
  * @brief  TIM Input Capture Configuration Structure definition
  */
typedef struct {
    u32  ICPolarity;  /*!< Specifies the active edge of the input signal.
                              This parameter can be a value of @ref TIM_Input_Capture_Polarity */

    u32 ICSelection;  /*!< Specifies the input.
                              This parameter can be a value of @ref TIM_Input_Capture_Selection */

    u32 ICPrescaler;  /*!< Specifies the Input Capture Prescaler.
                              This parameter can be a value of @ref TIM_Input_Capture_Prescaler */

    u32 ICFilter;     /*!< Specifies the input capture filter.
                              This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
} TIM_IC_InitTypeDef;

/**
  * @brief  TIM Encoder Configuration Structure definition
  */
typedef struct {
    u32 EncoderMode;   /*!< Specifies the active edge of the input signal.
                               This parameter can be a value of @ref TIM_Encoder_Mode */

    u32 IC1Polarity;   /*!< Specifies the active edge of the input signal.
                               This parameter can be a value of @ref TIM_Encoder_Input_Polarity */

    u32 IC1Selection;  /*!< Specifies the input.
                               This parameter can be a value of @ref TIM_Input_Capture_Selection */

    u32 IC1Prescaler;  /*!< Specifies the Input Capture Prescaler.
                               This parameter can be a value of @ref TIM_Input_Capture_Prescaler */

    u32 IC1Filter;     /*!< Specifies the input capture filter.
                               This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */

    u32 IC2Polarity;   /*!< Specifies the active edge of the input signal.
                               This parameter can be a value of @ref TIM_Encoder_Input_Polarity */

    u32 IC2Selection;  /*!< Specifies the input.
                              This parameter can be a value of @ref TIM_Input_Capture_Selection */

    u32 IC2Prescaler;  /*!< Specifies the Input Capture Prescaler.
                               This parameter can be a value of @ref TIM_Input_Capture_Prescaler */

    u32 IC2Filter;     /*!< Specifies the input capture filter.
                               This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
} TIM_Encoder_InitTypeDef;

/**
  * @brief  Clock Configuration Handle Structure definition
  */
typedef struct {
    u32 ClockSource;     /*!< TIM clock sources
                                 This parameter can be a value of @ref TIM_Clock_Source */
    u32 ClockPolarity;   /*!< TIM clock polarity
                                 This parameter can be a value of @ref TIM_Clock_Polarity */
    u32 ClockPrescaler;  /*!< TIM clock prescaler
                                 This parameter can be a value of @ref TIM_Clock_Prescaler */
    u32 ClockFilter;     /*!< TIM clock filter
                                 This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
} TIM_ClockConfigTypeDef;

/**
  * @brief  TIM Clear Input Configuration Handle Structure definition
  */
typedef struct {
    u32 ClearInputState;      /*!< TIM clear Input state
                                      This parameter can be ENABLE or DISABLE */
    u32 ClearInputSource;     /*!< TIM clear Input sources
                                      This parameter can be a value of @ref TIM_ClearInput_Source */
    u32 ClearInputPolarity;   /*!< TIM Clear Input polarity
                                      This parameter can be a value of @ref TIM_ClearInput_Polarity */
    u32 ClearInputPrescaler;  /*!< TIM Clear Input prescaler
                                      This parameter must be 0: When OCRef clear feature is used with ETR source, ETR prescaler must be off */
    u32 ClearInputFilter;     /*!< TIM Clear Input filter
                                      This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
} TIM_ClearInputConfigTypeDef;

/**
  * @brief  TIM Master configuration Structure definition
  * @note   Advanced timers provide TRGO2 internal line which is redirected
  *         to the ADC
  */
typedef struct {
    u32  MasterOutputTrigger;   /*!< Trigger output (TRGO) selection
                                        This parameter can be a value of @ref TIM_Master_Mode_Selection */
    u32  MasterOutputTrigger2;  /*!< Trigger output2 (TRGO2) selection
                                        This parameter can be a value of @ref TIM_Master_Mode_Selection_2 */
    u32  MasterSlaveMode;       /*!< Master/slave mode selection
                                        This parameter can be a value of @ref TIM_Master_Slave_Mode
                                        @note When the Master/slave mode is enabled, the effect of
                                        an event on the trigger input (TRGI) is delayed to allow a
                                        perfect synchronization between the current timer and its
                                        slaves (through TRGO). It is not mandatory in case of timer
                                        synchronization mode. */
} TIM_MasterConfigTypeDef;

/**
  * @brief  TIM Slave configuration Structure definition
  */
typedef struct {
    u32  SlaveMode;         /*!< Slave mode selection
                                    This parameter can be a value of @ref TIM_Slave_Mode */
    u32  InputTrigger;      /*!< Input Trigger source
                                    This parameter can be a value of @ref TIM_Trigger_Selection */
    u32  TriggerPolarity;   /*!< Input Trigger polarity
                                    This parameter can be a value of @ref TIM_Trigger_Polarity */
    u32  TriggerPrescaler;  /*!< Input trigger prescaler
                                    This parameter can be a value of @ref TIM_Trigger_Prescaler */
    u32  TriggerFilter;     /*!< Input trigger filter
                                    This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF  */

} TIM_SlaveConfigTypeDef;

/**
  * @brief  TIM Break input(s) and Dead time configuration Structure definition
  * @note   2 break inputs can be configured (BKIN and BKIN2) with configurable
  *        filter and polarity.
  */
typedef struct {
    u32 OffStateRunMode;      /*!< TIM off state in run mode
                                      This parameter can be a value of @ref TIM_OSSR_Off_State_Selection_for_Run_mode_state */
    u32 OffStateIDLEMode;     /*!< TIM off state in IDLE mode
                                      This parameter can be a value of @ref TIM_OSSI_Off_State_Selection_for_Idle_mode_state */
    u32 LockLevel;            /*!< TIM Lock level
                                      This parameter can be a value of @ref TIM_Lock_level */
    u32 DeadTime;             /*!< TIM dead Time
                                      This parameter can be a number between Min_Data = 0x00 and Max_Data = 0xFF */
    u32 BreakState;           /*!< TIM Break State
                                      This parameter can be a value of @ref TIM_Break_Input_enable_disable */
    u32 BreakPolarity;        /*!< TIM Break input polarity
                                      This parameter can be a value of @ref TIM_Break_Polarity */
    u32 BreakFilter;          /*!< Specifies the break input filter.
                                      This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
    u32 Break2State;          /*!< TIM Break2 State
                                      This parameter can be a value of @ref TIM_Break2_Input_enable_disable */
    u32 Break2Polarity;       /*!< TIM Break2 input polarity
                                      This parameter can be a value of @ref TIM_Break2_Polarity */
    u32 Break2Filter;         /*!< TIM break2 input filter.
                                      This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */
    u32 AutomaticOutput;      /*!< TIM Automatic Output Enable state
                                      This parameter can be a value of @ref TIM_AOE_Bit_Set_Reset */
} TIM_BreakDeadTimeConfigTypeDef;
/** @defgroup TIMEx_Exported_Types TIM Extended Exported Types
  * @{
  */

/**
  * @brief  TIM Hall sensor Configuration Structure definition
  */

typedef struct {
    uint32_t IC1Polarity;         /*!< Specifies the active edge of the input signal.
                                     This parameter can be a value of @ref TIM_Input_Capture_Polarity */

    uint32_t IC1Prescaler;        /*!< Specifies the Input Capture Prescaler.
                                     This parameter can be a value of @ref TIM_Input_Capture_Prescaler */

    uint32_t IC1Filter;           /*!< Specifies the input capture filter.
                                     This parameter can be a number between Min_Data = 0x0 and Max_Data = 0xF */

    uint32_t Commutation_Delay;   /*!< Specifies the pulse value to be loaded into the Capture Compare Register.
                                     This parameter can be a number between Min_Data = 0x0000 and Max_Data = 0xFFFF */
} TIM_HallSensor_InitTypeDef;


/**
  * @brief  TIM Break/Break2 input configuration
  */
typedef struct {
    uint32_t Source;         /*!< Specifies the source of the timer break input.
                                This parameter can be a value of @ref TIMEx_Break_Input_Source */
    uint32_t Enable;         /*!< Specifies whether or not the break input source is enabled.
                                This parameter can be a value of @ref TIMEx_Break_Input_Source_Enable */
    uint32_t Polarity;       /*!< Specifies the break input source polarity.
                                This parameter can be a value of @ref TIMEx_Break_Input_Source_Polarity
                                Not relevant when analog watchdog output of the DFSDM1 used as break input source */
}
TIMEx_BreakInputConfigTypeDef;
/**
  * @}
  */

/**
  * @brief  HAL State structures definition
  */
typedef enum {
    HAL_TIM_STATE_RESET             = 0x00U,    /*!< Peripheral not yet initialized or disabled  */
    HAL_TIM_STATE_READY             = 0x01U,    /*!< Peripheral Initialized and ready for use    */
    HAL_TIM_STATE_BUSY              = 0x02U,    /*!< An internal process is ongoing              */
    HAL_TIM_STATE_TIMEOUT           = 0x03U,    /*!< Timeout state                               */
    HAL_TIM_STATE_ERROR             = 0x04U     /*!< Reception process is ongoing                */
} HAL_TIM_StateTypeDef;

/**
  * @brief  TIM Channel States definition
  */
typedef enum {
    HAL_TIM_CHANNEL_STATE_RESET             = 0x00U,    /*!< TIM Channel initial state                         */
    HAL_TIM_CHANNEL_STATE_READY             = 0x01U,    /*!< TIM Channel ready for use                         */
    HAL_TIM_CHANNEL_STATE_BUSY              = 0x02U,    /*!< An internal process is ongoing on the TIM channel */
} HAL_TIM_ChannelStateTypeDef;

/**
  * @brief  DMA Burst States definition
  */
typedef enum {
    HAL_DMA_BURST_STATE_RESET             = 0x00U,    /*!< DMA Burst initial state */
    HAL_DMA_BURST_STATE_READY             = 0x01U,    /*!< DMA Burst ready for use */
    HAL_DMA_BURST_STATE_BUSY              = 0x02U,    /*!< Ongoing DMA Burst       */
} HAL_TIM_DMABurstStateTypeDef;

/**
  * @brief  HAL Active channel structures definition
  */
typedef enum {
    HAL_TIM_ACTIVE_CHANNEL_1        = 0x01U,    /*!< The active channel is 1     */
    HAL_TIM_ACTIVE_CHANNEL_2        = 0x02U,    /*!< The active channel is 2     */
    HAL_TIM_ACTIVE_CHANNEL_3        = 0x04U,    /*!< The active channel is 3     */
    HAL_TIM_ACTIVE_CHANNEL_4        = 0x08U,    /*!< The active channel is 4     */
    HAL_TIM_ACTIVE_CHANNEL_5        = 0x10U,    /*!< The active channel is 5     */
    HAL_TIM_ACTIVE_CHANNEL_6        = 0x20U,    /*!< The active channel is 6     */
    HAL_TIM_ACTIVE_CHANNEL_CLEARED  = 0x00U     /*!< All active channels cleared */
} HAL_TIM_ActiveChannel;

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
typedef struct __TIM_HandleTypeDef
#else
typedef struct {
    JL_ADV_TIMER_TypeDef               *Instance;         /*!< Register base address                             */
    TIM_Base_InitTypeDef               Init;              /*!< TIM Time Base required parameters                 */
    HAL_TIM_ActiveChannel              Channel;           /*!< Active channel                                    */
//  DMA_HandleTypeDef                  *hdma[7];          /*!< DMA Handlers array
//                                                             This array is accessed by a @ref DMA_Handle_index */
    HAL_LockTypeDef                    Lock;              /*!< Locking object                                    */
    __IO HAL_TIM_StateTypeDef          State;             /*!< TIM operation state                               */
    __IO HAL_TIM_ChannelStateTypeDef   ChannelState[6];   /*!< TIM channel operation state                       */
    __IO HAL_TIM_ChannelStateTypeDef   ChannelNState[4];  /*!< TIM complementary channel operation state         */
    __IO HAL_TIM_DMABurstStateTypeDef  DMABurstState;     /*!< DMA burst operation state                         */

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
    void (* Base_MspInitCallback)(struct __TIM_HandleTypeDef *htim);              /*!< TIM Base Msp Init Callback                              */
    void (* Base_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);            /*!< TIM Base Msp DeInit Callback                            */
    void (* IC_MspInitCallback)(struct __TIM_HandleTypeDef *htim);                /*!< TIM IC Msp Init Callback                                */
    void (* IC_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);              /*!< TIM IC Msp DeInit Callback                              */
    void (* OC_MspInitCallback)(struct __TIM_HandleTypeDef *htim);                /*!< TIM OC Msp Init Callback                                */
    void (* OC_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);              /*!< TIM OC Msp DeInit Callback                              */
    void (* PWM_MspInitCallback)(struct __TIM_HandleTypeDef *htim);               /*!< TIM PWM Msp Init Callback                               */
    void (* PWM_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);             /*!< TIM PWM Msp DeInit Callback                             */
    void (* OnePulse_MspInitCallback)(struct __TIM_HandleTypeDef *htim);          /*!< TIM One Pulse Msp Init Callback                         */
    void (* OnePulse_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);        /*!< TIM One Pulse Msp DeInit Callback                       */
    void (* Encoder_MspInitCallback)(struct __TIM_HandleTypeDef *htim);           /*!< TIM Encoder Msp Init Callback                           */
    void (* Encoder_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);         /*!< TIM Encoder Msp DeInit Callback                         */
    void (* HallSensor_MspInitCallback)(struct __TIM_HandleTypeDef *htim);        /*!< TIM Hall Sensor Msp Init Callback                       */
    void (* HallSensor_MspDeInitCallback)(struct __TIM_HandleTypeDef *htim);      /*!< TIM Hall Sensor Msp DeInit Callback                     */
    void (* PeriodElapsedCallback)(struct __TIM_HandleTypeDef *htim);             /*!< TIM Period Elapsed Callback                             */
    void (* PeriodElapsedHalfCpltCallback)(struct __TIM_HandleTypeDef *htim);     /*!< TIM Period Elapsed half complete Callback               */
    void (* TriggerCallback)(struct __TIM_HandleTypeDef *htim);                   /*!< TIM Trigger Callback                                    */
    void (* TriggerHalfCpltCallback)(struct __TIM_HandleTypeDef *htim);           /*!< TIM Trigger half complete Callback                      */
    void (* IC_CaptureCallback)(struct __TIM_HandleTypeDef *htim);                /*!< TIM Input Capture Callback                              */
    void (* IC_CaptureHalfCpltCallback)(struct __TIM_HandleTypeDef *htim);        /*!< TIM Input Capture half complete Callback                */
    void (* OC_DelayElapsedCallback)(struct __TIM_HandleTypeDef *htim);           /*!< TIM Output Compare Delay Elapsed Callback               */
    void (* PWM_PulseFinishedCallback)(struct __TIM_HandleTypeDef *htim);         /*!< TIM PWM Pulse Finished Callback                         */
    void (* PWM_PulseFinishedHalfCpltCallback)(struct __TIM_HandleTypeDef *htim); /*!< TIM PWM Pulse Finished half complete Callback           */
    void (* ErrorCallback)(struct __TIM_HandleTypeDef *htim);                     /*!< TIM Error Callback                                      */
    void (* CommutationCallback)(struct __TIM_HandleTypeDef *htim);               /*!< TIM Commutation Callback                                */
    void (* CommutationHalfCpltCallback)(struct __TIM_HandleTypeDef *htim);       /*!< TIM Commutation half complete Callback                  */
    void (* BreakCallback)(struct __TIM_HandleTypeDef *htim);                     /*!< TIM Break Callback                                      */
    void (* Break2Callback)(struct __TIM_HandleTypeDef *htim);                    /*!< TIM Break2 Callback                                     */
#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */
} TIM_HandleTypeDef;
#endif

#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
/**
  * @brief  HAL TIM Callback ID enumeration definition
  */
    typedef enum {
    HAL_TIM_BASE_MSPINIT_CB_ID            = 0x00U    /*!< TIM Base MspInit Callback ID                              */
    , HAL_TIM_BASE_MSPDEINIT_CB_ID          = 0x01U   /*!< TIM Base MspDeInit Callback ID                            */
    , HAL_TIM_IC_MSPINIT_CB_ID              = 0x02U   /*!< TIM IC MspInit Callback ID                                */
    , HAL_TIM_IC_MSPDEINIT_CB_ID            = 0x03U   /*!< TIM IC MspDeInit Callback ID                              */
    , HAL_TIM_OC_MSPINIT_CB_ID              = 0x04U   /*!< TIM OC MspInit Callback ID                                */
    , HAL_TIM_OC_MSPDEINIT_CB_ID            = 0x05U   /*!< TIM OC MspDeInit Callback ID                              */
    , HAL_TIM_PWM_MSPINIT_CB_ID             = 0x06U   /*!< TIM PWM MspInit Callback ID                               */
    , HAL_TIM_PWM_MSPDEINIT_CB_ID           = 0x07U   /*!< TIM PWM MspDeInit Callback ID                             */
    , HAL_TIM_ONE_PULSE_MSPINIT_CB_ID       = 0x08U   /*!< TIM One Pulse MspInit Callback ID                         */
    , HAL_TIM_ONE_PULSE_MSPDEINIT_CB_ID     = 0x09U   /*!< TIM One Pulse MspDeInit Callback ID                       */
    , HAL_TIM_ENCODER_MSPINIT_CB_ID         = 0x0AU   /*!< TIM Encoder MspInit Callback ID                           */
    , HAL_TIM_ENCODER_MSPDEINIT_CB_ID       = 0x0BU   /*!< TIM Encoder MspDeInit Callback ID                         */
    , HAL_TIM_HALL_SENSOR_MSPINIT_CB_ID     = 0x0CU   /*!< TIM Hall Sensor MspDeInit Callback ID                     */
    , HAL_TIM_HALL_SENSOR_MSPDEINIT_CB_ID   = 0x0DU   /*!< TIM Hall Sensor MspDeInit Callback ID                     */
    , HAL_TIM_PERIOD_ELAPSED_CB_ID          = 0x0EU   /*!< TIM Period Elapsed Callback ID                             */
    , HAL_TIM_PERIOD_ELAPSED_HALF_CB_ID     = 0x0FU   /*!< TIM Period Elapsed half complete Callback ID               */
    , HAL_TIM_TRIGGER_CB_ID                 = 0x10U   /*!< TIM Trigger Callback ID                                    */
    , HAL_TIM_TRIGGER_HALF_CB_ID            = 0x11U   /*!< TIM Trigger half complete Callback ID                      */

    , HAL_TIM_IC_CAPTURE_CB_ID              = 0x12U   /*!< TIM Input Capture Callback ID                              */
    , HAL_TIM_IC_CAPTURE_HALF_CB_ID         = 0x13U   /*!< TIM Input Capture half complete Callback ID                */
    , HAL_TIM_OC_DELAY_ELAPSED_CB_ID        = 0x14U   /*!< TIM Output Compare Delay Elapsed Callback ID               */
    , HAL_TIM_PWM_PULSE_FINISHED_CB_ID      = 0x15U   /*!< TIM PWM Pulse Finished Callback ID           */
    , HAL_TIM_PWM_PULSE_FINISHED_HALF_CB_ID = 0x16U   /*!< TIM PWM Pulse Finished half complete Callback ID           */
    , HAL_TIM_ERROR_CB_ID                   = 0x17U   /*!< TIM Error Callback ID                                      */
    , HAL_TIM_COMMUTATION_CB_ID             = 0x18U   /*!< TIM Commutation Callback ID                                */
    , HAL_TIM_COMMUTATION_HALF_CB_ID        = 0x19U   /*!< TIM Commutation half complete Callback ID                  */
    , HAL_TIM_BREAK_CB_ID                   = 0x1AU   /*!< TIM Break Callback ID                                      */
    , HAL_TIM_BREAK2_CB_ID                  = 0x1BU   /*!< TIM Break2 Callback ID                                     */
} HAL_TIM_CallbackIDTypeDef;

/**
  * @brief  HAL TIM Callback pointer definition
  */
typedef  void (*pTIM_CallbackTypeDef)(TIM_HandleTypeDef *htim);  /*!< pointer to the TIM callback function */

#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */

/*******************  Bit definition for TIM_CR1 register  ********************/
#define TIM_CR1_CEN_Pos           (0U)
#define TIM_CR1_CEN_Msk           (0x1UL << TIM_CR1_CEN_Pos)                   /*!< 0x00000001 */
#define TIM_CR1_CEN               TIM_CR1_CEN_Msk                              /*!<Counter enable */
#define TIM_CR1_UDIS_Pos          (1U)
#define TIM_CR1_UDIS_Msk          (0x1UL << TIM_CR1_UDIS_Pos)                  /*!< 0x00000002 */
#define TIM_CR1_UDIS              TIM_CR1_UDIS_Msk                             /*!<Update disable */
#define TIM_CR1_URS_Pos           (2U)
#define TIM_CR1_URS_Msk           (0x1UL << TIM_CR1_URS_Pos)                   /*!< 0x00000004 */
#define TIM_CR1_URS               TIM_CR1_URS_Msk                              /*!<Update request source */
#define TIM_CR1_OPM_Pos           (3U)
#define TIM_CR1_OPM_Msk           (0x1UL << TIM_CR1_OPM_Pos)                   /*!< 0x00000008 */
#define TIM_CR1_OPM               TIM_CR1_OPM_Msk                              /*!<One pulse mode */
#define TIM_CR1_DIR_Pos           (4U)
#define TIM_CR1_DIR_Msk           (0x1UL << TIM_CR1_DIR_Pos)                   /*!< 0x00000010 */
#define TIM_CR1_DIR               TIM_CR1_DIR_Msk                              /*!<Direction */

#define TIM_CR1_CMS_Pos           (5U)
#define TIM_CR1_CMS_Msk           (0x3UL << TIM_CR1_CMS_Pos)                   /*!< 0x00000060 */
#define TIM_CR1_CMS               TIM_CR1_CMS_Msk                              /*!<CMS[1:0] bits (Center-aligned mode selection) */
#define TIM_CR1_CMS_0             (0x1UL << TIM_CR1_CMS_Pos)                    /*!< 0x00000020 */
#define TIM_CR1_CMS_1             (0x2UL << TIM_CR1_CMS_Pos)                    /*!< 0x00000040 */

#define TIM_CR1_ARPE_Pos          (7U)
#define TIM_CR1_ARPE_Msk          (0x1UL << TIM_CR1_ARPE_Pos)                  /*!< 0x00000080 */
#define TIM_CR1_ARPE              TIM_CR1_ARPE_Msk                             /*!<Auto-reload preload enable */

#define TIM_CR1_CKD_Pos           (8U)
#define TIM_CR1_CKD_Msk           (0x3UL << TIM_CR1_CKD_Pos)                   /*!< 0x00000300 */
#define TIM_CR1_CKD               TIM_CR1_CKD_Msk                              /*!<CKD[1:0] bits (clock division) */
#define TIM_CR1_CKD_0             (0x1UL << TIM_CR1_CKD_Pos)                    /*!< 0x00000100 */
#define TIM_CR1_CKD_1             (0x2UL << TIM_CR1_CKD_Pos)                    /*!< 0x00000200 */

#define TIM_CR1_UIFREMAP_Pos      (11U)
#define TIM_CR1_UIFREMAP_Msk      (0x1UL << TIM_CR1_UIFREMAP_Pos)              /*!< 0x00000800 */
#define TIM_CR1_UIFREMAP          TIM_CR1_UIFREMAP_Msk                         /*!<Update interrupt flag remap */

/*******************  Bit definition for TIM_CR2 register  ********************/
#define TIM_CR2_CCPC_Pos          (0U)
#define TIM_CR2_CCPC_Msk          (0x1UL << TIM_CR2_CCPC_Pos)                  /*!< 0x00000001 */
#define TIM_CR2_CCPC              TIM_CR2_CCPC_Msk                             /*!<Capture/Compare Preloaded Control */
#define TIM_CR2_CCUS_Pos          (2U)
#define TIM_CR2_CCUS_Msk          (0x1UL << TIM_CR2_CCUS_Pos)                  /*!< 0x00000004 */
#define TIM_CR2_CCUS              TIM_CR2_CCUS_Msk                             /*!<Capture/Compare Control Update Selection */
#define TIM_CR2_CCDS_Pos          (3U)
#define TIM_CR2_CCDS_Msk          (0x1UL << TIM_CR2_CCDS_Pos)                  /*!< 0x00000008 */
#define TIM_CR2_CCDS              TIM_CR2_CCDS_Msk                             /*!<Capture/Compare DMA Selection */

#define TIM_CR2_MMS_Pos           (4U)
#define TIM_CR2_MMS_Msk           (0x7UL << TIM_CR2_MMS_Pos)                   /*!< 0x00000070 */
#define TIM_CR2_MMS               TIM_CR2_MMS_Msk                              /*!<MMS[2:0] bits (Master Mode Selection) */
#define TIM_CR2_MMS_0             (0x1UL << TIM_CR2_MMS_Pos)                    /*!< 0x00000010 */
#define TIM_CR2_MMS_1             (0x2UL << TIM_CR2_MMS_Pos)                    /*!< 0x00000020 */
#define TIM_CR2_MMS_2             (0x4UL << TIM_CR2_MMS_Pos)                    /*!< 0x00000040 */

#define TIM_CR2_TI1S_Pos          (7U)
#define TIM_CR2_TI1S_Msk          (0x1UL << TIM_CR2_TI1S_Pos)                  /*!< 0x00000080 */
#define TIM_CR2_TI1S              TIM_CR2_TI1S_Msk                             /*!<TI1 Selection */
#define TIM_CR2_OIS1_Pos          (8U)
#define TIM_CR2_OIS1_Msk          (0x1UL << TIM_CR2_OIS1_Pos)                  /*!< 0x00000100 */
#define TIM_CR2_OIS1              TIM_CR2_OIS1_Msk                             /*!<Output Idle state 1 (OC1 output) */
#define TIM_CR2_OIS1N_Pos         (9U)
#define TIM_CR2_OIS1N_Msk         (0x1UL << TIM_CR2_OIS1N_Pos)                 /*!< 0x00000200 */
#define TIM_CR2_OIS1N             TIM_CR2_OIS1N_Msk                            /*!<Output Idle state 1 (OC1N output) */
#define TIM_CR2_OIS2_Pos          (10U)
#define TIM_CR2_OIS2_Msk          (0x1UL << TIM_CR2_OIS2_Pos)                  /*!< 0x00000400 */
#define TIM_CR2_OIS2              TIM_CR2_OIS2_Msk                             /*!<Output Idle state 2 (OC2 output) */
#define TIM_CR2_OIS2N_Pos         (11U)
#define TIM_CR2_OIS2N_Msk         (0x1UL << TIM_CR2_OIS2N_Pos)                 /*!< 0x00000800 */
#define TIM_CR2_OIS2N             TIM_CR2_OIS2N_Msk                            /*!<Output Idle state 2 (OC2N output) */
#define TIM_CR2_OIS3_Pos          (12U)
#define TIM_CR2_OIS3_Msk          (0x1UL << TIM_CR2_OIS3_Pos)                  /*!< 0x00001000 */
#define TIM_CR2_OIS3              TIM_CR2_OIS3_Msk                             /*!<Output Idle state 3 (OC3 output) */
#define TIM_CR2_OIS3N_Pos         (13U)
#define TIM_CR2_OIS3N_Msk         (0x1UL << TIM_CR2_OIS3N_Pos)                 /*!< 0x00002000 */
#define TIM_CR2_OIS3N             TIM_CR2_OIS3N_Msk                            /*!<Output Idle state 3 (OC3N output) */
#define TIM_CR2_OIS4_Pos          (14U)
#define TIM_CR2_OIS4_Msk          (0x1UL << TIM_CR2_OIS4_Pos)                  /*!< 0x00004000 */
#define TIM_CR2_OIS4              TIM_CR2_OIS4_Msk                             /*!<Output Idle state 4 (OC4 output) */
#define TIM_CR2_OIS4N_Pos         (15U)
#define TIM_CR2_OIS4N_Msk         (0x1UL << TIM_CR2_OIS4N_Pos)                 /*!< 0x00002000 */
#define TIM_CR2_OIS4N             TIM_CR2_OIS4N_Msk
#define TIM_CR2_OIS5_Pos          (16U)
#define TIM_CR2_OIS5_Msk          (0x1UL << TIM_CR2_OIS5_Pos)                  /*!< 0x00010000 */
#define TIM_CR2_OIS5              TIM_CR2_OIS5_Msk                             /*!<Output Idle state 5 (OC5 output) */
#define TIM_CR2_OIS6_Pos          (18U)
#define TIM_CR2_OIS6_Msk          (0x1UL << TIM_CR2_OIS6_Pos)                  /*!< 0x00020000 */
#define TIM_CR2_OIS6              TIM_CR2_OIS6_Msk                             /*!<Output Idle state 6 (OC6 output) */

#define TIM_CR2_MMS2_Pos          (20U)
#define TIM_CR2_MMS2_Msk          (0xFUL << TIM_CR2_MMS2_Pos)                  /*!< 0x00F00000 */
#define TIM_CR2_MMS2              TIM_CR2_MMS2_Msk                             /*!<MMS[2:0] bits (Master Mode Selection) */
#define TIM_CR2_MMS2_0            (0x1UL << TIM_CR2_MMS2_Pos)                   /*!< 0x00100000 */
#define TIM_CR2_MMS2_1            (0x2UL << TIM_CR2_MMS2_Pos)                   /*!< 0x00200000 */
#define TIM_CR2_MMS2_2            (0x4UL << TIM_CR2_MMS2_Pos)                   /*!< 0x00400000 */
#define TIM_CR2_MMS2_3            (0x8UL << TIM_CR2_MMS2_Pos)                   /*!< 0x00800000 */

/*******************  Bit definition for TIM_SMCR register  *******************/
#define TIM_SMCR_SMS_Pos          (0U)
//#define TIM_SMCR_SMS_Msk          (0x10007UL << TIM_SMCR_SMS_Pos)              /*!< 0x00010007 */
#define TIM_SMCR_SMS_Msk          (0x000fUL << TIM_SMCR_SMS_Pos)              /*!< 0x0000000f */
#define TIM_SMCR_SMS              TIM_SMCR_SMS_Msk                             /*!<SMS[2:0] bits (Slave mode selection) */
#define TIM_SMCR_SMS_0            (0x0001UL << TIM_SMCR_SMS_Pos)               /*!< 0x00000001 */
#define TIM_SMCR_SMS_1            (0x0002UL << TIM_SMCR_SMS_Pos)               /*!< 0x00000002 */
#define TIM_SMCR_SMS_2            (0x0004UL << TIM_SMCR_SMS_Pos)               /*!< 0x00000004 */
#define TIM_SMCR_SMS_3            (0x0008UL << TIM_SMCR_SMS_Pos)               /*!< 0x00000008 */
//#define TIM_SMCR_SMS_3            (0x10000UL << TIM_SMCR_SMS_Pos)               /*!< 0x00010000 */

#define TIM_SMCR_TS_Pos           (4U)
//#define TIM_SMCR_TS_Msk           (0x30007UL << TIM_SMCR_TS_Pos)               /*!< 0x00300070 */
#define TIM_SMCR_TS_Msk           (0x0007UL << TIM_SMCR_TS_Pos)               /*!< 0x00000070 */
#define TIM_SMCR_TS               TIM_SMCR_TS_Msk                              /*!<TS[4:0] bits (Trigger selection) */
#define TIM_SMCR_TS_0             (0x0001UL << TIM_SMCR_TS_Pos)                /*!< 0x00000010 */
#define TIM_SMCR_TS_1             (0x0002UL << TIM_SMCR_TS_Pos)                /*!< 0x00000020 */
#define TIM_SMCR_TS_2             (0x0004UL << TIM_SMCR_TS_Pos)                /*!< 0x00000040 */
//#define TIM_SMCR_TS_3             (0x10000UL << TIM_SMCR_TS_Pos)                /*!< 0x00100000 */
//#define TIM_SMCR_TS_4             (0x20000UL << TIM_SMCR_TS_Pos)                /*!< 0x00200000 */

#define TIM_SMCR_MSM_Pos          (7U)
#define TIM_SMCR_MSM_Msk          (0x1UL << TIM_SMCR_MSM_Pos)                  /*!< 0x00000080 */
#define TIM_SMCR_MSM              TIM_SMCR_MSM_Msk                             /*!<Master/slave mode */

#define TIM_SMCR_ETF_Pos          (8U)
#define TIM_SMCR_ETF_Msk          (0xFUL << TIM_SMCR_ETF_Pos)                  /*!< 0x00000F00 */
#define TIM_SMCR_ETF              TIM_SMCR_ETF_Msk                             /*!<ETF[3:0] bits (External trigger filter) */
#define TIM_SMCR_ETF_0            (0x1UL << TIM_SMCR_ETF_Pos)                   /*!< 0x00000100 */
#define TIM_SMCR_ETF_1            (0x2UL << TIM_SMCR_ETF_Pos)                   /*!< 0x00000200 */
#define TIM_SMCR_ETF_2            (0x4UL << TIM_SMCR_ETF_Pos)                   /*!< 0x00000400 */
#define TIM_SMCR_ETF_3            (0x8UL << TIM_SMCR_ETF_Pos)                   /*!< 0x00000800 */

#define TIM_SMCR_ETPS_Pos         (12U)
#define TIM_SMCR_ETPS_Msk         (0x3UL << TIM_SMCR_ETPS_Pos)                 /*!< 0x00003000 */
#define TIM_SMCR_ETPS             TIM_SMCR_ETPS_Msk                            /*!<ETPS[1:0] bits (External trigger prescaler) */
#define TIM_SMCR_ETPS_0           (0x1UL << TIM_SMCR_ETPS_Pos)                  /*!< 0x00001000 */
#define TIM_SMCR_ETPS_1           (0x2UL << TIM_SMCR_ETPS_Pos)                  /*!< 0x00002000 */

#define TIM_SMCR_ECE_Pos          (14U)
#define TIM_SMCR_ECE_Msk          (0x1UL << TIM_SMCR_ECE_Pos)                  /*!< 0x00004000 */
#define TIM_SMCR_ECE              TIM_SMCR_ECE_Msk                             /*!<External clock enable */
#define TIM_SMCR_ETP_Pos          (15U)
#define TIM_SMCR_ETP_Msk          (0x1UL << TIM_SMCR_ETP_Pos)                  /*!< 0x00008000 */
#define TIM_SMCR_ETP              TIM_SMCR_ETP_Msk                             /*!<External trigger polarity */

/*******************  Bit definition for TIM_DIER register  *******************/
#define TIM_DIER_UIE_Pos          (0U)
#define TIM_DIER_UIE_Msk          (0x1UL << TIM_DIER_UIE_Pos)                  /*!< 0x00000001 */
#define TIM_DIER_UIE              TIM_DIER_UIE_Msk                             /*!<Update interrupt enable */
#define TIM_DIER_CC1IE_Pos        (1U)
#define TIM_DIER_CC1IE_Msk        (0x1UL << TIM_DIER_CC1IE_Pos)                /*!< 0x00000002 */
#define TIM_DIER_CC1IE            TIM_DIER_CC1IE_Msk                           /*!<Capture/Compare 1 interrupt enable */
#define TIM_DIER_CC2IE_Pos        (2U)
#define TIM_DIER_CC2IE_Msk        (0x1UL << TIM_DIER_CC2IE_Pos)                /*!< 0x00000004 */
#define TIM_DIER_CC2IE            TIM_DIER_CC2IE_Msk                           /*!<Capture/Compare 2 interrupt enable */
#define TIM_DIER_CC3IE_Pos        (3U)
#define TIM_DIER_CC3IE_Msk        (0x1UL << TIM_DIER_CC3IE_Pos)                /*!< 0x00000008 */
#define TIM_DIER_CC3IE            TIM_DIER_CC3IE_Msk                           /*!<Capture/Compare 3 interrupt enable */
#define TIM_DIER_CC4IE_Pos        (4U)
#define TIM_DIER_CC4IE_Msk        (0x1UL << TIM_DIER_CC4IE_Pos)                /*!< 0x00000010 */
#define TIM_DIER_CC4IE            TIM_DIER_CC4IE_Msk                           /*!<Capture/Compare 4 interrupt enable */
#define TIM_DIER_COMIE_Pos        (5U)
#define TIM_DIER_COMIE_Msk        (0x1UL << TIM_DIER_COMIE_Pos)                /*!< 0x00000020 */
#define TIM_DIER_COMIE            TIM_DIER_COMIE_Msk                           /*!<COM interrupt enable */
#define TIM_DIER_TIE_Pos          (6U)
#define TIM_DIER_TIE_Msk          (0x1UL << TIM_DIER_TIE_Pos)                  /*!< 0x00000040 */
#define TIM_DIER_TIE              TIM_DIER_TIE_Msk                             /*!<Trigger interrupt enable */
#define TIM_DIER_BIE_Pos          (7U)
#define TIM_DIER_BIE_Msk          (0x1UL << TIM_DIER_BIE_Pos)                  /*!< 0x00000080 */
#define TIM_DIER_BIE              TIM_DIER_BIE_Msk                             /*!<Break interrupt enable */
#define TIM_DIER_UDE_Pos          (8U)
#define TIM_DIER_UDE_Msk          (0x1UL << TIM_DIER_UDE_Pos)                  /*!< 0x00000100 */
#define TIM_DIER_UDE              TIM_DIER_UDE_Msk                             /*!<Update DMA request enable */
#define TIM_DIER_CC1DE_Pos        (9U)
#define TIM_DIER_CC1DE_Msk        (0x1UL << TIM_DIER_CC1DE_Pos)                /*!< 0x00000200 */
#define TIM_DIER_CC1DE            TIM_DIER_CC1DE_Msk                           /*!<Capture/Compare 1 DMA request enable */
#define TIM_DIER_CC2DE_Pos        (10U)
#define TIM_DIER_CC2DE_Msk        (0x1UL << TIM_DIER_CC2DE_Pos)                /*!< 0x00000400 */
#define TIM_DIER_CC2DE            TIM_DIER_CC2DE_Msk                           /*!<Capture/Compare 2 DMA request enable */
#define TIM_DIER_CC3DE_Pos        (11U)
#define TIM_DIER_CC3DE_Msk        (0x1UL << TIM_DIER_CC3DE_Pos)                /*!< 0x00000800 */
#define TIM_DIER_CC3DE            TIM_DIER_CC3DE_Msk                           /*!<Capture/Compare 3 DMA request enable */
#define TIM_DIER_CC4DE_Pos        (12U)
#define TIM_DIER_CC4DE_Msk        (0x1UL << TIM_DIER_CC4DE_Pos)                /*!< 0x00001000 */
#define TIM_DIER_CC4DE            TIM_DIER_CC4DE_Msk                           /*!<Capture/Compare 4 DMA request enable */
#define TIM_DIER_COMDE_Pos        (13U)
#define TIM_DIER_COMDE_Msk        (0x1UL << TIM_DIER_COMDE_Pos)                /*!< 0x00002000 */
#define TIM_DIER_COMDE            TIM_DIER_COMDE_Msk                           /*!<COM DMA request enable */
#define TIM_DIER_TDE_Pos          (14U)
#define TIM_DIER_TDE_Msk          (0x1UL << TIM_DIER_TDE_Pos)                  /*!< 0x00004000 */
#define TIM_DIER_TDE              TIM_DIER_TDE_Msk                             /*!<Trigger DMA request enable */


/********************  Bit definition for TIM_SR register  ********************/
#define TIM_SR_UIF_Pos            (0U)
#define TIM_SR_UIF_Msk            (0x1UL << TIM_SR_UIF_Pos)                    /*!< 0x00000001 */
#define TIM_SR_UIF                TIM_SR_UIF_Msk                               /*!<Update interrupt Flag */
#define TIM_SR_CC1IF_Pos          (1U)
#define TIM_SR_CC1IF_Msk          (0x1UL << TIM_SR_CC1IF_Pos)                  /*!< 0x00000002 */
#define TIM_SR_CC1IF              TIM_SR_CC1IF_Msk                             /*!<Capture/Compare 1 interrupt Flag */
#define TIM_SR_CC2IF_Pos          (2U)
#define TIM_SR_CC2IF_Msk          (0x1UL << TIM_SR_CC2IF_Pos)                  /*!< 0x00000004 */
#define TIM_SR_CC2IF              TIM_SR_CC2IF_Msk                             /*!<Capture/Compare 2 interrupt Flag */
#define TIM_SR_CC3IF_Pos          (3U)
#define TIM_SR_CC3IF_Msk          (0x1UL << TIM_SR_CC3IF_Pos)                  /*!< 0x00000008 */
#define TIM_SR_CC3IF              TIM_SR_CC3IF_Msk                             /*!<Capture/Compare 3 interrupt Flag */
#define TIM_SR_CC4IF_Pos          (4U)
#define TIM_SR_CC4IF_Msk          (0x1UL << TIM_SR_CC4IF_Pos)                  /*!< 0x00000010 */
#define TIM_SR_CC4IF              TIM_SR_CC4IF_Msk                             /*!<Capture/Compare 4 interrupt Flag */
#define TIM_SR_COMIF_Pos          (5U)
#define TIM_SR_COMIF_Msk          (0x1UL << TIM_SR_COMIF_Pos)                  /*!< 0x00000020 */
#define TIM_SR_COMIF              TIM_SR_COMIF_Msk                             /*!<COM interrupt Flag */
#define TIM_SR_TIF_Pos            (6U)
#define TIM_SR_TIF_Msk            (0x1UL << TIM_SR_TIF_Pos)                    /*!< 0x00000040 */
#define TIM_SR_TIF                TIM_SR_TIF_Msk                               /*!<Trigger interrupt Flag */
#define TIM_SR_BIF_Pos            (7U)
#define TIM_SR_BIF_Msk            (0x1UL << TIM_SR_BIF_Pos)                    /*!< 0x00000080 */
#define TIM_SR_BIF                TIM_SR_BIF_Msk                               /*!<Break interrupt Flag */
#define TIM_SR_B2IF_Pos           (8U)
#define TIM_SR_B2IF_Msk           (0x1UL << TIM_SR_B2IF_Pos)                   /*!< 0x00000100 */
#define TIM_SR_B2IF               TIM_SR_B2IF_Msk                              /*!<Break2 interrupt Flag */
#define TIM_SR_CC1OF_Pos          (9U)
#define TIM_SR_CC1OF_Msk          (0x1UL << TIM_SR_CC1OF_Pos)                  /*!< 0x00000200 */
#define TIM_SR_CC1OF              TIM_SR_CC1OF_Msk                             /*!<Capture/Compare 1 Overcapture Flag */
#define TIM_SR_CC2OF_Pos          (10U)
#define TIM_SR_CC2OF_Msk          (0x1UL << TIM_SR_CC2OF_Pos)                  /*!< 0x00000400 */
#define TIM_SR_CC2OF              TIM_SR_CC2OF_Msk                             /*!<Capture/Compare 2 Overcapture Flag */
#define TIM_SR_CC3OF_Pos          (11U)
#define TIM_SR_CC3OF_Msk          (0x1UL << TIM_SR_CC3OF_Pos)                  /*!< 0x00000800 */
#define TIM_SR_CC3OF              TIM_SR_CC3OF_Msk                             /*!<Capture/Compare 3 Overcapture Flag */
#define TIM_SR_CC4OF_Pos          (12U)
#define TIM_SR_CC4OF_Msk          (0x1UL << TIM_SR_CC4OF_Pos)                  /*!< 0x00001000 */
#define TIM_SR_CC4OF              TIM_SR_CC4OF_Msk                             /*!<Capture/Compare 4 Overcapture Flag */
#define TIM_SR_CC5IF_Pos          (16U)
#define TIM_SR_CC5IF_Msk          (0x1UL << TIM_SR_CC5IF_Pos)                  /*!< 0x00010000 */
#define TIM_SR_CC5IF              TIM_SR_CC5IF_Msk                             /*!<Capture/Compare 5 interrupt Flag */
#define TIM_SR_CC6IF_Pos          (17U)
#define TIM_SR_CC6IF_Msk          (0x1UL << TIM_SR_CC6IF_Pos)                  /*!< 0x00020000 */
#define TIM_SR_CC6IF              TIM_SR_CC6IF_Msk                             /*!<Capture/Compare 6 interrupt Flag */
#define TIM_SR_SBIF_Pos           (13U)
#define TIM_SR_SBIF_Msk           (0x1UL << TIM_SR_SBIF_Pos)                   /*!< 0x00002000 */
#define TIM_SR_SBIF               TIM_SR_SBIF_Msk                              /*!< System Break Flag */


/*******************  Bit definition for TIM_EGR register  ********************/
#define TIM_EGR_UG_Pos            (0U)
#define TIM_EGR_UG_Msk            (0x1UL << TIM_EGR_UG_Pos)                    /*!< 0x00000001 */
#define TIM_EGR_UG                TIM_EGR_UG_Msk                               /*!<Update Generation */
#define TIM_EGR_CC1G_Pos          (1U)
#define TIM_EGR_CC1G_Msk          (0x1UL << TIM_EGR_CC1G_Pos)                  /*!< 0x00000002 */
#define TIM_EGR_CC1G              TIM_EGR_CC1G_Msk                             /*!<Capture/Compare 1 Generation */
#define TIM_EGR_CC2G_Pos          (2U)
#define TIM_EGR_CC2G_Msk          (0x1UL << TIM_EGR_CC2G_Pos)                  /*!< 0x00000004 */
#define TIM_EGR_CC2G              TIM_EGR_CC2G_Msk                             /*!<Capture/Compare 2 Generation */
#define TIM_EGR_CC3G_Pos          (3U)
#define TIM_EGR_CC3G_Msk          (0x1UL << TIM_EGR_CC3G_Pos)                  /*!< 0x00000008 */
#define TIM_EGR_CC3G              TIM_EGR_CC3G_Msk                             /*!<Capture/Compare 3 Generation */
#define TIM_EGR_CC4G_Pos          (4U)
#define TIM_EGR_CC4G_Msk          (0x1UL << TIM_EGR_CC4G_Pos)                  /*!< 0x00000010 */
#define TIM_EGR_CC4G              TIM_EGR_CC4G_Msk                             /*!<Capture/Compare 4 Generation */
#define TIM_EGR_COMG_Pos          (5U)
#define TIM_EGR_COMG_Msk          (0x1UL << TIM_EGR_COMG_Pos)                  /*!< 0x00000020 */
#define TIM_EGR_COMG              TIM_EGR_COMG_Msk                             /*!<Capture/Compare Control Update Generation */
#define TIM_EGR_TG_Pos            (6U)
#define TIM_EGR_TG_Msk            (0x1UL << TIM_EGR_TG_Pos)                    /*!< 0x00000040 */
#define TIM_EGR_TG                TIM_EGR_TG_Msk                               /*!<Trigger Generation */
#define TIM_EGR_BG_Pos            (7U)
#define TIM_EGR_BG_Msk            (0x1UL << TIM_EGR_BG_Pos)                    /*!< 0x00000080 */
#define TIM_EGR_BG                TIM_EGR_BG_Msk                               /*!<Break Generation */
#define TIM_EGR_B2G_Pos           (8U)
#define TIM_EGR_B2G_Msk           (0x1UL << TIM_EGR_B2G_Pos)                   /*!< 0x00000100 */
#define TIM_EGR_B2G               TIM_EGR_B2G_Msk                              /*!<Break Generation */

/******************  Bit definition for TIM_CCMR1 register  *******************/
#define TIM_CCMR1_CC1S_Pos        (0U)
#define TIM_CCMR1_CC1S_Msk        (0x3UL << TIM_CCMR1_CC1S_Pos)                /*!< 0x00000003 */
#define TIM_CCMR1_CC1S            TIM_CCMR1_CC1S_Msk                           /*!<CC1S[1:0] bits (Capture/Compare 1 Selection) */
#define TIM_CCMR1_CC1S_0          (0x1UL << TIM_CCMR1_CC1S_Pos)                 /*!< 0x00000001 */
#define TIM_CCMR1_CC1S_1          (0x2UL << TIM_CCMR1_CC1S_Pos)                 /*!< 0x00000002 */

#define TIM_CCMR1_OC1FE_Pos       (2U)
#define TIM_CCMR1_OC1FE_Msk       (0x1UL << TIM_CCMR1_OC1FE_Pos)               /*!< 0x00000004 */
#define TIM_CCMR1_OC1FE           TIM_CCMR1_OC1FE_Msk                          /*!<Output Compare 1 Fast enable */
#define TIM_CCMR1_OC1PE_Pos       (3U)
#define TIM_CCMR1_OC1PE_Msk       (0x1UL << TIM_CCMR1_OC1PE_Pos)               /*!< 0x00000008 */
#define TIM_CCMR1_OC1PE           TIM_CCMR1_OC1PE_Msk                          /*!<Output Compare 1 Preload enable */

#define TIM_CCMR1_OC1M_Pos        (4U)
//#define TIM_CCMR1_OC1M_Msk        (0x1007UL << TIM_CCMR1_OC1M_Pos)             /*!< 0x00010070 */
#define TIM_CCMR1_OC1M_Msk        (0x000fUL << TIM_CCMR1_OC1M_Pos)             /*!< 0x000000f0 */
#define TIM_CCMR1_OC1M            TIM_CCMR1_OC1M_Msk                           /*!<OC1M[2:0] bits (Output Compare 1 Mode) */
#define TIM_CCMR1_OC1M_0          (0x0001UL << TIM_CCMR1_OC1M_Pos)              /*!< 0x00000010 */
#define TIM_CCMR1_OC1M_1          (0x0002UL << TIM_CCMR1_OC1M_Pos)              /*!< 0x00000020 */
#define TIM_CCMR1_OC1M_2          (0x0004UL << TIM_CCMR1_OC1M_Pos)              /*!< 0x00000040 */
//#define TIM_CCMR1_OC1M_3          (0x1000UL << TIM_CCMR1_OC1M_Pos)              /*!< 0x00010000 */
#define TIM_CCMR1_OC1M_3          (0x0008UL << TIM_CCMR1_OC1M_Pos)              /*!< 0x00000080 */

#define TIM_CCMR1_OC1CE_Pos       (16U)
#define TIM_CCMR1_OC1CE_Msk       (0x1UL << TIM_CCMR1_OC1CE_Pos)               /*!< 0x00010000 */
#define TIM_CCMR1_OC1CE           TIM_CCMR1_OC1CE_Msk                          /*!<Output Compare 1Clear Enable */

#define TIM_CCMR1_CC2S_Pos        (8U)
#define TIM_CCMR1_CC2S_Msk        (0x3UL << TIM_CCMR1_CC2S_Pos)                /*!< 0x00000300 */
#define TIM_CCMR1_CC2S            TIM_CCMR1_CC2S_Msk                           /*!<CC2S[1:0] bits (Capture/Compare 2 Selection) */
#define TIM_CCMR1_CC2S_0          (0x1UL << TIM_CCMR1_CC2S_Pos)                 /*!< 0x00000100 */
#define TIM_CCMR1_CC2S_1          (0x2UL << TIM_CCMR1_CC2S_Pos)                 /*!< 0x00000200 */

#define TIM_CCMR1_OC2FE_Pos       (10U)
#define TIM_CCMR1_OC2FE_Msk       (0x1UL << TIM_CCMR1_OC2FE_Pos)               /*!< 0x00000400 */
#define TIM_CCMR1_OC2FE           TIM_CCMR1_OC2FE_Msk                          /*!<Output Compare 2 Fast enable */
#define TIM_CCMR1_OC2PE_Pos       (11U)
#define TIM_CCMR1_OC2PE_Msk       (0x1UL << TIM_CCMR1_OC2PE_Pos)               /*!< 0x00000800 */
#define TIM_CCMR1_OC2PE           TIM_CCMR1_OC2PE_Msk                          /*!<Output Compare 2 Preload enable */

#define TIM_CCMR1_OC2M_Pos        (12U)
//#define TIM_CCMR1_OC2M_Msk        (0x1007UL << TIM_CCMR1_OC2M_Pos)             /*!< 0x01007000 */
#define TIM_CCMR1_OC2M_Msk        (0x000fUL << TIM_CCMR1_OC2M_Pos)             /*!< 0x0000f000 */
#define TIM_CCMR1_OC2M            TIM_CCMR1_OC2M_Msk                           /*!<OC2M[2:0] bits (Output Compare 2 Mode) */
#define TIM_CCMR1_OC2M_0          (0x0001UL << TIM_CCMR1_OC2M_Pos)              /*!< 0x00001000 */
#define TIM_CCMR1_OC2M_1          (0x0002UL << TIM_CCMR1_OC2M_Pos)              /*!< 0x00002000 */
#define TIM_CCMR1_OC2M_2          (0x0004UL << TIM_CCMR1_OC2M_Pos)              /*!< 0x00004000 */
//#define TIM_CCMR1_OC2M_3          (0x1000UL << TIM_CCMR1_OC2M_Pos)              /*!< 0x01000000 */
#define TIM_CCMR1_OC2M_3          (0x0008UL << TIM_CCMR1_OC2M_Pos)              /*!< 0x00008000 */

//#define TIM_CCMR1_OC2CE_Pos       (15U)
#define TIM_CCMR1_OC2CE_Pos       (24U)
#define TIM_CCMR1_OC2CE_Msk       (0x1UL << TIM_CCMR1_OC2CE_Pos)               /*!< 0x01000000 */
#define TIM_CCMR1_OC2CE           TIM_CCMR1_OC2CE_Msk                          /*!<Output Compare 2 Clear Enable */

/*----------------------------------------------------------------------------*/
/*input*/
#define TIM_CCMR1_IC1PSC_Pos      (2U)
#define TIM_CCMR1_IC1PSC_Msk      (0x3UL << TIM_CCMR1_IC1PSC_Pos)              /*!< 0x0000000C */
#define TIM_CCMR1_IC1PSC          TIM_CCMR1_IC1PSC_Msk                         /*!<IC1PSC[1:0] bits (Input Capture 1 Prescaler) */
#define TIM_CCMR1_IC1PSC_0        (0x1UL << TIM_CCMR1_IC1PSC_Pos)               /*!< 0x00000004 */
#define TIM_CCMR1_IC1PSC_1        (0x2UL << TIM_CCMR1_IC1PSC_Pos)               /*!< 0x00000008 */

#define TIM_CCMR1_IC1F_Pos        (4U)
#define TIM_CCMR1_IC1F_Msk        (0xFUL << TIM_CCMR1_IC1F_Pos)                /*!< 0x000000F0 */
#define TIM_CCMR1_IC1F            TIM_CCMR1_IC1F_Msk                           /*!<IC1F[3:0] bits (Input Capture 1 Filter) */
#define TIM_CCMR1_IC1F_0          (0x1UL << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR1_IC1F_1          (0x2UL << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR1_IC1F_2          (0x4UL << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000040 */
#define TIM_CCMR1_IC1F_3          (0x8UL << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000080 */

#define TIM_CCMR1_IC2PSC_Pos      (10U)
#define TIM_CCMR1_IC2PSC_Msk      (0x3UL << TIM_CCMR1_IC2PSC_Pos)              /*!< 0x00000C00 */
#define TIM_CCMR1_IC2PSC          TIM_CCMR1_IC2PSC_Msk                         /*!<IC2PSC[1:0] bits (Input Capture 2 Prescaler) */
#define TIM_CCMR1_IC2PSC_0        (0x1UL << TIM_CCMR1_IC2PSC_Pos)               /*!< 0x00000400 */
#define TIM_CCMR1_IC2PSC_1        (0x2UL << TIM_CCMR1_IC2PSC_Pos)               /*!< 0x00000800 */

#define TIM_CCMR1_IC2F_Pos        (12U)
#define TIM_CCMR1_IC2F_Msk        (0xFUL << TIM_CCMR1_IC2F_Pos)                /*!< 0x0000F000 */
#define TIM_CCMR1_IC2F            TIM_CCMR1_IC2F_Msk                           /*!<IC2F[3:0] bits (Input Capture 2 Filter) */
#define TIM_CCMR1_IC2F_0          (0x1UL << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR1_IC2F_1          (0x2UL << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR1_IC2F_2          (0x4UL << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00004000 */
#define TIM_CCMR1_IC2F_3          (0x8UL << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00008000 */


/******************  Bit definition for TIM_CCMR2 register  *******************/
#define TIM_CCMR2_CC3S_Pos        (0U)
#define TIM_CCMR2_CC3S_Msk        (0x3UL << TIM_CCMR2_CC3S_Pos)                /*!< 0x00000003 */
#define TIM_CCMR2_CC3S            TIM_CCMR2_CC3S_Msk                           /*!<CC3S[1:0] bits (Capture/Compare 3 Selection) */
#define TIM_CCMR2_CC3S_0          (0x1UL << TIM_CCMR2_CC3S_Pos)                 /*!< 0x00000001 */
#define TIM_CCMR2_CC3S_1          (0x2UL << TIM_CCMR2_CC3S_Pos)                 /*!< 0x00000002 */

#define TIM_CCMR2_OC3FE_Pos       (2U)
#define TIM_CCMR2_OC3FE_Msk       (0x1UL << TIM_CCMR2_OC3FE_Pos)               /*!< 0x00000004 */
#define TIM_CCMR2_OC3FE           TIM_CCMR2_OC3FE_Msk                          /*!<Output Compare 3 Fast enable */
#define TIM_CCMR2_OC3PE_Pos       (3U)
#define TIM_CCMR2_OC3PE_Msk       (0x1UL << TIM_CCMR2_OC3PE_Pos)               /*!< 0x00000008 */
#define TIM_CCMR2_OC3PE           TIM_CCMR2_OC3PE_Msk                          /*!<Output Compare 3 Preload enable */

#define TIM_CCMR2_OC3M_Pos        (4U)
#define TIM_CCMR2_OC3M_Msk        (0xfUL << TIM_CCMR2_OC3M_Pos)                /*!< 0x00000070 */
#define TIM_CCMR2_OC3M            TIM_CCMR2_OC3M_Msk                           /*!<OC3M[2:0] bits (Output Compare 3 Mode) */
#define TIM_CCMR2_OC3M_0          (0x1UL << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR2_OC3M_1          (0x2UL << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR2_OC3M_2          (0x4UL << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000040 */
#define TIM_CCMR2_OC3M_3          (0x8UL << TIM_CCMR2_OC3M_Pos)              /*!< 0x00000080 */

#define TIM_CCMR2_OC3CE_Pos       (16U)
#define TIM_CCMR2_OC3CE_Msk       (0x1UL << TIM_CCMR2_OC3CE_Pos)               /*!< 0x00000080 */
#define TIM_CCMR2_OC3CE           TIM_CCMR2_OC3CE_Msk                          /*!<Output Compare 3 Clear Enable */

#define TIM_CCMR2_CC4S_Pos        (8U)
#define TIM_CCMR2_CC4S_Msk        (0x3UL << TIM_CCMR2_CC4S_Pos)                /*!< 0x00000300 */
#define TIM_CCMR2_CC4S            TIM_CCMR2_CC4S_Msk                           /*!<CC4S[1:0] bits (Capture/Compare 4 Selection) */
#define TIM_CCMR2_CC4S_0          (0x1UL << TIM_CCMR2_CC4S_Pos)                 /*!< 0x00000100 */
#define TIM_CCMR2_CC4S_1          (0x2UL << TIM_CCMR2_CC4S_Pos)                 /*!< 0x00000200 */

#define TIM_CCMR2_OC4FE_Pos       (10U)
#define TIM_CCMR2_OC4FE_Msk       (0x1UL << TIM_CCMR2_OC4FE_Pos)               /*!< 0x00000400 */
#define TIM_CCMR2_OC4FE           TIM_CCMR2_OC4FE_Msk                          /*!<Output Compare 4 Fast enable */
#define TIM_CCMR2_OC4PE_Pos       (11U)
#define TIM_CCMR2_OC4PE_Msk       (0x1UL << TIM_CCMR2_OC4PE_Pos)               /*!< 0x00000800 */
#define TIM_CCMR2_OC4PE           TIM_CCMR2_OC4PE_Msk                          /*!<Output Compare 4 Preload enable */

#define TIM_CCMR2_OC4M_Pos        (12U)
#define TIM_CCMR2_OC4M_Msk        (0xfUL << TIM_CCMR2_OC4M_Pos)                /*!< 0x00007000 */
#define TIM_CCMR2_OC4M            TIM_CCMR2_OC4M_Msk                           /*!<OC4M[2:0] bits (Output Compare 4 Mode) */
#define TIM_CCMR2_OC4M_0          (0x1UL << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR2_OC4M_1          (0x2UL << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR2_OC4M_2          (0x4UL << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00004000 */
#define TIM_CCMR2_OC4M_3          (0x8UL << TIM_CCMR2_OC4M_Pos)               /*!< 0x00008000 */

#define TIM_CCMR2_OC4CE_Pos       (24U)
#define TIM_CCMR2_OC4CE_Msk       (0x1UL << TIM_CCMR2_OC4CE_Pos)               /*!< 0x00008000 */
#define TIM_CCMR2_OC4CE           TIM_CCMR2_OC4CE_Msk                          /*!<Output Compare 4 Clear Enable */

/*----------------------------------------------------------------------------*/
/*input*/
#define TIM_CCMR2_IC3PSC_Pos      (2U)
#define TIM_CCMR2_IC3PSC_Msk      (0x3UL << TIM_CCMR2_IC3PSC_Pos)              /*!< 0x0000000C */
#define TIM_CCMR2_IC3PSC          TIM_CCMR2_IC3PSC_Msk                         /*!<IC3PSC[1:0] bits (Input Capture 3 Prescaler) */
#define TIM_CCMR2_IC3PSC_0        (0x1UL << TIM_CCMR2_IC3PSC_Pos)               /*!< 0x00000004 */
#define TIM_CCMR2_IC3PSC_1        (0x2UL << TIM_CCMR2_IC3PSC_Pos)               /*!< 0x00000008 */

#define TIM_CCMR2_IC3F_Pos        (4U)
#define TIM_CCMR2_IC3F_Msk        (0xFUL << TIM_CCMR2_IC3F_Pos)                /*!< 0x000000F0 */
#define TIM_CCMR2_IC3F            TIM_CCMR2_IC3F_Msk                           /*!<IC3F[3:0] bits (Input Capture 3 Filter) */
#define TIM_CCMR2_IC3F_0          (0x1UL << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR2_IC3F_1          (0x2UL << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR2_IC3F_2          (0x4UL << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000040 */
#define TIM_CCMR2_IC3F_3          (0x8UL << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000080 */

#define TIM_CCMR2_IC4PSC_Pos      (10U)
#define TIM_CCMR2_IC4PSC_Msk      (0x3UL << TIM_CCMR2_IC4PSC_Pos)              /*!< 0x00000C00 */
#define TIM_CCMR2_IC4PSC          TIM_CCMR2_IC4PSC_Msk                         /*!<IC4PSC[1:0] bits (Input Capture 4 Prescaler) */
#define TIM_CCMR2_IC4PSC_0        (0x1UL << TIM_CCMR2_IC4PSC_Pos)               /*!< 0x00000400 */
#define TIM_CCMR2_IC4PSC_1        (0x2UL << TIM_CCMR2_IC4PSC_Pos)               /*!< 0x00000800 */

#define TIM_CCMR2_IC4F_Pos        (12U)
#define TIM_CCMR2_IC4F_Msk        (0xFUL << TIM_CCMR2_IC4F_Pos)                /*!< 0x0000F000 */
#define TIM_CCMR2_IC4F            TIM_CCMR2_IC4F_Msk                           /*!<IC4F[3:0] bits (Input Capture 4 Filter) */
#define TIM_CCMR2_IC4F_0          (0x1UL << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR2_IC4F_1          (0x2UL << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR2_IC4F_2          (0x4UL << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00004000 */
#define TIM_CCMR2_IC4F_3          (0x8UL << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00008000 */


/*******************  Bit definition for TIM_CCER register  *******************/
#define TIM_CCER_CC1E_Pos         (0U)
#define TIM_CCER_CC1E_Msk         (0x1UL << TIM_CCER_CC1E_Pos)                 /*!< 0x00000001 */
#define TIM_CCER_CC1E             TIM_CCER_CC1E_Msk                            /*!<Capture/Compare 1 output enable */
//#define TIM_CCER_CC1P_Pos         (1U)
#define TIM_CCER_CC1P_Pos         (2U)
#define TIM_CCER_CC1P_Msk         (0x1UL << TIM_CCER_CC1P_Pos)                 /*!< 0x00000002 */
#define TIM_CCER_CC1P             TIM_CCER_CC1P_Msk                            /*!<Capture/Compare 1 output Polarity */
//#define TIM_CCER_CC1NE_Pos        (2U)
#define TIM_CCER_CC1NE_Pos        (1U)
#define TIM_CCER_CC1NE_Msk        (0x1UL << TIM_CCER_CC1NE_Pos)                /*!< 0x00000004 */
#define TIM_CCER_CC1NE            TIM_CCER_CC1NE_Msk                           /*!<Capture/Compare 1 Complementary output enable */
#define TIM_CCER_CC1NP_Pos        (3U)
#define TIM_CCER_CC1NP_Msk        (0x1UL << TIM_CCER_CC1NP_Pos)                /*!< 0x00000008 */
#define TIM_CCER_CC1NP            TIM_CCER_CC1NP_Msk                           /*!<Capture/Compare 1 Complementary output Polarity */
#define TIM_CCER_CC2E_Pos         (4U)
#define TIM_CCER_CC2E_Msk         (0x1UL << TIM_CCER_CC2E_Pos)                 /*!< 0x00000010 */
#define TIM_CCER_CC2E             TIM_CCER_CC2E_Msk                            /*!<Capture/Compare 2 output enable */
//#define TIM_CCER_CC2P_Pos         (5U)
#define TIM_CCER_CC2P_Pos         (6U)
#define TIM_CCER_CC2P_Msk         (0x1UL << TIM_CCER_CC2P_Pos)                 /*!< 0x00000020 */
#define TIM_CCER_CC2P             TIM_CCER_CC2P_Msk                            /*!<Capture/Compare 2 output Polarity */
#define TIM_CCER_CC2NE_Pos        (5U)
#define TIM_CCER_CC2NE_Msk        (0x1UL << TIM_CCER_CC2NE_Pos)                /*!< 0x00000040 */
#define TIM_CCER_CC2NE            TIM_CCER_CC2NE_Msk                           /*!<Capture/Compare 2 Complementary output enable */
#define TIM_CCER_CC2NP_Pos        (7U)
#define TIM_CCER_CC2NP_Msk        (0x1UL << TIM_CCER_CC2NP_Pos)                /*!< 0x00000080 */
#define TIM_CCER_CC2NP            TIM_CCER_CC2NP_Msk                           /*!<Capture/Compare 2 Complementary output Polarity */
#define TIM_CCER_CC3E_Pos         (8U)
#define TIM_CCER_CC3E_Msk         (0x1UL << TIM_CCER_CC3E_Pos)                 /*!< 0x00000100 */
#define TIM_CCER_CC3E             TIM_CCER_CC3E_Msk                            /*!<Capture/Compare 3 output enable */
//#define TIM_CCER_CC3P_Pos         (9U)
#define TIM_CCER_CC3P_Pos         (10U)
#define TIM_CCER_CC3P_Msk         (0x1UL << TIM_CCER_CC3P_Pos)                 /*!< 0x00000200 */
#define TIM_CCER_CC3P             TIM_CCER_CC3P_Msk                            /*!<Capture/Compare 3 output Polarity */
#define TIM_CCER_CC3NE_Pos        (9U)
#define TIM_CCER_CC3NE_Msk        (0x1UL << TIM_CCER_CC3NE_Pos)                /*!< 0x00000400 */
#define TIM_CCER_CC3NE            TIM_CCER_CC3NE_Msk                           /*!<Capture/Compare 3 Complementary output enable */
#define TIM_CCER_CC3NP_Pos        (11U)
#define TIM_CCER_CC3NP_Msk        (0x1UL << TIM_CCER_CC3NP_Pos)                /*!< 0x00000800 */
#define TIM_CCER_CC3NP            TIM_CCER_CC3NP_Msk                           /*!<Capture/Compare 3 Complementary output Polarity */
#define TIM_CCER_CC4E_Pos         (12U)
#define TIM_CCER_CC4E_Msk         (0x1UL << TIM_CCER_CC4E_Pos)                 /*!< 0x00001000 */
#define TIM_CCER_CC4E             TIM_CCER_CC4E_Msk                            /*!<Capture/Compare 4 output enable */
#define TIM_CCER_CC4NE_Pos        (13U)
#define TIM_CCER_CC4NE_Msk        (0x1UL << TIM_CCER_CC3NE_Pos)                /*!< 0x00000400 */
#define TIM_CCER_CC4NE            TIM_CCER_CC4NE_Msk                           /*!<Capture/Compare 3 Complementary output enable */
#define TIM_CCER_CC4P_Pos         (14U)
#define TIM_CCER_CC4P_Msk         (0x1UL << TIM_CCER_CC4P_Pos)                 /*!< 0x00002000 */
#define TIM_CCER_CC4P             TIM_CCER_CC4P_Msk                            /*!<Capture/Compare 4 output Polarity */
#define TIM_CCER_CC4NP_Pos        (15U)
#define TIM_CCER_CC4NP_Msk        (0x1UL << TIM_CCER_CC4NP_Pos)                /*!< 0x00008000 */
#define TIM_CCER_CC4NP            TIM_CCER_CC4NP_Msk                           /*!<Capture/Compare 4 Complementary output Polarity */
#define TIM_CCER_CC5E_Pos         (16U)
#define TIM_CCER_CC5E_Msk         (0x1UL << TIM_CCER_CC5E_Pos)                 /*!< 0x00010000 */
#define TIM_CCER_CC5E             TIM_CCER_CC5E_Msk                            /*!<Capture/Compare 5 output enable */
#define TIM_CCER_CC5P_Pos         (18U)
#define TIM_CCER_CC5P_Msk         (0x1UL << TIM_CCER_CC5P_Pos)                 /*!< 0x00020000 */
#define TIM_CCER_CC5P             TIM_CCER_CC5P_Msk                            /*!<Capture/Compare 5 output Polarity */
#define TIM_CCER_CC6E_Pos         (20U)
#define TIM_CCER_CC6E_Msk         (0x1UL << TIM_CCER_CC6E_Pos)                 /*!< 0x00100000 */
#define TIM_CCER_CC6E             TIM_CCER_CC6E_Msk                            /*!<Capture/Compare 6 output enable */
#define TIM_CCER_CC6P_Pos         (22U)
#define TIM_CCER_CC6P_Msk         (0x1UL << TIM_CCER_CC6P_Pos)                 /*!< 0x00200000 */
#define TIM_CCER_CC6P             TIM_CCER_CC6P_Msk                            /*!<Capture/Compare 6 output Polarity */

/*******************  Bit definition for TIM_CNT register  ********************/
#define TIM_CNT_CNT_Pos           (0U)
#define TIM_CNT_CNT_Msk           (0xFFFFUL << TIM_CNT_CNT_Pos)            /*!< 0xFFFFFFFF */
#define TIM_CNT_CNT               TIM_CNT_CNT_Msk                              /*!<Counter Value */
#define TIM_CNT_UIFCPY_Pos        (31U)
#define TIM_CNT_UIFCPY_Msk        (0x1UL << TIM_CNT_UIFCPY_Pos)                /*!< 0x80000000 */
#define TIM_CNT_UIFCPY            TIM_CNT_UIFCPY_Msk                           /*!<Update interrupt flag copy */

/*******************  Bit definition for TIM_PSC register  ********************/
#define TIM_PSC_PSC_Pos           (0U)
#define TIM_PSC_PSC_Msk           (0xFFFFUL << TIM_PSC_PSC_Pos)                /*!< 0x0000FFFF */
#define TIM_PSC_PSC               TIM_PSC_PSC_Msk                              /*!<Prescaler Value */

/*******************  Bit definition for TIM_ARR register  ********************/
#define TIM_ARR_ARR_Pos           (0U)
#define TIM_ARR_ARR_Msk           (0xFFFFUL << TIM_ARR_ARR_Pos)            /*!< 0x0000FFFF */
#define TIM_ARR_ARR               TIM_ARR_ARR_Msk                              /*!<actual auto-reload Value */

/*******************  Bit definition for TIM_RCR register  ********************/
#define TIM_RCR_REP_Pos           (0U)
#define TIM_RCR_REP_Msk           (0xFFFFUL << TIM_RCR_REP_Pos)                  /*!< 0x0000FFFF */
#define TIM_RCR_REP               TIM_RCR_REP_Msk                              /*!<Repetition Counter Value */

/*******************  Bit definition for TIM_CCR1 register  *******************/
#define TIM_CCR1_CCR1_Pos         (0U)
#define TIM_CCR1_CCR1_Msk         (0xFFFFUL << TIM_CCR1_CCR1_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR1_CCR1             TIM_CCR1_CCR1_Msk                            /*!<Capture/Compare 1 Value */

/*******************  Bit definition for TIM_CCR2 register  *******************/
#define TIM_CCR2_CCR2_Pos         (0U)
#define TIM_CCR2_CCR2_Msk         (0xFFFFUL << TIM_CCR2_CCR2_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR2_CCR2             TIM_CCR2_CCR2_Msk                            /*!<Capture/Compare 2 Value */

/*******************  Bit definition for TIM_CCR3 register  *******************/
#define TIM_CCR3_CCR3_Pos         (0U)
#define TIM_CCR3_CCR3_Msk         (0xFFFFUL << TIM_CCR3_CCR3_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR3_CCR3             TIM_CCR3_CCR3_Msk                            /*!<Capture/Compare 3 Value */

/*******************  Bit definition for TIM_CCR4 register  *******************/
#define TIM_CCR4_CCR4_Pos         (0U)
#define TIM_CCR4_CCR4_Msk         (0xFFFFUL << TIM_CCR4_CCR4_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR4_CCR4             TIM_CCR4_CCR4_Msk                            /*!<Capture/Compare 4 Value */

/*******************  Bit definition for TIM_BDTR register  *******************/
#define TIM_BDTR_DTG_Pos          (0U)
#define TIM_BDTR_DTG_Msk          (0xFFUL << TIM_BDTR_DTG_Pos)                 /*!< 0x000000FF */
#define TIM_BDTR_DTG              TIM_BDTR_DTG_Msk                             /*!<DTG[0:7] bits (Dead-Time Generator set-up) */
#define TIM_BDTR_DTG_0            (0x01UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000001 */
#define TIM_BDTR_DTG_1            (0x02UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000002 */
#define TIM_BDTR_DTG_2            (0x04UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000004 */
#define TIM_BDTR_DTG_3            (0x08UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000008 */
#define TIM_BDTR_DTG_4            (0x10UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000010 */
#define TIM_BDTR_DTG_5            (0x20UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000020 */
#define TIM_BDTR_DTG_6            (0x40UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000040 */
#define TIM_BDTR_DTG_7            (0x80UL << TIM_BDTR_DTG_Pos)                  /*!< 0x00000080 */

#define TIM_BDTR_LOCK_Pos         (8U)
#define TIM_BDTR_LOCK_Msk         (0x3UL << TIM_BDTR_LOCK_Pos)                 /*!< 0x00000300 */
#define TIM_BDTR_LOCK             TIM_BDTR_LOCK_Msk                            /*!<LOCK[1:0] bits (Lock Configuration) */
#define TIM_BDTR_LOCK_0           (0x1UL << TIM_BDTR_LOCK_Pos)                  /*!< 0x00000100 */
#define TIM_BDTR_LOCK_1           (0x2UL << TIM_BDTR_LOCK_Pos)                  /*!< 0x00000200 */

#define TIM_BDTR_OSSI_Pos         (10U)
#define TIM_BDTR_OSSI_Msk         (0x1UL << TIM_BDTR_OSSI_Pos)                 /*!< 0x00000400 */
#define TIM_BDTR_OSSI             TIM_BDTR_OSSI_Msk                            /*!<Off-State Selection for Idle mode */
#define TIM_BDTR_OSSR_Pos         (11U)
#define TIM_BDTR_OSSR_Msk         (0x1UL << TIM_BDTR_OSSR_Pos)                 /*!< 0x00000800 */
#define TIM_BDTR_OSSR             TIM_BDTR_OSSR_Msk                            /*!<Off-State Selection for Run mode */
#define TIM_BDTR_BKE_Pos          (12U)
#define TIM_BDTR_BKE_Msk          (0x1UL << TIM_BDTR_BKE_Pos)                  /*!< 0x00001000 */
#define TIM_BDTR_BKE              TIM_BDTR_BKE_Msk                             /*!<Break enable for Break1 */
#define TIM_BDTR_BKP_Pos          (13U)
#define TIM_BDTR_BKP_Msk          (0x1UL << TIM_BDTR_BKP_Pos)                  /*!< 0x00002000 */
#define TIM_BDTR_BKP              TIM_BDTR_BKP_Msk                             /*!<Break Polarity for Break1 */
#define TIM_BDTR_AOE_Pos          (14U)
#define TIM_BDTR_AOE_Msk          (0x1UL << TIM_BDTR_AOE_Pos)                  /*!< 0x00004000 */
#define TIM_BDTR_AOE              TIM_BDTR_AOE_Msk                             /*!<Automatic Output enable */
#define TIM_BDTR_MOE_Pos          (15U)
#define TIM_BDTR_MOE_Msk          (0x1UL << TIM_BDTR_MOE_Pos)                  /*!< 0x00008000 */
#define TIM_BDTR_MOE              TIM_BDTR_MOE_Msk                             /*!<Main Output enable */

#define TIM_BDTR_BKF_Pos          (16U)
#define TIM_BDTR_BKF_Msk          (0xFUL << TIM_BDTR_BKF_Pos)                  /*!< 0x000F0000 */
#define TIM_BDTR_BKF              TIM_BDTR_BKF_Msk                             /*!<Break Filter for Break1 */
#define TIM_BDTR_BK2F_Pos         (20U)
#define TIM_BDTR_BK2F_Msk         (0xFUL << TIM_BDTR_BK2F_Pos)                 /*!< 0x00F00000 */
#define TIM_BDTR_BK2F             TIM_BDTR_BK2F_Msk                            /*!<Break Filter for Break2 */

#define TIM_BDTR_BK2E_Pos         (24U)
#define TIM_BDTR_BK2E_Msk         (0x1UL << TIM_BDTR_BK2E_Pos)                 /*!< 0x01000000 */
#define TIM_BDTR_BK2E             TIM_BDTR_BK2E_Msk                            /*!<Break enable for Break2 */
#define TIM_BDTR_BK2P_Pos         (25U)
#define TIM_BDTR_BK2P_Msk         (0x1UL << TIM_BDTR_BK2P_Pos)                 /*!< 0x02000000 */
#define TIM_BDTR_BK2P             TIM_BDTR_BK2P_Msk                            /*!<Break Polarity for Break2 */

/*******************  Bit definition for TIM_DCR register  ********************/
#define TIM_DCR_DBA_Pos           (0U)
#define TIM_DCR_DBA_Msk           (0x1FUL << TIM_DCR_DBA_Pos)                  /*!< 0x0000001F */
#define TIM_DCR_DBA               TIM_DCR_DBA_Msk                              /*!<DBA[4:0] bits (DMA Base Address) */

#define TIM_DCR_UNLOCK_Pos        (5U)
#define TIM_DCR_UNLOCK_Msk        (0x1UL << TIM_DCR_UNLOCK_Pos)
#define TIM_DCR_UNLOCK            TIM_DCR_UNLOCK_Msk

#define TIM_DCR_OP_Pos            (6U)
#define TIM_DCR_OP_Msk            (0x1UL << TIM_DCR_OP_Pos)
#define TIM_DCR_OP                TIM_DCR_OP_Msk

#define TIM_DCR_DMA_ROUND_EN_Pos  (7U)
#define TIM_DCR_DMA_ROUND_EN_Msk  (0x1UL << TIM_DCR_DMA_ROUND_EN_Pos)
#define TIM_DCR_DMA_ROUND_EN      TIM_DCR_DMA_ROUND_EN_Msk

#define TIM_DCR_DBL_Pos           (8U)
#define TIM_DCR_DBL_Msk           (0x1FUL << TIM_DCR_DBL_Pos)                  /*!< 0x00001F00 */
#define TIM_DCR_DBL               TIM_DCR_DBL_Msk                              /*!<DBL[4:0] bits (DMA Burst Length) */

#define TIM_DCR_ADR_Pos           (16U)
#define TIM_DCR_ADR_Msk           (0xFFUL << TIM_DCR_ADR_Pos)
#define TIM_DCR_ADR               TIM_DCR_ADR_Msk

#define TIM_DCR_DOL_Pos           (24U)
#define TIM_DCR_DOL_Msk           (0xFFUL << TIM_DCR_DOL_Pos)
#define TIM_DCR_DOL               TIM_DCR_DOL_Msk

/*******************  Bit definition for TIM_DMAR register  *******************/
#define TIM_DMAR_DMAB_Pos         (0U)
#define TIM_DMAR_DMAB_Msk         (0xFFFFFFFFUL << TIM_DMAR_DMAB_Pos)              /*!< 0x0000FFFF */
#define TIM_DMAR_DMAB             TIM_DMAR_DMAB_Msk                            /*!<DMA register for burst accesses */

/******************  Bit definition for TIM_CCMR3 register  *******************/
#define TIM_CCMR3_OC5FE_Pos       (2U)
#define TIM_CCMR3_OC5FE_Msk       (0x1UL << TIM_CCMR3_OC5FE_Pos)               /*!< 0x00000004 */
#define TIM_CCMR3_OC5FE           TIM_CCMR3_OC5FE_Msk                          /*!<Output Compare 5 Fast enable */
#define TIM_CCMR3_OC5PE_Pos       (3U)
#define TIM_CCMR3_OC5PE_Msk       (0x1UL << TIM_CCMR3_OC5PE_Pos)               /*!< 0x00000008 */
#define TIM_CCMR3_OC5PE           TIM_CCMR3_OC5PE_Msk                          /*!<Output Compare 5 Preload enable */

#define TIM_CCMR3_OC5M_Pos        (4U)
#define TIM_CCMR3_OC5M_Msk        (0x7UL << TIM_CCMR3_OC5M_Pos)                /*!< 0x00000070 */
#define TIM_CCMR3_OC5M            TIM_CCMR3_OC5M_Msk                           /*!<OC5M[2:0] bits (Output Compare 5 Mode) */
#define TIM_CCMR3_OC5M_0          (0x1UL << TIM_CCMR3_OC5M_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR3_OC5M_1          (0x2UL << TIM_CCMR3_OC5M_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR3_OC5M_2          (0x4UL << TIM_CCMR3_OC5M_Pos)                 /*!< 0x00000040 */
#define TIM_CCMR3_OC5M_3          (0x8UL << TIM_CCMR3_OC5M_Pos)              /*!< 0x00010000 */

#define TIM_CCMR3_OC5CE_Pos       (16U)
#define TIM_CCMR3_OC5CE_Msk       (0x1UL << TIM_CCMR3_OC5CE_Pos)               /*!< 0x00000080 */
#define TIM_CCMR3_OC5CE           TIM_CCMR3_OC5CE_Msk                          /*!<Output Compare 5 Clear Enable */

#define TIM_CCMR3_OC6FE_Pos       (10U)
#define TIM_CCMR3_OC6FE_Msk       (0x1UL << TIM_CCMR3_OC6FE_Pos)               /*!< 0x00000400 */
#define TIM_CCMR3_OC6FE           TIM_CCMR3_OC6FE_Msk                          /*!<Output Compare 4 Fast enable */
#define TIM_CCMR3_OC6PE_Pos       (11U)
#define TIM_CCMR3_OC6PE_Msk       (0x1UL << TIM_CCMR3_OC6PE_Pos)               /*!< 0x00000800 */
#define TIM_CCMR3_OC6PE           TIM_CCMR3_OC6PE_Msk                          /*!<Output Compare 4 Preload enable */

#define TIM_CCMR3_OC6M_Pos        (12U)
#define TIM_CCMR3_OC6M_Msk        (0x7UL << TIM_CCMR3_OC6M_Pos)                /*!< 0x00007000 */
#define TIM_CCMR3_OC6M            TIM_CCMR3_OC6M_Msk                           /*!<OC4M[2:0] bits (Output Compare 4 Mode) */
#define TIM_CCMR3_OC6M_0          (0x1UL << TIM_CCMR3_OC6M_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR3_OC6M_1          (0x2UL << TIM_CCMR3_OC6M_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR3_OC6M_2          (0x4UL << TIM_CCMR3_OC6M_Pos)                 /*!< 0x00004000 */
#define TIM_CCMR3_OC6M_3          (0x8UL << TIM_CCMR3_OC6M_Pos)               /*!< 0x00100000 */

#define TIM_CCMR3_OC6CE_Pos       (24U)
#define TIM_CCMR3_OC6CE_Msk       (0x1UL << TIM_CCMR3_OC6CE_Pos)               /*!< 0x00008000 */
#define TIM_CCMR3_OC6CE           TIM_CCMR3_OC6CE_Msk                          /*!<Output Compare 4 Clear Enable */

/*******************  Bit definition for TIM_CCR5 register  *******************/
#define TIM_CCR5_CCR5_Pos         (0U)
#define TIM_CCR5_CCR5_Msk         (0x0000FFFFUL << TIM_CCR5_CCR5_Pos)          /*!< 0xFFFFFFFF */
#define TIM_CCR5_CCR5             TIM_CCR5_CCR5_Msk                            /*!<Capture/Compare 5 Value */
#define TIM_CCR5_GC5C1_Pos        (29U)
#define TIM_CCR5_GC5C1_Msk        (0x1UL << TIM_CCR5_GC5C1_Pos)                /*!< 0x20000000 */
#define TIM_CCR5_GC5C1            TIM_CCR5_GC5C1_Msk                           /*!<Group Channel 5 and Channel 1 */
#define TIM_CCR5_GC5C2_Pos        (30U)
#define TIM_CCR5_GC5C2_Msk        (0x1UL << TIM_CCR5_GC5C2_Pos)                /*!< 0x40000000 */
#define TIM_CCR5_GC5C2            TIM_CCR5_GC5C2_Msk                           /*!<Group Channel 5 and Channel 2 */
#define TIM_CCR5_GC5C3_Pos        (31U)
#define TIM_CCR5_GC5C3_Msk        (0x1UL << TIM_CCR5_GC5C3_Pos)                /*!< 0x80000000 */
#define TIM_CCR5_GC5C3            TIM_CCR5_GC5C3_Msk                           /*!<Group Channel 5 and Channel 3 */

/*******************  Bit definition for TIM_CCR6 register  *******************/
#define TIM_CCR6_CCR6_Pos         (0U)
#define TIM_CCR6_CCR6_Msk         (0xFFFFUL << TIM_CCR6_CCR6_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR6_CCR6             TIM_CCR6_CCR6_Msk                            /*!<Capture/Compare 6 Value */

/*******************  Bit definition for TIM1_AF1 register  *********************/
#define TIM1_AF1_BKINE_Pos        (0U)
#define TIM1_AF1_BKINE_Msk        (0x1UL << TIM1_AF1_BKINE_Pos)                /*!< 0x00000001 */
#define TIM1_AF1_BKINE            TIM1_AF1_BKINE_Msk                           /*!<BKINE Break input enable bit */
#define TIM1_AF1_BKCMP0E_Pos      (1U)
#define TIM1_AF1_BKCMP0E_Msk      (0x1UL << TIM1_AF1_BKCMP0E_Pos)              /*!< 0x00000002 */
#define TIM1_AF1_BKCMP0E          TIM1_AF1_BKCMP0E_Msk                         /*!<BKCMP0E Break Compare1 Enable bit */
#define TIM1_AF1_BKCMP1E_Pos      (2U)
#define TIM1_AF1_BKCMP1E_Msk      (0x1UL << TIM1_AF1_BKCMP1E_Pos)              /*!< 0x00000004 */
#define TIM1_AF1_BKCMP1E          TIM1_AF1_BKCMP1E_Msk                         /*!<BKCMP1E Break Compare2 Enable bit */
#define TIM1_AF1_BKCMP2E_Pos      (3U)
#define TIM1_AF1_BKCMP2E_Msk      (0x1UL << TIM1_AF1_BKCMP2E_Pos)              /*!< 0x00000008 */
#define TIM1_AF1_BKCMP2E          TIM1_AF1_BKCMP2E_Msk                         /*!<BKCMP2E Break Compare1 Enable bit */
#define TIM1_AF1_BKCMP3E_Pos      (4U)
#define TIM1_AF1_BKCMP3E_Msk      (0x1UL << TIM1_AF1_BKCMP3E_Pos)              /*!< 0x00000010 */
#define TIM1_AF1_BKCMP3E          TIM1_AF1_BKCMP3E_Msk                         /*!<BKCMP3E Break Compare2 Enable bit */
#define TIM1_AF1_BKCMOE_Pos       (5U)
#define TIM1_AF1_BKCMOE_Msk       (0x1UL << TIM1_AF1_BKCMOE_Pos)               /*!< 0x00000020 */
#define TIM1_AF1_BKCMOE           TIM1_AF1_BKCMOE_Msk                          /*!<BKCMOE Break Compare2 Enable bit */
#define TIM1_AF1_BKINP_Pos        (8U)
#define TIM1_AF1_BKINP_Msk        (0x1UL << TIM1_AF1_BKINP_Pos)                /*!< 0x00000100 */
#define TIM1_AF1_BKINP            TIM1_AF1_BKINP_Msk                           /*!<BRKINP Break input polarity */
#define TIM1_AF1_BKCMP0P_Pos      (9U)
#define TIM1_AF1_BKCMP0P_Msk      (0x1UL << TIM1_AF1_BKCMP0P_Pos)              /*!< 0x00000200 */
#define TIM1_AF1_BKCMP0P          TIM1_AF1_BKCMP0P_Msk                         /*!<BKCMP0P Break COMP1 input polarity */
#define TIM1_AF1_BKCMP1P_Pos      (10U)
#define TIM1_AF1_BKCMP1P_Msk      (0x1UL << TIM1_AF1_BKCMP1P_Pos)              /*!< 0x00000400 */
#define TIM1_AF1_BKCMP1P          TIM1_AF1_BKCMP1P_Msk                         /*!<BKCMP1P Break COMP2 input polarity */
#define TIM1_AF1_BKCMP2P_Pos      (11U)
#define TIM1_AF1_BKCMP2P_Msk      (0x1UL << TIM1_AF1_BKCMP2P_Pos)              /*!< 0x00000800 */
#define TIM1_AF1_BKCMP2P          TIM1_AF1_BKCMP2P_Msk                         /*!<BKCMP2P Break COMP1 input polarity */
#define TIM1_AF1_BKCMP3P_Pos      (12U)
#define TIM1_AF1_BKCMP3P_Msk      (0x1UL << TIM1_AF1_BKCMP3P_Pos)              /*!< 0x00001000 */
#define TIM1_AF1_BKCMP3P          TIM1_AF1_BKCMP3P_Msk                         /*!<BKCMP3P Break COMP2 input polarity */
#define TIM1_AF1_BKCMOP_Pos       (13U)
#define TIM1_AF1_BKCMOP_Msk       (0x1UL << TIM1_AF1_BKCMOP_Pos)               /*!< 0x00002000 */
#define TIM1_AF1_BKCMOP           TIM1_AF1_BKCMOP_Msk                          /*!<BKCMOP Break Compare2 Enable bit */
#define TIM1_AF1_ETRSEL_Pos       (16U)
#define TIM1_AF1_ETRSEL_Msk       (0xFUL << TIM1_AF1_ETRSEL_Pos)               /*!< 0x0003C000 */
#define TIM1_AF1_ETRSEL           TIM1_AF1_ETRSEL_Msk                          /*!<ETRSEL[3:0] bits (TIM1 ETRSEL) */
#define TIM1_AF1_ETRSEL_0         (0x1UL << TIM1_AF1_ETRSEL_Pos)                /*!< 0x00008000 */
#define TIM1_AF1_ETRSEL_1         (0x2UL << TIM1_AF1_ETRSEL_Pos)                /*!< 0x00010000 */
#define TIM1_AF1_ETRSEL_2         (0x4UL << TIM1_AF1_ETRSEL_Pos)                /*!< 0x00020000 */

/*******************  Bit definition for TIM1_AF2 register  *********************/
#define TIM1_AF2_BK2INE_Pos        (0U)
#define TIM1_AF2_BK2INE_Msk        (0x1UL << TIM1_AF2_BK2INE_Pos)                /*!< 0x00000001 */
#define TIM1_AF2_BK2INE            TIM1_AF2_BK2INE_Msk                           /*!<BKINE Break input enable bit */
#define TIM1_AF2_BK2CMP0E_Pos      (1U)
#define TIM1_AF2_BK2CMP0E_Msk      (0x1UL << TIM1_AF2_BK2CMP0E_Pos)              /*!< 0x00000002 */
#define TIM1_AF2_BK2CMP0E          TIM1_AF2_BK2CMP0E_Msk                         /*!<BKCMP0E Break Compare1 Enable bit */
#define TIM1_AF2_BK2CMP1E_Pos      (2U)
#define TIM1_AF2_BK2CMP1E_Msk      (0x1UL << TIM1_AF2_BK2CMP1E_Pos)              /*!< 0x00000004 */
#define TIM1_AF2_BK2CMP1E          TIM1_AF2_BK2CMP1E_Msk                         /*!<BKCMP1E Break Compare2 Enable bit */
#define TIM1_AF2_BK2CMP2E_Pos      (3U)
#define TIM1_AF2_BK2CMP2E_Msk      (0x1UL << TIM1_AF2_BK2CMP2E_Pos)              /*!< 0x00000008 */
#define TIM1_AF2_BK2CMP2E          TIM1_AF2_BK2CMP2E_Msk                         /*!<BKCMP2E Break Compare1 Enable bit */
#define TIM1_AF2_BK2CMP3E_Pos      (4U)
#define TIM1_AF2_BK2CMP3E_Msk      (0x1UL << TIM1_AF2_BK2CMP3E_Pos)              /*!< 0x00000010 */
#define TIM1_AF2_BK2CMP3E          TIM1_AF2_BK2CMP3E_Msk                         /*!<BKCMP3E Break Compare2 Enable bit */
#define TIM1_AF2_BK2CMOE_Pos       (5U)
#define TIM1_AF2_BK2CMOE_Msk       (0x1UL << TIM1_AF2_BK2CMOE_Pos)               /*!< 0x00000020 */
#define TIM1_AF2_BK2CMOE           TIM1_AF2_BK2CMOE_Msk                          /*!<BKCMOE Break Compare2 Enable bit */
#define TIM1_AF2_BK2INP_Pos        (8U)
#define TIM1_AF2_BK2INP_Msk        (0x1UL << TIM1_AF2_BK2INP_Pos)                /*!< 0x00000100 */
#define TIM1_AF2_BK2INP            TIM1_AF2_BK2INP_Msk                           /*!<BRKINP Break input polarity */
#define TIM1_AF2_BK2CMP0P_Pos      (9U)
#define TIM1_AF2_BK2CMP0P_Msk      (0x1UL << TIM1_AF2_BK2CMP0P_Pos)              /*!< 0x00000200 */
#define TIM1_AF2_BK2CMP0P          TIM1_AF2_BK2CMP0P_Msk                         /*!<BKCMP0P Break COMP1 input polarity */
#define TIM1_AF2_BK2CMP1P_Pos      (10U)
#define TIM1_AF2_BK2CMP1P_Msk      (0x1UL << TIM1_AF2_BK2CMP1P_Pos)              /*!< 0x00000400 */
#define TIM1_AF2_BK2CMP1P          TIM1_AF2_BK2CMP1P_Msk                         /*!<BKCMP1P Break COMP2 input polarity */
#define TIM1_AF2_BK2CMP2P_Pos      (11U)
#define TIM1_AF2_BK2CMP2P_Msk      (0x1UL << TIM1_AF2_BK2CMP2P_Pos)              /*!< 0x00000800 */
#define TIM1_AF2_BK2CMP2P          TIM1_AF2_BK2CMP2P_Msk                         /*!<BKCMP2P Break COMP1 input polarity */
#define TIM1_AF2_BK2CMP3P_Pos      (12U)
#define TIM1_AF2_BK2CMP3P_Msk      (0x1UL << TIM1_AF2_BK2CMP3P_Pos)              /*!< 0x00001000 */
#define TIM1_AF2_BK2CMP3P          TIM1_AF2_BK2CMP3P_Msk                         /*!<BKCMP3P Break COMP2 input polarity */
#define TIM1_AF2_BK2CMOP_Pos       (13U)
#define TIM1_AF2_BK2CMOP_Msk       (0x1UL << TIM1_AF2_BK2CMOP_Pos)               /*!< 0x00002000 */
#define TIM1_AF2_BK2CMOP           TIM1_AF2_BK2CMOP_Msk                          /*!<BKCMOP Break Compare2 Enable bit */

/*------------------------------------------------------ADV_TIMER CR1 FUNCTION DEFINE----------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/** @defgroup TIM_Exported_Constants TIM Exported Constants
  * @{
  */

/** @defgroup TIM_ClearInput_Source TIM Clear Input Source
  * @{
  */
#define TIM_CLEARINPUTSOURCE_NONE           0x00000000U   /*!< OCREF_CLR is disabled */
#define TIM_CLEARINPUTSOURCE_ETR            0x00000001U   /*!< OCREF_CLR is connected to ETRF input */
/**
  * @}
  */
/** @defgroup TIM_DMA_Base_address TIM DMA Base Address
  * @{
  */
#define TIM_DMABASE_CR1                    0x00000000U
#define TIM_DMABASE_CR2                    0x00000001U
#define TIM_DMABASE_SMCR                   0x00000002U
#define TIM_DMABASE_DIER                   0x00000003U
#define TIM_DMABASE_SR                     0x00000004U
#define TIM_DMABASE_EGR                    0x00000005U
#define TIM_DMABASE_CCMR1                  0x00000006U
#define TIM_DMABASE_CCMR2                  0x00000007U
#define TIM_DMABASE_CCMR3                  0x00000008U
#define TIM_DMABASE_CCER                   0x00000009U
#define TIM_DMABASE_CNT                    0x0000000AU
#define TIM_DMABASE_PSC                    0x0000000BU
#define TIM_DMABASE_ARR                    0x0000000CU
#define TIM_DMABASE_RCR                    0x0000000DU
#define TIM_DMABASE_CCR1                   0x0000000EU
#define TIM_DMABASE_CCR2                   0x0000000FU
#define TIM_DMABASE_CCR3                   0x00000010U
#define TIM_DMABASE_CCR4                   0x00000011U
#define TIM_DMABASE_CCR5                   0x00000012U
#define TIM_DMABASE_CCR6                   0x00000013U
#define TIM_DMABASE_BDTR                   0x00000014U
#define TIM_DMABASE_DCR                    0x00000015U
#define TIM_DMABASE_DMAR                   0x00000016U
#define TIM_DMABASE_AF1                    0x00000017U
#define TIM_DMABASE_AF2                    0x00000018U
/**
  * @}
  */

/** @defgroup TIM_Event_Source TIM Event Source
  * @{
  */
#define TIM_EVENTSOURCE_UPDATE              TIM_EGR_UG     /*!< Reinitialize the counter and generates an update of the registers */
#define TIM_EVENTSOURCE_CC1                 TIM_EGR_CC1G   /*!< A capture/compare event is generated on channel 1 */
#define TIM_EVENTSOURCE_CC2                 TIM_EGR_CC2G   /*!< A capture/compare event is generated on channel 2 */
#define TIM_EVENTSOURCE_CC3                 TIM_EGR_CC3G   /*!< A capture/compare event is generated on channel 3 */
#define TIM_EVENTSOURCE_CC4                 TIM_EGR_CC4G   /*!< A capture/compare event is generated on channel 4 */
#define TIM_EVENTSOURCE_COM                 TIM_EGR_COMG   /*!< A commutation event is generated */
#define TIM_EVENTSOURCE_TRIGGER             TIM_EGR_TG     /*!< A trigger event is generated */
#define TIM_EVENTSOURCE_BREAK               TIM_EGR_BG     /*!< A break event is generated */
#define TIM_EVENTSOURCE_BREAK2              TIM_EGR_B2G    /*!< A break 2 event is generated */
/**
  * @}
  */

#define TIM_DMA_WRITE                       TIM_DCR_OP
#define TIM_DMA_READ                        0x00000000U

#define TIM_DMA_ROUND_ENABLE                0x00000000U
#define TIM_DMA_ROUND_DISABLE               TIM_DCR_DMA_ROUND_EN

/** @defgroup TIM_Input_Channel_Polarity TIM Input Channel polarity
  * @{
  */
#define  TIM_INPUTCHANNELPOLARITY_RISING      0x00000000U                       /*!< Polarity for TIx source */
#define  TIM_INPUTCHANNELPOLARITY_FALLING     TIM_CCER_CC1P                     /*!< Polarity for TIx source */
#define  TIM_INPUTCHANNELPOLARITY_BOTHEDGE    (TIM_CCER_CC1P | TIM_CCER_CC1NP)  /*!< Polarity for TIx source */
/**
  * @}
  */

/** @defgroup TIM_ETR_Polarity TIM ETR Polarity
  * @{
  */
#define TIM_ETRPOLARITY_INVERTED              TIM_SMCR_ETP                      /*!< Polarity for ETR source */
#define TIM_ETRPOLARITY_NONINVERTED           0x00000000U                       /*!< Polarity for ETR source */
/**
  * @}
  */

/** @defgroup TIM_ETR_Prescaler TIM ETR Prescaler
  * @{
  */
#define TIM_ETRPRESCALER_DIV1                 0x00000000U                       /*!< No prescaler is used */
#define TIM_ETRPRESCALER_DIV2                 TIM_SMCR_ETPS_0                   /*!< ETR input source is divided by 2 */
#define TIM_ETRPRESCALER_DIV4                 TIM_SMCR_ETPS_1                   /*!< ETR input source is divided by 4 */
#define TIM_ETRPRESCALER_DIV8                 TIM_SMCR_ETPS                     /*!< ETR input source is divided by 8 */
/**
  * @}
  */

/** @defgroup TIM_Counter_Mode TIM Counter Mode
  * @{
  */
#define TIM_COUNTERMODE_UP                 0x00000000U                          /*!< Counter used as up-counter   */
#define TIM_COUNTERMODE_DOWN               TIM_CR1_DIR                          /*!< Counter used as down-counter */
#define TIM_COUNTERMODE_CENTERALIGNED1     TIM_CR1_CMS_0                        /*!< Center-aligned mode 1        */
#define TIM_COUNTERMODE_CENTERALIGNED2     TIM_CR1_CMS_1                        /*!< Center-aligned mode 2        */
#define TIM_COUNTERMODE_CENTERALIGNED3     TIM_CR1_CMS                          /*!< Center-aligned mode 3        */
/**
  * @}
  */

/** @defgroup TIM_Update_Interrupt_Flag_Remap TIM Update Interrupt Flag Remap
  * @{
  */
#define TIM_UIFREMAP_DISABLE               0x00000000U                          /*!< Update interrupt flag remap disabled */
#define TIM_UIFREMAP_ENABLE                TIM_CR1_UIFREMAP                     /*!< Update interrupt flag remap enabled */
/**
  * @}
  */

/** @defgroup TIM_ClockDivision TIM Clock Division
  * @{
  */
#define TIM_CLOCKDIVISION_DIV1             0x00000000U                          /*!< Clock division: tDTS=tCK_INT   */
#define TIM_CLOCKDIVISION_DIV2             TIM_CR1_CKD_0                        /*!< Clock division: tDTS=2*tCK_INT */
#define TIM_CLOCKDIVISION_DIV4             TIM_CR1_CKD_1                        /*!< Clock division: tDTS=4*tCK_INT */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_State TIM Output Compare State
  * @{
  */
#define TIM_OUTPUTSTATE_DISABLE            0x00000000U                          /*!< Capture/Compare 1 output disabled */
#define TIM_OUTPUTSTATE_ENABLE             TIM_CCER_CC1E                        /*!< Capture/Compare 1 output enabled */
/**
  * @}
  */


/** @defgroup TIM_AutoReloadPreload TIM Auto-Reload Preload
  * @{
  */
#define TIM_AUTORELOAD_PRELOAD_DISABLE                0x00000000U               /*!< TIMx_ARR register is not buffered */
#define TIM_AUTORELOAD_PRELOAD_ENABLE                 TIM_CR1_ARPE              /*!< TIMx_ARR register is buffered */

/**
  * @}
  */

/** @defgroup TIM_Output_Fast_State TIM Output Fast State
  * @{
  */
#define TIM_OCFAST_DISABLE                 0x00000000U                          /*!< Output Compare fast disable */
#define TIM_OCFAST_ENABLE                  TIM_CCMR1_OC1FE                      /*!< Output Compare fast enable  */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_N_State TIM Complementary Output Compare State
  * @{
  */
#define TIM_OUTPUTNSTATE_DISABLE           0x00000000U                          /*!< OCxN is disabled  */
#define TIM_OUTPUTNSTATE_ENABLE            TIM_CCER_CC1NE                       /*!< OCxN is enabled   */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_Polarity TIM Output Compare Polarity
  * @{
  */
#define TIM_OCPOLARITY_HIGH                0x00000000U                          /*!< Capture/Compare output polarity  */
#define TIM_OCPOLARITY_LOW                 TIM_CCER_CC1P                        /*!< Capture/Compare output polarity  */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_N_Polarity TIM Complementary Output Compare Polarity
  * @{
  */
#define TIM_OCNPOLARITY_HIGH               0x00000000U                          /*!< Capture/Compare complementary output polarity */
#define TIM_OCNPOLARITY_LOW                TIM_CCER_CC1NP                       /*!< Capture/Compare complementary output polarity */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_Idle_State TIM Output Compare Idle State
  * @{
  */
#define TIM_OCIDLESTATE_SET                TIM_CR2_OIS1                         /*!< Output Idle state: OCx=1 when MOE=0 */
#define TIM_OCIDLESTATE_RESET              0x00000000U                          /*!< Output Idle state: OCx=0 when MOE=0 */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_N_Idle_State TIM Complementary Output Compare Idle State
  * @{
  */
#define TIM_OCNIDLESTATE_SET               TIM_CR2_OIS1N                        /*!< Complementary output Idle state: OCxN=1 when MOE=0 */
#define TIM_OCNIDLESTATE_RESET             0x00000000U                          /*!< Complementary output Idle state: OCxN=0 when MOE=0 */
/**
  * @}
  */

/** @defgroup TIM_Input_Capture_Polarity TIM Input Capture Polarity
  * @{
  */
#define  TIM_ICPOLARITY_RISING             TIM_INPUTCHANNELPOLARITY_RISING      /*!< Capture triggered by rising edge on timer input                  */
#define  TIM_ICPOLARITY_FALLING            TIM_INPUTCHANNELPOLARITY_FALLING     /*!< Capture triggered by falling edge on timer input                 */
#define  TIM_ICPOLARITY_BOTHEDGE           TIM_INPUTCHANNELPOLARITY_BOTHEDGE    /*!< Capture triggered by both rising and falling edges on timer input*/
/**
  * @}
  */

/** @defgroup TIM_Encoder_Input_Polarity TIM Encoder Input Polarity
  * @{
  */
#define  TIM_ENCODERINPUTPOLARITY_RISING   TIM_INPUTCHANNELPOLARITY_RISING      /*!< Encoder input with rising edge polarity  */
#define  TIM_ENCODERINPUTPOLARITY_FALLING  TIM_INPUTCHANNELPOLARITY_FALLING     /*!< Encoder input with falling edge polarity */
/**
  * @}
  */

/** @defgroup TIM_Input_Capture_Selection TIM Input Capture Selection
  * @{
  */
#define TIM_ICSELECTION_DIRECTTI           TIM_CCMR1_CC1S_0                     /*!< TIM Input 1, 2, 3 or 4 is selected to be
                                                                                     connected to IC1, IC2, IC3 or IC4, respectively */
#define TIM_ICSELECTION_INDIRECTTI         TIM_CCMR1_CC1S_1                     /*!< TIM Input 1, 2, 3 or 4 is selected to be
                                                                                     connected to IC2, IC1, IC4 or IC3, respectively */
#define TIM_ICSELECTION_TRC                TIM_CCMR1_CC1S                       /*!< TIM Input 1, 2, 3 or 4 is selected to be connected to TRC */
/**
  * @}
  */

/** @defgroup TIM_Input_Capture_Prescaler TIM Input Capture Prescaler
  * @{
  */
#define TIM_ICPSC_DIV1                     0x00000000U                          /*!< Capture performed each time an edge is detected on the capture input */
#define TIM_ICPSC_DIV2                     TIM_CCMR1_IC1PSC_0                   /*!< Capture performed once every 2 events                                */
#define TIM_ICPSC_DIV4                     TIM_CCMR1_IC1PSC_1                   /*!< Capture performed once every 4 events                                */
#define TIM_ICPSC_DIV8                     TIM_CCMR1_IC1PSC                     /*!< Capture performed once every 8 events                                */
/**
  * @}
  */

/** @defgroup TIM_One_Pulse_Mode TIM One Pulse Mode
  * @{
  */
#define TIM_OPMODE_SINGLE                  TIM_CR1_OPM                          /*!< Counter stops counting at the next update event */
#define TIM_OPMODE_REPETITIVE              0x00000000U                          /*!< Counter is not stopped at update event          */
/**
  * @}
  */

/** @defgroup TIM_Encoder_Mode TIM Encoder Mode
  * @{
  */
#define TIM_ENCODERMODE_TI1                      TIM_SMCR_SMS_0                                                      /*!< Quadrature encoder mode 1, x2 mode, counts up/down on TI1FP1 edge depending on TI2FP2 level  */
#define TIM_ENCODERMODE_TI2                      TIM_SMCR_SMS_1                                                      /*!< Quadrature encoder mode 2, x2 mode, counts up/down on TI2FP2 edge depending on TI1FP1 level. */
#define TIM_ENCODERMODE_TI12                     (TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0)                                   /*!< Quadrature encoder mode 3, x4 mode, counts up/down on both TI1FP1 and TI2FP2 edges depending on the level of the other input. */
/**
  * @}
  */

/** @defgroup TIM_Interrupt_definition TIM interrupt Definition
  * @{
  */
#define TIM_IT_UPDATE                      TIM_DIER_UIE                         /*!< Update interrupt            */
#define TIM_IT_CC1                         TIM_DIER_CC1IE                       /*!< Capture/Compare 1 interrupt */
#define TIM_IT_CC2                         TIM_DIER_CC2IE                       /*!< Capture/Compare 2 interrupt */
#define TIM_IT_CC3                         TIM_DIER_CC3IE                       /*!< Capture/Compare 3 interrupt */
#define TIM_IT_CC4                         TIM_DIER_CC4IE                       /*!< Capture/Compare 4 interrupt */
#define TIM_IT_COM                         TIM_DIER_COMIE                       /*!< Commutation interrupt       */
#define TIM_IT_TRIGGER                     TIM_DIER_TIE                         /*!< Trigger interrupt           */
#define TIM_IT_BREAK                       TIM_DIER_BIE                         /*!< Break interrupt             */
/**
  * @}
  */

#define TIM_DMA_SEL_UDE                   TIM_CR2_CCDS
#define TIM_DMA_SEL_CCx                   0x00000000U

/** @defgroup TIM_Commutation_Source  TIM Commutation Source
  * @{
  */
#define TIM_COMMUTATION_TRGI              TIM_CR2_CCUS                          /*!< When Capture/compare control bits are preloaded, they are updated by setting the COMG bit or when an rising edge occurs on trigger input */
#define TIM_COMMUTATION_SOFTWARE          0x00000000U                           /*!< When Capture/compare control bits are preloaded, they are updated by setting the COMG bit */
/**
  * @}
  */

/** @defgroup TIM_DMA_sources TIM DMA Sources
  * @{
  */
#define TIM_DMA_UPDATE                     TIM_DIER_UDE                         /*!< DMA request is triggered by the update event */
#define TIM_DMA_CC1                        TIM_DIER_CC1DE                       /*!< DMA request is triggered by the capture/compare macth 1 event */
#define TIM_DMA_CC2                        TIM_DIER_CC2DE                       /*!< DMA request is triggered by the capture/compare macth 2 event event */
#define TIM_DMA_CC3                        TIM_DIER_CC3DE                       /*!< DMA request is triggered by the capture/compare macth 3 event event */
#define TIM_DMA_CC4                        TIM_DIER_CC4DE                       /*!< DMA request is triggered by the capture/compare macth 4 event event */
#define TIM_DMA_COM                        TIM_DIER_COMDE                       /*!< DMA request is triggered by the commutation event */
#define TIM_DMA_TRIGGER                    TIM_DIER_TDE                         /*!< DMA request is triggered by the trigger event */
/**
  * @}
  */

/** @defgroup TIM_Flag_definition TIM Flag Definition
  * @{
  */
#define TIM_FLAG_UPDATE                    TIM_SR_UIF                           /*!< Update interrupt flag         */
#define TIM_FLAG_CC1                       TIM_SR_CC1IF                         /*!< Capture/Compare 1 interrupt flag */
#define TIM_FLAG_CC2                       TIM_SR_CC2IF                         /*!< Capture/Compare 2 interrupt flag */
#define TIM_FLAG_CC3                       TIM_SR_CC3IF                         /*!< Capture/Compare 3 interrupt flag */
#define TIM_FLAG_CC4                       TIM_SR_CC4IF                         /*!< Capture/Compare 4 interrupt flag */
#define TIM_FLAG_CC5                       TIM_SR_CC5IF                         /*!< Capture/Compare 5 interrupt flag */
#define TIM_FLAG_CC6                       TIM_SR_CC6IF                         /*!< Capture/Compare 6 interrupt flag */
#define TIM_FLAG_COM                       TIM_SR_COMIF                         /*!< Commutation interrupt flag    */
#define TIM_FLAG_TRIGGER                   TIM_SR_TIF                           /*!< Trigger interrupt flag        */
#define TIM_FLAG_BREAK                     TIM_SR_BIF                           /*!< Break interrupt flag          */
#define TIM_FLAG_BREAK2                    TIM_SR_B2IF                          /*!< Break 2 interrupt flag        */
#define TIM_FLAG_SYSTEM_BREAK              TIM_SR_SBIF                          /*!< System Break interrupt flag   */
#define TIM_FLAG_CC1OF                     TIM_SR_CC1OF                         /*!< Capture 1 overcapture flag    */
#define TIM_FLAG_CC2OF                     TIM_SR_CC2OF                         /*!< Capture 2 overcapture flag    */
#define TIM_FLAG_CC3OF                     TIM_SR_CC3OF                         /*!< Capture 3 overcapture flag    */
#define TIM_FLAG_CC4OF                     TIM_SR_CC4OF                         /*!< Capture 4 overcapture flag    */
/**
  * @}
  */

/** @defgroup TIM_Channel TIM Channel
  * @{
  */
#define TIM_CHANNEL_1                      0x00000000U                          /*!< Capture/compare channel 1 identifier      */
#define TIM_CHANNEL_2                      0x00000004U                          /*!< Capture/compare channel 2 identifier      */
#define TIM_CHANNEL_3                      0x00000008U                          /*!< Capture/compare channel 3 identifier      */
#define TIM_CHANNEL_4                      0x0000000CU                          /*!< Capture/compare channel 4 identifier      */
#define TIM_CHANNEL_5                      0x00000010U                          /*!< Compare channel 5 identifier              */
#define TIM_CHANNEL_6                      0x00000014U                          /*!< Compare channel 6 identifier              */
#define TIM_CHANNEL_ALL                    0x0000003CU                          /*!< Global Capture/compare channel identifier  */
/**
  * @}
  */

/** @defgroup TIM_Clock_Source TIM Clock Source
  * @{
  */
#define TIM_CLOCKSOURCE_ETRMODE2    TIM_SMCR_ETPS_1      /*!< External clock source mode 2                          */
#define TIM_CLOCKSOURCE_INTERNAL    TIM_SMCR_ETPS_0      /*!< Internal clock source                                 */
#define TIM_CLOCKSOURCE_ITR0        TIM_TS_ITR0          /*!< External clock source mode 1 (ITR0)                   */
#define TIM_CLOCKSOURCE_ITR1        TIM_TS_ITR1          /*!< External clock source mode 1 (ITR1)                   */
#define TIM_CLOCKSOURCE_ITR2        TIM_TS_ITR2          /*!< External clock source mode 1 (ITR2)                   */
#define TIM_CLOCKSOURCE_ITR3        TIM_TS_ITR3          /*!< External clock source mode 1 (ITR3)                   */
#define TIM_CLOCKSOURCE_TI1ED       TIM_TS_TI1F_ED       /*!< External clock source mode 1 (TTI1FP1 + edge detect.) */
#define TIM_CLOCKSOURCE_TI1         TIM_TS_TI1FP1        /*!< External clock source mode 1 (TTI1FP1)                */
#define TIM_CLOCKSOURCE_TI2         TIM_TS_TI2FP2        /*!< External clock source mode 1 (TTI2FP2)                */
#define TIM_CLOCKSOURCE_ETRMODE1    TIM_TS_ETRF          /*!< External clock source mode 1 (ETRF)                   */
#define TIM_CLOCKSOURCE_ITR4        TIM_TS_ITR4          /*!< External clock source mode 1 (ITR4)                   */
#define TIM_CLOCKSOURCE_ITR5        TIM_TS_ITR5          /*!< External clock source mode 1 (ITR5)                   */
#define TIM_CLOCKSOURCE_ITR6        TIM_TS_ITR6          /*!< External clock source mode 1 (ITR6)                   */
#define TIM_CLOCKSOURCE_ITR7        TIM_TS_ITR7          /*!< External clock source mode 1 (ITR7)                   */
#define TIM_CLOCKSOURCE_ITR8        TIM_TS_ITR8          /*!< External clock source mode 1 (ITR8)                   */
/**
  * @}
  */

/** @defgroup TIM_Clock_Polarity TIM Clock Polarity
  * @{
  */
#define TIM_CLOCKPOLARITY_INVERTED           TIM_ETRPOLARITY_INVERTED           /*!< Polarity for ETRx clock sources */
#define TIM_CLOCKPOLARITY_NONINVERTED        TIM_ETRPOLARITY_NONINVERTED        /*!< Polarity for ETRx clock sources */
#define TIM_CLOCKPOLARITY_RISING             TIM_INPUTCHANNELPOLARITY_RISING    /*!< Polarity for TIx clock sources */
#define TIM_CLOCKPOLARITY_FALLING            TIM_INPUTCHANNELPOLARITY_FALLING   /*!< Polarity for TIx clock sources */
#define TIM_CLOCKPOLARITY_BOTHEDGE           TIM_INPUTCHANNELPOLARITY_BOTHEDGE  /*!< Polarity for TIx clock sources */
/**
  * @}
  */

/** @defgroup TIM_Clock_Prescaler TIM Clock Prescaler
  * @{
  */
#define TIM_CLOCKPRESCALER_DIV1                 TIM_ETRPRESCALER_DIV1           /*!< No prescaler is used                                                     */
#define TIM_CLOCKPRESCALER_DIV2                 TIM_ETRPRESCALER_DIV2           /*!< Prescaler for External ETR Clock: Capture performed once every 2 events. */
#define TIM_CLOCKPRESCALER_DIV4                 TIM_ETRPRESCALER_DIV4           /*!< Prescaler for External ETR Clock: Capture performed once every 4 events. */
#define TIM_CLOCKPRESCALER_DIV8                 TIM_ETRPRESCALER_DIV8           /*!< Prescaler for External ETR Clock: Capture performed once every 8 events. */
/**
  * @}
  */

/** @defgroup TIM_ClearInput_Polarity TIM Clear Input Polarity
  * @{
  */
#define TIM_CLEARINPUTPOLARITY_INVERTED           TIM_ETRPOLARITY_INVERTED      /*!< Polarity for ETRx pin */
#define TIM_CLEARINPUTPOLARITY_NONINVERTED        TIM_ETRPOLARITY_NONINVERTED   /*!< Polarity for ETRx pin */
/**
  * @}
  */

/** @defgroup TIM_ClearInput_Prescaler TIM Clear Input Prescaler
  * @{
  */
#define TIM_CLEARINPUTPRESCALER_DIV1              TIM_ETRPRESCALER_DIV1         /*!< No prescaler is used                                                   */
#define TIM_CLEARINPUTPRESCALER_DIV2              TIM_ETRPRESCALER_DIV2         /*!< Prescaler for External ETR pin: Capture performed once every 2 events. */
#define TIM_CLEARINPUTPRESCALER_DIV4              TIM_ETRPRESCALER_DIV4         /*!< Prescaler for External ETR pin: Capture performed once every 4 events. */
#define TIM_CLEARINPUTPRESCALER_DIV8              TIM_ETRPRESCALER_DIV8         /*!< Prescaler for External ETR pin: Capture performed once every 8 events. */
/**
  * @}
  */

/** @defgroup TIM_OSSR_Off_State_Selection_for_Run_mode_state TIM OSSR OffState Selection for Run mode state
  * @{
  */
#define TIM_OSSR_ENABLE                          TIM_BDTR_OSSR                  /*!< When inactive, OC/OCN outputs are enabled (still controlled by the timer)           */
#define TIM_OSSR_DISABLE                         0x00000000U                    /*!< When inactive, OC/OCN outputs are disabled (not controlled any longer by the timer) */
/**
  * @}
  */

/** @defgroup TIM_OSSI_Off_State_Selection_for_Idle_mode_state TIM OSSI OffState Selection for Idle mode state
  * @{
  */
#define TIM_OSSI_ENABLE                          TIM_BDTR_OSSI                  /*!< When inactive, OC/OCN outputs are enabled (still controlled by the timer)           */
#define TIM_OSSI_DISABLE                         0x00000000U                    /*!< When inactive, OC/OCN outputs are disabled (not controlled any longer by the timer) */
/**
  * @}
  */
/** @defgroup TIM_Lock_level  TIM Lock level
  * @{
  */
#define TIM_LOCKLEVEL_OFF                  0x00000000U                          /*!< LOCK OFF     */
#define TIM_LOCKLEVEL_1                    TIM_BDTR_LOCK_0                      /*!< LOCK Level 1 */
#define TIM_LOCKLEVEL_2                    TIM_BDTR_LOCK_1                      /*!< LOCK Level 2 */
#define TIM_LOCKLEVEL_3                    TIM_BDTR_LOCK                        /*!< LOCK Level 3 */
/**
  * @}
  */

/** @defgroup TIM_Break_Input_enable_disable TIM Break Input Enable
  * @{
  */
#define TIM_BREAK_ENABLE                   TIM_BDTR_BKE                         /*!< Break input BRK is enabled  */
#define TIM_BREAK_DISABLE                  0x00000000U                          /*!< Break input BRK is disabled */
/**
  * @}
  */

/** @defgroup TIM_Break_Polarity TIM Break Input Polarity
  * @{
  */
#define TIM_BREAKPOLARITY_HIGH             0x00000000U                          /*!< Break input BRK is active low  */
#define TIM_BREAKPOLARITY_LOW              TIM_BDTR_BKP                         /*!< Break input BRK is active high */
/**
  * @}
  */

/** @defgroup TIM_Break2_Input_enable_disable TIM Break input 2 Enable
  * @{
  */
#define TIM_BREAK2_DISABLE                 0x00000000U                          /*!< Break input BRK2 is disabled  */
#define TIM_BREAK2_ENABLE                  TIM_BDTR_BK2E                        /*!< Break input BRK2 is enabled  */
/**
  * @}
  */

/** @defgroup TIM_Break2_Polarity TIM Break Input 2 Polarity
  * @{
  */
#define TIM_BREAK2POLARITY_LOW             0x00000000U                          /*!< Break input BRK2 is active low   */
#define TIM_BREAK2POLARITY_HIGH            TIM_BDTR_BK2P                        /*!< Break input BRK2 is active high  */
/**
  * @}
  */

/** @defgroup TIM_AOE_Bit_Set_Reset TIM Automatic Output Enable
  * @{
  */
#define TIM_AUTOMATICOUTPUT_DISABLE        0x00000000U                          /*!< MOE can be set only by software */
#define TIM_AUTOMATICOUTPUT_ENABLE         TIM_BDTR_AOE                         /*!< MOE can be set by software or automatically at the next update event
                                                                                    (if none of the break inputs BRK and BRK2 is active) */
/**
  * @}
  */

/** @defgroup TIM_Group_Channel5 Group Channel 5 and Channel 1, 2 or 3
  * @{
  */
#define TIM_GROUPCH5_NONE                  0x00000000U                          /* !< No effect of OC5REF on OC1REFC, OC2REFC and OC3REFC */
#define TIM_GROUPCH5_OC1REFC               TIM_CCR5_GC5C1                       /* !< OC1REFC is the logical AND of OC1REFC and OC5REF    */
#define TIM_GROUPCH5_OC2REFC               TIM_CCR5_GC5C2                       /* !< OC2REFC is the logical AND of OC2REFC and OC5REF    */
#define TIM_GROUPCH5_OC3REFC               TIM_CCR5_GC5C3                       /* !< OC3REFC is the logical AND of OC3REFC and OC5REF    */
/**
  * @}
  */

/** @defgroup TIM_Master_Mode_Selection TIM Master Mode Selection
  * @{
  */
#define TIM_TRGO_RESET            0x00000000U                                      /*!< TIMx_EGR.UG bit is used as trigger output (TRGO)              */
#define TIM_TRGO_ENABLE           TIM_CR2_MMS_0                                    /*!< TIMx_CR1.CEN bit is used as trigger output (TRGO)             */
#define TIM_TRGO_UPDATE           TIM_CR2_MMS_1                                    /*!< Update event is used as trigger output (TRGO)                 */
#define TIM_TRGO_OC1              (TIM_CR2_MMS_1 | TIM_CR2_MMS_0)                  /*!< Capture or a compare match 1 is used as trigger output (TRGO) */
#define TIM_TRGO_OC1REF           TIM_CR2_MMS_2                                    /*!< OC1REF signal is used as trigger output (TRGO)                */
#define TIM_TRGO_OC2REF           (TIM_CR2_MMS_2 | TIM_CR2_MMS_0)                  /*!< OC2REF signal is used as trigger output(TRGO)                 */
#define TIM_TRGO_OC3REF           (TIM_CR2_MMS_2 | TIM_CR2_MMS_1)                  /*!< OC3REF signal is used as trigger output(TRGO)                 */
#define TIM_TRGO_OC4REF           (TIM_CR2_MMS_2 | TIM_CR2_MMS_1 | TIM_CR2_MMS_0)  /*!< OC4REF signal is used as trigger output(TRGO)                 */
/**
  * @}
  */

/** @defgroup TIM_Master_Mode_Selection_2 TIM Master Mode Selection 2 (TRGO2)
  * @{
  */
#define TIM_TRGO2_RESET                          0x00000000U                                                         /*!< TIMx_EGR.UG bit is used as trigger output (TRGO2)              */
#define TIM_TRGO2_ENABLE                         TIM_CR2_MMS2_0                                                      /*!< TIMx_CR1.CEN bit is used as trigger output (TRGO2)             */
#define TIM_TRGO2_UPDATE                         TIM_CR2_MMS2_1                                                      /*!< Update event is used as trigger output (TRGO2)                 */
#define TIM_TRGO2_OC1                            (TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0)                                   /*!< Capture or a compare match 1 is used as trigger output (TRGO2) */
#define TIM_TRGO2_OC1REF                         TIM_CR2_MMS2_2                                                      /*!< OC1REF signal is used as trigger output (TRGO2)                */
#define TIM_TRGO2_OC2REF                         (TIM_CR2_MMS2_2 | TIM_CR2_MMS2_0)                                   /*!< OC2REF signal is used as trigger output (TRGO2)                */
#define TIM_TRGO2_OC3REF                         (TIM_CR2_MMS2_2 | TIM_CR2_MMS2_1)                                   /*!< OC3REF signal is used as trigger output (TRGO2)                */
#define TIM_TRGO2_OC4REF                         (TIM_CR2_MMS2_2 | TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0)                  /*!< OC4REF signal is used as trigger output (TRGO2)                */
#define TIM_TRGO2_OC5REF                         TIM_CR2_MMS2_3                                                      /*!< OC5REF signal is used as trigger output (TRGO2)                */
#define TIM_TRGO2_OC6REF                         (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_0)                                   /*!< OC6REF signal is used as trigger output (TRGO2)                */
#define TIM_TRGO2_OC4REF_RISINGFALLING           (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_1)                                   /*!< OC4REF rising or falling edges generate pulses on TRGO2        */
#define TIM_TRGO2_OC6REF_RISINGFALLING           (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0)                  /*!< OC6REF rising or falling edges generate pulses on TRGO2        */
#define TIM_TRGO2_OC4REF_RISING_OC6REF_RISING    (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2)                                   /*!< OC4REF or OC6REF rising edges generate pulses on TRGO2         */
#define TIM_TRGO2_OC4REF_RISING_OC6REF_FALLING   (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2 | TIM_CR2_MMS2_0)                  /*!< OC4REF rising or OC6REF falling edges generate pulses on TRGO2 */
#define TIM_TRGO2_OC5REF_RISING_OC6REF_RISING    (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2 |TIM_CR2_MMS2_1)                   /*!< OC5REF or OC6REF rising edges generate pulses on TRGO2         */
#define TIM_TRGO2_OC5REF_RISING_OC6REF_FALLING   (TIM_CR2_MMS2_3 | TIM_CR2_MMS2_2 | TIM_CR2_MMS2_1 | TIM_CR2_MMS2_0) /*!< OC5REF or OC6REF rising edges generate pulses on TRGO2         */
/**
  * @}
  */

/** @defgroup TIM_Master_Slave_Mode TIM Master/Slave Mode
  * @{
  */
#define TIM_MASTERSLAVEMODE_ENABLE         TIM_SMCR_MSM                         /*!< No action */
#define TIM_MASTERSLAVEMODE_DISABLE        0x00000000U                          /*!< Master/slave mode is selected */
/**
  * @}
  */

/** @defgroup TIM_Slave_Mode TIM Slave mode
  * @{
  */
#define TIM_SLAVEMODE_DISABLE                0x00000000U                                        /*!< Slave mode disabled           */
#define TIM_SLAVEMODE_RESET                  TIM_SMCR_SMS_2                                     /*!< Reset Mode                    */
#define TIM_SLAVEMODE_GATED                  (TIM_SMCR_SMS_2 | TIM_SMCR_SMS_0)                  /*!< Gated Mode                    */
#define TIM_SLAVEMODE_TRIGGER                (TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1)                  /*!< Trigger Mode                  */
#define TIM_SLAVEMODE_EXTERNAL1              (TIM_SMCR_SMS_2 | TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0) /*!< External Clock Mode 1         */
#define TIM_SLAVEMODE_COMBINED_RESETTRIGGER  TIM_SMCR_SMS_3                                     /*!< Combined reset + trigger mode */
/**
  * @}
  */

/** @defgroup TIM_Output_Compare_and_PWM_modes TIM Output Compare and PWM Modes
  * @{
  */
#define TIM_OCMODE_TIMING                   0x00000000U                                              /*!< Frozen                                 */
#define TIM_OCMODE_ACTIVE                   TIM_CCMR1_OC1M_0                                         /*!< Set channel to active level on match   */
#define TIM_OCMODE_INACTIVE                 TIM_CCMR1_OC1M_1                                         /*!< Set channel to inactive level on match */
#define TIM_OCMODE_TOGGLE                   (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0)                    /*!< Toggle                                 */
#define TIM_OCMODE_PWM1                     (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1)                    /*!< PWM mode 1                             */
#define TIM_OCMODE_PWM2                     (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0) /*!< PWM mode 2                             */
#define TIM_OCMODE_FORCED_ACTIVE            (TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_0)                    /*!< Force active level                     */
#define TIM_OCMODE_FORCED_INACTIVE          TIM_CCMR1_OC1M_2                                         /*!< Force inactive level                   */
#define TIM_OCMODE_RETRIGERRABLE_OPM1      TIM_CCMR1_OC1M_3                                          /*!< Retrigerrable OPM mode 1               */
#define TIM_OCMODE_RETRIGERRABLE_OPM2      (TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0)                     /*!< Retrigerrable OPM mode 2               */
#define TIM_OCMODE_COMBINED_PWM1           (TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_2)                     /*!< Combined PWM mode 1                    */
#define TIM_OCMODE_COMBINED_PWM2           (TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_2)  /*!< Combined PWM mode 2                    */
#define TIM_OCMODE_ASSYMETRIC_PWM1         (TIM_CCMR1_OC1M_3 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2)  /*!< Asymmetric PWM mode 1                  */
#define TIM_OCMODE_ASSYMETRIC_PWM2         TIM_CCMR1_OC1M                                            /*!< Asymmetric PWM mode 2                  */
/**
  * @}
  */

/** @defgroup TIM_Trigger_Selection TIM Trigger Selection
  * @{
  */
#define TIM_TS_ITR0          0x00000000U                                                       /*!< Internal Trigger 0 (ITR0)              */
#define TIM_TS_ITR1          TIM_SMCR_TS_0                                                     /*!< Internal Trigger 1 (ITR1)              */
#define TIM_TS_ITR2          TIM_SMCR_TS_1                                                     /*!< Internal Trigger 2 (ITR2)              */
#define TIM_TS_ITR3          (TIM_SMCR_TS_0 | TIM_SMCR_TS_1)                                   /*!< Internal Trigger 3 (ITR3)              */
#define TIM_TS_TI1F_ED       TIM_SMCR_TS_2                                                     /*!< TI1 Edge Detector (TI1F_ED)            */
#define TIM_TS_TI1FP1        (TIM_SMCR_TS_0 | TIM_SMCR_TS_2)                                   /*!< Filtered Timer Input 1 (TI1FP1)        */
#define TIM_TS_TI2FP2        (TIM_SMCR_TS_1 | TIM_SMCR_TS_2)                                   /*!< Filtered Timer Input 2 (TI2FP2)        */
#define TIM_TS_ETRF          (TIM_SMCR_TS_0 | TIM_SMCR_TS_1 | TIM_SMCR_TS_2)                   /*!< Filtered External Trigger input (ETRF) */
//#define TIM_TS_ITR4          (TIM_SMCR_TS_3)                                                   /*!< Internal Trigger 4 (ITR4)              */
//#define TIM_TS_ITR5          (TIM_SMCR_TS_0 | TIM_SMCR_TS_3)                                   /*!< Internal Trigger 5 (ITR5)              */
//#define TIM_TS_ITR6          (TIM_SMCR_TS_1 | TIM_SMCR_TS_3)                                   /*!< Internal Trigger 6 (ITR6)              */
//#define TIM_TS_ITR7          (TIM_SMCR_TS_0 | TIM_SMCR_TS_1 | TIM_SMCR_TS_3)                   /*!< Internal Trigger 7 (ITR7)              */
//#define TIM_TS_ITR8          (TIM_SMCR_TS_2 | TIM_SMCR_TS_3)                                   /*!< Internal Trigger 8 (ITR8)              */
//#define TIM_TS_ITR9          (TIM_SMCR_TS_0 | TIM_SMCR_TS_2 | TIM_SMCR_TS_3)                   /*!< Internal Trigger 9 (ITR9)              */
//#define TIM_TS_ITR10         (TIM_SMCR_TS_1 | TIM_SMCR_TS_2 | TIM_SMCR_TS_3)                   /*!< Internal Trigger 10 (ITR10)            */
//#define TIM_TS_ITR11         (TIM_SMCR_TS_0 | TIM_SMCR_TS_1 | TIM_SMCR_TS_2 | TIM_SMCR_TS_3)   /*!< Internal Trigger 11 (ITR11)            */
//#define TIM_TS_ITR12         (TIM_SMCR_TS_4)                                                   /*!< Internal Trigger 12 (ITR12)            */
//#define TIM_TS_ITR13         (TIM_SMCR_TS_0 | TIM_SMCR_TS_4)                                   /*!< Internal Trigger 13 (ITR13)            */
#define TIM_TS_NONE          0x0000FFFFU                                                       /*!< No trigger selected                    */
/**
  * @}
  */

/** @defgroup TIM_Trigger_Polarity TIM Trigger Polarity
  * @{
  */
#define TIM_TRIGGERPOLARITY_INVERTED           TIM_ETRPOLARITY_INVERTED               /*!< Polarity for ETRx trigger sources             */
#define TIM_TRIGGERPOLARITY_NONINVERTED        TIM_ETRPOLARITY_NONINVERTED            /*!< Polarity for ETRx trigger sources             */
#define TIM_TRIGGERPOLARITY_RISING             TIM_INPUTCHANNELPOLARITY_RISING        /*!< Polarity for TIxFPx or TI1_ED trigger sources */
#define TIM_TRIGGERPOLARITY_FALLING            TIM_INPUTCHANNELPOLARITY_FALLING       /*!< Polarity for TIxFPx or TI1_ED trigger sources */
#define TIM_TRIGGERPOLARITY_BOTHEDGE           TIM_INPUTCHANNELPOLARITY_BOTHEDGE      /*!< Polarity for TIxFPx or TI1_ED trigger sources */
/**
  * @}
  */

/** @defgroup TIM_Trigger_Prescaler TIM Trigger Prescaler
  * @{
  */
#define TIM_TRIGGERPRESCALER_DIV1             TIM_ETRPRESCALER_DIV1             /*!< No prescaler is used                                                       */
#define TIM_TRIGGERPRESCALER_DIV2             TIM_ETRPRESCALER_DIV2             /*!< Prescaler for External ETR Trigger: Capture performed once every 2 events. */
#define TIM_TRIGGERPRESCALER_DIV4             TIM_ETRPRESCALER_DIV4             /*!< Prescaler for External ETR Trigger: Capture performed once every 4 events. */
#define TIM_TRIGGERPRESCALER_DIV8             TIM_ETRPRESCALER_DIV8             /*!< Prescaler for External ETR Trigger: Capture performed once every 8 events. */
/**
  * @}
  */

/** @defgroup TIM_TI1_Selection TIM TI1 Input Selection
  * @{
  */
#define TIM_TI1SELECTION_CH1               0x00000000U                          /*!< The TIMx_CH1 pin is connected to TI1 input */
#define TIM_TI1SELECTION_XORCOMBINATION    TIM_CR2_TI1S                         /*!< The TIMx_CH1, CH2 and CH3 pins are connected to the TI1 input (XOR combination) */
/**
  * @}
  */

/** @defgroup TIM_DMA_Burst_Length TIM DMA Burst Length
  * @{
  */
#define TIM_DMABURSTLENGTH_1TRANSFER       0x00000000U                          /*!< The transfer is done to 1 register starting trom TIMx_CR1 + TIMx_DCR.DBA   */
#define TIM_DMABURSTLENGTH_2TRANSFERS      0x00000100U                          /*!< The transfer is done to 2 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_3TRANSFERS      0x00000200U                          /*!< The transfer is done to 3 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_4TRANSFERS      0x00000300U                          /*!< The transfer is done to 4 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_5TRANSFERS      0x00000400U                          /*!< The transfer is done to 5 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_6TRANSFERS      0x00000500U                          /*!< The transfer is done to 6 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_7TRANSFERS      0x00000600U                          /*!< The transfer is done to 7 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_8TRANSFERS      0x00000700U                          /*!< The transfer is done to 8 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_9TRANSFERS      0x00000800U                          /*!< The transfer is done to 9 registers starting trom TIMx_CR1 + TIMx_DCR.DBA  */
#define TIM_DMABURSTLENGTH_10TRANSFERS     0x00000900U                          /*!< The transfer is done to 10 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_11TRANSFERS     0x00000A00U                          /*!< The transfer is done to 11 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_12TRANSFERS     0x00000B00U                          /*!< The transfer is done to 12 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_13TRANSFERS     0x00000C00U                          /*!< The transfer is done to 13 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_14TRANSFERS     0x00000D00U                          /*!< The transfer is done to 14 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_15TRANSFERS     0x00000E00U                          /*!< The transfer is done to 15 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_16TRANSFERS     0x00000F00U                          /*!< The transfer is done to 16 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_17TRANSFERS     0x00001000U                          /*!< The transfer is done to 17 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
#define TIM_DMABURSTLENGTH_18TRANSFERS     0x00001100U                          /*!< The transfer is done to 18 registers starting trom TIMx_CR1 + TIMx_DCR.DBA */
/**
  * @}
  */

/** @defgroup DMA_Handle_index TIM DMA Handle Index
  * @{
  */
#define TIM_DMA_ID_UPDATE                ((uint16_t) 0x0000)       /*!< Index of the DMA handle used for Update DMA requests */
#define TIM_DMA_ID_CC1                   ((uint16_t) 0x0001)       /*!< Index of the DMA handle used for Capture/Compare 1 DMA requests */
#define TIM_DMA_ID_CC2                   ((uint16_t) 0x0002)       /*!< Index of the DMA handle used for Capture/Compare 2 DMA requests */
#define TIM_DMA_ID_CC3                   ((uint16_t) 0x0003)       /*!< Index of the DMA handle used for Capture/Compare 3 DMA requests */
#define TIM_DMA_ID_CC4                   ((uint16_t) 0x0004)       /*!< Index of the DMA handle used for Capture/Compare 4 DMA requests */
#define TIM_DMA_ID_COMMUTATION           ((uint16_t) 0x0005)       /*!< Index of the DMA handle used for Commutation DMA requests */
#define TIM_DMA_ID_TRIGGER               ((uint16_t) 0x0006)       /*!< Index of the DMA handle used for Trigger DMA requests */
/**
  * @}
  */

/** @defgroup Channel_CC_State TIM Capture/Compare Channel State
  * @{
  */
#define TIM_CCx_ENABLE                   0x00000001U                            /*!< Input or output channel is enabled */
#define TIM_CCx_DISABLE                  0x00000000U                            /*!< Input or output channel is disabled */
#define TIM_CCxN_ENABLE                  0x00000002U                            /*!< Complementary output channel is enabled */
#define TIM_CCxN_DISABLE                 0x00000000U                            /*!< Complementary output channel is enabled */
/**
  * @}
  */

/** @defgroup TIM_Break_System TIM Break System
  * @{
  */
#define TIM_BREAK_SYSTEM_ECC                 SYSCFG_CFGR2_ECCL   /*!< Enables and locks the ECC error signal with Break Input of TIM1/8/15/16/17 */
#define TIM_BREAK_SYSTEM_PVD                 SYSCFG_CFGR2_PVDL   /*!< Enables and locks the PVD connection with TIM1/8/15/16/17 Break Input and also the PVDE and PLS bits of the Power Control Interface */
#define TIM_BREAK_SYSTEM_SRAM_PARITY_ERROR   SYSCFG_CFGR2_SPL    /*!< Enables and locks the SRAM_PARITY error signal with Break Input of TIM1/8/15/16/17 */
#define TIM_BREAK_SYSTEM_LOCKUP              SYSCFG_CFGR2_CLL    /*!< Enables and locks the LOCKUP output of CortexM4 with Break Input of TIM1/8/15/16/17 */
/**
  * @}
  */



/* End of exported macros ----------------------------------------------------*/


/* Exported macros -----------------------------------------------------------*/
/** @defgroup TIM_Exported_Macros TIM Exported Macros
  * @{
  */

/** @brief  Reset TIM handle state.
  * @param  __HANDLE__ TIM handle.
  * @retval None
  */
#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
#define __HAL_TIM_RESET_HANDLE_STATE(__HANDLE__) do {                                                               \
                                                      (__HANDLE__)->State            = HAL_TIM_STATE_RESET;         \
                                                      (__HANDLE__)->ChannelState[0]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[1]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[2]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[3]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[4]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[5]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[0] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[1] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[2] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[3] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->DMABurstState    = HAL_DMA_BURST_STATE_RESET;   \
                                                      (__HANDLE__)->Base_MspInitCallback         = NULL;            \
                                                      (__HANDLE__)->Base_MspDeInitCallback       = NULL;            \
                                                      (__HANDLE__)->IC_MspInitCallback           = NULL;            \
                                                      (__HANDLE__)->IC_MspDeInitCallback         = NULL;            \
                                                      (__HANDLE__)->OC_MspInitCallback           = NULL;            \
                                                      (__HANDLE__)->OC_MspDeInitCallback         = NULL;            \
                                                      (__HANDLE__)->PWM_MspInitCallback          = NULL;            \
                                                      (__HANDLE__)->PWM_MspDeInitCallback        = NULL;            \
                                                      (__HANDLE__)->OnePulse_MspInitCallback     = NULL;            \
                                                      (__HANDLE__)->OnePulse_MspDeInitCallback   = NULL;            \
                                                      (__HANDLE__)->Encoder_MspInitCallback      = NULL;            \
                                                      (__HANDLE__)->Encoder_MspDeInitCallback    = NULL;            \
                                                      (__HANDLE__)->HallSensor_MspInitCallback   = NULL;            \
                                                      (__HANDLE__)->HallSensor_MspDeInitCallback = NULL;            \
                                                     } while(0)
#else
#define __HAL_TIM_RESET_HANDLE_STATE(__HANDLE__) do {                                                               \
                                                      (__HANDLE__)->State            = HAL_TIM_STATE_RESET;         \
                                                      (__HANDLE__)->ChannelState[0]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[1]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[2]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[3]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[4]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelState[5]  = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[0] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[1] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[2] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->ChannelNState[3] = HAL_TIM_CHANNEL_STATE_RESET; \
                                                      (__HANDLE__)->DMABurstState    = HAL_DMA_BURST_STATE_RESET;   \
                                                     } while(0)
#endif

#define __HAL_TIM_SET_DTG(__HANDLE__, __DTG__) do {                                              \
                                                    ((__HANDLE__)->Instance->BDTR &= ~(0xff));   \
                                                    ((__HANDLE__)->Instance->BDTR |= (__DTG__)); \
                                                  } while(0)

/**
  * @brief  Enable the TIM peripheral.
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_ENABLE(__HANDLE__)                 ((__HANDLE__)->Instance->CR1|=(TIM_CR1_CEN))

/**
  * @brief  Enable the TIM main Output.
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_MOE_ENABLE(__HANDLE__)             ((__HANDLE__)->Instance->BDTR|=(TIM_BDTR_MOE))

/**
  * @brief  Enable the TIM channel123 output
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_CC1E_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC1E))
#define __HAL_TIM_CC2E_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC2E))
#define __HAL_TIM_CC3E_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC3E))
#define __HAL_TIM_CC4E_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC4E))
/**
  * @brief  Disable the TIM channel123 output
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_CC1E_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC1E))
#define __HAL_TIM_CC2E_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC2E))
#define __HAL_TIM_CC3E_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC3E))
#define __HAL_TIM_CC4E_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC4E))
/**
  * @brief  Enable the TIM channeln123 output
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_CC1NE_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC1NE))
#define __HAL_TIM_CC2NE_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC2NE))
#define __HAL_TIM_CC3NE_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC3NE))
#define __HAL_TIM_CC4NE_ENABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER |=(TIM_CCER_CC4NE))
/**
  * @brief  Disable the TIM channeln123 output
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_CC1NE_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC1NE))
#define __HAL_TIM_CC2NE_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC2NE))
#define __HAL_TIM_CC3NE_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC3NE))
#define __HAL_TIM_CC4NE_DISABLE(__HANDLE__)            ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC4NE))
/**
  * @brief  the TIM CNT clear
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_CNT_CLEAR(__HANDLE__)               ((__HANDLE__)->Instance->CNT = 0UL)

/**
  * @brief  the TIM SOFT TO UG UPDATA
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_EGR_UG(__HANDLE__)                  ((__HANDLE__)->Instance->EGR = TIM_EGR_UG)
/**
  * @brief  Disable the TIM peripheral.
  * @param  __HANDLE__ TIM handle
  * @retval None
  */
#define __HAL_TIM_DISABLE(__HANDLE__) \
  do { \
    if (((__HANDLE__)->Instance->CCER & TIM_CCER_CCxE_MASK) == 0UL) \
    { \
      if(((__HANDLE__)->Instance->CCER & TIM_CCER_CCxNE_MASK) == 0UL) \
      { \
        (__HANDLE__)->Instance->CR1 &= ~(TIM_CR1_CEN); \
      } \
    } \
  } while(0)

/**
  * @brief  Disable the TIM main Output.
  * @param  __HANDLE__ TIM handle
  * @retval None
  * @note The Main Output Enable of a timer instance is disabled only if all the CCx and CCxN channels have been disabled
  */
#define __HAL_TIM_MOE_DISABLE(__HANDLE__) \
  do { \
    if (((__HANDLE__)->Instance->CCER & TIM_CCER_CCxE_MASK) == 0UL) \
    { \
      if(((__HANDLE__)->Instance->CCER & TIM_CCER_CCxNE_MASK) == 0UL) \
      { \
        (__HANDLE__)->Instance->BDTR &= ~(TIM_BDTR_MOE); \
      } \
    } \
  } while(0)

/**
  * @brief  Disable the TIM main Output.
  * @param  __HANDLE__ TIM handle
  * @retval None
  * @note The Main Output Enable of a timer instance is disabled unconditionally
  */
#define __HAL_TIM_MOE_DISABLE_UNCONDITIONALLY(__HANDLE__)  (__HANDLE__)->Instance->BDTR &= ~(TIM_BDTR_MOE)

/** @brief  Enable the specified TIM interrupt.
  * @param  __HANDLE__ specifies the TIM Handle.
  * @param  __INTERRUPT__ specifies the TIM interrupt source to enable.
  *          This parameter can be one of the following values:
  *            @arg TIM_IT_UPDATE: Update interrupt
  *            @arg TIM_IT_CC1:   Capture/Compare 1 interrupt
  *            @arg TIM_IT_CC2:  Capture/Compare 2 interrupt
  *            @arg TIM_IT_CC3:  Capture/Compare 3 interrupt
  *            @arg TIM_IT_CC4:  Capture/Compare 4 interrupt
  *            @arg TIM_IT_COM:   Commutation interrupt
  *            @arg TIM_IT_TRIGGER: Trigger interrupt
  *            @arg TIM_IT_BREAK: Break interrupt
  * @retval None
  */
#define __HAL_TIM_ENABLE_IT(__HANDLE__, __INTERRUPT__)    ((__HANDLE__)->Instance->DIER |= (__INTERRUPT__))

/** @brief  Disable the specified TIM interrupt.
  * @param  __HANDLE__ specifies the TIM Handle.
  * @param  __INTERRUPT__ specifies the TIM interrupt source to disable.
  *          This parameter can be one of the following values:
  *            @arg TIM_IT_UPDATE: Update interrupt
  *            @arg TIM_IT_CC1:   Capture/Compare 1 interrupt
  *            @arg TIM_IT_CC2:  Capture/Compare 2 interrupt
  *            @arg TIM_IT_CC3:  Capture/Compare 3 interrupt
  *            @arg TIM_IT_CC4:  Capture/Compare 4 interrupt
  *            @arg TIM_IT_COM:   Commutation interrupt
  *            @arg TIM_IT_TRIGGER: Trigger interrupt
  *            @arg TIM_IT_BREAK: Break interrupt
  * @retval None
  */
#define __HAL_TIM_DISABLE_IT(__HANDLE__, __INTERRUPT__)   ((__HANDLE__)->Instance->DIER &= ~(__INTERRUPT__))

/** @brief  Enable the specified DMA request.
  * @param  __HANDLE__ specifies the TIM Handle.
  * @param  __DMA__ specifies the TIM DMA request to enable.
  *          This parameter can be one of the following values:
  *            @arg TIM_DMA_UPDATE: Update DMA request
  *            @arg TIM_DMA_CC1:   Capture/Compare 1 DMA request
  *            @arg TIM_DMA_CC2:  Capture/Compare 2 DMA request
  *            @arg TIM_DMA_CC3:  Capture/Compare 3 DMA request
  *            @arg TIM_DMA_CC4:  Capture/Compare 4 DMA request
  *            @arg TIM_DMA_COM:   Commutation DMA request
  *            @arg TIM_DMA_TRIGGER: Trigger DMA request
  * @retval None
  */
#define __HAL_TIM_ENABLE_DMA(__HANDLE__, __DMA__)         ((__HANDLE__)->Instance->DIER |= (__DMA__))

/** @brief  Disable the specified DMA request.
  * @param  __HANDLE__ specifies the TIM Handle.
  * @param  __DMA__ specifies the TIM DMA request to disable.
  *          This parameter can be one of the following values:
  *            @arg TIM_DMA_UPDATE: Update DMA request
  *            @arg TIM_DMA_CC1:   Capture/Compare 1 DMA request
  *            @arg TIM_DMA_CC2:  Capture/Compare 2 DMA request
  *            @arg TIM_DMA_CC3:  Capture/Compare 3 DMA request
  *            @arg TIM_DMA_CC4:  Capture/Compare 4 DMA request
  *            @arg TIM_DMA_COM:   Commutation DMA request
  *            @arg TIM_DMA_TRIGGER: Trigger DMA request
  * @retval None
  */
#define __HAL_TIM_DISABLE_DMA(__HANDLE__, __DMA__)        ((__HANDLE__)->Instance->DIER &= ~(__DMA__))

/** @brief  Check whether the specified TIM interrupt flag is set or not.
  * @param  __HANDLE__ specifies the TIM Handle.
  * @param  __FLAG__ specifies the TIM interrupt flag to check.
  *        This parameter can be one of the following values:
  *            @arg TIM_FLAG_UPDATE: Update interrupt flag
  *            @arg TIM_FLAG_CC1: Capture/Compare 1 interrupt flag
  *            @arg TIM_FLAG_CC2: Capture/Compare 2 interrupt flag
  *            @arg TIM_FLAG_CC3: Capture/Compare 3 interrupt flag
  *            @arg TIM_FLAG_CC4: Capture/Compare 4 interrupt flag
  *            @arg TIM_FLAG_CC5: Compare 5 interrupt flag
  *            @arg TIM_FLAG_CC6: Compare 6 interrupt flag
  *            @arg TIM_FLAG_COM:  Commutation interrupt flag
  *            @arg TIM_FLAG_TRIGGER: Trigger interrupt flag
  *            @arg TIM_FLAG_BREAK: Break interrupt flag
  *            @arg TIM_FLAG_BREAK2: Break 2 interrupt flag
  *            @arg TIM_FLAG_SYSTEM_BREAK: System Break interrupt flag
  *            @arg TIM_FLAG_CC1OF: Capture/Compare 1 overcapture flag
  *            @arg TIM_FLAG_CC2OF: Capture/Compare 2 overcapture flag
  *            @arg TIM_FLAG_CC3OF: Capture/Compare 3 overcapture flag
  *            @arg TIM_FLAG_CC4OF: Capture/Compare 4 overcapture flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_TIM_GET_FLAG(__HANDLE__, __FLAG__)          (((__HANDLE__)->Instance->SR &(__FLAG__)) == (__FLAG__))

/** @brief  Clear the specified TIM interrupt flag.
  * @param  __HANDLE__ specifies the TIM Handle.
  * @param  __FLAG__ specifies the TIM interrupt flag to clear.
  *        This parameter can be one of the following values:
  *            @arg TIM_FLAG_UPDATE: Update interrupt flag
  *            @arg TIM_FLAG_CC1: Capture/Compare 1 interrupt flag
  *            @arg TIM_FLAG_CC2: Capture/Compare 2 interrupt flag
  *            @arg TIM_FLAG_CC3: Capture/Compare 3 interrupt flag
  *            @arg TIM_FLAG_CC4: Capture/Compare 4 interrupt flag
  *            @arg TIM_FLAG_CC5: Compare 5 interrupt flag
  *            @arg TIM_FLAG_CC6: Compare 6 interrupt flag
  *            @arg TIM_FLAG_COM:  Commutation interrupt flag
  *            @arg TIM_FLAG_TRIGGER: Trigger interrupt flag
  *            @arg TIM_FLAG_BREAK: Break interrupt flag
  *            @arg TIM_FLAG_BREAK2: Break 2 interrupt flag
  *            @arg TIM_FLAG_SYSTEM_BREAK: System Break interrupt flag
  *            @arg TIM_FLAG_CC1OF: Capture/Compare 1 overcapture flag
  *            @arg TIM_FLAG_CC2OF: Capture/Compare 2 overcapture flag
  *            @arg TIM_FLAG_CC3OF: Capture/Compare 3 overcapture flag
  *            @arg TIM_FLAG_CC4OF: Capture/Compare 4 overcapture flag
  * @retval The new state of __FLAG__ (TRUE or FALSE).
  */
#define __HAL_TIM_CLEAR_FLAG(__HANDLE__, __FLAG__)        ((__HANDLE__)->Instance->SR = ~(__FLAG__))

/**
  * @brief  Check whether the specified TIM interrupt source is enabled or not.
  * @param  __HANDLE__ TIM handle
  * @param  __INTERRUPT__ specifies the TIM interrupt source to check.
  *          This parameter can be one of the following values:
  *            @arg TIM_IT_UPDATE: Update interrupt
  *            @arg TIM_IT_CC1:   Capture/Compare 1 interrupt
  *            @arg TIM_IT_CC2:  Capture/Compare 2 interrupt
  *            @arg TIM_IT_CC3:  Capture/Compare 3 interrupt
  *            @arg TIM_IT_CC4:  Capture/Compare 4 interrupt
  *            @arg TIM_IT_COM:   Commutation interrupt
  *            @arg TIM_IT_TRIGGER: Trigger interrupt
  *            @arg TIM_IT_BREAK: Break interrupt
  * @retval The state of TIM_IT (SET or RESET).
  */
#define __HAL_TIM_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__) ((((__HANDLE__)->Instance->DIER & (__INTERRUPT__)) \
                                                             == (__INTERRUPT__)) ? STATE_SET : STATE_RESET)

/** @brief Clear the TIM interrupt pending bits.
  * @param  __HANDLE__ TIM handle
  * @param  __INTERRUPT__ specifies the interrupt pending bit to clear.
  *          This parameter can be one of the following values:
  *            @arg TIM_IT_UPDATE: Update interrupt
  *            @arg TIM_IT_CC1:   Capture/Compare 1 interrupt
  *            @arg TIM_IT_CC2:  Capture/Compare 2 interrupt
  *            @arg TIM_IT_CC3:  Capture/Compare 3 interrupt
  *            @arg TIM_IT_CC4:  Capture/Compare 4 interrupt
  *            @arg TIM_IT_COM:   Commutation interrupt
  *            @arg TIM_IT_TRIGGER: Trigger interrupt
  *            @arg TIM_IT_BREAK: Break interrupt
  * @retval None
  */
#define __HAL_TIM_CLEAR_IT(__HANDLE__, __INTERRUPT__)      ((__HANDLE__)->Instance->SR = ~(__INTERRUPT__))

/**
  * @brief  Force a continuous copy of the update interrupt flag (UIF) into the timer counter register (bit 31).
  * @note This allows both the counter value and a potential roll-over condition signalled by the UIFCPY flag to be read in an atomic way.
  * @param  __HANDLE__ TIM handle.
  * @retval None
mode.
  */
#define __HAL_TIM_UIFREMAP_ENABLE(__HANDLE__)    (((__HANDLE__)->Instance->CR1 |= TIM_CR1_UIFREMAP))

/**
  * @brief  Disable update interrupt flag (UIF) remapping.
  * @param  __HANDLE__ TIM handle.
  * @retval None
mode.
  */
#define __HAL_TIM_UIFREMAP_DISABLE(__HANDLE__)    (((__HANDLE__)->Instance->CR1 &= ~TIM_CR1_UIFREMAP))

/**
  * @brief  Get update interrupt flag (UIF) copy status.
  * @param  __COUNTER__ Counter value.
  * @retval The state of UIFCPY (TRUE or FALSE).
mode.
  */
#define __HAL_TIM_GET_UIFCPY(__COUNTER__)    (((__COUNTER__) & (TIM_CNT_UIFCPY)) == (TIM_CNT_UIFCPY))

/**
  * @brief  Indicates whether or not the TIM Counter is used as downcounter.
  * @param  __HANDLE__ TIM handle.
  * @retval False (Counter used as upcounter) or True (Counter used as downcounter)
  * @note This macro is particularly useful to get the counting mode when the timer operates in Center-aligned mode or Encoder
mode.
  */
#define __HAL_TIM_IS_TIM_COUNTING_DOWN(__HANDLE__)    (((__HANDLE__)->Instance->CR1 &(TIM_CR1_DIR)) == (TIM_CR1_DIR))

/**
  * @brief  Set the TIM Prescaler on runtime.
  * @param  __HANDLE__ TIM handle.
  * @param  __PRESC__ specifies the Prescaler new value.
  * @retval None
  */
#define __HAL_TIM_SET_PRESCALER(__HANDLE__, __PRESC__)       ((__HANDLE__)->Instance->PSC = (__PRESC__))

/**
  * @brief  Set the TIM Counter Register value on runtime.
  * Note Please check if the bit 31 of CNT register is used as UIF copy or not, this may affect the counter range in case of 32 bits counter TIM instance.
  *      Bit 31 of CNT can be enabled/disabled using __HAL_TIM_UIFREMAP_ENABLE()/__HAL_TIM_UIFREMAP_DISABLE() macros.
  * @param  __HANDLE__ TIM handle.
  * @param  __COUNTER__ specifies the Counter register new value.
  * @retval None
  */
#define __HAL_TIM_SET_COUNTER(__HANDLE__, __COUNTER__)  ((__HANDLE__)->Instance->CNT = (__COUNTER__))

/**
  * @brief  Get the TIM Counter Register value on runtime.
  * @param  __HANDLE__ TIM handle.
  * @retval 16-bit or 32-bit value of the timer counter register (TIMx_CNT)
  */
#define __HAL_TIM_GET_COUNTER(__HANDLE__)  ((__HANDLE__)->Instance->CNT)

/**
  * @brief  Set the TIM Autoreload Register value on runtime without calling another time any Init function.
  * @param  __HANDLE__ TIM handle.
  * @param  __AUTORELOAD__ specifies the Counter register new value.
  * @retval None
  */
#define __HAL_TIM_SET_AUTORELOAD(__HANDLE__, __AUTORELOAD__) \
  do{                                                    \
    (__HANDLE__)->Instance->ARR = (__AUTORELOAD__);  \
    (__HANDLE__)->Init.Period = (__AUTORELOAD__);    \
  } while(0)

/**
  * @brief  Get the TIM Autoreload Register value on runtime.
  * @param  __HANDLE__ TIM handle.
  * @retval 16-bit or 32-bit value of the timer auto-reload register(TIMx_ARR)
  */
#define __HAL_TIM_GET_AUTORELOAD(__HANDLE__)  ((__HANDLE__)->Instance->ARR)

/**
  * @brief  Set the TIM Clock Division value on runtime without calling another time any Init function.
  * @param  __HANDLE__ TIM handle.
  * @param  __CKD__ specifies the clock division value.
  *          This parameter can be one of the following value:
  *            @arg TIM_CLOCKDIVISION_DIV1: tDTS=tCK_INT
  *            @arg TIM_CLOCKDIVISION_DIV2: tDTS=2*tCK_INT
  *            @arg TIM_CLOCKDIVISION_DIV4: tDTS=4*tCK_INT
  * @retval None
  */
#define __HAL_TIM_SET_CLOCKDIVISION(__HANDLE__, __CKD__) \
  do{                                                   \
    (__HANDLE__)->Instance->CR1 &= (~TIM_CR1_CKD);  \
    (__HANDLE__)->Instance->CR1 |= (__CKD__);       \
    (__HANDLE__)->Init.ClockDivision = (__CKD__);   \
  } while(0)

/**
  * @brief  Get the TIM Clock Division value on runtime.
  * @param  __HANDLE__ TIM handle.
  * @retval The clock division can be one of the following values:
  *            @arg TIM_CLOCKDIVISION_DIV1: tDTS=tCK_INT
  *            @arg TIM_CLOCKDIVISION_DIV2: tDTS=2*tCK_INT
  *            @arg TIM_CLOCKDIVISION_DIV4: tDTS=4*tCK_INT
  */
#define __HAL_TIM_GET_CLOCKDIVISION(__HANDLE__)  ((__HANDLE__)->Instance->CR1 & TIM_CR1_CKD)

#define __HAL_TIM_DMA_SELECTION(__HANDLE__, __SEL__) \
  do{                                                   \
    (__HANDLE__)->Instance->CR2 &= (~TIM_CR2_CCDS);  \
    (__HANDLE__)->Instance->CR2 |= (__SEL__);       \
  } while(0)

/**
  * @brief  Set the TIM Input Capture prescaler on runtime without calling another time HAL_TIM_IC_ConfigChannel() function.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  * @param  __ICPSC__ specifies the Input Capture4 prescaler new value.
  *          This parameter can be one of the following values:
  *            @arg TIM_ICPSC_DIV1: no prescaler
  *            @arg TIM_ICPSC_DIV2: capture is done once every 2 events
  *            @arg TIM_ICPSC_DIV4: capture is done once every 4 events
  *            @arg TIM_ICPSC_DIV8: capture is done once every 8 events
  * @retval None
  */
#define __HAL_TIM_SET_ICPRESCALER(__HANDLE__, __CHANNEL__, __ICPSC__) \
  do{                                                    \
    TIM_RESET_ICPRESCALERVALUE((__HANDLE__), (__CHANNEL__));  \
    TIM_SET_ICPRESCALERVALUE((__HANDLE__), (__CHANNEL__), (__ICPSC__)); \
  } while(0)

/**
  * @brief  Get the TIM Input Capture prescaler on runtime.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: get input capture 1 prescaler value
  *            @arg TIM_CHANNEL_2: get input capture 2 prescaler value
  *            @arg TIM_CHANNEL_3: get input capture 3 prescaler value
  *            @arg TIM_CHANNEL_4: get input capture 4 prescaler value
  * @retval The input capture prescaler can be one of the following values:
  *            @arg TIM_ICPSC_DIV1: no prescaler
  *            @arg TIM_ICPSC_DIV2: capture is done once every 2 events
  *            @arg TIM_ICPSC_DIV4: capture is done once every 4 events
  *            @arg TIM_ICPSC_DIV8: capture is done once every 8 events
  */
#define __HAL_TIM_GET_ICPRESCALER(__HANDLE__, __CHANNEL__)  \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 & TIM_CCMR1_IC1PSC) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? (((__HANDLE__)->Instance->CCMR1 & TIM_CCMR1_IC2PSC) >> 8U) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 & TIM_CCMR2_IC3PSC) :\
   (((__HANDLE__)->Instance->CCMR2 & TIM_CCMR2_IC4PSC)) >> 8U)

/**
  * @brief  Set the TIM Capture Compare Register value on runtime without calling another time ConfigChannel function.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @param  __COMPARE__ specifies the Capture Compare register new value.
  * @retval None
  */
#define __HAL_TIM_SET_COMPARE(__HANDLE__, __CHANNEL__, __COMPARE__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCR1 = (__COMPARE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCR2 = (__COMPARE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCR3 = (__COMPARE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCR4 = (__COMPARE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCR5 = (__COMPARE__)) :\
   ((__HANDLE__)->Instance->CCR6 = (__COMPARE__)))

/**
  * @brief  Get the TIM Capture Compare Register value on runtime.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channel associated with the capture compare register
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: get capture/compare 1 register value
  *            @arg TIM_CHANNEL_2: get capture/compare 2 register value
  *            @arg TIM_CHANNEL_3: get capture/compare 3 register value
  *            @arg TIM_CHANNEL_4: get capture/compare 4 register value
  *            @arg TIM_CHANNEL_5: get capture/compare 5 register value
  *            @arg TIM_CHANNEL_6: get capture/compare 6 register value
  * @retval 16-bit or 32-bit value of the capture/compare register (TIMx_CCRy)
  */
#define __HAL_TIM_GET_COMPARE(__HANDLE__, __CHANNEL__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCR1) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCR2) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCR3) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCR4) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCR5) :\
   ((__HANDLE__)->Instance->CCR6))

/**
  * @brief  Set the TIM Output compare preload.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @retval None
  */
#define __HAL_TIM_ENABLE_OCxPRELOAD(__HANDLE__, __CHANNEL__)    \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC1PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC2PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC3PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC4PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCMR3 |= TIM_CCMR3_OC5PE) :\
   ((__HANDLE__)->Instance->CCMR3 |= TIM_CCMR3_OC6PE))

/**
  * @brief  Reset the TIM Output compare preload.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @retval None
  */
#define __HAL_TIM_DISABLE_OCxPRELOAD(__HANDLE__, __CHANNEL__)    \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 &= ~TIM_CCMR1_OC1PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 &= ~TIM_CCMR1_OC2PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 &= ~TIM_CCMR2_OC3PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCMR2 &= ~TIM_CCMR2_OC4PE) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCMR3 &= ~TIM_CCMR3_OC5PE) :\
   ((__HANDLE__)->Instance->CCMR3 &= ~TIM_CCMR3_OC6PE))

/**
  * @brief  Enable fast mode for a given channel.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @note  When fast mode is enabled an active edge on the trigger input acts
  *        like a compare match on CCx output. Delay to sample the trigger
  *        input and to activate CCx output is reduced to 3 clock cycles.
  * @note  Fast mode acts only if the channel is configured in PWM1 or PWM2 mode.
  * @retval None
  */
#define __HAL_TIM_ENABLE_OCxFAST(__HANDLE__, __CHANNEL__)    \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC1FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 |= TIM_CCMR1_OC2FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC3FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCMR2 |= TIM_CCMR2_OC4FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCMR3 |= TIM_CCMR3_OC5FE) :\
   ((__HANDLE__)->Instance->CCMR3 |= TIM_CCMR3_OC6FE))

/**
  * @brief  Disable fast mode for a given channel.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  *            @arg TIM_CHANNEL_5: TIM Channel 5 selected
  *            @arg TIM_CHANNEL_6: TIM Channel 6 selected
  * @note  When fast mode is disabled CCx output behaves normally depending
  *        on counter and CCRx values even when the trigger is ON. The minimum
  *        delay to activate CCx output when an active edge occurs on the
  *        trigger input is 5 clock cycles.
  * @retval None
  */
#define __HAL_TIM_DISABLE_OCxFAST(__HANDLE__, __CHANNEL__)    \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 &= ~TIM_CCMR1_OC1FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 &= ~TIM_CCMR1_OC2FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 &= ~TIM_CCMR2_OC3FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->Instance->CCMR2 &= ~TIM_CCMR2_OC4FE) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->Instance->CCMR3 &= ~TIM_CCMR3_OC5FE) :\
   ((__HANDLE__)->Instance->CCMR3 &= ~TIM_CCMR3_OC6FE))

/**
  * @brief  Set the Update Request Source (URS) bit of the TIMx_CR1 register.
  * @param  __HANDLE__ TIM handle.
  * @note  When the URS bit of the TIMx_CR1 register is set, only counter
  *        overflow/underflow generates an update interrupt or DMA request (if
  *        enabled)
  * @retval None
  */
#define __HAL_TIM_URS_ENABLE(__HANDLE__)  ((__HANDLE__)->Instance->CR1|= TIM_CR1_URS)

/**
  * @brief  Reset the Update Request Source (URS) bit of the TIMx_CR1 register.
  * @param  __HANDLE__ TIM handle.
  * @note  When the URS bit of the TIMx_CR1 register is reset, any of the
  *        following events generate an update interrupt or DMA request (if
  *        enabled):
  *           _ Counter overflow underflow
  *           _ Setting the UG bit
  *           _ Update generation through the slave mode controller
  * @retval None
  */
#define __HAL_TIM_URS_DISABLE(__HANDLE__)  ((__HANDLE__)->Instance->CR1&=~TIM_CR1_URS)

/**
  * @brief  Set the TIM Capture x input polarity on runtime.
  * @param  __HANDLE__ TIM handle.
  * @param  __CHANNEL__ TIM Channels to be configured.
  *          This parameter can be one of the following values:
  *            @arg TIM_CHANNEL_1: TIM Channel 1 selected
  *            @arg TIM_CHANNEL_2: TIM Channel 2 selected
  *            @arg TIM_CHANNEL_3: TIM Channel 3 selected
  *            @arg TIM_CHANNEL_4: TIM Channel 4 selected
  * @param  __POLARITY__ Polarity for TIx source
  *            @arg TIM_INPUTCHANNELPOLARITY_RISING: Rising Edge
  *            @arg TIM_INPUTCHANNELPOLARITY_FALLING: Falling Edge
  *            @arg TIM_INPUTCHANNELPOLARITY_BOTHEDGE: Rising and Falling Edge
  * @retval None
  */
#define __HAL_TIM_SET_CAPTUREPOLARITY(__HANDLE__, __CHANNEL__, __POLARITY__)    \
  do{                                                                     \
    TIM_RESET_CAPTUREPOLARITY((__HANDLE__), (__CHANNEL__));               \
    TIM_SET_CAPTUREPOLARITY((__HANDLE__), (__CHANNEL__), (__POLARITY__)); \
  }while(0)

/**
  * @}
  */
/* End of exported macros ----------------------------------------------------*/


















/* Private constants ---------------------------------------------------------*/
/** @defgroup TIM_Private_Constants TIM Private Constants
  * @{
  */
/* The counter of a timer instance is disabled only if all the CCx and CCxN
   channels have been disabled */
#define TIM_CCER_CCxE_MASK  ((uint32_t)(TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E))
#define TIM_CCER_CCxNE_MASK ((uint32_t)(TIM_CCER_CC1NE | TIM_CCER_CC2NE | TIM_CCER_CC3NE))
/**
  * @}
  */



/* Private macros ------------------------------------------------------------*/
/** @defgroup TIM_Private_Macros TIM Private Macros
  * @{
  */
#define IS_TIM_CLEARINPUT_SOURCE(__MODE__)  (((__MODE__) == TIM_CLEARINPUTSOURCE_NONE)      || \
                                             ((__MODE__) == TIM_CLEARINPUTSOURCE_ETR))

#define IS_TIM_DMA_BASE(__BASE__) (((__BASE__) == TIM_DMABASE_CR1)   || \
                                   ((__BASE__) == TIM_DMABASE_CR2)   || \
                                   ((__BASE__) == TIM_DMABASE_SMCR)  || \
                                   ((__BASE__) == TIM_DMABASE_DIER)  || \
                                   ((__BASE__) == TIM_DMABASE_SR)    || \
                                   ((__BASE__) == TIM_DMABASE_EGR)   || \
                                   ((__BASE__) == TIM_DMABASE_CCMR1) || \
                                   ((__BASE__) == TIM_DMABASE_CCMR2) || \
                                   ((__BASE__) == TIM_DMABASE_CCER)  || \
                                   ((__BASE__) == TIM_DMABASE_CNT)   || \
                                   ((__BASE__) == TIM_DMABASE_PSC)   || \
                                   ((__BASE__) == TIM_DMABASE_ARR)   || \
                                   ((__BASE__) == TIM_DMABASE_RCR)   || \
                                   ((__BASE__) == TIM_DMABASE_CCR1)  || \
                                   ((__BASE__) == TIM_DMABASE_CCR2)  || \
                                   ((__BASE__) == TIM_DMABASE_CCR3)  || \
                                   ((__BASE__) == TIM_DMABASE_CCR4)  || \
                                   ((__BASE__) == TIM_DMABASE_BDTR)  || \
                                   ((__BASE__) == TIM_DMABASE_CCMR3) || \
                                   ((__BASE__) == TIM_DMABASE_CCR5)  || \
                                   ((__BASE__) == TIM_DMABASE_CCR6)  || \
                                   ((__BASE__) == TIM_DMABASE_AF1)   || \
                                   ((__BASE__) == TIM_DMABASE_AF2)   || \
                                   ((__BASE__) == TIM_DMABASE_TISEL))


#define IS_TIM_EVENT_SOURCE(__SOURCE__) ((((__SOURCE__) & 0xFFFFFE00U) == 0x00000000U) && ((__SOURCE__) != 0x00000000U))

#define IS_TIM_COUNTER_MODE(__MODE__)      (((__MODE__) == TIM_COUNTERMODE_UP)              || \
                                            ((__MODE__) == TIM_COUNTERMODE_DOWN)            || \
                                            ((__MODE__) == TIM_COUNTERMODE_CENTERALIGNED1)  || \
                                            ((__MODE__) == TIM_COUNTERMODE_CENTERALIGNED2)  || \
                                            ((__MODE__) == TIM_COUNTERMODE_CENTERALIGNED3))

#define IS_TIM_UIFREMAP_MODE(__MODE__)     (((__MODE__) == TIM_UIFREMAP_DISABLE) || \
                                            ((__MODE__) == TIM_UIFREMAP_ENALE))

#define IS_TIM_CLOCKDIVISION_DIV(__DIV__)  (((__DIV__) == TIM_CLOCKDIVISION_DIV1) || \
                                            ((__DIV__) == TIM_CLOCKDIVISION_DIV2) || \
                                            ((__DIV__) == TIM_CLOCKDIVISION_DIV4))

#define IS_TIM_AUTORELOAD_PRELOAD(PRELOAD) (((PRELOAD) == TIM_AUTORELOAD_PRELOAD_DISABLE) || \
                                            ((PRELOAD) == TIM_AUTORELOAD_PRELOAD_ENABLE))

#define IS_TIM_FAST_STATE(__STATE__)       (((__STATE__) == TIM_OCFAST_DISABLE) || \
                                            ((__STATE__) == TIM_OCFAST_ENABLE))

#define IS_TIM_OC_POLARITY(__POLARITY__)   (((__POLARITY__) == TIM_OCPOLARITY_HIGH) || \
                                            ((__POLARITY__) == TIM_OCPOLARITY_LOW))

#define IS_TIM_OCN_POLARITY(__POLARITY__)  (((__POLARITY__) == TIM_OCNPOLARITY_HIGH) || \
                                            ((__POLARITY__) == TIM_OCNPOLARITY_LOW))

#define IS_TIM_OCIDLE_STATE(__STATE__)     (((__STATE__) == TIM_OCIDLESTATE_SET) || \
                                            ((__STATE__) == TIM_OCIDLESTATE_RESET))

#define IS_TIM_OCNIDLE_STATE(__STATE__)    (((__STATE__) == TIM_OCNIDLESTATE_SET) || \
                                            ((__STATE__) == TIM_OCNIDLESTATE_RESET))

#define IS_TIM_ENCODERINPUT_POLARITY(__POLARITY__)   (((__POLARITY__) == TIM_ENCODERINPUTPOLARITY_RISING)   || \
                                                      ((__POLARITY__) == TIM_ENCODERINPUTPOLARITY_FALLING))

#define IS_TIM_IC_POLARITY(__POLARITY__)   (((__POLARITY__) == TIM_ICPOLARITY_RISING)   || \
                                            ((__POLARITY__) == TIM_ICPOLARITY_FALLING)  || \
                                            ((__POLARITY__) == TIM_ICPOLARITY_BOTHEDGE))

#define IS_TIM_IC_SELECTION(__SELECTION__) (((__SELECTION__) == TIM_ICSELECTION_DIRECTTI) || \
                                            ((__SELECTION__) == TIM_ICSELECTION_INDIRECTTI) || \
                                            ((__SELECTION__) == TIM_ICSELECTION_TRC))

#define IS_TIM_IC_PRESCALER(__PRESCALER__) (((__PRESCALER__) == TIM_ICPSC_DIV1) || \
                                            ((__PRESCALER__) == TIM_ICPSC_DIV2) || \
                                            ((__PRESCALER__) == TIM_ICPSC_DIV4) || \
                                            ((__PRESCALER__) == TIM_ICPSC_DIV8))

#define IS_TIM_OPM_MODE(__MODE__)          (((__MODE__) == TIM_OPMODE_SINGLE) || \
                                            ((__MODE__) == TIM_OPMODE_REPETITIVE))

#define IS_TIM_ENCODER_MODE(__MODE__)      (((__MODE__) == TIM_ENCODERMODE_TI1) || \
                                            ((__MODE__) == TIM_ENCODERMODE_TI2) || \
                                            ((__MODE__) == TIM_ENCODERMODE_TI12))

#define IS_TIM_DMA_SOURCE(__SOURCE__) ((((__SOURCE__) & 0xFFFF80FFU) == 0x00000000U) && ((__SOURCE__) != 0x00000000U))

#define IS_TIM_CHANNELS(__CHANNEL__)       (((__CHANNEL__) == TIM_CHANNEL_1) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_2) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_3) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_4) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_5) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_6) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_ALL))

#define IS_TIM_OPM_CHANNELS(__CHANNEL__)   (((__CHANNEL__) == TIM_CHANNEL_1) || \
                                            ((__CHANNEL__) == TIM_CHANNEL_2))

#define IS_TIM_COMPLEMENTARY_CHANNELS(__CHANNEL__) (((__CHANNEL__) == TIM_CHANNEL_1) || \
                                                    ((__CHANNEL__) == TIM_CHANNEL_2) || \
                                                    ((__CHANNEL__) == TIM_CHANNEL_3))

#define IS_TIM_CLOCKSOURCE(__CLOCK__) (((__CLOCK__) == TIM_CLOCKSOURCE_INTERNAL) || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_ETRMODE2) || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_ITR0)     || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_ITR1)     || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_ITR2)     || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_ITR3)     || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_TI1ED)    || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_TI1)      || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_TI2)      || \
                                       ((__CLOCK__) == TIM_CLOCKSOURCE_ETRMODE1))

#define IS_TIM_CLOCKPOLARITY(__POLARITY__) (((__POLARITY__) == TIM_CLOCKPOLARITY_INVERTED)    || \
                                            ((__POLARITY__) == TIM_CLOCKPOLARITY_NONINVERTED) || \
                                            ((__POLARITY__) == TIM_CLOCKPOLARITY_RISING)      || \
                                            ((__POLARITY__) == TIM_CLOCKPOLARITY_FALLING)     || \
                                            ((__POLARITY__) == TIM_CLOCKPOLARITY_BOTHEDGE))

#define IS_TIM_CLOCKPRESCALER(__PRESCALER__) (((__PRESCALER__) == TIM_CLOCKPRESCALER_DIV1) || \
                                              ((__PRESCALER__) == TIM_CLOCKPRESCALER_DIV2) || \
                                              ((__PRESCALER__) == TIM_CLOCKPRESCALER_DIV4) || \
                                              ((__PRESCALER__) == TIM_CLOCKPRESCALER_DIV8))

#define IS_TIM_CLOCKFILTER(__ICFILTER__)      ((__ICFILTER__) <= 0xFU)

#define IS_TIM_CLEARINPUT_POLARITY(__POLARITY__) (((__POLARITY__) == TIM_CLEARINPUTPOLARITY_INVERTED) || \
                                                  ((__POLARITY__) == TIM_CLEARINPUTPOLARITY_NONINVERTED))

#define IS_TIM_CLEARINPUT_PRESCALER(__PRESCALER__) (((__PRESCALER__) == TIM_CLEARINPUTPRESCALER_DIV1) || \
                                                    ((__PRESCALER__) == TIM_CLEARINPUTPRESCALER_DIV2) || \
                                                    ((__PRESCALER__) == TIM_CLEARINPUTPRESCALER_DIV4) || \
                                                    ((__PRESCALER__) == TIM_CLEARINPUTPRESCALER_DIV8))

#define IS_TIM_CLEARINPUT_FILTER(__ICFILTER__) ((__ICFILTER__) <= 0xFU)

#define IS_TIM_OSSR_STATE(__STATE__)       (((__STATE__) == TIM_OSSR_ENABLE) || \
                                            ((__STATE__) == TIM_OSSR_DISABLE))

#define IS_TIM_OSSI_STATE(__STATE__)       (((__STATE__) == TIM_OSSI_ENABLE) || \
                                            ((__STATE__) == TIM_OSSI_DISABLE))

#define IS_TIM_LOCK_LEVEL(__LEVEL__)       (((__LEVEL__) == TIM_LOCKLEVEL_OFF) || \
                                            ((__LEVEL__) == TIM_LOCKLEVEL_1)   || \
                                            ((__LEVEL__) == TIM_LOCKLEVEL_2)   || \
                                            ((__LEVEL__) == TIM_LOCKLEVEL_3))

#define IS_TIM_BREAK_FILTER(__BRKFILTER__) ((__BRKFILTER__) <= 0xFUL)


#define IS_TIM_BREAK_STATE(__STATE__)      (((__STATE__) == TIM_BREAK_ENABLE) || \
                                            ((__STATE__) == TIM_BREAK_DISABLE))

#define IS_TIM_BREAK_POLARITY(__POLARITY__) (((__POLARITY__) == TIM_BREAKPOLARITY_LOW) || \
                                             ((__POLARITY__) == TIM_BREAKPOLARITY_HIGH))

#define IS_TIM_BREAK2_STATE(__STATE__)     (((__STATE__) == TIM_BREAK2_ENABLE) || \
                                            ((__STATE__) == TIM_BREAK2_DISABLE))

#define IS_TIM_BREAK2_POLARITY(__POLARITY__) (((__POLARITY__) == TIM_BREAK2POLARITY_LOW) || \
                                              ((__POLARITY__) == TIM_BREAK2POLARITY_HIGH))

#define IS_TIM_AUTOMATIC_OUTPUT_STATE(__STATE__) (((__STATE__) == TIM_AUTOMATICOUTPUT_ENABLE) || \
                                                  ((__STATE__) == TIM_AUTOMATICOUTPUT_DISABLE))

#define IS_TIM_GROUPCH5(__OCREF__) ((((__OCREF__) & 0x1FFFFFFFU) == 0x00000000U))

#define IS_TIM_TRGO_SOURCE(__SOURCE__) (((__SOURCE__) == TIM_TRGO_RESET)  || \
                                        ((__SOURCE__) == TIM_TRGO_ENABLE) || \
                                        ((__SOURCE__) == TIM_TRGO_UPDATE) || \
                                        ((__SOURCE__) == TIM_TRGO_OC1)    || \
                                        ((__SOURCE__) == TIM_TRGO_OC1REF) || \
                                        ((__SOURCE__) == TIM_TRGO_OC2REF) || \
                                        ((__SOURCE__) == TIM_TRGO_OC3REF) || \
                                        ((__SOURCE__) == TIM_TRGO_OC4REF))

#define IS_TIM_TRGO2_SOURCE(__SOURCE__) (((__SOURCE__) == TIM_TRGO2_RESET)                        || \
                                         ((__SOURCE__) == TIM_TRGO2_ENABLE)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_UPDATE)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC1)                          || \
                                         ((__SOURCE__) == TIM_TRGO2_OC1REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC2REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC3REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC3REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC4REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC5REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC6REF)                       || \
                                         ((__SOURCE__) == TIM_TRGO2_OC4REF_RISINGFALLING)         || \
                                         ((__SOURCE__) == TIM_TRGO2_OC6REF_RISINGFALLING)         || \
                                         ((__SOURCE__) == TIM_TRGO2_OC4REF_RISING_OC6REF_RISING)  || \
                                         ((__SOURCE__) == TIM_TRGO2_OC4REF_RISING_OC6REF_FALLING) || \
                                         ((__SOURCE__) == TIM_TRGO2_OC5REF_RISING_OC6REF_RISING)  || \
                                         ((__SOURCE__) == TIM_TRGO2_OC5REF_RISING_OC6REF_FALLING))

#define IS_TIM_MSM_STATE(__STATE__)      (((__STATE__) == TIM_MASTERSLAVEMODE_ENABLE) || \
                                          ((__STATE__) == TIM_MASTERSLAVEMODE_DISABLE))

#define IS_TIM_SLAVE_MODE(__MODE__) (((__MODE__) == TIM_SLAVEMODE_DISABLE)   || \
                                     ((__MODE__) == TIM_SLAVEMODE_RESET)     || \
                                     ((__MODE__) == TIM_SLAVEMODE_GATED)     || \
                                     ((__MODE__) == TIM_SLAVEMODE_TRIGGER)   || \
                                     ((__MODE__) == TIM_SLAVEMODE_EXTERNAL1) || \
                                     ((__MODE__) == TIM_SLAVEMODE_COMBINED_RESETTRIGGER))

#define IS_TIM_PWM_MODE(__MODE__) (((__MODE__) == TIM_OCMODE_PWM1)               || \
                                   ((__MODE__) == TIM_OCMODE_PWM2)               || \
                                   ((__MODE__) == TIM_OCMODE_COMBINED_PWM1)      || \
                                   ((__MODE__) == TIM_OCMODE_COMBINED_PWM2)      || \
                                   ((__MODE__) == TIM_OCMODE_ASSYMETRIC_PWM1)    || \
                                   ((__MODE__) == TIM_OCMODE_ASSYMETRIC_PWM2))

#define IS_TIM_OC_MODE(__MODE__)  (((__MODE__) == TIM_OCMODE_TIMING)             || \
                                   ((__MODE__) == TIM_OCMODE_ACTIVE)             || \
                                   ((__MODE__) == TIM_OCMODE_INACTIVE)           || \
                                   ((__MODE__) == TIM_OCMODE_TOGGLE)             || \
                                   ((__MODE__) == TIM_OCMODE_FORCED_ACTIVE)      || \
                                   ((__MODE__) == TIM_OCMODE_FORCED_INACTIVE)    || \
                                   ((__MODE__) == TIM_OCMODE_RETRIGERRABLE_OPM1) || \
                                   ((__MODE__) == TIM_OCMODE_RETRIGERRABLE_OPM2))

#define IS_TIM_TRIGGER_SELECTION(__SELECTION__) (((__SELECTION__) == TIM_TS_ITR0) || \
                                                 ((__SELECTION__) == TIM_TS_ITR1) || \
                                                 ((__SELECTION__) == TIM_TS_ITR2) || \
                                                 ((__SELECTION__) == TIM_TS_ITR3) || \
                                                 ((__SELECTION__) == TIM_TS_TI1F_ED) || \
                                                 ((__SELECTION__) == TIM_TS_TI1FP1) || \
                                                 ((__SELECTION__) == TIM_TS_TI2FP2) || \
                                                 ((__SELECTION__) == TIM_TS_ETRF))

#define IS_TIM_INTERNAL_TRIGGEREVENT_SELECTION(__SELECTION__) (((__SELECTION__) == TIM_TS_ITR0) || \
                                                               ((__SELECTION__) == TIM_TS_ITR1) || \
                                                               ((__SELECTION__) == TIM_TS_ITR2) || \
                                                               ((__SELECTION__) == TIM_TS_ITR3) || \
                                                               ((__SELECTION__) == TIM_TS_NONE))

#define IS_TIM_TRIGGERPOLARITY(__POLARITY__)   (((__POLARITY__) == TIM_TRIGGERPOLARITY_INVERTED   ) || \
                                                ((__POLARITY__) == TIM_TRIGGERPOLARITY_NONINVERTED) || \
                                                ((__POLARITY__) == TIM_TRIGGERPOLARITY_RISING     ) || \
                                                ((__POLARITY__) == TIM_TRIGGERPOLARITY_FALLING    ) || \
                                                ((__POLARITY__) == TIM_TRIGGERPOLARITY_BOTHEDGE   ))

#define IS_TIM_TRIGGERPRESCALER(__PRESCALER__) (((__PRESCALER__) == TIM_TRIGGERPRESCALER_DIV1) || \
                                                ((__PRESCALER__) == TIM_TRIGGERPRESCALER_DIV2) || \
                                                ((__PRESCALER__) == TIM_TRIGGERPRESCALER_DIV4) || \
                                                ((__PRESCALER__) == TIM_TRIGGERPRESCALER_DIV8))

#define IS_TIM_TRIGGERFILTER(__ICFILTER__) ((__ICFILTER__) <= 0xFU)

#define IS_TIM_TI1SELECTION(__TI1SELECTION__)  (((__TI1SELECTION__) == TIM_TI1SELECTION_CH1) || \
                                                ((__TI1SELECTION__) == TIM_TI1SELECTION_XORCOMBINATION))

#define IS_TIM_DMA_LENGTH(__LENGTH__)      (((__LENGTH__) == TIM_DMABURSTLENGTH_1TRANSFER)   || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_2TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_3TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_4TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_5TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_6TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_7TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_8TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_9TRANSFERS)  || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_10TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_11TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_12TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_13TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_14TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_15TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_16TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_17TRANSFERS) || \
                                            ((__LENGTH__) == TIM_DMABURSTLENGTH_18TRANSFERS))

#define IS_TIM_DMA_DATA_LENGTH(LENGTH) (((LENGTH) >= 0x1U) && ((LENGTH) < 0x10000U))

#define IS_TIM_IC_FILTER(__ICFILTER__)   ((__ICFILTER__) <= 0xFU)

#define IS_TIM_DEADTIME(__DEADTIME__)    ((__DEADTIME__) <= 0xFFU)

#define IS_TIM_BREAK_SYSTEM(__CONFIG__)    (((__CONFIG__) == TIM_BREAK_SYSTEM_ECC)                  || \
                                            ((__CONFIG__) == TIM_BREAK_SYSTEM_PVD)                  || \
                                            ((__CONFIG__) == TIM_BREAK_SYSTEM_SRAM_PARITY_ERROR)    || \
                                            ((__CONFIG__) == TIM_BREAK_SYSTEM_LOCKUP))

#define IS_TIM_SLAVEMODE_TRIGGER_ENABLED(__TRIGGER__) (((__TRIGGER__) == TIM_SLAVEMODE_TRIGGER) || \
                                                       ((__TRIGGER__) == TIM_SLAVEMODE_COMBINED_RESETTRIGGER))

#define TIM_SET_ICPRESCALERVALUE(__HANDLE__, __CHANNEL__, __ICPSC__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 |= (__ICPSC__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 |= ((__ICPSC__) << 8U)) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 |= (__ICPSC__)) :\
   ((__HANDLE__)->Instance->CCMR2 |= ((__ICPSC__) << 8U)))

#define TIM_RESET_ICPRESCALERVALUE(__HANDLE__, __CHANNEL__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCMR1 &= ~TIM_CCMR1_IC1PSC) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCMR1 &= ~TIM_CCMR1_IC2PSC) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCMR2 &= ~TIM_CCMR2_IC3PSC) :\
   ((__HANDLE__)->Instance->CCMR2 &= ~TIM_CCMR2_IC4PSC))

#define TIM_SET_CAPTUREPOLARITY(__HANDLE__, __CHANNEL__, __POLARITY__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCER |= (__POLARITY__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCER |= ((__POLARITY__) << 4U)) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCER |= ((__POLARITY__) << 8U)) :\
   ((__HANDLE__)->Instance->CCER |= (((__POLARITY__) << 12U))))

#define TIM_RESET_CAPTUREPOLARITY(__HANDLE__, __CHANNEL__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP)) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP)) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC3NP)) :\
   ((__HANDLE__)->Instance->CCER &= ~(TIM_CCER_CC4P | TIM_CCER_CC4NP)))

#define TIM_CHANNEL_STATE_GET(__HANDLE__, __CHANNEL__)\
  (((__CHANNEL__) == TIM_CHANNEL_1) ? (__HANDLE__)->ChannelState[0] :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? (__HANDLE__)->ChannelState[1] :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? (__HANDLE__)->ChannelState[2] :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? (__HANDLE__)->ChannelState[3] :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? (__HANDLE__)->ChannelState[4] :\
   (__HANDLE__)->ChannelState[5])

#define TIM_CHANNEL_STATE_SET(__HANDLE__, __CHANNEL__, __CHANNEL_STATE__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->ChannelState[0] = (__CHANNEL_STATE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->ChannelState[1] = (__CHANNEL_STATE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->ChannelState[2] = (__CHANNEL_STATE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_4) ? ((__HANDLE__)->ChannelState[3] = (__CHANNEL_STATE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_5) ? ((__HANDLE__)->ChannelState[4] = (__CHANNEL_STATE__)) :\
   ((__HANDLE__)->ChannelState[5] = (__CHANNEL_STATE__)))

#define TIM_CHANNEL_STATE_SET_ALL(__HANDLE__,  __CHANNEL_STATE__) do { \
  (__HANDLE__)->ChannelState[0]  = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelState[1]  = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelState[2]  = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelState[3]  = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelState[4]  = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelState[5]  = (__CHANNEL_STATE__);  \
 } while(0)

#define TIM_CHANNEL_N_STATE_GET(__HANDLE__, __CHANNEL__)\
  (((__CHANNEL__) == TIM_CHANNEL_1) ? (__HANDLE__)->ChannelNState[0] :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? (__HANDLE__)->ChannelNState[1] :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? (__HANDLE__)->ChannelNState[2] :\
   (__HANDLE__)->ChannelNState[3])

#define TIM_CHANNEL_N_STATE_SET(__HANDLE__, __CHANNEL__, __CHANNEL_STATE__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->ChannelNState[0] = (__CHANNEL_STATE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->ChannelNState[1] = (__CHANNEL_STATE__)) :\
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->ChannelNState[2] = (__CHANNEL_STATE__)) :\
   ((__HANDLE__)->ChannelNState[3] = (__CHANNEL_STATE__)))

#define TIM_CHANNEL_N_STATE_SET_ALL(__HANDLE__,  __CHANNEL_STATE__) do { \
  (__HANDLE__)->ChannelNState[0] = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelNState[1] = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelNState[2] = (__CHANNEL_STATE__);  \
  (__HANDLE__)->ChannelNState[3] = (__CHANNEL_STATE__);  \
 } while(0)

/**
  * @}
  */
/* End of private macros -----------------------------------------------------*/







/* Exported constants --------------------------------------------------------*/
/** @defgroup TIMEx_Exported_Constants TIM Extended Exported Constants
  * @{
  */

/** @defgroup TIMEx_Remap TIM Extended Remapping
  * @{
  */
#define TIM_ETR_GPIO            0x00000000U                                                 /* !< TIM1_ETR is connected to GPIO */
#define TIM_ETR_COMP0           TIM1_AF1_ETRSEL_0                                           /* !< TIM1_ETR is connected to COMP0 OUT */
#define TIM_ETR_COMP1           TIM1_AF1_ETRSEL_1                                           /* !< TIM1_ETR is connected to COMP1 OUT */
#define TIM_ETR_COMP2           (TIM1_AF1_ETRSEL_1 | TIM1_AF1_ETRSEL_0)                     /* !< TIM1_ETR is connected to COMP2 OUT */
#define TIM_ETR_COMP3           TIM1_AF1_ETRSEL_2                                           /* !< TIM1_ETR is connected to COMP3 OUT */
#define TIM_ETR_CAP_MUX_OUT     (TIM1_AF1_ETRSEL_2 | TIM1_AF1_ETRSEL_0)                     /* !< TIM1_ETR is connected to CAP MUX OUT */
#define TIM_ETR_CLK_MUX_IN      (TIM1_AF1_ETRSEL_2 | TIM1_AF1_ETRSEL_1)                     /* !< TIM1_ETR is connected to CLK MUX IN */
#define TIM_ETR_ADC_WDG         (TIM1_AF1_ETRSEL_2 | TIM1_AF1_ETRSEL_1 | TIM1_AF1_ETRSEL_0) /* !< TIM1_ETR is connected to ADC WDG */

#define TIM_BREAKINPUT_BRK     0x00000001U                                      /* !< Timer break input  */
#define TIM_BREAKINPUT_BRK2    0x00000002U                                      /* !< Timer break2 input */

/**
  * @}
  */

/** @defgroup TIMEx_Break_Input_Source TIM Extended Break input source
  * @{
  */
#define TIM_BREAKINPUTSOURCE_BKIN           0x00000000U                               /* !< An external source (GPIO) is connected to the BKIN pin  */
#define TIM_BREAKINPUTSOURCE_COMP0          0x00000001U                               /* !< The COMP1 output is connected to the break input */
#define TIM_BREAKINPUTSOURCE_COMP1          0x00000002U                               /* !< The COMP2 output is connected to the break input */
#define TIM_BREAKINPUTSOURCE_COMP2          0x00000003U                               /* !< The COMP1 output is connected to the break input */
#define TIM_BREAKINPUTSOURCE_COMP3          0x00000004U                               /* !< The COMP2 output is connected to the break input */
#define TIM_BREAKINPUTSOURCE_CAP_MUX_OUT    0x00000005U                               /* !< The COMP2 output is connected to the break input */
/**
  * @}
  */

/** @defgroup TIMEx_Break_Input_Source_Enable TIM Extended Break input source enabling
  * @{
  */
#define TIM_BREAKINPUTSOURCE_DISABLE     0x00000000U                            /* !< Break input source is disabled */
#define TIM_BREAKINPUTSOURCE_ENABLE      0x00000001U                            /* !< Break input source is enabled */
/**
  * @}
  */

/** @defgroup TIMEx_Break_Input_Source_Polarity TIM Extended Break input polarity
  * @{
  */
#define TIM_BREAKINPUTSOURCE_POLARITY_LOW     0x00000001U                       /* !< Break input source is active low */
#define TIM_BREAKINPUTSOURCE_POLARITY_HIGH    0x00000000U                       /* !< Break input source is active_high */
/**
  * @}
  */

/** @defgroup TIMEx_Timer_Input_Selection TIM Extended Timer input selection
  * @{
  */
//#define TIM_TIM1_TI1_GPIO                          0x00000000U                               /* !< TIM1_TI1 is connected to GPIO */
//#define TIM_TIM1_TI1_COMP1                         TIM_TISEL_TI1SEL_0                        /* !< TIM1_TI1 is connected to COMP1 OUT */
/**
  * @}
  */

/**
  * @}
  */
/* End of exported constants -------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/** @defgroup TIMEx_Private_Macros TIM Extended Private Macros
  * @{
  */
#define IS_TIM_BREAKINPUT(__BREAKINPUT__)  (((__BREAKINPUT__) == TIM_BREAKINPUT_BRK)  || \
                                            ((__BREAKINPUT__) == TIM_BREAKINPUT_BRK2))

#define IS_TIM_BREAKINPUTSOURCE(__SOURCE__)  (((__SOURCE__) == TIM_BREAKINPUTSOURCE_BKIN)  || \
                                              ((__SOURCE__) == TIM_BREAKINPUTSOURCE_COMP0) || \
                                              ((__SOURCE__) == TIM_BREAKINPUTSOURCE_COMP1) || \
                                              ((__SOURCE__) == TIM_BREAKINPUTSOURCE_COMP2) || \
                                              ((__SOURCE__) == TIM_BREAKINPUTSOURCE_COMP3) || \
                                              ((__SOURCE__) == TIM_BREAKINPUTSOURCE_CAP_MUX_OUT))

#define IS_TIM_BREAKINPUTSOURCE_STATE(__STATE__)  (((__STATE__) == TIM_BREAKINPUTSOURCE_DISABLE)  || \
                                                   ((__STATE__) == TIM_BREAKINPUTSOURCE_ENABLE))

#define IS_TIM_BREAKINPUTSOURCE_POLARITY(__POLARITY__)  (((__POLARITY__) == TIM_BREAKINPUTSOURCE_POLARITY_LOW)  || \
                                                         ((__POLARITY__) == TIM_BREAKINPUTSOURCE_POLARITY_HIGH))



#define HAL_TIM_SlaveConfigSynchronization              HAL_TIM_SlaveConfigSynchro
#define HAL_TIM_SlaveConfigSynchronization_IT           HAL_TIM_SlaveConfigSynchro_IT
#define HAL_TIMEx_CommutationCallback                   HAL_TIMEx_CommutCallback
#define HAL_TIMEx_ConfigCommutationEvent                HAL_TIMEx_ConfigCommutEvent
#define HAL_TIMEx_ConfigCommutationEvent_IT             HAL_TIMEx_ConfigCommutEvent_IT
#define HAL_TIMEx_ConfigCommutationEvent_DMA            HAL_TIMEx_ConfigCommutEvent_DMA


#define _IS_TIM_INSTANCE(INSTANCE)       (((INSTANCE) == JL_ADV_TIMER0) \
                                                                        )
/****************** TIM Instances : All supported instances *******************/
#define IS_TIM_INSTANCE(INSTANCE)       _IS_TIM_INSTANCE(INSTANCE)

/************* TIM Instances : at least 1 capture/compare channel *************/
#define IS_TIM_CC1_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/************ TIM Instances : at least 2 capture/compare channels *************/
#define IS_TIM_CC2_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/************ TIM Instances : at least 3 capture/compare channels *************/
#define IS_TIM_CC3_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/************ TIM Instances : at least 4 capture/compare channels *************/
#define IS_TIM_CC4_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/************ TIM Instances : at least 5 capture/compare channels *************/
#define IS_TIM_CC5_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)
/************ TIM Instances : at least 6 capture/compare channels *************/
#define IS_TIM_CC6_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/******************** TIM Instances : Advanced-control timers *****************/
#define IS_TIM_ADVANCED_INSTANCE(INSTANCE) _IS_TIM_INSTANCE(INSTANCE)

/******************** TIM Instances : Advanced-control timers *****************/

/******************* TIM Instances : Timer input XOR function *****************/
#define IS_TIM_XOR_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/****************** TIM Instances : DMA requests generation (UDE) *************/
#define IS_TIM_DMA_INSTANCE(INSTANCE)     _IS_TIM_INSTANCE(INSTANCE)

/************ TIM Instances : DMA requests generation (CCxDE) *****************/
#define IS_TIM_DMA_CC_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/************ TIM Instances : DMA requests generation (COMDE) *****************/
#define IS_TIM_CCDMA_INSTANCE(INSTANCE)    _IS_TIM_INSTANCE(INSTANCE)

/******************** TIM Instances : DMA burst feature ***********************/
#define IS_TIM_DMABURST_INSTANCE(INSTANCE) _IS_TIM_INSTANCE(INSTANCE)

/*************** TIM Instances : external trigger reamp input available *******/
#define IS_TIM_ETR_INSTANCE(INSTANCE)     _IS_TIM_INSTANCE(INSTANCE)

/****************** TIM Instances : remapping capability **********************/
#define IS_TIM_REMAP_INSTANCE(INSTANCE)  _IS_TIM_INSTANCE(INSTANCE)

/*************** TIM Instances : external trigger reamp input available *******/
#define IS_TIM_ETRSEL_INSTANCE(INSTANCE)     _IS_TIM_INSTANCE(INSTANCE)

/****** TIM Instances : master mode available (TIMx_CR2.MMS available )********/
#define IS_TIM_MASTER_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/****** TIM Instances : Salve mode available (TIMx_SMCR.TS available )*********/
#define IS_TIM_SLAVE_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/****** TIM Instances : TRGO2 available (TIMx_CR2.MMS2 available )*********/
#define IS_TIM_TRGO2_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)

/****** TIM Instances : TISEL available (TIMx_TISEL available )*********/
#define IS_TIM_TISEL_INSTANCE(INSTANCE)  _IS_TIM_INSTANCE(INSTANCE)

/****************** TIM Instances : supporting commutation event *************/
#define IS_TIM_COMMUTATION_EVENT_INSTANCE(INSTANCE) _IS_TIM_INSTANCE(INSTANCE)

/****************** TIM Instances : supporting encoder interface **************/
#define IS_TIM_ENCODER_INTERFACE_INSTANCE(INSTANCE)  _IS_TIM_INSTANCE(INSTANCE)

/****** TIM Instances : TIM_CCR5_GC5C available (TIMx_CCR5.GC5C available )*********/
#define IS_TIM_COMBINED3PHASEPWM_INSTANCE(INSTANCE)   _IS_TIM_INSTANCE(INSTANCE)
/******************* TIM Instances : output(s) available **********************/
#define IS_TIM_CCX_INSTANCE(INSTANCE, CHANNEL) \
    ((((INSTANCE) == JL_ADV_TIMER0) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4) ||          \
      ((CHANNEL) == TIM_CHANNEL_5) ||          \
      ((CHANNEL) == TIM_CHANNEL_6)))           \
                                    )

/****************** TIM Instances : supporting the break function *************/
#define IS_TIM_BREAK_INSTANCE(INSTANCE)  _IS_TIM_INSTANCE(INSTANCE)

/************** TIM Instances : supporting Break source selection *************/
#define IS_TIM_BREAKSOURCE_INSTANCE(INSTANCE) _IS_TIM_INSTANCE(INSTANCE)

/****************** TIM Instances : supporting complementary output(s) ********/
#define IS_TIM_CCXN_INSTANCE(INSTANCE, CHANNEL) \
   ((((INSTANCE) == JL_ADV_TIMER0) &&                    \
     (((CHANNEL) == TIM_CHANNEL_1) ||           \
      ((CHANNEL) == TIM_CHANNEL_2) ||           \
      ((CHANNEL) == TIM_CHANNEL_3) ||           \
      ((CHANNEL) == TIM_CHANNEL_4)))            \
                                    )


/****************** TIM Instances : supporting counting mode selection ********/
#define IS_TIM_COUNTER_MODE_SELECT_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting repetition counter *************/
#define IS_TIM_REPETITION_COUNTER_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting synchronization ****************/
#define IS_TIM_SYNCHRO_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting clock division *****************/
#define IS_TIM_CLOCK_DIVISION_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting external clock mode 1 for ETRF input */
#define IS_TIM_CLOCKSOURCE_ETRMODE1_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting external clock mode 2 **********/
#define IS_TIM_CLOCKSOURCE_ETRMODE2_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting external clock mode 1 for TIX inputs*/
#define IS_TIM_CLOCKSOURCE_TIX_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting internal trigger inputs(ITRX) *******/
#define IS_TIM_CLOCKSOURCE_ITRX_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting OCxREF clear *******************/
#define IS_TIM_OCXREF_CLEAR_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : TIM_BKIN2 ***************************/
#define IS_TIM_BKIN2_INSTANCE(INSTANCE)\
  _IS_TIM_INSTANCE(INSTANCE)
/****************** TIM Instances : supporting Hall sensor interface **********/
#define IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(__INSTANCE__) _IS_TIM_INSTANCE(__INSTANCE__)

//使用该模块前必须调用此接口对tmr0/1复位释放
void adv_timer_rst(void);

//打印advtimer所有寄存器值
void adv_timer_dump(JL_ADV_TIMER_TypeDef *htim);

// TIM0 IC1/COMP0 IC2/COMP1 IC3/COMP2 IC4/COMP3
// TIM1 IC1/COMP3 IC2/COMP2 IC3/COMP1 IC4/COMP0
enum timx_tix_sel {
    TIMx_ICx_IN,
    COMP,
};
void TIM_TIx_Sel_SetConfig(TIM_HandleTypeDef *htim, uint32_t Channel, enum timx_tix_sel sel);

void TIM_DMA_SetConfig(TIM_HandleTypeDef *htim, TIM_DMA_InitTypeDef *Structure);

/* Time Base functions ********************************************************/
//HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim);
//HAL_StatusTypeDef HAL_TIM_Base_DeInit(TIM_HandleTypeDef *htim);
/* Exported functions --------------------------------------------------------*/
/** @addtogroup TIM_Exported_Functions TIM Exported Functions
  * @{
  */

/** @addtogroup TIM_Exported_Functions_Group1 TIM Time Base functions
  *  @brief   Time Base functions
  * @{
  */
/* Time Base functions ********************************************************/
HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_DeInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim);
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIM_Base_Start_DMA(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_Stop_DMA(TIM_HandleTypeDef *htim);
/**
  * @}
  */

/** @addtogroup TIM_Exported_Functions_Group2 TIM Output Compare functions
  *  @brief   TIM Output Compare functions
  * @{
  */
/* Timer Output Compare functions *********************************************/
HAL_StatusTypeDef HAL_TIM_OC_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_OC_DeInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim);
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIM_OC_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_OC_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIM_OC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_OC_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIM_OC_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_OC_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */
/** @addtogroup TIM_Exported_Functions_Group3 TIM PWM functions
  *  @brief   TIM PWM functions
  * @{
  */
/* Timer PWM functions ********************************************************/
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_PWM_DeInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef *htim);
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIM_PWM_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIM_PWM_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/* End of exported functions -------------------------------------------------*/


/** @addtogroup TIM_Exported_Functions_Group4 TIM Input Capture functions
  *  @brief   TIM Input Capture functions
  * @{
  */
/* Timer Input Capture functions **********************************************/
HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_IC_DeInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_IC_MspDeInit(TIM_HandleTypeDef *htim);
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIM_IC_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_IC_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_IC_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIM_IC_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_IC_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIM_Exported_Functions_Group5 TIM One Pulse functions
  *  @brief   TIM One Pulse functions
  * @{
  */
/* Timer One Pulse functions **************************************************/
HAL_StatusTypeDef HAL_TIM_OnePulse_Init(TIM_HandleTypeDef *htim, uint32_t OnePulseMode);
HAL_StatusTypeDef HAL_TIM_OnePulse_DeInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_OnePulse_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_OnePulse_MspDeInit(TIM_HandleTypeDef *htim);
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIM_OnePulse_Start(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIM_OnePulse_Stop(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIM_OnePulse_Start_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIM_OnePulse_Stop_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
/**
  * @}
  */

/** @addtogroup TIM_Exported_Functions_Group6 TIM Encoder functions
  *  @brief   TIM Encoder functions
  * @{
  */
/* Timer Encoder functions ****************************************************/
HAL_StatusTypeDef HAL_TIM_Encoder_Init(TIM_HandleTypeDef *htim,  TIM_Encoder_InitTypeDef *sConfig);
HAL_StatusTypeDef HAL_TIM_Encoder_DeInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_Encoder_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIM_Encoder_MspDeInit(TIM_HandleTypeDef *htim);
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_Encoder_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIM_Encoder_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_Encoder_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
//HAL_StatusTypeDef HAL_TIM_Encoder_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel, uint32_t *pData1,
//                                           uint32_t *pData2, uint16_t Length);
//HAL_StatusTypeDef HAL_TIM_Encoder_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */


/** @defgroup TIM_Exported_Functions_Group8 TIM Peripheral Control functions
  *  @brief   Peripheral Control functions
  * @{
  */
/* Control functions  *********************************************************/
HAL_StatusTypeDef HAL_TIM_OC_ConfigChannel(TIM_HandleTypeDef *htim, TIM_OC_InitTypeDef *sConfig, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim, TIM_OC_InitTypeDef *sConfig, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_IC_ConfigChannel(TIM_HandleTypeDef *htim, TIM_IC_InitTypeDef *sConfig, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_OnePulse_ConfigChannel(TIM_HandleTypeDef *htim, TIM_OnePulse_InitTypeDef *sConfig,
        uint32_t OutputChannel,  uint32_t InputChannel);
HAL_StatusTypeDef HAL_TIM_ConfigOCrefClear(TIM_HandleTypeDef *htim, TIM_ClearInputConfigTypeDef *sClearInputConfig,
        uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_ConfigClockSource(TIM_HandleTypeDef *htim, TIM_ClockConfigTypeDef *sClockSourceConfig);
HAL_StatusTypeDef HAL_TIM_ConfigTI1Input(TIM_HandleTypeDef *htim, uint32_t TI1_Selection);
HAL_StatusTypeDef HAL_TIM_SlaveConfigSynchro(TIM_HandleTypeDef *htim, TIM_SlaveConfigTypeDef *sSlaveConfig);
HAL_StatusTypeDef HAL_TIM_SlaveConfigSynchro_IT(TIM_HandleTypeDef *htim, TIM_SlaveConfigTypeDef *sSlaveConfig);
HAL_StatusTypeDef HAL_TIM_DMABurst_WriteStart(TIM_HandleTypeDef *htim, uint32_t BurstRequestSrc);
HAL_StatusTypeDef HAL_TIM_DMABurst_MultiWriteStart(TIM_HandleTypeDef *htim, uint32_t BurstRequestSrc);
HAL_StatusTypeDef HAL_TIM_DMABurst_WriteStop(TIM_HandleTypeDef *htim, uint32_t BurstRequestSrc);
HAL_StatusTypeDef HAL_TIM_DMABurst_ReadStart(TIM_HandleTypeDef *htim, uint32_t BurstRequestSrc);
HAL_StatusTypeDef HAL_TIM_DMABurst_MultiReadStart(TIM_HandleTypeDef *htim, uint32_t BurstRequestSrc);
HAL_StatusTypeDef HAL_TIM_DMABurst_ReadStop(TIM_HandleTypeDef *htim, uint32_t BurstRequestSrc);
HAL_StatusTypeDef HAL_TIM_GenerateEvent(TIM_HandleTypeDef *htim, uint32_t EventSource);
uint32_t HAL_TIM_ReadCapturedValue(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef TIM_SlaveTimer_SetConfig(TIM_HandleTypeDef *htim,
        TIM_SlaveConfigTypeDef *sSlaveConfig);

/** @addtogroup TIM_Exported_Functions_Group7 TIM IRQ handler management
  *  @brief   IRQ handler management
  * @{
  */
/* Interrupt Handler functions  ***********************************************/
void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim);

/**
  * @}
  */

/** @defgroup TIM_Exported_Functions_Group9 TIM Callbacks functions
  *  @brief   TIM Callbacks functions
  * @{
  */
/* Callback in non blocking modes (Interrupt and DMA) *************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PeriodElapsedHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_TriggerCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_TriggerHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_CommutCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_CommutHalfCpltCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_BreakCallback(TIM_HandleTypeDef *htim);
void HAL_TIMEx_Break2Callback(TIM_HandleTypeDef *htim);

/* Callbacks Register/UnRegister functions  ***********************************/
#if (USE_HAL_TIM_REGISTER_CALLBACKS == 1)
HAL_StatusTypeDef HAL_TIM_RegisterCallback(TIM_HandleTypeDef *htim, HAL_TIM_CallbackIDTypeDef CallbackID,
        pTIM_CallbackTypeDef pCallback);
HAL_StatusTypeDef HAL_TIM_UnRegisterCallback(TIM_HandleTypeDef *htim, HAL_TIM_CallbackIDTypeDef CallbackID);

void TIM_ResetCallback(TIM_HandleTypeDef *htim);
#endif /* USE_HAL_TIM_REGISTER_CALLBACKS */

/** @defgroup TIM_Exported_Functions_Group10 TIM Peripheral State functions
  *  @brief  Peripheral State functions
  * @{
  */
/* Peripheral State functions  ************************************************/
HAL_TIM_StateTypeDef HAL_TIM_Base_GetState(TIM_HandleTypeDef *htim);
HAL_TIM_StateTypeDef HAL_TIM_OC_GetState(TIM_HandleTypeDef *htim);
HAL_TIM_StateTypeDef HAL_TIM_PWM_GetState(TIM_HandleTypeDef *htim);
HAL_TIM_StateTypeDef HAL_TIM_IC_GetState(TIM_HandleTypeDef *htim);
HAL_TIM_StateTypeDef HAL_TIM_OnePulse_GetState(TIM_HandleTypeDef *htim);
HAL_TIM_StateTypeDef HAL_TIM_Encoder_GetState(TIM_HandleTypeDef *htim);

/* Peripheral Channel state functions  ************************************************/
HAL_TIM_ActiveChannel HAL_TIM_GetActiveChannel(TIM_HandleTypeDef *htim);
HAL_TIM_ChannelStateTypeDef HAL_TIM_GetChannelState(TIM_HandleTypeDef *htim,  uint32_t Channel);
HAL_TIM_DMABurstStateTypeDef HAL_TIM_DMABurstState(TIM_HandleTypeDef *htim);
/**
  * @}
  */


/* Private functions----------------------------------------------------------*/
/** @defgroup TIM_Private_Functions TIM Private Functions
  * @{
  */
void TIM_CCxChannelCmd(JL_ADV_TIMER_TypeDef *TIMx, uint32_t Channel, uint32_t ChannelState);
void TIM_CCxNChannelCmd(JL_ADV_TIMER_TypeDef *TIMx, uint32_t Channel, uint32_t ChannelNState);
void TIM_Base_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_Base_InitTypeDef *Structure);
void TIM_ETR_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ExtTRGPrescaler,
                       uint32_t TIM_ExtTRGPolarity, uint32_t ExtTRGFilter);
void TIM_OC1_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);
void TIM_OC2_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);
void TIM_OC3_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);
void TIM_OC4_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);
void TIM_OC5_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);
void TIM_OC6_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, TIM_OC_InitTypeDef *OC_Config);
void TIM_TI1_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ICPolarity, uint32_t TIM_ICSelection, uint32_t TIM_ICFilter);
void TIM_TI2_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ICPolarity, uint32_t TIM_ICSelection,
                       uint32_t TIM_ICFilter);
void TIM_TI3_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ICPolarity, uint32_t TIM_ICSelection,
                       uint32_t TIM_ICFilter);
void TIM_TI4_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ICPolarity, uint32_t TIM_ICSelection,
                       uint32_t TIM_ICFilter);
void TIM_TI1_ConfigInputStage(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ICPolarity, uint32_t TIM_ICFilter);
void TIM_ITRx_SetConfig(JL_ADV_TIMER_TypeDef *TIMx, uint32_t InputTriggerSource);
void TIM_TI2_ConfigInputStage(JL_ADV_TIMER_TypeDef *TIMx, uint32_t TIM_ICPolarity, uint32_t TIM_ICFilter);
//void TIM_DMADelayPulseHalfCplt(DMA_HandleTypeDef *hdma);
//void TIM_DMAError(DMA_HandleTypeDef *hdma);
//void TIM_DMACaptureCplt(DMA_HandleTypeDef *hdma);
//void TIM_DMACaptureHalfCplt(DMA_HandleTypeDef *hdma);
/**
  * @}
  */
/* Exported functions --------------------------------------------------------*/
/** @addtogroup TIMEx_Exported_Functions TIM Extended Exported Functions
  * @{
  */

/** @addtogroup TIMEx_Exported_Functions_Group1 Extended Timer Hall Sensor functions
  *  @brief    Timer Hall Sensor functions
  * @{
  */
/*  Timer Hall Sensor functions  **********************************************/
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Init(TIM_HandleTypeDef *htim, TIM_HallSensor_InitTypeDef *sConfig);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_DeInit(TIM_HandleTypeDef *htim);

//void HAL_TIMEx_HallSensor_MspInit(TIM_HandleTypeDef *htim);
//void HAL_TIMEx_HallSensor_MspDeInit(TIM_HandleTypeDef *htim);

/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_IT(TIM_HandleTypeDef *htim);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Start_DMA(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIMEx_HallSensor_Stop_DMA(TIM_HandleTypeDef *htim);
/**
  * @}
  */
/** @addtogroup TIMEx_Exported_Functions_Group2 Extended Timer Complementary Output Compare functions
  *  @brief   Timer Complementary Output Compare functions
  * @{
  */
/*  Timer Complementary Output Compare functions  *****************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_OCN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_OCN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group3 Extended Timer Complementary PWM functions
  *  @brief    Timer Complementary PWM functions
  * @{
  */
/*  Timer Complementary PWM functions  ****************************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel);
/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop_DMA(TIM_HandleTypeDef *htim, uint32_t Channel);
/**
  * @}
  */

/** @addtogroup TIMEx_Exported_Functions_Group4 Extended Timer Complementary One Pulse functions
  *  @brief    Timer Complementary One Pulse functions
  * @{
  */
/*  Timer Complementary One Pulse functions  **********************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop(TIM_HandleTypeDef *htim, uint32_t OutputChannel);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Start_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
HAL_StatusTypeDef HAL_TIMEx_OnePulseN_Stop_IT(TIM_HandleTypeDef *htim, uint32_t OutputChannel);
/**
  * @}
  */
/* End of private functions --------------------------------------------------*/

/** @addtogroup TIMEx_Exported_Functions_Group5 Extended Peripheral Control functions
  *  @brief    Peripheral Control functions
  * @{
  */
/* Extended Control functions  ************************************************/
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutEvent(TIM_HandleTypeDef *htim, uint32_t  InputTrigger,
        uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutEvent_IT(TIM_HandleTypeDef *htim, uint32_t  InputTrigger,
        uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_ConfigCommutEvent_DMA(TIM_HandleTypeDef *htim, uint32_t  InputTrigger,
        uint32_t  CommutationSource);
HAL_StatusTypeDef HAL_TIMEx_MasterConfigSynchronization(TIM_HandleTypeDef *htim,
        TIM_MasterConfigTypeDef *sMasterConfig);
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakDeadTime(TIM_HandleTypeDef *htim,
        TIM_BreakDeadTimeConfigTypeDef *sBreakDeadTimeConfig);
HAL_StatusTypeDef HAL_TIMEx_ConfigBreakInput(TIM_HandleTypeDef *htim, uint32_t BreakInput,
        TIMEx_BreakInputConfigTypeDef *sBreakInputConfig);
HAL_StatusTypeDef HAL_TIMEx_GroupChannel5(TIM_HandleTypeDef *htim, uint32_t Channels);
HAL_StatusTypeDef HAL_TIMEx_RemapConfig(TIM_HandleTypeDef *htim, uint32_t Remap);
HAL_StatusTypeDef  HAL_TIMEx_TISelection(TIM_HandleTypeDef *htim, uint32_t TISelection, uint32_t Channel);
/**
  * @}
  */

/* End of private functions --------------------------------------------------*/
#endif

