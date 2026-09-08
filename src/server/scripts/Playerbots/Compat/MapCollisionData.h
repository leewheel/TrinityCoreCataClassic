#ifndef _ACMAPCOLLISIONDATA_H_
#define _ACMAPCOLLISIONDATA_H_

/*
 * AzerothCore的MapCollisionData是AC特有的碰撞数据管理
 * TrinityCore将碰撞数据集成到Map类中
 */

#include "Maps/Map.h"
#include "Collision/Management/VMapManager2.h"

// 碰撞数据在TC中由Map类直接管理
// 这个垫片只提供必要的类型定义，实际功能通过Map访问

#endif