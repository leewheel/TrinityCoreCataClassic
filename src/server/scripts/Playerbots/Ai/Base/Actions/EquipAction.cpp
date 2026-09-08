/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "Playerbots.h"

#include "EquipAction.h"
#include <utility>

#include "Event.h"
#include "ItemCountValue.h"
#include "ItemUsageValue.h"
#include "ItemVisitors.h"
#include "StatsWeightCalculator.h"
#include "ItemPackets.h"
#include "Item.h"
#include "Bag.h"

bool EquipAction::Execute(Event event)
{
    std::string const text = event.getParam();
    ItemIds ids = chat->parseItems(text);
    EquipItems(ids);
    return true;
}

void EquipAction::EquipItems(ItemIds ids)
{
    for (ItemIds::iterator i = ids.begin(); i != ids.end(); i++)
    {
        FindItemByIdVisitor visitor(*i);
        EquipItem(&visitor);
    }
}

// Return bagslot with smalest bag.
uint8 EquipAction::GetSmallestBagSlot()
{
    int8 curBag = 0;
    uint32 curSlots = 0;
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        const Bag* const pBag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
        if (pBag)
        {
            if (curBag > 0 && curSlots < pBag->GetBagSize())
                continue;

            curBag = bag;
            curSlots = pBag->GetBagSize();
        }
        else
            return bag;
    }

    return curBag;
}

void EquipAction::EquipItem(FindItemVisitor* visitor)
{
    IterateItems(visitor);
    std::vector<Item*> items = visitor->GetResult();
    if (!items.empty())
        EquipItem(*items.begin());
}

