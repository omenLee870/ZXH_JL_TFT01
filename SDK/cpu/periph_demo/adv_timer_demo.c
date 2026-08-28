#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adv_timer_demo.data.bss")
#pragma data_seg(".adv_timer_demo.data")
#pragma const_seg(".adv_timer_demo.text.const")
#pragma code_seg(".adv_timer_demo.text")
#endif
#include "asm/adv_timer.h"
#include "clock.h"
#include "debug.h"
#include "timer.h"
#include "gpio.h"

#define LOG_TAG_CONST   ADVTMR
#define LOG_TAG         "[advtmr_demo]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

extern void wdt_clear();


#define TEST_TIMEBASE               0
#define TEST_PWMOUTPUT              1
#define TEST_ONEPLUSE               2
#define TEST_OCTOGGLE               3
#define TEST_INPUTCAPTURE           4
#define TEST_DMA_DCR_UNLOCK         5
#define TEST_DMA_BURST              6
#define TEST_COMPLEMENTARYSIGNALS   7
#define TEST_COMBINE                8
#define TEST_6STEPS                 9
#define TEST_ENCODE                 10
#define TEST_ETR_CLR                11
#define TEST_PWMINPUT               12
#define TEST_EXT_CLOCK1             13
#define TEST_LIST                   TEST_TIMEBASE



TIM_HandleTypeDef TimHandle0;

___interrupt
static void adv_timer0_irq_callback(void)
{
    HAL_TIM_IRQHandler(&TimHandle0);
    wdt_clear();
}

#if (TEST_LIST == TEST_TIMEBASE)
// TIM_TimeBase
/*
 * 测试项：时基单元测试
 * 基本配置：系统时钟48MHz，tmr分频时钟1kHz，ARR=1000，RCR = 5，使能更新中断
 * 输出：每隔6s产生一次更新中断，打印一次sr
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    log_info("tim update");
}

static void timebase_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 1000 - 1;
    TimHandle0.Init.Prescaler         = (48000000L / 1000) - 1; // 1kHz
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHandle0.Init.RepetitionCounter = 5;

    HAL_TIM_Base_Init(&TimHandle0);

    HAL_TIM_Base_Start_IT(&TimHandle0);
}
#endif

#if (TEST_LIST == TEST_PWMOUTPUT)
// TIM_PWMOutput
/*
 * 测试项：PWM输出测试
 * 基本配置：系统时钟48MHz，tmr0 4个ch配置不同CRC控制占空比
 * 输出：4路占空比不一致的PWM对管波形。
 */
#define  PERIOD_VALUE    (u32)(1000 - 1)
#define  PULSE1_VALUE   (u32)(PERIOD_VALUE / 2)
#define  PULSE2_VALUE   (u32)(PERIOD_VALUE / 3)
#define  PULSE3_VALUE   (u32)(PERIOD_VALUE / 4)
#define  PULSE4_VALUE   (u32)(PERIOD_VALUE / 5)
static void pwmoutput_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = PERIOD_VALUE;
    TimHandle0.Init.Prescaler         = (48000000L / 48000000) - 1;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;

    HAL_TIM_PWM_Init(&TimHandle0);

    TIM_OC_InitTypeDef sConfig = {
        .OCMode         = TIM_OCMODE_PWM1,      // 输出比较模式
        .OCPolarity     = TIM_OCPOLARITY_HIGH,     // 输出有效电平极性
        .OCFastMode      = TIM_OCFAST_DISABLE,
        .OCNPolarity     = TIM_OCPOLARITY_HIGH,     // 互补输出有效电平极性
        .OCNIdleState     = TIM_OCNIDLESTATE_RESET,
        .OCIdleState      = TIM_OCIDLESTATE_RESET,
    };

    sConfig.Pulse = PULSE1_VALUE;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_1);

    sConfig.Pulse = PULSE2_VALUE;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_2);

    sConfig.Pulse = PULSE3_VALUE;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_3);

    sConfig.Pulse = PULSE4_VALUE;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);
    TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_1, TIM_CCxN_ENABLE);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_2);
    TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_2, TIM_CCxN_ENABLE);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_3);
    TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_3, TIM_CCxN_ENABLE);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_4);
    TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_4, TIM_CCxN_ENABLE);

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC1N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_05), PORT_FUNC_TMR0_OC2P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_06), PORT_FUNC_TMR0_OC2N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_07), PORT_FUNC_TMR0_OC3P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_08), PORT_FUNC_TMR0_OC3N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_09), PORT_FUNC_TMR0_OC4P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_10), PORT_FUNC_TMR0_OC4N);
}



