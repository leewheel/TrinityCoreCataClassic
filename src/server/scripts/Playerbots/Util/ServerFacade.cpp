/*
 * 服务器外观模式
 * 从 AzerothCore mod-playerbots 移植到 TrinityCore
 * 适配 TrinityCore 的 API 差异
 */

#include "ServerFacade.h"
#include "Player.h"
#include "Object.h"
#include "Unit.h"
#include "WorldSession.h"

#include "MotionMaster.h"
#include "MovementDefines.h"
#include "MovementGenerator.h"
#include "AbstractPursuer.h"
#include "ChaseMovementGenerator.h"

float ServerFacade::GetDistance2d(Unit* unit, WorldObject* wo)
{
    ASSERT_NOTNULL(unit);
    ASSERT_NOTNULL(wo);

    float dist = unit->GetDistance2d(wo);
    return std::round(dist * 10.0f) / 10.0f;
}

float ServerFacade::GetDistance2d(Unit* unit, float x, float y)
{
    float dist = unit->GetDistance2d(x, y);
    return std::round(dist * 10.0f) / 10.0f;
}

bool ServerFacade::IsDistanceLessThan(float dist1, float dist2)
{
    return dist1 < dist2;
}

bool ServerFacade::IsDistanceGreaterThan(float dist1, float dist2)
{
    return dist1 > dist2;
}

bool ServerFacade::IsDistanceGreaterOrEqualThan(float dist1, float dist2) { return !IsDistanceLessThan(dist1, dist2); }

bool ServerFacade::IsDistanceLessOrEqualThan(float dist1, float dist2) { return !IsDistanceGreaterThan(dist1, dist2); }

void ServerFacade::SetFacingTo(Player* bot, WorldObject* wo, bool /*force*/)
{
    if (!bot)
        return;

    //By leewheel 2026-07-20: 使用TC的SetFacingToObject替代SetOrientation
    //SetOrientation只设服务端数值，不发送移动包给客户端，也不更新移动样条系统
    //SetFacingToObject通过MoveSplineInit正确更新面向并广播，force=true确保移动中也能转向
    bot->SetFacingToObject(wo, true);
    //End By leewheel
}

Unit* ServerFacade::GetChaseTarget(Unit* target)
{
    MovementGenerator* movementGen = target->GetMotionMaster()->GetCurrentMovementGenerator();
    if (movementGen && movementGen->GetMovementGeneratorType() == CHASE_MOTION_TYPE)
    {
        ChaseMovementGenerator* chaseGen = dynamic_cast<ChaseMovementGenerator*>(movementGen);
        if (chaseGen)
            return chaseGen->GetTarget();
    }

    return nullptr;
}

void ServerFacade::SendPacket(Player* player, WorldPacket* packet)
{
    player->GetSession()->SendPacket(packet);
}
