# Knowledge Cards 测试

## 当前固件语音测试

`run_runtime_tests.sh` 直接编译固件正在使用的 `src/kc_voice.c`，并以 Mock
替代板端 Media Recorder、配置存储和火山 ASR 网络 I/O。

```bash
cd /home/mi/openvela/packages/demos/knowledge_cards/tests
./run_runtime_tests.sh
```

覆盖内容：

- 未配置凭据时进入 `KC_VOICE_CONFIG_REQUIRED`
- 保存并读取 App ID、Token 和默认集群
- 异步开始录音并进入 `KC_VOICE_RECORDING`
- 录音参数为 16 kHz、16 bit、单声道
- 录音中拒绝重复启动
- 异步停止录音并调用 ASR
- 中文 UTF-8 识别文本进入 `KC_VOICE_RESULT`

## Legacy 测试

`run_tests.sh` 中的 11 个用例覆盖早期的 `llm_client.c`、
`card_manager.c`、`voice_recognizer.c` 和 `wifi_utils.c`。这些文件目前未加入
BES2800BP 固件构建，因此这些测试只作为后续迁移参考，不能代表当前板端语音
实现的覆盖率。

```bash
./run_tests.sh
```

## 尚需板上验证

主机单测不能覆盖以下硬件和外部服务行为：

- BES 麦克风和 `audio` 核 Media 服务
- Wi-Fi、DNS、TLS 握手
- 火山引擎真实账号权限和识别准确率
- 454 px LCD 的实际裁切、触摸坐标和长按手感

这些项目需要重新接板后进行集成测试。

## 多主题卡组加载（当前固件）

运行 `bash run_deck_tests.sh`，直接编译 `kc_deck.c` 并使用真实 cJSON。
覆盖操作系统与英语切换、失败保留旧卡组、非法路径、无效 JSON、空卡组、字段类型错误、尾随数据、超量卡片与正文超长。
启用 ASan/UBSan；不覆盖 LCD 视觉与真机语音链路。
