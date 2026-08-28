#include "product_test.h"
#include "pt_gsensor.h"
#include "sensor_hub.h"
#include "health_manager.h"
#include "asm/math_fast_function.h"
#if (PT_GSENSOR_ENABLE && TCFG_SENSOR_HUB)
/*****************************************************/
//				PT_GSENSOR_CONFIG
/*****************************************************/

#define GSENSOR_CHECK_DATA_LEN 	(3*32) 	// 三轴*32组数据
#define GRAVITY_THR_MAX			(1024 + 100)		// 静态重力最大阈值
#define GRAVITY_THR_MIN			(1024 - 100)		// 静态重力最小阈值

static u8 pt_gsensor_busy = 0;      	// 忙碌标记
static u8 pt_gsensor_res = 0;      		// 测试结果
static u8 pt_gsensor_id[4] = {0};		// ID
/*****************************************************/
//				PT_SENSOR_DEBUG
/*****************************************************/
#define PT_GSENSOR_DEBUG 			1		// DEBUG
#define PT_GSENSOR_ERR_DEBUG		1		// DEBUG

#if PT_GSENSOR_DEBUG
#define pt_log_i(format, ...) printf("[PT_SENSOR(INFO)]@FUNC<%s>"format"\n", __func__, ##__VA_ARGS__)
#else
#define pt_log_i(format, ...)
#endif
#if PT_GSENSOR_ERR_DEBUG
#define pt_log_e(format, ...) printf("[PT_SENSOR(ERROR)]@FUNC<%s>"format"\n", __func__, ##__VA_ARGS__)
#else
#define pt_log_e(format, ...)
#endif
//***********************************************************//
//				获取传感器数据
// @ func:		pt_xxx_data_get
// @ return: 	0 		获取不到数据
//				other	数据长度
//***********************************************************//
static int pt_gsensor_data_get(void *data_buf, int data_len)
{
    return sensor_hub_get_data(SENSOR_DRV_ACCELER, 0, data_buf, data_len);
}
//***********************************************************//
//				校验数据是否合法
// @ func:		pt_xxx_data_check
// @ return: 	PT_E_OK(0) 	数据属于正常值
//				other		数据异常
// @ note：		正常数据范围参考sensor厂提供的数据手册/量产测试说明
//***********************************************************//
static int pt_gsensor_data_check(void *data_buf, int data_len)
{
    pt_log_i("%s len:%d", __func__, data_len);
    s16 *sensor_buf = (s16 *)data_buf;
    int point_num  = data_len / sizeof(s16);
    float gravity = 0;
    u16 avg_cnt = 0;
    //计算重力均值 gravity = (x^2+y^2+z^2)^(1/2)
    for (int i = 0; i < point_num; i += 3) {
        s16 gravity_x = sensor_buf[i];
        s16 gravity_y = sensor_buf[i + 1];
        s16 gravity_z = sensor_buf[i + 2];

        float gravity_tmp = root_float((float)(gravity_x * gravity_x + gravity_y * gravity_y + gravity_z * gravity_z));
        gravity += gravity_tmp;
        avg_cnt ++;
        pt_log_i("[%d] gavityg:%.3f gravity_tmp%.3f |x,y,z|=|%d,%d,%d|\n", i, (gravity / avg_cnt), gravity_tmp, gravity_x, gravity_y, gravity_z);
    }
    float gravity_avg = gravity / avg_cnt;
    pt_log_i("gravity_avg:%.3f", gravity_avg);
    // 判断数据是否异常
    if (gravity_avg <= GRAVITY_THR_MIN) {
        pt_log_e("gravity_avg <= GRAVITY_THR_MIN\n");
        return PT_E_MOD_ERROR;
    } else if (gravity_avg >= GRAVITY_THR_MAX) {
        pt_log_e("gravity_avg >= GRAVITY_THR_MAX\n");
        return PT_E_MOD_ERROR;
    }
    pt_log_i("gravity_avg is normal");

    return PT_E_OK;

}
static int pt_gsensor_test(int priv)
{

    u32 result = PT_E_OK;


    // 获取gsensor数据
    u16 sensor_data_point = GSENSOR_CHECK_DATA_LEN;
    u16 sensor_data_len = sensor_data_point * sizeof(u16);
    u16 *sensor_data = (u16 *)zalloc(sensor_data_len);
    ASSERT(sensor_data);

    u16 read_buf_len = pt_gsensor_data_get(sensor_data, sensor_data_len);
    if (!read_buf_len) {//数据为空
        pt_log_e("GSENSOR_read_buf_len null\n");
        result = PT_E_MOD_ERROR;
    } else {
        result = pt_gsensor_data_check(sensor_data, read_buf_len);
        if (result !=  PT_E_OK) {
            pt_log_e("pt_gsensor_data_check err");
        }

    }
    if (sensor_data) {
        free(sensor_data);
        sensor_data = NULL;
    }

    pt_gsensor_busy = 0;
    pt_gsensor_res = result;

    return 0;
}

