/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "BudgetValues.h"

#include "Playerbots.h"

uint32 MaxGearRepairCostValue::Calculate()
{
    uint32 totalCost = 0;

    for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        uint16 pos = ((INVENTORY_SLOT_BAG_0 << 8) | i);
        Item* item = bot->GetItemByPos(pos);

        if (!item)
            continue;

        uint32 maxDurability = item->GetMaxDurability();
        if (!maxDurability)
            continue;

        uint32 curDurability = item->GetDurability();

        if (uint32(i) >= EQUIPMENT_SLOT_END && curDurability >= maxDurability)  // Only count items equiped or already damanged.
            continue;

        ItemTemplate const* ditemProto = item->GetTemplate();

        DurabilityCostsEntry const* dcost = sDurabilityCostsStore.LookupEntry(ditemProto->GetItemLevel());
        if (!dcost)
            continue;

        uint32 dQualitymodEntryId = (ditemProto->GetQuality() + 1) * 2;
        DurabilityQualityEntry const* dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
        if (!dQualitymodEntry)
            continue;

        // AC兼容: DurabilityCostsEntry在TC中没有multiplier数组, 需要根据物品类型选择对应的子类费用数组
        uint32 dmultiplier = 0;
        if (ditemProto->GetClass() == ITEM_CLASS_WEAPON)
        {
            if (ditemProto->GetSubClass() < dcost->WeaponSubClassCost.size())
                dmultiplier = dcost->WeaponSubClassCost[ditemProto->GetSubClass()];
        }
        else
        {
            if (ditemProto->GetSubClass() < dcost->ArmorSubClassCost.size())
                dmultiplier = dcost->ArmorSubClassCost[ditemProto->GetSubClass()];
        }

        // AC兼容: DurabilityQualityEntry在TC中quality_mod改名为Data
        uint32 costs = uint32(maxDurability * dmultiplier * double(dQualitymodEntry->Data));

        totalCost += costs;
    }

    return totalCost;
}

uint32 RepairCostValue::Calculate()
{
    uint32 totalCost = 0;

    for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        uint16 pos = ((INVENTORY_SLOT_BAG_0 << 8) | i);
        Item* item = bot->GetItemByPos(pos);

        if (!item)
            continue;

        uint32 maxDurability = item->GetMaxDurability();
        if (!maxDurability)
            continue;

        uint32 curDurability = item->GetDurability();

        uint32 LostDurability = maxDurability - curDurability;

        if (LostDurability == 0)
            continue;

        ItemTemplate const* ditemProto = item->GetTemplate();

        DurabilityCostsEntry const* dcost = sDurabilityCostsStore.LookupEntry(ditemProto->GetItemLevel());
        if (!dcost)
            continue;

        uint32 dQualitymodEntryId = (ditemProto->GetQuality() + 1) * 2;
        DurabilityQualityEntry const* dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
        if (!dQualitymodEntry)
            continue;

        // AC兼容: DurabilityCostsEntry在TC中没有multiplier数组, 需要根据物品类型选择对应的子类费用数组
        uint32 dmultiplier = 0;
        if (ditemProto->GetClass() == ITEM_CLASS_WEAPON)
        {
            if (ditemProto->GetSubClass() < dcost->WeaponSubClassCost.size())
                dmultiplier = dcost->WeaponSubClassCost[ditemProto->GetSubClass()];
        }
        else
        {
            if (ditemProto->GetSubClass() < dcost->ArmorSubClassCost.size())
                dmultiplier = dcost->ArmorSubClassCost[ditemProto->GetSubClass()];
        }

        // AC兼容: DurabilityQualityEntry在TC中quality_mod改名为Data
        uint32 costs = uint32(LostDurability * dmultiplier * double(dQualitymodEntry->Data));

        totalCost += costs;
    }

    return totalCost;
}

uint32 TrainCostValue::Calculate()
{
    uint32 totalCost = 0;

    std::unordered_set<uint32> spells;

    // AC兼容: TC的GetCreatureTemplates()返回引用而非指针
    CreatureTemplateContainer const& ctc = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc.begin(); itr != ctc.end(); ++itr)
    {
        if (!(itr->second.npcflag & UNIT_NPC_FLAG_TRAINER))
            continue;

        // AC兼容: TC的GetTrainer()返回const指针
        Trainer::Trainer const* trainer = sObjectMgr->GetTrainer(itr->first);
        if (!trainer)
            continue;

        //By leewheel 2025-07-10 / 2026-09-09: TC Trainer API兼容
        // TC使用GetType()代替GetTrainerType()；无IsTrainerValidForPlayer，由下方CanTeachSpell逐法术校验
        // Trainer::Type::Talent 对应AC的职业训练师
        if (trainer->GetType() != Trainer::Type::Talent)
        //End By leewheel
            continue;

        for (auto const& spell : trainer->GetSpells())
        {
            Trainer::Spell const* trainerSpell = trainer->GetSpell(spell.SpellId);
            if (!trainerSpell)
                continue;

            if (!trainer->CanTeachSpell(bot, trainerSpell))
                continue;

            if (spells.find(trainerSpell->SpellId) != spells.end())
                continue;

            totalCost += trainerSpell->MoneyCost;
            spells.insert(trainerSpell->SpellId);
        }
    }

    return totalCost;
}

