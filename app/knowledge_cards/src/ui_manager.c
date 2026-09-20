/****************************************************************************
 * packages/demos/knowledge_cards/src/ui_manager.c
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
#include <lvgl/lvgl.h>

#include "knowledge_cards.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define SCREEN_WIDTH    (lv_obj_get_width(lv_scr_act()))
#define SCREEN_HEIGHT   (lv_obj_get_height(lv_scr_act()))

#define CARD_MARGIN     20
#define CARD_PADDING    15
#define BUTTON_SIZE     40
#define STATUS_HEIGHT   30

/* 颜色定义 */

#define COLOR_BG        lv_color_hex(0x1a1a2e)
#define COLOR_CARD      lv_color_hex(0x16213e)
#define COLOR_TITLE     lv_color_hex(0xe94560)
#define COLOR_CONTENT   lv_color_hex(0xffffff)
#define COLOR_STATUS    lv_color_hex(0x0f3460)
#define COLOR_BUTTON    lv_color_hex(0x533483)
#define COLOR_ACCENT    lv_color_hex(0x00d2d3)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static app_context_t *g_app_ctx = NULL;

/* 动画相关 */

static lv_anim_t g_scroll_anim;
static int g_scroll_offset = 0;
static bool g_is_scrolling = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: create_background
 *
 * Description:
 *   创建背景
 *
 ****************************************************************************/

static lv_obj_t* create_background(lv_obj_t *parent)
{
    lv_obj_t *bg = lv_obj_create(parent);
    lv_obj_set_size(bg, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(bg, COLOR_BG, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(bg);

    return bg;
}

/****************************************************************************
 * Name: create_title_label
 *
 * Description:
 *   创建标题标签
 *
 ****************************************************************************/

static lv_obj_t* create_title_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, SCREEN_WIDTH - 2 * CARD_MARGIN);
    lv_obj_set_style_text_color(label, COLOR_TITLE, 0);
    lv_obj_set_style_text_font(label, g_app_ctx->fonts.size_24, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_label_set_text(label, "知识闪卡");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, CARD_MARGIN);

    return label;
}

/****************************************************************************
 * Name: create_card_container
 *
 * Description:
 *   创建卡片容器
 *
 ****************************************************************************/

static lv_obj_t* create_card_container(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card,
                    SCREEN_WIDTH - 2 * CARD_MARGIN,
                    SCREEN_HEIGHT - 2 * CARD_MARGIN - STATUS_HEIGHT - BUTTON_SIZE);
    lv_obj_set_style_bg_color(card, COLOR_CARD, 0);
    lv_obj_set_style_border_color(card, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 15, 0);
    lv_obj_set_style_pad_all(card, CARD_PADDING, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);

    return card;
}

/****************************************************************************
 * Name: create_content_label
 *
 * Description:
 *   创建内容标签
 *
 ****************************************************************************/

static lv_obj_t* create_content_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, lv_obj_get_width(parent) - 2 * CARD_PADDING);
    lv_obj_set_style_text_color(label, COLOR_CONTENT, 0);
    lv_obj_set_style_text_font(label, g_app_ctx->fonts.size_20, 0);
    lv_obj_set_style_text_line_space(label, 8, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "语音输入或选择主题开始学习...");
    lv_obj_center(label);

    return label;
}

/****************************************************************************
 * Name: create_page_indicator
 *
 * Description:
 *   创建页码指示器
 *
 ****************************************************************************/

static lv_obj_t* create_page_indicator(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, COLOR_STATUS, 0);
    lv_obj_set_style_text_font(label, g_app_ctx->fonts.size_16, 0);
    lv_label_set_text(label, "0/0");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -BUTTON_SIZE - 10);

    return label;
}

/****************************************************************************
 * Name: create_status_label
 *
 * Description:
 *   创建状态标签
 *
 ****************************************************************************/

static lv_obj_t* create_status_label(lv_obj_t *parent)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_width(label, SCREEN_WIDTH - 2 * CARD_MARGIN);
    lv_obj_set_style_text_color(label, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(label, g_app_ctx->fonts.size_16, 0);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "就绪");
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -BUTTON_SIZE - 30);

    return label;
}

