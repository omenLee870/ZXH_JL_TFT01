#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".adkey.data.bss")
#pragma data_seg(".adkey.data")
#pragma const_seg(".adkey.text.const")
#pragma code_seg(".adkey.text")
#endif
#include "key_driver.h"
#include "adkey.h"
#include "gpio.h"
#include "app_config.h"
#include "asm/power_interface.h"
#include "system/timer.h"


#if TCFG_ADKEY_ENABLE

#define  TCFG_ADKEY_LED_IO_REUSE  0
#define  TCFG_ADKEY_IR_IO_REUSE  0
#define  TCFG_ADKEY_LED_SPI_IO_REUSE  0

static const struct adkey_platform_data *__this = NULL;

#if KEY_DIAG_ENABLE
/* Scan runs in the hardware timer. Only copy samples there; print from the
 * system timer in the initializing task (app_core), at most once per 200ms. */
static volatile u16 s_key_diag_adc;
static volatile u8 s_key_diag_value = NO_KEY;
static volatile u32 s_key_diag_samples;
static u16 s_key_diag_timer;

#if KEY_DIAG_DIRECT_ADC_ENABLE
static int adkey_diag_map_adc(u32 adc)
{
    for (u8 i = 0; i < CONFIG_ADKEY_MAX_NUM; i++) {
        if (adc <= __this->ad_value[i] && __this->ad_value[i] < 0xfffU) {
            return __this->key_value[i] == NO_KEY ? -1 : (int)__this->key_value[i];
        }
    }
    return -1;
}

static void adkey_diag_probe(void)
{
    static u32 last_print_ms;
    static u32 last_queued;
    static u32 last_direct;
    static int last_queued_key;
    static int last_direct_key;
    static u8 printed;
    u32 queued = adc_get_value(__this->ad_channel);
    u32 begin = sys_timer_get_ms();
    /* This SDK API temporarily takes over GPADC then releases it back to the
     * scan queue. It does not update that queue's cached sample itself. */
    u32 direct = adc_get_value_blocking(__this->ad_channel);
    u32 now = sys_timer_get_ms();
    u32 queued_after = adc_get_value(__this->ad_channel);
    int queued_key = adkey_diag_map_adc(queued);
    int direct_key = adkey_diag_map_adc(direct);
    int queued_change = (int)queued - (int)last_queued;
    int direct_change = (int)direct - (int)last_direct;
    if (printed && queued_key == last_queued_key && direct_key == last_direct_key &&
        queued_change > -32 && queued_change < 32 &&
        direct_change > -32 && direct_change < 32 &&
        (u32)(now - last_print_ms) < 2000U && (u32)(now - begin) < 20U) {
        return;
    }
    printf("[KEY_DIAG] ADC_PROBE t=%u channel=0x%x queued=%u queued_key=%d direct=%u direct_key=%d queued_after=%u probe_ms=%u scans=%u\n",
           now, __this->ad_channel, queued, queued_key, direct, direct_key,
           queued_after, now - begin, s_key_diag_samples);
    last_print_ms = now;
    last_queued = queued;
    last_direct = direct;
    last_queued_key = queued_key;
    last_direct_key = direct_key;
    printed = 1;
}
#endif

static void adkey_diag_poll(void *priv)
{
    static u32 last_print_ms;
    static u32 last_samples;
    static u16 last_adc;
    static u8 last_value = NO_KEY;
    static u8 printed;
    u32 samples = s_key_diag_samples;
    u32 now = sys_timer_get_ms();
    u16 adc = s_key_diag_adc;
    u8 value = s_key_diag_value;
    int change = (int)adc - (int)last_adc;
    (void)priv;
#if KEY_DIAG_DIRECT_ADC_ENABLE
    adkey_diag_probe();
#endif
    if (samples == 0) {
        if ((u32)(now - last_print_ms) >= 2000U) {
            printf("[KEY_DIAG] ADC no_samples t=%u\n", now);
            last_print_ms = now;
        }
        return;
    }
    if (printed && value == last_value && change > -32 && change < 32 &&
        (u32)(now - last_print_ms) < 2000U) {
        return;
    }
    printf("[KEY_DIAG] ADC t=%u raw=%u mapped=%d scans=%u delta_scans=%u status=%s\n",
           now, adc, value == NO_KEY ? -1 : (int)value, samples, samples - last_samples,
           printed && samples == last_samples ? "SCAN_STALLED" : "SCANNING");
    last_adc = adc;
    last_value = value;
    last_samples = samples;
    last_print_ms = now;
    printed = 1;
}

static void adkey_diag_init(void)
{
    unsigned int count = 0;
    unsigned int low = 0;
    for (unsigned int i = 0; i < CONFIG_ADKEY_MAX_NUM; i++) {
        u8 value = __this->key_value[i];
        u16 high = __this->ad_value[i];
        if (value >= KEY_AD_NUM0 && value <= KEY_AD_NUM19 && high < 0xfffU) {
            printf("[KEY_DIAG] AD table slot=%u key=KEY_AD_NUM%u value=%u adc_min=%u adc_max=%u\n",
                   i, value - KEY_AD_NUM0, value, low, high);
            low = (unsigned int)high + 1U;
            count++;
        }
    }
    printf("[KEY_DIAG] AD configured_keys=%u pin=%u channel=0x%x external_pullup=%u\n",
           count, __this->adkey_pin, __this->ad_channel, __this->extern_up_en);
    if (!s_key_diag_timer) {
        s_key_diag_timer = sys_timer_add(NULL, adkey_diag_poll, 200);
    }
    printf("[KEY_DIAG] ADC monitor timer=%u poll_ms=200 heartbeat_ms=2000\n", s_key_diag_timer);
    printf("[KEY_DIAG] direct_adc_probe=%u task=app_core\n", KEY_DIAG_DIRECT_ADC_ENABLE);
}
#endif

