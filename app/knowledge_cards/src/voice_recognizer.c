/****************************************************************************
 * packages/demos/knowledge_cards/src/voice_recognizer.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <media_api.h>
#include <uv.h>
#include <cJSON.h>

#include "knowledge_cards.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define VOICE_BUFFER_SIZE       (128 * 1024)  /* 128KB 音频缓冲区 */
#define VOICE_SAMPLE_RATE       16000         /* 16kHz 采样率 */
#define VOICE_CHANNELS          1             /* 单声道 */
#define VOICE_BITS              16            /* 16位 */
#define VOICE_FRAME_SIZE        320           /* 20ms 一帧 (16000 * 0.02) */

/* 火山引擎 ASR 配置 */

#define VOLC_ASR_HOST           "openspeech.bytedance.com"
#define VOLC_ASR_PATH           "/api/v1/asr"
#define VOLC_ASR_PORT           443

/* 讯飞 ASR 配置 */

#define XFYUN_ASR_HOST          "api.xfyun.cn"
#define XFYUN_ASR_PATH          "/v1/service/v1/iat"
#define XFYUN_ASR_PORT          443

/* HTTP 请求缓冲区 */

#define HTTP_REQUEST_MAX        (256 * 1024)  /* 256KB */
#define HTTP_RESPONSE_MAX       (64 * 1024)   /* 64KB */

/* 语音识别状态 */

typedef enum voice_state_e
{
    VOICE_STATE_IDLE,
    VOICE_STATE_RECORDING,
    VOICE_STATE_PROCESSING,
    VOICE_STATE_DONE,
    VOICE_STATE_ERROR
} voice_state_t;

/* 语音识别服务类型 */

typedef enum asr_provider_e
{
    ASR_PROVIDER_VOLC,      /* 火山引擎 */
    ASR_PROVIDER_XFYUN,     /* 讯飞 */
    ASR_PROVIDER_LOCAL      /* 本地（回退） */
} asr_provider_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

typedef struct voice_context_s
{
    /* Media 相关 */

    void *recorder_handle;
    uv_loop_t *loop;
    uv_pipe_t *recorder_pipe;

    /* 状态管理 */

    voice_state_t state;
    bool is_running;
    pthread_t process_thread;
    sem_t state_sem;

    /* 音频缓冲区 */

    char *audio_buffer;
    int buffer_pos;
    int buffer_size;

    /* 识别结果 */

    char result_text[256];
    voice_result_callback_t result_callback;
    void *callback_userdata;

    /* 配置 */

    const char *stream_name;
    asr_provider_t asr_provider;
    char api_key[128];
    char api_secret[128];
} voice_context_t;

static voice_context_t g_voice_ctx;

/****************************************************************************
 * Private Functions - Base64 编码
 ****************************************************************************/

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* base64_encode(const unsigned char *data, size_t input_length,
                           size_t *output_length)
{
    *output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(*output_length + 1);
    if (encoded_data == NULL)
    {
        return NULL;
    }

    size_t i = 0;
    size_t j = 0;
    while (i < input_length)
    {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = base64_chars[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 0 * 6) & 0x3F];
    }

    for (size_t k = 0; k < (3 - input_length % 3) % 3; k++)
    {
        encoded_data[*output_length - 1 - k] = '=';
    }

    encoded_data[*output_length] = '\0';
    return encoded_data;
}

/****************************************************************************
 * Private Functions - HTTP 请求
 ****************************************************************************/

static int http_post(const char *host, int port, const char *path,
                     const char *headers, const char *body, int body_len,
                     char *response, int resp_size)
{
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char *request;
    int ret;

    /* DNS 解析 */

    server = gethostbyname(host);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR: DNS resolution failed for %s\n", host);
        return -ENXIO;
    }

    /* 创建 socket */

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return -errno;
    }

    /* 设置服务器地址 */

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    /* 连接服务器 */

    ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        close(sock);
        return -errno;
    }

    /* 构建 HTTP 请求 */

    request = malloc(HTTP_REQUEST_MAX);
    if (request == NULL)
    {
        close(sock);
        return -ENOMEM;
    }

    int req_len = snprintf(request, HTTP_REQUEST_MAX,
                           "POST %s HTTP/1.1\r\n"
                           "Host: %s\r\n"
                           "Content-Type: application/json\r\n"
                           "%s"
                           "Content-Length: %d\r\n"
                           "\r\n"
                           "%s",
                           path, host, headers ? headers : "",
                           body_len, body);

    /* 发送请求 */

    ret = send(sock, request, req_len, 0);
    free(request);

    if (ret < 0)
    {
        close(sock);
        return -errno;
    }

    /* 接收响应 */

    ret = recv(sock, response, resp_size - 1, 0);
    close(sock);

    if (ret < 0)
    {
        return -errno;
    }

    response[ret] = '\0';
    return ret;
}

