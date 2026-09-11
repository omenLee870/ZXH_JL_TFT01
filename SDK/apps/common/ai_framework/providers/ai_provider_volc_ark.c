#include "ai_internal.h"
#include "app_config.h"

#if TCFG_NETWORK_ENABLE

#include "cJSON.h"

#include <stdlib.h>
#include <string.h>

#define VOLC_ARK_DEFAULT_MODEL "doubao-1-5-vision-pro-32k-250115"
#define VOLC_ARK_VISION_HTTP_URL "https://ark.cn-beijing.volces.com/api/v3/chat/completions"
#define VOLC_ARK_JPEG_DATA_URL_PREFIX "data:image/jpeg;base64,"

static int volc_ark_json_print(cJSON *root, char **out_json)
{
    char *printed;

    if (!root || !out_json) {
        return AI_ERR_PARAM;
    }

    *out_json = NULL;
    printed = cJSON_PrintUnformatted(root);
    if (!printed) {
        return AI_ERR_NOMEM;
    }

    *out_json = ai_strdup(printed);
    cJSON_free(printed);

    return *out_json ? AI_OK : AI_ERR_NOMEM;
}

static const char *volc_ark_model_name(struct ai_client *client)
{
    if (client && client->model && client->model[0]) {
        return client->model;
    }

    return VOLC_ARK_DEFAULT_MODEL;
}

static cJSON *volc_ark_parse_provider_params(struct ai_client *client)
{
    if (!client || !client->provider_params || !client->provider_params[0]) {
        return NULL;
    }

    return cJSON_Parse(client->provider_params);
}

static cJSON *volc_ark_get_param(cJSON *params, const char *name)
{
    if (!params || !name) {
        return NULL;
    }

    return cJSON_GetObjectItem(params, name);
}

static const char *volc_ark_get_string(cJSON *root, const char *name)
{
    cJSON *item;

    if (!root) {
        return NULL;
    }

    item = cJSON_GetObjectItem(root, name);
    if (cJSON_IsString(item)) {
        return item->valuestring;
    }

    return NULL;
}

static int volc_ark_get_bool_param(cJSON *params, const char *name, int default_value)
{
    cJSON *item = volc_ark_get_param(params, name);

    if (cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }

    return default_value;
}

