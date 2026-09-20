/* SPDX-License-Identifier: Apache-2.0 */
#include "kc_deck.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static struct kc_deck_s deck;
static struct kc_deck_s saved;
static void invalid(const char *dir, const char *body)
{
  char path[512];
  snprintf(path, sizeof(path), "%s/bad.json", dir);
  FILE *file = fopen(path, "w");
  assert(file);
  fputs(body, file);
  fclose(file);
  assert(kc_deck_load(dir, "bad", &deck) < 0);
  assert(memcmp(&saved, &deck, sizeof(deck)) == 0);
}
int main(int argc, char **argv)
{
  char dir[] = "/tmp/kc-deck-XXXXXX";
  char path[512];
  assert(argc == 2);
  assert(kc_deck_load(argv[1], "操作系统", &deck) == 0);
  assert(deck.count == 2 && strcmp(deck.cards[0].title, "进程与线程") == 0);
  puts("PASS: loads an operating-system topic with actual card content");
  assert(kc_deck_load(argv[1], "英语", &deck) == 0);
  assert(strcmp(deck.topic, "英语") == 0 && strcmp(deck.cards[0].title, "主动回忆") == 0);
  puts("PASS: changes both topic and content");
  saved = deck;
  assert(mkdtemp(dir));
  assert(kc_deck_load(dir, "missing", &deck) < 0);
  assert(memcmp(&saved, &deck, sizeof(deck)) == 0);
  puts("PASS: missing deck preserves previous topic");
  assert(kc_deck_load(dir, "../escape", &deck) < 0);
  puts("PASS: rejects path traversal");
  invalid(dir, "{broken");
  invalid(dir, "{\"cards\":[]}");
  invalid(dir, "{\"cards\":[{\"title\":\"x\",\"content\":7}]}");
  invalid(dir, "{\"cards\":[{\"title\":\"\",\"content\":\"x\"}]}");
  invalid(dir, "{\"cards\":[{\"title\":\"x\",\"content\":\"x\"}]}garbage");
  puts("PASS: malformed, empty, wrong-type and trailing data rejected atomically");
  snprintf(path, sizeof(path), "%s/bad.json", dir);
  FILE *f = fopen(path, "w");
  assert(f);
  fputs("{\"cards\":[", f);
  for (int i = 0; i < 51; i++)
    fprintf(f, "%s{\"title\":\"a\",\"content\":\"b\"}", i ? "," : "");
  fputs("]}", f); fclose(f);
  assert(kc_deck_load(dir, "bad", &deck) < 0);
  assert(memcmp(&saved, &deck, sizeof(deck)) == 0);
  puts("PASS: rejects more than 50 cards without replacing active deck");
  f = fopen(path, "w"); assert(f);
  fputs("{\"cards\":[{\"title\":\"x\",\"content\":\"", f);
  for (int i=0; i<512; i++) fputc('a', f);
  fputs("\"}]}", f); fclose(f);
  assert(kc_deck_load(dir, "bad", &deck) < 0);
  assert(memcmp(&saved, &deck, sizeof(deck)) == 0);
  puts("PASS: rejects oversized content without cutting UTF-8 text");
  unlink(path); rmdir(dir);
  puts("deck runtime test: PASS");
  return 0;
}
