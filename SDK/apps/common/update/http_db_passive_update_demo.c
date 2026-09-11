#include "system/includes.h"
#include "os/os_api.h"
#include "http/http_cli.h"
#include <stdlib.h>
#include "app_config.h"
#include "dual_bank_updata_api_v2.h"
#include "cpu.h"

#if TCFG_HTTP_UPDATE_ENABLE
#include "user_cfg_id.h"   /* VM_HTTP_OTA_INSTALL_FLAG: watch侧定义; 其它app此宏为0不展开 */
#include "syscfg_id.h"     /* syscfg_read/write: VM标记读写 */
#define DEFAULT_RECV_BUF_SIZE   (4 * 1024)   //数据缓存BUF大小
#define MAX_RECONNECT_CNT 10086              //最大重连次数

#define ONCE_REQ_LEN DEFAULT_RECV_BUF_SIZE

/* ===== 用于HTTP OTA 状态标记===== */
static u8 g_http_ota_active = 0;

void watch_http_ota_vm_set(void)
{
    u32 m = 0x4F544157u;                     /* 'OTAW' */
    syscfg_write(VM_HTTP_OTA_INSTALL_FLAG, &m, sizeof(m));
}
int watch_http_ota_vm_get(void)
{
    u32 m = 0;
    return (syscfg_read(VM_HTTP_OTA_INSTALL_FLAG, &m, sizeof(m)) == sizeof(m)) && (m == 0x4F544157u);
}
void watch_http_ota_vm_clear(void)
{
    u32 m = 0;
    syscfg_write(VM_HTTP_OTA_INSTALL_FLAG, &m, sizeof(m));
}

__attribute__((weak)) void ui_update_page_enter(void) {}

/* 预留区升级开始时调用，UI资源单备份升级失败时重启后不加载UI */
void app_user_file_http_update_begin(void)
{
    if (g_http_ota_active) {
        printf("[OTA] reserve(UI res) upgrade begin -> set installing flag\n");
        watch_http_ota_vm_set();   /* 置标记, 掉电后开机直入升级页 */
    }
}

struct download_hdl {
    char *url;                    //下载链接
    u32 file_size;                //需要下载的文件大小
    u8 *recv_buf;                 //数据缓存BUF
    u32 recv_buf_size;            //数据缓存BUF大小
    u32 download_len;             //已下载的数据长度
    u32 reconnect_download_len;   //重连后已下载的数据长度
    u32 reconnecting;             //标记当前连接是否为重连
    u32 reconnect_cnt;            //重连次数
    u8 db_update_init;            //双备份被动升级是否已初始化
    httpcli_ctx ctx;              //http请求参数
    const struct net_download_ops *download_ops; //http操作集
};

struct download_parm {
    const char *url;  //下载链接
};

//http回调函数
static int http_user_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    struct download_hdl *hdl = (struct download_hdl *)priv;

    if (status == HTTPIN_HEADER) {
        //第一次连接时，记录文件总长度
        if (hdl->ctx.lowRange == 0) {
            hdl->file_size = hdl->ctx.content_length;
        }

        log_e("http_need_download_file=%d KB", hdl->file_size / 1024);
    }
    return 0;
}

//通知app升级进度
int cmd_put_ota_download_progress(int download_len, int total, int status);

static int db_update_write_data(struct download_hdl *hdl, u8 *buf, u32 len)
{
    int ret;

    if (len == 0) {
        return 0;
    }

    if (hdl->db_update_init == 0) {
        if (hdl->file_size == 0) {
            printf("db update file_size err\n");
            return -1;
        }

        /* 被动升级初始化，告知db_update_data.bin长度和后续单包大小，crc可为0 */
        ret = dual_bank_passive_update_init(0, hdl->file_size, ONCE_REQ_LEN, NULL);
        if (ret) {
            printf("db update init fail,%d\n", ret);
            return ret;
        }
        hdl->db_update_init = 1;
    }

    ret = dual_bank_update_write(buf, len, NULL);
    if (ret < 0) {
        printf("db update write fail,%d\n", ret);
        return ret;
    }

    return len;
}