/****************************************************************************
 * Name: mic_button_event_cb
 *
 * Description:
 *   麦克风按钮事件回调
 *
 ****************************************************************************/

static void mic_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        if (g_app_ctx->state == UI_STATE_LISTENING)
        {
            /* 停止监听 */

            voice_stop_listening();
            ui_set_state(g_app_ctx, UI_STATE_IDLE);
        }
        else
        {
            /* 开始监听 */

            voice_start_listening();
            ui_set_state(g_app_ctx, UI_STATE_LISTENING);
        }
    }
}

/****************************************************************************
 * Name: prev_button_event_cb
 *
 * Description:
 *   上一张按钮事件回调
 *
 ****************************************************************************/

static void prev_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        knowledge_card_t *card = card_manager_get_prev_card();
        if (card != NULL)
        {
            ui_update_card_display(g_app_ctx);
        }
    }
}

/****************************************************************************
 * Name: next_button_event_cb
 *
 * Description:
 *   下一张按钮事件回调
 *
 ****************************************************************************/

static void next_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        knowledge_card_t *card = card_manager_get_next_card();
        if (card != NULL)
        {
            ui_update_card_display(g_app_ctx);
        }
    }
}

/****************************************************************************
 * Name: create_control_buttons
 *
 * Description:
 *   创建控制按钮
 *
 ****************************************************************************/

static void create_control_buttons(lv_obj_t *parent)
{
    lv_obj_t *btn;
    lv_obj_t *label;

    /* 麦克风按钮（居中） */

    btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BUTTON_SIZE, BUTTON_SIZE);
    lv_obj_set_style_bg_color(btn, COLOR_BUTTON, 0);
    lv_obj_set_style_radius(btn, BUTTON_SIZE / 2, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(btn, mic_button_event_cb, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    g_app_ctx->ui.mic_button = btn;

    /* 上一张按钮（左侧） */

    btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BUTTON_SIZE, BUTTON_SIZE);
    lv_obj_set_style_bg_color(btn, COLOR_BUTTON, 0);
    lv_obj_set_style_radius(btn, BUTTON_SIZE / 2, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_LEFT, CARD_MARGIN, -10);
    lv_obj_add_event_cb(btn, prev_button_event_cb, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_PREV);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    g_app_ctx->ui.prev_button = btn;

    /* 下一张按钮（右侧） */

    btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BUTTON_SIZE, BUTTON_SIZE);
    lv_obj_set_style_bg_color(btn, COLOR_BUTTON, 0);
    lv_obj_set_style_radius(btn, BUTTON_SIZE / 2, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -CARD_MARGIN, -10);
    lv_obj_add_event_cb(btn, next_button_event_cb, LV_EVENT_CLICKED, NULL);

    label = lv_label_create(btn);
    lv_label_set_text(label, LV_SYMBOL_NEXT);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    g_app_ctx->ui.next_button = btn;
}

/****************************************************************************
 * Name: create_loading_spinner
 *
 * Description:
 *   创建加载动画
 *
 ****************************************************************************/

