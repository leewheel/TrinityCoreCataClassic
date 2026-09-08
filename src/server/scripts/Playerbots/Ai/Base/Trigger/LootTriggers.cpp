/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LootTriggers.h"

#include "LootObjectStack.h"
#include "Playerbots.h"
#include "ServerFacade.h"

bool LootAvailableTrigger::IsActive()
{
    //By leewheel 2026-08-01: 移植loot管道死锁修复(4851a7f1)——无可拾取物时直接返回false
    if (!AI_VALUE(bool, "has available loot"))
        return false;
    //End By leewheel

    bool distanceCheck = false;
    if (botAI->HasStrategy("stay", BOT_STATE_NON_COMBAT))
    {
        distanceCheck =
            ServerFacade::instance().IsDistanceLessOrEqualThan(AI_VALUE2(float, "distance", "loot target"), CONTACT_DISTANCE);
    }
    else
    {
        distanceCheck = ServerFacade::instance().IsDistanceLessOrEqualThan(AI_VALUE2(float, "distance", "loot target"),
                                                                 INTERACTION_DISTANCE - 2.0f);
    }

    // Loot target in range, or no hostile targets to deal with first.
    if (distanceCheck || AI_VALUE(GuidVector, "all targets").empty())
        return true;

    //By leewheel 2026-08-01: 移植loot管道死锁修复(4851a7f1)——目标在选中后变得不可拾取
    //(途中被拉入战斗、或目标消失)，永远无法回到拾取范围；若不在此上报active，
    //该失效目标会一直保留，拾取动作再也不会选中新目标。此处上报active以让拾取动作丢弃它。
    LootObject lootTarget = AI_VALUE(LootObject, "loot target");
    return !lootTarget.IsEmpty() && !lootTarget.IsLootPossible(bot);
    //End By leewheel
}

bool FarFromCurrentLootTrigger::IsActive()
{
    LootObject loot = AI_VALUE(LootObject, "loot target");
    if (!loot.IsLootPossible(bot))
        return false;

    return AI_VALUE2(float, "distance", "loot target") >= INTERACTION_DISTANCE - 2.0f;
}

bool CanLootTrigger::IsActive() { return AI_VALUE(bool, "can loot"); }