/****************************************************************************
 * Private Functions - 火山引擎 ASR
 ****************************************************************************/

static int volc_asr_recognize(const char *audio_data, int audio_len,
                              char *result, int result_size)
{
    char *audio_b64;
    size_t b64_len;
    char *body;
    char *response;
    char headers[256];
    int ret;

    /* Base64 编码音频数据 */

    audio_b64 = base64_encode((const unsigned char *)audio_data, audio_len,
                              &b64_len);
    if (audio_b64 == NULL)
    {
        return -ENOMEM;
    }

    /* 构建请求体 */

    body = malloc(HTTP_REQUEST_MAX);
    response = malloc(HTTP_RESPONSE_MAX);
    if (body == NULL || response == NULL)
    {
        free(audio_b64);
        free(body);
        free(response);
        return -ENOMEM;
    }

    /* 构建火山引擎 ASR 请求 JSON */

    cJSON *root = cJSON_CreateObject();
    cJSON *audio = cJSON_CreateObject();

    cJSON_AddStringToObject(audio, "format", "pcm");
    cJSON_AddNumberToObject(audio, "rate", VOICE_SAMPLE_RATE);
    cJSON_AddNumberToObject(audio, "bits", VOICE_BITS);
    cJSON_AddNumberToObject(audio, "channel", VOICE_CHANNELS);
    cJSON_AddStringToObject(audio, "language", "zh-CN");

    cJSON_AddStringToObject(root, "app", "knowledge_cards");
    cJSON_AddStringToObject(root, "token", g_voice_ctx.api_key);
    cJSON_AddStringToObject(root, "audio", audio_b64);
    cJSON_AddNumberToObject(root, "audio_size", audio_len);
    cJSON_AddItemToObject(root, "additions", audio);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL)
    {
        free(audio_b64);
        free(body);
        free(response);
        return -ENOMEM;
    }

    /* 构建认证头 */

    snprintf(headers, sizeof(headers),
             "Authorization: Bearer; %s\r\n",
             g_voice_ctx.api_key);

    /* 发送请求 */

    ret = http_post(VOLC_ASR_HOST, VOLC_ASR_PORT, VOLC_ASR_PATH,
                    headers, json_str, strlen(json_str),
                    response, HTTP_RESPONSE_MAX);

    free(json_str);
    free(audio_b64);

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Volc ASR request failed: %d\n", ret);
        free(body);
        free(response);
        return ret;
    }

    /* 解析响应 */

    char *json_start = strstr(response, "\r\n\r\n");
    if (json_start == NULL)
    {
        free(body);
        free(response);
        return -EINVAL;
    }
    json_start += 4;

    cJSON *resp_json = cJSON_Parse(json_start);
    if (resp_json == NULL)
    {
        free(body);
        free(response);
        return -EINVAL;
    }

    /* 提取识别结果 */

    cJSON *result_obj = cJSON_GetObjectItem(resp_json, "result");
    if (result_obj != NULL && cJSON_IsString(result_obj))
    {
        strncpy(result, result_obj->valuestring, result_size - 1);
        result[result_size - 1] = '\0';
        ret = 0;
    }
    else
    {
        /* 尝试其他字段 */

        cJSON *text_obj = cJSON_GetObjectItem(resp_json, "text");
        if (text_obj != NULL && cJSON_IsString(text_obj))
        {
            strncpy(result, text_obj->valuestring, result_size - 1);
            result[result_size - 1] = '\0';
            ret = 0;
        }
        else
        {
            fprintf(stderr, "ERROR: No result in ASR response\n");
            ret = -EINVAL;
        }
    }

    cJSON_Delete(resp_json);
    free(body);
    free(response);

    return ret;
}

