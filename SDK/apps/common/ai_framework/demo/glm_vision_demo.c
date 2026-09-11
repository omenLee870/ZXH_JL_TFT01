#include "system/includes.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE
#include "ai_provider.h"
#include "fs/fs.h"

#include <stdlib.h>
#include <string.h>

/*
 * GLM 应用层示例：只演示业务层如何直接调用 ai_framework。
 *
 * 这里不放 provider 适配逻辑。协议拼装、解析和底层收发都留给框架。
 * 业务层只负责：参数、回调、线程、结果打印。
 */

#ifndef GLM_VISION_API_KEY
#define GLM_VISION_API_KEY ""
#endif

#ifndef GLM_VISION_IMAGE_PATH
#define GLM_VISION_IMAGE_PATH "storage/sd0/C/1.jpg"
#endif

#ifndef GLM_VISION_PROMPT
#define GLM_VISION_PROMPT "请描述这张图片"
#endif

#ifndef GLM_TEXT_PROMPT
#define GLM_TEXT_PROMPT "请用一句话介绍你自己"
#endif

#ifndef GLM_VISION_STREAM_PROVIDER_PARAMS
#define GLM_VISION_STREAM_PROVIDER_PARAMS "{\"stream\":true,\"request_id\":\"glm-vision-stream-0001\"}"
#endif

#ifndef GLM_TEXT_PROVIDER_PARAMS
#define GLM_TEXT_PROVIDER_PARAMS "{\"request_id\":\"glm-text-0001\"}"
#endif

#ifndef GLM_TEXT_STREAM_PROVIDER_PARAMS
#define GLM_TEXT_STREAM_PROVIDER_PARAMS "{\"stream\":true,\"request_id\":\"glm-text-stream-0001\"}"
#endif

static volatile int glm_vision_demo_running;
static volatile int glm_vision_demo_started;

static void glm_vision_demo_stream_cb(const struct ai_dialogue_event *event, void *user)
{
    const char *tag = (const char *)user;

    if (!event || !tag) {
        return;
    }

    if (event->type == AI_DIALOGUE_TEXT_DELTA && event->text && event->text[0]) {
        printf("[%s] delta: %s\n", tag, event->text);
    } else if (event->type == AI_DIALOGUE_TEXT_DONE) {
        printf("[%s] stream done\n", tag);
    }
}

static unsigned char *glm_vision_demo_read_file(const char *path, unsigned int *out_len)
{
    FILE *fd;
    unsigned char *buf;
    int len;
    int rlen;

    if (!path || !out_len) {
        return NULL;
    }

    *out_len = 0;
    fd = fopen(path, "r");
    if (!fd) {
        printf("[glm] image open failed: %s\n", path);
        return NULL;
    }

    len = flen(fd);
    if (len <= 0) {
        printf("[glm] invalid image len: %d\n", len);
        fclose(fd);
        return NULL;
    }

    buf = (unsigned char *)malloc(len);
    if (!buf) {
        printf("[glm] image malloc failed: %d\n", len);
        fclose(fd);
        return NULL;
    }

    rlen = fread(buf, 1, len, fd);
    fclose(fd);
    if (rlen != len) {
        printf("[glm] image read failed: %d/%d\n", rlen, len);
        free(buf);
        return NULL;
    }

    *out_len = (unsigned int)len;
    return buf;
}

struct glm_vision_demo_cfg {
    enum ai_http_backend http_backend;
    const char *provider_params;
    const char *tag;
    const char *prompt;
    int use_image;
};

static const struct glm_vision_demo_cfg glm_vision_demo_cfg_curl = {
    .http_backend = AI_HTTP_BACKEND_CURL,
    .provider_params = NULL,
    .tag = "glm-curl",
    .prompt = GLM_VISION_PROMPT,
    .use_image = 1,
};

static const struct glm_vision_demo_cfg glm_vision_demo_cfg_lws_stream = {
    .http_backend = AI_HTTP_BACKEND_LWS,
    .provider_params = GLM_VISION_STREAM_PROVIDER_PARAMS,
    .tag = "glm-lws-stream",
    .prompt = GLM_VISION_PROMPT,
    .use_image = 1,
};

static const struct glm_vision_demo_cfg glm_text_demo_cfg_curl = {
    .http_backend = AI_HTTP_BACKEND_CURL,
    .provider_params = GLM_TEXT_PROVIDER_PARAMS,
    .tag = "glm-text-curl",
    .prompt = GLM_TEXT_PROMPT,
    .use_image = 0,
};

