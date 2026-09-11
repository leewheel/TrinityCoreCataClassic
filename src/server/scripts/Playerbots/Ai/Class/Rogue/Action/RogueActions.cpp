/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RogueActions.h"

#include "Event.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

namespace
{
constexpr uint32 BG_WS_SPELL_WARSONG_FLAG = 23333;
constexpr uint32 BG_WS_SPELL_SILVERWING_FLAG = 23335;
constexpr uint32 BG_EY_NETHERSTORM_FLAG_SPELL = 34976;
constexpr uint32 SPELL_MASTER_POISONER_RANK_3 = 58410;

// By leewheel 2026-08-07: Stealth 全等级 (玩家可学版本), 用于 locale 匹配失败时的 spell ID fallback
constexpr uint32 STEALTH_SPELL_IDS[] = { 1784, 1785, 1786, 1787, 32199 };

bool HasAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
}

bool CastStealthAction::isUseful()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    if (target && bot->GetDistance(target) >= sPlayerbotAIConfig.spellDistance)
        return false;
    return true;
}

bool CastStealthAction::isPossible()
{
    // do not use with WSG flag or EYE flag
    return !bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) && !bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG) &&
           !bot->HasAura(BG_EY_NETHERSTORM_FLAG_SPELL);
}

bool UnstealthAction::Execute(Event /*event*/)
{
    botAI->RemoveAura("stealth");
    // botAI->ChangeStrategy("+dps,-stealthed", BOT_STATE_COMBAT);

    return true;
}

bool CheckStealthAction::Execute(Event /*event*/)
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致潜行策略切换错误
    if (botAI->HasAura("stealth", bot) || HasAuraFromList(bot, STEALTH_SPELL_IDS, std::size(STEALTH_SPELL_IDS)))
    {
        botAI->ChangeStrategy("-dps,+stealthed", BOT_STATE_COMBAT);
    }
    else
    {
        botAI->ChangeStrategy("+dps,-stealthed", BOT_STATE_COMBAT);
    }

    return true;
}

bool CastVanishAction::isUseful()
{
    // do not use with WSG flag or EYE flag
    return !bot->HasAura(BG_WS_SPELL_WARSONG_FLAG) && !bot->HasAura(BG_WS_SPELL_SILVERWING_FLAG) &&
           !bot->HasAura(BG_EY_NETHERSTORM_FLAG_SPELL);
}

bool CastEnvenomAction::isUseful()
{
    return AI_VALUE2(uint8, "energy", "self target") >= 35;
}

bool CastEnvenomAction::isPossible()
{
    // alternate to eviscerate if talents unlearned
    return bot->HasAura(SPELL_MASTER_POISONER_RANK_3);
}

bool CastTricksOfTheTradeOnMainTankAction::isUseful()
{
    return CastSpellAction::isUseful() && AI_VALUE2(float, "distance", GetTargetName()) < 20.0f;
}

//By leewheel 2026-09-05: 上游d33f3592——毒药查找由逐后缀轮询改为按基名一次性匹配+
//物品类别过滤,消除多次背包扫描;物品列表按等级排序所以首个匹配即最高级毒药
bool UseDeadlyPoisonAction::Execute(Event /*event*/)
{
    std::vector<Item*> const items =
        AI_VALUE2(std::vector<Item*>, "inventory items", "Deadly Poison");
    for (Item* const item : items)
    {
        // 名称子串匹配可能命中非消耗品(如“致命毒药手册”),需按类别过滤
        if (item->GetTemplate()->GetClass() != ITEM_CLASS_CONSUMABLE)
            continue;

        Item* const itemForSpell = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        return UseItem(item, ObjectGuid::Empty, itemForSpell);
    }

    return false;
}

bool UseInstantPoisonAction::Execute(Event /*event*/)
{
    std::vector<Item*> const items =
        AI_VALUE2(std::vector<Item*>, "inventory items", "Instant Poison");
    for (Item* const item : items)
    {
        if (item->GetTemplate()->GetClass() != ITEM_CLASS_CONSUMABLE)
            continue;

        Item* const itemForSpell = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
        return UseItem(item, ObjectGuid::Empty, itemForSpell);
    }

    return false;
}

bool UseInstantPoisonOffHandAction::Execute(Event /*event*/)
{
    std::vector<Item*> const items =
        AI_VALUE2(std::vector<Item*>, "inventory items", "Instant Poison");
    for (Item* const item : items)
    {
        if (item->GetTemplate()->GetClass() != ITEM_CLASS_CONSUMABLE)
            continue;

        Item* const itemForSpell = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
        return UseItem(item, ObjectGuid::Empty, itemForSpell);
    }

    return false;
}
//End By leewheel
