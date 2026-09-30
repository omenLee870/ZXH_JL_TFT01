#include "app_config.h"
#include "ui/ui_api.h"
#include "jlui/ui.h"
#include "ui_style.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "init.h"
#include "key_event_deal.h"
#include "device/device.h"
#include "app_power_manage.h"
#include "btstack/avctp_user.h"
#include "asm/charge.h"
#include "cat1/cat1_common.h"
#include "cJSON.h"
#include "media/includes.h"
#include "audio_config.h"
#include "res/resfile.h"
#include "res_config.h"
#include "circular_buf.h"
#include "ntp/ntp.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "authentication.h"
#include "rtc.h"
#include "ifly_common.h"

#if TCFG_IFLYTEK_ENABLE

#define LOG_TAG_CONST       NET_IFLY
#define LOG_TAG             "[IFLY_AUTH]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#ifdef TCFG_IFLYTEK_APP_SECRET
#define API_SECRET   			TCFG_IFLYTEK_APP_SECRET
#else
#define API_SECRET   			"123"
#endif

#ifdef TCFG_IFLYTEK_APP_KEY
#define API_KEY      			TCFG_IFLYTEK_APP_KEY
#else
#define API_KEY      			"123"
#endif


// 鉴权url的buf长度。拼接后的url约300bytes，留出余量
#define IFLY_AUTH_URL_BUF_LEN   400

// 鉴权信息结构体
struct auth_info_t {
    char sig_ori[100];
    char sig_hmac[100];
    char sig_hmac_base64[50];
    char author_ori[200];
    char author[300];
    char date_str[50];
    struct sys_time curtime;
};

extern int ntp_client_get_time(const char *host);

// 拼接RFC1123格式的日期，用于鉴权签名，例如 "Wed, 10 Jul 2024 03:52:31 GMT"
static void ifly_format_http_date(char *buf, int len, struct sys_time *t, int wday)
{
    static const char *week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                               };
    // get_weekday_from_time返回值：1=周一 ... 7=周日，映射为0=周日 ... 6=周六
    int w = wday % 7;
    sprintf(buf, "%s, %02d %s %04d %02d:%02d:%02d GMT",
            week[w], t->day, mon[t->month - 1], t->year, t->hour, t->min, t->sec);
}

/* void *_calloc_r(struct _reent *r, size_t a, size_t b) */
/* { */
/*     return calloc(a, b); */
/* } */

static char dec2hex(short int c)
{
    if (0 <= c && c <= 9) {
        return c + '0';
    } else if (10 <= c && c <= 15) {
        return c + 'A' - 10;
    } else {
        return -1;
    }
}

static void urlencode(char url[])
{
    int i = 0;
    int len = strlen(url);
    int res_len = 0;
    char res[100];
    for (i = 0; i < len; ++i) {
        char c = url[i];
        if (('0' <= c && c <= '9') ||
            ('a' <= c && c <= 'z') ||
            ('A' <= c && c <= 'Z') ||
            c == '/' || c == '.') {
            res[res_len++] = c;
        } else {
            int j = (short int)c;
            if (j < 0) {
                j += 256;
            }
            int i1, i0;
            i1 = j / 16;
            i0 = j - i1 * 16;
            res[res_len++] = '%';
            res[res_len++] = dec2hex(i1);
            res[res_len++] = dec2hex(i0);
        }
    }
    res[res_len] = '\0';
    strcpy(url, res);
}

static int ifly_get_sys_time(struct sys_time *time)//获取时间
{
    rtc_read_time(time);
    int week = get_weekday_from_time(time);

    return week;
    /* void *fd = dev_open("rtc", NULL); */
    /* if (!fd) { */
    /*     return ; */
    /* } */
    /* dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)time); */
    /* dev_close(fd); */
}

char *ifly_authentication(char *host_name, char *host, char *path, int retry)
{
    //初始化
    struct auth_info_t *auth = net_iflytek_malloc(sizeof(struct auth_info_t));
    char *url_xf = net_iflytek_malloc(IFLY_AUTH_URL_BUF_LEN);
    int retry_cnt = 0;

    if ((auth == NULL) || (url_xf == NULL)) {
        log_error("malloc err!!\n");
        net_iflytek_free(auth);
        net_iflytek_free(url_xf);
        return NULL;
    }

__retry:
    //获取网络时间
    /* ntp_client_get_time("s2c.time.edu.cn"); */
    int ntp = ntp_client_get_time("s2c.time.edu.cn");
    if (ntp == -1) {
        log_error("get ntp error!!\n");
        net_iflytek_free(auth);
        net_iflytek_free(url_xf);
        return NULL;
    }
    int week = ifly_get_sys_time(&(auth->curtime));
    ifly_format_http_date(auth->date_str, sizeof(auth->date_str), &(auth->curtime), week);
    log_info("%s\n", auth->date_str);

    //拼接sig原始字符
    sprintf(auth->sig_ori, "%s%s\ndate: %s\n%s", "host: ", host, auth->date_str, path);
    //通过hmac_sha256加密
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info;
    mbedtls_md_init(&ctx);
    info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    mbedtls_md_setup(&ctx, info, 1);
    mbedtls_md_hmac_starts(&ctx, (unsigned char *)API_SECRET, strlen(API_SECRET));
    mbedtls_md_hmac_update(&ctx, (unsigned char *)auth->sig_ori, strlen(auth->sig_ori));
    mbedtls_md_hmac_finish(&ctx, (unsigned char *)auth->sig_hmac);
    mbedtls_md_free(&ctx);
    //进行base64编码
    int res;
    mbedtls_base64_encode((unsigned char *)auth->sig_hmac_base64, 50, (size_t *)&res, (unsigned char *)auth->sig_hmac, mbedtls_md_get_size(info));
    if (strlen(auth->sig_hmac_base64) != 44) {    // 根据科大讯飞文档，hamc base64编码后，正常为44bytes
        log_info("base64 encode error!!\n");
        if (retry_cnt < retry) {
            memset(auth, 0, sizeof(struct auth_info_t));
            memset((char *)url_xf, 0, IFLY_AUTH_URL_BUF_LEN);
            retry_cnt++;
            goto __retry;
        }
    }
    //拼接authorization_ori字符串
    strcat(auth->author_ori, "api_key=\"");
    strcat(auth->author_ori, API_KEY);
    strcat(auth->author_ori, "\",");
    strcat(auth->author_ori, "algorithm=\"hmac-sha256\",");
    strcat(auth->author_ori, "headers=\"host date request-line\",");
    strcat(auth->author_ori, "signature=\"");
    strcat(auth->author_ori, auth->sig_hmac_base64);
    strcat(auth->author_ori, "\"");
    //对autho_ori进行base64编码
    int author_res;
    mbedtls_base64_encode((unsigned char *)auth->author, 300, (size_t *)&author_res, (unsigned char *)auth->author_ori, strlen(auth->author_ori));
    //拼接url
    strcat(url_xf, host_name);
    strcat(url_xf, "?");
    urlencode(auth->date_str);
    strcat(url_xf, "authorization=");
    strcat(url_xf, auth->author);
    strcat(url_xf, "&date=");
    strcat(url_xf, auth->date_str);
    strcat(url_xf, "&host=");
    strcat(url_xf, host);
    log_info("url_xf:%s\n", url_xf);

    net_iflytek_free(auth);

    return url_xf;
}

#endif