/****************************************************************************
 * Private Functions - 讯飞 ASR
 ****************************************************************************/

static int xfyun_asr_recognize(const char *audio_data, int audio_len,
                               char *result, int result_size)
{
    char *audio_b64;
    size_t b64_len;
    char *body;
    char *response;
    char headers[512];
    int ret;

    /* Base64 编码音频数据 */

    audio_b64 = base64_encode((const unsigned char *)audio_data, audio_len,
                              &b64_len);
    if (audio_b64 == NULL)
    {
        return -ENOMEM;
    }

    /* 构建请求体 */

    body = malloc(HTTP_REQUEST_MAX);
    response = malloc(HTTP_RESPONSE_MAX);
    if (body == NULL || response == NULL)
    {
        free(audio_b64);
        free(body);
        free(response);
        return -ENOMEM;
    }

    /* 构建讯飞 ASR 请求 JSON */

    cJSON *root = cJSON_CreateObject();
    cJSON *common = cJSON_CreateObject();
    cJSON *business = cJSON_CreateObject();
    cJSON *data = cJSON_CreateObject();

    cJSON_AddStringToObject(common, "app_id", "knowledge_cards");

    cJSON_AddStringToObject(business, "language", "zh_cn");
    cJSON_AddStringToObject(business, "domain", "iat");
    cJSON_AddStringToObject(business, "accent", "mandarin");
    cJSON_AddStringToObject(business, "dwa", "wpgs");

    cJSON_AddNumberToObject(data, "status", 2);
    cJSON_AddStringToObject(data, "format", "audio/pcm");
    cJSON_AddNumberToObject(data, "rate", VOICE_SAMPLE_RATE);
    cJSON_AddStringToObject(data, "encoding", "raw");
    cJSON_AddStringToObject(data, "audio", audio_b64);

    cJSON_AddItemToObject(root, "common", common);
    cJSON_AddItemToObject(root, "business", business);
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL)
    {
        free(audio_b64);
        free(body);
        free(response);
        return -ENOMEM;
    }

    /* 构建认证头 */

    snprintf(headers, sizeof(headers),
             "X-Appid: knowledge_cards\r\n"
             "X-CurTime: %ld\r\n"
             "X-Param: eyJsYW5ndWFnZSI6ImNoX2NuIiwiZG9tYWluIjoiaWF0In0=\r\n"
             "X-CheckSum: %s\r\n",
             (long)time(NULL),
             g_voice_ctx.api_key);

    /* 发送请求 */

    ret = http_post(XFYUN_ASR_HOST, XFYUN_ASR_PORT, XFYUN_ASR_PATH,
                    headers, json_str, strlen(json_str),
                    response, HTTP_RESPONSE_MAX);

    free(json_str);
    free(audio_b64);

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: XFYun ASR request failed: %d\n", ret);
        free(body);
        free(response);
        return ret;
    }

    /* 解析响应 */

    char *json_start = strstr(response, "\r\n\r\n");
    if (json_start == NULL)
    {
        free(body);
        free(response);
        return -EINVAL;
    }
    json_start += 4;

    cJSON *resp_json = cJSON_Parse(json_start);
    if (resp_json == NULL)
    {
        free(body);
        free(response);
        return -EINVAL;
    }

    /* 提取识别结果 */

    cJSON *result_obj = cJSON_GetObjectItem(resp_json, "result");
    if (result_obj != NULL)
    {
        cJSON *ws_array = cJSON_GetObjectItem(result_obj, "ws");
        if (ws_array != NULL && cJSON_IsArray(ws_array))
        {
            result[0] = '\0';
            int ws_count = cJSON_GetArraySize(ws_array);

            for (int i = 0; i < ws_count; i++)
            {
                cJSON *ws_item = cJSON_GetArrayItem(ws_array, i);
                cJSON *cw_array = cJSON_GetObjectItem(ws_item, "cw");

                if (cw_array != NULL && cJSON_IsArray(cw_array))
                {
                    int cw_count = cJSON_GetArraySize(cw_array);
                    for (int j = 0; j < cw_count; j++)
                    {
                        cJSON *cw_item = cJSON_GetArrayItem(cw_array, j);
                        cJSON *w_obj = cJSON_GetObjectItem(cw_item, "w");
                        if (w_obj != NULL && cJSON_IsString(w_obj))
                        {
                            strncat(result, w_obj->valuestring,
                                    result_size - strlen(result) - 1);
                        }
                    }
                }
            }
            ret = 0;
        }
        else
        {
            ret = -EINVAL;
        }
    }
    else
    {
        ret = -EINVAL;
    }

    cJSON_Delete(resp_json);
    free(body);
    free(response);

    return ret;
}

