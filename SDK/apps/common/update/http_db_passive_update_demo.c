#include "system/includes.h"
#include "os/os_api.h"
#include "http/http_cli.h"
#include <stdlib.h>
#include "app_config.h"
#include "dual_bank_updata_api_v2.h"
#include "cpu.h"
#include "system/spinlock.h"
#include "system/timer.h"

#if TCFG_HTTP_UPDATE_ENABLE
#include "user_cfg_id.h"   /* VM_HTTP_OTA_INSTALL_FLAG: watch侧定义; 其它app此宏为0不展开 */
#include "syscfg_id.h"     /* syscfg_read/write: VM标记读写 */
#define DEFAULT_RECV_BUF_SIZE   (4 * 1024)   //数据缓存BUF大小
#define HTTP_RESUME_TIMEOUT_MS 60000         //连续60秒没有写入新数据则结束本次升级
#define HTTP_RESOURCE_WAIT_TICKS ((HTTP_RESUME_TIMEOUT_MS / 1000 + 10) * OS_TICKS_PER_SEC)
#define RESOURCE_DEFAULT_WAIT_TICKS (5 * OS_TICKS_PER_SEC) //底层默认500 tick

extern void user_file_flash_read_timeout_set(int read_timeout);

#define ONCE_REQ_LEN DEFAULT_RECV_BUF_SIZE

#define OTA_E_DOWNLOAD_FAILED  1
#define OTA_E_NOT_MATCH        2
#define OTA_E_STORAGE_FULL     3
#define OTA_E_LOW_BATTERY      4  //保留：当前未接入电量阈值检查
#define OTA_E_TIMEOUT          5
#define OTA_E_USER_ABORT       6
#define OTA_E_INTERNAL         7

static int http_ota_error_code(int ret, int fallback)
{
    int code = ret < 0 ? -ret : ret;
    switch (code) {
    case PUPDATE_RET_PACKAGE_PID_NOMATCH:
    case PUPDATE_RET_PACKAGE_VID_NOMATCH:
    case PUPDATE_RET_PACKAGE_CRC_NOMATCH:
    case PUPDATE_RET_PACKAGE_HEAD_ERROR:
    case PUPDATE_RET_PACKAGE_HEAD_SIZE_ERROR:
    case PUPDATE_RET_PACKAGE_HEAD_MAGIC_ERROR:
    case PUPDATE_RET_PACKAGE_HEAD_CRC_ERROR:
    case PUPDATE_RET_PACKAGE_HEAD_FIRST_NOT_DIR:
    case PUPDATE_RET_APP_FILE_CHECK_ERR:
        return OTA_E_NOT_MATCH;
    case PUPDATE_RET_FLASH_WRITE_OVERFLOW:
    case PUPDATE_RET_PL_OTA_OVERFLOW:
    case PUPDATE_RET_SM_OTA_OVERFLOW:
        return OTA_E_STORAGE_FULL;
    case PUPDATE_RET_TARGET_READ_TIMEOUT:
    case PUPDATE_RET_TARGET_WRITE_TIMEOUT:
    case PUPDATE_RET_EXIT_TIMEOUT_ERR:
        return OTA_E_TIMEOUT;
    case PUPDATE_RET_MALLOC_FAILED:
    case PUPDATE_RET_INVALID_PARM:
    case PUPDATE_RET_UNEXPECTED_ERROR:
    case PUPDATE_RET_NOT_INIT:
        return OTA_E_INTERNAL;
    default:
        return fallback;
    }
}

/* ===== 用于HTTP OTA 状态标记===== */
static volatile u8 g_http_ota_active = 0;   //标记已有升级任务，防止重复创建任务，并控制资源升级时写入VM恢复标志
static volatile u8 g_http_ota_cancel = 0;   //标记取消或退出，阻止继续下载及复用正在清理的任务
static volatile u8 g_http_ota_waiting = 0;  //标记重试等待阶段，同URL请求可提前唤醒原任务
static volatile u8 g_http_ota_resume_requested = 0; //记录手机续传请求，唤醒原任务，重复请求合并处理
static char *g_http_ota_url;                 //原始请求URL，由下载任务发布，release释放
static u8 g_http_ota_resource_wait_changed;  //记录资源写任务超时已延长，避免等待续传时提前退出，结束后恢复默认值
static DEFINE_SPINLOCK(g_http_ota_lock);

