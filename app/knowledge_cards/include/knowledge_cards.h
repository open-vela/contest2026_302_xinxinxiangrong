/****************************************************************************
 * packages/demos/knowledge_cards/include/knowledge_cards.h
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

#ifndef __PACKAGES_DEMOS_KNOWLEDGE_CARDS_H
#define __PACKAGES_DEMOS_KNOWLEDGE_CARDS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <lvgl/lvgl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* 知识卡片配置 */

#define CARD_MAX_TITLE_LEN      64
#define CARD_MAX_CONTENT_LEN    512
#define CARD_MAX_CARDS          50
#define CARD_MAX_TOPIC_LEN      32

/* LLM API 配置 */

#define LLM_ENDPOINT_MAX_LEN   256
#define LLM_API_KEY_MAX_LEN    128
#define LLM_RESPONSE_MAX_LEN   2048

/* UI 配置 */

#define UI_ANIM_TIME            300
#define UI_SCROLL_SPEED         50  /* ms per character */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* 知识卡片结构体 */

typedef struct knowledge_card_s
{
    char title[CARD_MAX_TITLE_LEN];       /* 卡片标题 */
    char content[CARD_MAX_CONTENT_LEN];   /* 卡片内容 */
    int difficulty;                        /* 难度：0=基础, 1=甜点, 2=进阶 */
    bool is_loaded;                        /* 是否已加载 */
} knowledge_card_t;

/* 知识主题结构体 */

typedef struct knowledge_topic_s
{
    char topic[CARD_MAX_TOPIC_LEN];       /* 主题名称，如 "CSAPP" */
    knowledge_card_t cards[CARD_MAX_CARDS]; /* 知识卡片数组 */
    int card_count;                        /* 当前卡片数量 */
    int current_index;                     /* 当前显示的卡片索引 */
    bool outline_generated;                /* 大纲是否已生成 */
} knowledge_topic_t;

/* LLM 客户端配置 */

typedef struct llm_config_s
{
    char endpoint[LLM_ENDPOINT_MAX_LEN];  /* API 端点 */
    char api_key[LLM_API_KEY_MAX_LEN];    /* API Key */
    char model[64];                        /* 模型名称 */
    int max_tokens;                        /* 最大 token 数 */
    int timeout;                           /* 超时时间(秒) */
} llm_config_t;

/* UI 状态 */

typedef enum ui_state_e
{
    UI_STATE_IDLE,          /* 空闲状态，等待输入 */
    UI_STATE_LISTENING,     /* 语音监听中 */
    UI_STATE_GENERATING,    /* 知识生成中 */
    UI_STATE_DISPLAYING,    /* 显示卡片中 */
    UI_STATE_ERROR          /* 错误状态 */
} ui_state_t;

/* 主应用上下文 */

typedef struct app_context_s
{
    /* 知识数据 */

    knowledge_topic_t current_topic;
    llm_config_t llm_config;

    /* UI 组件 */

    struct
    {
        lv_obj_t *background;
        lv_obj_t *title_label;
        lv_obj_t *content_label;
        lv_obj_t *page_indicator;
        lv_obj_t *status_label;
        lv_obj_t *mic_button;
        lv_obj_t *prev_button;
        lv_obj_t *next_button;
        lv_obj_t *loading_spinner;
    } ui;

    /* 字体 */

    struct
    {
        lv_font_t *size_16;
        lv_font_t *size_20;
        lv_font_t *size_24;
        lv_font_t *size_32;
    } fonts;

    /* 状态 */

    ui_state_t state;
    bool wifi_connected;
    bool is_speaking;      /* 是否正在语音输入 */

} app_context_t;

/****************************************************************************
 * Public Types - Callbacks
 ****************************************************************************/

/* 语音识别结果回调函数类型 */

typedef void (*voice_result_callback_t)(const char *text, void *userdata);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* LLM 客户端函数 */

int llm_init(llm_config_t *config);
int llm_generate_outline(const char *topic, knowledge_topic_t *result);
int llm_generate_card_content(knowledge_card_t *card);
void llm_cleanup(void);

/* 卡片管理函数 */

int card_manager_init(void);
int card_manager_load_topic(const char *topic);
int card_manager_save_topic(const char *topic);
knowledge_card_t* card_manager_get_current_card(void);
knowledge_card_t* card_manager_get_next_card(void);
knowledge_card_t* card_manager_get_prev_card(void);
int card_manager_get_current_index(void);
int card_manager_get_total_cards(void);

/* UI 函数 */

int ui_init(app_context_t *ctx);
void ui_update_card_display(app_context_t *ctx);
void ui_show_status(app_context_t *ctx, const char *status);
void ui_show_loading(app_context_t *ctx, bool show);
void ui_set_state(app_context_t *ctx, ui_state_t state);

/* 语音识别函数 */

int voice_init(void);
int voice_start_listening(void);
int voice_stop_listening(void);
bool voice_is_listening(void);
int voice_register_callback(voice_result_callback_t callback, void *userdata);
const char* voice_get_result(void);
void voice_cleanup(void);

/* WiFi 函数 */

int wifi_check_connection(void);
int wifi_connect(void);
int wifi_wait_for_connection(int timeout_ms);

#endif /* __PACKAGES_DEMOS_KNOWLEDGE_CARDS_H */
