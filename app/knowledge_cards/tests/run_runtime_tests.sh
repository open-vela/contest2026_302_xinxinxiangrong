#!/bin/sh

set -eu

TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
APP_DIR=$(CDPATH= cd -- "$TEST_DIR/.." && pwd)
ROOT_DIR=${OPENVELA_ROOT:-$(CDPATH= cd -- "$APP_DIR/../../.." && pwd)}
OUTPUT=/tmp/knowledge_cards_voice_runtime_test

cc -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror -pthread \
  -I"$TEST_DIR/runtime_include" \
  -I"$APP_DIR/src" \
  -I"$ROOT_DIR/packages/ai_agent/include" \
  -I"$ROOT_DIR/packages/ai_agent/src" \
  "$TEST_DIR/test_kc_voice_runtime.c" \
  "$APP_DIR/src/kc_voice.c" \
  -o "$OUTPUT"

"$OUTPUT"
