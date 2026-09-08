# 项目规则文件 — mod-playerbots 移植到 TrinityCore CataClassic

> 作者：leewheel
> 本文件是本项目的最高工作准则，所有工作必须严格遵守。

## 任务目标

1. 分析 `E:\1.ClassicCataSlym\TrinityCoreCataClassic`（目标核心，TrinityCore cata_classic 分支，4.3.5  Cataclysm）
2. 分析 `E:\1.ClassicCataSlym\LWCoreWotlkClassicEdition`（参考核心，WotLK 3.3.5）
3. 分析 `E:\1.ClassicCataSlym\azerothcore-wotlk` 及其 `modules\mod-playerbots`（移植来源，AzerothCore WotLK 3.3.5）
4. 把 `mod-playerbots` 移植到 `TrinityCoreCataClassic`

## 绝对禁止的行为

以下行为在任何情况下都绝对禁止，违反任何一条都是严重错误：

000. **绝对禁止使用任何自动化脚本、工具批量修改代码。** 所有代码修改必须手动逐处进行（Edit/Write 工具逐文件修改）。禁止用 sed/awk/正则批量替换脚本改代码。
1. **禁止停下来问"要不要继续"** —— 永远继续，直到老大手动叫停。
2. **禁止停下来问"选哪个方案"** —— 自己分析，自己决定，立刻执行。
3. **禁止停下来问"该怎么处理"** —— 自己分析代码，自己做决定，立刻修复。
4. **禁止停下来汇报进度求确认** —— 直接干活，不需要确认。
5. **禁止在回复末尾提出选项让老大选择** —— 不要给选项，直接执行最优方案。
6. **禁止说"任务太大"** —— 不管 28216 条错误还是 282160 条错误，按顺序逐个修，一个都不放过。
7. **禁止说"要不要先做简单的"** —— 不允许偷懒跳过困难部分，按顺序硬啃。
8. **禁止在遇到不确定时停下来** —— 自己查代码、查 API、做决定，然后继续修复。
9. **禁止在回复末尾出现任何形式的"请确认""请指示""您希望我"等语句** —— 这等于停下来。
10. **禁止用任何理由中断工作流** —— 包括但不限于"需要更多信息""需要用户输入"等借口。

## 工作原则

- 凡事必须先分析清楚前因后果、代码细节、全局了解后，再执行动作。
- 所有工作中产生的文件（分析文档、日志、临时脚本、SQL 整理等）都必须保存在 `Document` 目录，保持项目目录的干净整洁。
- 随时记录工作日志到 `Document/工作日志.md`。
- **所有注释必须为中文**，所有游戏 log 输出必须为中文。
- **所有代码都要写出作者：leewheel**。
- **修改代码必须注释，而且头尾加上 `//By leewheel 年月日` 与 `//End By leewheel`**。
- 任务不管工作量多大，在老大手动结束之前不允许停下来，务必持续完成任务。
- 所有玩家能学的技能、天赋，机器人也要按照本系统（TrinityCore CataClassic）的天赋系统修改。
- 种族技能默认给机器人学会本种族默认技能。

## 移植技术路线（自定，不得更改方向除非分析证明错误）

1. **构建集成**：TrinityCore 没有 AzerothCore 的 module loader。将 mod-playerbots 作为静态脚本集成进 `src/server/scripts/Custom/playerbots`，纳入 worldserver 编译。
2. **脚本系统适配**：AC 的 `PlayerScript`/`WorldScript`/`ScriptMgr` hook 签名与 TC 不同，逐个适配。
3. **天赋/技能系统适配**：Cata 4.3.4 天赋树、双天赋、spell ID 与 WotLK 不同，需按 TC 的 `SpellMgr`/`Player::GetTalentSpellCost`/`TalentSpellPos` 重写。
4. **数据库适配**：`playerbots_*` 表迁移到 TC 的 character DB，SQL 结构对齐 TC。
5. **编译驱动修复**：以编译错误清单为准，按文件顺序逐个硬啃，直到通过编译。

## 工作日志索引

- `Document/01_分析_TrinityCoreCataClassic.md`
- `Document/02_分析_LWCoreWotlkClassicEdition.md`
- `Document/03_分析_azerothcore与mod-playerbots.md`
- `Document/04_API差异映射表.md`
- `Document/工作日志.md`
