/****************************************************************************
 * packages/demos/knowledge_cards/src/llm_client.c
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cJSON.h>

#include "knowledge_cards.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define HTTP_PORT           443
#define HTTP_REQUEST_MAX    4096
#define DNS_TIMEOUT         5000

/****************************************************************************
 * Private Data
 ****************************************************************************/

static llm_config_t *g_llm_config = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: http_post_request
 *
 * Description:
 *   发送 HTTP POST 请求到 LLM API
 *
 ****************************************************************************/

static int http_post_request(const char *host, const char *path,
                             const char *body, char *response, int resp_len)
{
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char request[HTTP_REQUEST_MAX];
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
        fprintf(stderr, "ERROR: Failed to create socket: %d\n", errno);
        return -errno;
    }

    /* 设置服务器地址 */

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(HTTP_PORT);

    /* 连接服务器 */

    ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to connect: %d\n", errno);
        close(sock);
        return -errno;
    }

    /* 构建 HTTP 请求 */

    snprintf(request, sizeof(request),
             "POST %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/json\r\n"
             "Authorization: Bearer %s\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%s",
             path, host, g_llm_config->api_key,
             (int)strlen(body), body);

    /* 发送请求 */

    ret = send(sock, request, strlen(request), 0);
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to send request: %d\n", errno);
        close(sock);
        return -errno;
    }

    /* 接收响应 */

    ret = recv(sock, response, resp_len - 1, 0);
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to receive response: %d\n", errno);
        close(sock);
        return -errno;
    }

    response[ret] = '\0';
    close(sock);

    return ret;
}

/****************************************************************************
 * Name: parse_json_response
 *
 * Description:
 *   解析 LLM API 的 JSON 响应
 *
 ****************************************************************************/

static int parse_json_response(const char *response, char *content, int content_len)
{
    cJSON *root;
    cJSON *choices;
    cJSON *first_choice;
    cJSON *message;
    cJSON *content_obj;
    const char *json_start;

    /* 找到 JSON 开始位置（跳过 HTTP 头部） */

    json_start = strstr(response, "\r\n\r\n");
    if (json_start == NULL)
    {
        fprintf(stderr, "ERROR: Invalid HTTP response\n");
        return -EINVAL;
    }
    json_start += 4;

    /* 解析 JSON */

    root = cJSON_Parse(json_start);
    if (root == NULL)
    {
        fprintf(stderr, "ERROR: Failed to parse JSON\n");
        return -EINVAL;
    }

    /* 提取 content */

    choices = cJSON_GetObjectItem(root, "choices");
    if (choices == NULL || !cJSON_IsArray(choices))
    {
        fprintf(stderr, "ERROR: No choices in response\n");
        cJSON_Delete(root);
        return -EINVAL;
    }

    first_choice = cJSON_GetArrayItem(choices, 0);
    if (first_choice == NULL)
    {
        fprintf(stderr, "ERROR: Empty choices array\n");
        cJSON_Delete(root);
        return -EINVAL;
    }

    message = cJSON_GetObjectItem(first_choice, "message");
    if (message == NULL)
    {
        fprintf(stderr, "ERROR: No message in choice\n");
        cJSON_Delete(root);
        return -EINVAL;
    }

    content_obj = cJSON_GetObjectItem(message, "content");
    if (content_obj == NULL || !cJSON_IsString(content_obj))
    {
        fprintf(stderr, "ERROR: No content in message\n");
        cJSON_Delete(root);
        return -EINVAL;
    }

    strncpy(content, content_obj->valuestring, content_len - 1);
    content[content_len - 1] = '\0';

    cJSON_Delete(root);
    return 0;
}

/****************************************************************************
 * Name: extract_host_from_url
 *
 * Description:
 *   从 URL 中提取主机名
 *
 ****************************************************************************/

