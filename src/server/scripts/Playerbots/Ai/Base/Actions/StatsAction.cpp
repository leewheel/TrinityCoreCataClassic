/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "StatsAction.h"
//By leewheel 20260709: 添加Playerbots.h以获取PLAYER_NEXT_LEVEL_XP等兼容宏
#include "Playerbots.h"
//End By leewheel

#include "ChatHelper.h"
#include "Event.h"
#include "PlayerbotAI.h"

bool StatsAction::Execute(Event /*event*/)
{
    std::ostringstream out;

    ListGold(out);

    out << ", ";
    ListBagSlots(out);

    out << ", ";
    ListRepairCost(out);

    if (bot->GetUInt32Value(PLAYER_NEXT_LEVEL_XP))
    {
        out << ", ";
        ListXP(out);
    }

    botAI->TellMaster(out);
    return true;
}

void StatsAction::ListGold(std::ostringstream& out) { out << chat->formatMoney(bot->GetMoney()); }

void StatsAction::ListBagSlots(std::ostringstream& out)
{
    uint32 totalused = 0, total = 16;

    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            ++totalused;
        }
    }

    uint32 totalfree = 16 - totalused;

    // list out items in other removable backpacks
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        if (Bag const* pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag))
        {
            ItemTemplate const* pBagProto = pBag->GetTemplate();
            if (pBagProto->GetClass() == ITEM_CLASS_CONTAINER && pBagProto->GetSubClass() == ITEM_SUBCLASS_CONTAINER)
            {
                total += pBag->GetBagSize();
                totalfree += pBag->GetFreeSlots();
            }
        }
    }

    std::string color = "ff00ff00";
    if (totalfree < total / 2)
        color = "ffffff00";

    if (totalfree < total / 4)
        color = "ffff0000";

    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "|h|c" << color << totalfree << "/" << total << "|h|cffffffff 背包格";
    //End By leewheel
}

void StatsAction::ListXP(std::ostringstream& out)
{
    uint32 curXP = bot->GetUInt32Value(PLAYER_XP);
    uint32 nextLevelXP = bot->GetUInt32Value(PLAYER_NEXT_LEVEL_XP);
    uint32 restXP = bot->GetUInt32Value(PLAYER_REST_STATE_EXPERIENCE);
    uint32 xpPercent = 0;

    if (nextLevelXP)
        xpPercent = 100 * curXP / nextLevelXP;

    uint32 restPercent = 0;
    if (restXP && nextLevelXP)
        restPercent = 2 * (100 * restXP / nextLevelXP);

    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "|cff00ff00" << xpPercent << "|cffffd333/|cff00ff00" << restPercent << "%|cffffffff 经验";
    //End By leewheel
}

void StatsAction::ListRepairCost(std::ostringstream& out)
{
    uint32 totalCost = 0;
    double repairPercent = 0;
    double repairCount = 0;

    for (uint32 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        uint16 pos = ((INVENTORY_SLOT_BAG_0 << 8) | i);
        totalCost += EstRepair(pos);
        double repair = RepairPercent(pos);
        if (repair < 100)
        {
            repairPercent += repair;
            ++repairCount;
        }
    }

    repairPercent /= repairCount;

    std::string color = "ff00ff00";
    if (repairPercent < 50)
        color = "ffffff00";

    if (repairPercent < 25)
        color = "ffff0000";

    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "|c" << color << (uint32)ceil(repairPercent) << "% (" << chat->formatMoney(totalCost) << ")|cffffffff 耐久";
    //End By leewheel
}

uint32 StatsAction::EstRepair(uint16 pos)
{
    Item* item = bot->GetItemByPos(pos);

    uint32 TotalCost = 0;
    if (!item)
        return TotalCost;

    //By leewheel 20260710: TC使用GetMaxDurability()/GetDurability()替代GetUInt32Value
    uint32 maxDurability = item->GetMaxDurability();
    //End By leewheel
    if (!maxDurability)
        return TotalCost;

    //By leewheel 20260710: TC使用GetDurability()替代GetUInt32Value
    uint32 curDurability = item->GetDurability();
    //End By leewheel

    uint32 LostDurability = maxDurability - curDurability;
    if (LostDurability > 0)
    {
        ItemTemplate const* ditemProto = item->GetTemplate();

        //By leewheel 20260710: TC使用GetItemLevel()替代直接成员访问
        DurabilityCostsEntry const* dcost = sDurabilityCostsStore.LookupEntry(ditemProto->GetItemLevel());
        //End By leewheel
        if (!dcost)
        {
            //By leewheel 20260710: TC使用GetItemLevel()替代直接成员访问
            TC_LOG_ERROR("playerbots", "RepairDurability: Wrong item lvl {}", ditemProto->GetItemLevel());
            //End By leewheel
            return TotalCost;
        }

        //By leewheel 20260710: TC使用GetQuality()替代直接成员访问
        uint32 dQualitymodEntryId = (ditemProto->GetQuality() + 1) * 2;
        //End By leewheel
        DurabilityQualityEntry const* dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
        if (!dQualitymodEntry)
        {
            TC_LOG_ERROR("playerbots", "RepairDurability: Wrong dQualityModEntry {}", dQualitymodEntryId);
            return TotalCost;
        }

        //By leewheel 20260710: TC使用WeaponSubClassCost/ArmorSubClassCost替代multiplier数组
        uint32 dmultiplier = 0;
        if (ditemProto->GetClass() == ITEM_CLASS_WEAPON)
            dmultiplier = dcost->WeaponSubClassCost[ditemProto->GetSubClass()];
        else if (ditemProto->GetClass() == ITEM_CLASS_ARMOR)
            dmultiplier = dcost->ArmorSubClassCost[ditemProto->GetSubClass()];
        //End By leewheel
        //By leewheel 20260710: TC使用Data替代quality_mod
        uint32 costs = uint32(LostDurability * dmultiplier * double(dQualitymodEntry->Data));
        //End By leewheel

        if (!costs)  // fix for ITEM_QUALITY_ARTIFACT
            costs = 1;

        TotalCost = costs;
    }

    return TotalCost;
}

double StatsAction::RepairPercent(uint16 pos)
{
    Item* item = bot->GetItemByPos(pos);
    if (!item)
        return 100;

    //By leewheel 20260710: TC使用GetMaxDurability()/GetDurability()替代GetUInt32Value
    uint32 maxDurability = item->GetMaxDurability();
    //End By leewheel
    if (!maxDurability)
        return 100;

    //By leewheel 20260710: TC使用GetDurability()替代GetUInt32Value
    uint32 curDurability = item->GetDurability();
    //End By leewheel
    if (!curDurability)
        return 0;

    return curDurability * 100.0 / maxDurability;
}