/****************************************************************************
 * Private Functions - 本地识别（回退方案）
 ****************************************************************************/

static int local_asr_recognize(const char *audio_data, int audio_len,
                               char *result, int result_size)
{
    /* 本地识别回退方案
     * 这里可以集成轻量级本地模型
     * 目前返回一个默认值用于测试
     */

    printf("Using local ASR fallback (audio: %d bytes)\n", audio_len);

    /* 分析音频能量，判断是否有语音 */

    int16_t *samples = (int16_t *)audio_data;
    int sample_count = audio_len / 2;
    long energy = 0;

    for (int i = 0; i < sample_count; i++)
    {
        energy += abs(samples[i]);
    }

    long avg_energy = energy / sample_count;

    /* 如果平均能量超过阈值，认为有语音输入 */

    if (avg_energy > 1000)
    {
        /* 检测到语音，但无法识别具体内容 */

        snprintf(result, result_size, "检测到语音输入");
        return 0;
    }
    else
    {
        snprintf(result, result_size, "未检测到语音");
        return 0;
    }
}

/****************************************************************************
 * Private Functions - Media Callbacks
 ****************************************************************************/

static void recorder_open_cb(void *cookie, int ret)
{
    voice_context_t *ctx = (voice_context_t *)cookie;

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Recorder open failed: %d\n", ret);
        ctx->state = VOICE_STATE_ERROR;
        sem_post(&ctx->state_sem);
        return;
    }

    printf("Recorder opened successfully\n");
}

static void recorder_prepare_cb(void *cookie, int ret)
{
    voice_context_t *ctx = (voice_context_t *)cookie;

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Recorder prepare failed: %d\n", ret);
        ctx->state = VOICE_STATE_ERROR;
        sem_post(&ctx->state_sem);
        return;
    }

    printf("Recorder prepared, starting recording...\n");

    ret = media_uv_recorder_start(ctx->recorder_handle,
                                  (media_uv_recorder_start_cb)recorder_open_cb,
                                  ctx);
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to start recording: %d\n", ret);
        ctx->state = VOICE_STATE_ERROR;
        sem_post(&ctx->state_sem);
        return;
    }

    ctx->state = VOICE_STATE_RECORDING;
    sem_post(&ctx->state_sem);
}

static void recorder_data_cb(void *cookie, int ret, void *data, int len)
{
    voice_context_t *ctx = (voice_context_t *)cookie;

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Recorder data error: %d\n", ret);
        return;
    }

    if (data == NULL || len <= 0)
    {
        return;
    }

    /* 将音频数据存入缓冲区 */

    if (ctx->buffer_pos + len < ctx->buffer_size)
    {
        memcpy(ctx->audio_buffer + ctx->buffer_pos, data, len);
        ctx->buffer_pos += len;
    }
    else
    {
        printf("Audio buffer full, stopping recording\n");
        ctx->is_running = false;
    }
}

static void recorder_event_cb(void *cookie, int event, int ret, const char *extra)
{
    voice_context_t *ctx = (voice_context_t *)cookie;

    switch (event)
    {
        case MEDIA_RECORDER_EVENT_STARTED:
            printf("Recording started\n");
            break;

        case MEDIA_RECORDER_EVENT_STOPPED:
            printf("Recording stopped\n");
            ctx->state = VOICE_STATE_PROCESSING;
            sem_post(&ctx->state_sem);
            break;

        case MEDIA_RECORDER_EVENT_ERROR:
            fprintf(stderr, "Recorder error: %d\n", ret);
            ctx->state = VOICE_STATE_ERROR;
            sem_post(&ctx->state_sem);
            break;

        default:
            break;
    }
}

