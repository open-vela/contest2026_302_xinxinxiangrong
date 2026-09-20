/****************************************************************************
 * packages/demos/knowledge_cards/src/card_manager.c
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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <cJSON.h>

#include "knowledge_cards.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define TOPIC_FILE_FORMAT   "%s/%s.json"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char *get_storage_dir(void)
{
    const char *env = getenv("KNOWLEDGE_STORAGE_DIR");
    return env ? env : "/data/knowledge";
}

/****************************************************************************
 * Private Data
 ****************************************************************************/

static knowledge_topic_t g_current_topic;
static bool g_initialized = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ensure_storage_dir
 *
 * Description:
 *   确保存储目录存在
 *
 ****************************************************************************/

static int ensure_storage_dir(void)
{
    /* 尝试创建存储目录 */

    const char *dir = get_storage_dir();
    int ret = mkdir(dir, 0755);
    if (ret < 0 && errno != EEXIST)
    {
        fprintf(stderr, "ERROR: Failed to create storage dir %s: %d\n",
                dir, errno);
        return -errno;
    }

    return 0;
}

/****************************************************************************
 * Name: get_topic_file_path
 *
 * Description:
 *   获取主题文件的完整路径
 *
 ****************************************************************************/

static int get_topic_file_path(const char *topic, char *path, int path_len)
{
    snprintf(path, path_len, TOPIC_FILE_FORMAT, get_storage_dir(), topic);
    return 0;
}

/****************************************************************************
 * Name: save_topic_to_file
 *
 * Description:
 *   将主题保存到文件
 *
 ****************************************************************************/

static int save_topic_to_file(const char *topic, const knowledge_topic_t *data)
{
    char filepath[256];
    cJSON *root;
    cJSON *cards_array;
    cJSON *card_obj;
    char *json_string;
    int fd;
    int ret;

    get_topic_file_path(topic, filepath, sizeof(filepath));

    /* 确保存储目录存在 */

    ensure_storage_dir();

    /* 创建 JSON 对象 */

    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return -ENOMEM;
    }

    cJSON_AddStringToObject(root, "topic", data->topic);
    cJSON_AddNumberToObject(root, "card_count", data->card_count);
    cJSON_AddNumberToObject(root, "current_index", data->current_index);
    cJSON_AddBoolToObject(root, "outline_generated", data->outline_generated);

    /* 创建卡片数组 */

    cards_array = cJSON_CreateArray();
    if (cards_array == NULL)
    {
        cJSON_Delete(root);
        return -ENOMEM;
    }

    cJSON_AddItemToObject(root, "cards", cards_array);

    for (int i = 0; i < data->card_count; i++)
    {
        card_obj = cJSON_CreateObject();
        if (card_obj == NULL)
        {
            continue;
        }

        cJSON_AddStringToObject(card_obj, "title", data->cards[i].title);
        cJSON_AddStringToObject(card_obj, "content", data->cards[i].content);
        cJSON_AddNumberToObject(card_obj, "difficulty", data->cards[i].difficulty);
        cJSON_AddBoolToObject(card_obj, "is_loaded", data->cards[i].is_loaded);

        cJSON_AddItemToArray(cards_array, card_obj);
    }

    /* 转换为字符串 */

    json_string = cJSON_Print(root);
    if (json_string == NULL)
    {
        cJSON_Delete(root);
        return -ENOMEM;
    }

    /* 写入文件 */

    fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "ERROR: Failed to open file %s: %d\n", filepath, errno);
        free(json_string);
        cJSON_Delete(root);
        return -errno;
    }

    ret = write(fd, json_string, strlen(json_string));
    close(fd);

    free(json_string);
    cJSON_Delete(root);

    if (ret < 0)
    {
        fprintf(stderr, "ERROR: Failed to write file: %d\n", errno);
        return -errno;
    }

    printf("Saved topic \"%s\" to %s\n", topic, filepath);
    return 0;
}

/****************************************************************************
 * Name: load_topic_from_file
 *
 * Description:
 *   从文件加载主题
 *
 ****************************************************************************/

