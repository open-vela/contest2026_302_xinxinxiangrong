#!/bin/bash

############################################################################
# packages/demos/knowledge_cards/tests/run_tests.sh
#
# 测试运行脚本
#
############################################################################

set -e

# 颜色定义
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# 输出函数
info() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 打印横幅
echo "╔════════════════════════════════════════╗"
echo "║    Knowledge Cards 测试套件           ║"
echo "╚════════════════════════════════════════╝"
echo ""

# 检查依赖
info "检查依赖..."

if ! command -v gcc &> /dev/null; then
    error "gcc 未安装"
    exit 1
fi

# 创建 mock 头文件
info "创建 NuttX mock 头文件..."
mkdir -p /tmp/nuttx /tmp/lvgl /tmp/media

# nuttx/config.h
cat > /tmp/nuttx/config.h << 'EOF'
#ifndef __NUTTX_CONFIG_H
#define __NUTTX_CONFIG_H

/* Mock NuttX config for testing */

#define CONFIG_BES 1
#define CONFIG_ARCH_CHIP_BES2800BP 1
#define CONFIG_BES_BES2800BP 1

/* Media config */
#define CONFIG_MEDIA 1
#define CONFIG_AUDIO 1

/* Network config */
#define CONFIG_NET 1
#define CONFIG_NET_TCP 1
#define CONFIG_NET_UDP 1
#define CONFIG_NETUTILS_CJSON 1

/* File system */
#define CONFIG_FS_ROMFS 1

/* LVGL */
#define CONFIG_LV_USE_FONT_PLACEHOLDER 1

/* Debug */
#define CONFIG_DEBUG_ERROR 1
#define CONFIG_DEBUG_WARN 1

/* Knowledge Cards */
#define CONFIG_KNOWLEDGE_CARDS_VOICE_STREAM "default"

#endif /* __NUTTX_CONFIG_H */
EOF

# lvgl/lvgl.h
cat > /tmp/lvgl/lvgl.h << 'EOF'
#ifndef __LVGL_H
#define __LVGL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* LVGL types */

typedef int16_t lv_coord_t;
typedef uint8_t lv_opa_t;

typedef enum {
    LV_OPA_TRANSP = 0,
    LV_OPA_10     = 25,
    LV_OPA_20     = 51,
    LV_OPA_30     = 76,
    LV_OPA_40     = 102,
    LV_OPA_50     = 127,
    LV_OPA_60     = 153,
    LV_OPA_70     = 178,
    LV_OPA_80     = 204,
    LV_OPA_90     = 229,
    LV_OPA_COVER  = 255,
} lv_opa_values_t;

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
} lv_color_t;

