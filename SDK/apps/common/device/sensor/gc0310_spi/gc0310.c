#include "asm/iic_hw.h"
#include "isp_dev.h"
#include "iic_api.h"
#include "gc0310.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/device.h"
#include "app_config.h"
#include "asm/power/p33/p33_api.h"
#include "device/sdmmc/sdmmc.h"

#if TCFG_CAMERA_SPI_ENABLE

#define GC0310_DEVP_INPUT_W 640
#define GC0310_DEVP_INPUT_H 480

#define GC0310_DEVP_MAX_W	648
#define GC0310_DEVP_MAX_H	488

#define CONFIG_INPUT_FPS 	12

static hw_iic_dev iic_dev = HW_IIC_2;
static enum iic_state_enum iic_init_state = -1;
static u8 GC0310_reset_io = -1;
static u8 GC0310_power_io = -1;

#define GC0310_WRCMD 0x42
#define GC0310_RDCMD 0x43

#define GC0310_1WIRE_MODE	0

#define DELAY_TIME	10

#define gpio_set_die(gpio, value)  			gpio_hw_set_die(IO_PORT_SPILT(gpio), value)
#define gpio_direction_output(gpio, value) 	gpio_hw_direction_output(IO_PORT_SPILT(gpio), value)
#define gpio_direction_input(gpio) 			gpio_hw_direction_input(IO_PORT_SPILT(gpio))
#define gpio_set_pull_up(gpio, value) 		gpio_hw_set_pull_up(IO_PORT_SPILT(gpio), value)
#define gpio_set_pull_down(gpio, value) 	gpio_hw_set_pull_down(IO_PORT_SPILT(gpio), value)
#define gpio_set_direction(gpio, value) 	gpio_hw_set_direction(IO_PORT_SPILT(gpio), value)

static s32 GC0310_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *freq);

static unsigned char wrGC0310Reg(unsigned char regID, unsigned char regDat)
{
    int ret;
    ret = i2c_master_write_nbytes_to_device_reg(iic_dev, GC0310_WRCMD, &regID, 1, &regDat, 1);
    if (ret < 0) {
        return 0;
    } else {
        return 1;
    }
}

static unsigned char rdGC0310Reg(unsigned char regID, unsigned char *regDat)
{
    int ret;
    ret = i2c_master_read_nbytes_from_device_reg(iic_dev, GC0310_WRCMD, &regID, 1, regDat, 1);
    if (ret < 0) {
        return 0;
    } else {
        return 1;
    }

}

static void GC0310_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    /////////////////////////////////////////////////
    /////////////////   system reg  /////////////////
    /////////////////////////////////////////////////
    wrGC0310Reg(0xfe, 0xf0);
    wrGC0310Reg(0xfe, 0xf0);
    wrGC0310Reg(0xfe, 0x00);

    wrGC0310Reg(0xfc, 0x16); //4e
    wrGC0310Reg(0xfc, 0x16); //4e // [0]apwd [6]regf_clk_gate
    wrGC0310Reg(0xf2, 0x07); //sync output
    wrGC0310Reg(0xf3, 0x83); //ff//1f//01 data output
    wrGC0310Reg(0xf5, 0x07); //sck_dely

#if 1 //48M路径 (已验证能出帧)
    wrGC0310Reg(0xf7, 0x01);
    wrGC0310Reg(0xf8, 0x81);
    wrGC0310Reg(0xf9, 0x4e);
    wrGC0310Reg(0xfa, 0x32);
#else  //24M原始路径
    wrGC0310Reg(0xf7, 0x88);
    wrGC0310Reg(0xf8, 0x00);
    wrGC0310Reg(0xf9, 0x4f);
    wrGC0310Reg(0xfa, 0x32);
