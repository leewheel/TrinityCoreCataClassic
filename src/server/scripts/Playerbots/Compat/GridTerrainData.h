#ifndef _ACGRIDTERRAINDATA_H_
#define _ACGRIDTERRAINDATA_H_

/*
 * AzerothCore的GridTerrainData是AC特有的地形数据管理
 * TrinityCore使用TerrainMgr
 */

#include "Maps/TerrainMgr.h"

#define MAX_HEIGHT 100000.0f
#define INVALID_HEIGHT -100000.0f
#define MAX_FALL_DISTANCE 250000.0f
#define MIN_HEIGHT -500.0f

// 液体状态定义
//By leewheel 2026-09-03 修复C4005警告：TC原生MapDefines.h已定义MAP_LIQUID_STATUS_SWIMMING/IN_CONTACT（基于LIQUID_MAP枚举的精确表达式），
//垫片的裸值0x3/0x7与其冲突且语义不同；加#ifndef保护让TC原生定义生效，仅作AC垫片兜底
#ifndef MAP_LIQUID_STATUS_SWIMMING
#define MAP_LIQUID_STATUS_SWIMMING 0x3
#endif
#ifndef MAP_LIQUID_STATUS_IN_CONTACT
#define MAP_LIQUID_STATUS_IN_CONTACT 0x7
#endif
//End By leewheel

// 液体类型定义
//By leewheel 2026-09-03 同上：加#ifndef保护避免与Playerbots.h/TC原生定义重复
#ifndef MAP_LIQUID_TYPE_NO_WATER
#define MAP_LIQUID_TYPE_NO_WATER 0x00
#endif
#ifndef MAP_LIQUID_TYPE_WATER
#define MAP_LIQUID_TYPE_WATER 0x01
#endif
#ifndef MAP_ALL_LIQUIDS
#define MAP_ALL_LIQUIDS 0x0F
#endif
//End By leewheel

#endif