/****************************************************************************
 * Private Functions - 处理线程
 ****************************************************************************/

static void *voice_process_thread(void *arg)
{
    voice_context_t *ctx = (voice_context_t *)arg;
    int ret;

    /* 等待录音完成 */

    while (ctx->is_running)
    {
        usleep(10000); /* 10ms */
    }

    /* 停止录音机 */

    if (ctx->recorder_handle)
    {
        media_uv_recorder_stop(ctx->recorder_handle, NULL, NULL);
    }

    /* 处理音频数据 */

    if (ctx->buffer_pos > 0 && ctx->state != VOICE_STATE_ERROR)
    {
        printf("Processing %d bytes of audio data\n", ctx->buffer_pos);

        /* 调用语音识别 API */

        switch (ctx->asr_provider)
        {
            case ASR_PROVIDER_VOLC:
                ret = volc_asr_recognize(ctx->audio_buffer, ctx->buffer_pos,
                                         ctx->result_text, sizeof(ctx->result_text));
                break;

            case ASR_PROVIDER_XFYUN:
                ret = xfyun_asr_recognize(ctx->audio_buffer, ctx->buffer_pos,
                                          ctx->result_text, sizeof(ctx->result_text));
                break;

            case ASR_PROVIDER_LOCAL:
            default:
                ret = local_asr_recognize(ctx->audio_buffer, ctx->buffer_pos,
                                          ctx->result_text, sizeof(ctx->result_text));
                break;
        }

        if (ret == 0)
        {
            printf("ASR result: %s\n", ctx->result_text);
            ctx->state = VOICE_STATE_DONE;

            /* 调用回调函数 */

            if (ctx->result_callback)
            {
                ctx->result_callback(ctx->result_text, ctx->callback_userdata);
            }
        }
        else
        {
            fprintf(stderr, "ASR failed: %d\n", ret);
            ctx->state = VOICE_STATE_ERROR;
        }
    }
    else
    {
        printf("No audio data to process\n");
        ctx->state = VOICE_STATE_DONE;
    }

    /* 清理 */

    if (ctx->recorder_handle)
    {
        media_uv_recorder_close(ctx->recorder_handle, NULL);
        ctx->recorder_handle = NULL;
    }

    sem_post(&ctx->state_sem);
    return NULL;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int voice_init(void)
{
    voice_context_t *ctx = &g_voice_ctx;

    memset(ctx, 0, sizeof(voice_context_t));

    ctx->audio_buffer = malloc(VOICE_BUFFER_SIZE);
    if (ctx->audio_buffer == NULL)
    {
        return -ENOMEM;
    }

    ctx->buffer_size = VOICE_BUFFER_SIZE;
    ctx->buffer_pos = 0;
    ctx->state = VOICE_STATE_IDLE;
    ctx->is_running = false;
    ctx->stream_name = CONFIG_KNOWLEDGE_CARDS_VOICE_STREAM;

    /* 根据配置选择 ASR 提供商 */

    const char *provider = getenv("ASR_PROVIDER");
    if (provider != NULL && strcmp(provider, "volc") == 0)
    {
        ctx->asr_provider = ASR_PROVIDER_VOLC;
    }
    else if (provider != NULL && strcmp(provider, "xfyun") == 0)
    {
        ctx->asr_provider = ASR_PROVIDER_XFYUN;
    }
    else
    {
        ctx->asr_provider = ASR_PROVIDER_LOCAL;
    }

    /* 读取 API Key */

    const char *api_key = getenv("ASR_API_KEY");
    if (api_key != NULL)
    {
        strncpy(ctx->api_key, api_key, sizeof(ctx->api_key) - 1);
    }

    sem_init(&ctx->state_sem, 0, 0);

    printf("Voice recognizer initialized\n");
    printf("  Stream: %s\n", ctx->stream_name);
    printf("  Buffer size: %d bytes\n", VOICE_BUFFER_SIZE);
    printf("  ASR provider: %s\n",
           ctx->asr_provider == ASR_PROVIDER_VOLC ? "Volc Engine" :
           ctx->asr_provider == ASR_PROVIDER_XFYUN ? "Xunfei" : "Local");

    return 0;
}

int voice_start_listening(void)
{
    voice_context_t *ctx = &g_voice_ctx;
    pthread_attr_t attr;
    int ret;

    if (ctx->state == VOICE_STATE_RECORDING)
    {
        printf("Already recording\n");
        return -EALREADY;
    }

    ctx->buffer_pos = 0;
    ctx->state = VOICE_STATE_IDLE;
    ctx->is_running = true;
    ctx->result_text[0] = '\0';

    ctx->loop = uv_default_loop();

    ctx->recorder_handle = media_uv_recorder_open(
        ctx->loop,
        ctx->stream_name,
        recorder_open_cb,
        ctx
    );

    if (ctx->recorder_handle == NULL)
    {
        fprintf(stderr, "ERROR: Failed to open recorder\n");
        ctx->is_running = false;
        return -ENODEV;
    }

    ret = media_uv_recorder_listen(ctx->recorder_handle, recorder_event_cb);
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to listen recorder events: %d\n", ret);
        media_uv_recorder_close(ctx->recorder_handle, NULL);
        ctx->recorder_handle = NULL;
        ctx->is_running = false;
        return ret;
    }

    char format[64];
    snprintf(format, sizeof(format), "pcm/%d/%d/%d",
             VOICE_SAMPLE_RATE, VOICE_CHANNELS, VOICE_BITS);

    ret = media_uv_recorder_prepare(
        ctx->recorder_handle,
        NULL,
        format,
        (media_uv_recorder_prepare_cb)recorder_prepare_cb,
        NULL,
        NULL
    );

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to prepare recorder: %d\n", ret);
        media_uv_recorder_close(ctx->recorder_handle, NULL);
        ctx->recorder_handle = NULL;
        ctx->is_running = false;
        return ret;
    }

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8192);

    ret = pthread_create(&ctx->process_thread, &attr,
                         voice_process_thread, ctx);
    pthread_attr_destroy(&attr);

    if (ret != 0)
    {
        fprintf(stderr, "ERROR: Failed to create process thread: %d\n", ret);
        media_uv_recorder_close(ctx->recorder_handle, NULL);
        ctx->recorder_handle = NULL;
        ctx->is_running = false;
        return -ret;
    }

    sem_wait(&ctx->state_sem);

    if (ctx->state == VOICE_STATE_ERROR)
    {
        ctx->is_running = false;
        return -EIO;
    }

    printf("Voice listening started\n");
    return 0;
}

