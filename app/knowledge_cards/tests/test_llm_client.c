/****************************************************************************
 * packages/demos/knowledge_cards/tests/test_llm_client.c
 *
 * LLM 客户端单元测试
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
 * Name: test_llm_init
 *
 * Description:
 *   测试 LLM 客户端初始化
 *
 ****************************************************************************/

int test_llm_init(void)
{
    llm_config_t config;
    int ret;

    /* 测试 1: 正常初始化 */

    printf("\n测试 1: 正常初始化\n");

    memset(&config, 0, sizeof(config));
    strncpy(config.api_key, "test-api-key", sizeof(config.api_key) - 1);
    strncpy(config.endpoint, "https://api.openai.com/v1/chat/completions",
            sizeof(config.endpoint) - 1);
    strncpy(config.model, "gpt-3.5-turbo", sizeof(config.model) - 1);
    config.max_tokens = 512;
    config.timeout = 30;

    ret = llm_init(&config);
    if (test_assert(ret == 0, "LLM 初始化应该成功") != 0)
    {
        return -1;
    }

    /* 测试 2: 空配置 */

    printf("\n测试 2: 空配置\n");

    ret = llm_init(NULL);
    if (test_assert(ret != 0, "空配置应该失败") != 0)
    {
        return -1;
    }

    /* 测试 3: 默认值设置 */

    printf("\n测试 3: 默认值设置\n");

    memset(&config, 0, sizeof(config));
    strncpy(config.api_key, "test-key", sizeof(config.api_key) - 1);

    ret = llm_init(&config);
    if (test_assert(ret == 0, "默认值初始化应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(strlen(config.endpoint) > 0, "endpoint 应该有默认值") != 0)
    {
        return -1;
    }

    if (test_assert(strlen(config.model) > 0, "model 应该有默认值") != 0)
    {
        return -1;
    }

    if (test_assert(config.max_tokens > 0, "max_tokens 应该有默认值") != 0)
    {
        return -1;
    }

    if (test_assert(config.timeout > 0, "timeout 应该有默认值") != 0)
    {
        return -1;
    }

    return 0;
}

/****************************************************************************
 * Name: test_llm_parse_response
 *
 * Description:
 *   测试 LLM 响应解析
 *
 ****************************************************************************/

int test_llm_parse_response(void)
{
    /* 注意：parse_json_response 是 static 函数，无法直接测试 */
    /* 这里测试 LLM 初始化和配置验证 */

    printf("\n测试 1: LLM 配置验证\n");

    llm_config_t config;
    memset(&config, 0, sizeof(config));

    /* 测试各种配置组合 */

    strncpy(config.api_key, "key1", sizeof(config.api_key) - 1);
    strncpy(config.endpoint, "https://api.example.com/v1", sizeof(config.endpoint) - 1);
    strncpy(config.model, "gpt-4", sizeof(config.model) - 1);
    config.max_tokens = 1024;
    config.timeout = 60;

    int ret = llm_init(&config);
    if (test_assert(ret == 0, "配置验证应该成功") != 0)
    {
        return -1;
    }

    printf("\n测试 2: 不同模型配置\n");

    const char *models[] = {"gpt-3.5-turbo", "gpt-4", "claude-3-opus"};
    for (int i = 0; i < 3; i++)
    {
        memset(&config, 0, sizeof(config));
        strncpy(config.api_key, "test", sizeof(config.api_key) - 1);
        strncpy(config.model, models[i], sizeof(config.model) - 1);

        ret = llm_init(&config);
        if (test_assert(ret == 0, "模型配置应该成功") != 0)
        {
            return -1;
        }
    }

    return 0;
}

/****************************************************************************
 * Name: test_llm_url_parsing
 *
 * Description:
 *   测试 URL 解析功能
 *
 ****************************************************************************/

int test_llm_url_parsing(void)
{
    /* 注意：extract_host_from_url 和 extract_path_from_url 是 static 函数 */
    /* 这里测试 URL 配置的正确性 */

    printf("\n测试 1: 标准 URL 配置\n");

    llm_config_t config;
    memset(&config, 0, sizeof(config));
    strncpy(config.api_key, "test", sizeof(config.api_key) - 1);
    strncpy(config.endpoint, "https://api.openai.com/v1/chat/completions",
            sizeof(config.endpoint) - 1);

    int ret = llm_init(&config);
    if (test_assert(ret == 0, "标准 URL 配置应该成功") != 0)
    {
        return -1;
    }

    printf("\n测试 2: 不同服务提供商 URL\n");

    const char *urls[] = {
        "https://api.openai.com/v1/chat/completions",
        "https://api.anthropic.com/v1/messages",
        "https://openspeech.bytedance.com/api/v1/asr",
        "https://api.xfyun.cn/v1/service/v1/iat"
    };

    for (int i = 0; i < 4; i++)
    {
        memset(&config, 0, sizeof(config));
        strncpy(config.api_key, "test", sizeof(config.api_key) - 1);
        strncpy(config.endpoint, urls[i], sizeof(config.endpoint) - 1);

        ret = llm_init(&config);
        if (test_assert(ret == 0, "URL 配置应该成功") != 0)
        {
            return -1;
        }
    }

    return 0;
}
