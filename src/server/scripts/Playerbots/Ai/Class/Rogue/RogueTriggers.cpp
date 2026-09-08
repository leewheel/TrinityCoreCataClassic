/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "RogueTriggers.h"

#include "GenericTriggers.h"
#include "Playerbots.h"
#include "ServerFacade.h"

namespace
{
constexpr uint32 SPELL_STEALTH = 1784;
constexpr uint32 SPELL_SPRINT_RANK_1 = 2983;

// By leewheel 2026-08-07: Stealth 全等级 spell ID (依据 classic_db2.spelllevels 玩家可学版本)
// 用于 locale 匹配失败时的 spell ID fallback, 防止潜行状态判断错误
constexpr uint32 STEALTH_SPELL_IDS[] = { 1784, 1785, 1786, 1787, 32199 };
// Sunder Armor 全等级 spell ID (用于 ExposeArmorTrigger 检查目标是否已有破甲)
constexpr uint32 SUNDER_ARMOR_SPELL_IDS[] = {
    7386, 7405, 8380, 11596, 11597, 24317, 25225, 47467, 58567
};

bool HasAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
}

// bool AdrenalineRushTrigger::isPossible()
// {
//     return !botAI->HasAura("stealth", bot);
// }

bool UnstealthTrigger::IsActive()
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致潜行状态判断错误
    if (!botAI->HasAura("stealth", bot) && !HasAuraFromList(bot, STEALTH_SPELL_IDS, std::size(STEALTH_SPELL_IDS)))
        return false;

    return (botAI->HasAura("stealth", bot) || HasAuraFromList(bot, STEALTH_SPELL_IDS, std::size(STEALTH_SPELL_IDS))) && !AI_VALUE(uint8, "attacker count") &&
           (AI_VALUE2(bool, "moving", "self target") &&
            ((botAI->GetMaster() &&
              ServerFacade::instance().IsDistanceGreaterThan(AI_VALUE2(float, "distance", "group leader"), 10.0f) &&
              AI_VALUE2(bool, "moving", "group leader")) ||
             !AI_VALUE(uint8, "attacker count")));
}

bool StealthTrigger::IsActive()
{
    if (bot->HasAura(SPELL_STEALTH) || bot->IsInCombat() || bot->HasSpellCooldown(SPELL_STEALTH))
        return false;

    float distance = 30.f;

    Unit* target = AI_VALUE(Unit*, "enemy player target");
    if (target && !target->IsInWorld())
    {
        return false;
    }
    if (!target)
        target = AI_VALUE(Unit*, "grind target");

    if (!target)
        target = AI_VALUE(Unit*, "dps target");

    if (!target)
        return false;

    if (target && target->GetVictim())
        distance -= 10;

    if (target->isMoving() && target->GetVictim())
        distance -= 10;

    if (bot->InBattleground())
        distance += 15;

    if (bot->InArena())
        distance += 15;

    return target && ServerFacade::instance().GetDistance2d(bot, target) < distance;
}

bool SapTrigger::IsPossible() { return bot->GetLevel() > 10 && botAI->HasSpell("sap") && !bot->IsInCombat(); }

bool SprintTrigger::IsPossible() { return bot->HasSpell(SPELL_SPRINT_RANK_1); }

bool SprintTrigger::IsActive()
{
    if (bot->HasSpellCooldown(SPELL_SPRINT_RANK_1))
        return false;

    float distance = botAI->GetMaster() ? 45.0f : 35.0f;
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback
    if ((botAI->HasAura("stealth", bot) || HasAuraFromList(bot, STEALTH_SPELL_IDS, std::size(STEALTH_SPELL_IDS))))
        distance -= 10;

    bool targeted = false;

    Unit* dps = AI_VALUE(Unit*, "dps target");
    Unit* enemyPlayer = AI_VALUE(Unit*, "enemy player target");

    if (enemyPlayer && !enemyPlayer->IsInWorld())
    {
        return false;
    }
    if (dps)
        targeted = (dps == AI_VALUE(Unit*, "current target"));

    if (enemyPlayer && !targeted)
        targeted = (enemyPlayer == AI_VALUE(Unit*, "current target"));

    if (!targeted)
        return false;

    if ((dps && dps->IsInCombat()) || enemyPlayer)
        distance -= 10;

    return AI_VALUE2(bool, "moving", "self target") &&
           (AI_VALUE2(bool, "moving", "dps target") || AI_VALUE2(bool, "moving", "enemy player target")) && targeted &&
           (ServerFacade::instance().IsDistanceGreaterThan(AI_VALUE2(float, "distance", "dps target"), distance) ||
            ServerFacade::instance().IsDistanceGreaterThan(AI_VALUE2(float, "distance", "enemy player target"), distance));
}

bool ExposeArmorTrigger::IsActive()
{
    Unit* target = AI_VALUE(Unit*, "current target");
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放破甲
    bool hasSunderByName = botAI->HasAura("sunder armor", target, false, false, -1, true);
    bool hasSunderById = !hasSunderByName && HasAuraFromList(target, SUNDER_ARMOR_SPELL_IDS, std::size(SUNDER_ARMOR_SPELL_IDS));
    return DebuffTrigger::IsActive() && !hasSunderByName && !hasSunderById &&
           AI_VALUE2(uint8, "combo", "current target") <= 3;
}

bool MainHandWeaponNoEnchantTrigger::IsActive()
{
    Item* const itemForSpell = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    if (!itemForSpell || itemForSpell->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
        return false;
    return true;
}

bool OffHandWeaponNoEnchantTrigger::IsActive()
{
    Item* const itemForSpell = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);
    if (!itemForSpell || itemForSpell->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
        return false;
    return true;
}
