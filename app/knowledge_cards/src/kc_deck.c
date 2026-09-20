/* SPDX-License-Identifier: Apache-2.0 */
#include "kc_deck.h"

#include <cJSON.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int kc_deck_load(const char *directory, const char *topic,
                 struct kc_deck_s *deck)
{
  char path[512];
  FILE *file = NULL;
  char *text = NULL;
  struct kc_deck_s *candidate = NULL;
  cJSON *root = NULL;
  cJSON *cards;
  long length;
  int count;
  int i;
  int ret = -EINVAL;

  if (directory == NULL || topic == NULL || deck == NULL ||
      topic[0] == '\0' || strlen(topic) >= KC_DECK_TOPIC_SIZE ||
      strchr(topic, '/') != NULL || strchr(topic, '\\') != NULL ||
      strcmp(topic, ".") == 0 || strcmp(topic, "..") == 0)
    {
      return -EINVAL;
    }

  for (i = 0; topic[i] != '\0'; i++)
    {
      if ((unsigned char)topic[i] < 32)
        {
          return -EINVAL;
        }
    }

  if (snprintf(path, sizeof(path), "%s/%s.json", directory, topic) >=
      (int)sizeof(path))
    {
      return -ENAMETOOLONG;
    }

  file = fopen(path, "rb");
  if (file == NULL)
    {
      return -errno;
    }

  if (fseek(file, 0, SEEK_END) != 0 ||
      (length = ftell(file)) <= 0 || length > 65536 ||
      fseek(file, 0, SEEK_SET) != 0)
    {
      goto out;
    }

  text = malloc((size_t)length + 1);
  candidate = calloc(1, sizeof(*candidate));
  if (text == NULL || candidate == NULL)
    {
      ret = -ENOMEM;
      goto out;
    }

  if (fread(text, 1, (size_t)length, file) != (size_t)length)
    {
      ret = -EIO;
      goto out;
    }

  text[length] = '\0';
  if (memchr(text, '\0', (size_t)length) != NULL)
    {
      goto out;
    }

  root = cJSON_ParseWithOpts(text, NULL, 1);
  cards = cJSON_GetObjectItemCaseSensitive(root, "cards");
  count = cJSON_GetArraySize(cards);
  if (!cJSON_IsObject(root) || !cJSON_IsArray(cards) ||
      count <= 0 || count > KC_DECK_MAX_CARDS)
    {
      goto out;
    }

  for (i = 0; i < count; i++)
    {
      cJSON *card = cJSON_GetArrayItem(cards, i);
      cJSON *title = cJSON_GetObjectItemCaseSensitive(card, "title");
      cJSON *content = cJSON_GetObjectItemCaseSensitive(card, "content");

      if (!cJSON_IsString(title) || !cJSON_IsString(content) ||
          title->valuestring[0] == '\0' || content->valuestring[0] == '\0' ||
          strlen(title->valuestring) >= KC_DECK_TITLE_SIZE ||
          strlen(content->valuestring) >= KC_DECK_CONTENT_SIZE)
        {
          goto out;
        }

      strcpy(candidate->cards[i].title, title->valuestring);
      strcpy(candidate->cards[i].content, content->valuestring);
    }

  strcpy(candidate->topic, topic);
  candidate->count = (size_t)count;
  memcpy(deck, candidate, sizeof(*deck));
  ret = 0;

out:
  cJSON_Delete(root);
  free(candidate);
  free(text);
  fclose(file);
  return ret;
}