static void download_task(void *priv)
{
    struct download_hdl *hdl = priv;
    int ret = 0;
    int err = 0;
    int downloaded = 0;
    int new = 0;
    int offset = 0;

_reconnect_:
    //发起连接请求，建立socket连接
    ret = hdl->download_ops->init(&hdl->ctx);

    if (ret != HERROR_OK) {
        if (hdl->ctx.req_exit_flag == 0) {
            os_time_dly(50);
            goto _reconnect_;
        }
    }

    printf("downloaded >>> 00%%\n");
    while (hdl->ctx.req_exit_flag == 0) {
        ret = hdl->download_ops->read(&hdl->ctx, (char *)hdl->recv_buf, hdl->recv_buf_size);//最大接收为recv_buf_size
        os_time_dly(1);
        printf("download len: %d\n", ret);

_resume_test_:
        //读取数据失败，记录断点
        if (ret < 0) {
            printf("recv fail, record and resume ------\n");

            //1. 保存重定向的url
            if (hdl->ctx.redirection_url) {
                free(hdl->url);
                hdl->url = NULL;
                hdl->url = calloc(1, strlen(hdl->ctx.redirection_url) + 1);
                if (hdl->url == NULL) {
                    printf("reconnect calloc url fail.\n");
                    goto _out_;
                }
                strcpy(hdl->url, hdl->ctx.redirection_url);
                hdl->ctx.url = hdl->url;
            }

            //2. 关闭网络连接
            hdl->download_ops->close(&hdl->ctx);
            //记录断点位置，重新发起下载请求
            if (hdl->reconnect_cnt < MAX_RECONNECT_CNT) {
                hdl->ctx.lowRange = hdl->download_len;
                hdl->ctx.highRange = 0;
                hdl->reconnect_cnt++;
                hdl->reconnecting = 1;
                hdl->reconnect_download_len = 0;  //用于不支持range的场景
                goto _reconnect_;
            } else {
                printf("download reconnect upto max count.\n");
                goto _out_;
            }

        } else {  //数据正常下载
            //服务器不支持带range字段的http请求,即服务器从头开始发数据。
            if (hdl->reconnecting && (hdl->ctx.support_range == 0)) {
                printf("reloading... %d / %d \n", hdl->reconnect_download_len, hdl->download_len);
                if (hdl->reconnect_download_len + ret > hdl->download_len) {
                    //追回到之前已经写入的位置再开始写
                    offset = hdl->download_len - hdl->reconnect_download_len;
                    err = db_update_write_data(hdl, hdl->recv_buf + offset, ret - offset);
                    if (err != ret - offset) {
                        printf("write offset len err----\n");
                        goto _out_;
                    }

                    hdl->download_len += ret - offset;
                    hdl->reconnecting = 0;  //完成恢复
                } else {
                    //先持续读已经写过的部分，等待回到上一次下载的位置
                    hdl->reconnect_download_len += ret;
                }
            } else {
                hdl->reconnecting = 0;

                //写入双备份升级区
                err = db_update_write_data(hdl, hdl->recv_buf, ret);
                if (err != ret) {
                    printf("write len ----\n");
                    goto _out_;
                }

                //downloaded new用于打印
                downloaded = (hdl->download_len * 100) / hdl->file_size;
                hdl->download_len += ret;
                new = (hdl->download_len * 100) / hdl->file_size;
                if (downloaded != new) {
                    printf("downloaded >>> %02d %%\n", new);
                    cmd_put_ota_download_progress(hdl->download_len, hdl->file_size, 0); //通知app升级进度
                    void ui_update_page_set_percent(int pct);
                    ui_update_page_set_percent(new);  //UI进度更新(UI已启动, 升级页可正常刷新)

#if 0
                    //测试断点续传
                    if ((rand() % 10) < 1) {
                        printf("random disconnect\n");
                        ret = -1;
                        goto _resume_test_;

                    }
#endif
                }
            }
        }


        if (hdl->download_len >= hdl->file_size) {
            goto _finish_;
        }
    }

_finish_:
    printf("download success.\n");
    cmd_put_ota_download_progress(hdl->download_len, hdl->file_size, 1);

    /* 校验 */
    ret = dual_bank_update_verify_without_crc();
    if (ret < 0) {
        printf("verify fail,%d\n", ret);
        goto _out_;
    }

    /* 整包(含预留区UI资源)校验通过: 清"安装中"标记后再烧boot_info复位 */
    watch_http_ota_vm_clear();
    g_http_ota_active = 0;

    /* 写入boot_info启动参数 */
    ret = dual_bank_update_burn_boot_info(NULL);
    if (ret < 0) {
        printf("set bootinfo,%d\n", ret);
        goto _out_;
    }

    dual_bank_passive_update_exit(NULL);
    hdl->db_update_init = 0;
    hdl->download_ops->close(&hdl->ctx);

    free(hdl->url);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);

    //CPU发生复位，就会运行新的app
    cpu_reset();
    return;

_out_:
    //关闭网络连接
    hdl->download_ops->close(&hdl->ctx);

    if (hdl->db_update_init) {
        dual_bank_passive_update_exit(NULL);
    }

    g_http_ota_active = 0;

    free(hdl->url);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);
}

int http_create_passive_update_task(const char *url)
{
    struct download_parm parm = {0};
    struct download_hdl *hdl = NULL;

    parm.url = url;  //改为app传过来的url

    //申请堆内存
    hdl = (struct download_hdl *)calloc(1, sizeof(struct download_hdl));

    if (hdl == NULL) {
        return -ENOMEM;
    }

    hdl->download_ops = &http_ops;
    hdl->recv_buf_size = DEFAULT_RECV_BUF_SIZE;

    hdl->url = (char *)calloc(1, strlen(parm.url) + 1);

    if (hdl->url == NULL) {
        free(hdl);
        return -ENOMEM;
    }

    hdl->recv_buf = (u8 *)calloc(1, hdl->recv_buf_size);

    if (hdl->recv_buf == NULL) {
        free(hdl->url);
        free(hdl);
        return -ENOMEM;
    }

    strcpy(hdl->url, parm.url);

    //http请求参数赋值
    hdl->ctx.url = hdl->url;
    hdl->ctx.cb = http_user_cb;
    hdl->ctx.priv = hdl;
    hdl->ctx.connection = "close";

    g_http_ota_active = 1;

    //创建下载线程
    return thread_fork("download_task", 20,  2 * 1024, 0, NULL, download_task, hdl);
}

#endif /* TCFG_HTTP_UPDATE_ENABLE */