static lv_obj_t* create_loading_spinner(lv_obj_t *parent)
{
    lv_obj_t *spinner = lv_spinner_create(parent, 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_set_style_arc_color(spinner, COLOR_ACCENT, 0);
    lv_obj_center(spinner);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);

    return spinner;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ui_init
 *
 * Description:
 *   初始化 UI
 *
 ****************************************************************************/

int ui_init(app_context_t *ctx)
{
    if (ctx == NULL)
    {
        return -EINVAL;
    }

    g_app_ctx = ctx;

    /* 创建背景 */

    ctx->ui.background = create_background(lv_scr_act());

    /* 创建标题 */

    ctx->ui.title_label = create_title_label(ctx->ui.background);

    /* 创建卡片容器 */

    lv_obj_t *card_container = create_card_container(ctx->ui.background);

    /* 创建内容标签 */

    ctx->ui.content_label = create_content_label(card_container);

    /* 创建页码指示器 */

    ctx->ui.page_indicator = create_page_indicator(ctx->ui.background);

    /* 创建状态标签 */

    ctx->ui.status_label = create_status_label(ctx->ui.background);

    /* 创建控制按钮 */

    create_control_buttons(ctx->ui.background);

    /* 创建加载动画 */

    ctx->ui.loading_spinner = create_loading_spinner(ctx->ui.background);

    printf("UI initialized\n");
    return 0;
}

/****************************************************************************
 * Name: ui_update_card_display
 *
 * Description:
 *   更新卡片显示
 *
 ****************************************************************************/

void ui_update_card_display(app_context_t *ctx)
{
    knowledge_card_t *card;
    char page_text[32];

    if (ctx == NULL)
    {
        return;
    }

    card = card_manager_get_current_card();
    if (card == NULL)
    {
        lv_label_set_text(ctx->ui.content_label, "暂无知识点");
        lv_label_set_text(ctx->ui.page_indicator, "0/0");
        return;
    }

    /* 更新标题 */

    lv_label_set_text(ctx->ui.title_label, card->title);

    /* 更新内容 */

    if (card->is_loaded && strlen(card->content) > 0)
    {
        lv_label_set_text(ctx->ui.content_label, card->content);
    }
    else
    {
        lv_label_set_text(ctx->ui.content_label, "正在加载内容...");
    }

    /* 更新页码 */

    snprintf(page_text, sizeof(page_text), "%d/%d",
             card_manager_get_current_index() + 1,
             card_manager_get_total_cards());
    lv_label_set_text(ctx->ui.page_indicator, page_text);

    /* 更新难度指示 */

    const char *difficulty_icon;
    switch (card->difficulty)
    {
        case 0:
            difficulty_icon = LV_SYMBOL_HOME; /* 基础 */
            break;
        case 1:
            difficulty_icon = LV_SYMBOL_OK;   /* 甜点 */
            break;
        case 2:
            difficulty_icon = LV_SYMBOL_UP;   /* 进阶 */
            break;
        default:
            difficulty_icon = "";
            break;
    }

    /* 可以在标题前添加难度图标 */

    printf("Displaying card: %s\n", card->title);
}

/****************************************************************************
 * Name: ui_show_status
 *
 * Description:
 *   显示状态信息
 *
 ****************************************************************************/

void ui_show_status(app_context_t *ctx, const char *status)
{
    if (ctx == NULL || status == NULL)
    {
        return;
    }

    lv_label_set_text(ctx->ui.status_label, status);
}

/****************************************************************************
 * Name: ui_show_loading
 *
 * Description:
 *   显示/隐藏加载动画
 *
 ****************************************************************************/

void ui_show_loading(app_context_t *ctx, bool show)
{
    if (ctx == NULL)
    {
        return;
    }

    if (show)
    {
        lv_obj_clear_flag(ctx->ui.loading_spinner, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(ctx->ui.loading_spinner, LV_OBJ_FLAG_HIDDEN);
    }
}

/****************************************************************************
 * Name: ui_set_state
 *
 * Description:
 *   设置 UI 状态
 *
 ****************************************************************************/

void ui_set_state(app_context_t *ctx, ui_state_t state)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->state = state;

    switch (state)
    {
        case UI_STATE_IDLE:
            ui_show_status(ctx, "就绪 - 按麦克风输入主题");
            lv_obj_set_style_bg_color(ctx->ui.mic_button, COLOR_BUTTON, 0);
            break;

        case UI_STATE_LISTENING:
            ui_show_status(ctx, "正在聆听...");
            lv_obj_set_style_bg_color(ctx->ui.mic_button, COLOR_TITLE, 0);
            break;

        case UI_STATE_GENERATING:
            ui_show_status(ctx, "正在生成知识...");
            ui_show_loading(ctx, true);
            break;

        case UI_STATE_DISPLAYING:
            ui_show_status(ctx, "学习中 - 左右切换卡片");
            ui_show_loading(ctx, false);
            break;

        case UI_STATE_ERROR:
            ui_show_status(ctx, "出错了，请重试");
            lv_obj_set_style_bg_color(ctx->ui.mic_button, COLOR_BUTTON, 0);
            break;
    }
}
