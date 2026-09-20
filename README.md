# 知镜 KnowLens · 知识闪卡 UI 原型

面向学生、程序员和碎片化学习者的知识闪卡 UI 原型。当前阶段完成 BES2800BP 上的初步界面展示：显示安全区、知识卡片、标题、正文、逐行显示、点击展开和左右翻页。固件使用内置演示文本，**尚未接入麦克风、云端 AI 或 AI 自动生成知识卡**。

队伍：欣欣向荣　|　参赛者：吴荣飞（GitHub：`lknt`，负责全部工作）

## 当前完成范围

- 基于 openvela/NuttX 和 LVGL 的 454×454 显示屏 UI 原型。
- 中心显示安全面板、知识卡、标题/正文/分页布局。
- 正文逐行显示、点击卡片展开、左右按钮和滑动翻页。
- 中文字体编译进固件，使用预置演示文本验证排版。
- AP 固件可完成编译和链接。

## 尚未完成、计划后续实现

- 麦克风硬件接入、录音和真实语音输入。
- WiFi 配网、云端 ASR 和网络异常处理。
- 接入大模型，根据用户主题自动生成知识卡。
- 可变主题导入、持久化卡组、学习记录和间隔重复。
- 真机帧率、功耗、长时间运行和完整交互验收。

仓内保留了语音、JSON 卡组和 LLM 的实验性代码，供后续迭代参考；这些代码不代表当前原型已经具备对应产品功能。

## 构建与烧录

```sh
cd /home/mi/openvela
python3 contest2026_302_xinxinxiangrong/scripts/prepare_workspace.py "$PWD" --check
python3 contest2026_302_xinxinxiangrong/scripts/prepare_workspace.py "$PWD"
bash contest2026_302_xinxinxiangrong/board/bes2800bp/1700_ap.sh
```

生成：`cmake_out/best1700_ep/aos_evb/out/nuttx_ap.bin`。

```sh
sudo vendor/bes/prebuild/m1/dldtool -v /dev/ttyUSB0 \
  vendor/bes/prebuild/programmer1700_dual.bin \
  -M cmake_out/best1700_ep/aos_evb/out/nuttx_ap.bin
```

烧录后按 POWER/RESET 重启，串口使用 921600 baud、8N1、无流控：

```sh
picocom --baud 921600 --databits 8 --parity n --stopbits 1 \
  --flow n --imap lfcrlf --noinit --noreset /dev/ttyUSB0
```

当前启动后观察 UI 原型和触控交互即可。没有可验收的麦克风语音操作，不要把 `knowledge_cards --set-asr` 当作当前功能步骤。

## 测试

```sh
bash contest2026_302_xinxinxiangrong/app/knowledge_cards/tests/run_runtime_tests.sh
bash contest2026_302_xinxinxiangrong/app/knowledge_cards/tests/run_deck_tests.sh
```

这些是实验性语音/卡组模块的主机 Mock 和单元测试，不是麦克风、云端 AI 或真实 UI 的硬件验收。当前提交的核心验收是源码构建和 UI 初步展示。

## 目录

- `app/knowledge_cards/`：UI 应用、字体和后续实验代码。
- `board/bes2800bp/`：板级配置快照和补丁。
- `docs/TECHNICAL_REPORT.md`：按大赛模板编写的正式技术报告。
- `docs/evidence/`：构建与实验性测试记录。
- `skills/`：后续卡组制作 Skill 草案。
- `logs/`：AI Coding 日志，包含 prompt、回复和工具事件。

## 提交说明

完整报告、演示视频录制脚本和材料清单位于 `/home/mi/backup/2026-09-20/报告/`。官网提交前还需补充真实 UI 演示视频和 BES2800BP 实物照片。源码与 AI Coding 日志从 GitHub 参赛仓获取。