#endif

    wrGC0310Reg(0xfc, 0xce);
    wrGC0310Reg(0xfd, 0x00);
    /////////////////////////////////////
    /////   CISCTL reg  /////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x00, 0x2f);
    wrGC0310Reg(0x01, 0x0f);
    wrGC0310Reg(0x02, 0x04);

    wrGC0310Reg(0x03, 0x02);
    wrGC0310Reg(0x04, 0x12);

    wrGC0310Reg(0x09, 0x00);
    wrGC0310Reg(0x0a, 0x00);
    wrGC0310Reg(0x0b, 0x00);
    wrGC0310Reg(0x0c, 0x04);
    wrGC0310Reg(0x0d, 0x01);
    wrGC0310Reg(0x0e, 0xe8);
    wrGC0310Reg(0x0f, 0x02);
    wrGC0310Reg(0x10, 0x88);
    wrGC0310Reg(0x16, 0x00);
    wrGC0310Reg(0x17, 0x14);
    wrGC0310Reg(0x18, 0x1a);
    wrGC0310Reg(0x19, 0x14);
    wrGC0310Reg(0x1b, 0x48);
    wrGC0310Reg(0x1c, 0x1c);
    wrGC0310Reg(0x1e, 0x6b);
    wrGC0310Reg(0x1f, 0x28);
    wrGC0310Reg(0x20, 0x8b); //0x89 travis20140801
    wrGC0310Reg(0x21, 0x49);
    wrGC0310Reg(0x22, 0xb0);
    wrGC0310Reg(0x23, 0x04);
    wrGC0310Reg(0x24, 0xf2);
    /* wrGC0310Reg(0x24, 0x55);//pclk IO驱动能力 */
    wrGC0310Reg(0x34, 0x20);


    /////////////////////////////////////
    ////////   BLK   ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x26, 0x23);
    wrGC0310Reg(0x28, 0xff);
    wrGC0310Reg(0x29, 0x00);
    wrGC0310Reg(0x32, 0x00);
    wrGC0310Reg(0x33, 0x10);
    wrGC0310Reg(0x37, 0x20);
    wrGC0310Reg(0x38, 0x10);
    wrGC0310Reg(0x47, 0x80);
    wrGC0310Reg(0x4e, 0x66);
    wrGC0310Reg(0xa8, 0x02);
    wrGC0310Reg(0xa9, 0x80);


    /////////////////////////////////////
    //////   ISP reg  ///////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x40, 0xff);
    wrGC0310Reg(0x41, 0x21);
    wrGC0310Reg(0x42, 0xcf);
    wrGC0310Reg(0x44, 0x02);
    wrGC0310Reg(0x45, 0xa8);
    wrGC0310Reg(0x46, 0x02); //sync
    wrGC0310Reg(0x4a, 0x11);
    wrGC0310Reg(0x4b, 0x01);
    wrGC0310Reg(0x4c, 0x20);
    wrGC0310Reg(0x4d, 0x05);
    wrGC0310Reg(0x4f, 0x01); //AEC开启
    wrGC0310Reg(0x50, 0x01);
    wrGC0310Reg(0x55, 0x01);
    wrGC0310Reg(0x56, 0xe0);
    wrGC0310Reg(0x57, 0x02);
    wrGC0310Reg(0x58, 0x80);

    /////////////////////////////////////
    ///////   GAIN   ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x70, 0x70);
    wrGC0310Reg(0x5a, 0x84);
    wrGC0310Reg(0x5b, 0xc9);
    wrGC0310Reg(0x5c, 0xed);
    wrGC0310Reg(0x77, 0x74);
    wrGC0310Reg(0x78, 0x40);
    wrGC0310Reg(0x79, 0x5f);


    /////////////////////////////////////
    ///////   DNDD  /////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x82, 0x14);
    wrGC0310Reg(0x83, 0x0b);
    wrGC0310Reg(0x89, 0xf0);
    /////////////////////////////////////
    //////   EEINTP  ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x8f, 0xaa);
    wrGC0310Reg(0x90, 0x8c);
    wrGC0310Reg(0x91, 0x90);
    wrGC0310Reg(0x92, 0x03);
    wrGC0310Reg(0x93, 0x03);
    wrGC0310Reg(0x94, 0x05);
    wrGC0310Reg(0x95, 0x65);
    wrGC0310Reg(0x96, 0xf0);
    /////////////////////////////////////
    /////////  ASDE  ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x00);

    /* wrGC0310Reg(0x11,0x00); */

    wrGC0310Reg(0x9a, 0x20);
    wrGC0310Reg(0x9b, 0x80);
    wrGC0310Reg(0x9c, 0x40);
    wrGC0310Reg(0x9d, 0x80);

    wrGC0310Reg(0xa1, 0x30);
    wrGC0310Reg(0xa2, 0x32);
    wrGC0310Reg(0xa4, 0x30);
    wrGC0310Reg(0xa5, 0x30);
    wrGC0310Reg(0xaa, 0x10);
    wrGC0310Reg(0xac, 0x22);

    /////////////////////////////////////
    ///////   GAMMA   ///////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x00); //default
    wrGC0310Reg(0xbf, 0x08);
    wrGC0310Reg(0xc0, 0x16);
    wrGC0310Reg(0xc1, 0x28);
    wrGC0310Reg(0xc2, 0x41);
    wrGC0310Reg(0xc3, 0x5a);
    wrGC0310Reg(0xc4, 0x6c);
    wrGC0310Reg(0xc5, 0x7a);
    wrGC0310Reg(0xc6, 0x96);
    wrGC0310Reg(0xc7, 0xac);
    wrGC0310Reg(0xc8, 0xbc);
    wrGC0310Reg(0xc9, 0xc9);
    wrGC0310Reg(0xca, 0xd3);
    wrGC0310Reg(0xcb, 0xdd);
    wrGC0310Reg(0xcc, 0xe5);
    wrGC0310Reg(0xcd, 0xf1);
    wrGC0310Reg(0xce, 0xfa);
    wrGC0310Reg(0xcf, 0xff);
    /*
    	wrGC0310Reg(0xfe,0x00);//big gamma
    	wrGC0310Reg(0xbf,0x08);
    	wrGC0310Reg(0xc0,0x1d);
    	wrGC0310Reg(0xc1,0x34);
    	wrGC0310Reg(0xc2,0x4b);
    	wrGC0310Reg(0xc3,0x60);
    	wrGC0310Reg(0xc4,0x73);
    	wrGC0310Reg(0xc5,0x85);
    	wrGC0310Reg(0xc6,0x9f);
    	wrGC0310Reg(0xc7,0xb5);
    	wrGC0310Reg(0xc8,0xc7);
    	wrGC0310Reg(0xc9,0xd5);
    	wrGC0310Reg(0xca,0xe0);
    	wrGC0310Reg(0xcb,0xe7);
    	wrGC0310Reg(0xcc,0xec);
    	wrGC0310Reg(0xcd,0xf4);
    	wrGC0310Reg(0xce,0xfa);
    	wrGC0310Reg(0xcf,0xff);
    */

    /*
    	wrGC0310Reg(0xfe,0x00);//small gamma
    	wrGC0310Reg(0xbf,0x08);
    	wrGC0310Reg(0xc0,0x18);
    	wrGC0310Reg(0xc1,0x2c);
    	wrGC0310Reg(0xc2,0x41);
    	wrGC0310Reg(0xc3,0x59);
    	wrGC0310Reg(0xc4,0x6e);
    	wrGC0310Reg(0xc5,0x81);
    	wrGC0310Reg(0xc6,0x9f);
    	wrGC0310Reg(0xc7,0xb5);
    	wrGC0310Reg(0xc8,0xc7);
    	wrGC0310Reg(0xc9,0xd5);
    	wrGC0310Reg(0xca,0xe0);
    	wrGC0310Reg(0xcb,0xe7);
    	wrGC0310Reg(0xcc,0xec);
    	wrGC0310Reg(0xcd,0xf4);
    	wrGC0310Reg(0xce,0xfa);
    	wrGC0310Reg(0xcf,0xff);
    */
    /////////////////////////////////////
    ///////   YCP  //////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xd0, 0x40);
    wrGC0310Reg(0xd1, 0x34);
    wrGC0310Reg(0xd2, 0x34);
    wrGC0310Reg(0xd3, 0x40);
    wrGC0310Reg(0xd6, 0xf2);
    wrGC0310Reg(0xd7, 0x1b);
    wrGC0310Reg(0xd8, 0x18);
    wrGC0310Reg(0xdd, 0x03);

    /////////////////////////////////////
    ////////   AEC   ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x01);
    wrGC0310Reg(0x05, 0x30);
    wrGC0310Reg(0x06, 0x75);
    wrGC0310Reg(0x07, 0x40);
    wrGC0310Reg(0x08, 0xb0);
    wrGC0310Reg(0x0a, 0xc5);
    wrGC0310Reg(0x0b, 0x11);
    wrGC0310Reg(0x0c, 0x00);
    wrGC0310Reg(0x12, 0x52);
    wrGC0310Reg(0x13, 0x38);
    wrGC0310Reg(0x18, 0x95);
    wrGC0310Reg(0x19, 0x96);
    wrGC0310Reg(0x1f, 0x20);
    wrGC0310Reg(0x20, 0xc0);
    wrGC0310Reg(0x3e, 0x40);
    wrGC0310Reg(0x3f, 0x57);
    wrGC0310Reg(0x40, 0x7d);

    wrGC0310Reg(0x03, 0x60);

    wrGC0310Reg(0x44, 0x02);
    /////////////////////////////////////
    ////////   AWB   ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x01);
    wrGC0310Reg(0x1c, 0x91);
    wrGC0310Reg(0x21, 0x15);
    wrGC0310Reg(0x50, 0x80);
    wrGC0310Reg(0x56, 0x04);
    wrGC0310Reg(0x59, 0x08);
    wrGC0310Reg(0x5b, 0x02);
    wrGC0310Reg(0x61, 0x8d);
    wrGC0310Reg(0x62, 0xa7);
    wrGC0310Reg(0x63, 0xd0);
    wrGC0310Reg(0x65, 0x06);
    wrGC0310Reg(0x66, 0x06);
    wrGC0310Reg(0x67, 0x84);
    wrGC0310Reg(0x69, 0x08);
    wrGC0310Reg(0x6a, 0x25);
    wrGC0310Reg(0x6b, 0x01);
    wrGC0310Reg(0x6c, 0x00);
    wrGC0310Reg(0x6d, 0x02);
    wrGC0310Reg(0x6e, 0xf0);
    wrGC0310Reg(0x6f, 0x80);
    wrGC0310Reg(0x76, 0x80);
    wrGC0310Reg(0x78, 0xaf);
    wrGC0310Reg(0x79, 0x75);
    wrGC0310Reg(0x7a, 0x40);
    wrGC0310Reg(0x7b, 0x50);
    wrGC0310Reg(0x7c, 0x0c);

    wrGC0310Reg(0x90, 0xc9); //stable AWB
    wrGC0310Reg(0x91, 0xbe);
    wrGC0310Reg(0x92, 0xe2);
    wrGC0310Reg(0x93, 0xc9);
    wrGC0310Reg(0x95, 0x1b);
    wrGC0310Reg(0x96, 0xe2);
    wrGC0310Reg(0x97, 0x49);
    wrGC0310Reg(0x98, 0x1b);
    wrGC0310Reg(0x9a, 0x49);
    wrGC0310Reg(0x9b, 0x1b);
    wrGC0310Reg(0x9c, 0xc3);
    wrGC0310Reg(0x9d, 0x49);
    wrGC0310Reg(0x9f, 0xc7);
    wrGC0310Reg(0xa0, 0xc8);
    wrGC0310Reg(0xa1, 0x00);
    wrGC0310Reg(0xa2, 0x00);
    wrGC0310Reg(0x86, 0x00);
    wrGC0310Reg(0x87, 0x00);
    wrGC0310Reg(0x88, 0x00);
    wrGC0310Reg(0x89, 0x00);
    wrGC0310Reg(0xa4, 0xb9);
    wrGC0310Reg(0xa5, 0xa0);
    wrGC0310Reg(0xa6, 0xba);
    wrGC0310Reg(0xa7, 0x92);
    wrGC0310Reg(0xa9, 0xba);
    wrGC0310Reg(0xaa, 0x80);
    wrGC0310Reg(0xab, 0x9d);
    wrGC0310Reg(0xac, 0x7f);
    wrGC0310Reg(0xae, 0xbb);
    wrGC0310Reg(0xaf, 0x9d);
    wrGC0310Reg(0xb0, 0xc8);
    wrGC0310Reg(0xb1, 0x97);
    wrGC0310Reg(0xb3, 0xb7);
    wrGC0310Reg(0xb4, 0x7f);
    wrGC0310Reg(0xb5, 0x00);
    wrGC0310Reg(0xb6, 0x00);
    wrGC0310Reg(0x8b, 0x00);
    wrGC0310Reg(0x8c, 0x00);
    wrGC0310Reg(0x8d, 0x00);
    wrGC0310Reg(0x8e, 0x00);
    wrGC0310Reg(0x94, 0x55);
    wrGC0310Reg(0x99, 0xa6);
    wrGC0310Reg(0x9e, 0xaa);
    wrGC0310Reg(0xa3, 0x0a);
    wrGC0310Reg(0x8a, 0x00);
    wrGC0310Reg(0xa8, 0x55);
    wrGC0310Reg(0xad, 0x55);
    wrGC0310Reg(0xb2, 0x55);
    wrGC0310Reg(0xb7, 0x05);
    wrGC0310Reg(0x8f, 0x00);
    wrGC0310Reg(0xb8, 0xcb);
    wrGC0310Reg(0xb9, 0x9b);
    /*
    	wrGC0310Reg(0xa4,0xb9); //default AWB
    	wrGC0310Reg(0xa5,0xa0);
    	wrGC0310Reg(0x90,0xc9);
    	wrGC0310Reg(0x91,0xbe);
    	wrGC0310Reg(0xa6,0xb8);
    	wrGC0310Reg(0xa7,0x95);
    	wrGC0310Reg(0x92,0xe6);
    	wrGC0310Reg(0x93,0xca);
    	wrGC0310Reg(0xa9,0xbc);
    	wrGC0310Reg(0xaa,0x95);
    	wrGC0310Reg(0x95,0x23);
    	wrGC0310Reg(0x96,0xe7);
    	wrGC0310Reg(0xab,0x9d);
    	wrGC0310Reg(0xac,0x80);
    	wrGC0310Reg(0x97,0x43);
    	wrGC0310Reg(0x98,0x24);
    	wrGC0310Reg(0xae,0xb7);
    	wrGC0310Reg(0xaf,0x9e);
    	wrGC0310Reg(0x9a,0x43);
    	wrGC0310Reg(0x9b,0x24);
    	wrGC0310Reg(0xb0,0xc8);
    	wrGC0310Reg(0xb1,0x97);
    	wrGC0310Reg(0x9c,0xc4);
    	wrGC0310Reg(0x9d,0x44);
    	wrGC0310Reg(0xb3,0xb7);
    	wrGC0310Reg(0xb4,0x7f);
    	wrGC0310Reg(0x9f,0xc7);
    	wrGC0310Reg(0xa0,0xc8);
    	wrGC0310Reg(0xb5,0x00);
    	wrGC0310Reg(0xb6,0x00);
    	wrGC0310Reg(0xa1,0x00);
    	wrGC0310Reg(0xa2,0x00);
    	wrGC0310Reg(0x86,0x60);
    	wrGC0310Reg(0x87,0x08);
    	wrGC0310Reg(0x88,0x00);
    	wrGC0310Reg(0x89,0x00);
    	wrGC0310Reg(0x8b,0xde);
    	wrGC0310Reg(0x8c,0x80);
    	wrGC0310Reg(0x8d,0x00);
    	wrGC0310Reg(0x8e,0x00);
    	wrGC0310Reg(0x94,0x55);
    	wrGC0310Reg(0x99,0xa6);
    	wrGC0310Reg(0x9e,0xaa);
    	wrGC0310Reg(0xa3,0x0a);
    	wrGC0310Reg(0x8a,0x0a);
    	wrGC0310Reg(0xa8,0x55);
    	wrGC0310Reg(0xad,0x55);
    	wrGC0310Reg(0xb2,0x55);
    	wrGC0310Reg(0xb7,0x05);
    	wrGC0310Reg(0x8f,0x05);
    	wrGC0310Reg(0xb8,0xcc);
    	wrGC0310Reg(0xb9,0x9a);
    */

    /////////////////////////////////////
    ////////  CC ////////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x01);

    wrGC0310Reg(0xd0, 0x38); //skin red
    wrGC0310Reg(0xd1, 0x00);
    wrGC0310Reg(0xd2, 0x02);
    wrGC0310Reg(0xd3, 0x04);
    wrGC0310Reg(0xd4, 0x38);
    wrGC0310Reg(0xd5, 0x12);
    /*
    	wrGC0310Reg(0xd0,0x38);//skin white
    	wrGC0310Reg(0xd1,0xfd);
    	wrGC0310Reg(0xd2,0x06);
    	wrGC0310Reg(0xd3,0xf0);
    	wrGC0310Reg(0xd4,0x40);
    	wrGC0310Reg(0xd5,0x08);
    */
    /*
    	wrGC0310Reg(0xd0,0x38);//guodengxiang
    	wrGC0310Reg(0xd1,0xf8);
    	wrGC0310Reg(0xd2,0x06);
    	wrGC0310Reg(0xd3,0xfd);
    	wrGC0310Reg(0xd4,0x40);
    	wrGC0310Reg(0xd5,0x00);
    */
    wrGC0310Reg(0xd6, 0x30);
    wrGC0310Reg(0xd7, 0x00);
    wrGC0310Reg(0xd8, 0x0a);
    wrGC0310Reg(0xd9, 0x16);
    wrGC0310Reg(0xda, 0x39);
    wrGC0310Reg(0xdb, 0xf8);
    /////////////////////////////////////
    ////////   LSC   ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x01);
    wrGC0310Reg(0xc1, 0x3c);
    wrGC0310Reg(0xc2, 0x50);
    wrGC0310Reg(0xc3, 0x00);
    wrGC0310Reg(0xc4, 0x40);
    wrGC0310Reg(0xc5, 0x30);
    wrGC0310Reg(0xc6, 0x30);
    wrGC0310Reg(0xc7, 0x10);
    wrGC0310Reg(0xc8, 0x00);
    wrGC0310Reg(0xc9, 0x00);
    wrGC0310Reg(0xdc, 0x20);
    wrGC0310Reg(0xdd, 0x10);
    wrGC0310Reg(0xdf, 0x00);
    wrGC0310Reg(0xde, 0x00);


    /////////////////////////////////////
    ///////  Histogram  /////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x01, 0x10);
    wrGC0310Reg(0x0b, 0x31);
    wrGC0310Reg(0x0e, 0x50);
    wrGC0310Reg(0x0f, 0x0f);
    wrGC0310Reg(0x10, 0x6e);
    wrGC0310Reg(0x12, 0xa0);
    wrGC0310Reg(0x15, 0x60);
    wrGC0310Reg(0x16, 0x60);
    wrGC0310Reg(0x17, 0xe0);


    /////////////////////////////////////
    //   Measure Window   ///////////////
    /////////////////////////////////////
    wrGC0310Reg(0xcc, 0x0c);
    wrGC0310Reg(0xcd, 0x10);
    wrGC0310Reg(0xce, 0xa0);
    wrGC0310Reg(0xcf, 0xe6);


    /////////////////////////////////////
    /////   dark sun   //////////////////
    /////////////////////////////////////
    wrGC0310Reg(0x45, 0xf7);
    wrGC0310Reg(0x46, 0xff);
    wrGC0310Reg(0x47, 0x15);
    wrGC0310Reg(0x48, 0x03);
    wrGC0310Reg(0x4f, 0x60);

    /////////////////////////////////////
    ///////  banding  ///////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x00);
    wrGC0310Reg(0x05, 0x01);
    wrGC0310Reg(0x06, 0x2C); //HB=300 (12fps)
    wrGC0310Reg(0x07, 0x00);
    wrGC0310Reg(0x08, 0x08); //VB=8


    wrGC0310Reg(0xfe, 0x01);
    wrGC0310Reg(0x25, 0x00); //step
    wrGC0310Reg(0x26, 0x1f); //恢复原值
    wrGC0310Reg(0x27, 0x02); //20fps
    wrGC0310Reg(0x28, 0x12);
    wrGC0310Reg(0x29, 0x03); //12.5fps
    wrGC0310Reg(0x2a, 0x50);
    wrGC0310Reg(0x2b, 0x04); //8.33fps
    wrGC0310Reg(0x2c, 0xf8);
    wrGC0310Reg(0x2d, 0x07); //5.55fps
    wrGC0310Reg(0x2e, 0x74);
    wrGC0310Reg(0x3c, 0x20);
    /////////  SPI   ////////////////////
    /////////////////////////////////////
    wrGC0310Reg(0xfe, 0x03);

    wrGC0310Reg(0x02, 0xff);
    wrGC0310Reg(0x03, 0xff);

    wrGC0310Reg(0x51, 0x01);
    wrGC0310Reg(0x52, 0xd8);
    wrGC0310Reg(0x53, 0x25); //[7]crc [3:2]line_num
    wrGC0310Reg(0x54, 0x20);
    wrGC0310Reg(0x55, 0x00);
    wrGC0310Reg(0x59, 0x10);
    wrGC0310Reg(0x5a, 0x00);
    wrGC0310Reg(0x5b, 0x80);
    wrGC0310Reg(0x5c, 0x02);
    wrGC0310Reg(0x5d, 0xe0);
    wrGC0310Reg(0x5e, 0x01);
    wrGC0310Reg(0x60, 0xab);
    wrGC0310Reg(0x61, 0x80);
    wrGC0310Reg(0x62, 0x9d);
    wrGC0310Reg(0x63, 0xb6);
    wrGC0310Reg(0x64, 0x01); //[1]sck always 06 04
    wrGC0310Reg(0x65, 0xff); //head sync code
    wrGC0310Reg(0x66, 0x00);
    wrGC0310Reg(0x67, 0x00);
}
static s32 GC0310_set_output_size(u8 isp_dev, u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width;
    u16 liv_height = *height;
    return 0;
}

