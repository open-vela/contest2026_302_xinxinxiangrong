/* SPDX-License-Identifier: Apache-2.0 */
#ifndef KC_DECK_H
#define KC_DECK_H

#include <stddef.h>

#define KC_DECK_MAX_CARDS 50
#define KC_DECK_TITLE_SIZE 64
#define KC_DECK_CONTENT_SIZE 512
#define KC_DECK_TOPIC_SIZE 128

struct kc_card_s
{
  char title[KC_DECK_TITLE_SIZE];
  char content[KC_DECK_CONTENT_SIZE];
};

struct kc_deck_s
{
  char topic[KC_DECK_TOPIC_SIZE];
  size_t count;
  struct kc_card_s cards[KC_DECK_MAX_CARDS];
};

/* On failure the current deck is preserved. Topic is a filename stem. */
int kc_deck_load(const char *directory, const char *topic,
                 struct kc_deck_s *deck);
#endif
