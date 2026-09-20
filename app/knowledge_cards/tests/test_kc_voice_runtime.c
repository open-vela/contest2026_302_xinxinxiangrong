/****************************************************************************
 * Current firmware voice state-machine test with mocked board/cloud I/O.
 ****************************************************************************/

#include "kc_voice.h"

#include "agent_config.h"
#include "voice/audio_capture.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct audio_capture
{
  int unused;
};

static struct audio_capture g_capture;
static bool g_configured;
static char g_app_id[128];
static char g_token[256];
static char g_cluster[128];
static int g_failures;

#define CHECK(condition, message) \
  do \
    { \
      if (!(condition)) \
        { \
          fprintf(stderr, "FAIL: %s\n", message); \
          g_failures++; \
        } \
      else \
        { \
          printf("PASS: %s\n", message); \
        } \
    } \
  while (0)

int config_store_init(void)
{
  return OK;
}

int claw_config_get(const char *key, char *buffer, size_t size)
{
  const char *value = NULL;

  if (!g_configured)
    {
      return ERROR;
    }

  if (strcmp(key, AGENT_CFG_KEY_VOLC_APPKEY) == 0)
    {
      value = g_app_id;
    }
  else if (strcmp(key, AGENT_CFG_KEY_VOLC_TOKEN) == 0)
    {
      value = g_token;
    }
  else if (strcmp(key, AGENT_CFG_KEY_VOLC_ASR_CLUSTER) == 0)
    {
      value = g_cluster;
    }

  if (value == NULL || value[0] == '\0')
    {
      return ERROR;
    }

  snprintf(buffer, size, "%s", value);
  return OK;
}

int claw_config_set(const char *key, const char *value)
{
  if (strcmp(key, AGENT_CFG_KEY_VOLC_APPKEY) == 0)
    {
      snprintf(g_app_id, sizeof(g_app_id), "%s", value);
    }
  else if (strcmp(key, AGENT_CFG_KEY_VOLC_TOKEN) == 0)
    {
      snprintf(g_token, sizeof(g_token), "%s", value);
    }
  else if (strcmp(key, AGENT_CFG_KEY_VOLC_ASR_CLUSTER) == 0)
    {
      snprintf(g_cluster, sizeof(g_cluster), "%s", value);
    }

  g_configured = g_app_id[0] != '\0' && g_token[0] != '\0';
  return OK;
}

audio_capture_t *audio_capture_open(const char *path, unsigned int rate,
                                    unsigned int channels,
                                    unsigned int bits)
{
  CHECK(strcmp(path, AGENT_AUDIO_CAPTURE_DEV) == 0,
        "uses configured capture device");
  CHECK(rate == 16000 && channels == 1 && bits == 16,
        "uses 16 kHz mono 16-bit PCM");
  return &g_capture;
}

int audio_capture_start(audio_capture_t *capture)
{
  return capture == &g_capture ? 0 : -EINVAL;
}

int audio_capture_read(audio_capture_t *capture, void *buffer, size_t size)
{
  if (capture != &g_capture)
    {
      return -EINVAL;
    }

  memset(buffer, 0x2a, size);
  usleep(1000);
  return (int)size;
}

int audio_capture_abort(audio_capture_t *capture)
{
  return capture == &g_capture ? 0 : -EINVAL;
}

void audio_capture_close(audio_capture_t *capture)
{
  CHECK(capture == &g_capture, "closes the active capture device");
}

int volc_asr_recognize(const unsigned char *pcm, size_t pcm_len,
                       const char *app_id, const char *token,
                       const char *cluster, char *text, size_t text_size)
{
  CHECK(pcm != NULL && pcm_len > 0, "sends recorded PCM to ASR");
  CHECK(strcmp(app_id, "test-app") == 0, "passes ASR App ID");
  CHECK(strcmp(token, "test-token") == 0, "passes ASR token");
  CHECK(strcmp(cluster, AGENT_DOUBAO_ASR_CLUSTER) == 0,
        "passes default ASR cluster");
  snprintf(text, text_size, "线性代数");
  return 0;
}

static int wait_for_state(enum kc_voice_state_e expected,
                          struct kc_voice_snapshot_s *snapshot)
{
  int attempt;

  for (attempt = 0; attempt < 200; attempt++)
    {
      kc_voice_get_snapshot(snapshot);
      if (snapshot->state == expected)
        {
          return 0;
        }

      usleep(5000);
    }

  return -ETIMEDOUT;
}

int main(void)
{
  struct kc_voice_snapshot_s snapshot;

  CHECK(kc_voice_init() == 0, "initializes voice worker");
  kc_voice_get_snapshot(&snapshot);
  CHECK(snapshot.state == KC_VOICE_CONFIG_REQUIRED,
        "reports missing credentials");

  CHECK(kc_voice_configure("test-app", "test-token", NULL) == 0,
        "configures ASR credentials");
  CHECK(kc_voice_is_configured(), "detects configured ASR");

  CHECK(kc_voice_start_async() == 0, "queues recording start");
  CHECK(wait_for_state(KC_VOICE_RECORDING, &snapshot) == 0,
        "enters recording state");
  CHECK(kc_voice_start_async() == -EBUSY,
        "rejects a second recording start");

  CHECK(kc_voice_stop_async() == 0, "queues recording stop");
  CHECK(wait_for_state(KC_VOICE_RESULT, &snapshot) == 0,
        "enters result state after ASR");
  CHECK(strcmp(snapshot.text, "线性代数") == 0,
        "returns UTF-8 Chinese recognition text");

  printf("voice runtime test: %s\n", g_failures == 0 ? "PASS" : "FAIL");
  return g_failures == 0 ? 0 : 1;
}
