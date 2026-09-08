/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "AddLootAction.h"

#include "AiObjectContext.h"
#include "CellImpl.h"
#include "Event.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "LootObjectStack.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "ServerFacade.h"

bool AddLootAction::Execute(Event event)
{
    ObjectGuid guid = event.getObject();
    if (!guid)
        return false;

    return AI_VALUE(LootObjectStack*, "available loot")->Add(guid);
}

bool AddAllLootAction::Execute(Event /*event*/)
{
    bool added = false;

    GuidVector gos = context->GetValue<GuidVector>("nearest game objects")->Get();
    for (GuidVector::iterator i = gos.begin(); i != gos.end(); i++)
        added |= AddLoot(*i);

    GuidVector corpses = context->GetValue<GuidVector>("nearest corpses")->Get();
    for (GuidVector::iterator i = corpses.begin(); i != corpses.end(); i++)
        added |= AddLoot(*i);

    return added;
}

bool AddLootAction::isUseful() { return true; }

bool AddAllLootAction::isUseful() { return true; }

//By leewheel 2026-07-23: 增加技能过滤，防止无采集技能的Bot被引导到采集节点后卡死
//原代码直接添加所有GO，不检查bot是否有能力交互，导致牧师等无采矿技能的Bot
//被反复引导到矿节点→采集失败→清理→重新添加→死循环
bool AddAllLootAction::AddLoot(ObjectGuid guid)
{
    LootObject loot(bot, guid);
    if (!loot.IsEmpty())
    {
        // 有技能要求的采集节点（采矿/采药/剥皮），但bot无法采集时跳过
        if (loot.skillId != SKILL_NONE && !loot.IsLootPossible(bot))
            return false;
    }
    return AI_VALUE(LootObjectStack*, "available loot")->Add(guid);
}
//End By leewheel

bool AddGatheringLootAction::AddLoot(ObjectGuid guid)
{
    LootObject loot(bot, guid);

    WorldObject* wo = loot.GetWorldObject(bot);
    if (loot.IsEmpty() || !wo)
        return false;

    if (loot.skillId == SKILL_NONE)
        return false;

    if (!loot.IsLootPossible(bot))
        return false;

    return AddAllLootAction::AddLoot(guid);
}
