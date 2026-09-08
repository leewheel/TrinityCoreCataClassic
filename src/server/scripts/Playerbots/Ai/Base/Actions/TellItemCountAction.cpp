/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "TellItemCountAction.h"

#include "Event.h"
#include "ItemCountValue.h"
#include "Playerbots.h"

bool TellItemCountAction::Execute(Event event)
{
    std::string const text = event.getParam();
    std::vector<Item*> found = parseItems(text);
    std::map<uint32, uint32> itemMap;
    std::map<uint32, bool> soulbound;

    for (Item* item : found)
    {
        ItemTemplate const* proto = item->GetTemplate();
        itemMap[proto->GetId()] += item->GetCount();
        soulbound[proto->GetId()] = item->IsSoulBound();
    }

    //By leewheel 2026-08-01: 玩家可见文本中文化
    botAI->TellMaster("=== 背包物品 ===");
    //End By leewheel
    for (std::map<uint32, uint32>::iterator i = itemMap.begin(); i != itemMap.end(); ++i)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(i->first);
        TellItem(proto, i->second, soulbound[i->first]);
    }

    return true;
}
