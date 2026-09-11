/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LootValues.h"

#include "Playerbots.h"
#include "SharedValueContext.h"

LootTemplateAccess const* DropMapValue::GetLootTemplate(ObjectGuid guid, LootType type)
{
    LootTemplate const* lTemplate = nullptr;

    if (guid.IsCreature())
    {
        CreatureTemplate const* info = sObjectMgr->GetCreatureTemplate(guid.GetEntry());

        if (info)
        {
            if (type == LOOT_CORPSE)
                //By leewheel 2026-09-09: TC-Cata使用CreatureTemplate_GetLootID兼容函数获取loot ID
                lTemplate = LootTemplates_Creature.GetLootFor(CreatureTemplate_GetLootID(info));
                //End By leewheel
            //By leewheel 2026-09-09: TC-Cata的pickpocket/skin loot在CreatureDifficulty中
            //使用兼容函数以避免GetDifficulty宏冲突
            else if (type == LOOT_PICKPOCKETING)
            {
                uint32 lootId = CreatureTemplate_GetPickPocketLootID(info);
                if (lootId)
                    lTemplate = LootTemplates_Pickpocketing.GetLootFor(lootId);
            }
            else if (type == LOOT_SKINNING)
            {
                uint32 lootId = CreatureTemplate_GetSkinLootID(info);
                if (lootId)
                    lTemplate = LootTemplates_Skinning.GetLootFor(lootId);
            }
            //End By leewheel
        }
    }
    else if (guid.IsGameObject())
    {
        GameObjectTemplate const* info = sObjectMgr->GetGameObjectTemplate(guid.GetEntry());
        if (info && info->GetLootId() != 0)
        {
            if (type == LOOT_CORPSE)
                lTemplate = LootTemplates_Gameobject.GetLootFor(info->GetLootId());
            else if (type == LOOT_FISHINGHOLE)
                lTemplate = LootTemplates_Fishing.GetLootFor(info->GetLootId());
        }
    }
    else if (guid.IsItem())
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(guid.GetEntry());

        if (proto)
        {
            if (type == LOOT_CORPSE)
                lTemplate = LootTemplates_Item.GetLootFor(proto->GetId());
            else if (type == LOOT_DISENCHANTING && ItemTemplate_GetDisenchantID(proto))
                lTemplate = LootTemplates_Disenchant.GetLootFor(ItemTemplate_GetDisenchantID(proto));
            if (type == LOOT_MILLING)
                lTemplate = LootTemplates_Milling.GetLootFor(proto->GetId());
            if (type == LOOT_PROSPECTING)
                lTemplate = LootTemplates_Prospecting.GetLootFor(proto->GetId());
        }
    }

    LootTemplateAccess const* lTemplateA = reinterpret_cast<LootTemplateAccess const*>(lTemplate);

    return lTemplateA;
}

DropMap* DropMapValue::Calculate()
{
    DropMap* dropMap = new DropMap;

    int32 sEntry = 0;

    //By leewheel 2026-07-10: TC中GetCreatureTemplates返回引用而非指针
    CreatureTemplateContainer const& creatures = sObjectMgr->GetCreatureTemplates();
    for (auto const& itr : creatures)
    //End By leewheel
        {
            //By leewheel 2026-07-10: range-based for的itr是pair引用，用.而非->
            sEntry = itr.first;
            //End By leewheel

            //By leewheel 2026-07-10: TC中使用ObjectGuid::Create<HighGuid::X>(entry)创建GUID
            //By leewheel 2026-07-10: TC的ObjectGuid::Create<HighGuid::Creature>需要3个参数: mapId, entry, counter
            if (LootTemplateAccess const* lTemplateA = GetLootTemplate(ObjectGuid::Create<HighGuid::Creature>(0, sEntry, sEntry), LOOT_CORPSE))
            {
                //End By leewheel
                //End By leewheel
                for (auto const& lItem : lTemplateA->Entries)
                    dropMap->insert(std::make_pair(lItem->itemid, sEntry));
            }
        }

    //By leewheel 2026-07-10: TC中GetGameObjectTemplates返回引用而非指针
    GameObjectTemplateContainer const& gameobjects = sObjectMgr->GetGameObjectTemplates();
    //End By leewheel
    for (auto const& itr : gameobjects)
    {
        sEntry = itr.first;

        //By leewheel 2026-07-10: TC中使用ObjectGuid::Create<HighGuid::X>(entry)创建GUID
        //By leewheel 2026-07-10: TC的ObjectGuid::Create<HighGuid::GameObject>需要3个参数: mapId, entry, counter
        if (LootTemplateAccess const* lTemplateA = GetLootTemplate(ObjectGuid::Create<HighGuid::GameObject>(0, sEntry, sEntry), LOOT_CORPSE))
        //End By leewheel
        //End By leewheel
            for (auto const& lItem : lTemplateA->Entries)
                dropMap->insert(std::make_pair(lItem->itemid, -sEntry));
    }

    return dropMap;
}

// What items does this entry have in its loot list?
std::vector<int32> ItemDropListValue::Calculate()
{
    uint32 itemId = stoi(getQualifier());

    DropMap* dropMap = GAI_VALUE(DropMap*, "drop map");

    std::vector<int32> entries;

    auto range = dropMap->equal_range(itemId);

    for (auto itr = range.first; itr != range.second; ++itr)
        entries.push_back(itr->second);

    return entries;
}

// What items does this entry have in its loot list?
std::vector<uint32> EntryLootListValue::Calculate()
{
    int32 entry = stoi(getQualifier());

    std::vector<uint32> items;

    LootTemplateAccess const* lTemplateA;

    if (entry > 0)
        lTemplateA = DropMapValue::GetLootTemplate(ObjectGuid::Create<HighGuid::Creature>(0, entry, uint32(1)), LOOT_CORPSE); // By leewheel 2026-07-08
    else
        lTemplateA =
            //By leewheel 2026-07-10: TC的ObjectGuid::Create<HighGuid::GameObject>需要3个参数: mapId, entry, counter
            DropMapValue::GetLootTemplate(ObjectGuid::Create<HighGuid::GameObject>(0, static_cast<uint32>(-entry), uint32(1)), LOOT_CORPSE);
            //End By leewheel

    if (lTemplateA)
        for (auto const& lItem : lTemplateA->Entries)
            items.push_back(lItem->itemid);

    return items;
}

itemUsageMap EntryLootUsageValue::Calculate()
{
    itemUsageMap items;

    for (auto itemId : GAI_VALUE2(std::vector<uint32>, "entry loot list", getQualifier()))
    {
        items[AI_VALUE2(ItemUsage, "item usage", itemId)].push_back(itemId);
    }

    return items;
};

bool HasUpgradeValue::Calculate()
{
    itemUsageMap uMap = AI_VALUE2(itemUsageMap, "entry loot usage", getQualifier());
    return uMap.find(ITEM_USAGE_EQUIP) != uMap.end() || uMap.find(ITEM_USAGE_REPLACE) != uMap.end();
}
