#include "typedef.h"
/* #include "alipay_iot_kal.h" */
#include "alipay_common.h"
#include "alipay_transit.h"
#include "os/os_api.h"
#include "system/task.h"
#include "system/includes.h"
#include "app_config.h"
#include "ui_api.h"

#if TCFG_PAY_TRANSITCODE_ENABLE
#if (TCFG_PAY_ALIOS_WAY_SEL==TCFG_PAY_ALIOS_WAY_T_HEAD)

//版本升級替換函數
#define alipay_transit_get_default_card_info  alipay_transit_get_the_last_transcode_info
//被刪
/* alipay_transit_set_default_card_info */

extern int net_get_dhcp_flag();
extern int ui_show_transitcodes_list(u8);

extern void task_info_del(const char *name);
extern const int config_system_info;



enum {
    TRANSITCODE_GET_DEFAUL_CARD = 1,//获取默认参数
    TRANSITCODE_SET_DEFAUL_CARD = 2,//设置默认参数
    TRANSITCODE_LOCAL_GET_CARD_LIST  = 3,//获取本地卡片
    TRANSITCODE_SERVER_GET_CARD_LIST = 4,//获取服务器卡片
    TRANSITCODE_GET_QRCODED = 5,//获取二维码
};




//乘车码对内存消耗极大 因此这里专门任务处理

#define MAX_CODE_CITY 40

struct transit_param_t {
    u8 init;
    u8 task_init;
    u8 Card_no_need_release;
    int CardBase_Qr_len;//乘车码数字
    int CardBasenum; //卡片数
    int CardBaseindex;//设置的索引
    char *CardBase_Qr_Data;//乘车码数字
    alipay_tansit_CardBaseVO_t CardBaseVO;//全局信息
    alipay_tansit_CardBaseVO_t *CardBasebuf;

    OS_MUTEX mutex;
    OS_SEM start_sem;
};
static struct transit_param_t *transit_param = NULL;

void set_cardcode_no_release()
{
    if (!transit_param) {
        return;
    }
    transit_param->Card_no_need_release = 1;
}

u8 cardcode_is_need_release()
{
    if (!transit_param) {
        return 0;
    }
    return !transit_param->Card_no_need_release;
}

void clear_cardcode_no_release()
{
    if (!transit_param) {
        return;
    }
    transit_param->Card_no_need_release = 0;
}

void transit_task_param_init()
{
    if (!transit_param) {

        transit_param = zalloc(sizeof(struct transit_param_t));
    }
}

void transit_task_param_deinit()
{
    if (transit_param) {
        free(transit_param);
        transit_param = NULL;
    }
}

u8 transit_task_is_init()
{
    return transit_param->task_init;
}


extern int transit_code_data_callback(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list);

static int __alipay_tansit_get_card_data(int event, int (*callback)(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list));

static int __alipay_tansit_get_qr_data(int event, alipay_tansit_CardBaseVO_t *CardBasebuf, int (*callback)(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list));

u8 is_task_active;
u8 transitcode_task_idle_query(void)
{
    return !is_task_active;
}
REGISTER_LP_TARGET(transitcode_lp_target) = {
    .name = "transit",
    .is_idle = transitcode_task_idle_query,
};

