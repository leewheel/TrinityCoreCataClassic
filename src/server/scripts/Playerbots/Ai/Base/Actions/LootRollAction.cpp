/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LootRollAction.h"

#include "Event.h"
#include "Group.h"
#include "ItemUsageValue.h"
#include "LootAction.h"
#include "Loot.h"
#include "ObjectMgr.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

//By leewheel 2026-09-06: 移植到TrinityCore-Cata
//Cata的RollVote是enum class，为保持AC原版代码写法(非限定名PASS/NEED等)在此补齐兼容常量
constexpr RollVote PASS = RollVote::Pass;
constexpr RollVote NEED = RollVote::Need;
constexpr RollVote GREED = RollVote::Greed;
constexpr RollVote DISENCHANT = RollVote::Disenchant;
constexpr RollVote NOT_EMITED_YET = RollVote::NotEmitedYet;
//End By leewheel

bool LootRollAction::Execute(Event /*event*/)
{
    //By leewheel 2026-09-06: 移植到TrinityCore-Cata
    //WotLK链路: Group::GetRolls() + Group::CountRollVote() → Cata重构:
    //roll列表由Player持有(Player::GetLootRolls兼容访问器)，投票提交走LootRoll::PlayerVote
    //End By leewheel
    bool voted = false;
    for (LootRoll* roll : bot->GetLootRolls())
    {
        if (!roll || !roll->IsStarted())
            continue;

        auto voteItr = roll->GetRollVoteMap().find(bot->GetGUID());
        if (voteItr == roll->GetRollVoteMap().end() || voteItr->second != NOT_EMITED_YET)
            continue;

        LootItem const* lootItem = roll->GetLootItem();
        if (!lootItem)
            continue;

        uint32 itemId = lootItem->itemid;
        int32 randomProperty = int32(lootItem->randomPropertiesId);

        RollVote vote = PASS;
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            continue;

        std::string itemUsageParam;
        if (randomProperty != 0)
            itemUsageParam = std::to_string(itemId) + "," + std::to_string(randomProperty);
        else
            itemUsageParam = std::to_string(itemId);

        ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", itemUsageParam);

        // Armor Tokens are classed as MISC JUNK (Class 15, Subclass 0), luckily no other items I found have class bits and epic quality.
        if (proto->GetClass() == ITEM_CLASS_MISC && proto->GetSubClass() == ITEM_SUBCLASS_JUNK && proto->GetQuality() == ITEM_QUALITY_EPIC)
        {
            if (CanBotUseToken(proto, bot))
                vote = NEED; // Eligible for "Need"
            else
                vote = GREED; // Not eligible, so "Greed"
        }
        else if (usage == ITEM_USAGE_DISENCHANT)
            vote = sPlayerbotAIConfig.lootRollDisenchant ? DISENCHANT : GREED;
        else
        {
            switch (proto->GetClass())
            {
                case ITEM_CLASS_WEAPON:
                case ITEM_CLASS_ARMOR:
                    //By leewheel 2026-08-24: 调整Roll投票策略更贴近真实玩家——
                    //(1) 白装/灰装(品质<绿装)不参与Roll直接PASS;
                    //(2) 比机器人当前装备好的/空槽可穿/替换损坏装备(EQUIP/REPLACE/BROKEN_EQUIP)需求(NEED);
                    //(3) 能穿但不如当前的(BAD_EQUIP)贪婪(GREED)。
                    if (proto->GetQuality() < ITEM_QUALITY_UNCOMMON)
                        vote = PASS;
                    else if (usage == ITEM_USAGE_REPLACE || usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_BROKEN_EQUIP)
                        vote = NEED;
                    else if (usage == ITEM_USAGE_BAD_EQUIP || usage != ITEM_USAGE_NONE)
                        vote = GREED;
                    //End By leewheel
                    break;
                case ITEM_CLASS_RECIPE:
                    if (!sPlayerbotAIConfig.lootRollRecipe)
                        vote = PASS;
                    else if (usage == ITEM_USAGE_SKILL)
                        vote = NEED;  // Bot can learn this recipe
                    else if (proto->GetBonding() != BIND_WHEN_PICKED_UP)
                        vote = GREED;  // BoE recipe bot can't learn - GREED for AH/trade
                    break;
                default:
                    if (StoreLootAction::IsLootAllowed(itemId, botAI))
                        vote = CalculateRollVote(proto, usage);
                    break;
            }
        }
        if (vote == NEED)
        {
            if (sPlayerbotAIConfig.lootNeedRollLevel == 0 || RollUniqueCheck(proto, bot))
                vote = PASS;
            else if (sPlayerbotAIConfig.lootNeedRollLevel == 1)
                vote = GREED;
        }
        else if (vote == GREED && !sPlayerbotAIConfig.lootGreedRollLevel)
            vote = PASS;

        //By leewheel 2026-09-06: 移植到TrinityCore-Cata，投票提交走LootRoll::PlayerVote
        //(替代WotLK的Group::CountRollVote链路，Cata的LootRoll内部处理广播与结算)
        if (roll->PlayerVote(bot, vote))
            voted = true;
        //End By leewheel
    }

    return voted;
}

