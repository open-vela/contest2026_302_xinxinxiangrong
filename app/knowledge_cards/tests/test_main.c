/****************************************************************************
 * packages/demos/knowledge_cards/tests/test_main.c
 *
 * 单元测试主入口
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/****************************************************************************
 * Test Function Prototypes
 ****************************************************************************/

/* LLM 客户端测试 */

extern int test_llm_init(void);
extern int test_llm_parse_response(void);
extern int test_llm_url_parsing(void);

/* 卡片管理器测试 */

extern int test_card_manager_init(void);
extern int test_card_manager_load_save(void);
extern int test_card_manager_navigation(void);

/* 语音识别测试 */

extern int test_voice_init(void);
extern int test_base64_encode(void);
extern int test_local_asr(void);
extern int test_voice_state_machine(void);

/* WiFi 工具测试 */

extern int test_wifi_check_connection(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_tests_passed = 0;
static int g_tests_failed = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void run_test(const char *name, int (*test_func)(void))
{
    printf("\n[TEST] %s\n", name);
    printf("────────────────────────────────────────\n");

    int ret = test_func();

    if (ret == 0)
    {
        printf("✓ PASSED\n");
        g_tests_passed++;
    }
    else
    {
        printf("✗ FAILED (ret=%d)\n", ret);
        g_tests_failed++;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
    printf("╔════════════════════════════════════════╗\n");
    printf("║    Knowledge Cards 单元测试           ║\n");
    printf("╚════════════════════════════════════════╝\n");

    /* LLM 客户端测试 */

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         LLM 客户端测试                 ║\n");
    printf("╚════════════════════════════════════════╝\n");

    run_test("LLM 初始化", test_llm_init);
    run_test("LLM 响应解析", test_llm_parse_response);
    run_test("URL 解析", test_llm_url_parsing);

    /* 卡片管理器测试 */

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         卡片管理器测试                 ║\n");
    printf("╚════════════════════════════════════════╝\n");

    run_test("卡片管理器初始化", test_card_manager_init);
    run_test("卡片加载保存", test_card_manager_load_save);
    run_test("卡片导航", test_card_manager_navigation);

    /* 语音识别测试 */

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         语音识别测试                   ║\n");
    printf("╚════════════════════════════════════════╝\n");

    run_test("语音初始化", test_voice_init);
    run_test("Base64 编码", test_base64_encode);
    run_test("本地 ASR", test_local_asr);
    run_test("语音状态机", test_voice_state_machine);

    /* WiFi 工具测试 */

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         WiFi 工具测试                  ║\n");
    printf("╚════════════════════════════════════════╝\n");

    run_test("WiFi 连接检测", test_wifi_check_connection);

    /* 测试总结 */

    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         测试总结                       ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n总测试数: %d\n", g_tests_passed + g_tests_failed);
    printf("通过: %d\n", g_tests_passed);
    printf("失败: %d\n", g_tests_failed);

    if (g_tests_failed == 0)
    {
        printf("\n✓ 所有测试通过！\n\n");
        return 0;
    }
    else
    {
        printf("\n✗ 有 %d 个测试失败\n\n", g_tests_failed);
        return 1;
    }
}