static void http_ota_restore_resource_timeout(void)
{
    if (g_http_ota_resource_wait_changed) {
        user_file_flash_read_timeout_set(RESOURCE_DEFAULT_WAIT_TICKS);
        g_http_ota_resource_wait_changed = 0;
    }
}

/* 显式取消接口；WiFi断开不调用，保留任务用于续传。 */
void http_cancel_passive_update_task(void)
{
    spin_lock(&g_http_ota_lock);
    if (g_http_ota_active) {
        g_http_ota_cancel = 1;
    }
    spin_unlock(&g_http_ota_lock);
}

static void http_ota_release(void)
{
    char *url;
    http_ota_restore_resource_timeout();  //写任务已退出，再恢复默认等待
    spin_lock(&g_http_ota_lock);
    url = g_http_ota_url;
    g_http_ota_url = NULL;
    g_http_ota_waiting = 0;
    g_http_ota_resume_requested = 0;
    g_http_ota_active = 0;
    spin_unlock(&g_http_ota_lock);
    free(url);
}

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
        /* 此钩子在底层重置默认超时后、创建资源写任务前调用。 */
        user_file_flash_read_timeout_set(HTTP_RESOURCE_WAIT_TICKS);
        g_http_ota_resource_wait_changed = 1;
        printf("[OTA] resource writer wait=%u s\n", HTTP_RESOURCE_WAIT_TICKS / OS_TICKS_PER_SEC);
        printf("[OTA] reserve(UI res) upgrade begin -> set installing flag\n");
        watch_http_ota_vm_set();   /* 置标记, 掉电后开机直入升级页 */
    }
}