static int volc_ark_add_optional_item(cJSON *root, cJSON *params, const char *name)
{
    cJSON *item;
    cJSON *dup;

    if (!root || !name) {
        return AI_ERR_PARAM;
    }

    item = volc_ark_get_param(params, name);
    if (!item) {
        return AI_OK;
    }

    dup = cJSON_Duplicate(item, 1);
    if (!dup) {
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddItemToObject(root, name, dup)) {
        cJSON_Delete(dup);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int volc_ark_make_jpeg_data_url(const struct ai_image_input *image, char **out_data_url)
{
    char *base64 = NULL;
    char *data_url;
    size_t prefix_len;
    size_t base64_len;
    int ret;

    if (!image || !image->data || !image->size || !out_data_url) {
        return AI_ERR_PARAM;
    }

    *out_data_url = NULL;
    if (image->format != AI_IMAGE_JPEG) {
        return AI_ERR_UNSUPPORTED;
    }

    ret = ai_base64_encode(image->data, image->size, &base64);
    if (ret != AI_OK) {
        return ret;
    }

    prefix_len = strlen(VOLC_ARK_JPEG_DATA_URL_PREFIX);
    base64_len = strlen(base64);
    data_url = (char *)ai_malloc_large(prefix_len + base64_len + 1);
    if (!data_url) {
        ai_free_large(base64);
        return AI_ERR_NOMEM;
    }

    memcpy(data_url, VOLC_ARK_JPEG_DATA_URL_PREFIX, prefix_len);
    memcpy(data_url + prefix_len, base64, base64_len + 1);
    ai_free_large(base64);

    *out_data_url = data_url;
    return AI_OK;
}

static int volc_ark_add_image_content(cJSON *content, const char *image_url)
{
    cJSON *item;
    cJSON *url;
    cJSON *url_value;

    if (!content || !image_url || !image_url[0]) {
        return AI_ERR_PARAM;
    }

    item = cJSON_CreateObject();
    if (!item) {
        return AI_ERR_NOMEM;
    }

    url = cJSON_CreateObject();
    if (!url) {
        cJSON_Delete(item);
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddStringToObject(item, "type", "image_url")) {
        cJSON_Delete(item);
        cJSON_Delete(url);
        return AI_ERR_NOMEM;
    }

    /* The caller keeps this string alive until JSON serialization completes. */
    url_value = cJSON_CreateStringReference(image_url);
    if (!url_value || !cJSON_AddItemToObject(url, "url", url_value)) {
        cJSON_Delete(url_value);
        cJSON_Delete(item);
        cJSON_Delete(url);
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddItemToObject(item, "image_url", url)) {
        cJSON_Delete(item);
        cJSON_Delete(url);
        return AI_ERR_NOMEM;
    }
    url = NULL;

    if (!cJSON_AddItemToArray(content, item)) {
        cJSON_Delete(item);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int volc_ark_add_text_content(cJSON *content, const char *prompt)
{
    cJSON *item;

    if (!content || !prompt || !prompt[0]) {
        return AI_ERR_PARAM;
    }

    item = cJSON_CreateObject();
    if (!item) {
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddStringToObject(item, "type", "text") ||
        !cJSON_AddStringToObject(item, "text", prompt)) {
        cJSON_Delete(item);
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddItemToArray(content, item)) {
        cJSON_Delete(item);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int volc_ark_add_message_text(cJSON *messages, const char *role, const char *text)
{
    cJSON *message;

    if (!messages || !role || !text || !text[0]) {
        return AI_ERR_PARAM;
    }

    message = cJSON_CreateObject();
    if (!message) {
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddStringToObject(message, "role", role) ||
        !cJSON_AddStringToObject(message, "content", text)) {
        cJSON_Delete(message);
        return AI_ERR_NOMEM;
    }

    if (!cJSON_AddItemToArray(messages, message)) {
        cJSON_Delete(message);
        return AI_ERR_NOMEM;
    }

    return AI_OK;
}

static int volc_ark_build_vision_http_body(struct ai_client *client,
        const struct ai_image_input *image,
        const char *prompt,
        char **out_body)
{
    cJSON *root = NULL;
    cJSON *messages = NULL;
    cJSON *message = NULL;
    cJSON *content = NULL;
    cJSON *params = NULL;
    char *owned_data_url = NULL;
    const char *resolved_image_url = NULL;
    const char *system_prompt;
    int stream;
    int ret;

    if (!client || !prompt || !prompt[0] || !out_body) {
        return AI_ERR_PARAM;
    }

    *out_body = NULL;
    if (client->provider_params && client->provider_params[0]) {
        params = volc_ark_parse_provider_params(client);
        if (!params) {
            return AI_ERR_PARSE;
        }
    }

    stream = volc_ark_get_bool_param(params, "stream", 0);
    if (stream && client->http_backend != AI_HTTP_BACKEND_LWS) {
        cJSON_Delete(params);
        return AI_ERR_UNSUPPORTED;
    }

    if (image && image->data && image->size) {
        ret = volc_ark_make_jpeg_data_url(image, &owned_data_url);
        if (ret != AI_OK) {
            cJSON_Delete(params);
            return ret;
        }
        resolved_image_url = owned_data_url;
    } else {
        resolved_image_url = volc_ark_get_string(params, "image_url");
    }

    root = cJSON_CreateObject();
    messages = cJSON_CreateArray();
    message = cJSON_CreateObject();
    content = cJSON_CreateArray();
    if (!root || !messages || !message || !content) {
        ret = AI_ERR_NOMEM;
        goto out;
    }

    if (!cJSON_AddStringToObject(root, "model", volc_ark_model_name(client)) ||
        !cJSON_AddBoolToObject(root, "stream", stream) ||
        !cJSON_AddStringToObject(message, "role", "user")) {
        ret = AI_ERR_NOMEM;
        goto out;
    }

    system_prompt = volc_ark_get_string(params, "system_prompt");
    if (!system_prompt) {
        system_prompt = volc_ark_get_string(params, "system");
    }
    if (system_prompt && system_prompt[0]) {
        ret = volc_ark_add_message_text(messages, "system", system_prompt);
        if (ret != AI_OK) {
            goto out;
        }
    }

    ret = volc_ark_add_optional_item(root, params, "temperature");
    if (ret == AI_OK) {
        ret = volc_ark_add_optional_item(root, params, "top_p");
    }
    if (ret == AI_OK) {
        ret = volc_ark_add_optional_item(root, params, "max_tokens");
    }
    if (ret == AI_OK) {
        ret = volc_ark_add_optional_item(root, params, "stop");
    }
    if (ret == AI_OK) {
        ret = volc_ark_add_optional_item(root, params, "user");
    }
    if (ret != AI_OK) {
        goto out;
    }

    if (resolved_image_url && resolved_image_url[0]) {
        ret = volc_ark_add_image_content(content, resolved_image_url);
        if (ret != AI_OK) {
            goto out;
        }
    }

    ret = volc_ark_add_text_content(content, prompt);
    if (ret != AI_OK) {
        goto out;
    }

    if (!cJSON_AddItemToObject(message, "content", content)) {
        ret = AI_ERR_NOMEM;
        goto out;
    }
    content = NULL;

    if (!cJSON_AddItemToArray(messages, message)) {
        ret = AI_ERR_NOMEM;
        goto out;
    }
    message = NULL;

    if (!cJSON_AddItemToObject(root, "messages", messages)) {
        ret = AI_ERR_NOMEM;
        goto out;
    }
    messages = NULL;

    ret = volc_ark_json_print(root, out_body);

out:
    cJSON_Delete(content);
    cJSON_Delete(message);
    cJSON_Delete(messages);
    cJSON_Delete(root);
    cJSON_Delete(params);
    ai_free_large(owned_data_url);
    return ret;
}

static int volc_ark_append_text(char **text, const char *value)
{
    char *next;
    size_t old_len;
    size_t value_len;

    if (!value || !value[0]) {
        return AI_OK;
    }

    if (!*text) {
        *text = ai_strdup(value);
        return *text ? AI_OK : AI_ERR_NOMEM;
    }

    old_len = strlen(*text);
    value_len = strlen(value);
    next = (char *)realloc(*text, old_len + value_len + 2);
    if (!next) {
        return AI_ERR_NOMEM;
    }

    next[old_len] = '\n';
    memcpy(next + old_len + 1, value, value_len + 1);
    *text = next;
    return AI_OK;
}

static int volc_ark_copy_content_text(cJSON *content, char **out_text)
{
    cJSON *item;
    const char *value;
    int count;
    int i;
    int ret;

    if (!out_text) {
        return AI_ERR_PARAM;
    }

    *out_text = NULL;
    if (!content) {
        *out_text = ai_strdup("");
        return *out_text ? AI_OK : AI_ERR_NOMEM;
    }

    if (cJSON_IsString(content)) {
        *out_text = ai_strdup(content->valuestring ? content->valuestring : "");
        return *out_text ? AI_OK : AI_ERR_NOMEM;
    }

    if (!cJSON_IsArray(content)) {
        *out_text = ai_strdup("");
        return *out_text ? AI_OK : AI_ERR_NOMEM;
    }

    count = cJSON_GetArraySize(content);
    for (i = 0; i < count; i++) {
        item = cJSON_GetArrayItem(content, i);
        value = NULL;
        if (cJSON_IsString(item)) {
            value = item->valuestring;
        } else {
            value = volc_ark_get_string(item, "text");
        }

        ret = volc_ark_append_text(out_text, value);
        if (ret != AI_OK) {
            free(*out_text);
            *out_text = NULL;
            return ret;
        }
    }

    if (!*out_text) {
        *out_text = ai_strdup("");
    }

    return *out_text ? AI_OK : AI_ERR_NOMEM;
}

static int volc_ark_copy_error(cJSON *root, struct ai_vision_result *out)
{
    cJSON *error;
    const char *message;

    error = cJSON_GetObjectItem(root, "error");
    if (!error) {
        return AI_OK;
    }

    message = volc_ark_get_string(error, "message");
    if (!message) {
        message = volc_ark_get_string(error, "msg");
    }
    if (!message) {
        message = volc_ark_get_string(root, "message");
    }
    if (!message) {
        message = "volc ark response error";
    }

    out->text = ai_strdup(message);
    return out->text ? AI_ERR_PARSE : AI_ERR_NOMEM;
}

static cJSON *volc_ark_choice_message(cJSON *choice)
{
    cJSON *message;

    if (!choice) {
        return NULL;
    }

    message = cJSON_GetObjectItem(choice, "message");
    if (message) {
        return message;
    }

    return cJSON_GetObjectItem(choice, "delta");
}

static int volc_ark_copy_message_content_text(cJSON *message, char **out_text)
{
    cJSON *content;

    if (!message || !out_text) {
        return AI_ERR_PARAM;
    }

    content = cJSON_GetObjectItem(message, "content");
    return volc_ark_copy_content_text(content, out_text);
}

static int volc_ark_parse_vision_http_stream_event(struct ai_client *client,
        const char *json,
        char **out_text,
        int *out_done)
{
    cJSON *root;
    cJSON *choices;
    cJSON *choice;
    cJSON *finish_reason;
    cJSON *message;
    int ret = AI_OK;

    (void)client;

    if (!json || !out_done) {
        return AI_ERR_PARAM;
    }

    if (out_text) {
        *out_text = NULL;
    }
    *out_done = 0;

    if (strcmp(json, "[DONE]") == 0) {
        *out_done = 1;
        return AI_OK;
    }

    root = cJSON_Parse(json);
    if (!root) {
        return AI_ERR_PARSE;
    }

    choices = cJSON_GetObjectItem(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) <= 0) {
        cJSON_Delete(root);
        return AI_OK;
    }

    choice = cJSON_GetArrayItem(choices, 0);
    if (!choice) {
        cJSON_Delete(root);
        return AI_OK;
    }

    finish_reason = cJSON_GetObjectItem(choice, "finish_reason");
    if (finish_reason && !cJSON_IsNull(finish_reason)) {
        *out_done = 1;
    }

    message = volc_ark_choice_message(choice);
    if (message && out_text) {
        ret = volc_ark_copy_message_content_text(message, out_text);
    }

    cJSON_Delete(root);
    return ret;
}

static int volc_ark_parse_vision_http_response(struct ai_client *client,
        const char *json,
        struct ai_vision_result *out)
{
    cJSON *root;
    cJSON *choices;
    cJSON *choice;
    cJSON *message;
    int ret;

    (void)client;

    if (!json || !out) {
        return AI_ERR_PARAM;
    }

    out->text = NULL;
    out->raw_json = ai_strdup(json);
    if (!out->raw_json) {
        return AI_ERR_NOMEM;
    }

    root = cJSON_Parse(json);
    if (!root) {
        return AI_ERR_PARSE;
    }

    ret = volc_ark_copy_error(root, out);
    if (ret != AI_OK) {
        cJSON_Delete(root);
        return ret;
    }

    choices = cJSON_GetObjectItem(root, "choices");
    if (!cJSON_IsArray(choices) || cJSON_GetArraySize(choices) <= 0) {
        cJSON_Delete(root);
        return AI_ERR_PARSE;
    }

    choice = cJSON_GetArrayItem(choices, 0);
    message = volc_ark_choice_message(choice);
    if (!message) {
        cJSON_Delete(root);
        return AI_ERR_PARSE;
    }

    ret = volc_ark_copy_message_content_text(message, &out->text);

    cJSON_Delete(root);
    return ret;
}

const struct ai_provider_ops ai_provider_volc_ark = {
    .name = "volc_ark",
    .caps = AI_PROVIDER_CAP_IMAGE_TO_TEXT,
    .flags = AI_PROVIDER_FLAG_VISION_API_KEY_REQUIRED,
    .default_vision_http_url = VOLC_ARK_VISION_HTTP_URL,
    .default_model = VOLC_ARK_DEFAULT_MODEL,
    .build_vision_http_body = volc_ark_build_vision_http_body,
    .parse_vision_http_response = volc_ark_parse_vision_http_response,
    .parse_vision_http_stream_event = volc_ark_parse_vision_http_stream_event,
};

#endif