RollVote LootRollAction::CalculateRollVote(ItemTemplate const* proto, ItemUsage usage)
{
    if (usage == ITEM_USAGE_NONE)
    {
        std::ostringstream out;
        out << proto->GetId();
        usage = AI_VALUE2(ItemUsage, "item usage", out.str());
    }

    RollVote needVote = PASS;
    //By leewheel 2026-08-24: 对齐新Roll策略——白/灰装不参与; 空槽可穿/比当前好/替换损坏的需求,
    //能穿但不如当前的贪婪。
    if (proto->GetQuality() < ITEM_QUALITY_UNCOMMON)
        return PASS;
    //End By leewheel
    switch (usage)
    {
        case ITEM_USAGE_EQUIP:
        case ITEM_USAGE_REPLACE:
        case ITEM_USAGE_BROKEN_EQUIP:
            needVote = NEED;
            break;
        case ITEM_USAGE_BAD_EQUIP:
            needVote = GREED;
            break;
        case ITEM_USAGE_GUILD_TASK:
            needVote = NEED;
            break;
        case ITEM_USAGE_SKILL:
        case ITEM_USAGE_USE:
        case ITEM_USAGE_AH:
        case ITEM_USAGE_VENDOR:
            needVote = GREED;
            break;
        case ITEM_USAGE_DISENCHANT:
            needVote = sPlayerbotAIConfig.lootRollDisenchant ? DISENCHANT : GREED;
            break;
        default:
            break;
    }

    return StoreLootAction::IsLootAllowed(proto->GetId(), GET_PLAYERBOT_AI(bot)) ? needVote : PASS;
}

bool MasterLootRollAction::isUseful() { return !botAI->HasActivePlayerMaster(); }

