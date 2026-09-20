#!/bin/sh
set -eu
TEST_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
APP_DIR=$(CDPATH= cd -- "$TEST_DIR/.." && pwd)
ROOT_DIR=${OPENVELA_ROOT:-$(CDPATH= cd -- "$APP_DIR/../../.." && pwd)}
CJSON_DIR="$ROOT_DIR/apps/netutils/cjson/cJSON"
OUTPUT=$(mktemp /tmp/kc-deck-test.XXXXXX)
trap 'rm -f "$OUTPUT"' EXIT HUP INT TERM
cc -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$APP_DIR/src" -I"$CJSON_DIR" \
  "$TEST_DIR/test_kc_deck.c" "$APP_DIR/src/kc_deck.c" \
  "$CJSON_DIR/cJSON.c" -lm -o "$OUTPUT"
"$OUTPUT" "$APP_DIR/decks"