#if 0
void pwmoutput_decimals_test(void)
{
    log_info("%s", __FUNCTION__);

    void clk_set_en(u8 en);
    u32 pll_frac_switch(u32 div, u32 nr);
    clk_set_en(1);
    clk_set_api("lsb", 96000000);
    clk_set_en(0);
    u32 pll_freq = 0;//471000
    pll_freq = pll_frac_switch(120, 961); //471000
    printf("******pll_freq:%d\n", pll_freq);

    u32 target_freq = 471000;
    /* u32 target_freq = 485000; */
    u32 src_freq = clk_get("lsb");
    u32 src_div = 1;

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = src_freq / src_div / target_freq;
    TimHandle0.Init.Prescaler         = src_div - 1;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;

    HAL_TIM_PWM_Init(&TimHandle0);

    TIM_OC_InitTypeDef sConfig = {
        .OCMode         = TIM_OCMODE_PWM1,      // 输出比较模式
        .OCPolarity     = TIM_OCPOLARITY_HIGH,     // 输出有效电平极性
        .OCFastMode      = TIM_OCFAST_DISABLE,
        .OCNPolarity     = TIM_OCPOLARITY_HIGH,     // 互补输出有效电平极性
        .OCNIdleState     = TIM_OCNIDLESTATE_RESET,
        .OCIdleState      = TIM_OCIDLESTATE_RESET,
    };

    sConfig.Pulse = TimHandle0.Init.Period / 2;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_1);
    /* sConfig.Pulse = TimHandle0.Init.Period/3; */
    /* HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_2); */
    /* sConfig.Pulse = TimHandle0.Init.Period/4; */
    /* HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_3); */
    /* sConfig.Pulse = TimHandle0.Init.Period/5; */
    /* HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_4); */

    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);
    TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_1, TIM_CCxN_ENABLE);
    /* HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_2); */
    /* TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_2, TIM_CCxN_ENABLE); */
    /* HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_3); */
    /* TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_3, TIM_CCxN_ENABLE); */
    /* HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_4); */
    /* TIM_CCxNChannelCmd(TimHandle0.Instance, TIM_CHANNEL_4, TIM_CCxN_ENABLE); */

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_01), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC1N);
    /* gpio_set_function(IO_PORT_SPILT(IO_PORTA_05), PORT_FUNC_TMR0_OC2P); */
    /* gpio_set_function(IO_PORT_SPILT(IO_PORTA_06), PORT_FUNC_TMR0_OC2N); */
    /* gpio_set_function(IO_PORT_SPILT(IO_PORTA_07), PORT_FUNC_TMR0_OC3P); */
    /* gpio_set_function(IO_PORT_SPILT(IO_PORTA_08), PORT_FUNC_TMR0_OC3N); */
    /* gpio_set_function(IO_PORT_SPILT(IO_PORTA_09), PORT_FUNC_TMR0_OC4P); */
    /* gpio_set_function(IO_PORT_SPILT(IO_PORTA_10), PORT_FUNC_TMR0_OC4N); */
}
#endif
#endif

#if (TEST_LIST == TEST_ONEPLUSE)
// TIM_OnePulse
/*
 * 测试项：单脉冲测试
 * 基本配置：系统时钟48MHz，tmr 捕获ch2输入上升沿触发ch1输出单脉冲
 * 脉冲延迟输出时间和脉宽宽度可配置
 * 输出：捕获ch2输入上升沿触发ch1输出单脉冲
 */
static void onepulse_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC2);
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_03), PORT_INPUT_FLOATING);

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 65535;
    TimHandle0.Init.Prescaler         = (48000000L / (2 * 24000000)) - 1;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;

    HAL_TIM_OnePulse_Init(&TimHandle0, TIM_OPMODE_SINGLE);

    TIM_OnePulse_InitTypeDef sConfig = {
        .OCMode       = TIM_OCMODE_PWM2,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        // delay=16383/24000000[sec], pulse=(65535-16383)/24000000[sec]
        .Pulse        = 16383,
        .ICPolarity   = TIM_ICPOLARITY_RISING,
        .ICSelection  = TIM_ICSELECTION_DIRECTTI,
        .ICFilter     = 0,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCIdleState  = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
    };

    HAL_TIM_OnePulse_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_1, TIM_CHANNEL_2);

    HAL_TIM_OnePulse_Start(&TimHandle0, TIM_CHANNEL_1);
}
#endif

#if (TEST_LIST == TEST_OCTOGGLE)
// TIM_OCToggle
/*
 * 测试项：翻转测试
 * 基本配置：系统时钟48MHz，ARR=65535，当CNT=CCR时，OCREF翻转
 * 输出：ch1输出582.92kHz的pwm
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    // Fo = 24MHz/40960 = 585.92kHz
    u32 ccr1 = HAL_TIM_ReadCapturedValue(&TimHandle0, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&TimHandle0, TIM_CHANNEL_1, (ccr1 + 40960));
}

static void octoggle_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 65535;
    TimHandle0.Init.Prescaler         = (48000000L / (2 * 24000000)) - 1; // 24MHz
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;

    HAL_TIM_OC_Init(&TimHandle0);

    TIM_OC_InitTypeDef sConfig = {
        .OCMode     = TIM_OCMODE_TOGGLE,      // 输出比较模式-翻转
        .OCPolarity = TIM_OCPOLARITY_HIGH,     // 输出有效电平极性
        .OCNPolarity = TIM_OCPOLARITY_HIGH,     // 互补输出有效电平极性
    };

    // 输出比较配置
    sConfig.Pulse = 40960; // CCR1
    HAL_TIM_OC_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_1);

    // 启动输出
    HAL_TIM_OC_Start_IT(&TimHandle0, TIM_CHANNEL_1);
    HAL_TIMEx_OCN_Start_IT(&TimHandle0, TIM_CHANNEL_1);

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC1N);
}
#endif

#if (TEST_LIST == TEST_INPUTCAPTURE)
// TIM_InputCapture
/*
 * 测试项：输入捕获测试
 * 基本配置：系统时钟1kHz，ARR=1000，一个输入捕获通道, 一个输出pwm波形，ccrx 500
 * 将输入和输出通道短接
 * 输出：根据配置输出边沿捕获cnt
 */
#define INPUT_TIMx              JL_ADV_TIMER0
#define INPUT_CAPTURE_EDGE      TIM_ICPOLARITY_RISING
#define INPUT_CHx               TIM_CHANNEL_2
#define OUTPUT_CHx              TIM_CHANNEL_1
#define INPUT_ICxPSC            TIM_ICPSC_DIV1

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
#if (INPUT_CHx == TIM_CHANNEL_1)
    log_info("%d\n", INPUT_TIMx->CCR1);