static int load_topic_from_file(const char *topic, knowledge_topic_t *data)
{
    char filepath[256];
    cJSON *root;
    cJSON *cards_array;
    cJSON *card_obj;
    cJSON *item;
    char *json_buffer;
    int fd;
    int ret;
    int file_size;

    get_topic_file_path(topic, filepath, sizeof(filepath));

    /* 读取文件 */

    fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        /* 文件不存在，不是错误 */

        return -ENOENT;
    }

    /* 获取文件大小 */

    file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (file_size <= 0 || file_size > 65536)
    {
        close(fd);
        return -EINVAL;
    }

    /* 分配缓冲区 */

    json_buffer = malloc(file_size + 1);
    if (json_buffer == NULL)
    {
        close(fd);
        return -ENOMEM;
    }

    /* 读取内容 */

    ret = read(fd, json_buffer, file_size);
    close(fd);

    if (ret != file_size)
    {
        free(json_buffer);
        return -EIO;
    }

    json_buffer[file_size] = '\0';

    /* 解析 JSON */

    root = cJSON_Parse(json_buffer);
    free(json_buffer);

    if (root == NULL)
    {
        fprintf(stderr, "ERROR: Failed to parse JSON from %s\n", filepath);
        return -EINVAL;
    }

    /* 提取数据 */

    item = cJSON_GetObjectItem(root, "topic");
    if (item != NULL && cJSON_IsString(item))
    {
        strncpy(data->topic, item->valuestring, CARD_MAX_TOPIC_LEN - 1);
    }

    item = cJSON_GetObjectItem(root, "card_count");
    if (item != NULL && cJSON_IsNumber(item))
    {
        data->card_count = (int)item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "current_index");
    if (item != NULL && cJSON_IsNumber(item))
    {
        data->current_index = (int)item->valuedouble;
    }

    item = cJSON_GetObjectItem(root, "outline_generated");
    if (item != NULL && cJSON_IsBool(item))
    {
        data->outline_generated = cJSON_IsTrue(item);
    }

    /* 提取卡片数组 */

    cards_array = cJSON_GetObjectItem(root, "cards");
    if (cards_array != NULL && cJSON_IsArray(cards_array))
    {
        int array_size = cJSON_GetArraySize(cards_array);
        int count = (array_size < CARD_MAX_CARDS) ? array_size : CARD_MAX_CARDS;

        for (int i = 0; i < count; i++)
        {
            card_obj = cJSON_GetArrayItem(cards_array, i);
            if (card_obj == NULL)
            {
                continue;
            }

            item = cJSON_GetObjectItem(card_obj, "title");
            if (item != NULL && cJSON_IsString(item))
            {
                strncpy(data->cards[i].title, item->valuestring,
                        CARD_MAX_TITLE_LEN - 1);
            }

            item = cJSON_GetObjectItem(card_obj, "content");
            if (item != NULL && cJSON_IsString(item))
            {
                strncpy(data->cards[i].content, item->valuestring,
                        CARD_MAX_CONTENT_LEN - 1);
            }

            item = cJSON_GetObjectItem(card_obj, "difficulty");
            if (item != NULL && cJSON_IsNumber(item))
            {
                data->cards[i].difficulty = (int)item->valuedouble;
            }

            item = cJSON_GetObjectItem(card_obj, "is_loaded");
            if (item != NULL && cJSON_IsBool(item))
            {
                data->cards[i].is_loaded = cJSON_IsTrue(item);
            }
        }
    }

    cJSON_Delete(root);

    printf("Loaded topic \"%s\" with %d cards\n", topic, data->card_count);
    return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: card_manager_init
 *
 * Description:
 *   初始化卡片管理器
 *
 ****************************************************************************/

int card_manager_init(void)
{
    if (g_initialized)
    {
        return 0;
    }

    memset(&g_current_topic, 0, sizeof(g_current_topic));
    g_initialized = true;

    ensure_storage_dir();

    printf("Card manager initialized\n");
    return 0;
}

/****************************************************************************
 * Name: card_manager_load_topic
 *
 * Description:
 *   加载指定主题
 *
 ****************************************************************************/

int card_manager_load_topic(const char *topic)
{
    int ret;

    if (topic == NULL)
    {
        return -EINVAL;
    }

    /* 尝试从文件加载 */

    ret = load_topic_from_file(topic, &g_current_topic);
    if (ret == 0)
    {
        return 0;
    }

    /* 文件不存在，初始化新主题 */

    memset(&g_current_topic, 0, sizeof(g_current_topic));
    strncpy(g_current_topic.topic, topic, CARD_MAX_TOPIC_LEN - 1);
    g_current_topic.card_count = 0;
    g_current_topic.current_index = 0;
    g_current_topic.outline_generated = false;

    printf("Initialized new topic: %s\n", topic);
    return 0;
}

/****************************************************************************
 * Name: card_manager_save_topic
 *
 * Description:
 *   保存当前主题
 *
 ****************************************************************************/

int card_manager_save_topic(const char *topic)
{
    if (topic == NULL)
    {
        return -EINVAL;
    }

    return save_topic_to_file(topic, &g_current_topic);
}

/****************************************************************************
 * Name: card_manager_get_current_card
 *
 * Description:
 *   获取当前卡片
 *
 ****************************************************************************/

knowledge_card_t* card_manager_get_current_card(void)
{
    if (g_current_topic.card_count == 0)
    {
        return NULL;
    }

    if (g_current_topic.current_index < 0 ||
        g_current_topic.current_index >= g_current_topic.card_count)
    {
        return NULL;
    }

    return &g_current_topic.cards[g_current_topic.current_index];
}

/****************************************************************************
 * Name: card_manager_get_next_card
 *
 * Description:
 *   获取下一张卡片
 *
 ****************************************************************************/

knowledge_card_t* card_manager_get_next_card(void)
{
    if (g_current_topic.card_count == 0)
    {
        return NULL;
    }

    g_current_topic.current_index++;
    if (g_current_topic.current_index >= g_current_topic.card_count)
    {
        g_current_topic.current_index = 0; /* 循环到第一张 */
    }

    return &g_current_topic.cards[g_current_topic.current_index];
}

/****************************************************************************
 * Name: card_manager_get_prev_card
 *
 * Description:
 *   获取上一张卡片
 *
 ****************************************************************************/

knowledge_card_t* card_manager_get_prev_card(void)
{
    if (g_current_topic.card_count == 0)
    {
        return NULL;
    }

    g_current_topic.current_index--;
    if (g_current_topic.current_index < 0)
    {
        g_current_topic.current_index = g_current_topic.card_count - 1; /* 循环到最后一张 */
    }

    return &g_current_topic.cards[g_current_topic.current_index];
}

/****************************************************************************
 * Name: card_manager_get_current_index
 *
 * Description:
 *   获取当前卡片索引
 *
 ****************************************************************************/

int card_manager_get_current_index(void)
{
    return g_current_topic.current_index;
}

/****************************************************************************
 * Name: card_manager_get_total_cards
 *
 * Description:
 *   获取总卡片数
 *
 ****************************************************************************/

int card_manager_get_total_cards(void)
{
    return g_current_topic.card_count;
}