void EquipAction::EquipItem(Item* item)
{
    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();
    const ItemTemplate* itemProto = item->GetTemplate();
    uint32 itemId = itemProto->GetId();
    uint8 invType = itemProto->GetInventoryType();

    // Handle ammunition separately
    if (invType == INVTYPE_AMMO)
    {
        bot->SetAmmo(itemId);
        std::ostringstream out;
        //By leewheel 2026-08-01: 玩家可见文本中文化
        out << "正在装备 " << chat->FormatItem(itemProto);
        //End By leewheel
        botAI->TellMaster(out);
        return;
    }

    // Handle bags first
    bool equippedBag = false;
    if (itemProto->GetClass() == ITEM_CLASS_CONTAINER)
    {
        // Attempt to equip as a bag
        uint8 newBagSlot = GetSmallestBagSlot();

        if (newBagSlot > 0)
        {
            uint16 src = ((bagIndex << 8) | slot);
            uint16 dst = ((INVENTORY_SLOT_BAG_0 << 8) | newBagSlot);
            bot->SwapItem(src, dst);
            equippedBag = true;
        }
    }

    // If we didn't equip as a bag, try to equip as gear
    if (!equippedBag)
    {
        // Ranged weapons aren't handled by the rest of the weapon equip logic
        // Handle them early here to avoid issues.
        if (invType == INVTYPE_RANGED || invType == INVTYPE_THROWN || invType == INVTYPE_RANGEDRIGHT)
        {
            //By leewheel 2026-07-12: 使用WPPCompat替代手动构造WorldPacket(TC的包结构不同)
            ObjectGuid itemguid = item->GetGUID();
            WPPCompat::AutoEquipItemSlot(bot->GetSession(), itemguid, EQUIPMENT_SLOT_RANGED);
            //End By leewheel

            std::ostringstream out;
            //By leewheel 2026-08-01: 玩家可见文本中文化
            out << "正在装备 " << chat->FormatItem(itemProto) << " 到远程栏位";
            //End By leewheel
            botAI->TellMaster(out);
            return;
        }

        uint8 dstSlot = botAI->FindEquipSlot(itemProto, NULL_SLOT, true);

        //By leewheel 2026-08-02: 严谨化装备——目标槽位已装备同id物品时不重复装备
        //(防背包多件同装备反复"正在装备"刷屏占AI——如信仰手套22517已穿戴, 背包副本不再装)
        if (dstSlot != NULL_SLOT)
        {
            if (Item* equipped = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, dstSlot))
            {
                if (equipped->GetTemplate()->GetId() == itemId)
                    return;
            }
        }
        //End By leewheel

        // Check if the item is a weapon and whether the bot can dual wield or use Titan Grip
        bool isWeapon = (itemProto->GetClass() == ITEM_CLASS_WEAPON);
        bool canTitanGrip = bot->CanTitanGrip();
        bool canDualWield = bot->CanDualWield();

        bool isTwoHander = (invType == INVTYPE_2HWEAPON);
        bool isValidTGWeapon = false;
        if (canTitanGrip && isTwoHander)
        {
            // Titan Grip-valid 2H weapon subclasses: Axe2, Mace2, Sword2
            isValidTGWeapon = (itemProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_AXE2 ||
                               itemProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_MACE2 ||
                               itemProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_SWORD2);
        }

        // Check if the main hand currently has a 2H weapon equipped
        Item* currentMHItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        bool have2HWeaponEquipped = (currentMHItem && currentMHItem->GetTemplate()->GetInventoryType() == INVTYPE_2HWEAPON);

        // bool canDualWieldOrTG = (canDualWield || (canTitanGrip && isTwoHander));
        bool canDualWieldOrTG = (canDualWield || isTwoHander);

        // If this is a weapon and we can dual wield or Titan Grip, check if we can improve main/off-hand setup
        if (isWeapon && canDualWieldOrTG)
        {
            // Fetch current main hand and offhand items
            Item* mainHandItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
            Item* offHandItem  = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

            // Set up the stats calculator once and reuse results for performance
            StatsWeightCalculator calculator(bot);
            calculator.SetItemSetBonus(false);
            calculator.SetOverflowPenalty(false);

            // Calculate item scores once and store them
            float newItemScore = calculator.CalculateItem(itemId, item->GetItemRandomPropertyId());
            float mainHandScore = mainHandItem
                ? calculator.CalculateItem(mainHandItem->GetTemplate()->GetId(), mainHandItem->GetItemRandomPropertyId()) : 0.0f;
            float offHandScore = offHandItem
                ? calculator.CalculateItem(offHandItem->GetTemplate()->GetId(), offHandItem->GetItemRandomPropertyId()) : 0.0f;

            // Determine where this weapon can go
            bool canGoMain = (invType == INVTYPE_WEAPON ||
                              invType == INVTYPE_WEAPONMAINHAND ||
                              isTwoHander);

            bool canTGOff = false;
            if (canTitanGrip && isTwoHander && isValidTGWeapon)
                canTGOff = true;

            bool canGoOff = (invType == INVTYPE_WEAPON ||
                             invType == INVTYPE_WEAPONOFFHAND ||
                             canTGOff);

            // Check if the main hand item can go to offhand if needed
            bool mainHandCanGoOff = false;
            if (mainHandItem)
            {
                const ItemTemplate* mhProto = mainHandItem->GetTemplate();
                bool mhIsValidTG = false;
                if (canTitanGrip && mhProto->GetInventoryType() == INVTYPE_2HWEAPON)
                {
                    mhIsValidTG = (mhProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_AXE2 ||
                                   mhProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_MACE2 ||
                                   mhProto->GetSubClass() == ITEM_SUBCLASS_WEAPON_SWORD2);
                }

                mainHandCanGoOff = (mhProto->GetInventoryType() == INVTYPE_WEAPON ||
                                    mhProto->GetInventoryType() == INVTYPE_WEAPONOFFHAND ||
                                    (mhProto->GetInventoryType() == INVTYPE_2HWEAPON && mhIsValidTG));
            }

            // Priority 1: Replace main hand if the new weapon is strictly better
            // and if conditions allow (e.g. no conflicting 2H logic)
            bool betterThanMH = (newItemScore > mainHandScore);
            // If a one-handed weapon is better, we can still use it instead of a two-handed weapon
            bool mhConditionOK = (invType != INVTYPE_2HWEAPON ||
                      (isTwoHander && !canTitanGrip) ||
                      (canTitanGrip && isValidTGWeapon));

            if (canGoMain && betterThanMH && mhConditionOK)
            {
                // Equip new weapon in main hand
                //By leewheel 2026-08-02: 改用WPPCompat::AutoEquipItemSlot(填充Inv.Items)——原直接构造包+Read()从空包读InvUpdate导致Inv.Items为空,装备包被服务器静默拒绝
                {
                    ObjectGuid newItemGuid = item->GetGUID();
                    WPPCompat::AutoEquipItemSlot(bot->GetSession(), newItemGuid, EQUIPMENT_SLOT_MAINHAND);
                }
                //End By leewheel

                // Try moving old main hand weapon to offhand if beneficial
                if (mainHandItem && mainHandCanGoOff && (!offHandItem || mainHandScore > offHandScore))
                {
                    const ItemTemplate* oldMHProto = mainHandItem->GetTemplate();

                    ObjectGuid oldMHGuid = mainHandItem->GetGUID();
                    WPPCompat::AutoEquipItemSlot(bot->GetSession(), oldMHGuid, EQUIPMENT_SLOT_OFFHAND);

                    std::ostringstream moveMsg;
                    //By leewheel 2026-08-01: 玩家可见文本中文化
                    moveMsg << "发现主手可升级，正在将 " << chat->FormatItem(oldMHProto) << " 移动到副手";
                    //End By leewheel
                    botAI->TellMaster(moveMsg);
                }

                std::ostringstream out;
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << "正在装备 " << chat->FormatItem(itemProto) << " 到主手";
                //End By leewheel
                botAI->TellMaster(out);
                return;
            }

            // Priority 2: If not better than main hand, check if better than offhand
            else if (canGoOff && newItemScore > offHandScore)
            {
                // Equip in offhand
                //By leewheel 2026-08-02: 改用WPPCompat::AutoEquipItemSlot(填充Inv.Items)——原直接构造包+Read()导致Inv.Items为空,装备包被服务器静默拒绝
                ObjectGuid newItemGuid = item->GetGUID();
                WPPCompat::AutoEquipItemSlot(bot->GetSession(), newItemGuid, EQUIPMENT_SLOT_OFFHAND);
                //End By leewheel

                std::ostringstream out;
                //By leewheel 2026-08-01: 玩家可见文本中文化
                out << "正在装备 " << chat->FormatItem(itemProto) << " 到副手";
                //End By leewheel
                botAI->TellMaster(out);
                return;
            }
            else
            {
                // No improvement, do nothing
                return;
            }
        }

        // If not a special dual-wield/TG scenario or no improvement found, fall back to original logic
        if (dstSlot == EQUIPMENT_SLOT_FINGER1 ||
            dstSlot == EQUIPMENT_SLOT_TRINKET1 ||
            (dstSlot == EQUIPMENT_SLOT_MAINHAND && canDualWield &&
                ((invType != INVTYPE_2HWEAPON && !have2HWeaponEquipped) || (canTitanGrip && isValidTGWeapon))))
        {
            // Handle ring/trinket dual-slot logic
            Item* const equippedItems[2] = {
                bot->GetItemByPos(INVENTORY_SLOT_BAG_0, dstSlot),
                bot->GetItemByPos(INVENTORY_SLOT_BAG_0, dstSlot + 1)
            };

            if (equippedItems[0])
            {
                if (equippedItems[1])
                {
                    // Both slots are full - pick the worst item to replace, but only if new item is better
                    StatsWeightCalculator calc(bot);
                    calc.SetItemSetBonus(false);
                    calc.SetOverflowPenalty(false);

                    // Calculate new item score with random properties
                    int32 newItemRandomProp = item->GetItemRandomPropertyId();
                    float newItemScore = calc.CalculateItem(itemId, newItemRandomProp);

                    // Calculate equipped items scores with random properties
                    int32 firstRandomProp = equippedItems[0]->GetItemRandomPropertyId();
                    int32 secondRandomProp = equippedItems[1]->GetItemRandomPropertyId();
                    float firstItemScore = calc.CalculateItem(equippedItems[0]->GetTemplate()->GetId(), firstRandomProp);
                    float secondItemScore = calc.CalculateItem(equippedItems[1]->GetTemplate()->GetId(), secondRandomProp);

                    // Determine which slot (if any) should be replaced
                    bool betterThanFirst = newItemScore > firstItemScore;
                    bool betterThanSecond = newItemScore > secondItemScore;

                    // Early return if new item is not better than either equipped item
                    if (!betterThanFirst && !betterThanSecond)
                        return;

                    if (betterThanFirst && betterThanSecond)
                    {
                        // New item is better than both - replace the worse of the two equipped items
                        if (firstItemScore > secondItemScore)
                            dstSlot++; // Replace second slot (worse)
                        // else: keep dstSlot as-is (replace first slot)
                    }
                    else if (betterThanSecond)
                        dstSlot++; // Only better than second slot - replace it
                }
                else
                {
                    // Second slot empty, use it
                    dstSlot++;
                }
            }
        }

        //By leewheel 2026-07-12: 使用WPPCompat替代手动构造WorldPacket(TC的包结构不同)
        {
            ObjectGuid itemguid = item->GetGUID();
            WPPCompat::AutoEquipItemSlot(bot->GetSession(), itemguid, dstSlot);
        }
        //End By leewheel
    }

    std::ostringstream out;
    //By leewheel 2026-08-01: 玩家可见文本中文化
    out << "正在装备 " << chat->FormatItem(itemProto);
    //End By leewheel
    botAI->TellMaster(out);
}

