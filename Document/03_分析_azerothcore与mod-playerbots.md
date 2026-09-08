# 03 分析：azerothcore-wotlk 与 mod-playerbots（移植来源）

作者：leewheel ｜ 日期：2026-09-06

## azerothcore-wotlk
- AzerothCore，WotLK 3.3.5 分支。模块系统：`modules/` 下每模块含 `include.sh`，由 AC 的 module loader（`modules/CMakeLists.txt` + `ModulesLoader.cpp.in.cmake`）动态/静态加载。
- 脚本注册宏：`Addmod_playerbotsScripts()`。

## mod-playerbots 规模
- 646 cpp / 728 h，约 17.9 万行 cpp。
- 目录：`src/{Ai/{Base,Class,Dungeon,Raid,World}, Bot/{Cmd,Debug,Engine,Factory}, Db, Mgr/{Guild,Item,Move,Security,Talent,Text,Travel}, Script, Util}`。

## AC 专有依赖（移植时须替换为 TC 等价物）
1. **脚本系统**：AC 的 `PlayerScript/WorldScript/ServerScript/MiscScript/DatabaseScript/CommandScript/BGScript/BattlefieldScript` 构造带 hook 列表 `{PLAYERHOOK_ON_LOGIN,...}`；TC 无此参数。
2. **PlayerbotScript**：AC 版自定义脚本基类（LWCore 已合并进 PlayerScript）。
3. **数据库**：AC 的 `PlayerbotsDatabase` + `DatabaseLoader::DATABASE_PLAYERBOTS` + `SetUpdateFlags` + `GetOption<bool>`；TC 用构造传 updateMask + `GetBoolDefault`。
4. **WorldSession::IsBot()**：AC 原生有；TC 需自行加。
5. **事件系统**：AC 的 `player->m_Events.AddEventAtOffset(...)`；TC 用 `AddEventAtOffset`/`Events`（签名不同）。
6. **LFG**：AC 的 `lfg::Lfg5Guids`；TC 的 LFG 结构不同。
7. **GuildTaskMgr**：AC 专有。
8. **可见性**：AC 的 `GetObjectVisibilityContainer().CleanVisibilityReferences()`。
9. **天赋/法术**：AC 基于 WotLK 3.3.5 天赋树与 spell ID。

## 结论
AC 版依赖大量 AC-only 接口，直接翻译到 TC-Cata 工作量巨大且易错。LWCore 已完成 AC→TC(WotLK) 这一半，本项目只需完成 TC(WotLK)→TC(Cata) 这一半，风险与工作量都显著更低。

## 保真度验证（2026-09-06 老大指示：基础功能必须以AC原版为准，LWCore仅作参考）
- 文件级对比：AC 1374 个源文件中仅 1 个缺失（SethData.h → 实际被改名为 SethShared.h，常量齐全）；
  移植版另有 95 个纯新增文件（PvpCombat/SWP/MgT/HFR/FastGroup等扩展功能，不替换AC基础功能）。
- 内容级抽查：
  - Talentspec.cpp 差异 74/509 行：TalentTabID→ID、tabpage→OrderIndex、RankID→SpellRank、玩家文本中文化（符合规则）。
  - WarriorActions.cpp 差异 50/281 行：IsClass→Cata API 等适配。
  - PlayerbotAI.cpp 差异约 1928 行：含头文件重组+日志清理+TC适配，核心逻辑保留。
- 结论：当前模块内容 = AC mod-playerbots 完整功能 + TC-Cata 适配 + 扩展，符合"基础功能从AC获取"的要求。
- 待办：天赋 spell ID 表(Talentspec.cpp/各职业AI)仍为WotLK数据，需按 Cata 4.3.4 天赋系统修正（老大规则要求）。