#elif (INPUT_CHx == TIM_CHANNEL_2)
    log_info("%d\n", INPUT_TIMx->CCR2);
#elif (INPUT_CHx == TIM_CHANNEL_3)
    log_info("%d\n", INPUT_TIMx->CCR3);
#elif (INPUT_CHx == TIM_CHANNEL_4)
    log_info("%d\n", INPUT_TIMx->CCR4);
#endif
}

static void inputcapture_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

#if (OUTPUT_CHx == TIM_CHANNEL_1)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
#elif (OUTPUT_CHx == TIM_CHANNEL_2)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC2P);
#elif (OUTPUT_CHx == TIM_CHANNEL_3)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC3P);
#elif (OUTPUT_CHx == TIM_CHANNEL_4)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC4P);
#endif

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_03), PORT_INPUT_FLOATING);
#if (INPUT_CHx == TIM_CHANNEL_1)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC1);
#elif (INPUT_CHx == TIM_CHANNEL_2)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC2);
#elif (INPUT_CHx == TIM_CHANNEL_3)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC3);
#elif (INPUT_CHx == TIM_CHANNEL_4)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC4);
#endif

    TimHandle0.Instance               = INPUT_TIMx;
    TimHandle0.Init.Period            = 1000;
    TimHandle0.Init.Prescaler         = 48000000L / 1000 - 1;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;

    HAL_TIM_IC_Init(&TimHandle0);

    TIM_IC_InitTypeDef sICConfig = {
        .ICPolarity  = INPUT_CAPTURE_EDGE,
        .ICSelection = TIM_ICSELECTION_DIRECTTI,
        .ICPrescaler = INPUT_ICxPSC,
        .ICFilter    = 0,
    };
    HAL_TIM_IC_ConfigChannel(&TimHandle0, &sICConfig, INPUT_CHx);

    TIM_OC_InitTypeDef sConfig = {
        .Pulse            = 500,
        .OCMode         = TIM_OCMODE_PWM2,      // 输出比较模式
        .OCPolarity     = TIM_OCPOLARITY_HIGH,     // 输出有效电平极性
        .OCFastMode      = TIM_OCFAST_DISABLE,
        .OCNPolarity     = TIM_OCPOLARITY_HIGH,     // 互补输出有效电平极性
        .OCNIdleState     = TIM_OCNIDLESTATE_RESET,
        .OCIdleState      = TIM_OCIDLESTATE_RESET,
    };
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, OUTPUT_CHx);

    HAL_TIM_IC_Start_IT(&TimHandle0, INPUT_CHx);

    HAL_TIM_PWM_Start(&TimHandle0, OUTPUT_CHx);
}
#endif

#if (TEST_LIST == TEST_DMA_DCR_UNLOCK)
// DMA_DCR_UNLOCK
/*
 * 测试项：DMA DCR UNLOCK锁测试
 * 分别开关锁 对DCR DMAR进行读写 查看是否锁成功
*/
void dma_dcr_unlock_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    JL_ADV_TIMER0->DCR &= ~TIM_DCR_UNLOCK;
    JL_ADV_TIMER0->DCR |= 0xffffffdf;
    JL_ADV_TIMER0->DMAR = 0xffffffff;
    log_info("0x%x 0x%x", JL_ADV_TIMER0->DCR, JL_ADV_TIMER0->DMAR);
    JL_ADV_TIMER0->DCR |= TIM_DCR_UNLOCK;
    JL_ADV_TIMER0->DCR |= 0xffffffdf;
    JL_ADV_TIMER0->DMAR = 0xffffffff;
    log_info("0x%x 0x%x", JL_ADV_TIMER0->DCR, JL_ADV_TIMER0->DMAR);
}
#endif

#if (TEST_LIST == TEST_DMA_BURST)
// DMA_BURST
/*
 * 测试项：DMA连续传送读写测试
 * 基本配置：配置以下宏进行测试，mem初始化为6个word,分别是1 2 3 4 5 6
 * CCR1-CCR6分别为111 222 333 444 555 666，寄存器操作偏移是CCR1
 * 输出:
 * 1)DMA_WR_TEST=TIM_DMA_WRITE, DMA_ROUND_EN=TIM_DMA_ROUND_DISABLE,DMA_DBL=3,DMA_DOL=4
 * 打印：mem为1 2 3 4 5 6，ccr为1 2 3 4 555 666
 * 2)DMA_WR_TEST=TIM_DMA_WRITE, DMA_ROUND_EN=TIM_DMA_ROUND_ENABLE,DMA_DBL=3,DMA_DOL=4
 * 打印：mem为1 2 3 4 5 6，ccr为1 2 3 4 555 666/5 1 2 3 555 666/4 1 2 3 555 666...循环
 * 3)DMA_WR_TEST=TIM_DMA_READ, DMA_ROUND_EN=TIM_DMA_ROUND_DISABLE,DMA_DBL=3,DMA_DOL=4
 * 打印：mem为111 222 333 444 5 6，ccr为111 222 333 444 555 666
 * 4)DMA_WR_TEST=TIM_DMA_READ, DMA_ROUND_EN=TIM_DMA_ROUND_ENABLE,DMA_DBL=3,DMA_DOL=4
 * 打印：mem为111 222 333 444 5 6/222 333 444 444 111 6/333 444 444 111 222 6/444 444 111 222 333 6...循环
 *，ccr为111 222 333 444 555 666
 */
