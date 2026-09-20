############################################################################
# packages/demos/knowledge_cards/tests/Makefile
#
# 知识闪卡单元测试 Makefile
#
############################################################################

# 工具配置

CC = gcc
CFLAGS = -Wall -Wextra -g -O0
LDFLAGS = -lm

# 测试源文件

TEST_SRCS = test_main.c \
            test_llm_client.c \
            test_card_manager.c \
            test_voice_recognizer.c \
            test_wifi_utils.c

# Mock 实现

MOCK_SRCS = test_mocks.c

# 被测源文件 (从 src/ 中编译)

SRC_DIR = ../src
SRC_SRCS = $(SRC_DIR)/llm_client.c \
           $(SRC_DIR)/card_manager.c \
           $(SRC_DIR)/voice_recognizer.c \
           $(SRC_DIR)/wifi_utils.c

# cJSON 源文件 (使用项目内的 cJSON)

CJSON_DIR = ../../../../apps/netutils/cjson/cJSON
CJSON_SRC = $(CJSON_DIR)/cJSON.c

# 头文件搜索路径

INCLUDES = -I/tmp \
           -I$(CJSON_DIR) \
           -I../include

# 目标

TARGET = knowledge_cards_tests

# 对象文件

TEST_OBJS = $(TEST_SRCS:.c=.o)
MOCK_OBJS = $(MOCK_SRCS:.c=.o)
SRC_OBJS  = $(SRC_SRCS:.c=.o)
CJSON_OBJ = $(CJSON_SRC:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(TEST_OBJS) $(MOCK_OBJS) $(SRC_OBJS) $(CJSON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# 测试文件编译 (使用简化头文件)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 源文件编译

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -I/tmp -c $< -o $@

# cJSON 编译

$(CJSON_DIR)/%.o: $(CJSON_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 运行测试

run: $(TARGET)
	./$(TARGET)

# 清理

clean:
	rm -f $(TEST_OBJS) $(MOCK_OBJS) $(SRC_OBJS) $(CJSON_OBJ)
	rm -f $(TARGET)