typedef struct {
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;

typedef struct {
    lv_coord_t x1;
    lv_coord_t y1;
    lv_coord_t x2;
    lv_coord_t y2;
} lv_area_t;

typedef void* lv_obj_t;
typedef void* lv_style_t;
typedef void* lv_font_t;
typedef void* lv_anim_t;
typedef void* lv_group_t;
typedef void* lv_indev_t;

typedef enum {
    LV_ALIGN_DEFAULT = 0,
    LV_ALIGN_CENTER,
    LV_ALIGN_TOP_MID,
    LV_ALIGN_BOTTOM_MID,
    LV_ALIGN_LEFT_MID,
    LV_ALIGN_RIGHT_MID,
} lv_align_t;

typedef enum {
    LV_DIR_NONE = 0,
    LV_DIR_LEFT = (1 << 0),
    LV_DIR_RIGHT = (1 << 1),
    LV_DIR_TOP = (1 << 2),
    LV_DIR_BOTTOM = (1 << 3),
    LV_DIR_HOR = LV_DIR_LEFT | LV_DIR_RIGHT,
    LV_DIR_VER = LV_DIR_TOP | LV_DIR_BOTTOM,
    LV_DIR_ALL = LV_DIR_HOR | LV_DIR_VER,
} lv_dir_t;

/* Color macros */

static inline lv_color_t lv_color_hex(uint32_t c)
{
    lv_color_t color;
    color.red = (c >> 16) & 0xFF;
    color.green = (c >> 8) & 0xFF;
    color.blue = c & 0xFF;
    return color;
}

static inline lv_color_t lv_color_make(uint8_t r, uint8_t g, uint8_t b)
{
    lv_color_t color;
    color.red = r;
    color.green = g;
    color.blue = b;
    return color;
}

/* Dummy function stubs */

static inline lv_obj_t* lv_obj_create(lv_obj_t *parent) { (void)parent; return NULL; }
static inline lv_obj_t* lv_label_create(lv_obj_t *parent) { (void)parent; return NULL; }
static inline lv_obj_t* lv_btn_create(lv_obj_t *parent) { (void)parent; return NULL; }
static inline lv_obj_t* lv_spinner_create(lv_obj_t *parent, uint32_t time, uint32_t angle) { (void)parent; (void)time; (void)angle; return NULL; }

static inline void lv_obj_del(lv_obj_t *obj) { (void)obj; }
static inline void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h) { (void)obj; (void)w; (void)h; }
static inline void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y) { (void)obj; (void)x; (void)y; }
static inline void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) { (void)obj; (void)align; (void)x_ofs; (void)y_ofs; }
static inline void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t color, uint32_t selector) { (void)obj; (void)color; (void)selector; }
static inline void lv_obj_set_style_bg_opa(lv_obj_t *obj, lv_opa_t opa, uint32_t selector) { (void)obj; (void)opa; (void)selector; }
static inline void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t color, uint32_t selector) { (void)obj; (void)color; (void)selector; }
static inline void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *font, uint32_t selector) { (void)obj; (void)font; (void)selector; }
static inline void lv_obj_set_style_text_align(lv_obj_t *obj, uint32_t align, uint32_t selector) { (void)obj; (void)align; (void)selector; }
static inline void lv_obj_set_style_radius(lv_obj_t *obj, lv_coord_t radius, uint32_t selector) { (void)obj; (void)radius; (void)selector; }
static inline void lv_obj_set_style_pad_all(lv_obj_t *obj, lv_coord_t padding, uint32_t selector) { (void)obj; (void)padding; (void)selector; }
static inline void lv_obj_set_style_pad_gap(lv_obj_t *obj, lv_coord_t gap, uint32_t selector) { (void)obj; (void)gap; (void)selector; }
static inline void lv_obj_set_style_border_width(lv_obj_t *obj, lv_coord_t width, uint32_t selector) { (void)obj; (void)width; (void)selector; }
static inline void lv_obj_set_style_border_color(lv_obj_t *obj, lv_color_t color, uint32_t selector) { (void)obj; (void)color; (void)selector; }
static inline void lv_obj_set_style_border_opa(lv_obj_t *obj, lv_opa_t opa, uint32_t selector) { (void)obj; (void)opa; (void)selector; }
static inline void lv_obj_set_style_shadow_width(lv_obj_t *obj, lv_coord_t width, uint32_t selector) { (void)obj; (void)width; (void)selector; }
static inline void lv_obj_set_style_shadow_opa(lv_obj_t *obj, lv_opa_t opa, uint32_t selector) { (void)obj; (void)opa; (void)selector; }
static inline void lv_obj_set_style_shadow_color(lv_obj_t *obj, lv_color_t color, uint32_t selector) { (void)obj; (void)color; (void)selector; }
static inline void lv_obj_set_style_outline_width(lv_obj_t *obj, lv_coord_t width, uint32_t selector) { (void)obj; (void)width; (void)selector; }
static inline void lv_obj_set_style_outline_color(lv_obj_t *obj, lv_color_t color, uint32_t selector) { (void)obj; (void)color; (void)selector; }
static inline void lv_obj_set_style_outline_opa(lv_obj_t *obj, lv_opa_t opa, uint32_t selector) { (void)obj; (void)opa; (void)selector; }
static inline void lv_obj_set_style_outline_pad(lv_obj_t *obj, lv_coord_t padding, uint32_t selector) { (void)obj; (void)padding; (void)selector; }
static inline void lv_obj_set_style_anim_time(lv_obj_t *obj, uint32_t time, uint32_t selector) { (void)obj; (void)time; (void)selector; }
static inline void lv_obj_set_flex_flow(lv_obj_t *obj, uint32_t flow) { (void)obj; (void)flow; }
static inline void lv_obj_set_flex_align(lv_obj_t *obj, uint32_t main_place, uint32_t cross_place, uint32_t track_place) { (void)obj; (void)main_place; (void)cross_place; (void)track_place; }
static inline void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w) { (void)obj; (void)w; }
static inline void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h) { (void)obj; (void)h; }
static inline void lv_obj_clear_flag(lv_obj_t *obj, uint32_t flag) { (void)obj; (void)flag; }
static inline void lv_label_set_text(lv_obj_t *obj, const char *text) { (void)obj; (void)text; }
static inline void lv_label_set_long_mode(lv_obj_t *obj, uint32_t mode) { (void)obj; (void)mode; }
static inline void lv_obj_add_event_cb(lv_obj_t *obj, void *cb, uint32_t event, void *userdata) { (void)obj; (void)cb; (void)event; (void)userdata; }
static inline void lv_scr_load(lv_obj_t *scr) { (void)scr; }
static inline lv_obj_t* lv_scr_act(void) { return NULL; }

/* LVGL constants */

#define LV_PART_MAIN         0x000000
#define LV_STATE_DEFAULT     0x000000
#define LV_STATE_PRESSED     0x000002
#define LV_STATE_FOCUSED     0x000008
#define LV_STATE_CHECKED     0x000010

#define LV_FLEX_FLOW_COLUMN  1
#define LV_FLEX_ALIGN_CENTER 1
#define LV_FLEX_ALIGN_START  0
#define LV_TEXT_ALIGN_CENTER 1