bool MasterLootRollAction::Execute(Event event)
{
    Player* bot = QueryItemUsageAction::botAI->GetBot();

    WorldPacket p(event.getPacket());  // TC SMSG_START_LOOT_ROLL (StartLootRoll)

    // By leewheel 2026-08-07: 重写——TC 3.4.3 的 SMSG_START_LOOT_ROLL 包是 StartLootRoll 结构:
    //   LootObj(8)+MapID(4)+RollTime(4)+ValidRolls(1)+LootRollIneligibleReason(4)+Method(1)=22字节
    //   + Item(LootItemData): bits(Type2+UIType3+CanTrade1)=1字节
    //     + ItemInstance: ItemID(4)+RandomPropertiesSeed(4)+RandomPropertiesID(4)+hasItemBonus bit(1)
    //       + ItemModList(WriteBits(size,6)=1字节 + size*5字节) + [ItemBonuses: Context(1)+size(4)+size*4]
    //     + Quantity(4)+LootItemType(1)+LootListID(1)
    // 原 AC 代码按扁平 36 字节读(creatureGuid+mapId+itemSlot+itemId+randomSuffix+randomPropertyId+count+timeout),
    // 在 TC 下全部错位(itemSlot/itemId 读到垃圾值), 且宏 SMSG_LOOT_START_ROLL 被错误映射到 SMSG_LOOT_ROLL
    // (掷骰结果广播 LootRollBroadcast) 导致从未正确触发。修复: 宏改映射 SMSG_START_LOOT_ROLL, 按 TC 布局解析。
    // 包体最小(ModList空+无Bonus): 22+1+13+1+4+1+1 = 43字节
    if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 43)
        return false;

    ObjectGuid creatureGuid;
    p >> creatureGuid;  // LootObj

    // 跳过 MapID(4)+RollTime(4)+ValidRolls(1)+LootRollIneligibleReason(4)+Method(1)
    p.rpos(p.rpos() + 14);

    // LootItemData 头部 bits(Type 2 + UIType 3 + CanTradeToTapList 1) → FlushBits 后 1 字节
    p.read_skip<uint8>();

    // ItemInstance
    int32 itemId = p.read<int32>();
    p.read<int32>();  // RandomPropertiesSeed
    p.read<int32>();  // RandomPropertiesID
    bool hasItemBonus = p.ReadBit();
    p.ResetBitPos();

    // ItemModList
    uint32 modCount = p.ReadBits(6);
    p.ResetBitPos();
    for (uint32 i = 0; i < modCount; ++i)
    {
        p.read<int32>();  // ItemMod::Value
        p.read<uint8>();  // ItemMod::Type
    }

    // ItemBonuses (可选)
    if (hasItemBonus)
    {
        p.read_skip<uint8>();  // Context
        uint32 bonusCount = p.read<uint32>();
        p.rpos(p.rpos() + bonusCount * 4);
    }

    p.read_skip<uint32>();  // Quantity
    p.read_skip<uint8>();   // LootItemType
    uint8 lootListId = p.read<uint8>();

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // TC: CountRollVote(playerGuid, lootObjectGuid, lootListId, choice)
    group->CountRollVote(bot->GetGUID(), creatureGuid, lootListId, CalculateRollVote(proto));

    return true;
}

bool CanBotUseToken(ItemTemplate const* proto, Player* bot)
{
    // Get the bitmask for the bot's class
    uint32 botClassMask = (1 << (bot->getClass() - 1));

    // Check if the bot's class is allowed to use the token
    if (proto->GetAllowableClass() & botClassMask)
        return true; // Bot's class is eligible to use this token

    return false; // Bot's class cannot use this token
}

bool RollUniqueCheck(ItemTemplate const* proto, Player* bot)
{
    // Count the total number of the item (equipped + in bags)
    uint32 totalItemCount = bot->GetItemCount(proto->GetId(), true);

    // Count the number of the item in bags only
    uint32 bagItemCount = bot->GetItemCount(proto->GetId(), false);

    // Determine if the unique item is already equipped
    bool isEquipped = (totalItemCount > bagItemCount);
    if (isEquipped && proto->HasFlag(ITEM_FLAG_UNIQUE_EQUIPPABLE))
        return true;  // Unique Item is already equipped
    else if (proto->HasFlag(ITEM_FLAG_UNIQUE_EQUIPPABLE) && (bagItemCount > 1))
        return true; // Unique item already in bag, don't roll for it
    return false; // Item is not equipped or in bags, roll for it
}

bool RollAction::Execute(Event event)
{
    std::string link = event.getParam();

    if (link.empty())
    {
        bot->DoRandomRoll(0,100);
        return false;
    }
    ItemIds itemIds = chat->parseItems(link);
    if (itemIds.empty())
        return false;
    uint32 itemId = *itemIds.begin();
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
    {
        return false;
    }
    std::string itemUsageParam;
    itemUsageParam = std::to_string(itemId);

    ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", itemUsageParam);
    switch (proto->GetClass())
    {
        case ITEM_CLASS_WEAPON:
        case ITEM_CLASS_ARMOR:
        if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE || usage == ITEM_USAGE_BAD_EQUIP)
        {
            bot->DoRandomRoll(0,100);
        }
    }
    return true;
}
