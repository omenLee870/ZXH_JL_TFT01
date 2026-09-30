#ifndef _GPIO_H
#define _GPIO_H

#define IO_PORT_SPILT(io) ((u32)(io)) / 16, BIT(((u32)(io)) %16)


// PORT引脚输入输出模式
enum gpio_mode {
    PORT_OUTPUT_LOW = 0,
    PORT_OUTPUT_HIGH = 1,
    PORT_HIGHZ = 2,               //高阻模式

    PORT_INPUT_FLOATING = 0x10,   //浮空输入

    //不同芯片的上下拉不一样，具体的阻值请查阅对应芯片的gpio_hw.h
    //gpio 函数接口的参数请使用PORT_INPUT_PULLUP_10K,..._100K,..._200K,..._1M宏定义
    //当芯片不支持对应的阻值的时候，具体的映射关系请查阅对应芯片的gpio_hw.h
    //比如：br56 没有100k，头文件有如此定义 #define PORT_INPUT_PULLUP_100K PORT_INPUT_PULLUP_200K
    // 就是说当参数为 PORT_INPUT_PULLUP_100K的时候，实际上上拉阻值为PORT_INPUT_PULLUP_200K
    PORT_INPUT_PULLUP_LEVEL1 = 0x11,
    PORT_INPUT_PULLUP_LEVEL2,
    PORT_INPUT_PULLUP_LEVEL3,

    PORT_INPUT_PULLDOWN_LEVEL1 = 0x21,//参考上拉注释
    PORT_INPUT_PULLDOWN_LEVEL2,
    PORT_INPUT_PULLDOWN_LEVEL3,

    PORT_KEEP_STATE = 0x30,
};

//驱动电流有4档,不同IO引脚驱动能力有差异,请查阅芯片规格书
enum gpio_drive_strength {
    PORT_DRIVE_STRENGT_2p4mA,		///< 最大驱动电流  2.4mA
    PORT_DRIVE_STRENGT_8p0mA,		///< 最大驱动电流  8.0mA
    PORT_DRIVE_STRENGT_24p0mA,		///< 最大驱动电流 24.0mA
    PORT_DRIVE_STRENGT_64p0mA,		///< 最大驱动电流 64.0mA
};

//上拉电阻有4档,具体阻值请查阅芯片规格书查看芯片规格书
enum gpio_pullup_mode {
    GPIO_PULLUP_DISABLE,
    GPIO_PULLUP_10K,
    GPIO_PULLUP_100K,
    GPIO_PULLUP_1M,
};

//下拉电阻有4档,具体阻值请查阅芯片规格书
enum gpio_pulldown_mode {
    GPIO_PULLDOWN_DISABLE,
    GPIO_PULLDOWN_10K,
    GPIO_PULLDOWN_100K,
    GPIO_PULLDOWN_1M,
};


#define PORT_PIN_0                 ((uint16_t)0x0001)  /* Pin 0 selected    */
#define PORT_PIN_1                 ((uint16_t)0x0002)  /* Pin 1 selected    */
#define PORT_PIN_2                 ((uint16_t)0x0004)  /* Pin 2 selected    */
#define PORT_PIN_3                 ((uint16_t)0x0008)  /* Pin 3 selected    */
#define PORT_PIN_4                 ((uint16_t)0x0010)  /* Pin 4 selected    */
#define PORT_PIN_5                 ((uint16_t)0x0020)  /* Pin 5 selected    */
#define PORT_PIN_6                 ((uint16_t)0x0040)  /* Pin 6 selected    */
#define PORT_PIN_7                 ((uint16_t)0x0080)  /* Pin 7 selected    */
#define PORT_PIN_8                 ((uint16_t)0x0100)  /* Pin 8 selected    */
#define PORT_PIN_9                 ((uint16_t)0x0200)  /* Pin 9 selected    */
#define PORT_PIN_10                ((uint16_t)0x0400)  /* Pin 10 selected   */
#define PORT_PIN_11                ((uint16_t)0x0800)  /* Pin 11 selected   */
#define PORT_PIN_12                ((uint16_t)0x1000)  /* Pin 12 selected   */
#define PORT_PIN_13                ((uint16_t)0x2000)  /* Pin 13 selected   */
#define PORT_PIN_14                ((uint16_t)0x4000)  /* Pin 14 selected   */
#define PORT_PIN_15                ((uint16_t)0x8000)  /* Pin 15 selected   */
#define PORT_PIN_All               ((uint16_t)0xFFFF)  /* All pins selected */