#define LV_LABEL_LONG_WRAP   0
#define LV_LABEL_LONG_DOT    3

#define LV_EVENT_CLICKED     1
#define LV_EVENT_PRESSED     2
#define LV_EVENT_RELEASED    3

#define LV_OBJ_FLAG_CLICKABLE 0x0001
#define LV_OBJ_FLAG_SCROLLABLE 0x0010

/* Flex flow values */
#define LV_FLEX_FLOW_ROW         0
#define LV_FLEX_FLOW_COLUMN      1
#define LV_FLEX_FLOW_ROW_WRAP    2
#define LV_FLEX_FLOW_COLUMN_WRAP 3

#define LV_OBJ_FLAG_HIDDEN 0x0002

/* Font placeholder */
#define LV_FONT_DEFAULT NULL

#endif /* __LVGL_H */
EOF

# media_api.h
cat > /tmp/media_api.h << 'EOF'
#ifndef __MEDIA_API_H
#define __MEDIA_API_H

#include <stdint.h>
#include <stddef.h>

/* Media recorder event types */

#define MEDIA_RECORDER_EVENT_STARTED   1
#define MEDIA_RECORDER_EVENT_STOPPED   2
#define MEDIA_RECORDER_EVENT_ERROR     3

/* Media recorder callback types */

typedef void (*media_uv_recorder_open_cb)(void *cookie, int ret);
typedef void (*media_uv_recorder_prepare_cb)(void *cookie, int ret);
typedef void (*media_uv_recorder_start_cb)(void *cookie, int ret);
typedef void (*media_uv_recorder_event_cb)(void *cookie, int event, int ret, const char *extra);
typedef void (*media_uv_recorder_data_cb)(void *cookie, int ret, void *data, int len);

typedef void* media_recorder_t;

/* Forward declare uv_loop_t */
struct uv_loop_s;
typedef struct uv_loop_s uv_loop_t;

/* Media recorder functions */

media_recorder_t media_uv_recorder_open(uv_loop_t *loop, const char *stream_name,
                                         media_uv_recorder_open_cb callback, void *cookie);
int media_uv_recorder_listen(media_recorder_t recorder, media_uv_recorder_event_cb callback);
int media_uv_recorder_prepare(media_recorder_t recorder, void *reserved, const char *format,
                               media_uv_recorder_prepare_cb callback, void *cookie, void *extra);
int media_uv_recorder_start(media_recorder_t recorder, media_uv_recorder_start_cb callback, void *cookie);
int media_uv_recorder_stop(media_recorder_t recorder, media_uv_recorder_start_cb callback, void *cookie);
int media_uv_recorder_close(media_recorder_t recorder, media_uv_recorder_open_cb callback);

#endif /* __MEDIA_API_H */
EOF

# uv.h
cat > /tmp/uv.h << 'EOF'
#ifndef __UV_H
#define __UV_H

#include <stdint.h>
#include <stddef.h>

/* Basic libuv types */

struct uv_loop_s {
    void *data;
};
typedef struct uv_loop_s uv_loop_t;

typedef struct uv_handle_s {
    void *data;
} uv_handle_t;

typedef struct uv_timer_s {
    uv_handle_t handle;
} uv_timer_t;

typedef struct uv_pipe_s {
    uv_handle_t handle;
} uv_pipe_t;

typedef void (*uv_close_cb)(uv_handle_t *handle);
typedef void (*uv_timer_cb)(uv_timer_t *handle);

/* Simplified loop functions */

uv_loop_t *uv_default_loop(void);
int uv_loop_init(uv_loop_t *loop);
int uv_loop_close(uv_loop_t *loop);
int uv_run(uv_loop_t *loop, int mode);

/* Timer functions */

int uv_timer_init(uv_loop_t *loop, uv_timer_t *handle);
int uv_timer_start(uv_timer_t *handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat);
int uv_timer_stop(uv_timer_t *handle);

#endif /* __UV_H */
EOF

info "✓ 依赖检查完成"
echo ""

# 清理
info "清理旧的构建..."
make clean 2>/dev/null || true
echo ""

# 编译
info "编译测试..."
if make; then
    echo ""
    info "✓ 编译成功"
else
    echo ""
    error "✗ 编译失败"
    exit 1
fi
echo ""

# 运行
info "运行测试..."
echo "════════════════════════════════════════"
echo ""

# 使用临时目录存储测试数据
export KNOWLEDGE_STORAGE_DIR="/tmp/knowledge_cards_test"
mkdir -p "$KNOWLEDGE_STORAGE_DIR"

if ./knowledge_cards_tests; then
    echo ""
    echo "════════════════════════════════════════"
    info "✓ 所有测试通过"
else
    echo ""
    echo "════════════════════════════════════════"
    error "✗ 测试失败"
    exit 1
fi