struct download_hdl {
    char *url;                    //下载链接
    char *request_url;            //原始URL，重定向时保持不变
    u32 file_size;                //需要下载的文件大小
    u8 *recv_buf;                 //数据缓存BUF
    u32 recv_buf_size;            //数据缓存BUF大小
    u32 download_len;             //已下载的数据长度
    u32 reconnect_download_len;   //重连后已下载的数据长度
    u32 reconnecting;             //标记当前连接是否为重连
    u32 reconnect_cnt;            //重连次数
    u8 db_update_init;            //双备份被动升级是否已初始化
    int err_code;                //终止原因；断网等待续传不报异常终止
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
int cmd_put_ota_download_error(int download_len, int total, int err_code);

static int db_update_write_data(struct download_hdl *hdl, u8 *buf, u32 len)
{
    int ret;

    if (len == 0) {
        return 0;
    }

    if (hdl->db_update_init == 0) {
        if (hdl->file_size == 0) {
            printf("db update file_size err\n");
            hdl->err_code = OTA_E_NOT_MATCH;
            return -1;
        }

        /* 被动升级初始化，告知db_update_data.bin长度和后续单包大小，crc可为0 */
        ret = dual_bank_passive_update_init(0, hdl->file_size, ONCE_REQ_LEN, NULL);
        if (ret) {
            printf("db update init fail,%d\n", ret);
            hdl->err_code = http_ota_error_code(ret, OTA_E_INTERNAL);
            return -1;  //避免SDK正数错误码与本次写入长度相等
        }
        hdl->db_update_init = 1;
    }

    ret = dual_bank_update_write(buf, len, NULL);
    if (ret != 0) {
        printf("db update write fail,%d\n", ret);
        hdl->err_code = http_ota_error_code(ret, OTA_E_DOWNLOAD_FAILED);
        return -1;
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
    int http_open = 0;
    int retry_wait;
    u32 last_progress = sys_timer_get_ms();
    u32 previous_len;

    spin_lock(&g_http_ota_lock);
    g_http_ota_url = hdl->request_url;
    spin_unlock(&g_http_ota_lock);

_reconnect_:
    if (g_http_ota_cancel) {
        goto _out_;
    }
    if ((u32)(sys_timer_get_ms() - last_progress) >= HTTP_RESUME_TIMEOUT_MS) {
        goto _timeout_;
    }
    hdl->ctx.req_exit_flag = 0;
    hdl->ctx.exit_flag = 0;
    hdl->ctx.support_range = 0;
    //发起连接请求，建立socket连接
    http_open = 1;
    ret = hdl->download_ops->init(&hdl->ctx);
    if (g_http_ota_cancel) {
        goto _out_;
    }

    if (ret != HERROR_OK || hdl->ctx.req_exit_flag) {
        goto _retry_;
    }
    if (hdl->file_size == 0) {
        hdl->err_code = OTA_E_DOWNLOAD_FAILED;
        goto _out_;
    }

    printf("[OTA] connected, resume offset=%u\n", hdl->download_len);
    while (!g_http_ota_cancel && hdl->ctx.req_exit_flag == 0) {
        if ((u32)(sys_timer_get_ms() - last_progress) >= HTTP_RESUME_TIMEOUT_MS) {
            goto _timeout_;
        }
        previous_len = hdl->download_len;
        ret = hdl->download_ops->read(&hdl->ctx, (char *)hdl->recv_buf, hdl->recv_buf_size);//最大接收为recv_buf_size
        os_time_dly(1);
        printf("download len: %d\n", ret);
        if (g_http_ota_cancel) {
            goto _out_;
        }
        if ((u32)(sys_timer_get_ms() - last_progress) >= HTTP_RESUME_TIMEOUT_MS) {
            goto _timeout_;
        }
        if (hdl->ctx.req_exit_flag) {
            ret = -1;
        }

_resume_test_:
        //读取数据失败，记录断点
        if (ret <= 0) {
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

            goto _retry_;

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
                    if (hdl->download_len < hdl->file_size && (rand() % 10) < 1) {
                        last_progress = sys_timer_get_ms();  //测试跳转前记录本次新增数据
                        printf("random disconnect\n");
                        ret = -1;
                        goto _resume_test_;

                    }
#endif
                }
            }
        }


        if (hdl->download_len != previous_len) {
            last_progress = sys_timer_get_ms();
        }
        if (hdl->download_len >= hdl->file_size) {
            goto _finish_;
        }
    }
    if (g_http_ota_cancel) {
        goto _out_;
    }
    goto _retry_;  //连接退出也保留断点，交给原任务自动重连

_retry_:
    /* 先公布重试等待状态，手机请求只唤醒原任务，不重置断点。 */
    spin_lock(&g_http_ota_lock);
    g_http_ota_resume_requested = 0;
    g_http_ota_waiting = 1;
    spin_unlock(&g_http_ota_lock);
    if (http_open) {
        hdl->download_ops->close(&hdl->ctx);
        http_open = 0;
    }
    printf("[OTA] auto resume, offset=%u\n", hdl->download_len);
    for (retry_wait = 0; retry_wait < 10 && !g_http_ota_cancel && !g_http_ota_resume_requested; retry_wait++) {
        if ((u32)(sys_timer_get_ms() - last_progress) >= HTTP_RESUME_TIMEOUT_MS) {
            goto _timeout_;
        }
        os_time_dly(OS_TICKS_PER_SEC / 10);  //每100ms检查请求，1000ms后自动重试
    }
    if (g_http_ota_cancel) {
        goto _out_;
    }
    spin_lock(&g_http_ota_lock);
    g_http_ota_waiting = 0;
    g_http_ota_resume_requested = 0;
    spin_unlock(&g_http_ota_lock);
    hdl->ctx.lowRange = hdl->download_len;
    hdl->ctx.highRange = 0;
    hdl->reconnect_cnt++;
    hdl->reconnecting = 1;
    hdl->reconnect_download_len = 0;
    goto _reconnect_;  //连接失败仍回到重试流程，WiFi恢复后可自动续传

_finish_:
    if (g_http_ota_cancel || !hdl->db_update_init || hdl->download_len != hdl->file_size) {
        goto _out_;
    }
    printf("[OTA] download compelete, verifying... \n");


    /* 校验 */
    ret = dual_bank_update_verify_v2(NULL);  //保留完整返回码，避免旧接口u8截断
    if (ret != 0) {
        printf("verify fail,%d\n", ret);
        hdl->err_code = http_ota_error_code(ret, OTA_E_NOT_MATCH);
        goto _out_;
    }

    /* 整包(含预留区UI资源)校验通过: 清"安装中"标记后再烧boot_info复位 */
    watch_http_ota_vm_clear();

    /* 写入boot_info启动参数 */
    ret = dual_bank_update_burn_boot_info(NULL);
    if (ret != 0) {
        printf("set bootinfo,%d\n", ret);
        hdl->err_code = http_ota_error_code(ret, OTA_E_INTERNAL);
        goto _out_;
    }

    printf("[OTA] verify success \n");
    /* 通知手机app升级成功 */
    cmd_put_ota_download_progress(hdl->download_len, hdl->file_size, 1);

    dual_bank_passive_update_exit(NULL);
    http_ota_restore_resource_timeout();
    hdl->db_update_init = 0;
    hdl->download_ops->close(&hdl->ctx);

