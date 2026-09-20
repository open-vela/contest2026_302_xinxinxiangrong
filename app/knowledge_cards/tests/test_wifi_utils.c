/****************************************************************************
 * packages/demos/knowledge_cards/tests/test_wifi_utils.c
 *
 * WiFi 工具单元测试
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* 包含被测试的头文件 */

#include <knowledge_cards_test.h>

/****************************************************************************
 * Private Functions - 辅助函数
 ****************************************************************************/

static int test_assert(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "  ASSERT FAILED: %s\n", message);
        return -1;
    }
    printf("  ✓ %s\n", message);
    return 0;
}

/****************************************************************************
 * Public Functions - 测试用例
 ****************************************************************************/

/****************************************************************************
 * Name: test_wifi_check_connection
 *
 * Description:
 *   测试 WiFi 连接检测
 *
 ****************************************************************************/

int test_wifi_check_connection(void)
{
    int ret;

    /* 测试 1: 检查连接（可能失败，因为没有真实网络） */

    printf("\n测试 1: 检查连接\n");

    ret = wifi_check_connection();

    /* 注意：在没有网络的环境中，这个测试可能会失败 */
    /* 我们只测试函数调用不会崩溃 */

    if (ret == 0)
    {
        printf("  ✓ WiFi 连接成功（有网络环境）\n");
    }
    else
    {
        printf("  ℹ WiFi 连接失败（无网络环境或网络不可用）\n");
        printf("    错误码: %d\n", ret);
    }

    /* 测试 2: 等待连接（短超时） */

    printf("\n测试 2: 等待连接（短超时）\n");

    ret = wifi_wait_for_connection(1000);  /* 1秒超时 */

    if (ret == 0)
    {
        printf("  ✓ WiFi 连接成功\n");
    }
    else if (ret == -ETIMEDOUT)
    {
        printf("  ℹ WiFi 连接超时（预期行为，在无网络环境中）\n");
    }
    else
    {
        printf("  ℹ WiFi 连接失败，错误码: %d\n", ret);
    }

    /* 测试 3: 连接函数 */

    printf("\n测试 3: 连接函数\n");

    ret = wifi_connect();

    /* wifi_connect 只是打印配置说明，应该返回 0 或网络错误 */

    printf("  ℹ wifi_connect 返回: %d\n", ret);

    return 0;
}