static void transitcode_task(void *p)
{

    int msg[32];
    int ret;
    alipay_tansit_CardBaseVO_t *Cardsetdefaul;
    os_sem_post(&transit_param->start_sem);
    while (1) {
        is_task_active = 0;
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)); //500ms_reflash
        if (os_task_del_req(OS_TASK_SELF) == OS_TASK_DEL_REQ) {
            os_task_del_res(OS_TASK_SELF);
        }
        if (ret != OS_TASKQ) {
            continue;
        }

        if (!(msg[0] & Q_EVENT)) {
            continue;
        }

        int (*callback)(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list);
        callback  = (int (*)())msg[1];

        int event = msg[0] & 0xffff;
        is_task_active = 1;

        printf("%s msg :%d\n", __FUNCTION__, msg[0] & 0xffff);

        switch (msg[0] & 0xffff) {
        case TRANSITCODE_GET_DEFAUL_CARD:
            printf("%s %d\n", __FUNCTION__, __LINE__);

#if 0
            extern  int alipay_get_active_request_with_antchain(uint8_t *buf_output, int32_t *len_output);
            u8 table[1024] = {0};
            int table_len = 1024;

            int te_ret =  alipay_get_active_request_with_antchain(table, &table_len);
            printf("!!!!!!!!!!! %x %x\n", table_len, te_ret);
            put_buf(table, table_len);
#endif

            //ASSERT(0);



            if (net_get_dhcp_flag()) {
                /* alipay_pre_init(); */
            }

            ret = alipay_transit_get_default_card_info(&transit_param->CardBaseVO);
            if (callback) {
                callback(event, ret, 1, &transit_param->CardBaseVO, sizeof(alipay_tansit_CardBaseVO_t));
            }
            break;
        case TRANSITCODE_SET_DEFAUL_CARD:
            /* printf("%s %d\n",__FUNCTION__,__LINE__); */
            Cardsetdefaul = (alipay_tansit_CardBaseVO_t *)msg[2];
            /* ret = alipay_transit_set_default_card_info(Cardsetdefaul); */
            if (callback) {
                callback(event, ret, 1, Cardsetdefaul, sizeof(alipay_tansit_CardBaseVO_t));
            }
            break;
        case TRANSITCODE_LOCAL_GET_CARD_LIST:
            /* printf("%s %d\n",__FUNCTION__,__LINE__); */


            __alipay_tansit_get_card_data(event, callback);
            break;
        case TRANSITCODE_SERVER_GET_CARD_LIST:
            /* printf("%s %d\n",__FUNCTION__,__LINE__); */
            __alipay_tansit_get_card_data(event, callback);
            break;
        case TRANSITCODE_GET_QRCODED:
            /* printf("%s %d\n",__FUNCTION__,__LINE__); */
            Cardsetdefaul = (alipay_tansit_CardBaseVO_t *)msg[2];
            __alipay_tansit_get_qr_data(event, Cardsetdefaul, callback);
            break;
        default:
            break;
        }


    }
}




static int __alipay_tansit_get_cardcode(alipay_tansit_CardBaseVO_t *card_list, int (*callback)(int err, u8 *transitcode, int len_card_list))
{



    /***
     * 获取指定卡的乘车码码值
     * @param   [in]cardNo              卡序号
     *          [in]cardType            卡类型
     *          [out]transitcode        储存码值的缓存，buffer大小建议为512，字节码，不是字符串
     *          [inout]len_transitcode  输入时为允许获取的最大长度，输出时为实际获取的长度
     *          [out] error_message     错误原因，仅在返回值为RV_SERVER_FAIL_ERROR的时候使用
     *          [in] len_error_message  error_message的buffer大小，建议为256
     * @return  RV_OK 更新成功
     *          RV_CARD_DATA_LIMITED    由于风控原因生码受限
     *          RV_UNKNOWN              未知错误
     *          RV_BUF_TOO_SHORT        允许的最大长度小于实际获取的长度
     *          RV_CODE_GEN_ERROR       生码异常
     *          RV_NETWORK_ERROR        网络错误
     *          RV_SERVER_FAIL_ERROR    服务端返回错误，屏幕显示error_message参数输出的错误原因
     *          RV_COMMON_ERROR         显示“暂不支持，请重试或切换到手机侧使用”
     *          RV_UNSUPPORTED_CARD     不支持该乘车码，显示“暂不支持，请重试或切换到手机侧使用”
     **/
    u8 transitcode[512] = {0};
    int len_transitcode  = sizeof(transitcode);
    u8 error_message[256] = {0};
    memset(error_message, 0, sizeof(error_message));

    int ret =  alipay_transit_get_TransitCode(card_list[0].cardNo, card_list[0].cardType, transitcode, (uint32_t *)&len_transitcode, (char *)error_message, sizeof(error_message));
    if (callback) {
        callback(ret, transitcode, len_transitcode);
    }
    return 0;

}