#define DMA_TMx             JL_ADV_TIMER0
#define DMA_TMx_CHx         TIM_CHANNEL_1 //测试CC1-CC4触发DMA请求
#define DMA_WR_TEST         TIM_DMA_READ
#define DMA_ROUND_EN        TIM_DMA_ROUND_ENABLE
#define DMA_DBL             3 //ADV_TIMER寄存器读/写长度(DMA_DBL+1)
#define DMA_DOL             4 //MEM读/写长度(DMA_DOL+1)
#define COMDE_TEST          0 //COM事件触发DMA请求测试
#define TDE_TEST            0 //触发事件触发DMA请求测试
#define UDE_TEST            0 //更新事件触发DMA请求测试
#define CCDS_TEST           0 //CCDS更新事件发送CCx DMA请求测试
static void dmaburst_test(void)
{
    log_info("%s", __FUNCTION__);

    /* u32 *mem = malloc(8 * 4); */
    u32 mem_buf[8];
    u32 *mem = mem_buf;
    mem[0] = 1;
    mem[1] = 2;
    mem[2] = 3;
    mem[3] = 4;
    mem[4] = 5;
    mem[5] = 6;
    log_info("0x%x", (u32)mem);

    adv_timer_rst();

    TimHandle0.Instance               = DMA_TMx;
    TimHandle0.Init.Period            = 65535;
    TimHandle0.Init.RepetitionCounter = 0;
    TimHandle0.Init.Prescaler         = 0;
    TimHandle0.Init.ClockDivision     = 0;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&TimHandle0);

    TIM_OC_InitTypeDef sConfig = {
        .OCMode       = TIM_OCMODE_PWM1,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        .Pulse        = 0,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCFastMode   = TIM_OCFAST_DISABLE,
        .OCIdleState  = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
    };
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, DMA_TMx_CHx);

    TIM_DMA_InitTypeDef DMAStructure = {
        .DBA = TIM_DMABASE_CCR1,
        .OP = DMA_WR_TEST,
        .DmaRoundEn = DMA_ROUND_EN,
        .DBL = DMA_DBL,
        .ADR = (u32)mem,
        .DOL = DMA_DOL,
    };
    TIM_DMA_SetConfig(&TimHandle0, &DMAStructure);

    DMA_TMx->CCR1 = 111;
    DMA_TMx->CCR2 = 222;
    DMA_TMx->CCR3 = 333;
    DMA_TMx->CCR4 = 444;
    DMA_TMx->CCR5 = 555;
    DMA_TMx->CCR6 = 666;

    HAL_TIM_PWM_Start_DMA(&TimHandle0, DMA_TMx_CHx);

#if COMDE_TEST
    __HAL_TIM_DISABLE_DMA(&TimHandle0, TIM_DMA_CC1);
    __HAL_TIM_ENABLE_DMA(&TimHandle0, TIM_DMA_COM);
    HAL_TIM_GenerateEvent(&TimHandle0, TIM_EVENTSOURCE_COM);
#elif TDE_TEST
    __HAL_TIM_DISABLE_DMA(&TimHandle0, TIM_DMA_CC1);
    __HAL_TIM_ENABLE_DMA(&TimHandle0, TIM_DMA_TRIGGER);
    HAL_TIM_GenerateEvent(&TimHandle0, TIM_EVENTSOURCE_TRIGGER);
#elif UDE_TEST
    __HAL_TIM_DISABLE_DMA(&TimHandle0, TIM_DMA_CC1);
    __HAL_TIM_ENABLE_DMA(&TimHandle0, TIM_DMA_UPDATE);
#elif CCDS_TEST
    __HAL_TIM_DMA_SELECTION(&TimHandle0, TIM_DMA_SEL_UDE);
#endif

    while (1) {
        wdt_clear();
        mdelay(100);
        log_info("%d %d %d %d %d %d\n", mem[0], mem[1], mem[2], mem[3], mem[4], mem[5]);
        log_info("%d %d %d %d %d %d\n", DMA_TMx->CCR1, DMA_TMx->CCR2, DMA_TMx->CCR3, DMA_TMx->CCR4, DMA_TMx->CCR5, DMA_TMx->CCR6);
    };
}
#endif

#if (TEST_LIST == TEST_COMPLEMENTARYSIGNALS)
// TIM_ComplementarySignals
/*
 * 测试项：互补输出和死区插入测试
 * 基本配置：系统时钟48MHz，tmr0三个通道分别配置不同的占空比，互补输入，插入死区
 * 输出：输出插入死区时间的互补pwm
 */
#define CCPC_SET    0 //CCPC预装载测试
static void complementarysignals_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 2000 - 1;
    TimHandle0.Init.Prescaler         = 0;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&TimHandle0);

    TIM_OC_InitTypeDef sPWMConfig = {
        .OCMode       = TIM_OCMODE_PWM1,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCIdleState  = TIM_OCIDLESTATE_SET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
        .OCFastMode   = TIM_OCFAST_DISABLE,
    };

#if CCPC_SET
    JL_ADV_TIMER0->CR2 |= TIM_CR2_CCPC;
