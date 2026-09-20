/****************************************************************************
 * packages/demos/knowledge_cards/src/kc_voice.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>

#include "kc_voice.h"

#include "agent_config.h"
#include "infra/config_store.h"
#include "voice/audio_capture.h"
#include "voice/volc_asr.h"

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define KC_VOICE_PCM_CAPACITY (192 * 1024)
#define KC_VOICE_THREAD_STACK  (16 * 1024)
#define KC_CAPTURE_THREAD_STACK (8 * 1024)

struct kc_voice_context_s
{
  pthread_mutex_t lock;
  sem_t command_sem;
  pthread_t worker;
  pthread_t capture_worker;
  enum kc_voice_state_e state;
  audio_capture_t *capture;
  unsigned char *pcm;
  size_t pcm_len;
  bool initialized;
  bool start_pending;
  bool stop_pending;
  bool capture_running;
  bool capture_started;
  int capture_error;
  int error;
  uint32_t generation;
  char text[256];
};

static struct kc_voice_context_s g_voice =
{
  .lock = PTHREAD_MUTEX_INITIALIZER,
  .state = KC_VOICE_UNINITIALIZED
};

static void kc_voice_copy(char *destination, size_t size,
                          const char *source)
{
  if (size > 0)
    {
      snprintf(destination, size, "%s", source != NULL ? source : "");
    }
}

static void kc_voice_set_state_locked(enum kc_voice_state_e state,
                                      int error, const char *text)
{
  g_voice.state = state;
  g_voice.error = error;
  g_voice.generation++;

  if (text != NULL)
    {
      kc_voice_copy(g_voice.text, sizeof(g_voice.text), text);
    }
  else
    {
      g_voice.text[0] = '\0';
    }
}

static int kc_voice_load_credentials(char *app_id, size_t app_id_size,
                                     char *token, size_t token_size,
                                     char *cluster, size_t cluster_size)
{
  if (claw_config_get(AGENT_CFG_KEY_VOLC_APPKEY,
                      app_id, app_id_size) != OK ||
      claw_config_get(AGENT_CFG_KEY_VOLC_TOKEN,
                      token, token_size) != OK)
    {
      return -ENOENT;
    }

  if (claw_config_get(AGENT_CFG_KEY_VOLC_ASR_CLUSTER,
                      cluster, cluster_size) != OK || cluster[0] == '\0')
    {
      kc_voice_copy(cluster, cluster_size, AGENT_DOUBAO_ASR_CLUSTER);
    }

  return 0;
}

int kc_voice_is_configured(void)
{
  char app_id[128];
  char token[256];
  char cluster[128];

  return kc_voice_load_credentials(app_id, sizeof(app_id),
                                   token, sizeof(token),
                                   cluster, sizeof(cluster)) == 0;
}

static void *kc_capture_thread(void *arg)
{
  unsigned char chunk[AGENT_ASR_CHUNK_SIZE];
  int read_size;

  (void)arg;

  while (true)
    {
      pthread_mutex_lock(&g_voice.lock);
      if (!g_voice.capture_running ||
          g_voice.pcm_len >= KC_VOICE_PCM_CAPACITY)
        {
          pthread_mutex_unlock(&g_voice.lock);
          break;
        }
      pthread_mutex_unlock(&g_voice.lock);

      read_size = audio_capture_read(g_voice.capture, chunk, sizeof(chunk));
      if (read_size <= 0)
        {
          pthread_mutex_lock(&g_voice.lock);
          if (g_voice.capture_running && read_size < 0)
            {
              g_voice.capture_error = read_size;
            }
          pthread_mutex_unlock(&g_voice.lock);
          break;
        }

      pthread_mutex_lock(&g_voice.lock);
      if ((size_t)read_size > KC_VOICE_PCM_CAPACITY - g_voice.pcm_len)
        {
          read_size = (int)(KC_VOICE_PCM_CAPACITY - g_voice.pcm_len);
        }

      memcpy(g_voice.pcm + g_voice.pcm_len, chunk, (size_t)read_size);
      g_voice.pcm_len += (size_t)read_size;

      if (g_voice.pcm_len >= KC_VOICE_PCM_CAPACITY)
        {
          g_voice.stop_pending = true;
          sem_post(&g_voice.command_sem);
        }
      pthread_mutex_unlock(&g_voice.lock);
    }

  return NULL;
}

static int kc_voice_start_capture(void)
{
  pthread_attr_t attr;
  int ret;

  pthread_mutex_lock(&g_voice.lock);
  kc_voice_set_state_locked(KC_VOICE_STARTING, 0, NULL);
  pthread_mutex_unlock(&g_voice.lock);

  if (!kc_voice_is_configured())
    {
      pthread_mutex_lock(&g_voice.lock);
      kc_voice_set_state_locked(KC_VOICE_CONFIG_REQUIRED, -ENOENT, NULL);
      pthread_mutex_unlock(&g_voice.lock);
      return -ENOENT;
    }

  g_voice.pcm = malloc(KC_VOICE_PCM_CAPACITY);
  if (g_voice.pcm == NULL)
    {
      ret = -ENOMEM;
      goto failed;
    }

  g_voice.capture = audio_capture_open(AGENT_AUDIO_CAPTURE_DEV,
                                       AGENT_VOICE_SAMPLE_RATE,
                                       AGENT_VOICE_CHANNELS,
                                       AGENT_VOICE_BITS);
  if (g_voice.capture == NULL)
    {
      ret = -ENODEV;
      goto failed;
    }

  pthread_mutex_lock(&g_voice.lock);
  g_voice.pcm_len = 0;
  g_voice.capture_error = 0;
  g_voice.capture_running = true;
  pthread_mutex_unlock(&g_voice.lock);

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, KC_CAPTURE_THREAD_STACK);
  ret = pthread_create(&g_voice.capture_worker, &attr,
                       kc_capture_thread, NULL);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      ret = -ret;
      pthread_mutex_lock(&g_voice.lock);
      g_voice.capture_running = false;
      pthread_mutex_unlock(&g_voice.lock);
      goto failed;
    }

  g_voice.capture_started = true;
  ret = audio_capture_start(g_voice.capture);
  if (ret < 0)
    {
      audio_capture_abort(g_voice.capture);
      pthread_join(g_voice.capture_worker, NULL);
      g_voice.capture_started = false;
      goto failed;
    }

  pthread_mutex_lock(&g_voice.lock);
  kc_voice_set_state_locked(KC_VOICE_RECORDING, 0, NULL);
  pthread_mutex_unlock(&g_voice.lock);
  syslog(LOG_INFO, "KNOWLEDGE_CARDS: voice recording started\n");
  return 0;

failed:
  if (g_voice.capture != NULL)
    {
      audio_capture_close(g_voice.capture);
      g_voice.capture = NULL;
    }

  free(g_voice.pcm);
  g_voice.pcm = NULL;
  g_voice.pcm_len = 0;

  pthread_mutex_lock(&g_voice.lock);
  kc_voice_set_state_locked(KC_VOICE_ERROR, ret, NULL);
  pthread_mutex_unlock(&g_voice.lock);
  syslog(LOG_ERR, "KNOWLEDGE_CARDS: voice capture start failed: %d\n",
         ret);
  return ret;
}

static int kc_voice_stop_and_recognize(void)
{
  char app_id[128];
  char token[256];
  char cluster[128];
  char result[256];
  size_t pcm_len;
  int capture_error;
  int ret;

  pthread_mutex_lock(&g_voice.lock);
  if (g_voice.state != KC_VOICE_RECORDING &&
      g_voice.state != KC_VOICE_STARTING)
    {
      pthread_mutex_unlock(&g_voice.lock);
      return -EINVAL;
    }

  g_voice.capture_running = false;
  pthread_mutex_unlock(&g_voice.lock);

  if (g_voice.capture != NULL)
    {
      audio_capture_abort(g_voice.capture);
    }

  if (g_voice.capture_started)
    {
      pthread_join(g_voice.capture_worker, NULL);
      g_voice.capture_started = false;
    }

  if (g_voice.capture != NULL)
    {
      audio_capture_close(g_voice.capture);
      g_voice.capture = NULL;
    }

  pthread_mutex_lock(&g_voice.lock);
  pcm_len = g_voice.pcm_len;
  capture_error = g_voice.capture_error;
  kc_voice_set_state_locked(KC_VOICE_RECOGNIZING, 0, NULL);
  pthread_mutex_unlock(&g_voice.lock);

  if (pcm_len == 0)
    {
      ret = capture_error < 0 ? capture_error : -ENODATA;
      goto failed;
    }

  ret = kc_voice_load_credentials(app_id, sizeof(app_id),
                                  token, sizeof(token),
                                  cluster, sizeof(cluster));
  if (ret < 0)
    {
      goto failed;
    }

  result[0] = '\0';
  ret = volc_asr_recognize(g_voice.pcm, pcm_len,
                           app_id, token, cluster,
                           result, sizeof(result));
  if (ret < 0 || result[0] == '\0')
    {
      if (ret == 0)
        {
          ret = -ENODATA;
        }
      goto failed;
    }

  free(g_voice.pcm);
  g_voice.pcm = NULL;
  g_voice.pcm_len = 0;

  pthread_mutex_lock(&g_voice.lock);
  kc_voice_set_state_locked(KC_VOICE_RESULT, 0, result);
  pthread_mutex_unlock(&g_voice.lock);
  syslog(LOG_INFO, "KNOWLEDGE_CARDS: ASR result: %s\n", result);
  return 0;

failed:
  free(g_voice.pcm);
  g_voice.pcm = NULL;
  g_voice.pcm_len = 0;

  pthread_mutex_lock(&g_voice.lock);
  kc_voice_set_state_locked(KC_VOICE_ERROR, ret, NULL);
  pthread_mutex_unlock(&g_voice.lock);
  syslog(LOG_ERR, "KNOWLEDGE_CARDS: ASR failed: %d\n", ret);
  return ret;
}

static void *kc_voice_worker(void *arg)
{
  bool start;
  bool stop;

  (void)arg;

  while (true)
    {
      while (sem_wait(&g_voice.command_sem) < 0 && errno == EINTR)
        {
        }

      pthread_mutex_lock(&g_voice.lock);
      start = g_voice.start_pending;
      stop = g_voice.stop_pending;
      g_voice.start_pending = false;
      g_voice.stop_pending = false;
      pthread_mutex_unlock(&g_voice.lock);

      if (start)
        {
          kc_voice_start_capture();
        }

      pthread_mutex_lock(&g_voice.lock);
      stop = stop || g_voice.stop_pending;
      g_voice.stop_pending = false;
      pthread_mutex_unlock(&g_voice.lock);

      if (stop)
        {
          kc_voice_stop_and_recognize();
        }
    }

  return NULL;
}

int kc_voice_init(void)
{
  pthread_attr_t attr;
  int ret;

  pthread_mutex_lock(&g_voice.lock);
  if (g_voice.initialized)
    {
      pthread_mutex_unlock(&g_voice.lock);
      return 0;
    }
  pthread_mutex_unlock(&g_voice.lock);

  config_store_init();
  sem_init(&g_voice.command_sem, 0, 0);

  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, KC_VOICE_THREAD_STACK);
  ret = pthread_create(&g_voice.worker, &attr, kc_voice_worker, NULL);
  pthread_attr_destroy(&attr);
  if (ret != 0)
    {
      return -ret;
    }

  pthread_mutex_lock(&g_voice.lock);
  g_voice.initialized = true;
  kc_voice_set_state_locked(kc_voice_is_configured() ? KC_VOICE_IDLE :
                            KC_VOICE_CONFIG_REQUIRED, 0, NULL);
  pthread_mutex_unlock(&g_voice.lock);
  return 0;
}

int kc_voice_start_async(void)
{
  int ret = 0;

  pthread_mutex_lock(&g_voice.lock);
  if (!g_voice.initialized)
    {
      ret = -ENODEV;
    }
  else if (g_voice.state == KC_VOICE_STARTING ||
           g_voice.state == KC_VOICE_RECORDING ||
           g_voice.state == KC_VOICE_RECOGNIZING)
    {
      ret = -EBUSY;
    }
  else
    {
      g_voice.start_pending = true;
      g_voice.stop_pending = false;
      kc_voice_set_state_locked(KC_VOICE_STARTING, 0, NULL);
      sem_post(&g_voice.command_sem);
    }
  pthread_mutex_unlock(&g_voice.lock);
  return ret;
}

int kc_voice_stop_async(void)
{
  int ret = 0;

  pthread_mutex_lock(&g_voice.lock);
  if (!g_voice.initialized)
    {
      ret = -ENODEV;
    }
  else if (g_voice.state != KC_VOICE_STARTING &&
           g_voice.state != KC_VOICE_RECORDING)
    {
      ret = -EINVAL;
    }
  else
    {
      g_voice.stop_pending = true;
      sem_post(&g_voice.command_sem);
    }
  pthread_mutex_unlock(&g_voice.lock);
  return ret;
}

void kc_voice_get_snapshot(struct kc_voice_snapshot_s *snapshot)
{
  if (snapshot == NULL)
    {
      return;
    }

  pthread_mutex_lock(&g_voice.lock);
  snapshot->state = g_voice.state;
  snapshot->error = g_voice.error;
  snapshot->generation = g_voice.generation;
  kc_voice_copy(snapshot->text, sizeof(snapshot->text), g_voice.text);
  pthread_mutex_unlock(&g_voice.lock);
}

int kc_voice_configure(const char *app_id, const char *token,
                       const char *cluster)
{
  int ret;

  if (app_id == NULL || app_id[0] == '\0' ||
      token == NULL || token[0] == '\0')
    {
      return -EINVAL;
    }

  config_store_init();
  ret = claw_config_set(AGENT_CFG_KEY_VOLC_APPKEY, app_id);
  if (ret != OK)
    {
      return -EIO;
    }

  ret = claw_config_set(AGENT_CFG_KEY_VOLC_TOKEN, token);
  if (ret != OK)
    {
      return -EIO;
    }

  ret = claw_config_set(AGENT_CFG_KEY_VOLC_ASR_CLUSTER,
                        cluster != NULL && cluster[0] != '\0' ? cluster :
                        AGENT_DOUBAO_ASR_CLUSTER);
  if (ret != OK)
    {
      return -EIO;
    }

  pthread_mutex_lock(&g_voice.lock);
  if (g_voice.initialized &&
      g_voice.state == KC_VOICE_CONFIG_REQUIRED)
    {
      kc_voice_set_state_locked(KC_VOICE_IDLE, 0, NULL);
    }
  pthread_mutex_unlock(&g_voice.lock);
  return 0;
}