static int __alipay_tansit_get_card_data(int event, int (*callback)(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list))
{

    /***
     * 获取卡列表
     * @param   [out]card_list          存放获取的卡列表
     *          [inout]len_card_list    输入时为允许获取的最大长度，输出时为实际获取的长度
     *          [out]card_num           获取的卡片数量
     *          [in]onlineFlag          1:优先从服务端获取；0:只获取本地保存的
     * @return  RV_OK 更新成功
     *          RV_JS_ERROR             CJSON库或接口错误
     *          RV_NETWORK_ERROR        网络错误
     *          RV_UNKNOWN              未知错误
     *          RV_SERVER_FAIL_ERROR    服务端返回错误
     *          RV_WRITE_ERROR          KV写错误，检查KV写接口的实现
     *          RV_BUF_TOO_SHORT        允许的最大长度小于实际获取的长度
     **/


    int onlineFlag = 0;
    if (event == TRANSITCODE_SERVER_GET_CARD_LIST) { //获取服务器卡片
        onlineFlag = 1;
    }



    alipay_tansit_CardBaseVO_t *card_list = (alipay_tansit_CardBaseVO_t *)zalloc(sizeof(alipay_tansit_CardBaseVO_t) * MAX_CODE_CITY);
    u32 len_card_list = sizeof(alipay_tansit_CardBaseVO_t) * MAX_CODE_CITY;
    u32 card_num = 0;
    char error_message[256];
    error_message[0] = 0;
    int ret = 0;
    if (onlineFlag) {
        ret =  alipay_transit_get_card_list_online(card_list, &len_card_list, &card_num);
    } else {
        ret =  alipay_transit_get_card_list_offline(card_list, &len_card_list, &card_num);
    }
    printf("alipay_transit_get_card_list ret %d,card_num %d,onlineFlag %d ,len %d\n", ret, card_num, onlineFlag, len_card_list);
    /* if (onlineFlag) { */
    if (1) {
        for (int i = 0; i < card_num; i++) {
            /* error_message[0] = 0; */
            /* ret = alipay_transit_update_card_data(card_list[i].cardNo, card_list[i].cardType, error_message, sizeof(error_message)); */
            /* if (ret) { */
            /*     printf("ret %x %s \n", ret, error_message); */
            /*     break; */
            /* } */
            printf("no: %s type: %s title: %s\n", card_list[i].cardNo, card_list[i].cardType, card_list[i].title);
        }
    }
    callback(event, ret, card_num, card_list, len_card_list);
    free(card_list);
    return ret;

}



static int __alipay_tansit_get_qr_data(int event, alipay_tansit_CardBaseVO_t *CardBasebuf, int (*callback)(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list))
{
    transit_param->CardBase_Qr_len = 512;
    if (transit_param->CardBase_Qr_Data) {
        transit_param->CardBase_Qr_Data  = realloc(transit_param->CardBase_Qr_Data, transit_param->CardBase_Qr_len);

    } else {
        transit_param->CardBase_Qr_Data  = zalloc(transit_param->CardBase_Qr_len);
    }
    char error_message[256];
    memset(error_message, 0, sizeof(error_message));

    /* typedef struct { */
    /*     bool    is_exists;              //本地是否有缓存； */
    /*     int32_t expire_timestamp;       //过期时间 */
    /*     int32_t remain_use_count;       //未来12小时剩余使用次数 */
    /* } alipay_transit_card_status_t; */
    alipay_transit_card_status_t card_status_t = {0};
    alipay_transit_check_card_status(CardBasebuf[0].cardNo, CardBasebuf[0].cardType, &card_status_t);
    /* printf("\n card_status_t,is_exists :%d, expire_timestamp :%d,remain_use_count :%d\n",card_status_t.is_exists,card_status_t.expire_timestamp,card_status_t.remain_use_count); */
    extern time_t time(time_t *timer);
    int32_t timestamp =  time(NULL);
    //最后10分钟也不给用
    if ((!card_status_t.is_exists) || !card_status_t.remain_use_count || (timestamp + 60 * 10 > card_status_t.expire_timestamp)) {
        if (callback(event, RV_CARD_DATA_OVERDUE, 0, (alipay_tansit_CardBaseVO_t *)transit_param->CardBase_Qr_Data, 0)) {
            return 0;
        }//卡数据过期
    }

    int ret =  alipay_transit_get_TransitCode(CardBasebuf[0].cardNo, CardBasebuf[0].cardType, (uint8_t *)transit_param->CardBase_Qr_Data, (uint32_t *)&transit_param->CardBase_Qr_len, error_message, sizeof(error_message));
    callback(event, ret, 1, (alipay_tansit_CardBaseVO_t *)transit_param->CardBase_Qr_Data, transit_param->CardBase_Qr_len);
    return 0;
}