#endif

    sPWMConfig.Pulse = 2000 / 2;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig, TIM_CHANNEL_1);

    sPWMConfig.Pulse = 2000 / 4;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig, TIM_CHANNEL_2);

    sPWMConfig.Pulse = 2000 / 8;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig, TIM_CHANNEL_3);

    TIM_BreakDeadTimeConfigTypeDef sBreakConfig = {
        .BreakState       = TIM_BREAK_DISABLE,
        .DeadTime         = 100,
        .OffStateRunMode  = TIM_OSSR_ENABLE,
        .OffStateIDLEMode = TIM_OSSI_ENABLE,
        .LockLevel        = TIM_LOCKLEVEL_1,
        .BreakPolarity    = TIM_BREAKPOLARITY_HIGH,
        .BreakFilter      = 0,
        .AutomaticOutput  = TIM_AUTOMATICOUTPUT_ENABLE,
    };

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_09), PORT_INPUT_FLOATING);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_09), PORT_FUNC_TMR0_BRK_IN1);
    HAL_TIMEx_ConfigBreakDeadTime(&TimHandle0, &sBreakConfig);

    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);
    HAL_TIMEx_PWMN_Start(&TimHandle0, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_2);
    HAL_TIMEx_PWMN_Start(&TimHandle0, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_3);
    HAL_TIMEx_PWMN_Start(&TimHandle0, TIM_CHANNEL_3);

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC1N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_05), PORT_FUNC_TMR0_OC2P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_06), PORT_FUNC_TMR0_OC2N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_07), PORT_FUNC_TMR0_OC3P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_08), PORT_FUNC_TMR0_OC3N);

#if CCPC_SET
    HAL_TIM_GenerateEvent(&TimHandle0, TIM_EVENTSOURCE_COM);
#endif
}
#endif

#if (TEST_LIST == TEST_COMBINE)
// TIM_Combined
/*
 * 测试项：组合三相PWM测试
 * 基本配置：系统时钟48MHz，tmr0三个通道分别配置不同的占空比；使能GC5C，
 * 三个通道输出与CCR5相关，ch1-3选择PWM1模式，ch5选择PWM2模式
 * 输出：输出与OC5REF相与后的pwm波形，波形中心对齐，相与部分相当于挖洞
 */
static void combined_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    u32 TimerPeriod = (48000000 / (8780 * 2)) - 1;  // 8.78kHz
    u16 Channel1Pulse = (5 * (TimerPeriod - 1)) / 10; // 50%
    u16 Channel2Pulse = (375 * (TimerPeriod - 1)) / 1000; // 37.5%
    u16 Channel3Pulse = (25 * (TimerPeriod - 1)) / 100; // 25%
    u16 Channel5Pulse = (125 * (TimerPeriod - 1)) / 1000; //12.5%

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = TimerPeriod;
    TimHandle0.Init.Prescaler         = 0;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_CENTERALIGNED1;
    HAL_TIM_PWM_Init(&TimHandle0);

    TIM_OC_InitTypeDef sConfig = {
        .OCMode = TIM_OCMODE_PWM1,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCNPolarity = TIM_OCNPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
        .OCIdleState = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
    };

    sConfig.Pulse = Channel1Pulse;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_1);

    sConfig.Pulse = Channel2Pulse;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_2);

    sConfig.Pulse = Channel3Pulse;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_3);

    sConfig.OCMode = TIM_OCMODE_PWM2;
    sConfig.Pulse = Channel5Pulse;
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, TIM_CHANNEL_5);

    //GC5C1置1，OC1REFC由TIMx_CCR1和TIMx_CCR5控制，OC1REFC = OC1REF & OC5REF
    //GC5C2置1，OC2REFC由TIMx_CCR2和TIMx_CCR5控制，OC2REFC = OC2REF & OC5REF
    //GC5C3置1，OC3REFC由TIMx_CCR3和TIMx_CCR5控制，OC3REFC = OC3REF & OC5REF
    HAL_TIMEx_GroupChannel5(&TimHandle0, (TIM_GROUPCH5_OC1REFC | TIM_GROUPCH5_OC2REFC | TIM_GROUPCH5_OC3REFC));

    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_5);

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC2P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_05), PORT_FUNC_TMR0_OC3P);
}
#endif

#if (TEST_LIST == TEST_6STEPS)
// TIM_6Steps
/*
 * 测试项：6步换相测试
 * 基本配置：系统时钟48MHz
 * 通过定时产出COM事件触发进行换相
 * 输出：输出6步换相波形
 */

TIM_OC_InitTypeDef sPWMConfig1, sPWMConfig2, sPWMConfig3;

static void commutation_event_generate_callback(void *priv)
{
    // 生成COM事件
    HAL_TIM_GenerateEvent(&TimHandle0, TIM_EVENTSOURCE_COM);
}