ItemIds EquipAction::SelectInventoryItemsToEquip()
{
    CollectItemsVisitor visitor;
    IterateItems(&visitor, ITERATE_ITEMS_IN_BAGS);

    ItemIds items;
    for (auto i = visitor.items.begin(); i != visitor.items.end(); ++i)
    {
        Item* item = *i;
        if (!item)
            continue;

        ItemTemplate const* itemTemplate = item->GetTemplate();
        if (!itemTemplate)
            continue;

        //TODO Expand to Glyphs and Gems, that can be placed in equipment
        //Pre-filter non-equipable items
        if (itemTemplate->GetInventoryType() == INVTYPE_NON_EQUIP)
            continue;

        int32 randomProperty = item->GetItemRandomPropertyId();
        uint32 itemId = item->GetTemplate()->GetId();
        std::string itemUsageParam;
        if (randomProperty != 0)
            itemUsageParam = std::to_string(itemId) + "," + std::to_string(randomProperty);
        else
            itemUsageParam = std::to_string(itemId);

        ItemUsage usage = AI_VALUE2(ItemUsage, "item upgrade", itemUsageParam);
        if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE || usage == ITEM_USAGE_BAD_EQUIP)
        {
            //By leewheel 2026-08-02: 严谨化装备——背包中与已装备物品同id的副本不重复装备
            //(如bot已装备一件信仰手套22517, 背包还有第二件——不选第二件, 防止反复"正在装备"刷屏占AI)
            uint8 dstSlot = botAI->FindEquipSlot(itemTemplate, NULL_SLOT, true);
            if (dstSlot != NULL_SLOT)
            {
                if (Item* equipped = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, dstSlot))
                {
                    if (equipped->GetTemplate()->GetId() == itemId)
                        continue;
                }
            }
            //End By leewheel
            items.insert(itemId);
        }
    }
    return items;
}