void transitcode_kill()
{
    printf("%s\n", __FUNCTION__);
    os_mutex_pend(&transit_param->mutex, 0);

    if (config_system_info) {
        task_info_del("transitcode");
    }

    while (OS_TASK_NOT_EXIST != os_task_del_req("transitcode")) {
        os_taskq_post("transitcode", 1, 0);
        os_time_dly(1);
    }

    if (transit_param->CardBasebuf) {
        free(transit_param->CardBasebuf);
        transit_param->CardBasebuf = NULL;
    }
    transit_param->CardBasenum = 0;
    transit_param->CardBaseindex = 0;
    if (transit_param->CardBase_Qr_Data) {
        free(transit_param->CardBase_Qr_Data);
        transit_param->CardBase_Qr_Data = NULL;
    }
    transit_param->CardBase_Qr_len = 0;
    transit_param->task_init  = 0;
    os_mutex_post(&transit_param->mutex);
}


void transitcode_create()
{
    int err;
    /* static u8 init = 0; */
    if (!transit_param->init) {
        os_mutex_create(&transit_param->mutex);
        transit_param->init = 1;
    }
    os_mutex_pend(&transit_param->mutex, 0);
    if (transit_param->task_init) {
        os_mutex_post(&transit_param->mutex);
        return;
    }
    os_sem_create(&transit_param->start_sem, 0);
    err = task_create(transitcode_task, NULL, "transitcode");
    os_sem_pend(&transit_param->start_sem, 0);
    os_sem_del(&transit_param->start_sem, OS_DEL_NO_PEND);
    transit_param->task_init = 1;
    os_mutex_post(&transit_param->mutex);
}


//获取
int local_transitcode_cardlist_get()
{
    int msg[3];
    transitcode_create();
    os_mutex_pend(&transit_param->mutex, 0);
    msg[0] = TRANSITCODE_LOCAL_GET_CARD_LIST;
    msg[1] = (int)transit_code_data_callback;
    os_taskq_del_type("transitcode", msg[0] | Q_EVENT);//删消息池命令
    int err = os_taskq_post_type("transitcode", msg[0] | Q_EVENT, 1, &msg[1]);
    os_mutex_post(&transit_param->mutex);
    return err;
}


//获取网上
int net_transitcode_cardlist_get()
{
    int msg[3];
    transitcode_create();
    os_mutex_pend(&transit_param->mutex, 0);
    msg[0] = TRANSITCODE_SERVER_GET_CARD_LIST;
    msg[1] = (int)transit_code_data_callback;
    os_taskq_del_type("transitcode", msg[0] | Q_EVENT);//删消息池命令
    int err = os_taskq_post_type("transitcode", msg[0] | Q_EVENT, 1, &msg[1]);
    os_mutex_post(&transit_param->mutex);
    return err;
}

