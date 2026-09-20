/****************************************************************************
 * packages/demos/knowledge_cards/tests/test_voice_recognizer.c
 *
 * 语音识别模块单元测试
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>

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

/* Base64 编码（与 voice_recognizer.c 中的实现相同） */

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* encode_base64(const unsigned char *data, size_t input_length,
                           size_t *output_length)
{
    *output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(*output_length + 1);
    if (encoded_data == NULL)
    {
        return NULL;
    }

    size_t i = 0;
    size_t j = 0;
    while (i < input_length)
    {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = base64_chars[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 0 * 6) & 0x3F];
    }

    for (size_t k = 0; k < (3 - input_length % 3) % 3; k++)
    {
        encoded_data[*output_length - 1 - k] = '=';
    }

    encoded_data[*output_length] = '\0';
    return encoded_data;
}

/* 本地 ASR 识别（与 voice_recognizer.c 中的实现相同） */

static int local_asr_test(const char *audio_data, int audio_len,
                          char *result, int result_size)
{
    int16_t *samples = (int16_t *)audio_data;
    int sample_count = audio_len / 2;
    long energy = 0;

    for (int i = 0; i < sample_count; i++)
    {
        energy += abs(samples[i]);
    }

    long avg_energy = energy / sample_count;

    if (avg_energy > 1000)
    {
        snprintf(result, result_size, "检测到语音输入");
        return 0;
    }
    else
    {
        snprintf(result, result_size, "未检测到语音");
        return 0;
    }
}

/****************************************************************************
 * Public Functions - 测试用例
 ****************************************************************************/

/****************************************************************************
 * Name: test_voice_init
 *
 * Description:
 *   测试语音识别初始化
 *
 ****************************************************************************/

int test_voice_init(void)
{
    int ret;

    /* 测试 1: 正常初始化 */

    printf("\n测试 1: 正常初始化\n");

    ret = voice_init();
    if (test_assert(ret == 0, "语音初始化应该成功") != 0)
    {
        return -1;
    }

    /* 测试 2: 初始状态 */

    printf("\n测试 2: 初始状态\n");

    bool is_listening = voice_is_listening();
    if (test_assert(is_listening == false, "初始状态不应该在监听") != 0)
    {
        return -1;
    }

    const char *result = voice_get_result();
    if (test_assert(result != NULL, "应该能获取结果指针") != 0)
    {
        return -1;
    }

    if (test_assert(strlen(result) == 0, "初始结果应该为空") != 0)
    {
        return -1;
    }

    /* 测试 3: 注册回调 */

    printf("\n测试 3: 注册回调\n");

    ret = voice_register_callback(NULL, NULL);
    if (test_assert(ret == 0, "注册回调应该成功") != 0)
    {
        return -1;
    }

    return 0;
}

/****************************************************************************
 * Name: test_base64_encode
 *
 * Description:
 *   测试 Base64 编码
 *
 ****************************************************************************/

