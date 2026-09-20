/****************************************************************************
 * packages/demos/knowledge_cards/src/knowledge_cards_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>

#include "kc_voice.h"
#include "kc_deck.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/boardctl.h>
#include <syslog.h>
#include <unistd.h>

#include <lvgl/lvgl.h>
#include <lvgl/src/drivers/nuttx/lv_nuttx_touchscreen.h>

LV_FONT_DECLARE(kc_font_20);
LV_FONT_DECLARE(kc_font_24);
LV_FONT_DECLARE(kc_font_cjk_16);

#define KC_REVEAL_PERIOD_MS 1300
#define KC_TEXT_BUFFER_SIZE  512

#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
#  define KC_NEED_BOARDINIT 1
#endif

static const struct kc_card_s g_csapp_cards[] =
{
  {
    "计算机系统",
    "程序最初只是比特与字节。\n"
    "编译器把源代码转换成机器指令。\n"
    "硬件执行指令并搬运数据。\n"
    "操作系统负责管理共享资源。"
  },
  {
    "数据表示",
    "比特只有经过解释才有含义。\n"
    "整数使用二进制和补码表示。\n"
    "浮点数用精度换取更大的范围。\n"
    "溢出是程序必须处理的真实行为。"
  },
  {
    "编译过程",
    "预处理器展开头文件和宏。\n"
    "编译器生成汇编指令。\n"
    "汇编器生成可重定位目标文件。\n"
    "链接器解析符号并生成可执行文件。"
  },
  {
    "存储层次",
    "寄存器容量最小，速度最快。\n"
    "缓存把常用数据放在处理器附近。\n"
    "主存容量更大，但速度更慢。\n"
    "局部性让整个层次高效工作。"
  },
  {
    "虚拟内存",
    "每个进程拥有独立的地址空间。\n"
    "页表把虚拟页映射到物理内存。\n"
    "快表缓存最近的地址转换。\n"
    "缺失的页面可以从存储设备载入。"
  },
  {
    "并发",
    "线程会交错访问共享状态。\n"
    "数据竞争让结果依赖执行时序。\n"
    "锁可以保护临界区。\n"
    "正确同步还要避免死锁。"
  }
};

static struct kc_deck_s g_deck;

static lv_obj_t *g_screen;
static lv_obj_t *g_topic_label;
static lv_obj_t *g_title_label;
static lv_obj_t *g_content_label;
static lv_obj_t *g_page_label;
static lv_obj_t *g_state_label;
static lv_obj_t *g_mic_button;
static lv_timer_t *g_reveal_timer;
static lv_timer_t *g_voice_timer;
static size_t g_card_index;
static size_t g_visible_lines;
static char g_visible_text[KC_TEXT_BUFFER_SIZE];
static uint32_t g_voice_generation = UINT32_MAX;

static bool kc_topic_is_csapp(const char *topic);

static bool kc_load_topic(const char *topic)
{
  if (kc_topic_is_csapp(topic))
    {
      memcpy(g_deck.cards, g_csapp_cards, sizeof(g_csapp_cards));
      g_deck.count = sizeof(g_csapp_cards) / sizeof(g_csapp_cards[0]);
      snprintf(g_deck.topic, sizeof(g_deck.topic), "CSAPP");
      return true;
    }

  return kc_deck_load("/data/knowledge", topic, &g_deck) == 0;
}

static int kc_attach_touchscreen(void)
{
  lv_indev_t *indev = lv_indev_get_next(NULL);
  lv_timer_t *timer;

  if (indev == NULL)
    {
      indev = lv_nuttx_touchscreen_create("/dev/input0");
      if (indev == NULL)
        {
          syslog(LOG_ERR,
                 "KNOWLEDGE_CARDS: cannot attach touchscreen /dev/input0\n");
          return -1;
        }

      syslog(LOG_INFO,
             "KNOWLEDGE_CARDS: attached touchscreen /dev/input0\n");
    }
  else
    {
      syslog(LOG_INFO,
             "KNOWLEDGE_CARDS: reusing touchscreen indev=%p\n", indev);
    }

  lv_indev_set_display(indev, lv_display_get_default());
  lv_indev_set_mode(indev, LV_INDEV_MODE_TIMER);
  timer = lv_indev_get_read_timer(indev);
  if (timer != NULL)
    {
      lv_timer_set_period(timer, 20);
      lv_timer_resume(timer);
    }

  syslog(LOG_INFO,
         "KNOWLEDGE_CARDS: touch ready indev=%p timer=%p\n", indev, timer);
  return 0;
}

static size_t kc_line_count(const char *text)
{
  size_t count = 1;

  while (*text != '\0')
    {
      if (*text++ == '\n')
        {
          count++;
        }
    }

  return count;
}

static void kc_set_visible_lines(const char *text, size_t lines)
{
  size_t copied = 0;
  size_t seen = 1;

  while (text[copied] != '\0' && copied < sizeof(g_visible_text) - 1)
    {
      if (text[copied] == '\n' && seen >= lines)
        {
          break;
        }

      if (text[copied] == '\n')
        {
          seen++;
        }

      g_visible_text[copied] = text[copied];
      copied++;
    }

  g_visible_text[copied] = '\0';
  lv_label_set_text(g_content_label, g_visible_text);
}

static void kc_show_card(void)
{
  const struct kc_card_s *card = &g_deck.cards[g_card_index];
  char page[24];

  g_visible_lines = 1;
  lv_label_set_text(g_title_label, card->title);
  kc_set_visible_lines(card->content, g_visible_lines);
  snprintf(page, sizeof(page), "%u / %u", (unsigned int)g_card_index + 1,
           (unsigned int)g_deck.count);
  lv_label_set_text(g_page_label, page);
  lv_label_set_text(g_state_label, "逐行阅读");

  if (g_reveal_timer != NULL)
    {
      lv_timer_reset(g_reveal_timer);
      lv_timer_resume(g_reveal_timer);
    }

  syslog(LOG_INFO, "KNOWLEDGE_CARDS: card %u/%u: %s\n",
         (unsigned int)g_card_index + 1, (unsigned int)g_deck.count,
         card->title);
}

static void kc_reveal_cb(lv_timer_t *timer)
{
  const char *content = g_deck.cards[g_card_index].content;
  size_t total = kc_line_count(content);

  if (g_visible_lines < total)
    {
      g_visible_lines++;
      kc_set_visible_lines(content, g_visible_lines);
    }

  if (g_visible_lines >= total)
    {
      lv_label_set_text(g_state_label, "已展开");
      lv_timer_pause(timer);
    }
}

static void kc_next(void)
{
  g_card_index = (g_card_index + 1) % g_deck.count;
  kc_show_card();
}

static void kc_previous(void)
{
  g_card_index = (g_card_index + g_deck.count - 1) % g_deck.count;
  kc_show_card();
}

static void kc_next_event(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED)
    {
      kc_next();
    }
}

static void kc_previous_event(lv_event_t *event)
{
  if (lv_event_get_code(event) == LV_EVENT_CLICKED)
    {
      kc_previous();
    }
}

static bool kc_topic_is_csapp(const char *topic)
{
  char normalized[64];
  size_t output = 0;

  if (strstr(topic, "计算机系统") != NULL ||
      strstr(topic, "深入理解计算机系统") != NULL)
    {
      return true;
    }

  while (*topic != '\0' && output < sizeof(normalized) - 1)
    {
      char ch = *topic++;

      if (ch >= 'a' && ch <= 'z')
        {
          ch = (char)(ch - 'a' + 'A');
        }

      if ((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
        {
          normalized[output++] = ch;
        }
    }

  normalized[output] = '\0';
  return strstr(normalized, "CSAPP") != NULL;
}

static void kc_set_mic_color(uint32_t color)
{
  if (g_mic_button != NULL)
    {
      lv_obj_set_style_bg_color(g_mic_button, lv_color_hex(color), 0);
    }
}

static void kc_voice_poll_cb(lv_timer_t *timer)
{
  struct kc_voice_snapshot_s snapshot;

  (void)timer;
  kc_voice_get_snapshot(&snapshot);
  if (snapshot.generation == g_voice_generation)
    {
      return;
    }

  g_voice_generation = snapshot.generation;
  switch (snapshot.state)
    {
      case KC_VOICE_CONFIG_REQUIRED:
        lv_label_set_text(g_state_label, "请配置语音服务");
        kc_set_mic_color(0x57615d);
        break;

      case KC_VOICE_IDLE:
        lv_label_set_text(g_state_label, "按住说出学习主题");
        kc_set_mic_color(0x2c7a62);
        break;

      case KC_VOICE_STARTING:
        lv_label_set_text(g_state_label, "正在打开麦克风");
        kc_set_mic_color(0xb7791f);
        break;

      case KC_VOICE_RECORDING:
        lv_label_set_text(g_state_label, "正在聆听，松开识别");
        kc_set_mic_color(0xc2413b);
        break;

      case KC_VOICE_RECOGNIZING:
        lv_label_set_text(g_state_label, "正在识别");
        kc_set_mic_color(0xb7791f);
        break;

      case KC_VOICE_RESULT:
        kc_set_mic_color(0x2c7a62);
        if (kc_load_topic(snapshot.text))
          {
            lv_label_set_text_fmt(g_topic_label, "知识闪卡 / %s", g_deck.topic);
            g_card_index = 0;
            kc_show_card();
            lv_label_set_text(g_state_label, "识别成功");
          }
        else
          {
            lv_label_set_text(g_state_label, "该主题尚未缓存");
          }
        break;

      case KC_VOICE_ERROR:
        if (snapshot.error == -ENODATA)
          {
            lv_label_set_text(g_state_label, "没有听清，请重试");
          }
        else if (snapshot.error == -ENODEV)
          {
            lv_label_set_text(g_state_label, "麦克风不可用");
          }
        else
          {
            lv_label_set_text(g_state_label, "识别失败，请重试");
          }
        kc_set_mic_color(0x8f3b36);
        break;

      default:
        break;
    }
}

static void kc_mic_event(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  int ret;

  if (code == LV_EVENT_PRESSED)
    {
      if (g_reveal_timer != NULL)
        {
          lv_timer_pause(g_reveal_timer);
        }

      ret = kc_voice_start_async();
      if (ret < 0 && ret != -EBUSY)
        {
          lv_label_set_text(g_state_label, "语音服务未就绪");
        }
    }
  else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST)
    {
      ret = kc_voice_stop_async();
      if (ret < 0 && ret != -EINVAL)
        {
          lv_label_set_text(g_state_label, "无法结束录音");
        }
    }
}

static void kc_card_event(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_CLICKED)
    {
      const char *content = g_deck.cards[g_card_index].content;

      g_visible_lines = kc_line_count(content);
      kc_set_visible_lines(content, g_visible_lines);
      lv_label_set_text(g_state_label, "已展开");
      lv_timer_pause(g_reveal_timer);
    }
  else if (code == LV_EVENT_GESTURE)
    {
      lv_dir_t direction = lv_indev_get_gesture_dir(lv_indev_get_act());

      if (direction == LV_DIR_LEFT)
        {
          kc_next();
        }
      else if (direction == LV_DIR_RIGHT)
        {
          kc_previous();
        }
    }
}

static lv_obj_t *kc_create_nav_button(lv_obj_t *parent, lv_align_t align,
                                      int32_t x_offset, const char *symbol,
                                      lv_event_cb_t callback)
{
  lv_obj_t *button = lv_button_create(parent);
  lv_obj_t *label;

  lv_obj_set_size(button, 48, 48);
  lv_obj_align(button, align, x_offset, -51);
  lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x245f4f), 0);
  lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
  lv_obj_set_style_shadow_width(button, 0, 0);
  lv_obj_add_event_cb(button, callback, LV_EVENT_CLICKED, NULL);

  label = lv_label_create(button);
  lv_label_set_text(label, symbol);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_center(label);
  return button;
}

static int kc_create_ui(const char *topic)
{
  lv_obj_t *surface;
  lv_obj_t *card;
  lv_obj_t *divider;
  lv_obj_t *mic_label;

  g_screen = lv_screen_active();
  if (g_screen == NULL || lv_display_get_default() == NULL)
    {
      return -1;
    }

  if (kc_attach_touchscreen() < 0)
    {
      return -1;
    }

  lv_obj_clean(g_screen);
  /* Keep the square framebuffer corners black and draw the application on a
   * real circular surface.  The RM69330 panel is 454x454, so a 424px surface
   * leaves a 15px safety margin around the physical circular edge. */
  lv_obj_set_style_bg_color(g_screen, lv_color_hex(0x050807), 0);
  lv_obj_set_style_bg_opa(g_screen, LV_OPA_COVER, 0);
  lv_obj_clear_flag(g_screen, LV_OBJ_FLAG_SCROLLABLE);

  surface = lv_obj_create(g_screen);
  lv_obj_set_size(surface, 424, 424);
  lv_obj_center(surface);
  lv_obj_set_style_radius(surface, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(surface, lv_color_hex(0x101513), 0);
  lv_obj_set_style_bg_opa(surface, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(surface, 2, 0);
  lv_obj_set_style_border_color(surface, lv_color_hex(0x2d5e50), 0);
  lv_obj_set_style_shadow_width(surface, 0, 0);
  lv_obj_clear_flag(surface, LV_OBJ_FLAG_SCROLLABLE);

  g_topic_label = lv_label_create(surface);
  lv_label_set_text_fmt(g_topic_label, "知识闪卡 / %s", topic);
  lv_obj_set_width(g_topic_label, 290);
  lv_label_set_long_mode(g_topic_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(g_topic_label, &kc_font_cjk_16, 0);
  lv_obj_set_style_text_color(g_topic_label, lv_color_hex(0x7ed9b7), 0);
  lv_obj_set_style_text_align(g_topic_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(g_topic_label, LV_ALIGN_TOP_MID, 0, 22);

  g_state_label = lv_label_create(surface);
  lv_label_set_text(g_state_label, "离线卡组");
  lv_obj_set_width(g_state_label, 320);
  lv_obj_set_style_text_font(g_state_label, &kc_font_20, 0);
  lv_obj_set_style_text_color(g_state_label, lv_color_hex(0x9aa6a0), 0);
  lv_obj_set_style_text_align(g_state_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(g_state_label, LV_ALIGN_TOP_MID, 0, 51);

  card = lv_obj_create(surface);
  lv_obj_set_size(card, 336, 238);
  lv_obj_align(card, LV_ALIGN_CENTER, 0, -18);
  lv_obj_set_style_radius(card, 26, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x19211e), 0);
  lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x33433d), 0);
  lv_obj_set_style_pad_all(card, 19, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(card, kc_card_event, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(card, kc_card_event, LV_EVENT_GESTURE, NULL);

  g_title_label = lv_label_create(card);
  lv_obj_set_width(g_title_label, 296);
  lv_label_set_long_mode(g_title_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_font(g_title_label, &kc_font_cjk_16, 0);
  lv_obj_set_style_text_color(g_title_label, lv_color_hex(0xffffff), 0);
  lv_obj_align(g_title_label, LV_ALIGN_TOP_LEFT, 0, 0);

  divider = lv_obj_create(card);
  lv_obj_set_size(divider, 296, 2);
  lv_obj_set_style_radius(divider, 0, 0);
  lv_obj_set_style_border_width(divider, 0, 0);
  lv_obj_set_style_bg_color(divider, lv_color_hex(0x245f4f), 0);
  lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, 0);
  lv_obj_align(divider, LV_ALIGN_TOP_LEFT, 0, 40);

  g_content_label = lv_label_create(card);
  lv_obj_set_size(g_content_label, 296, 157);
  lv_label_set_long_mode(g_content_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(g_content_label, &kc_font_cjk_16, 0);
  lv_obj_set_style_text_color(g_content_label, lv_color_hex(0xdce7e2), 0);
  lv_obj_set_style_text_line_space(g_content_label, 6, 0);
  lv_obj_align(g_content_label, LV_ALIGN_TOP_LEFT, 0, 53);

  g_page_label = lv_label_create(surface);
  lv_label_set_text(g_page_label, "1 / 6");
  lv_obj_set_width(g_page_label, 110);
  lv_obj_set_style_text_font(g_page_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(g_page_label, lv_color_hex(0xb9c5bf), 0);
  lv_obj_set_style_text_align(g_page_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(g_page_label, LV_ALIGN_BOTTOM_MID, 0, -99);

  kc_create_nav_button(surface, LV_ALIGN_BOTTOM_LEFT, 88, LV_SYMBOL_LEFT,
                       kc_previous_event);
  kc_create_nav_button(surface, LV_ALIGN_BOTTOM_RIGHT, -88, LV_SYMBOL_RIGHT,
                       kc_next_event);

  g_mic_button = lv_button_create(surface);
  lv_obj_set_size(g_mic_button, 72, 72);
  lv_obj_align(g_mic_button, LV_ALIGN_BOTTOM_MID, 0, -24);
  lv_obj_set_style_radius(g_mic_button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(g_mic_button, lv_color_hex(0x2c7a62), 0);
  lv_obj_set_style_bg_opa(g_mic_button, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(g_mic_button, 2, 0);
  lv_obj_set_style_border_color(g_mic_button, lv_color_hex(0x8edbc0), 0);
  lv_obj_set_style_shadow_width(g_mic_button, 0, 0);
  lv_obj_add_event_cb(g_mic_button, kc_mic_event, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(g_mic_button, kc_mic_event, LV_EVENT_RELEASED, NULL);
  lv_obj_add_event_cb(g_mic_button, kc_mic_event, LV_EVENT_PRESS_LOST, NULL);

  mic_label = lv_label_create(g_mic_button);
  lv_label_set_text(mic_label, LV_SYMBOL_AUDIO);
  lv_obj_set_style_text_font(mic_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(mic_label, lv_color_white(), 0);
  lv_obj_center(mic_label);

  g_reveal_timer = lv_timer_create(kc_reveal_cb, KC_REVEAL_PERIOD_MS, NULL);
  if (g_reveal_timer == NULL)
    {
      return -1;
    }

  g_voice_timer = lv_timer_create(kc_voice_poll_cb, 100, NULL);
  if (g_voice_timer == NULL)
    {
      return -1;
    }

  g_card_index = 0;
  kc_show_card();
  kc_voice_poll_cb(g_voice_timer);
  lv_obj_update_layout(g_screen);
  lv_refr_now(lv_display_get_default());
  return 0;
}

int main(int argc, FAR char *argv[])
{
  const char *topic = argc > 1 ? argv[1] : "CSAPP";
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;
  int ret;

  if (argc > 1 && strcmp(argv[1], "--set-asr") == 0)
    {
      if (argc < 4)
        {
          fprintf(stderr,
                  "usage: knowledge_cards --set-asr APP_ID TOKEN [CLUSTER]\n");
          return 1;
        }

      ret = kc_voice_configure(argv[2], argv[3],
                               argc > 4 ? argv[4] : NULL);
      if (ret < 0)
        {
          fprintf(stderr, "knowledge_cards: ASR config failed: %d\n", ret);
          return 1;
        }

      printf("knowledge_cards: ASR credentials configured\n");
      return 0;
    }

  if (argc > 1 && strcmp(argv[1], "--asr-status") == 0)
    {
      printf("knowledge_cards: ASR %s\n",
             kc_voice_is_configured() ? "configured" : "not configured");
      return kc_voice_is_configured() ? 0 : 1;
    }

  syslog(LOG_INFO, "KNOWLEDGE_CARDS: start topic=%s\n", topic);
  printf("KNOWLEDGE_CARDS: start topic=%s\n", topic);

  if (!kc_load_topic(topic))
    {
      fprintf(stderr, "knowledge_cards: no valid local deck for %s\n", topic);
      return 1;
    }

  topic = g_deck.topic;

  ret = kc_voice_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "KNOWLEDGE_CARDS: voice init failed: %d\n", ret);
    }

  if (lv_is_initialized())
    {
      if (kc_create_ui(topic) < 0)
        {
          syslog(LOG_ERR, "KNOWLEDGE_CARDS: existing display unavailable\n");
          return 1;
        }

      syslog(LOG_INFO, "KNOWLEDGE_CARDS: attached to board LVGL\n");
      return 0;
    }

#ifdef KC_NEED_BOARDINIT
  boardctl(BOARDIOC_INIT, 0);
#endif

  lv_init();
  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

  lv_nuttx_init(&info, &result);
  if (result.disp == NULL)
    {
      syslog(LOG_ERR, "KNOWLEDGE_CARDS: display initialization failed\n");
      lv_deinit();
      return 1;
    }

  if (kc_create_ui(topic) < 0)
    {
      syslog(LOG_ERR, "KNOWLEDGE_CARDS: UI initialization failed\n");
      lv_nuttx_deinit(&result);
      lv_deinit();
      return 1;
    }

  while (1)
    {
      uint32_t idle = lv_timer_handler();
      usleep((idle ? idle : 1) * 1000);
    }

  return 0;
}
