/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#ifndef PLAYERBOTS_LOOTVALUES_H
#define PLAYERBOTS_LOOTVALUES_H

#include "ItemUsageValue.h"
//By leewheel 2026-09-06: 移植到TrinityCore-Cata，LootMgr.h仅前向声明LootType，枚举值(LOOT_CORPSE等)定义在Loot.h中
#include "Loot.h"
#include "LootMgr.h"
#include "NamedObjectContext.h"
#include "Value.h"
//End By leewheel

class PlayerbotAI;

// Cheat class copy to hack into the loot system
class LootTemplateAccess
{
public:
    class LootGroup;  // A set of loot definitions for items (refs are not allowed inside)
    typedef std::vector<LootGroup> LootGroups;
    LootStoreItemList Entries;  // not grouped only
    LootGroups Groups;          // groups have own (optimized) processing, grouped entries go there
};

//                   itemId, entry
//By leewheel 2026-08-18: 移植 brighton-chi the-lab 6fb08816(索引掉落者修正#2626)——一个物品可能被多个生物/物体掉落,
//改用 unordered_multimap 保留全部掉落者(消费端 ItemDropListValue 已用 equal_range 遍历)
typedef std::unordered_multimap<uint32, int32> DropMap;
//End By leewheel

// Returns the loot map of all entries
class DropMapValue : public SingleCalculatedValue<DropMap*>
{
public:
    DropMapValue(PlayerbotAI* botAI) : SingleCalculatedValue(botAI, "drop map") {}

    static LootTemplateAccess const* GetLootTemplate(ObjectGuid guid, LootType type = LOOT_CORPSE);

    DropMap* Calculate() override;
};

// Returns the entries that drop a specific item
class ItemDropListValue : public SingleCalculatedValue<std::vector<int32>>, public Qualified
{
public:
    ItemDropListValue(PlayerbotAI* botAI) : SingleCalculatedValue(botAI, "item drop list") {}

    std::vector<int32> Calculate() override;
};

// Returns the items a specific entry can drop
class EntryLootListValue : public SingleCalculatedValue<std::vector<uint32>>, public Qualified
{
public:
    EntryLootListValue(PlayerbotAI* botAI) : SingleCalculatedValue(botAI, "entry loot list") {}

    std::vector<uint32> Calculate() override;
};

typedef std::unordered_map<ItemUsage, std::vector<uint32>> itemUsageMap;

class EntryLootUsageValue : public CalculatedValue<itemUsageMap>, public Qualified
{
public:
    EntryLootUsageValue(PlayerbotAI* botAI) : CalculatedValue(botAI, "entry loot usage", 2 * 1000) {}

    itemUsageMap Calculate() override;
};

class HasUpgradeValue : public BoolCalculatedValue, public Qualified
{
public:
    HasUpgradeValue(PlayerbotAI* botAI) : BoolCalculatedValue(botAI, "has upgrade", 2 * 1000) {}

    bool Calculate() override;
};

#endif