/* ========== 摄像头 VCC 电源控制 ========== */
/* 按 TCFG_CAMERA_SPI_VCC 配置选择供电方式:
 *   IO_PORT_LDO28  -> LDO28 供电 3.0V
 *   SD_PWR_SDPG    -> SDPG 供电
 *   普通 GPIO      -> GPIO 强驱供电(高/低电平使能见 TCFG_CAMERA_SPI_POWER_IO_CTRL)
 */
static void GC0310_vcc_power_set(u8 on_off)
{
#if (TCFG_CAMERA_SPI_VCC == IO_PORT_LDO28)
    /* LDO28 供电 3.0V */
    iovdd28_config(on_off, IOVDD28_VOL_3P0V);
#elif (TCFG_CAMERA_SPI_VCC == SD_PWR_SDPG)
    /* SDPG 供电 */
    if (on_off) {
        sdpg_config(SDPG_IO_OUTPUT_H_HD_HD0_SDPG_USER);
    } else {
        sdpg_config(SDPG_IO_HI_Z);
    }
#elif (TCFG_CAMERA_SPI_VCC != NO_CONFIG_PORT)
    /* 普通 GPIO 强驱供电 */
    if (on_off) {
        gpio_set_drive_strength(IO_PORT_SPILT(TCFG_CAMERA_SPI_VCC), PORT_DRIVE_STRENGT_8p0mA);
        gpio_set_mode(IO_PORT_SPILT(TCFG_CAMERA_SPI_VCC), TCFG_CAMERA_SPI_POWER_IO_CTRL);
        udelay(200);
        gpio_set_drive_strength(IO_PORT_SPILT(TCFG_CAMERA_SPI_VCC), PORT_DRIVE_STRENGT_64p0mA);
    } else {
        gpio_set_mode(IO_PORT_SPILT(TCFG_CAMERA_SPI_VCC), !TCFG_CAMERA_SPI_POWER_IO_CTRL);
    }
#endif
}