void HAL_TIMEx_CommutCallback(TIM_HandleTypeDef *htim)
{
    static u8 uwStep = 0;

    switch (uwStep) {
    case 0:
        sPWMConfig1.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);
        HAL_TIMEx_OCN_Stop(&TimHandle0, TIM_CHANNEL_1);

        /*  Channel3 configuration */
        sPWMConfig3.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig3, TIM_CHANNEL_3);
        HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_3);
        HAL_TIM_PWM_Stop(&TimHandle0, TIM_CHANNEL_3);

        /*  Channel2 configuration */
        HAL_TIM_OC_Stop(&TimHandle0, TIM_CHANNEL_2);
        HAL_TIMEx_OCN_Stop(&TimHandle0, TIM_CHANNEL_2);
        break;
    case 1:
        /*  Channel1 configuration */
        /* Same configuration as the previous step */

        /*  Channel2 configuration */
        sPWMConfig2.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig2, TIM_CHANNEL_2);
        HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_2);

        /*  Channel3 configuration */
        HAL_TIMEx_OCN_Stop(&TimHandle0, TIM_CHANNEL_3);
        break;
    case 2:
        /*  Channel2 configuration */
        /* Same configuration as the previous step */

        /*  Channel3 configuration */
        sPWMConfig3.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig3, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_3);

        /*  Channel1 configuration */
        HAL_TIM_OC_Stop(&TimHandle0, TIM_CHANNEL_1);
        break;
    case 3:
        /*  Channel3 configuration */
        /* Same configuration as the previous step */

        /*  Channel2 configuration */
        HAL_TIMEx_OCN_Stop(&TimHandle0, TIM_CHANNEL_2);

        /*  Channel1 configuration */
        sPWMConfig1.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig1, TIM_CHANNEL_1);
        HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_1);
        break;
    case 4:
        /*  Channel3 configuration */
        HAL_TIM_OC_Stop(&TimHandle0, TIM_CHANNEL_3);

        /*  Channel1 configuration */
        /* Same configuration as the previous step */

        /*  Channel2 configuration */
        sPWMConfig2.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig2, TIM_CHANNEL_2);
        HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_2);
        break;
    case 5:
        /*  Channel3 configuration */
        sPWMConfig3.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig3, TIM_CHANNEL_3);
        HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_3);

        /*  Channel1 configuration */
        HAL_TIMEx_OCN_Stop(&TimHandle0, TIM_CHANNEL_1);

        /*  Channel2 configuration */
        /* Same configuration as the previous step */
        break;
    case 6:
        /*  Channel1 configuration */
        sPWMConfig1.OCMode = TIM_OCMODE_PWM1;
        HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sPWMConfig1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);

        /*  Channel3 configuration */
        /* Same configuration as the previous step */

        /*  Channel2 configuration */
        HAL_TIM_OC_Stop(&TimHandle0, TIM_CHANNEL_2);
        break;
    default:
        break;
    }

    if (++uwStep == 7) {
        uwStep = 1;
    }
}

void tim_6steps_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance           = JL_ADV_TIMER0;
    TimHandle0.Init.Period        = 4095;
    TimHandle0.Init.Prescaler     = 0;
    TimHandle0.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode   = TIM_COUNTERMODE_UP;
    HAL_TIM_OC_Init(&TimHandle0);

    sPWMConfig1.OCMode       = TIM_OCMODE_TIMING; //冻结
    sPWMConfig1.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sPWMConfig1.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    sPWMConfig1.OCIdleState  = TIM_OCIDLESTATE_SET;
    sPWMConfig1.OCNIdleState = TIM_OCNIDLESTATE_SET;
    sPWMConfig1.OCFastMode   = TIM_OCFAST_DISABLE;
    sPWMConfig1.Pulse          = 2047; // CCR1
    HAL_TIM_OC_ConfigChannel(&TimHandle0, &sPWMConfig1, TIM_CHANNEL_1);

    sPWMConfig2 = sPWMConfig1;
    sPWMConfig2.Pulse = 1023; // CCR2
    HAL_TIM_OC_ConfigChannel(&TimHandle0, &sPWMConfig2, TIM_CHANNEL_2);

    sPWMConfig3 = sPWMConfig1;
    sPWMConfig3.Pulse = 511; // CCR3
    HAL_TIM_OC_ConfigChannel(&TimHandle0, &sPWMConfig3, TIM_CHANNEL_3);

    TIM_BreakDeadTimeConfigTypeDef sBreakConfig = {
        .OffStateRunMode  = TIM_OSSR_ENABLE,         // 运行模式下的关闭状态
        .OffStateIDLEMode = TIM_OSSI_ENABLE,         // 空闲模式下的关闭状态
        .LockLevel        = TIM_LOCKLEVEL_OFF,      // 锁定配置
        .BreakState       = TIM_BREAK_DISABLE,        // 断路输入1使能
        .BreakPolarity    = TIM_BREAKPOLARITY_HIGH,    // 断路输入1极性
        .Break2State      = TIM_BREAK_DISABLE,        // 断路输入2使能
        .Break2Polarity   = TIM_BREAK2POLARITY_HIGH,    // 断路输入2极性
        .AutomaticOutput  = TIM_AUTOMATICOUTPUT_ENABLE, // 自动输出使能
        .DeadTime         = 1,    // 死区时间
    };

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_09), PORT_INPUT_FLOATING);
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_10), PORT_INPUT_FLOATING);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_09), PORT_FUNC_TMR0_BRK_IN1);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_10), PORT_FUNC_TMR0_BRK_IN2);
    HAL_TIMEx_ConfigBreakDeadTime(&TimHandle0, &sBreakConfig);

    HAL_TIMEx_ConfigCommutationEvent_IT(&TimHandle0, TIM_TS_NONE, TIM_COMMUTATION_SOFTWARE);

    HAL_TIM_OC_Start(&TimHandle0, TIM_CHANNEL_1);
    HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_1);

    HAL_TIM_OC_Start(&TimHandle0, TIM_CHANNEL_2);
    HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_2);

    HAL_TIM_OC_Start(&TimHandle0, TIM_CHANNEL_3);
    HAL_TIMEx_OCN_Start(&TimHandle0, TIM_CHANNEL_3);

    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC1N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_05), PORT_FUNC_TMR0_OC2P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_06), PORT_FUNC_TMR0_OC2N);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_07), PORT_FUNC_TMR0_OC3P);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_08), PORT_FUNC_TMR0_OC3N);

    /* usr_timer_add(NULL, commutation_event_generate_callback, 2, 0); */
    sys_timer_add(NULL, commutation_event_generate_callback, 2);
}
#endif

#if (TEST_LIST == TEST_ENCODE)
//Encode_test
/*
 * 测试项：旋转编码器测试, 此例程使用编码器是1000线的
 * 基本配置：系统时钟48MHz 配置编码器模式1/2/3进行测试 A-TI1 B-TI2 Z-TI3
 * 输出：电机转动一圈捕获到的脉冲数
 */
