/****************************************************************************
 * packages/demos/knowledge_cards/src/kc_proxy_stub.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <stdbool.h>

/* Volc ASR can optionally use AI Agent's HTTP proxy.  Knowledge Cards uses
 * a direct TLS connection, so provide weak fallbacks without pulling the
 * complete proxy/LLM stack into the watch image. */

bool __attribute__((weak)) http_proxy_is_enabled(void)
{
  return false;
}

int __attribute__((weak)) proxy_open_tunnel(const char *host, int port,
                                            int timeout_ms)
{
  (void)host;
  (void)port;
  (void)timeout_ms;
  return -1;
}