static s32 GC0310_power_ctl(u8 isp_dev, u8 is_work)
{
    GC0310_vcc_power_set(is_work);
    return 0;
}

static s32 GC0310_ID_check(void)
{
    u8 pid = 0x00;
    u16 id = 0;

    rdGC0310Reg(0xf0, &pid);
    id |= (pid << 8) & 0xff00;
    rdGC0310Reg(0xf1, &pid);
    id |= pid;
    printf("GC0310 Sensor ID : 0x%x\n", id);

    if (id != 0xa310) {
        return -1;
    }

    return 0;
}

void GC0310_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0310_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0310 reset\n");

    res_io = GC0310_reset_io;
    powd_io = GC0310_power_io;

    if (powd_io != (u8) - 1) {
        GC0310_powerio_ctl(powd_io, 0);
    }

    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}

s32 GC0310_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio, char *iic_name)
{
    /* disable_dev 会断电但不重置 iic_init_state, 每次 check 都要重新上电 */
    GC0310_vcc_power_set(1);

    if (iic_init_state == -1) {
        struct iic_master_config iic_config = {
            .role = IIC_MASTER,
            .scl_io = TCFG_CAMERA_SPI_SCL,
            .sda_io = TCFG_CAMERA_SPI_SDA,
            .io_mode = PORT_INPUT_PULLUP_10K,//上拉或浮空
            .hdrive = PORT_DRIVE_STRENGT_2p4mA,   //enum GPIO_HDRIVE 0:2.4MA, 1:8MA, 2:26.4MA, 3:40MA
            .master_frequency = TCFG_HW_I2C0_CLK,  //IIC通讯波特率
            .io_filter = 1,  //软件无效
            /* .ie_en = 1,//1:注册中断 */
            /* .irq_priority = 3,//优先级 */
        };

        iic_init_state = iic_init(iic_dev, &iic_config);
        printf("iic init state: %d\n", iic_init_state);
        GC0310_reset_io = (u8)_reset_gpio;
        GC0310_power_io = (u8)_power_gpio;
    }

    if (iic_init_state != IIC_OK) {
        printf("GC0310 iic open err!!!\n\n");
        GC0310_vcc_power_set(0);
        return -1;
    }

    GC0310_reset(isp_dev);

    if (0 != GC0310_ID_check()) {
        printf("-------not GC0310------\n\n");
        iic_deinit(iic_dev);
        iic_init_state = -1;
        GC0310_vcc_power_set(0);
        return -1;
    }

    printf("-------hello GC0310------\n\n");
    return 0;
}


