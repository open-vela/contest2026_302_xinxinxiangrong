/****************************************************************************
 * packages/demos/knowledge_cards/tests/test_card_manager.c
 *
 * 卡片管理器单元测试
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

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
 * Name: test_card_manager_init
 *
 * Description:
 *   测试卡片管理器初始化
 *
 ****************************************************************************/

int test_card_manager_init(void)
{
    int ret;

    /* 测试 1: 正常初始化 */

    printf("\n测试 1: 正常初始化\n");

    ret = card_manager_init();
    if (test_assert(ret == 0, "卡片管理器初始化应该成功") != 0)
    {
        return -1;
    }

    /* 测试 2: 重复初始化 */

    printf("\n测试 2: 重复初始化\n");

    ret = card_manager_init();
    if (test_assert(ret == 0, "重复初始化应该成功（幂等）") != 0)
    {
        return -1;
    }

    /* 测试 3: 初始状态 */

    printf("\n测试 3: 初始状态\n");

    int total = card_manager_get_total_cards();
    if (test_assert(total == 0, "初始卡片数应该为 0") != 0)
    {
        return -1;
    }

    int current = card_manager_get_current_index();
    if (test_assert(current == 0, "初始索引应该为 0") != 0)
    {
        return -1;
    }

    knowledge_card_t *card = card_manager_get_current_card();
    if (test_assert(card == NULL, "初始卡片应该为 NULL") != 0)
    {
        return -1;
    }

    return 0;
}

/****************************************************************************
 * Name: test_card_manager_load_save
 *
 * Description:
 *   测试卡片加载和保存
 *
 ****************************************************************************/

int test_card_manager_load_save(void)
{
    int ret;

    /* 测试 1: 加载新主题 */

    printf("\n测试 1: 加载新主题\n");

    ret = card_manager_load_topic("test_topic");
    if (test_assert(ret == 0, "加载新主题应该成功") != 0)
    {
        return -1;
    }

    /* 测试 2: 保存主题 */

    printf("\n测试 2: 保存主题\n");

    ret = card_manager_save_topic("test_topic");
    if (test_assert(ret == 0, "保存主题应该成功") != 0)
    {
        return -1;
    }

    /* 测试 3: 重新加载主题 */

    printf("\n测试 3: 重新加载主题\n");

    ret = card_manager_load_topic("test_topic");
    if (test_assert(ret == 0, "重新加载主题应该成功") != 0)
    {
        return -1;
    }

    /* 测试 4: 加载空主题名 */

    printf("\n测试 4: 加载空主题名\n");

    ret = card_manager_load_topic(NULL);
    if (test_assert(ret != 0, "空主题名应该失败") != 0)
    {
        return -1;
    }

    /* 测试 5: 保存空主题名 */

    printf("\n测试 5: 保存空主题名\n");

    ret = card_manager_save_topic(NULL);
    if (test_assert(ret != 0, "空主题名应该失败") != 0)
    {
        return -1;
    }

    return 0;
}

/****************************************************************************
 * Name: test_card_manager_navigation
 *
 * Description:
 *   测试卡片导航功能
 *
 ****************************************************************************/

int test_card_manager_navigation(void)
{
    /* 注意：这个测试需要先有卡片数据 */

    /* 测试 1: 空卡片导航 */

    printf("\n测试 1: 空卡片导航\n");

    card_manager_load_topic("empty_topic");

    knowledge_card_t *card = card_manager_get_current_card();
    if (test_assert(card == NULL, "空主题应该返回 NULL") != 0)
    {
        return -1;
    }

    card = card_manager_get_next_card();
    if (test_assert(card == NULL, "空主题下一张应该返回 NULL") != 0)
    {
        return -1;
    }

    card = card_manager_get_prev_card();
    if (test_assert(card == NULL, "空主题上一张应该返回 NULL") != 0)
    {
        return -1;
    }

    /* 测试 2: 索引检查 */

    printf("\n测试 2: 索引检查\n");

    int index = card_manager_get_current_index();
    if (test_assert(index == 0, "空主题索引应该为 0") != 0)
    {
        return -1;
    }

    int total = card_manager_get_total_cards();
    if (test_assert(total == 0, "空主题总数应该为 0") != 0)
    {
        return -1;
    }

    return 0;
}
