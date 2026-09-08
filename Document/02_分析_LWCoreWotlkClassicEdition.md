# 02 分析：LWCoreWotlkClassicEdition（移植基座）

作者：leewheel ｜ 日期：2026-09-06

## 版本
- TrinityCore 3.4.3（WotLK Classic / 3.4.3 客户端），README 明示 "World of Warcraft 3.4.3 Source code"
- 源码根：`E:\1.ClassicCataSlym\LWCoreWotlkClassicEdition`

## 核心价值：已完成的 AC→TC(WotLK) 移植
LWCore 的 `src/server/scripts/Playerbots/` 是 **leewheel 本人 2026-07 把 mod-playerbots 移植到 TrinityCore(WotLK) 的成果**：
- 规模：681 cpp / 785 h（比 AC 原版 646/728 更大，含额外自研功能）。
- 结构：`Ai/ Bot/ Db/ Mgr/ Script/ Util/ Compat/` + `Playerbots.h PlayerbotAIConfig.* LfgPlayerbots.h AreaDefines.h`。
- 全中文注释、`//By leewheel 日期` 标记，符合本项目规范。

## Compat 兼容层（AC 风格 API → TC 映射）
`src/server/scripts/Playerbots/Compat/`：
- `AllScriptCompat.h` `CompatHeaders.h` —— 总兼容头
- `DBCStores.h` `DBCStructure.h` —— AC 的 DBC 名映射到 TC
- `MapMgr.h` `VMapMgr2.h` `IVMapMgr.h` `MapCollisionData.h` `GridTerrainData.h` —— 碰撞/地图 API 映射
- `WorldSessionMgr.h/.cpp` —— AC 的 sWorldSessionMgr → TC 的 sWorld
- `AllCreatureScript.h` `DynamicObjectScript.h` `RaceMgr.h` `GameGraveyard.h` `PlayerbotsWPPCompat.h`

## 脚本注册（TC 风格，见 Script/Playerbots.cpp）
- 注册类：`PlayerbotsDatabaseScript(DatabaseScript)`、`PlayerbotsWorldScript(WorldScript)`、`PlayerbotsPlayerScript(PlayerScript)`、`PlayerbotsMiscScript(MiscScript)`、`PlayerbotsServerScript(ServerScript)`、`PlayerBotsBGScript(AllBattlegroundScript)`。
- TC 构造无 hook 列表；`AddPlayerbotsScripts()` 内 `new XxxScript();` 注册。
- 额外：`AddPlayerbotsSecureLoginScripts()`、`AddPlayerbotsCommandscripts()`、`PlayerBotsGuildValidationScript()`、副本 bot 脚本等。

## 构建集成（见 src/server/scripts/CMakeLists.txt:221-233）
- `file(GLOB_RECURSE PLAYERBOTS_ALL_HEADERS ...)` 递归收集 Playerbots 下所有目录，逐个加入 `target_include_directories(scripts PRIVATE ...)`。
- Playerbots 作为 scripts 静态库的一部分编译。

## 核心层支撑改动（34 文件，需在 TC-Cata 复刻）
关键项：
- `WorldSession.h/.cpp`：`bool _isBot=false;` + `IsBot()/SetBot()`；`LoginQueryHolder` 移入头文件供继承；`GetPacketQueue` 改公开。
- `Scripting/ScriptMgr.h/.cpp`：把 `OnPlayerbot*` 9 个虚方法合并进 `PlayerScript`（默认空实现），并在 `ScriptMgr` 加对应派发方法；LFG 前向声明 `lfg::Lfg5Guids`。
- `database/Database/`：`DatabaseEnvFwd.h` 加 Playerbots 全套别名；`DatabaseEnv.h/.cpp` 加 `PlayerbotsDatabase` 全局；`DatabaseLoader.h/.cpp` 加 `DATABASE_PLAYERBOTS=16`；`DatabaseWorkerPool.cpp`、`DBUpdater.cpp` 注册；新增 `Implementation/PlayerbotsDatabase.{h,cpp}`。
- 各 Handler/Entity 的 bot 判断：`Player.cpp/.h`、`Unit.cpp/.h`、`ObjectAccessor`、`ChatHandler`、`World.cpp/.h`、`Spell.cpp/.h`、`SpellMgr.cpp/.h`、`Map.cpp/.h`、`LFGQueue.cpp`、`Guild.h`、`Item.h`、`ObjectGuid.h`、`RBAC.h`、`Trainer.h`、`QuestDef.h`、`OutdoorPvP.h`、`BattlegroundAV.h`、`PetitionsHandler.cpp`、`AuctionHouseMgr.cpp`、`Creature.cpp`、`InstanceScript.cpp`、`TaxiHandler.cpp`、`TradeHandler.cpp`、`MailHandler.cpp`、`MiscHandler.cpp`、`CharacterHandler.cpp`、`AuctionHouseHandler.cpp`、`WorldserverFriendsService.cpp`、`SpellAuras.cpp`、`SpellScript.cpp`、`FlightPathMovementGenerator.cpp`、`WaypointMovementGenerator.cpp`。

## 移植决策
以 LWCore 的 TC(WotLK) Playerbots 为基座移植到 TC-Cata，而非直接翻译 AC 版。理由：同源 TrinityCore，脚本系统/DB 层/实体 API 结构一致，差异集中在 WotLK→Cata 的天赋/法术/DBC 语义与少量签名。