int pt_gsensor_init(void)
{
    sensor_info_t *info = sensor_hub_get_info(SENSOR_DRV_ACCELER);
    if (!info) { //设备是否注册
        pt_log_e("pt_gsensor_dev_not_find\n");
        return PT_E_NO_DEV;
    }

    sensor_hub_enable(SENSOR_DRV_ACCELER, 1);
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, 0, 1);
    /* if(info->state ==SENSOR_STATE_OFFLINE){//设备是否在线 */
    /* pt_log_e("pt_gsensor_state_error\n"); */
    /* return PT_E_DEV_ID; */
    /* } */
    return PT_E_OK;
}

int pt_gsensor_start(void)
{
    if (pt_gsensor_busy) {
        return PT_E_MOD_RUN;
    }
    pt_gsensor_res = PT_E_MOD_RUN;
    int msg[3] = {0};
    msg[0] = (int)pt_gsensor_test;
    msg[1] = 1;
    msg[2] = (int)0;
    do {
        int os_err = os_taskq_post_type("app_core", Q_CALLBACK, 3, msg);
        if (os_err == OS_ERR_NONE) {
            break;
        }
        if (os_err != OS_Q_FULL) {
            pt_gsensor_res = PT_E_SYS_ERROR;
            return PT_E_SYS_ERROR;
        }
        os_time_dly(1);
    } while (1);

    pt_gsensor_busy = 1;

    return 0;
}

int pt_gsensor_stop(void)
{
    if (pt_gsensor_busy) {
        return PT_E_MOD_CANT_STOP;
    }
    sensor_hub_enable(SENSOR_DRV_ACCELER, 0);
    sensor_hub_cbuf_enable(SENSOR_DRV_ACCELER, 0, 0);
    if (pt_gsensor_res == PT_E_MOD_RUN) {
        pt_gsensor_res = PT_E_MOD_STOP_NO_END;
    }
    return 0;
}

int pt_gsensor_ioctrl(u32 order, int len, void *param)
{
    u32 result = 0;
    switch (PT_ORDER_C_GET(order)) {
    case PT_N_C_START:
        result = pt_gsensor_start();
        break;
    case PT_N_C_STOP:

        result = pt_gsensor_stop();
        break;
    case PT_N_C_GETINFO:
        u8 info[8];
        memcpy(&info[0], &result, 4);
        memcpy(&info[4], pt_gsensor_id, 4);
        product_test_push_data(order, sizeof(info), (u8 *)info);
        return 0;
    case PT_N_C_GET_RESULT:
        result = pt_gsensor_res;
        product_test_push_data(order, 4, (u8 *)&result);
        return 0;
    default:

        result = PT_E_PARAM;
        break;
    }
    product_test_push_data(order, 4, (u8 *)&result);
    return result;
}

REGISTER_PT_MODULE(gsensor) = {
    .module = PT_M_GSENSOR,
    .attr	= PT_ATTR_SELF,
    .init	= pt_gsensor_init,
    .ioctrl	= pt_gsensor_ioctrl,
};

int pt_gsensor_simulation_test(void)
{
    pt_gsensor_init();
    os_time_dly(100);
    pt_gsensor_test(0);
    pt_gsensor_stop();
    return 0;
}

#endif /* #if PT_GSENSOR_ENABLE */