static int extract_host_from_url(const char *url, char *host, int host_len)
{
    const char *start;
    const char *end;

    /* 跳过协议前缀 */

    if (strncmp(url, "https://", 8) == 0)
    {
        start = url + 8;
    }
    else if (strncmp(url, "http://", 7) == 0)
    {
        start = url + 7;
    }
    else
    {
        start = url;
    }

    /* 找到路径开始位置 */

    end = strchr(start, '/');
    if (end == NULL)
    {
        end = start + strlen(start);
    }

    /* 复制主机名 */

    int len = end - start;
    if (len >= host_len)
    {
        len = host_len - 1;
    }

    strncpy(host, start, len);
    host[len] = '\0';

    return 0;
}

/****************************************************************************
 * Name: extract_path_from_url
 *
 * Description:
 *   从 URL 中提取路径
 *
 ****************************************************************************/

static int extract_path_from_url(const char *url, char *path, int path_len)
{
    const char *start;

    /* 找到路径开始位置 */

    start = strchr(url, '/');
    if (start == NULL)
    {
        strncpy(path, "/", path_len);
        return 0;
    }

    start = strchr(start + 1, '/');
    if (start == NULL)
    {
        strncpy(path, "/", path_len);
        return 0;
    }

    start = strchr(start + 1, '/');
    if (start == NULL)
    {
        strncpy(path, "/", path_len);
        return 0;
    }

    strncpy(path, start, path_len - 1);
    path[path_len - 1] = '\0';

    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: llm_init
 *
 * Description:
 *   初始化 LLM 客户端
 *
 ****************************************************************************/

int llm_init(llm_config_t *config)
{
    if (config == NULL)
    {
        fprintf(stderr, "ERROR: NULL config\n");
        return -EINVAL;
    }

    g_llm_config = config;

    /* 设置默认值 */

    if (strlen(config->endpoint) == 0)
    {
        strncpy(config->endpoint,
                "https://api.openai.com/v1/chat/completions",
                LLM_ENDPOINT_MAX_LEN - 1);
    }

    if (strlen(config->model) == 0)
    {
        strncpy(config->model, "gpt-3.5-turbo", sizeof(config->model) - 1);
    }

    if (config->max_tokens == 0)
    {
        config->max_tokens = 512;
    }

    if (config->timeout == 0)
    {
        config->timeout = 30;
    }

    printf("LLM client initialized\n");
    printf("  Endpoint: %s\n", config->endpoint);
    printf("  Model: %s\n", config->model);
    printf("  Max tokens: %d\n", config->max_tokens);

    return 0;
}

/****************************************************************************
 * Name: llm_generate_outline
 *
 * Description:
 *   生成知识大纲
 *
 ****************************************************************************/

int llm_generate_outline(const char *topic, knowledge_topic_t *result)
{
    char prompt[512];
    char body[1024];
    char response[LLM_RESPONSE_MAX_LEN];
    char content[LLM_RESPONSE_MAX_LEN];
    char host[128];
    char path[256];
    int ret;

    if (topic == NULL || result == NULL)
    {
        return -EINVAL;
    }

    /* 构建 prompt */

    snprintf(prompt, sizeof(prompt),
             "请为 \"%s\" 生成一个简洁的知识大纲，包含5-8个核心知识点。"
             "每个知识点用一行描述，格式为："
             "1. 知识点标题：简短描述（20字以内）"
             "2. 知识点标题：简短描述（20字以内）"
             "..."
             "只输出大纲，不要其他内容。",
             topic);

    /* 构建请求体 */

    snprintf(body, sizeof(body),
             "{\"model\":\"%s\","
             "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
             "\"max_tokens\":%d,"
             "\"temperature\":0.7}",
             g_llm_config->model, prompt, g_llm_config->max_tokens);

    /* 提取主机和路径 */

    extract_host_from_url(g_llm_config->endpoint, host, sizeof(host));
    extract_path_from_url(g_llm_config->endpoint, path, sizeof(path));

    /* 发送请求 */

    ret = http_post_request(host, path, body, response, sizeof(response));
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: HTTP request failed: %d\n", ret);
        return ret;
    }

    /* 解析响应 */

    ret = parse_json_response(response, content, sizeof(content));
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to parse response: %d\n", ret);
        return ret;
    }

    /* 解析大纲内容 */

    strncpy(result->topic, topic, CARD_MAX_TOPIC_LEN - 1);
    result->card_count = 0;
    result->current_index = 0;
    result->outline_generated = true;

    /* 简单解析：按行分割，每行一个知识点 */

    char *line = strtok(content, "\n");
    while (line != NULL && result->card_count < CARD_MAX_CARDS)
    {
        /* 跳过空行 */

        if (strlen(line) < 3)
        {
            line = strtok(NULL, "\n");
            continue;
        }

        /* 提取标题（冒号前的部分） */

        char *colon = strchr(line, '：');
        if (colon == NULL)
        {
            colon = strchr(line, ':');
        }

        if (colon != NULL)
        {
            int title_len = colon - line;
            if (title_len >= CARD_MAX_TITLE_LEN)
            {
                title_len = CARD_MAX_TITLE_LEN - 1;
            }

            strncpy(result->cards[result->card_count].title, line, title_len);
            result->cards[result->card_count].title[title_len] = '\0';

            /* 提取内容（冒号后的部分） */

            strncpy(result->cards[result->card_count].content,
                    colon + 1, CARD_MAX_CONTENT_LEN - 1);
        }
        else
        {
            /* 没有冒号，整行作为标题 */

            strncpy(result->cards[result->card_count].title,
                    line, CARD_MAX_TITLE_LEN - 1);
            result->cards[result->card_count].content[0] = '\0';
        }

        result->cards[result->card_count].is_loaded = false;
        result->cards[result->card_count].difficulty = 1; /* 默认甜点 */
        result->card_count++;

        line = strtok(NULL, "\n");
    }

    printf("Generated outline for \"%s\" with %d cards\n",
           topic, result->card_count);

    return 0;
}

