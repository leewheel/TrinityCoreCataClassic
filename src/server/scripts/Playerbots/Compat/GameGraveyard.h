#ifndef _ACGAMEGRAVEYARD_H_
#define _ACGAMEGRAVEYARD_H_

/*
 * AzerothCore的GameGraveyard是AC特有的墓地管理器
 * TrinityCore使用ObjectMgr管理WorldSafeLocsEntry
 */

#include "ObjectGuid.h"
#include "ObjectMgr.h"

struct GraveyardStruct
{
    uint32 ID;
    uint32 Map;
    float x;
    float y;
    float z;
    std::string name;
};

// TC中不直接提供Graveyard类，墓地数据在ObjectMgr中
class Graveyard
{
public:
    static Graveyard* instance()
    {
        static Graveyard inst;
        return &inst;
    }

    // 从TC的WorldSafeLocsEntry构造GraveyardStruct
    //By leewheel 2026-08-15: 改按值返回——原static缓冲每次调用覆盖(别名bug)，
    //调用方持有时内容已被后续查询改写，复活选错墓地。按值返回可安全持有跨多次查询
    GraveyardStruct GetGraveyard(uint32 ID) const
    {
        WorldSafeLocsEntry const* safeLoc = sObjectMgr->GetWorldSafeLoc(ID);
        if (!safeLoc)
            return GraveyardStruct();

        GraveyardStruct graveyard;
        graveyard.ID = safeLoc->ID;
        graveyard.Map = safeLoc->Loc.GetMapId();
        graveyard.x = safeLoc->Loc.GetPositionX();
        graveyard.y = safeLoc->Loc.GetPositionY();
        graveyard.z = safeLoc->Loc.GetPositionZ();
        graveyard.name = "";

        return graveyard;
    }
    //End By leewheel

    //By leewheel 2026-07-11: 实现GetClosestGraveyard使用TC的API
    //By leewheel 2026-08-15: 改按值返回(见GetGraveyard注释)
    //By leewheel 2026-09-03 修复C4100警告：nearCorpse参数在此实现中未使用，显式省略参数名
    GraveyardStruct GetClosestGraveyard(Player* player, TeamId teamId, bool /*nearCorpse*/ = false)
    //End By leewheel
    {
        if (!player)
            return GraveyardStruct();
        
        // TC使用uint32 team (ALLIANCE/HORDE)，AC使用TeamId枚举
        uint32 team = (teamId == TEAM_ALLIANCE) ? ALLIANCE : HORDE;
        
        WorldLocation loc(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());
        WorldSafeLocsEntry const* safeLoc = sObjectMgr->GetClosestGraveyard(loc, team, player);
        if (!safeLoc)
            return GraveyardStruct();
        
        // 构造GraveyardStruct
        GraveyardStruct graveyard;
        graveyard.ID = safeLoc->ID;
        graveyard.Map = safeLoc->Loc.GetMapId();
        graveyard.x = safeLoc->Loc.GetPositionX();
        graveyard.y = safeLoc->Loc.GetPositionY();
        graveyard.z = safeLoc->Loc.GetPositionZ();
        graveyard.name = "";
        
        return graveyard;
    }
    //End By leewheel

    //By leewheel 2025-01-16
    // AC兼容: 多参数版本的GetClosestGraveyard
    //By leewheel 2026-08-15: 改按值返回(见GetGraveyard注释)
    //By leewheel 2026-09-03 修复C4100警告：areaId/zoneId/isDK参数在此实现中未使用，显式省略参数名
    GraveyardStruct GetClosestGraveyard(uint32 mapId, float x, float y, float z, TeamId teamId, uint32 /*areaId*/ = 0, uint32 /*zoneId*/ = 0, bool /*isDK*/ = false)
    //End By leewheel
    {
        WorldLocation loc(mapId, x, y, z, 0.0f);
        // TC使用GetTeam()返回Team(ALLIANCE/HORDE)而不是TeamId
        uint32 team = (teamId == TEAM_ALLIANCE) ? ALLIANCE : HORDE;
        WorldSafeLocsEntry const* safeLoc = sObjectMgr->GetClosestGraveyard(loc, team, nullptr);
        if (!safeLoc)
            return GraveyardStruct();

        GraveyardStruct graveyard;
        graveyard.ID = safeLoc->ID;
        graveyard.Map = safeLoc->Loc.GetMapId();
        graveyard.x = safeLoc->Loc.GetPositionX();
        graveyard.y = safeLoc->Loc.GetPositionY();
        graveyard.z = safeLoc->Loc.GetPositionZ();
        graveyard.name = "";

        return graveyard;
    }
    //End By leewheel 2025-01-16
    //End By leewheel 2026-08-15
};

#define sGraveyard Graveyard::instance()

#endif