#define PORT_PIN_MASK              (0x0000FFFFu) /* PIN mask for assert test */

#include "cpu.h"
#define GPIOA_INDEX     0
#define GPIOB_INDEX     1
#define GPIOC_INDEX     2
#define GPIOD_INDEX     3
#define GPIOE_INDEX     4
#define GPIOF_INDEX     5
#define GPIOG_INDEX     6
#define GPIOH_INDEX     7
#define GPIOI_INDEX     8
#define GPIOJ_INDEX     9
#define GPIOK_INDEX     10
#define GPIOL_INDEX     11
#define GPIOM_INDEX     12
#define GPIOP_INDEX     13
#define GPIOUSB_INDEX   14
#define GPIOR_INDEX     15

#define IO_GROUP_NUM 		16

#include "asm/gpio_hw.h"

struct gpio_config {
    u32 pin;
    enum gpio_mode mode;
    enum gpio_drive_strength hd;
};
int port_get_reg_num(const int port_index);
void *port2reg(const u32 port, u32 pin);
int get_gpio(const char *p);//return <0:error
const char *gpio_get_name(u32 gpio);

u32 gpio_hw_port_pin_judge(const enum gpio_port port, u32 pin);//pin:ok, 0:error
//=================================================================================//
//@brief: CrossBar 获取某IO的输出映射寄存器
//=================================================================================//
u32 *gpio2crossbar_outreg(u32 gpio);
u32 gpio2crossbar_inport(u32 gpio);
int crossbar_imap2gpio(u32 imap);
int gpio_set_fun_output_port(u32 gpio, u32 fun_index, u32 dir_ctl, u32 data_ctl);
int gpio_disable_fun_output_port(u32 gpio);
int gpio_set_fun_input_port(u32 gpio, enum PFI_TABLE pfun);
int gpio_disable_fun_input_port(enum PFI_TABLE pfun);
// 将src_io的crossbar输出信号复制到另一个io输出(两个io需要支持crossbar)
//两个io输出同一个信号,如果是och信号,可节省och通道数
int gpio_copy_fun_output_port(u32 gpio_src, u32 gpio_dst);
//ich输入通道信号拷贝,同一个io输入到不同的内部信号,可节省ich输入通道
int gpio_ich_copy_input_signal(enum INPUT_CH_SIGNAL signal_src, enum INPUT_CH_SIGNAL signal_dst);
//=================================================================================//
//@brief: Output/input Channel输出设置 API, 将指定IO口设置为某个外设的输出
//=================================================================================//
int gpio_och_sel_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);
int gpio_och_disable_output_signal(u32 gpio, enum OUTPUT_CH_SIGNAL signal);
int gpio_ich_sel_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);
int gpio_ich_disable_input_signal(u32 gpio, enum INPUT_CH_SIGNAL signal, enum INPUT_CH_TYPE type);
u32 gpio_get_ich_use_flag();
//获取空闲的gp_ich
//return: 0xff:error
u32 gpio_get_unoccupied_gp_ich();
//value:gp_ich序号
void gpio_release_gp_ich(u32 value);

//=================================================================================//
//@brief: 打印指定组别指定pin的crossbar信息
//=================================================================================//
void gpio_crossbar_fo_dump(char px_name[], u32 max_px_out_num, u32 px_mask, u32 *omap_ptr);
void gpio_crossbar_fi_dump(char px_name[], u32 max_px_in_num, u32 px_mask, u32 px_in);
//打印芯片全部gpio寄存器,crossbar信息
void gpio_dump();
//打印芯片指定io寄存器,crossbar信息
void gpio_appoint_dump(enum gpio_port port, u32 pin);

//=================================================================================//
//配置同组多个io模式及强驱. 形参详见枚举; pin:PORT_PIN_0 or PORT_PIN_0 | PORT_PIN_2等
int gpio_init(enum gpio_port port, const struct gpio_config *config);
//恢复同组多个io为高阻态. 形参详见枚举; pin:PORT_PIN_0 or PORT_PIN_0 | PORT_PIN_2等
int gpio_deinit(enum gpio_port port, u32 pin);
//配置同组多个io模式. 形参详见枚举; pin:PORT_PIN_0 or PORT_PIN_0 | PORT_PIN_2等
//return:<0:error
int gpio_set_mode(enum gpio_port port, u32 pin, enum gpio_mode mode);
int gpio_keep_mode_at_sleep(enum gpio_port port, u32 pin);
enum gpio_mode gpio_get_mode(enum gpio_port port, u32 pin);