#define ENCODE_MODE    TIM_ENCODERMODE_TI1

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    u16 capture_cnt = __HAL_TIM_GET_COUNTER(&TimHandle0);
    u32 motor_dir = __HAL_TIM_IS_TIM_COUNTING_DOWN(&TimHandle0);

    log_info("motor dir: %d, count: %d", motor_dir, capture_cnt);
}

void encode_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 65535;
    TimHandle0.Init.Prescaler         = 0;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;

    TIM_Encoder_InitTypeDef eStructure = {
#if (ENCODE_MODE == TIM_ENCODERMODE_TI12)
        .EncoderMode = TIM_ENCODERMODE_TI12, //四分频
#elif (ENCODE_MODE == TIM_ENCODERMODE_TI1)
        .EncoderMode = TIM_ENCODERMODE_TI1, //二分频
#elif (ENCODE_MODE == TIM_ENCODERMODE_TI2)
        .EncoderMode = TIM_ENCODERMODE_TI2, //二分频
#endif
        //Polarity代表编码器正方向，与计数触发边沿无关，只用来匹配电机和编码器方向
        .IC1Polarity = TIM_ICPOLARITY_FALLING,
        .IC1Selection = TIM_ICSELECTION_DIRECTTI,//编码器模式固定配置，ch1只能IC1映射到TI1
        .IC1Prescaler = TIM_ICPSC_DIV1,
        .IC1Filter = 0,
        .IC2Polarity = TIM_ICPOLARITY_FALLING,
        .IC2Selection = TIM_ICSELECTION_DIRECTTI,//编码器模式固定配置，ch2只能IC2映射到TI2
        .IC2Prescaler = TIM_ICPSC_DIV1,
        .IC2Filter = 0,
    };

    HAL_TIM_Encoder_Init(&TimHandle0, &eStructure);

#if (ENCODE_MODE == TIM_ENCODERMODE_TI12)
    HAL_TIM_Encoder_Start(&TimHandle0, TIM_CHANNEL_ALL);
#elif (ENCODE_MODE == TIM_ENCODERMODE_TI1)
    HAL_TIM_Encoder_Start(&TimHandle0, TIM_CHANNEL_1);
#elif (ENCODE_MODE == TIM_ENCODERMODE_TI2)
    HAL_TIM_Encoder_Start(&TimHandle0, TIM_CHANNEL_2);
#endif

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_03), PORT_INPUT_FLOATING);
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_INPUT_FLOATING);
    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_05), PORT_INPUT_FLOATING);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC1);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_IC2);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_05), PORT_FUNC_TMR0_IC3);

    HAL_TIM_IC_Init(&TimHandle0);
    TIM_IC_InitTypeDef sICConfig = {
        .ICPolarity  = TIM_ICPOLARITY_FALLING,
        .ICSelection = TIM_ICSELECTION_DIRECTTI,
        .ICPrescaler = TIM_ICPSC_DIV1,
        .ICFilter    = 0,
    };
    HAL_TIM_IC_ConfigChannel(&TimHandle0, &sICConfig, TIM_CHANNEL_3);
    HAL_TIM_IC_Start_IT(&TimHandle0, TIM_CHANNEL_3);
}
#endif

#if (TEST_LIST == TEST_ETR_CLR)
//Etr_test
/*
 * 测试项：外部信号清除OCxREF信号测试
 * 基本配置：外部信号PB1作ETR输入，可配触发极性
 * 输出：在触发极性下，OCxREF输出信号被清除
 */
#define    ETR_CHx          TIM_CHANNEL_1
#define ETR_POLARITY    TIM_ETRPOLARITY_NONINVERTED
void etr_clr_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 1000;
    TimHandle0.Init.Prescaler         = 0;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;
    HAL_TIM_PWM_Init(&TimHandle0);

    TIM_OC_InitTypeDef sConfig = {
        .Pulse            = 500,
        .OCMode         = TIM_OCMODE_PWM1,      // 输出比较模式
        .OCPolarity     = TIM_OCPOLARITY_HIGH,     // 输出有效电平极性
        .OCNIdleState     = TIM_OCNIDLESTATE_RESET,
        .OCIdleState      = TIM_OCIDLESTATE_RESET,
    };

    TIM_ClearInputConfigTypeDef cConfig = {
        .ClearInputState = STATE_ENABLE,
        .ClearInputSource = TIM_CLEARINPUTSOURCE_ETR,
        .ClearInputPolarity = ETR_POLARITY,
        .ClearInputPrescaler = TIM_CLEARINPUTPRESCALER_DIV1, //关闭预分频器
        .ClearInputFilter = 0,
    };

#if (ETR_CHx == TIM_CHANNEL_1)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC1P);
#elif (ETR_CHx == TIM_CHANNEL_2)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC2P);
#elif (ETR_CHx == TIM_CHANNEL_3)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC3P);
#elif (ETR_CHx == TIM_CHANNEL_4)
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_OC4P);
#endif

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_02), PORT_INPUT_FLOATING);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_ETR_IN);
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sConfig, ETR_CHx);
    HAL_TIM_ConfigOCrefClear(&TimHandle0, &cConfig, ETR_CHx);
    HAL_TIM_PWM_Start(&TimHandle0, ETR_CHx);
}
#endif

