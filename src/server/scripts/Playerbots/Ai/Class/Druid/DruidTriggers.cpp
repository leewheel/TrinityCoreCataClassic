/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "DruidTriggers.h"
#include "DynamicObject.h"
#include "Player.h"
#include "Playerbots.h"
#include "ServerFacade.h"

//By leewheel 2026-08-06: Druid buff spell ID数组,防止locale匹配失败导致重复加buff/重复切形态
//By leewheel 2026-08-07: 通过 classic_db2.spellname 复核修正, 原数组漏了多个 MotW/GotW 高等级rank, Thorns 也漏多个版本
namespace {
// Mark of the Wild 全等级 + Gift of the Wild 群体版 (依据 classic_db2.spellname 实际查得)
constexpr uint32 MARK_OF_THE_WILD_SPELL_IDS[] = {
    // Mark of the Wild rank 1-12
    1126, 5232, 5234, 6756, 8907, 9884, 9885, 16878, 24752, 26990, 39233, 48469,
    // Gift of the Wild rank 1-6
    21849, 21850, 26991, 48470, 69381, 72588
};
// Thorns 全等级 (含 NPC/物品触发版本, 加上无害)
constexpr uint32 THORNS_SPELL_IDS[] = {
    467, 782, 1075, 8914, 9756, 9910, 15438, 16877, 21335, 21337, 22128, 22351,
    22696, 25640, 25777, 26992, 31271, 33907, 34343, 34663, 35361, 43420, 53307, 66068
};
// Bear Form + Dire Bear Form (玩家可学版本, 其他 NPC 同名 spell 不影响)
constexpr uint32 BEAR_FORM_SPELL_IDS[] = { 5487, 9634 };
// Cat Form
constexpr uint32 CAT_FORM_SPELL_ID = 768;
// Aquatic Form
constexpr uint32 AQUATIC_FORM_SPELL_ID = 1066;

bool HasAnyAuraFromList(Unit* unit, const uint32* spellIds, size_t count)
{
    if (!unit) return false;
    for (size_t i = 0; i < count; ++i)
        if (unit->HasAura(spellIds[i])) return true;
    return false;
}
} // namespace
//End By leewheel

//By leewheel 2026-08-06: 添加spell ID fallback
bool MarkOfTheWildTrigger::IsActive()
{
    if (!BuffTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    if (botAI->HasAura("gift of the wild", target))
        return false;
    if (HasAnyAuraFromList(target, MARK_OF_THE_WILD_SPELL_IDS, std::size(MARK_OF_THE_WILD_SPELL_IDS)))
        return false;
    return true;
}

bool ThornsOnPartyTrigger::IsActive()
{
    if (!BuffOnPartyTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    if (botAI->HasAura("thorns", target))
        return false;
    if (HasAnyAuraFromList(target, THORNS_SPELL_IDS, std::size(THORNS_SPELL_IDS)))
        return false;
    return true;
}

bool EntanglingRootsKiteTrigger::IsActive()
{
    return DebuffTrigger::IsActive() && AI_VALUE(uint8, "attacker count") < 3 && !GetTarget()->GetPower(POWER_MANA);
}

bool ThornsTrigger::IsActive()
{
    if (!BuffTrigger::IsActive())
        return false;
    Unit* target = GetTarget();
    if (botAI->HasAura("thorns", target))
        return false;
    if (HasAnyAuraFromList(target, THORNS_SPELL_IDS, std::size(THORNS_SPELL_IDS)))
        return false;
    return true;
}

bool BearFormTrigger::IsActive()
{
    if (!botAI->HasAnyAuraOf(bot, "bear form", "dire bear form", nullptr))
        return !HasAnyAuraFromList(bot, BEAR_FORM_SPELL_IDS, std::size(BEAR_FORM_SPELL_IDS));
    return false;
}

bool TreeFormTrigger::IsActive()
{
    constexpr uint32 SPELL_TREE_OF_LIFE = 33891;
    return !bot->HasAura(SPELL_TREE_OF_LIFE);
}

bool CatFormTrigger::IsActive()
{
    if (!botAI->HasAura("cat form", bot))
        return !bot->HasAura(CAT_FORM_SPELL_ID);
    return false;
}

bool AquaticFormTrigger::IsActive()
{
    //By leewheel 2026-07-10: TC中IsInWater和IsUnderWater判断是否在水下
    if (bot->IsInCombat() || !bot->IsInWater() || !bot->IsUnderWater())
        return false;
    if (botAI->HasAura("aquatic form", bot))
        return false;
    return !bot->HasAura(AQUATIC_FORM_SPELL_ID);
    //End By leewheel
}
//End By leewheel

bool ProwlTrigger::IsActive()
{
    if (botAI->HasAura("prowl", bot) || bot->IsInCombat())
        return false;

    if (!botAI->HasSpell("prowl"))
        return false;

    uint32 const prowlId = AI_VALUE2(uint32, "spell id", "prowl");
    if (bot->HasSpellCooldown(prowlId))
        return false;

    float distance = 30.f;

    Unit* target = AI_VALUE(Unit*, "enemy player target");
    if (target && !target->IsInWorld())
        return false;
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

const std::set<uint32> HurricaneChannelCheckTrigger::HURRICANE_SPELL_IDS = {
    16914,  // Hurricane Rank 1
    17401,  // Hurricane Rank 2
    17402,  // Hurricane Rank 3
    27012,  // Hurricane Rank 4
    48467   // Hurricane Rank 5
};

bool HurricaneChannelCheckTrigger::IsActive()
{
    if (Spell* spell = bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
    {
        if (!HURRICANE_SPELL_IDS.count(spell->m_spellInfo->Id))
            return false;

        // Find this bot's own Hurricane DynamicObject
        DynamicObject* dynObj = nullptr;
        for (uint32 spellId : HURRICANE_SPELL_IDS)
        {
            dynObj = bot->GetDynObject(spellId);
            if (dynObj)
                break;
        }

        if (!dynObj)
            return false;

        // Count attackers actually inside the Hurricane AoE
        float radius = dynObj->GetRadius();
        GuidVector attackers = AI_VALUE(GuidVector, "attackers");
        uint32 count = 0;
        for (ObjectGuid const& guid : attackers)
        {
            Unit* unit = botAI->GetUnit(guid);
            if (!unit || !unit->IsAlive())
                continue;
            if (unit->GetDistance(dynObj->GetPosition()) <= radius)
                count++;
        }

        return count < minEnemies;
    }

    return false;
}