// 读取单个io输入值. gpio:IO_PORTA_00
int gpio_read(u32 gpio);
//读取同组多个io值. 形参详见枚举; pin:PORT_PIN_0 or PORT_PIN_0 | PORT_PIN_2等
int gpio_read_port(enum gpio_port port, u32 pin);

// 设置单个io输出电平(需先配置为输出). gpio:IO_PORTA_00; value:0:out 0, 1:out 1
int gpio_write(u32 gpio, u32 value);
// 设置同组多个io输出电平(需先配置为输出).
// pin:PORT_PIN_0 or PORT_PIN_0 | PORT_PIN_2等
// out_state:0:out 0, 1:out 1
int gpio_write_port(enum gpio_port port, u32 pin, int out_state);

//翻转同组多个io输出电平(需先配置为输出). pin:PORT_PIN_0 or PORT_PIN_0 | PORT_PIN_2等
//return:<0:error
int gpio_toggle_port(enum gpio_port port, u32 pin);

// 获取同组多个io输出电平
int gpio_get_out_level(enum gpio_port port, u32 pin);

// 设置同组多个io强驱
int gpio_set_drive_strength(enum gpio_port port, u32 pin, enum gpio_drive_strength drive);

// 获取单个io输出强度 pin:只能带入1个io
enum gpio_drive_strength  gpio_get_drive_strength(enum gpio_port port, u32 pin);



// ----------------------------------------
// PORT中断
enum gpio_irq_edge {
    PORT_IRQ_DISABLE = 0,      ///< Disable PORT interrupt
    PORT_IRQ_EDGE_RISE  = 1,   ///< PORT interrupt type : rising edge
    PORT_IRQ_EDGE_FALL   = 2,  ///< PORT interrupt type : falling edge
    PORT_IRQ_ANYEDGE = 3,      ///< PORT interrupt type : both rising and falling edge
};
typedef void (*gpio_irq_callback_p)(enum gpio_port port, u32 pin, enum gpio_irq_edge edge);
struct gpio_irq_config_st {
    u32 pin;
    enum gpio_irq_edge irq_edge;
    gpio_irq_callback_p callback;
    u8 irq_priority;//中断优先级
};
//配置中断(已使能)
//禁止同一个io同一边沿多次注册
//单边沿与双边沿切换: 请先注销(PORT_IRQ_DISABLE)再注册
int gpio_irq_config(enum gpio_port port, const struct gpio_irq_config_st *config);//pa,pb,pc,pp0,usb
//注销io中断接口
int gpio_irq_deinit(enum gpio_port port, u32 pin);
//修改中断回调函数
int gpio_irq_set_callback(enum gpio_port port, u32 pin, gpio_irq_callback_p callback);
//快速切换使能同组多个io中断响应
int gpio_irq_enable(enum gpio_port port, u32 pin);
//快速切换暂停同组多个io中断响应
int gpio_irq_disable(enum gpio_port port, u32 pin);

//只有注册单边沿触发才能调用该函数切换边沿
//单边沿与双边沿切换: 请先注销再注册
int gpio_irq_set_edge(enum gpio_port port, u32 pin, enum gpio_irq_edge irq_edge);
//  获取单个io触发边沿 pin:只能带入1个io
enum gpio_irq_edge gpio_irq_get_edge(enum gpio_port port, u32 pin);




// ----------------------------------------
// PORT 功能配置
// 配置单个io为特殊功能. pin:只能带入1个io
//return:<0:error
int gpio_set_function(enum gpio_port port, u32 pin, enum gpio_function fn);
// 注销单个io的特殊功能. pin:只能带入1个io
//return:<0:error
int gpio_disable_function(enum gpio_port port, u32 pin, enum gpio_function fn);
/* 示例：
   gpio_set_function(PORTA, PORT_PIN_0, PORT_FUNC_UART0_TX);
   gpio_set_function(PORTA, PORT_PIN_1, PORT_FUNC_UART0_RX);
   gpio_set_function(PORTA, PORT_PIN_2, PORT_FUNC_UART0_CTS);
   gpio_set_function(PORTA, PORT_PIN_3, PORT_FUNC_UART0_RTS);
*/

// io复用时，io资源申请
int gpio_request_function(enum gpio_port port, u32 pin, enum gpio_function fn, u32 timeout);
// io复用时，io资源释放
int gpio_release_function(enum gpio_port port, u32 pin, enum gpio_function fn);

#endif

