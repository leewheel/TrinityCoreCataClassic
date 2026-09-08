/*
 * 移植来源: AC azerothcore-wotlk mod-playerbots 提交 a773b8c6 (Naxx Heigan 安全舞)
 * 移植适配 TC 框架
 * 业务对标: AC azerothcore-wotlk mod-playerbots
 * 作者: leewheel
 */

#include "NaxxActions.h"
#include "LastMovementValue.h"
#include "Playerbots.h"

bool HeiganDanceAction::MoveToWaypoint(uint32 index, float distance)
{
    if (index >= HeiganBossHelper::WaypointCount)
        return false;

    if (int32(index) != lastWaypoint)
    {
        // 新安全点：清除之前的移动（同优先级），停止当前施法
        lastWaypoint = int32(index);
        AI_VALUE(LastMovement&, "last movement").Set(nullptr);
        bot->CastStop();
    }

    return MoveInside(bot->GetMapId(), HeiganBossHelper::WaypointX[index], HeiganBossHelper::WaypointY[index],
                      bot->GetPositionZ(), distance, MovementPriority::MOVEMENT_COMBAT);
}

bool HeiganDanceAction::MoveToPlatform(float distance)
{
    if (lastWaypoint != -1)
    {
        lastWaypoint = -1;
        AI_VALUE(LastMovement&, "last movement").Set(nullptr);
    }

    return MoveInside(bot->GetMapId(), HeiganBossHelper::PlatformX, HeiganBossHelper::PlatformY,
                      HeiganBossHelper::PlatformZ, distance, MovementPriority::MOVEMENT_COMBAT);
}

bool HeiganDanceAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    // 慢速阶段：平台不受喷发影响。远程在最后一次慢速喷发后离开平台，在快速阶段开始前到达第一个快速安全点
    if (ranged)
        return helper.ShouldRangedHoldPlatform() ? MoveToPlatform(2.0f)
                                                 : MoveToWaypoint(helper.GetSafeWaypoint(), 1.5f);

    // Boss跟随主坦克，所以坦克必须先拉住仇恨才能带领跳舞
    if (!helper.ShouldDance())
        return false;

    return MoveToWaypoint(helper.GetSafeWaypoint(), PlayerbotAI::IsMainTank(bot) ? 0.5f : 1.5f);
}
//End By leewheel 2026-08-30