s32 GC0310_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0310_init \n\n");

    if (0 != GC0310_check(isp_dev, 0, 0, NULL)) {
        return -1;
    }

    GC0310_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0310(u16 rev_flag)
{
    if (!rev_flag) {
        wrGC0310Reg(0x17, 0x17);
    } else {
        wrGC0310Reg(0x17, 0x14);
    }
}

//注意这个地方是假休眠函数 主要用于开源板两个摄像头io互斥操作
void GC0310_in_sleep_mode(void)
{
    wrGC0310Reg(0xfe, 0x00);
    wrGC0310Reg(0xf3, 0x00);
}

void GC0310_out_sleep_mode(u16 addr)
{
    wrGC0310Reg(0xfe, 0x00);
    wrGC0310Reg(0xf3, 0x83);
}

u16 GC0310_dvp_rd_reg(u8 isp_dev, u16 addr)
{
    u8 val;
    rdGC0310Reg((u8)addr, &val);
    return val;
}

void GC0310_dvp_wr_reg(u8 isp_dev, u16 addr, u16 val)
{
    wrGC0310Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0310) = {
    .logo 				= 	"GC0310",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_YUYV,
    .mbus_type          =   SEN_MBUS_BT656,
    .mbus_config        =   SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,
    .fps         		= 	CONFIG_INPUT_FPS,
    .sen_size 			= 	{GC0310_DEVP_INPUT_W, GC0310_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0310_DEVP_INPUT_W, GC0310_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0310_check,
        .init 		        = 	GC0310_init,
        .set_size_fps 		=	GC0310_set_output_size,
        .power_ctrl         =   GC0310_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0310_dvp_wr_reg,
        .read_reg 		    =	GC0310_dvp_rd_reg,
        /* .set_sensor_reverse =   set_rev_sensor_GC0310, */
    }
};


#endif /* #if TCFG_CAMERA_SPI_ENABLE */