#if (TEST_LIST == TEST_PWMINPUT)
//Pwm_input
/*
 * 测试项：PWM输入测试，计算输入pwm的周期和占空比
 * 基本配置：TIM0 IC1/IC2映射到TI1输入，边沿有效极性相反。TI1FP为触发输入用于从模式复位模式
 * 复位TIM0计数器，TI1FP触发边沿和IC1一致。
 * 输出：IC1 IC2捕获pwm周期和占空比
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    log_info("ccr1 %d, ccr2 %d", JL_ADV_TIMER0->CCR1, JL_ADV_TIMER0->CCR2);
}

void pwminput_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_03), PORT_INPUT_FLOATING);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_03), PORT_FUNC_TMR0_IC1);

    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 2000;
    TimHandle0.Init.Prescaler         = 48000000L / 1000 - 1;
    TimHandle0.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;
    HAL_TIM_IC_Init(&TimHandle0);

    TIM_IC_InitTypeDef sICConfig1 = {
        .ICPolarity  = TIM_ICPOLARITY_FALLING,//TIM_ICPOLARITY_RISING,
        .ICSelection = TIM_ICSELECTION_DIRECTTI,
        .ICPrescaler = TIM_ICPSC_DIV1,
        .ICFilter    = 0,
    };
    HAL_TIM_IC_ConfigChannel(&TimHandle0, &sICConfig1, TIM_CHANNEL_1);

    TIM_IC_InitTypeDef sICConfig2 = {
        .ICPolarity  = TIM_ICPOLARITY_RISING,//TIM_ICPOLARITY_FALLING,
        .ICSelection = TIM_ICSELECTION_INDIRECTTI,
        .ICPrescaler = TIM_ICPSC_DIV1,
        .ICFilter    = 0,
    };
    HAL_TIM_IC_ConfigChannel(&TimHandle0, &sICConfig2, TIM_CHANNEL_2);

    TIM_SlaveConfigTypeDef sSlaveConfig = {
        .SlaveMode        = TIM_SLAVEMODE_RESET,
        .InputTrigger     = TIM_TS_TI2FP2,//TIM_TS_TI1FP1,
        .TriggerPolarity  = TIM_TRIGGERPOLARITY_NONINVERTED,
        .TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1,
        .TriggerFilter    = 0,
    };
    HAL_TIM_SlaveConfigSynchronization(&TimHandle0, &sSlaveConfig);

    HAL_TIM_IC_Start_IT(&TimHandle0, TIM_CHANNEL_1);
    HAL_TIM_IC_Start_IT(&TimHandle0, TIM_CHANNEL_2);
}
#endif

#if (TEST_LIST == TEST_EXT_CLOCK1)
//Ext_clock1_test
/*
 * 测试项：外部时钟模式1测试
 * 基本配置：可配置外部时钟触发源 PA3和PA1短接，PA3灌入时钟
 * 输出：对输入的外部时钟进行上升沿计数
 */
void ext_clock1_test(void)
{
    log_info("%s", __FUNCTION__);

    adv_timer_rst();

    gpio_set_mode(IO_PORT_SPILT(IO_PORTA_03), PORT_INPUT_FLOATING);
    gpio_set_function(IO_PORT_SPILT(IO_PORTA_02), PORT_FUNC_TMR0_OC1P);
    TimHandle0.Instance               = JL_ADV_TIMER0;
    TimHandle0.Init.Period            = 500;
    TimHandle0.Init.Prescaler         = 48000000L / 1000 - 1;
    TimHandle0.Init.ClockDivision     = 0;
    TimHandle0.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimHandle0.Init.RepetitionCounter = 0;
    HAL_TIM_PWM_Init(&TimHandle0);
    TIM_OC_InitTypeDef sOCConfig = {
        .OCMode       = TIM_OCMODE_PWM1,
        .OCPolarity   = TIM_OCPOLARITY_HIGH,
        .Pulse        = 250,
        .OCNPolarity  = TIM_OCNPOLARITY_HIGH,
        .OCFastMode   = TIM_OCFAST_DISABLE,
        .OCIdleState  = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
    };
    HAL_TIM_PWM_ConfigChannel(&TimHandle0, &sOCConfig, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&TimHandle0, TIM_CHANNEL_1);

    while (1) {
        wdt_clear();
        mdelay(100);
        log_info("%d ", JL_ADV_TIMER0->CNT);
    }
}
#endif

void adv_timer_test(void)
{
    request_irq(IRQ_ADV_TMR0_IDX, 0, adv_timer0_irq_callback, 0);

#if (TEST_LIST == TEST_TIMEBASE)
    timebase_test();
#elif (TEST_LIST == TEST_PWMOUTPUT)
    pwmoutput_test();
    /* pwmoutput_decimals_test(); */
#elif (TEST_LIST == TEST_ONEPLUSE)
    onepulse_test();
#elif (TEST_LIST == TEST_OCTOGGLE)
    octoggle_test();
#elif (TEST_LIST == TEST_INPUTCAPTURE)
    inputcapture_test();
#elif (TEST_LIST == TEST_DMA_DCR_UNLOCK)
    dma_dcr_unlock_test();
#elif (TEST_LIST == TEST_DMA_BURST)
    dmaburst_test();
#elif (TEST_LIST == TEST_COMPLEMENTARYSIGNALS)
    complementarysignals_test();
#elif (TEST_LIST == TEST_COMBINE)
    combined_test();
#elif (TEST_LIST == TEST_6STEPS)
    tim_6steps_test();
#elif (TEST_LIST == TEST_ENCODE)
    encode_test();
#elif (TEST_LIST == TEST_ETR_CLR)
    etr_clr_test();
#elif (TEST_LIST == TEST_PWMINPUT)
    pwminput_test();
#elif (TEST_LIST == TEST_EXT_CLOCK1)
    ext_clock1_test();
#endif
}

