---
name: knowledge-cards-author
description: 为 openvela 知识闪卡制作或更新任意学习主题的本地 JSON 卡组，校验小显示屏排版和当前固件的数据边界。
---

# 知识卡组制作与校验

用户给出主题或学习材料时，产出 `app/knowledge_cards/decks/<主题>.json`。主题可为编程、语言、科学等，CSAPP 仅为示例。内容依据用户材料或可核实知识，不把未经验证的说法写成事实。

使用对象 `{"topic":"主题", "cards":[{"title":"标题", "content":"短句\n短句"}]}`。文件名去掉 `.json` 即为设备查找键，语音输入需与此名称一致。禁止路径分隔符和控制字符。

每个卡组 1–50 张。标题最多 63 字节，正文最多 511 字节（均按 UTF-8 字节计算，不能截断汉字）；建议每张 3–4 行、每行约 15 个汉字，减少显示屏换行与溢出。16 px 字库覆盖基本汉字和 ASCII，生僻字、emoji 需另行验证。

用 Python `json.load` 及 `len(text.encode('utf-8'))` 校验文件和长度；运行 `bash app/knowledge_cards/tests/run_deck_tests.sh` 验证加载器。若仓库不在 openvela 工作区根下一层，设置 `OPENVELA_ROOT`。不要把主机测试描述成真机显示测试。

部署见仓库 README；设备 `/data` 当前为 tmpfs，重启需重新导入。不能声称提供了在线 LLM 自动生卡，除非当前固件已集成并测试该能力。
