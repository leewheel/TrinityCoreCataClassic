/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option) any later version.
 */

//By leewheel 2026-07-09: 适配TC的AreaTriggerEntry替代AC的AreaTrigger结构体
// AC使用struct AreaTrigger含直接成员(map,x,y,z,radius,orientation,length,width,height)
// TC使用AreaTriggerEntry(DBC条目)含方法访问器(x(),y(),z(),map(),radius(),orientation())
// TC的sObjectMgr->GetAreaTrigger()返回AreaTriggerStruct(传送数据)，不是位置数据
// 位置数据需要从sAreaTriggerStore.LookupEntry()获取
//End By leewheel

#include "WithinAreaTrigger.h"

#include "LastMovementValue.h"
#include "Playerbots.h"
#include "DBCStores.h"

bool WithinAreaTrigger::IsActive()
{
    LastMovement& movement = context->GetValue<LastMovement&>("last area trigger")->Get();
    if (!movement.lastAreaTrigger)
        return false;

    //By leewheel 2026-07-09: 使用sAreaTriggerStore获取位置数据，而非sObjectMgr->GetAreaTrigger
    AreaTriggerEntry const* at = sAreaTriggerStore.LookupEntry(movement.lastAreaTrigger);
    //End By leewheel
    if (!at)
        return false;

    //By leewheel 2026-09-06: 移植到TrinityCore-Cata，ObjectMgr的方法名为GetAreaTrigger
    if (!sObjectMgr->GetAreaTrigger(movement.lastAreaTrigger))
        return false;

    return IsPointInAreaTriggerZone(at, bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(),
                                    0.5f);
}

//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//Cata的AreaTriggerEntry字段为Pos(DBCPosition3D)/ContinentID/Radius/BoxYaw等直接成员，
//替代WotLK的map()/x()/y()/z()/radius()/orientation()方法访问器
bool WithinAreaTrigger::IsPointInAreaTriggerZone(AreaTriggerEntry const* atEntry, uint32 mapid, float x, float y, float z,
                                                 float delta)
{
    if (mapid != static_cast<uint32>(atEntry->ContinentID))
        return false;

    if (atEntry->Radius > 0)
    {
        // if we have radius check it
        float dist2 = (x - atEntry->Pos.X) * (x - atEntry->Pos.X) + (y - atEntry->Pos.Y) * (y - atEntry->Pos.Y) +
                      (z - atEntry->Pos.Z) * (z - atEntry->Pos.Z);
        if (dist2 > (atEntry->Radius + delta) * (atEntry->Radius + delta))
            return false;
    }
    else
    {
        // we have only extent

        // rotate the players position instead of rotating the whole cube, that way we can make a simplified
        // is-in-cube check and we have to calculate only one point instead of 4

        // 2PI = 360, keep in mind that ingame orientation is counter-clockwise
        double rotation = 2 * M_PI - atEntry->BoxYaw;
        double sinVal = sin(rotation);
        double cosVal = cos(rotation);

        float playerBoxDistX = x - atEntry->Pos.X;
        float playerBoxDistY = y - atEntry->Pos.Y;

        float rotPlayerX = float(atEntry->Pos.X + playerBoxDistX * cosVal - playerBoxDistY * sinVal);
        float rotPlayerY = float(atEntry->Pos.Y + playerBoxDistY * cosVal + playerBoxDistX * sinVal);

        // box edges are parallel to coordiante axis, so we can treat every dimension independently :D
        // By leewheel 2026-07-09: 修复AC代码bug - 原代码错误使用x/y/z坐标代替length/width/height尺寸
        float dz = z - atEntry->Pos.Z;
        float dx = rotPlayerX - atEntry->Pos.X;
        float dy = rotPlayerY - atEntry->Pos.Y;
        if ((fabs(dx) > atEntry->BoxLength / 2 + delta) || (fabs(dy) > atEntry->BoxWidth / 2 + delta) ||
            (fabs(dz) > atEntry->BoxHeight / 2 + delta))
        // End By leewheel
        {
            return false;
        }
    }

    return true;
}
//End By leewheel