//获取本地
int local_transitcode_get_default_card_info()
{
    int msg[3];
    transitcode_create();
    os_mutex_pend(&transit_param->mutex, 0);
    msg[0] = TRANSITCODE_GET_DEFAUL_CARD;
    msg[1] = (int)transit_code_data_callback;
    os_taskq_del_type("transitcode", msg[0] | Q_EVENT);//删消息池命令
    int err = os_taskq_post_type("transitcode", msg[0] | Q_EVENT, 1, &msg[1]);
    os_mutex_post(&transit_param->mutex);
    return err;
}


//设置默认卡片
int local_transitcode_set_default_card_info_by_index(int index)
{
    int msg[3];
    ASSERT(transit_param->CardBasebuf);
    os_mutex_pend(&transit_param->mutex, 0);
    memcpy(&transit_param->CardBaseVO, &transit_param->CardBasebuf[index], sizeof(transit_param->CardBaseVO));
    msg[0] = TRANSITCODE_SET_DEFAUL_CARD;
    msg[1] = (int)transit_code_data_callback;
    msg[2] = (int)&transit_param->CardBaseVO;
    transit_param->CardBaseindex = index;
    os_taskq_del_type("transitcode", msg[0] | Q_EVENT);//删消息池命令
    int err = os_taskq_post_type("transitcode", msg[0] | Q_EVENT, 2, &msg[1]);
    os_mutex_post(&transit_param->mutex);
    return err;
}



//获取qr
int local_transitcode_get_qr_data(int (*callback)(int, int, int, u8 *, int))
{
    int msg[3];
    transitcode_create();
    os_mutex_pend(&transit_param->mutex, 0);
    msg[0] = TRANSITCODE_GET_QRCODED;
    msg[1] = (int)callback;
    msg[2] = (int)&transit_param->CardBaseVO;
    os_taskq_del_type("transitcode", msg[0] | Q_EVENT);//删消息池命令
    int err = os_taskq_post_type("transitcode", msg[0] | Q_EVENT, 2, &msg[1]);
    os_mutex_post(&transit_param->mutex);
    printf("ret %d\n", err);
    return err;
}