//按键驱动扫描参数列表
struct key_driver_para adkey_scan_param = {
    .last_key 		  = NO_KEY,  		//上一次get_value按键值, 初始化为NO_KEY;
    .key_type		  = KEY_DRIVER_TYPE_AD,
    .filter_time  	  = 2,				//按键消抖延时;
    .long_time 		  = 75,  			//按键判定长按数量
    .hold_time 		  = (75 + 15),  	//按键判定HOLD数量
    .click_delay_time = 20,				//按键被抬起后等待连击延时数量
    .scan_time 	  	  = 10,				//按键扫描频率, 单位: ms
};

u8 ad_get_key_value(void)
{
    u8 i;
    u8 key_value = NO_KEY;
    u16 ad_data;

    if (!__this->enable) {
        return NO_KEY;
    }

    /* ad_data = adc_get_voltage(__this->ad_channel); */
    ad_data = adc_get_value(__this->ad_channel);
    for (i = 0; i < CONFIG_ADKEY_MAX_NUM; i++) {
        if ((ad_data <= __this->ad_value[i]) && (__this->ad_value[i] < 0xfffL)) {
            key_value = __this->key_value[i];
            break;
        }
    }
#if KEY_DIAG_ENABLE
    s_key_diag_adc = ad_data;
    s_key_diag_value = key_value;
    s_key_diag_samples++;
#endif
    return key_value;
}

__attribute__((weak))
const struct adkey_platform_data *get_adkey_platform_data()
{
    return NULL;
}

int adkey_init(void)
{
    __this = get_adkey_platform_data();

    if (!__this) {
#if KEY_DIAG_ENABLE
        printf("[KEY_DIAG] AD init failed: missing configuration\n");
#endif
        return -EINVAL;
    }

    if (!__this->enable) {
#if KEY_DIAG_ENABLE
        printf("[KEY_DIAG] AD init failed: disabled\n");
#endif
        return KEY_NOT_SUPPORT;
    }
    adc_add_sample_ch(__this->ad_channel);          //注意：初始化AD_KEY之前，先初始化ADC
#if (TCFG_ADKEY_LED_IO_REUSE || TCFG_ADKEY_IR_IO_REUSE || TCFG_ADKEY_LED_SPI_IO_REUSE)
#else
    if (__this->extern_up_en) {
        gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_INPUT_FLOATING);
    } else {
        gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_INPUT_PULLUP_100K);
    }
    gpio_set_function(IO_PORT_SPILT(__this->adkey_pin), PORT_FUNC_GPADC);
#endif

    if (__this->long_press_enable) {
        gpio_longpress_pin0_reset_config(__this->adkey_pin, 0, __this->long_press_time,  1,  PORT_INPUT_PULLUP_100K, 0);
    }
#if KEY_DIAG_ENABLE
    adkey_diag_init();
#endif
    return 0;
}

#if (TCFG_ADKEY_LED_IO_REUSE || TCFG_ADKEY_IR_IO_REUSE || TCFG_ADKEY_LED_SPI_IO_REUSE)

#if TCFG_ADKEY_IR_IO_REUSE
static u8 ir_io_sus = 0;
extern u8 ir_io_suspend(void);
extern u8 ir_io_resume(void);
#endif
#if TCFG_ADKEY_LED_IO_REUSE
static u8 led_io_sus = 0;
extern u8 led_io_suspend(void);
extern u8 led_io_resume(void);
#endif
#if TCFG_ADKEY_LED_SPI_IO_REUSE
static u8 led_spi_sus = 0;
extern u8 led_spi_suspend(void);
extern u8 led_spi_resume(void);
#endif
u8 adc_io_reuse_enter(u32 ch)
{
    if (ch == __this->ad_channel) {
#if TCFG_ADKEY_IR_IO_REUSE
        if (ir_io_suspend()) {
            return 1;
        } else {
            ir_io_sus = 1;
        }
#endif
#if TCFG_ADKEY_LED_IO_REUSE
        if (led_io_suspend()) {
            return 1;
        } else {
            led_io_sus = 1;
        }
#endif
#if TCFG_ADKEY_LED_SPI_IO_REUSE
        if (led_spi_suspend()) {
            return 1;
        } else {
            led_spi_sus = 1;
        }
#endif
        if (__this->extern_up_en) {
            gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_HIGHZ);
        } else {
            gpio_set_mode(IO_PORT_SPILT(__this->adkey_pin), PORT_INPUT_PULLUP_10K);
            gpio_set_function(IO_PORT_SPILT(__this->adkey_pin), PORT_FUNC_GPADC);
        }
    }
    return 0;
}

u8 adc_io_reuse_exit(u32 ch)
{
    if (ch == __this->ad_channel) {
#if TCFG_ADKEY_IR_IO_REUSE
        if (ir_io_sus) {
            ir_io_sus = 0;
            ir_io_resume();
        }
#endif
#if TCFG_ADKEY_LED_IO_REUSE
        if (led_io_sus) {
            led_io_sus = 0;
            led_io_resume();
        }
#endif
#if TCFG_ADKEY_LED_SPI_IO_REUSE
        if (led_spi_sus) {
            led_spi_sus = 0;
            led_spi_resume();
        }
#endif
    }
    return 0;
}

#endif

REGISTER_KEY_OPS(adkey) = {
    .idle_query_en    = 1,
    .param            = &adkey_scan_param,
    .get_value        = ad_get_key_value,
    .key_init         = adkey_init,
};
#endif  /* #if TCFG_ADKEY_ENABLE */





