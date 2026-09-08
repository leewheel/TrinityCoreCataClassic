/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "WarriorActions.h"

#include "AiFactory.h"
#include "Playerbots.h"

namespace
{
constexpr uint32 SPELL_RETALIATION = 20230;
constexpr uint32 SPELL_DIVINE_SHIELD = 642;
constexpr uint32 SPELL_ICE_BLOCK = 45438;
constexpr uint32 SPELL_BLESSING_OF_PROTECTION = 41450;
constexpr uint32 SPELL_SHATTERING_THROW = 64382;

// By leewheel 2026-08-07: Warrior Actions spell ID fallback, 防止 locale 匹配失败导致技能判断错误
constexpr uint32 SPELL_BERSERKER_RAGE = 18499;
constexpr uint32 SPELL_VIGILANCE = 50720;
// Sunder Armor 全等级 (玩家可学版本)
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

// 通过 spell ID 查找 Sunder Armor aura (locale 匹配失败时的 fallback)
Aura* FindSunderArmorAuraBySpellId(Unit* unit)
{
    if (!unit) return nullptr;
    for (uint32 spellId : SUNDER_ARMOR_SPELL_IDS)
    {
        if (Aura* aura = unit->GetAura(spellId))
            return aura;
    }
    return nullptr;
}
}

bool CastBerserkerRageAction::isPossible()
{
    if (botAI->IsInVehicle() && !botAI->IsInVehicle(false, false, true))
        return false;

    uint32 spellId = AI_VALUE2(uint32, "spell id", spell);
    if (!spellId)
        return false;

    if (!bot->HasSpell(spellId))
        return false;

    if (bot->HasSpellCooldown(spellId))
        return false;

    return true;
}

bool CastBerserkerRageAction::isUseful()
{
    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放狂暴之怒
    return (bot->HasAuraType(SPELL_AURA_MOD_FEAR) ||
           bot->HasAuraWithMechanic(1 << MECHANIC_SLEEP) ||
           bot->HasAuraWithMechanic(1 << MECHANIC_SAPPED))
        && !botAI->HasAura("berserker rage", bot)
        && !bot->HasAura(SPELL_BERSERKER_RAGE)
        && CastSpellAction::isUseful();
}

bool CastSunderArmorAction::isUseful()
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    if (!botAI->IsTank(bot, false))
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member || member == bot || !member->IsAlive() || !member->IsInWorld() ||
                member->GetMapId() != bot->GetMapId())
            {
                continue;
            }

            if (member->getClass() == CLASS_WARRIOR &&
                botAI->IsTank(member, false))
                return false;
        }
    }

    // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放破甲
    Aura* aura = botAI->GetAura("sunder armor", GetTarget(), false, true);
    if (!aura)
        aura = FindSunderArmorAuraBySpellId(GetTarget());
    return !aura || aura->GetStackAmount() < 5 || aura->GetDuration() <= 6000;
}

Unit* CastVigilanceAction::GetTarget()
{
    Group* group = bot->GetGroup();
    if (!group)
    {
        return nullptr;
    }

    Player* currentVigilanceTarget = nullptr;
    Player* mainTank = nullptr;
    Player* assistTank1 = nullptr;
    Player* assistTank2 = nullptr;
    Player* highestGearScorePlayer = nullptr;
    uint32 highestGearScore = 0;

    // Iterate once through the group to gather all necessary information
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive())
            continue;

        // Check if member has Vigilance applied by the bot
        // By leewheel 2026-08-07: 名称检查 + spell ID fallback, 防止 locale 匹配失败导致重复施放警戒
        if (!currentVigilanceTarget && (botAI->HasAura("vigilance", member, false, true) || member->HasAura(SPELL_VIGILANCE)))
        {
            currentVigilanceTarget = member;
        }

        // Identify Main Tank
        if (!mainTank && botAI->IsMainTank(member))
        {
            mainTank = member;
        }

        // Identify Assist Tanks
        if (assistTank1 == nullptr && botAI->IsAssistTankOfIndex(member, 0))
        {
            assistTank1 = member;
        }
        else if (assistTank2 == nullptr && botAI->IsAssistTankOfIndex(member, 1))
        {
            assistTank2 = member;
        }

        // Determine Highest Gear Score
        uint32 gearScore = botAI->GetEquipGearScore(member/*, false, false*/);
        if (gearScore > highestGearScore)
        {
            highestGearScore = gearScore;
            highestGearScorePlayer = member;
        }
    }

    // Determine the highest-priority target
    Player* highestPriorityTarget = mainTank ? mainTank :
                                      (assistTank1 ? assistTank1 :
                                      (assistTank2 ? assistTank2 : highestGearScorePlayer));

    // If no valid target, return nullptr
    if (!highestPriorityTarget)
    {
        return nullptr;
    }

    // If the current target is already the highest-priority target, do nothing
    if (currentVigilanceTarget == highestPriorityTarget)
    {
        return nullptr;
    }

    // Assign the new target
    Unit* targetUnit = highestPriorityTarget->ToUnit();
    if (targetUnit)
    {
        return targetUnit;
    }

    return nullptr;
}