bool EquipUpgradesPacketAction::Execute(Event event)
{
    if (!sPlayerbotAIConfig.autoEquipUpgradeLoot && !sRandomPlayerbotMgr.IsRandomBot(bot))
        return false;
    std::string const source = event.GetSource();
    if (source == "trade status")
    {
        WorldPacket p(event.getPacket());
        //By leewheel 2026-08-04: 修复——TC的SMSG_TRADE_STATUS是bit-packed包,与AC扁平格式完全不同
        //AC格式: uint32 status (扁平)
        //TC格式: WriteBit(PartnerIsSameBnetAccount) + WriteBits(Status, 5) + 根据Status分支写不同字段
        //原代码 p.rpos(0) + p >> status(uint32) 读到的是bit-packed头部后的字节流,不是Status
        //修复: 按TC bit-packed格式读取Status(跳过PartnerIsSameBnetAccount bit)
        //包体最小: 1bit + 5bits = 6bits, 对齐后1字节
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 1)
            return false;

        p.ReadBit();  // PartnerIsSameBnetAccount, 业务不需要
        uint32 status = p.ReadBits(5);
        p.ResetBitPos();
        //End By leewheel

        if (status != TRADE_STATUS_TRADE_ACCEPT)
            return false;
    }

    else if (source == "item push result")
    {
        WorldPacket p(event.getPacket());
        //By leewheel 2026-08-04: 修复——TC的SMSG_ITEM_PUSH_RESULT格式与AC完全不同
        //AC格式: playerGuid(8)+received(4)+created(4)+sendChatMessage(4)+bagSlot(1)+itemSlot(4)+itemId(4)
        //TC格式: PlayerGUID(8)+Slot(1)+SlotInBag(4)+QuestLogItemID(4)+Quantity(4)+QuantityInInventory(4)
        //        +DungeonEncounterID(4)+BattlePetSpeciesID(4)+BattlePetBreedID(4)+BattlePetBreedQuality(4)
        //        +BattlePetLevel(4)+ItemGUID(8)+bits...+Item(ItemInstance)
        //原代码按AC格式读7字段29字节,在TC下全部错位,itemId读到的是Quantity等垃圾值
        //修复: 按TC格式读取,跳过前41字节到ItemGUID,用bot->GetItemByGuid反查itemId
        //包体最小: 8+1+4*9+8 = 53字节(到ItemGUID结束)
        if (p.wpos() < p.rpos() || (p.wpos() - p.rpos()) < 53)
            return false;

        ObjectGuid playerGuid;
        p >> playerGuid;  // 跳过
        uint8 slot;
        p >> slot;  // 跳过
        int32 slotInBag, questLogItemId, quantity, quantityInInventory, dungeonEncounterId;
        int32 battlePetSpeciesId, battlePetBreedId, battlePetLevel;
        uint32 battlePetBreedQuality;
        p >> slotInBag >> questLogItemId >> quantity >> quantityInInventory;
        p >> dungeonEncounterId >> battlePetSpeciesId >> battlePetBreedId;
        p >> battlePetBreedQuality >> battlePetLevel;

        ObjectGuid itemGuid;
        p >> itemGuid;  // 物品GUID,用于反查itemId

        // 用ItemGUID反查Item,获取itemId
        uint32 itemId = 0;
        if (Item* item = bot->GetItemByGuid(itemGuid))
            itemId = item->GetEntry();
        //End By leewheel

        ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);
        //By leewheel 2026-07-14: 添加空指针保护防止ACCESS_VIOLATION崩溃
        if (!item || item->GetInventoryType() == INVTYPE_NON_EQUIP)
        //End By leewheel
            return false;
    }

    ItemIds items = SelectInventoryItemsToEquip();
    EquipItems(items);
    //By leewheel 2026-08-02: 装备后设置1秒冷却,避免每次tick反复触发占满AI
    //现象: NonCombatStrategy默认行动每tick执行"equip upgrades packet action", 背包有可升级装备时
    //      bot反复"正在装备"占满AI决策, /p 集合等命令无法及时执行(bot等在门口不过来)
    botAI->SetNextCheckDelay(1000);
    //End By leewheel
    return true;
}

bool EquipUpgradeAction::Execute(Event /*event*/)
{
    ItemIds items = SelectInventoryItemsToEquip();
    EquipItems(items);
    //By leewheel 2026-08-02: 同EquipUpgradesPacketAction——装备后1秒冷却,避免反复占满AI
    botAI->SetNextCheckDelay(1000);
    //End By leewheel
    return true;
}