int transit_code_data_callback(int event, int err, int card_num, alipay_tansit_CardBaseVO_t *card_list, int len_card_list)
{
    /*
     * @return  RV_OK 更新成功
     *          RV_JS_ERROR             CJSON库或接口错误
     *          RV_NETWORK_ERROR        网络错误
     *          RV_UNKNOWN              未知错误
     *          RV_SERVER_FAIL_ERROR    服务端返回错误
     *          RV_WRITE_ERROR          KV写错误，检查KV写接口的实现
     *          RV_BUF_TOO_SHORT        允许的最大长度小于实际获取的长度
     *          RV_IO_ERROR             读取IO错误
     **/


    if (err) {
        switch (err) {
        case RV_JS_ERROR:
            puts("RV_JS_ERROR\n");
            break;
        case RV_NETWORK_ERROR:
            puts("RV_NETWORK_ERROR\n");
            break;
        case RV_UNKNOWN:
            puts("RV_UNKNOWN\n");
            break;
        case RV_SERVER_FAIL_ERROR:
            puts("RV_SERVER_FAIL_ERROR\n");
            break;
        case RV_WRITE_ERROR:
            puts("RV_WRITE_ERROR\n");
            break;
        case RV_BUF_TOO_SHORT:
            puts("RV_BUF_TOO_SHORT\n");
            break;
        case RV_IO_ERROR:
            puts("RV_IO_ERROR\n");
            break;
        default:
            printf("RV ERROR %x\n", err);
            break;
        }

    }

    switch (event) {
    case TRANSITCODE_GET_DEFAUL_CARD:
        puts("TRANSITCODE_GET_DEFAUL_CARD \n");
        if (strlen(card_list[0].cardNo) == 0) {
            /* int net_get_dhcp_flag(); */
            /* if(net_get_dhcp_flag()){ */
            /*  */
            /* } */
            local_transitcode_cardlist_get();//获取本地缓存
        } else {
            printf("cardNo %s\n", card_list[0].cardNo);
            printf("cardType %s\n", card_list[0].cardType);
            printf("cardtitle %s\n", card_list[0].title);
            alipay_transit_card_status_t card_status_t = {0};
            alipay_transit_check_card_status(card_list[0].cardNo, card_list[0].cardType, &card_status_t);
            extern time_t time(time_t *timer);
            int32_t timestamp =  time(NULL);
            //最后10分钟也不给用
            if ((!card_status_t.is_exists) || !card_status_t.remain_use_count || (timestamp + 60 * 10 > card_status_t.expire_timestamp)) {
                //卡数据过期
                //1显示卡片列表
                printf("\n card_status_t,is_exists :%d, expire_timestamp :%d,remain_use_count :%d %d\n", card_status_t.is_exists, card_status_t.expire_timestamp, card_status_t.remain_use_count, timestamp);

                local_transitcode_cardlist_get();//获取本地缓存
            } else {
                //0直接显示二维码
                UI_MSG_POST("transit_show:event=%4", 0);
                /* ui_show_transitcodes_list(0); */
            }
        }
        break;
    case TRANSITCODE_SET_DEFAUL_CARD:
        puts("TRANSITCODE_SET_DEFAUL_CARD \n");
        break;
    case TRANSITCODE_SERVER_GET_CARD_LIST:
    case TRANSITCODE_LOCAL_GET_CARD_LIST:
        puts("TRANSITCODE_LOCAL_GET_CARD_LIST \n");
        //card_list 需要拷贝使用
        if (!transit_param) {    // 防止意外退出导致读到空指针
            return 0;
        }
        transit_param->CardBasenum = card_num;
        if (!err && card_num) {
            if (transit_param->CardBasebuf) {
                transit_param->CardBasebuf  = realloc(transit_param->CardBasebuf, len_card_list);
            } else {
                transit_param->CardBasebuf  = zalloc(len_card_list);
            }
            memcpy(transit_param->CardBasebuf, card_list, len_card_list);

            //0直接显示二维码
            //1显示卡片列表
            //2显示进度
            UI_MSG_POST("transit_show:event=%4", 1);
            /* ui_show_transitcodes_list(1); */
            for (int i = 0; i < card_num; i++) {
                printf("cardNo %s\n", transit_param->CardBasebuf[i].cardNo);
                printf("cardType %s\n", transit_param->CardBasebuf[i].cardType);
                printf("cardtitle %s\n", transit_param->CardBasebuf[i].title);
            }

        } else {
            if ((event == TRANSITCODE_LOCAL_GET_CARD_LIST) && net_get_dhcp_flag()) {
                //本地获取失败 带dhcp 情况
                UI_MSG_POST("transit_show:event=%4", 2);
                /* ui_show_transitcodes_list(2); */
                net_transitcode_cardlist_get();
            } else {
                if ((event == TRANSITCODE_SERVER_GET_CARD_LIST) && (!err)) {
                    //联网情况获取失败，显示没有开通
                    UI_MSG_POST("transit_show:event=%4", 6);
                    /* ui_show_transitcodes_list(6); */
                } else {
                    //这里直接显示 城市卡片获取失败
                    UI_MSG_POST("transit_show:event=%4", 4);
                    /* ui_show_transitcodes_list(4); */
                }
            }
        }
        break;
    case TRANSITCODE_GET_QRCODED:
        puts("TRANSITCODE_GET_QRCODED \n");
        break;
    default:
        break;
    }
    return 0;
}


void *alipay_tansit_buf_get()
{
    return (void *)transit_param->CardBasebuf;
}

int alipay_tansit_num_get()
{
    return transit_param->CardBasenum;
}


char *alipay_transit_name_get_by_index(int index)
{
    ASSERT(index < transit_param->CardBasenum);
    ASSERT(transit_param->CardBasebuf);
    return transit_param->CardBasebuf[index].title;
}


int alipay_transit_get_default_card_index()
{
    ASSERT(transit_param->CardBasebuf);
    return transit_param->CardBaseindex;
}

char *alipay_transit_get_global_default_card_name()
{
    return  transit_param->CardBaseVO.title;
}



#endif
#endif