static const struct glm_vision_demo_cfg glm_text_demo_cfg_lws_stream = {
    .http_backend = AI_HTTP_BACKEND_LWS,
    .provider_params = GLM_TEXT_STREAM_PROVIDER_PARAMS,
    .tag = "glm-text-stream",
    .prompt = GLM_TEXT_PROMPT,
    .use_image = 0,
};

static void glm_vision_demo_task(void *priv)
{
    const struct glm_vision_demo_cfg *cfg = (const struct glm_vision_demo_cfg *)priv;
    struct ai_image_input image;
    struct ai_config ai_cfg;
    struct ai_client *client = NULL;
    struct ai_vision_result result = {0};
    unsigned char *image_data = NULL;
    unsigned int image_len = 0;
    const char *api_key = GLM_VISION_API_KEY;
    int ret;

    /* mdelay(10 * 1000); */

    if (!cfg) {
        cfg = &glm_vision_demo_cfg_curl;
    }

    if (!api_key || !api_key[0]) {
        printf("[glm] missing GLM_VISION_API_KEY\n");
        goto out;
    }

    memset(&image, 0, sizeof(image));
    if (cfg->use_image) {
        image_data = glm_vision_demo_read_file(GLM_VISION_IMAGE_PATH, &image_len);
        if (!image_data) {
            goto out;
        }
        image.format = AI_IMAGE_JPEG;
        image.data = image_data;
        image.size = image_len;
    }

    memset(&ai_cfg, 0, sizeof(ai_cfg));
    ai_cfg.provider_name = "glm";
    ai_cfg.api_key = api_key;
    ai_cfg.provider_params = cfg->provider_params;
    ai_cfg.timeout_ms = 30000;
    ai_cfg.http_backend = cfg->http_backend;

    ret = ai_client_create(&ai_cfg, &client);
    if (ret != AI_OK) {
        printf("[%s] ai_client_create failed: %d\n", cfg->tag, ret);
        goto out;
    }
    if (cfg->http_backend == AI_HTTP_BACKEND_LWS) {
        ai_client_set_dialogue_callback(client, glm_vision_demo_stream_cb, (void *)cfg->tag);
    }

    ret = ai_vision_recognize(client, cfg->use_image ? &image : NULL, cfg->prompt, &result);
    if (ret == AI_OK) {
        printf("[%s] result: %s\n", cfg->tag, result.text ? result.text : "");
    } else {
        printf("[%s] request failed: %d\n", cfg->tag, ret);
        if (result.text) {
            printf("[%s] error: %s\n", cfg->tag, result.text);
        }
        if (result.raw_json) {
            printf("[%s] raw: %s\n", cfg->tag, result.raw_json);
        }
    }

out:
    ai_vision_result_free(&result);
    ai_client_destroy(client);
    free(image_data);
    glm_vision_demo_running = 0;
    glm_vision_demo_started = 0;
}

static void glm_vision_demo_start_with_cfg(const struct glm_vision_demo_cfg *cfg)
{
    int ret;

    if (glm_vision_demo_started) {
        printf("[glm] vision demo already started\n");
        return;
    }
    if (glm_vision_demo_running) {
        printf("[glm] vision demo already running\n");
        return;
    }

    glm_vision_demo_started = 1;
    glm_vision_demo_running = 1;
    ret = thread_fork("glm_vision_demo", 20, 4 * 1024, 0, 0, glm_vision_demo_task, (void *)cfg);
    if (ret != OS_NO_ERR) {
        glm_vision_demo_started = 0;
        glm_vision_demo_running = 0;
        printf("[glm] thread_fork failed: %d\n", ret);
    }
}

void glm_vision_demo_start(void)
{
    glm_vision_demo_start_with_cfg(&glm_vision_demo_cfg_curl);
}

void glm_vision_demo_stream_start(void)
{
    glm_vision_demo_start_with_cfg(&glm_vision_demo_cfg_lws_stream);
}

void glm_text_demo_start(void)
{
    glm_vision_demo_start_with_cfg(&glm_text_demo_cfg_curl);
}

void glm_text_demo_stream_start(void)
{
    glm_vision_demo_start_with_cfg(&glm_text_demo_cfg_lws_stream);
}

#endif