int test_base64_encode(void)
{
    /* 测试 1: 标准 ASCII 字符串 */

    printf("\n测试 1: 标准 ASCII 字符串\n");

    const char *test1 = "Hello, World!";
    size_t len1;
    char *encoded1 = encode_base64((const unsigned char *)test1,
                                   strlen(test1), &len1);

    if (test_assert(encoded1 != NULL, "编码应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(strcmp(encoded1, "SGVsbG8sIFdvcmxkIQ==") == 0,
                    "编码结果应该匹配") != 0)
    {
        free(encoded1);
        return -1;
    }

    if (test_assert(len1 == 20, "编码长度应该正确") != 0)
    {
        free(encoded1);
        return -1;
    }

    free(encoded1);

    /* 测试 2: 空字符串 */

    printf("\n测试 2: 空字符串\n");

    const char *test2 = "";
    size_t len2;
    char *encoded2 = encode_base64((const unsigned char *)test2,
                                   0, &len2);

    if (test_assert(encoded2 != NULL, "空字符串编码应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(len2 == 0, "空字符串编码长度应该为 0") != 0)
    {
        free(encoded2);
        return -1;
    }

    free(encoded2);

    /* 测试 3: 二进制数据 */

    printf("\n测试 3: 二进制数据\n");

    uint8_t test3[] = {0x00, 0xFF, 0x80, 0x7F};
    size_t len3;
    char *encoded3 = encode_base64(test3, sizeof(test3), &len3);

    if (test_assert(encoded3 != NULL, "二进制编码应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(strlen(encoded3) == len3, "编码长度应该一致") != 0)
    {
        free(encoded3);
        return -1;
    }

    free(encoded3);

    /* 测试 4: 长字符串 */

    printf("\n测试 4: 长字符串\n");

    char test4[1000];
    memset(test4, 'A', sizeof(test4) - 1);
    test4[sizeof(test4) - 1] = '\0';

    size_t len4;
    char *encoded4 = encode_base64((const unsigned char *)test4,
                                   strlen(test4), &len4);

    if (test_assert(encoded4 != NULL, "长字符串编码应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(len4 > 0, "长字符串编码长度应该大于 0") != 0)
    {
        free(encoded4);
        return -1;
    }

    free(encoded4);

    return 0;
}

/****************************************************************************
 * Name: test_local_asr
 *
 * Description:
 *   测试本地 ASR 识别
 *
 ****************************************************************************/

int test_local_asr(void)
{
    char result[256];
    int ret;

    /* 测试 1: 高能量音频（有语音） */

    printf("\n测试 1: 高能量音频（有语音）\n");

    int16_t loud_audio[1600];  /* 100ms at 16kHz */
    for (int i = 0; i < 1600; i++)
    {
        loud_audio[i] = (int16_t)(3000 * sin(i * 0.1));
    }

    ret = local_asr_test((const char *)loud_audio,
                         sizeof(loud_audio),
                         result, sizeof(result));

    if (test_assert(ret == 0, "高能量音频识别应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(strstr(result, "检测到语音") != NULL,
                    "应该检测到语音") != 0)
    {
        return -1;
    }

    /* 测试 2: 低能量音频（无语音） */

    printf("\n测试 2: 低能量音频（无语音）\n");

    int16_t silent_audio[1600];
    memset(silent_audio, 0, sizeof(silent_audio));

    ret = local_asr_test((const char *)silent_audio,
                         sizeof(silent_audio),
                         result, sizeof(result));

    if (test_assert(ret == 0, "静音音频识别应该成功") != 0)
    {
        return -1;
    }

    if (test_assert(strstr(result, "未检测到语音") != NULL,
                    "应该未检测到语音") != 0)
    {
        return -1;
    }

    /* 测试 3: 中等能量音频 */

    printf("\n测试 3: 中等能量音频\n");

    int16_t medium_audio[1600];
    for (int i = 0; i < 1600; i++)
    {
        medium_audio[i] = (int16_t)(500 * sin(i * 0.05));
    }

    ret = local_asr_test((const char *)medium_audio,
                         sizeof(medium_audio),
                         result, sizeof(result));

    if (test_assert(ret == 0, "中等能量音频识别应该成功") != 0)
    {
        return -1;
    }

    return 0;
}

/****************************************************************************
 * Name: test_voice_state_machine
 *
 * Description:
 *   测试语音状态机
 *
 ****************************************************************************/

int test_voice_state_machine(void)
{
    /* 测试 1: 初始状态 */

    printf("\n测试 1: 初始状态\n");

    voice_init();

    bool is_listening = voice_is_listening();
    if (test_assert(is_listening == false, "初始状态不应该在监听") != 0)
    {
        return -1;
    }

    /* 测试 2: 获取结果（未录音状态） */

    printf("\n测试 2: 获取结果（未录音状态）\n");

    const char *result = voice_get_result();
    if (test_assert(result != NULL, "应该能获取结果指针") != 0)
    {
        return -1;
    }

    /* 测试 3: 回调注册 */

    printf("\n测试 3: 回调注册\n");

    int ret = voice_register_callback(NULL, NULL);
    if (test_assert(ret == 0, "回调注册应该成功") != 0)
    {
        return -1;
    }

    return 0;
}
