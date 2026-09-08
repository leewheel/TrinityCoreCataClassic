/*
 * AC到TC兼容性头文件垫片
 * 将AzerothCore特有的头文件名映射到TrinityCore的对应头文件
 */

#ifndef PLAYERBOTS_COMPAT_HEADERS_H
#define PLAYERBOTS_COMPAT_HEADERS_H

// DBC → DB2 映射 (TrinityCore使用DB2格式替代DBC)
#include "DataStores/DB2Stores.h"
#include "DataStores/DB2Structure.h"

// MapMgr → MapManager
#include "Maps/MapManager.h"

// IVMapMgr → IVMapManager (TC中接口名不同)
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，WotLK的IVMapManager.h在Cata更名为VMapManager.h
#include "Collision/Management/VMapManager.h"
//End By leewheel

//By leewheel 2026-09-06: 移植到TrinityCore-Cata，Cata核心移除了SkillType枚举，在Compat层补齐
#include "SkillDefinesCompat.h"
//End By leewheel

// VMap相关定义
#include "Collision/VMapDefinitions.h"

// 脚本系统 - TC在ScriptMgr.h中定义了所有脚本基类
#include "Scripting/ScriptMgr.h"

// 地形管理
#include "Maps/TerrainMgr.h"

// 对象访问器
#include "Globals/ObjectAccessor.h"

// 世界管理
#include "World/World.h"

// 对象管理器（含墓地数据）
#include "Globals/ObjectMgr.h"

//By leewheel 2026-08-15: 修复——原#define sWorldSessionMgr (nullptr)是地雷：若某TU先include
//本头文件再include WorldSessionMgr.h，宏展开为nullptr会使PlayerbotMgr等所有会话查找空指针崩溃。
//sWorldSessionMgr的正确实现是Compat/WorldSessionMgr.h的WorldSessionMgr::instance()(转发sWorld->FindSession)。
//本头文件不再定义该宏，避免与WorldSessionMgr.h冲突
//End By leewheel

#endif // PLAYERBOTS_COMPAT_HEADERS_H