/****************************************************************************
 * Name: llm_generate_card_content
 *
 * Description:
 *   为单个卡片生成详细内容
 *
 ****************************************************************************/

int llm_generate_card_content(knowledge_card_t *card)
{
    char prompt[512];
    char body[1024];
    char response[LLM_RESPONSE_MAX_LEN];
    char content[LLM_RESPONSE_MAX_LEN];
    char host[128];
    char path[256];
    int ret;

    if (card == NULL)
    {
        return -EINVAL;
    }

    /* 如果已经加载，直接返回 */

    if (card->is_loaded)
    {
        return 0;
    }

    /* 构建 prompt */

    snprintf(prompt, sizeof(prompt),
             "请详细解释 \"%s\" 这个知识点。"
             "要求："
             "1. 用简单易懂的语言"
             "2. 控制在100字以内"
             "3. 适合在手表小屏幕上阅读"
             "4. 只输出解释内容，不要标题",
             card->title);

    /* 构建请求体 */

    snprintf(body, sizeof(body),
             "{\"model\":\"%s\","
             "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
             "\"max_tokens\":256,"
             "\"temperature\":0.7}",
             g_llm_config->model, prompt);

    /* 提取主机和路径 */

    extract_host_from_url(g_llm_config->endpoint, host, sizeof(host));
    extract_path_from_url(g_llm_config->endpoint, path, sizeof(path));

    /* 发送请求 */

    ret = http_post_request(host, path, body, response, sizeof(response));
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: HTTP request failed: %d\n", ret);
        return ret;
    }

    /* 解析响应 */

    ret = parse_json_response(response, content, sizeof(content));
    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to parse response: %d\n", ret);
        return ret;
    }

    /* 更新卡片内容 */

    strncpy(card->content, content, CARD_MAX_CONTENT_LEN - 1);
    card->is_loaded = true;

    printf("Generated content for: %s\n", card->title);

    return 0;
}

/****************************************************************************
 * Name: llm_cleanup
 *
 * Description:
 *   清理 LLM 客户端资源
 *
 ****************************************************************************/

void llm_cleanup(void)
{
    g_llm_config = NULL;
    printf("LLM client cleaned up\n");
}