    spin_lock(&g_http_ota_lock);
    g_http_ota_url = NULL;
    spin_unlock(&g_http_ota_lock);
    free(hdl->request_url);
    free(hdl->url);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);

    //CPU发生复位，就会运行新的app
    cpu_reset();
    return;

_timeout_:
    hdl->err_code = OTA_E_TIMEOUT;
    printf("[OTA] resume timeout: no new data for 60s\n");
_out_:
    spin_lock(&g_http_ota_lock);
    if (g_http_ota_cancel) {
        hdl->err_code = OTA_E_USER_ABORT;
    }
    g_http_ota_cancel = 1;  //清理期间不再接受重复请求
    g_http_ota_waiting = 0;
    g_http_ota_resume_requested = 0;
    spin_unlock(&g_http_ota_lock);
    if (hdl->err_code == 0) {
        hdl->err_code = OTA_E_INTERNAL;
    }
    if (cmd_put_ota_download_error(hdl->download_len, hdl->file_size, hdl->err_code) < 0) {
        printf("[OTA] error report failed, err_code=%d\n", hdl->err_code);
    }
    printf("[OTA] stopped, downloaded=%u/%u\n", hdl->download_len, hdl->file_size);
    //关闭网络连接
    if (http_open) {
        hdl->download_ops->close(&hdl->ctx);
    }

    if (hdl->db_update_init) {
        void user_file_exit_write_task(void);
        user_file_exit_write_task();
        os_time_dly(OS_TICKS_PER_SEC / 2); //等待约500ms，让100ms检查定时器处理退出
        dual_bank_passive_update_exit(NULL);
    }

    free(hdl->url);
    free(hdl->recv_buf);
    memset(hdl, 0, sizeof(struct download_hdl));
    free(hdl);
    http_ota_release();  //清理完成后才允许再次升级
}

int http_create_passive_update_task(const char *url)
{
    struct download_parm parm = {0};
    struct download_hdl *hdl = NULL;
    int ret;

    if (!url || !url[0]) {
        return -EINVAL;
    }
    spin_lock(&g_http_ota_lock);
    if (g_http_ota_active) {
        ret = !g_http_ota_cancel && g_http_ota_url && !strcmp(g_http_ota_url, url) ? 0 : -EBUSY;
        if (ret == 0 && g_http_ota_waiting) {
            g_http_ota_resume_requested = 1;
        }
        spin_unlock(&g_http_ota_lock);
        printf("[OTA] %s\n", ret == 0 ? "reuse existing download" : "busy, reject request");
        return ret;
    }
    g_http_ota_active = 1;
    g_http_ota_cancel = 0;
    g_http_ota_waiting = 0;
    g_http_ota_resume_requested = 0;
    spin_unlock(&g_http_ota_lock);

    parm.url = url;  //改为app传过来的url

    //申请堆内存
    hdl = (struct download_hdl *)calloc(1, sizeof(struct download_hdl));

    if (hdl == NULL) {
        http_ota_release();
        return -ENOMEM;
    }

    hdl->download_ops = &http_ops;
    hdl->recv_buf_size = DEFAULT_RECV_BUF_SIZE;

    hdl->url = (char *)calloc(1, strlen(parm.url) + 1);

    if (hdl->url == NULL) {
        free(hdl);
        http_ota_release();
        return -ENOMEM;
    }

    hdl->recv_buf = (u8 *)calloc(1, hdl->recv_buf_size);

    if (hdl->recv_buf == NULL) {
        free(hdl->url);
        free(hdl);
        http_ota_release();
        return -ENOMEM;
    }

    strcpy(hdl->url, parm.url);
    hdl->request_url = calloc(1, strlen(parm.url) + 1);
    if (!hdl->request_url) {
        free(hdl->recv_buf);
        free(hdl->url);
        free(hdl);
        http_ota_release();
        return -ENOMEM;
    }
    strcpy(hdl->request_url, parm.url);

    //http请求参数赋值
    hdl->ctx.url = hdl->url;
    hdl->ctx.cb = http_user_cb;
    hdl->ctx.priv = hdl;
    hdl->ctx.connection = "close";
    hdl->ctx.timeout_millsec = 3000;

    ret = thread_fork("download_task", 20,  2 * 1024, 256, NULL, download_task, hdl);
    if (ret) {
        free(hdl->request_url);
        free(hdl->url);
        free(hdl->recv_buf);
        free(hdl);
        http_ota_release();
    }
    return ret;
}

#endif /* TCFG_HTTP_UPDATE_ENABLE */

