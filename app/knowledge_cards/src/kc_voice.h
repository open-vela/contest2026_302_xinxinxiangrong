/****************************************************************************
 * packages/demos/knowledge_cards/src/kc_voice.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __PACKAGES_DEMOS_KNOWLEDGE_CARDS_SRC_KC_VOICE_H
#define __PACKAGES_DEMOS_KNOWLEDGE_CARDS_SRC_KC_VOICE_H

#include <stddef.h>
#include <stdint.h>

enum kc_voice_state_e
{
  KC_VOICE_UNINITIALIZED = 0,
  KC_VOICE_CONFIG_REQUIRED,
  KC_VOICE_IDLE,
  KC_VOICE_STARTING,
  KC_VOICE_RECORDING,
  KC_VOICE_RECOGNIZING,
  KC_VOICE_RESULT,
  KC_VOICE_ERROR
};

struct kc_voice_snapshot_s
{
  enum kc_voice_state_e state;
  int error;
  uint32_t generation;
  char text[256];
};

int kc_voice_init(void);
int kc_voice_start_async(void);
int kc_voice_stop_async(void);
void kc_voice_get_snapshot(struct kc_voice_snapshot_s *snapshot);
int kc_voice_configure(const char *app_id, const char *token,
                       const char *cluster);
int kc_voice_is_configured(void);

#endif /* __PACKAGES_DEMOS_KNOWLEDGE_CARDS_SRC_KC_VOICE_H */
