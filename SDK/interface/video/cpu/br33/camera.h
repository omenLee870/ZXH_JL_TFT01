#ifndef DEVICE_CAMERA_H
#define DEVICE_CAMERA_H



#include "device/device.h"
#include "isp_dev.h"
// #include "video/video.h"
// #include "sensor_dev.h"
#include "video.h"


#define VIDEO_TAG_CAMERA        VIDEO_TAG('c', 'a', 'm', 'e')
#define VIDEO_TAG_UVC           VIDEO_TAG('u', 'v', 'c', ' ')
#define VIDEO_TAG_MASS           VIDEO_TAG('m', 'a', 's', 's')


#define ENDPOINT_NUM 	5
#define CAMERA_DEVICE_NUM  3

#define CSI2_X0_LANE    0
#define CSI2_X1_LANE    1
#define CSI2_X2_LANE    2
#define CSI2_X3_LANE    3
#define CSI2_X4_LANE    4

#define DVP_GROUP_A		0
#define DVP_GROUP_B		1
#define DVP_GROUP_C		2
#define DVP_GROUP_D		3

struct camera_platform_data {
    char *iic_sel;

    //可自定义IO
    u8 xclk_gpio;
    u8 reset_gpio;
    u8 pwdn_gpio;

    u8 power_value;
    u32 interface;
    bool (*online_detect)();
    union {
        struct {
            /****************
            信号名称	    GROUP_A (推荐)	    GROUP_B	    GROUP_C	    GROUP_D
            PCK (像素时钟)	PB2	                PB2	        PH1	        PC4
            VSYNC (场同步)	PA9	                PB0	        PD10	    PC6
            HSYNC (行同步)	PA10	            PB1	        PD11	    PC5
            D0	            PB0	                PB3	        PH7	        PC6(复用)
            D1	            PB1	                PB4	        PH6	        PC5(复用)
            D2	            PB3	                PB5	        PH5	        PC3
            D3	            PB4	                PB6	        PH4	        PC2
            D4	            PB5	                PC0	        PH3	        PC1
            D5	            PB6	                PC1	        PH2	        PC0
            D6	            PA8	                PC2	        PD9	        PG3
            D7	            PA7	                PC3	        PD8	        PG2
            D8	            PA6	                PC4	        -	        PG1
            D9	            PA5	                PC5	        -	        PG0
            注: 1. C组口只有8bit
                2. D组口VS HS复用D0 D1, SENSOR用到VS HS只能8bit,反之可10bit(BT656)
            **************/
            u8 group_port;
            //固定IO
            u32 pclk_gpio;
            u32 vsync_gpio;
            u32 hsync_gpio;
            u32 data_gpio[10];

            u8 msb_position; //摄像头数据引脚MSB接到芯片sen接口的标号
            u8 reverse;     //0: 正序 1: 反序
            u8 bus_width; //摄像头有效数据宽度
            u8 timing_mode; //时序选择 0:DVP 1:BT601 2:VSYNC 3:BT656
            u8 scan_mode;   //扫描模式 0:逐行扫描(default) 1:奇场数据 2:偶场数据
            u8 width_mode;  //位宽选择 0:全位宽(8/10bit) 1: 4bit 2: 2bit 3: 1bit
            u32 h_blank; //水平消隐后肩周期数, 默认填0
            u32 v_blank; //垂直消隐后肩行数, 默认填0
        } dvp;

        struct {
            u8 data_lane_num;
            u8 clk_rmap;
            u8 clk_inv;
            u8 d0_rmap;
            u8 d0_inv;
            u8 d1_rmap;
            u8 d1_inv;
            u8 d2_rmap;
            u8 d2_inv;
            u8 d3_rmap;
            u8 d3_inv;
            u8 tval_hstt;
            u8 tval_stto;
        } csi2;
    };
};

struct camera_device_info {
    u16 fps;
    u16 sen_width;
    u16 sen_height;
    u16 width;
    u16 height;
    u32 real_fps;
};

struct camera_device {
    u8 online;
    u8 detect_cnt;
    u8 remount;
    atomic_t ref;
    int timer;
    void *sensor;
    struct camera_device_info info;
    struct video_endpoint ep[ENDPOINT_NUM];
    const struct camera_platform_data *data;
};


#define CAMERA_PLATFORM_DATA_BEGIN(data) \
	static const struct camera_platform_data data = { \


#define CAMERA_PLATFORM_DATA_END() \
	};

#define CAMERA_CMD_BASE		0x00400000
#define CAMERA_CROP_TRIG		(CAMERA_CMD_BASE + 4)
#define CAMERA_GET_SENSOR_ID    (CAMERA_CMD_BASE + 6)

extern const struct device_operations camera_dev_ops;
// extern struct camera_device;

//dvp sensor io sel
#define DVP_SENSOR0(sel)   (((sel?1:0) << 24) | (22))//sel 0 PC3~PC13  1 PA6~PA15
#define DVP_SENSOR1(sel)   (((sel?1:0) << 24) | (23))//sel 0 PH1~PH11  1 PD0~PD10


int camera_init(const char *name, void *camera_data);

void *sensor_driver_open(int id);
void *camera_driver_open(int id, struct camera_device_info *info);
int camera_driver_ioctl(void *_camera, u32 cmd, void *arg);

int camera_driver_close(void *_camera);
int camera_driver_force_close(void *_camera);

int camera_mux_init(const char *name, void *camera_data);
bool camera_mux_online(const char *name);
int camera_mux_close(void *_camera);
void *camera_mux_open(int id, struct camera_device_info *info);
void *camera_get_sen_dev(void *c);
void *camera_get_platform_data(void *c);
int camera_get_in_format(void *c);
int get_camera_devices_fps(int id);

int camera_dvp_io_init(u8 channel);



#endif

