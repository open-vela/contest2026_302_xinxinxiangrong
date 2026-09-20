/****************************************************************************
 * packages/demos/knowledge_cards/src/wifi_utils.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "knowledge_cards.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WIFI_TEST_HOST      "api.openai.com"
#define WIFI_TEST_PORT      443
#define WIFI_CONNECT_TIMEOUT 5000  /* 5秒超时 */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wifi_check_connection
 *
 * Description:
 *   检查 WiFi 连接状态
 *
 ****************************************************************************/

int wifi_check_connection(void)
{
    struct hostent *server;
    int sock;
    struct sockaddr_in server_addr;
    int ret;
    int flags;

    /* 尝试 DNS 解析 */

    server = gethostbyname(WIFI_TEST_HOST);
    if (server == NULL)
    {
        /* DNS 解析失败，可能没有网络 */

        return -ENXIO;
    }

    /* 尝试连接 */

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return -errno;
    }

    /* 设置非阻塞模式 */

    flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(WIFI_TEST_PORT);

    ret = connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    close(sock);

    /* 非阻塞连接会返回 EINPROGRESS */

    if (ret < 0 && errno != EINPROGRESS)
    {
        return -errno;
    }

    return 0;
}

/****************************************************************************
 * Name: wifi_connect
 *
 * Description:
 *   连接 WiFi（需要系统配置）
 *
 ****************************************************************************/

int wifi_connect(void)
{
    /* WiFi 连接需要系统级配置 */

    printf("WiFi connection should be configured at system level\n");
    printf("Use the following commands:\n");
    printf("  ifup wlan0\n");
    printf("  wapi psk wlan0 <SSID> <PASSWORD>\n");
    printf("  renew wlan0\n");

    return wifi_check_connection();
}

/****************************************************************************
 * Name: wifi_wait_for_connection
 *
 * Description:
 *   等待 WiFi 连接
 *
 ****************************************************************************/

int wifi_wait_for_connection(int timeout_ms)
{
    int elapsed = 0;
    int interval = 500;  /* 500ms 检查一次 */

    while (elapsed < timeout_ms)
    {
        if (wifi_check_connection() == 0)
        {
            printf("WiFi connected\n");
            return 0;
        }

        usleep(interval * 1000);
        elapsed += interval;
    }

    fprintf(stderr, "ERROR: WiFi connection timeout\n");
    return -ETIMEDOUT;
}