int voice_stop_listening(void)
{
    voice_context_t *ctx = &g_voice_ctx;

    if (ctx->state != VOICE_STATE_RECORDING)
    {
        printf("Not recording\n");
        return -EALREADY;
    }

    ctx->is_running = false;

    sem_wait(&ctx->state_sem);

    pthread_join(ctx->process_thread, NULL);

    printf("Voice listening stopped\n");
    printf("Result: %s\n", ctx->result_text);

    return 0;
}

bool voice_is_listening(void)
{
    voice_context_t *ctx = &g_voice_ctx;
    return (ctx->state == VOICE_STATE_RECORDING);
}

int voice_register_callback(voice_result_callback_t callback, void *userdata)
{
    voice_context_t *ctx = &g_voice_ctx;

    ctx->result_callback = callback;
    ctx->callback_userdata = userdata;

    return 0;
}

const char* voice_get_result(void)
{
    voice_context_t *ctx = &g_voice_ctx;
    return ctx->result_text;
}

void voice_cleanup(void)
{
    voice_context_t *ctx = &g_voice_ctx;

    if (ctx->state == VOICE_STATE_RECORDING)
    {
        voice_stop_listening();
    }

    if (ctx->audio_buffer)
    {
        free(ctx->audio_buffer);
        ctx->audio_buffer = NULL;
    }

    sem_destroy(&ctx->state_sem);

    printf("Voice recognizer cleaned up\n");
}
