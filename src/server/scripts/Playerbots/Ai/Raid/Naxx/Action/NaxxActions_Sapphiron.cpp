/*
 * 纳克萨玛斯 萨菲隆 Boss 动作策略
 *
 * 作者: leewheel
 */

#include "NaxxActions.h"

#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "NaxxBossHelper.h"
#include "NaxxSpellIds.h"

bool SapphironGroundPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (botAI->IsMainTank(bot))
    {
        if (AI_VALUE2(bool, "has aggro", "current target"))
            return MoveTo(NAXX_MAP_ID, helper.mainTankPos.first, helper.mainTankPos.second, helper.GENERIC_HEIGHT, false, false, false,
                          false, MovementPriority::MOVEMENT_COMBAT);

        return false;
    }
    if (helper.JustLanded())
    {
        uint32 index = botAI->GetGroupSlotIndex(bot);
        //By leewheel 2026-09-03 修复C4305警告：M_PI为double字面量，float初始化统一float常量
        float start_angle = 0.85f * float(M_PI);
        float offset_angle = float(M_PI) * 0.02f * index;
        //End By leewheel
        float angle = start_angle + offset_angle;
        float distance;
        if (botAI->IsRanged(bot))
            distance = 35.0f;
        else if (botAI->IsHeal(bot))
            distance = 30.0f;
        else
            distance = 5.0f;

        float posX = helper.center.first + cos(angle) * distance;
        float posY = helper.center.second + sin(angle) * distance;
        if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
            return true;

        return MoveInside(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, 2.0f, MovementPriority::MOVEMENT_COMBAT);
    }
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool SapphironFlightPositionAction::Execute(Event /*event*/)
{
    if (!helper.UpdateBossAI())
        return false;

    if (helper.WaitForExplosion())
        return MoveToNearestIcebolt();
    else
    {
        std::vector<float> dest;
        if (helper.FindPosToAvoidChill(dest))
            return MoveTo(NAXX_MAP_ID, dest[0], dest[1], dest[2], false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
    }
    return false;
}

bool SapphironFlightPositionAction::MoveToNearestIcebolt()
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Player* playerWithIcebolt = nullptr;
    float minDistance;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (NaxxSpellIds::HasAnyAura(member, {NaxxSpellIds::Icebolt10, NaxxSpellIds::Icebolt25}) ||
            botAI->HasAura("icebolt", member, false, false, -1, true))
        {
            if (!playerWithIcebolt || minDistance > bot->GetDistance(member))
            {
                playerWithIcebolt = member;
                minDistance = bot->GetDistance(member);
            }
        }
    }
    if (playerWithIcebolt)
    {
        Unit* boss = AI_VALUE2(Unit*, "find target", "15989");
        if (boss)
        {
            float angle = boss->GetAngle(playerWithIcebolt);
            float posX = playerWithIcebolt->GetPositionX() + cos(angle) * 3.0f;
            float posY = playerWithIcebolt->GetPositionY() + sin(angle) * 3.0f;
            if (MoveTo(NAXX_MAP_ID, posX, posY, helper.GENERIC_HEIGHT, false, false, false, false, MovementPriority::MOVEMENT_COMBAT))
                return true;

            return MoveNear(playerWithIcebolt, 3.0f, MovementPriority::MOVEMENT_COMBAT);
        }
    }
    return false;
}