uint32 MoneyNeededForValue::Calculate()
{
    NeedMoneyFor needMoneyFor = NeedMoneyFor(stoi(getQualifier()));

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    AiObjectContext* context = botAI->GetAiObjectContext();

    uint32 moneyWanted = 0;

    uint32 level = bot->GetLevel();

    switch (needMoneyFor)
    {
        case NeedMoneyFor::none:
            moneyWanted = 0;
            break;
        case NeedMoneyFor::repair:
            moneyWanted = AI_VALUE(uint32, "max repair cost");
            break;
        case NeedMoneyFor::ammo:
            moneyWanted = (bot->getClass() == CLASS_HUNTER)
                              ? (level * level * level) / 10
                              : 0;  // Or level^3 (1s @ lvl10, 30s @ lvl30, 2g @ lvl60, 5g @ lvl80): Todo replace
                                    // (should be best ammo buyable x 8 stacks cost)
            break;
        case NeedMoneyFor::spells:
            moneyWanted = AI_VALUE(uint32, "train cost");
            break;
        case NeedMoneyFor::travel:
            moneyWanted =
                bot->isTaxiCheater()
                    ? 0
                    : 1500;  // 15s for traveling half a continent. Todo: Add better calculation (Should be ???)
            break;
        case NeedMoneyFor::gear:
            moneyWanted = level * level * level;  // Or level^3 (10s @ lvl10, 3g @ lvl30, 20g @ lvl60, 50g @ lvl80):
                                                  // Todo replace (Should be ~total cost of all >green gear equiped)
            break;
        case NeedMoneyFor::consumables:
            moneyWanted =
                (level * level * level) / 10;  // Or level^3 (1s @ lvl10, 30s @ lvl30, 2g @ lvl60, 5g @ lvl80): Todo
                                               // replace (Should be best food/drink x 2 stacks cost)
            break;
        case NeedMoneyFor::guild:
            if (botAI->HasStrategy("guild", BOT_STATE_NON_COMBAT))
            {
                if (bot->GetGuildId())
                    moneyWanted = AI_VALUE2(uint32, "item count", chat->FormatQItem(5976)) ? 0 : 10000;  // 1g (tabard)
                else
                    moneyWanted =
                        AI_VALUE2(uint32, "item count", chat->FormatQItem(5863)) ? 0 : 10000;  // 10s (guild charter)
            }
            break;
        case NeedMoneyFor::tradeskill:
            moneyWanted = (level * level *
                           level);  // Or level^3 (10s @ lvl10, 3g @ lvl30, 20g @ lvl60, 50g @ lvl80): Todo replace
                                    // (Should be buyable reagents that combined allow crafting of usefull items)
            break;
        default:
            break;
    }

    return moneyWanted;
};

uint32 TotalMoneyNeededForValue::Calculate()
{
    NeedMoneyFor needMoneyFor = NeedMoneyFor(stoi(getQualifier()));

    uint32 moneyWanted = AI_VALUE2(uint32, "money needed for", (uint32)needMoneyFor);

    auto needPtr = std::find(saveMoneyFor.begin(), saveMoneyFor.end(), needMoneyFor);

    while (needPtr != saveMoneyFor.begin())
    {
        needPtr--;

        NeedMoneyFor alsoNeed = *needPtr;

        moneyWanted = moneyWanted + AI_VALUE2(uint32, "money needed for", (uint32)alsoNeed);
    }

    return moneyWanted;
}

uint32 FreeMoneyForValue::Calculate()
{
    uint32 money = bot->GetMoney();

    if (botAI->HasCheat(BotCheatMask::gold))
        return 10000000;

    if (botAI->HasActivePlayerMaster())
        return money;

    uint32 savedMoney = AI_VALUE2(uint32, "total money needed for", getQualifier()) -
                        AI_VALUE2(uint32, "money needed for", getQualifier());

    if (savedMoney > money)
        return 0;

    return money - savedMoney;
};

bool ShouldGetMoneyValue::Calculate() { return !AI_VALUE2(uint32, "free money for", (uint32)NeedMoneyFor::anything); };