bool CastVigilanceAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target || target == bot)
        return false;

    return botAI->CastSpell("vigilance", target);
}

bool CastRetaliationAction::isUseful()
{
    if (!bot->HasSpell(SPELL_RETALIATION) || bot->HasSpellCooldown(SPELL_RETALIATION) ||
        bot->HasAura(SPELL_RETALIATION))
    {
        return false;
    }

    uint8 meleeAttackers = 0;
    GuidVector attackers = AI_VALUE(GuidVector, "attackers");

    for (ObjectGuid const& guid : attackers)
    {
        Unit* attacker = botAI->GetUnit(guid);
        if (!attacker || !attacker->IsAlive() || attacker->GetVictim() != bot)
            continue;

        // Check if the attacker is melee-based using unit_class
        if (attacker->IsCreature())
        {
            Creature* creature = attacker->ToCreature();
    //By leewheel 2026-07-10: TC没有IsClass方法，使用getClass()替代
    if (creature && (creature->getClass() == CLASS_WARRIOR
        || creature->getClass() == CLASS_ROGUE
        || creature->getClass() == CLASS_PALADIN))
    //End By leewheel
            {
                ++meleeAttackers;
            }
        }
        else if (attacker->IsPlayer())
        {
            Player* playerAttacker = attacker->ToPlayer();
            if (playerAttacker && botAI->IsMelee(playerAttacker)) // Reuse existing Player melee check
            {
                ++meleeAttackers;
            }
        }

        // Early exit if we already have enough melee attackers
        if (meleeAttackers >= 2)
            break;
    }

    // Only cast Retaliation if there are at least 2 melee attackers
    return meleeAttackers >= 2;
}

Unit* CastShatteringThrowAction::GetTarget()
{
    GuidVector enemies = AI_VALUE(GuidVector, "possible targets");

    for (ObjectGuid const& guid : enemies)
    {
        Unit* enemy = botAI->GetUnit(guid);
        if (!enemy || !enemy->IsAlive() || enemy->IsFriendlyTo(bot))
            continue;

        if (bot->IsWithinDistInMap(enemy, 25.0f) &&
            (enemy->HasAura(SPELL_DIVINE_SHIELD) ||
             enemy->HasAura(SPELL_ICE_BLOCK) ||
             enemy->HasAura(SPELL_BLESSING_OF_PROTECTION)))
        {
            return enemy;
        }
    }

    return nullptr;
}

bool CastShatteringThrowAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    return botAI->CastSpell("shattering throw", target);
}

bool CastShatteringThrowAction::isUseful()
{
    if (!bot->HasSpell(SPELL_SHATTERING_THROW) || bot->HasSpellCooldown(SPELL_SHATTERING_THROW))
        return false;

    GuidVector enemies = AI_VALUE(GuidVector, "possible targets");

    for (ObjectGuid const& guid : enemies)
    {
        Unit* enemy = botAI->GetUnit(guid);
        if (!enemy || !enemy->IsAlive() || enemy->IsFriendlyTo(bot))
            continue;

        if (bot->IsWithinDistInMap(enemy, 25.0f) &&
            (enemy->HasAura(SPELL_DIVINE_SHIELD) ||
             enemy->HasAura(SPELL_ICE_BLOCK) ||
             enemy->HasAura(SPELL_BLESSING_OF_PROTECTION)))
        {
            return true;
        }
    }

    return false;
}

bool CastShatteringThrowAction::isPossible()
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    // Range check: Shattering Throw is 30 yards
    if (!bot->IsWithinDistInMap(target, 30.0f))
        return false;

    // Check line of sight
    if (!bot->IsWithinLOSInMap(target))
        return false;

    // If the minimal checks above pass, simply return true.
    return true;
}
