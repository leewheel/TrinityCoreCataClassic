// By leewheel 2026-07-08: 重写以适配TC的OutdoorPvP API
// TC的OPvPCapturePoint没有_capturePoint、GetMinValue()、GetSlider()、m_capturePointSpawnId等成员
// 改为使用spawnId从ObjectMgr获取GameObjectData和GameObjectTemplate来获取位置和半径信息
// End By leewheel

#include "NewRpgOutdoorPvP.h"
//By leewheel 20260709: 添加Playerbots.h以获取AREA_NAGRAND等兼容宏
#include "Playerbots.h"
//End By leewheel
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"
#include "GameObject.h"
#include "GameObjectData.h"
#include "Map.h"
#include "ObjectMgr.h"

bool NewRpgOutdoorPvpAction::Execute(Event)
{
    if (!bot->IsPvP())
    {
        botAI->rpgInfo.ChangeToIdle();
        return false;
    }
    if (IsWaitingForLastMove(MovementPriority::MOVEMENT_NORMAL) || !bot->IsOutdoorPvPActive())
        return false;

    uint32 zoneId = bot->GetZoneId();
    //By leewheel 20260709: TC的GetOutdoorPvPToZoneId需要Map*参数
    OutdoorPvP* outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(bot->GetMap(), zoneId);
    //End By leewheel
    if (!outdoorPvP || zoneId == AREA_NAGRAND)
    {
        botAI->rpgInfo.ChangeToIdle();
        return false;
    }

    OutdoorPvP::OPvPCapturePointMap const& capturePointMap = outdoorPvP->GetCapturePoints();

    NewRpgInfo& info = botAI->rpgInfo;
    auto* dataPtr = std::get_if<NewRpgInfo::OutdoorPvP>(&info.data);
    if (!dataPtr)
        return false;
    auto& data = *dataPtr;

    // By leewheel 2026-07-08: TC的OPvPCapturePoint没有_capturePoint等成员
    // 通过spawnId从ObjectMgr获取GameObjectData来获取位置信息
    GameObjectData const* goData = nullptr;
    GameObjectTemplate const* goTemplate = nullptr;
    ObjectGuid::LowType targetSpawnId = 0;

    // 如果已有目标spawnId，尝试查找对应的GameObjectData
    if (data.capturePointSpawnId && !capturePointMap.empty())
    {
        auto it = capturePointMap.find(data.capturePointSpawnId);
        if (it != capturePointMap.end())
        {
            goData = sObjectMgr->GetGameObjectData(data.capturePointSpawnId);
            if (goData)
            {
                goTemplate = sObjectMgr->GetGameObjectTemplate(goData->id);
                if (goTemplate && goTemplate->type == GAMEOBJECT_TYPE_CAPTURE_POINT)
                {
                    targetSpawnId = data.capturePointSpawnId;
                }
            }
        }
        if (!targetSpawnId)
            data.capturePointSpawnId = 0;
    }

    // 选择新的目标
    if (!targetSpawnId)
    {
        for (auto const& [spawnId, point] : capturePointMap)
        {
            GameObjectData const* candidateData = sObjectMgr->GetGameObjectData(spawnId);
            if (!candidateData)
                continue;

            GameObjectTemplate const* candidateTemplate = sObjectMgr->GetGameObjectTemplate(candidateData->id);
            if (!candidateTemplate || candidateTemplate->type != GAMEOBJECT_TYPE_CAPTURE_POINT)
                continue;

            // 选择第一个有效的夺旗点
            goData = candidateData;
            goTemplate = candidateTemplate;
            targetSpawnId = spawnId;
            break;
        }

        if (!targetSpawnId)
        {
            // TC_LOG_DEBUG("playerbots", "[NEW RPG] 机器人 {} 在区域 {} 没有找到有效的户外PvP夺旗点", bot->GetName(), zoneId);
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }

        data.capturePointSpawnId = targetSpawnId;
        // TC_LOG_DEBUG("playerbots", "[NEW RPG] 机器人 {} 选择了户外PvP目标 capturePointSpawnId {}", bot->GetName(), data.capturePointSpawnId);
    }

    if (!goData || !goTemplate)
        return false;

    // 使用GameObjectData的位置创建WorldPosition
    WorldPosition targetPos(goData->mapId, goData->posX(), goData->posY(), goData->posZ(), goData->orientation());

    // 获取夺旗点半径
    //By leewheel 2026-07-10: TC的capturePoint没有radius成员，使用InteractRadiusOverride(单位:百分之一)
    float radius = goTemplate->capturePoint.InteractRadiusOverride / 100.0f;
    //End By leewheel
    if (radius <= 0.0f)
        radius = 20.0f; // 默认半径

    // 检查是否在范围内
    float distToTarget = bot->GetDistance(targetPos);
    if (distToTarget > radius)
        return MoveFarTo(targetPos);

    return PatrolCapturePoint(targetPos, radius);
}

OPvPCapturePoint* NewRpgOutdoorPvpAction::SelectNewObjective(OutdoorPvP::OPvPCapturePointMap const& capturePointMap)
{
    // By leewheel 2026-07-08: TC的OPvPCapturePoint没有slider/threshold信息
    // 简化为随机选择一个夺旗点
    OPvPCapturePoint* objective = nullptr;

    if (capturePointMap.empty())
    {
        botAI->rpgInfo.ChangeToIdle();
        return objective;
    }

    // 随机选择一个夺旗点
    int randomIndex = urand(0, static_cast<int>(capturePointMap.size()) - 1);
    auto it = capturePointMap.begin();
    std::advance(it, randomIndex);
    objective = it->second.get();
    return objective;
}

//By leewheel 2026-09-03 修复C4100警告：TC的OPvPCapturePoint无slider/threshold信息，简化为随机选点后
//targetPos不再使用，显式省略参数名
bool NewRpgOutdoorPvpAction::PatrolCapturePoint(WorldPosition const& /*targetPos*/, float radius)
//End By leewheel
{
    if (IsWaitingForLastMove(MovementPriority::MOVEMENT_NORMAL))
        return false;

    // 随机暂停后再选择新的巡逻点
    if (urand(0, 2) == 0)
        return ForceToWait(urand(3000, 6000));

    float patrolRadius = radius * 0.8f;
    // By leewheel 2026-07-08: bot已在夺旗点附近，使用nullptr以bot当前位置为中心随机移动
    if (MoveRandomNear(patrolRadius, MovementPriority::MOVEMENT_NORMAL, nullptr))
        return true;

    return ForceToWait(urand(3000, 6000));
}
