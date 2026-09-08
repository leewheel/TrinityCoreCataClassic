# 01 分析：TrinityCoreCataClassic（移植目标核心）

作者：leewheel ｜ 日期：2026-09-06

## 版本
- TrinityCore `cata_classic` 分支（Cataclysm Classic，4.3.5/4.4.x 客户端）
- git 最新提交：`a2fcb5101a Merge branch 'TrinityCore:cata_classic'`
- 源码根：`E:\1.ClassicCataSlym\TrinityCoreCataClassic`

## 构建系统（脚本编译）
- 脚本目录：`src/server/scripts/`，每个子目录自动成为一个"脚本模块"。
- 模块自动发现：`cmake/macros/ConfigureScripts.cmake` 的 `GetScriptModuleList()` 用 `file(GLOB ${BASE_PATH}/*)` 枚举目录。
- 加载函数命名规则：目录名 `Xxx` → 函数 `AddXxxScripts()`，由 `ScriptLoader.cpp.in.cmake` 生成 `AddScripts()` 统一调用。
- 源码收集：`cmake/macros/AutoCollect.cmake` 的 `CollectAndAddSourceFiles()` **递归**收集子目录 cpp，并把每个子目录的 h 加入 `FILE_SET HEADERS`（BASE_DIRS=该子目录），因此头文件可按文件名直接 include。
- 静态链接：`add_library(scripts STATIC)`，最终链入 worldserver。
- 已有 `src/server/scripts/Custom/custom_script_loader.cpp`，内含空的 `AddCustomScripts()`。

## 数据库层（与 LWCore 同源）
- `DatabaseLoader(std::string const& logger, uint32 const defaultUpdateMask)` —— 构造第二参数即 updateMask（与 LWCore 一致）。
- `enum DatabaseTypeFlags { DATABASE_NONE=0, DATABASE_LOGIN=1, DATABASE_CHARACTER=2, DATABASE_WORLD=4, DATABASE_HOTFIX=8, DATABASE_MASK_ALL=15 }` —— **无 DATABASE_PLAYERBOTS**，需新增=16。
- `DatabaseEnvFwd.h` 有 Character/Hotfix/Login/World 四种 Connection 的 PreparedStatement/Transaction/QueryHolder 别名 —— 需补 Playerbots 一套。
- `Implementation/` 下仅 4 个库 —— 需补 `PlayerbotsDatabase.{h,cpp}`。

## 脚本系统
- `src/server/game/Scripting/ScriptMgr.h`：TC 的脚本基类 `PlayerScript / WorldScript / ServerScript / MiscScript / DatabaseScript / BGScript / BattlefieldScript / CommandScript`。
- TC 脚本构造**不接受 hook 列表参数**（AC 的 `{PLAYERHOOK_ON_LOGIN,...}` 在 TC 不存在），直接 override 虚函数即可。
- 注册：`new XxxScript();` 构造即注册到 sScriptMgr。

## 关键 API（WotLK→Cata 差异风险点，待逐个核对）
- 天赋：Cata 4.3.4 有双天赋、新天赋树、`Player::GetTalentSpellCost`、`TalentSpellPos`、`PlayerTalent`、`m_talentGroup`。
- 法术：`SpellMgr::GetSpellInfo(uint32)` 返回 `SpellInfo const*`；Cata 的 SpellIconID/SpellCategory/Coef 与 WotLK 有差异。
- 日志：`LOG_INFO("category", "fmt {}", args)`（TC 用 fmt 风格 `{}`）。
- 配置：`sConfigMgr->GetBoolDefault / GetFloatDefault / GetIntValue / GetStringValue`。

## 移植集成点（结论）
1. 新建脚本模块目录 `src/server/scripts/Playerbots/`（自动被 GLOB 发现，加载函数 `AddPlayerbotsScripts()`）。
2. 数据库层新增 Playerbots 库（照 LWCore 改法）。
3. 核心层复刻 LWCore 的 34 个 playerbots 支撑改动（WorldSession::IsBot、ScriptMgr::PlayerbotScript、各 Handler bot 判断等）。
